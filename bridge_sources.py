"""Generate src/bridge/: byte-identical matched functions compiled into the playable build.

    uv run bridge_sources.py            regenerate src/bridge/ and its manifest units and pins
    uv run bridge_sources.py --check    exit 1 if a generated file or bridge unit is missing, differs or is stale

This is phase 2 of STRUCTURE.md. For every address the spec names, the matched
source (matching/src, status MATCH) is sliced to the preamble its file declares
plus the selected functions, with its matching/include headers inlined. The
slice sits in a namespace of its own, so none of its classes, inline functions
or vtables can merge with another unit's copy. It adds one extern "C" entry per
function, named kotor_fastcall_<address> as the playable build names original
thiscall functions.

What a slice uses from outside its unit is bound explicitly:
  calls    the forwards its spec entry names, defined by calling the playable
           build's entry for the same address, after checking that entry's
           parameter count and return against the original's signature;
  globals  each matched extern global the slice uses becomes a reference to the
           playable build's storage for the same original address, found by
           scanning the pinned sources (cpp_scope.py) and checked for size;
  vtables  a bridged constructor or destructor re-points the object at the
           playable build's table for the class's original vftable address.
Anything else the slice uses stays unresolved and fails the link, and each
generated unit lists what it bound and what it could not.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import re
import sys
from dataclasses import dataclass
from functools import cached_property
from pathlib import Path

import cpp_scope
import pin_sources

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "src" / "bridge"
SPEC = OUT / "bridge.json"
BRIDGE_PREFIX = "src/bridge/"
# The manifest's predominant MSVC recipe; the matched slices compile warning-free under it.
RECIPE = ["/nologo", "/O2", "/Ob0", "/Oy-", "/GS", "/EHsc", "/MT", "/W4", "/WX", "/std:c++17", "/showIncludes",
          "/D_CRT_SECURE_NO_WARNINGS"]
MATCHING = ROOT / "matching"
INCLUDE = MATCHING / "include"
SIGNATURE = re.compile(r"^public: (?:virtual )?(?:(?P<ret>.+?) )?__thiscall (?P<owner>.+)::(?P<name>~?\w+)"
                       r"\((?P<params>.*)\)(?P<const>const)?\s*$")
ANNOTATION = re.compile(r"^// FUNCTION: SWKOTOR 0x([0-9A-Fa-f]{8})\b")
DIRECTIVE = re.compile(r"^\s*#\s*(?P<word>\w+)(?P<rest>.*)$")
VFTABLE = re.compile(r"^const (?P<owner>.+)::`vftable'(?P<more>.*)$")
BUILTIN = {"void", "bool", "char", "short", "int", "long", "float", "double", "__int64", "unsigned", "signed", "const"}
# Sizes of the scalar types a global may be declared with on either side (x86).
SIZES = {"bool": 1, "char": 1, "short": 2, "int": 4, "long": 4, "float": 4, "double": 8, "__int64": 8,
         "int8_t": 1, "uint8_t": 1, "int16_t": 2, "uint16_t": 2, "int32_t": 4, "uint32_t": 4, "int64_t": 8,
         "uint64_t": 8, "size_t": 4, "intptr_t": 4, "uintptr_t": 4}
QUALIFIERS = {"const", "volatile", "unsigned", "signed", "std"}
SOURCE_SUFFIXES = {".cpp", ".c", ".h", ".hpp", ".inc"}
WRITABLE = 0x0078D000              # the original's .data; below it .rdata, whose values never change
# A bound global is a reference to the playable storage, cast from it without a function call, so it is
# initialized statically (no dynamic initializer can run after a caller), next to a check of the storage's size.
HELPERS = """\
namespace kotor_bridge {
template <class T> using ref = T&;
template <class T> using storage = std::remove_cv_t<std::remove_reference_t<T>>;
}  // namespace kotor_bridge"""


@dataclass(frozen=True)
class Function:
    """A public thiscall member function, constructor or destructor, from its demangled MSVC signature."""
    address: int
    owner: str
    name: str
    returns: str
    params: tuple[str, ...]
    const: bool

    @classmethod
    def parse(cls, address: int, demangled: str) -> Function:
        found = SIGNATURE.match(demangled)
        if not found:
            raise ValueError(f"{address:08X}: only public __thiscall members are bridged: {demangled}")
        owner, name, params = plain(found["owner"]), found["name"], found["params"].strip()
        special = name in {short_name(owner), f"~{short_name(owner)}"}
        if bool(found["ret"]) == special:
            raise ValueError(f"{address:08X}: unexpected signature shape: {demangled}")
        returns = ("void*" if not name.startswith("~") else "void") if special else plain(found["ret"])
        return cls(address, owner, name, returns,
                   () if params == "void" else tuple(map(plain, split_params(params))), bool(found["const"]))

    @property
    def entry(self) -> str:
        return f"kotor_fastcall_{self.address:08x}"

    @property
    def kind(self) -> str:
        return "destructor" if self.name.startswith("~") else \
            "constructor" if self.name == short_name(self.owner) else "member"

    @property
    def returns_value(self) -> bool:
        return self.returns != "void"

    def prototype(self) -> str:
        """The extern "C" entry: thiscall as fastcall with the unused EDX, as the playable build declares it."""
        params = ", ".join(["void* self", "void*", *(f"{abi(t)} a{i}" for i, t in enumerate(self.params))])
        return f"{abi(self.returns)} __fastcall {self.entry}({params})"

    def entry_definition(self, table: str | None = None) -> str:
        """The entry; for a constructor or destructor, table is the playable build's vtable for the class."""
        args = ", ".join(argument(t, f"a{i}") for i, t in enumerate(self.params))
        if self.kind == "member":
            body = [f"{'return ' if self.returns_value else ''}static_cast<{self.owner}*>(self)->{self.name}({args});"]
        else:
            call = (f"::new (self) {self.owner}({args});" if self.kind == "constructor"
                    else f"static_cast<{self.owner}*>(self)->{self.owner}::{self.name}();")
            if table:
                body = [f"static_assert(std::is_polymorphic<{self.owner}>::value, \"a vtable is bound\");", call,
                        f"*static_cast<const void**>(self) = static_cast<const void*>({table});"
                        "  // the playable build's table"]
            else:
                body = [f"static_assert(!std::is_polymorphic<{self.owner}>::value, "
                        f"\"{self.owner} has a vtable but no playable table is bound for it\");", call]
            body += ["return self;"] if self.kind == "constructor" else []
        lines = "\n".join(f"    {line}" for line in body)
        return f'extern "C" {self.prototype()}\n{{\n{lines}\n}}\n'

    def forward_definition(self) -> str:
        """Define the member by calling the playable build's entry for the same address."""
        params = ", ".join(f"{t} a{i}" for i, t in enumerate(self.params))
        args = ", ".join(["const_cast<void*>(static_cast<const void*>(this))", "nullptr",
                          *(passed(t, f"a{i}") for i, t in enumerate(self.params))])
        call = f"{self.entry}({args})"
        if is_object(self.returns):
            call = f"static_cast<{self.returns}>({call})"
        const = " const" if self.const else ""
        return (f"{self.returns} {self.owner}::{self.name}({params}){const}\n"
                f"{{\n    {'return ' if self.returns_value else ''}{call};\n}}\n")


def plain(text: str) -> str:
    """'class CExoString const &' -> 'CExoString const&'."""
    text = re.sub(r"\b(?:class|struct|enum) ", "", text.strip())
    return re.sub(r"\s+([*&])", r"\1", text)


def short_name(owner: str) -> str:
    """'CExoArrayList<CSWSTagNode>' -> 'CExoArrayList'; 'A::B' -> 'B'."""
    return re.sub(r"<.*>", "", owner).split("::")[-1]


def split_params(text: str) -> list[str]:
    params, depth, start = [], 0, 0
    for i, ch in enumerate(text):
        depth += (ch == "<") - (ch == ">")
        if ch == "," and depth == 0:
            params.append(text[start:i])
            start = i + 1
    return [*params, text[start:]]


def is_object(type_: str) -> bool:
    """A pointer or reference to a class, which an entry passes as void*."""
    return type_.endswith(("*", "&")) and not set(re.findall(r"\w+", type_)) <= BUILTIN


def abi(type_: str) -> str:
    """The entry's view of a type: pointers and references to classes cross as void*, references as pointers."""
    return "void*" if is_object(type_) else type_.replace("&", "*")


def argument(type_: str, name: str) -> str:
    """Rebuild a member's argument from the entry's parameter."""
    if not is_object(type_):
        return f"*{name}" if type_.endswith("&") else name
    cast = f"static_cast<{type_[:-1]}*>({name})"
    return f"*{cast}" if type_.endswith("&") else cast


def passed(type_: str, name: str) -> str:
    """Hand a member's argument to an entry."""
    if type_.endswith("&"):
        return f"&{name}" if not is_object(type_) else f"const_cast<void*>(static_cast<const void*>(&{name}))"
    return f"const_cast<void*>(static_cast<const void*>({name}))" if is_object(type_) else name


def size_of(type_: str, bounds: str) -> int | None:
    """Bytes of a scalar, pointer or array of them on x86; None for anything else (a class, an unknown bound)."""
    counts = re.findall(r"\[([^\]]*)\]", bounds)
    if any(not c.strip().isdigit() for c in counts):
        return None
    words = [w for w in re.findall(r"\w+", type_) if w not in QUALIFIERS]
    if re.search(r"\*\s*(?:const|volatile|\s)*$", type_):
        element = 4
    elif len(words) == 1 and words[0] in SIZES:
        element = SIZES[words[0]]
    elif not words and re.search(r"\b(?:un)?signed\b", type_):
        element = 4                                    # plain 'unsigned' or 'signed': int
    else:
        return None
    return element * math.prod(int(c) for c in counts)


# ---------------------------------------------------------------- slicing matched sources

def definition_end(text: str, start: int) -> int:
    """End of the definition starting at start: its closing brace, or the ';' of a body-less declaration."""
    depth, i = 0, start
    while i < len(text):
        two, ch = text[i:i + 2], text[i]
        if two == "//":
            i = text.find("\n", i)
            if i < 0:
                break
        elif two == "/*":
            i = text.index("*/", i + 2) + 1
        elif ch in "\"'":
            i += 1
            while text[i] != ch:
                i += 2 if text[i] == "\\" else 1
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        elif ch == ";" and depth == 0:
            return i + 1
        i += 1
    raise ValueError("Unterminated definition")


def select(text: str, keep: set[int]) -> tuple[str, set[int]]:
    """text without the annotated definitions outside keep; returns (text, addresses kept)."""
    lines = text.splitlines(keepends=True)
    offsets = [0]
    for line in lines:
        offsets.append(offsets[-1] + len(line))
    out, kept, i = [], set(), 0
    while i < len(lines):
        found = ANNOTATION.match(lines[i])
        if not found:
            out.append(lines[i])
            i += 1
            continue
        address = int(found[1], 16)
        code = next((k for k in range(i + 1, len(lines)) if ANNOTATION.match(lines[k])
                     or (lines[k].strip() and not lines[k].lstrip().startswith("//"))), None)
        if code is None or ANNOTATION.match(lines[code]):
            if address in keep:                   # a marker: the compiler generates this function from the class
                raise ValueError(f"{address:08X} is compiler-generated; its source is the class, not a definition")
            i += 1
            continue
        end = definition_end(text, offsets[i + 1])
        j = next(k for k in range(i + 1, len(offsets)) if offsets[k] >= end)
        if address in keep:
            out.extend(lines[i:j])
            kept.add(address)
        i = j
    return "".join(out), kept


def guarded(text: str) -> bool:
    """Whether text is wrapped in the include-guard idiom: #ifndef G, #define G, ..., #endif."""
    directives = [m for m in map(DIRECTIVE.match, text.splitlines()) if m]
    if len(directives) < 3:
        return False
    first, second, last = directives[0], directives[1], directives[-1]
    return (first["word"], second["word"], last["word"]) == ("ifndef", "define", "endif") \
        and first["rest"].split() == second["rest"].split()[:1]


def flatten(text: str, origin: Path, seen: set[Path], system: list[str]) -> str:
    """Inline matching/include headers once each and collect <system> includes, which must not be conditional."""
    out, depth, guard = [], 0, int(guarded(text))
    for line in text.splitlines(keepends=True):
        found = DIRECTIVE.match(line)
        word = found["word"] if found else ""
        depth += (word in {"if", "ifdef", "ifndef"}) - (word == "endif")
        if word == "pragma" and found["rest"].strip() == "once":
            continue
        if word != "include":
            out.append(line)
            continue
        if depth > guard:
            raise ValueError(f"{origin}: an #include inside a conditional block cannot be bridged")
        name = found["rest"].strip()
        if name.startswith("<"):
            system.append(name)
            continue
        header = next((p for p in (origin.parent / name[1:-1], INCLUDE / name[1:-1]) if p.is_file()), None)
        if header is None:
            raise ValueError(f"{origin}: header {name} not found")
        if header.resolve() not in seen:
            seen.add(header.resolve())
            out.append(f"// ---- {header.resolve().relative_to(ROOT).as_posix()}\n")
            out.append(flatten(header.read_text(encoding="utf-8"), header, seen, system))
    return "".join(out)


# ---------------------------------------------------------------- the playable build's data

@dataclass(frozen=True)
class Storage:
    """A playable definition of an original data address: a namespace-scope variable named after it."""
    address: int
    variable: cpp_scope.Variable

    @property
    def qualified(self) -> str:
        namespace = "" if self.variable.statement.c_linkage else "::".join(self.variable.statement.namespace)
        return f"::{namespace}::{self.variable.name}" if namespace else f"::{self.variable.name}"

    @property
    def size(self) -> int | None:
        return size_of(self.variable.type, self.variable.bounds)

    def declaration(self) -> str:
        """Its declaration at global scope; arrays keep an unknown bound, which only an address needs."""
        v = self.variable
        text = f"extern {v.type} {v.name}{'[]' if v.bounds else ''};"
        if v.statement.c_linkage:
            return f'extern "C" {text[len("extern "):]}'
        for namespace in reversed(v.statement.namespace):
            text = f"namespace {namespace} {{ {text} }}"
        return text

    def supported(self) -> bool:
        """Whether the slice can spell its type: scalars and pointers to them, with std:: integer names."""
        words = re.findall(r"\w+", self.variable.type)
        return set(words) <= BUILTIN | QUALIFIERS | set(SIZES)


class PlayableData:
    """The pinned playable sources' storage for original data addresses, read once and scanned on demand."""

    def __init__(self, files: list[Path]) -> None:
        self.files = files
        self.found: dict[int, tuple[cpp_scope.Variable, ...]] = {}

    @cached_property
    def texts(self) -> dict[Path, str]:
        return {p: p.read_text(encoding="utf-8", errors="replace") for p in self.files}

    def definitions(self, address: int) -> tuple[cpp_scope.Variable, ...]:
        """Every non-reference namespace-scope definition named after address, internal ones included."""
        if address not in self.found:
            tag = f"_{address:08x}"
            self.found[address] = tuple(v for text in self.texts.values() if tag in text.lower()
                                        for v in map(cpp_scope.variables, cpp_scope.statements(text))
                                        if v and v.name.lower().endswith(tag) and v.defined and not v.reference)
        return self.found[address]

    def storage(self, address: int) -> Storage | str:
        """The one external definition named after address, or why there is none."""
        found = {(v.statement.c_linkage, v.statement.namespace, v.name): Storage(address, v)
                 for v in self.definitions(address) if v.external}
        if len(found) != 1:
            names = sorted(s.qualified for s in found.values())
            internal = len(self.definitions(address)) - len(found)
            return (f"{len(found)} external playable definitions" + (f": {', '.join(names)}" if names else "")
                    + (f" ({internal} internal)" if internal else ""))
        storage = next(iter(found.values()))
        return storage if storage.supported() else f"unsupported playable type {storage.variable.type}"

    def constant(self, address: int) -> tuple[str, int] | None:
        """(value, definitions) when every playable definition of address is a literal of the same value."""
        values = {literal(v.initializer) for v in self.definitions(address)}
        if len(values) != 1 or None in values:
            return None
        value = values.pop()
        return (repr(value) if isinstance(value, float) else str(value)), len(self.definitions(address))


@dataclass(frozen=True)
class Constant:
    """A read-only original datum the slice may copy: every playable definition agrees on its value."""
    address: int
    value: str
    definitions: int


def literal(text: str) -> int | float | None:
    """The value of an integer or floating literal, braces allowed ('{0}', '0x00u', '1.5f', '0x1p+0f')."""
    text = text.strip()
    if text.startswith("{") and text.endswith("}"):
        text = text[1:-1].strip()
    if re.fullmatch(r"[-+]?(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*", text):
        digits = text.rstrip("uUlL")
        unsigned = digits.lstrip("+-")
        base = 16 if unsigned[:2] in {"0x", "0X"} else 8 if len(unsigned) > 1 and unsigned[0] == "0" else 10
        return int(digits, base)
    if re.fullmatch(r"[-+]?0[xX][0-9a-fA-F.]+[pP][-+]?\d+[fFlL]?", text):
        return float.fromhex(text.rstrip("fFlL"))
    if re.fullmatch(r"[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?[fFlL]?", text):
        return float(text.rstrip("fFlL"))
    return None


def pinned_sources() -> list[Path]:
    files = json.loads(pin_sources.MANIFEST.read_bytes())["targets"]["game"]["files"]
    return [ROOT / f["path"] for f in files
            if not f["path"].startswith(BRIDGE_PREFIX) and Path(f["path"]).suffix in SOURCE_SUFFIXES]


# ---------------------------------------------------------------- generation

@dataclass
class Tables:
    """What matching/ records about the original: function status, signatures, data and vftable addresses."""
    status: dict[int, dict[str, str]]
    signatures: dict[int, set[str]]
    globals: dict[str, set[int]]               # namespace-scope global name -> original addresses
    vftables: dict[str, set[int]]              # class -> original vftable addresses (several: multiple bases)

    @classmethod
    def load(cls) -> Tables:
        with (MATCHING / "status.csv").open(encoding="utf-8") as rows:
            status = {int(r["address"], 16): r for r in csv.DictReader(rows)}
        tables = cls(status, {}, {}, {})
        with (MATCHING / "symbols.csv").open(encoding="utf-8") as rows:
            for r in csv.DictReader(rows):
                address = int(r["address"], 16)
                if r["role"] in {"defines", "calls"}:
                    tables.signatures.setdefault(address, set()).add(r["demangled"])
                elif r["role"] == "data":
                    vftable = VFTABLE.match(r["demangled"])
                    if vftable:
                        tables.vftables.setdefault(plain(vftable["owner"]), set()).add(address)
                    elif not r["symbol"].startswith("??_") and "::" not in r["demangled"]:
                        name = re.findall(r"\w+", r["demangled"])[-1]
                        tables.globals.setdefault(name, set()).add(address)
        return tables

    def signature(self, address: int, body: str | None = None) -> str:
        """The demangled signature of address; with several, the one whose declaration the slice body holds."""
        known = sorted(self.signatures.get(address, set()))
        if len(known) > 1 and body is not None:
            known = [s for s in known if declared(Function.parse(address, s), body)]
        if len(known) != 1:
            raise ValueError(f"{address:08X}: {len(known)} matching signatures, the bridge needs exactly one")
        return known[0]


def declared(f: Function, body: str) -> bool:
    """Whether body declares f's member with exactly f's return type (not merely ending in it)."""
    found = re.finditer(rf"(?:^|[;{{}}:])\s*(?:(?:virtual|inline|static)\s+)*(?P<ret>[\w\s*&<>]+?)\s*"
                        rf"\b{re.escape(f.name)}\s*\(", body, re.MULTILINE)
    return any(plain(m["ret"]) == f.returns for m in found)


def playable_entry(name: str, data: PlayableData) -> tuple[int, bool]:
    """(parameter count, returns a value) of the playable build's one definition of an extern "C" entry."""
    pattern = re.compile(r'extern\s+"C"\s+(?P<ret>[\w:<>* ]+?)\s+__fastcall\s+' + name
                         + r"\s*\((?P<params>[^;{}]*)\)\s*\{")
    found = [m for path, text in data.texts.items() if path.suffix in {".cpp", ".inc"} and name in text
             for m in pattern.finditer(text)]
    if len(found) != 1:
        raise ValueError(f"Forward target {name} needs exactly one playable definition, found {len(found)}")
    params = found[0]["params"].strip()
    return (0 if params in {"", "void"} else len(split_params(params))), found[0]["ret"].strip() != "void"


def generate() -> dict[Path, str]:
    """{generated file: text} for the spec."""
    spec = json.loads(SPEC.read_text(encoding="utf-8"))
    tables, data = Tables.load(), PlayableData(pinned_sources())
    functions: dict[str, list[Function]] = {}
    forwards: dict[str, set[int]] = {}
    for item in spec["functions"]:
        address = int(item["address"], 16)
        row = tables.status.get(address)
        if not row or row["status"] != "MATCH":
            raise ValueError(f"{address:08X} is not a byte-identical (MATCH) function")
        functions.setdefault(row["file"], []).append(Function.parse(address, tables.signature(address)))
        forwards.setdefault(row["file"], set()).update(int(a, 16) for a in item.get("forwards", []))
    return {OUT / f"{Path(source).parent.name}_{Path(source).stem}.cpp":
            Unit(ROOT / source, members, sorted(forwards[source]), tables, data).text()
            for source, members in sorted(functions.items())}


@dataclass
class Unit:
    """One generated unit: a matched file's slice with its bindings."""
    source: Path
    functions: list[Function]
    forward_addresses: list[int]
    tables: Tables
    data: PlayableData

    def text(self) -> str:
        original = self.source.read_text(encoding="utf-8")
        wanted = {f.address for f in self.functions}
        sliced, kept = select(original, wanted)
        if kept != wanted:
            raise ValueError(f"{self.source}: no annotated definition for "
                             f"{sorted(f'{a:08X}' for a in wanted - kept)}")
        system: list[str] = []
        body = flatten(sliced, self.source, set(), system)
        forwards = [self.forward(a, body) for a in self.forward_addresses]
        body, bound, unbound = self.bind_globals(body)
        tables = {f.address: self.vtable(f) for f in self.functions if f.kind != "member"}
        globals_ = list(dict.fromkeys(s for _, s in bound if isinstance(s, Storage)))
        storages = list(dict.fromkeys([*globals_, *(s for s in tables.values() if s)]))
        if tables:
            system.append("<new>")
        if tables or globals_:
            system.append("<type_traits>")
        if any("std::" in s.variable.type for s in storages):
            system.append("<cstdint>")
        namespace = f"kotor_bridge_{self.source.parent.name}_{self.source.stem}"
        return "\n".join([
            f"// GENERATED by bridge_sources.py from {self.source.relative_to(ROOT).as_posix()} "
            f"(sha256 {hashlib.sha256(original.encode()).hexdigest()[:16]}). Do not edit:",
            "// change the matched source or src/bridge/bridge.json and regenerate. The byte-identical matched code",
            f'// lives in namespace {namespace}; only the extern "C" entries at its end are meant to be called.',
            *(f"// global {name} -> {s.qualified} ({s.address:08X})" if isinstance(s, Storage) else
              f"// constant {name} = {s.value} ({s.address:08X}, as all {s.definitions} playable definitions agree)"
              for name, s in bound),
            *(f"// vtable {f.owner} -> {s.qualified} ({s.address:08X})"
              for f in self.functions if (s := tables.get(f.address))),
            *(f"// unbound {name}: {why} (the link fails if it is used)" for name, why in unbound),
            *(f"#include {name}" for name in dict.fromkeys(system)),
            *([HELPERS] if globals_ else []),
            *(s.declaration() for s in storages),
            *(f'extern "C" {f.prototype()};' for f in forwards),
            f"namespace {namespace} {{",
            body.rstrip(),
            "",
            *(f.forward_definition() for f in forwards),
            *(f.entry_definition(tables[f.address].qualified if tables.get(f.address) else None)
              for f in sorted(self.functions, key=lambda f: f.address)),
            f"}}  // namespace {namespace}",
            "",
        ])

    def forward(self, address: int, body: str) -> Function:
        """A callee of a bridged function, defined by calling the playable build's entry for it."""
        callee = Function.parse(address, self.tables.signature(address, body))
        if playable_entry(callee.entry, self.data) != (len(callee.params) + 2, callee.returns_value):
            raise ValueError(f"{callee.entry}: the playable definition does not have the original's signature")
        return callee

    def bind_globals(self, body: str) -> tuple[str, list[tuple[str, Storage | Constant]], list[tuple[str, str]]]:
        """body with each used matched extern global bound: a reference to the playable storage, or for read-only
        data a copy of the value every playable definition agrees on. Returns (body, bound, unbound)."""
        bound, unbound, edits = [], [], []
        for statement in cpp_scope.statements(body):
            v = cpp_scope.variables(statement)
            if not v or v.defined or not v.external or v.reference or statement.c_linkage or statement.namespace:
                continue
            if len(re.findall(rf"\b{re.escape(v.name)}\b", body)) < 2:
                continue                                         # declared, never used
            target = self.global_binding(v)
            if isinstance(target, str):
                unbound.append((v.name, target))
                continue
            bound.append((v.name, target))
            matched = f"{v.type}{v.bounds}"
            q = target.qualified if isinstance(target, Storage) else ""
            edits.append((statement.start, statement.end,
                          f"{v.type} {v.name} = {target.value};" if isinstance(target, Constant) else
                          f'static_assert(sizeof({matched}) <= sizeof({q}), "the playable storage is too small"); '
                          f"kotor_bridge::ref<{matched}> {v.name} = reinterpret_cast<kotor_bridge::ref<{matched}>>"
                          f"(const_cast<kotor_bridge::storage<decltype({q})>&>({q}));"))
        for start, end, text in sorted(edits, reverse=True):
            body = body[:start] + text + body[end:]
        return body, bound, unbound

    def global_binding(self, v: cpp_scope.Variable) -> Storage | Constant | str:
        addresses = self.tables.globals.get(v.name, set())
        if len(addresses) != 1:
            return f"{len(addresses)} original addresses in symbols.csv"
        address = next(iter(addresses))
        storage = self.data.storage(address)
        if isinstance(storage, Storage):
            if storage.variable.bounds:
                return f"{address:08X}: the playable storage {storage.qualified} is an array"
            matched, playable = size_of(v.type, v.bounds), storage.size
            if matched and playable and matched > playable:
                return f"{address:08X}: {matched} bytes, the playable {storage.qualified} holds {playable}"
            return storage
        read_only = address < WRITABLE and cpp_scope.top_level_const(re.findall(r"\w+|[*&]", v.type)) and not v.bounds
        constant = self.data.constant(address) if read_only else None
        return Constant(address, *constant) if constant else f"{address:08X}: {storage}"

    def vtable(self, f: Function) -> Storage | None:
        """The playable table for a constructed or destroyed class's original vftable, if it has one."""
        addresses = self.tables.vftables.get(f.owner, set())
        if len(addresses) > 1:
            raise ValueError(f"{f.address:08X}: {f.owner} has several vftables (multiple bases), not bridged")
        if not addresses:
            return None
        storage = self.data.storage(next(iter(addresses)))
        if isinstance(storage, str):
            raise ValueError(f"{f.address:08X}: no playable table for {f.owner}'s vftable: {storage}")
        return storage


def register(target: dict, sources: list[str]) -> list[str]:
    """Make target's bridge units and pins exactly sources (already on disk); returns what changed.

    Only src/bridge/ entries are touched; other pins stay pin_sources.py's business."""
    bridged = [u for u in target["units"] if u["source"].startswith(BRIDGE_PREFIX)]
    wanted = [{"source": s, "compiler": "msvc", "flags": RECIPE} for s in sources]
    notes = [] if bridged == wanted else ["differs: the manifest's bridge units"]
    target["units"] = [u for u in target["units"] if not u["source"].startswith(BRIDGE_PREFIX)] + wanted
    pins = [f for f in target["files"] if f["path"] in sources]
    if len(pins) != len([f for f in target["files"] if f["path"].startswith(BRIDGE_PREFIX)]):
        notes.append("differs: the manifest pins removed bridge files")
    changed, added = pin_sources.pin(pins, sources)
    target["files"] = [f for f in target["files"] if not f["path"].startswith(BRIDGE_PREFIX)] + pins
    return notes + [f"differs: pin of {p}" for p in changed] + [f"unpinned: {p}" for p in added]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="only report; exit 1 if anything would change")
    args = parser.parse_args()
    outputs = generate()
    changed = [p for p, text in outputs.items() if not p.is_file() or p.read_text(encoding="utf-8") != text]
    stale = sorted(p for p in OUT.glob("*.cpp") if p not in outputs)
    for path in changed:
        print(f"{'differs' if args.check else 'wrote'}: {path.relative_to(ROOT).as_posix()}")
    for path in stale:
        print(f"{'stale' if args.check else 'removed'}: {path.relative_to(ROOT).as_posix()}")
    if args.check:
        return 1 if changed or stale else check_manifest(outputs)
    for path in changed:
        path.write_text(outputs[path], encoding="utf-8", newline="\n")
    for path in stale:
        path.unlink()
    manifest = json.loads(pin_sources.MANIFEST.read_bytes())
    if register(manifest["targets"]["game"], sorted(p.relative_to(ROOT).as_posix() for p in outputs)):
        pin_sources.write_manifest(manifest)
        print("updated: build-manifest.json bridge units and pins")
    return 0


def check_manifest(outputs: dict[Path, str]) -> int:
    """With the generated files current: 1 if the manifest's bridge units or pins would change."""
    manifest = json.loads(pin_sources.MANIFEST.read_bytes())
    notes = register(manifest["targets"]["game"], sorted(p.relative_to(ROOT).as_posix() for p in outputs))
    for note in notes:
        print(note)
    return 1 if notes else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValueError as exc:
        print(f"Bridge stopped: {exc}", file=sys.stderr)
        sys.exit(1)
