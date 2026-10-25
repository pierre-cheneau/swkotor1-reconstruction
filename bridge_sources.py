"""Generate src/bridge/: byte-identical matched functions compiled into the playable build.

    uv run bridge_sources.py            regenerate src/bridge/ and its manifest units and pins
    uv run bridge_sources.py --check    exit 1 if a generated file or bridge unit is missing, differs or is stale

This is phase 2 of STRUCTURE.md. For every address the spec names, the matched
source (matching/src, status MATCH) is sliced to the preamble its file declares
plus the selected functions, with its matching/include headers inlined. The
slice sits in a namespace of its own, so none of its classes, inline functions
or vtables can merge with another unit's copy. It adds one extern "C" entry per
function, named kotor_fastcall_<address> as the playable build names original
thiscall functions; a class returned by value is built at the hidden pointer
the caller passes after EDX, and the entry returns it, as the original does.

What a slice uses from outside its unit is bound explicitly:
  calls    the forwards its spec entry names, each defined as a jump to a
           playable entry for the callee: kotor_fastcall_<address> (a
           bridged function's generated entry, or src/'s), or the extern "C"
           function the spec names. A thiscall member and its fastcall entry
           share one frame, so src/'s entry must pop the original's stack
           bytes and return in its register; bridge_gate.py checks the bytes
           against the original's code;
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
from dataclasses import dataclass, field
from functools import cached_property
from pathlib import Path

import cpp_scope
import pin_sources
from local_generators import game_prepare

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "src" / "bridge"
SPEC = OUT / "bridge.json"
BRIDGE_PREFIX = "src/bridge/"
# The manifest's predominant MSVC recipe; the matched slices compile warning-free under it.
RECIPE = ["/nologo", "/O2", "/Ob0", "/Oy-", "/GS", "/EHsc", "/MT", "/W4", "/WX", "/std:c++17", "/showIncludes",
          "/D_CRT_SECURE_NO_WARNINGS"]
MATCHING = ROOT / "matching"
INCLUDE = MATCHING / "include"
SIGNATURE = re.compile(r"^public: (?:virtual )?(?:(?P<ret>.+?) )?__thiscall (?P<owner>.+)::"
                       r"(?:(?P<name>~?\w+)(?:<[^()]*>)?|(?P<operator>operator(?:\(\)|\[\]|[^\w\s(]+)))"
                       r"\((?P<params>.*)\)(?P<const>const)?\s*$")      # a template's ctor: X<T>::X<T>(...)
ANNOTATION = re.compile(r"^// FUNCTION: SWKOTOR 0x([0-9A-Fa-f]{8})\b")
# An explicit instantiation: of a class template ('template class X<T>;') or of one member ('template void X<T>::F(T);')
INSTANTIATION = re.compile(r"^\s*template\s+(?!<)[^;{}]*;\s*$")
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
# Matched code's direct C heap calls, rebound to the game heap inside the slice's namespace, where unqualified lookup
# finds them before the toolchain CRT's: a block from one heap freed by the other corrupts memory (see
# src/units/0211/643fca338b-game_heap_global_new.cpp, which does the same for ::operator new and delete).
GAME_HEAP = {
    "malloc": "inline void* malloc(std::size_t size) { return live_malloc(static_cast<std::uint32_t>(size)); }",
    "free": "inline void free(void* block) { live_free(block); }",
}
GAME_HEAP_DECLARATIONS = 'extern "C" { void* __cdecl live_malloc(std::uint32_t); void __cdecl live_free(void*); }'
# Warnings the byte-identical matched code raises by design under the recipe, silenced around it alone (the entries
# and forwards stay under /W4 /WX): C4100, a parameter the original ignores too; C4244, a narrowing conversion the
# original performs too (an int returned as char).
MATCHED_WARNINGS = (4100, 4244)
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
    hidden: bool = False                 # returns a class by value, built at a hidden pointer the caller passes
    sizes: tuple[int | None, ...] = ()   # stack bytes of each parameter; None for a class passed by value

    @classmethod
    def parse(cls, address: int, demangled: str) -> Function:
        found = SIGNATURE.match(demangled)
        if not found:
            raise ValueError(f"{address:08X}: only public __thiscall members are bridged: {demangled}")
        owner, name, params = plain(found["owner"]), found["name"] or found["operator"], found["params"].strip()
        special = name in {short_name(owner), f"~{short_name(owner)}"}
        if bool(found["ret"]) == special:
            raise ValueError(f"{address:08X}: unexpected signature shape: {demangled}")
        returns = ("void*" if not name.startswith("~") else "void") if special else plain(found["ret"])
        raw = [] if params == "void" else split_params(params)
        return cls(address, owner, name, returns, tuple(map(plain, raw)), bool(found["const"]),
                   not special and by_value_class(found["ret"]), tuple(stack_size(p) for p in raw))

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

    @property
    def result(self) -> str:
        """Where the caller finds the result: 'void', 'eax' (a word, or the hidden pointer), 'edx:eax', 'st0'."""
        if self.hidden or self.kind == "constructor":
            return "eax"
        return return_register(self.returns)

    @property
    def stack(self) -> int | None:
        """Stack bytes the callee pops (thiscall), the hidden result pointer included; None if not computable."""
        return None if None in self.sizes else sum(self.sizes) + 4 * self.hidden

    def prototype(self, name: str | None = None) -> str:
        """The extern "C" entry: thiscall as fastcall with the unused EDX, as the playable build declares it; a
        class returned by value is built at the pointer that follows EDX, which the entry returns."""
        params = ", ".join(["void* self", "void*", *(["void* out"] if self.hidden else []),
                            *(f"{abi(t)} a{i}" for i, t in enumerate(self.params))])
        return f"{'void*' if self.hidden else abi(self.returns)} __fastcall {name or self.entry}({params})"

    def entry_definition(self, table: str | None = None) -> str:
        """The entry; for a constructor or destructor, table is the playable build's vtable for the class."""
        args = ", ".join(argument(t, f"a{i}") for i, t in enumerate(self.params))
        call = f"static_cast<{self.owner}*>(self)->{self.name}({args})"
        if self.hidden:                  # C++17 builds the prvalue in place: no copy, as the original's caller
            body = [f"::new (out) {self.returns}({call});", "return out;"]
        elif self.kind == "member":
            body = [f"{'return ' if self.returns_value else ''}{call};"]
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

    def forward_definition(self, symbol: str) -> str:
        """Define the member as a jump to the playable build's entry symbol. A thiscall member and its fastcall entry
        share one frame (ECX this, EDX unused, the same stack arguments, the same cleanup), so the jump serves every
        kind alike: constructors and destructors, operators, and classes returned through a hidden pointer."""
        params = ", ".join(self.params)
        returns = "" if self.kind != "member" else f"{self.returns} "
        const = " const" if self.const else ""
        head = f"__declspec(naked) {returns}{self.owner}::{self.name}({params}){const}"
        return f"{head}\n{{\n    __asm jmp {symbol}\n}}\n"


def plain(text: str) -> str:
    """'class CExoString const &' -> 'CExoString const&'."""
    text = re.sub(r"\b(?:class|struct|enum) ", "", text.strip())
    return re.sub(r"\s+([*&])", r"\1", text)


def short_name(owner: str) -> str:
    """'CExoArrayList<CSWSTagNode>' -> 'CExoArrayList'; 'A::B' -> 'B'."""
    return re.sub(r"<.*>", "", owner).split("::")[-1]


def by_value_class(type_: str) -> bool:
    """Whether a demangled type is a class, struct or union itself (not a pointer or reference to one)."""
    type_ = type_.strip()
    return bool(re.match(r"(?:const |volatile )*(?:class|struct|union) ", type_)) and not type_.endswith(("*", "&"))


def stack_size(type_: str) -> int | None:
    """Stack bytes of a demangled parameter on x86: a word, or two for 64-bit values; None for a class by value."""
    if type_.strip().endswith(("*", "&")):
        return 4
    if by_value_class(type_):
        return None
    wide = set(re.findall(r"\w+", type_)) & {"double", "__int64", "int64_t", "uint64_t"} or "long long" in type_
    return 8 if wide else 4


def return_register(type_: str) -> str:
    """Where a returned scalar lives: 'void', 'st0' (floating), 'edx:eax' (64-bit) or 'eax'."""
    words = set(re.findall(r"\w+", type_))
    if type_ == "void":
        return "void"
    if type_.endswith(("*", "&")):
        return "eax"
    if words & {"float", "double"}:
        return "st0"
    return "edx:eax" if words & {"__int64", "int64_t", "uint64_t"} or "long long" in type_ else "eax"


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
    """text without the annotated definitions outside keep; returns (text, addresses kept).

    Explicit instantiations ('template class X<T>;') go too: the matched file has them to emit its template members
    for the original's link, while a slice's entries instantiate exactly the members they call, and an instantiation
    would emit every member, with callees nothing forwards."""
    lines = text.splitlines(keepends=True)
    offsets = [0]
    for line in lines:
        offsets.append(offsets[-1] + len(line))
    out, kept, i = [], set(), 0
    while i < len(lines):
        found = ANNOTATION.match(lines[i])
        if not found:
            if not INSTANTIATION.match(lines[i]):
                out.append(lines[i])
            i += 1
            continue
        address = int(found[1], 16)
        code = next((k for k in range(i + 1, len(lines)) if ANNOTATION.match(lines[k])
                     or (lines[k].strip() and not lines[k].lstrip().startswith("//"))), None)
        after = next((k for k in range(i, len(lines)) if not ANNOTATION.match(lines[k])), len(lines))
        if after < len(lines) and INSTANTIATION.match(lines[after]):
            # members of a class template, which the instantiation emits: their source is the template itself
            kept |= {int(ANNOTATION.match(lines[k])[1], 16) for k in range(i, after)} & keep
            i = after + 1
            continue
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


def inline_helpers(text: str) -> str:
    """text with each unannotated namespace-scope function definition made inline. Such helpers of a matched file
    exist for the original's link (to make the compiler emit a class's generated members) or for its own functions;
    inline, the slice emits one only when an entry reaches it, so a helper's callees need no forward unless used."""
    lines = text.splitlines(keepends=True)
    starts = [0]
    for line in lines:
        starts.append(starts[-1] + len(line))
    edits = []
    for s in cpp_scope.statements(text):
        words = [t.text for t in s.tokens]
        head = words[:words.index("(")] if "(" in words else words
        if words[-1] != "{}" or head == words or "=" in head or words[0] in cpp_scope.SKIPPED | {"extern"} \
                or set(head) & {"inline", "__forceinline", "constexpr"}:
            continue
        line = next(k for k in range(len(lines)) if starts[k + 1] > s.start)
        above = next((k for k in range(line - 1, -1, -1) if lines[k].strip()
                      and not lines[k].lstrip().startswith("//")), None)
        annotated = any(ANNOTATION.match(lines[k]) for k in range((above or -1) + 1, line))
        if not annotated:
            edits.append(s.start)
    for start in reversed(edits):
        text = text[:start] + "inline " + text[start:]
    return text


def without_vtable(body: str, owner: str, source: Path) -> str:
    """body with owner declared __declspec(novtable): its bridged constructor or destructor points the object at the
    playable build's table, so the slice needs no table of its own, nor the virtual functions one would name. Sound
    only while nothing else in the slice constructs an owner, which would then lack a table: that is refused."""
    name = re.escape(owner)
    constructed = re.search(rf"\bnew\s+{name}\b|(?<![\w:~.>*&]){name}\s+(?!operator\b)[A-Za-z_]\w*\s*[;(={{\[]", body)
    if constructed:
        raise ValueError(f"{source}: {owner} gets the playable table, so the slice cannot construct one itself: "
                         f"{constructed.group().strip()!r}")
    head = re.compile(rf"\b(class|struct)\s+{name}\s*(?=[:{{])")
    if len(head.findall(body)) != 1:
        raise ValueError(f"{source}: {owner} needs exactly one class definition in the slice")
    return head.sub(lambda m: f"{m[1]} __declspec(novtable) {owner} ", body)


def class_body(body: str, name: str) -> tuple[str, str] | None:
    """(base clause, member text) of the one definition of class name in body, access labels dropped."""
    head = rf"\b(?:class|struct)\s+(?:__declspec\(\w+\)\s+)?{re.escape(name)}\s*(?P<bases>:[^{{;]*)?\{{"
    found = list(re.finditer(head, body))
    if len(found) != 1:
        return None
    start = found[0].end() - 1
    end = definition_end(body, start)
    inner = re.sub(r"\b(?:public|protected|private)\s*:", " ", body[start + 1:end - 1])
    return (found[0]["bases"] or "").strip(), inner


def implicit_work(owner: str, kind: str, body: str, seen: frozenset[str] = frozenset()) -> str | None:
    """What the compiler itself does in owner's constructor (or destructor) before its body, or None. A naked jump
    forward runs that code with no frame of its own, so it is sound only when there is none: no table pointer, and
    for a constructor no bases and no member that needs constructing (recursively)."""
    parts = class_body(body, owner)
    if parts is None:
        return f"{owner}'s definition is not in the slice, so what its {kind} does implicitly is unknown"
    bases, inner = parts
    if bases:                                         # a base may carry the table pointer, or need constructing
        return f"{owner} has bases {bases[1:].strip()!r}, which its {kind} handles before the jump"
    if re.search(r"\bvirtual\b", inner):
        return f"{owner} has a table pointer, which its {kind} sets before the jump"
    if kind == "destructor":
        return None                                   # members are destroyed after the body: never reached
    for statement in cpp_scope.statements(inner):
        v = cpp_scope.variables(statement)
        if v is None or "static" in [t.text for t in statement.tokens] or "typedef" in statement.text:
            continue
        if v.initializer:
            return f"{owner}::{v.name} has a default member initializer"
        if "*" in v.type or set(re.findall(r"\w+", v.type)) <= BUILTIN | QUALIFIERS | set(SIZES):
            continue
        member = re.sub(r"\b(?:const|volatile|class|struct)\b", "", v.type).strip()
        if re.search(rf"\benum\s+{re.escape(member)}\b", body) or member in typedefs(body):
            continue
        if member in seen:
            return f"{owner} contains itself through {member}"
        if class_body(body, member) is None:
            return f"{owner}::{v.name}'s type {member} is not defined in the slice"
        if re.search(rf"(?<![~\w]){re.escape(member)}\s*\(\s*(?:void\s*)?\)", class_body(body, member)[1]):
            return f"{owner}::{v.name} is a {member}, whose default constructor runs first"
        why = implicit_work(member, kind, body, seen | {owner})
        if why:
            return why
    return None


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

    @cached_property
    def generated(self) -> str:
        """The storage the build generates rather than src/ defines: game_prepare.py's cells, which it emits in an
        extern "C" block with the original's initial values, written here without them."""
        return 'extern "C" {\n' + "".join(f"{kind} {name};\n" for kind, name, *_ in game_prepare.CELLS) + "}\n"

    def definitions(self, address: int) -> tuple[cpp_scope.Variable, ...]:
        """Every non-reference namespace-scope definition named after address, internal ones included."""
        if address not in self.found:
            tag = f"_{address:08x}"
            self.found[address] = tuple(v for text in [*self.texts.values(), self.generated] if tag in text.lower()
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

    def functions(self, name: str) -> list[cpp_scope.Function]:
        """Every external extern "C" definition of the function name."""
        return [f for text in self.texts.values() if name in text
                for f in map(cpp_scope.functions, cpp_scope.statements(text))
                if f and f.name == name and f.defined and f.external and f.statement.c_linkage]

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

    @property
    def agreement(self) -> str:
        return ("as its playable definition gives" if self.definitions == 1
                else f"as all {self.definitions} playable definitions agree")


def constant_type(type_: str) -> str:
    """type_ made const itself, if it is not: a copied constant that matched code tried to write fails to compile."""
    return type_ if cpp_scope.top_level_const(re.findall(r"\w+|[*&]", type_)) else f"{type_} const"


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
    defines: dict[int, set[str]] = field(default_factory=dict)   # the signatures the defining files give

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
                    if r["role"] == "defines":
                        tables.defines.setdefault(address, set()).add(r["demangled"])
                elif r["role"] == "data":
                    vftable = VFTABLE.match(r["demangled"])
                    if vftable:
                        tables.vftables.setdefault(plain(vftable["owner"]), set()).add(address)
                    elif not r["symbol"].startswith("??_") and "::" not in r["demangled"]:
                        name = re.findall(r"\w+", r["demangled"])[-1]
                        tables.globals.setdefault(name, set()).add(address)
        return tables

    def signature(self, address: int, body: str | None = None) -> str:
        """The demangled signature of address: as its defining file gives it, or for a callee, the one whose
        declaration the slice body holds (callers' headers may declare a function differently)."""
        source = self.signatures if body is not None else self.defines if address in self.defines else self.signatures
        known = sorted({Function.parse(address, s): s for s in sorted(source.get(address, set()))
                        if SIGNATURE.match(s)}.values())   # one per function: class keys may differ by caller
        if len(known) > 1 and body is not None:
            known = [s for s in known if declared(Function.parse(address, s), body)]
        if len(known) != 1:
            raise ValueError(f"{address:08X}: {len(known)} matching signatures, the bridge needs exactly one")
        return known[0]


def declared(f: Function, body: str) -> bool:
    """Whether body declares f's member with exactly f's return type (not merely ending in it) and parameter types,
    seeing through the body's simple typedefs; a constructor or destructor, which has no return type, by its name
    and parameters."""
    aliases = typedefs(body)
    ret = r"" if f.kind != "member" else r"(?P<ret>(?:\w|::|[ \t*&<>,])+?)\s*\b"     # not past 'public:'
    found = re.finditer(rf"(?:^|[;{{}}:])\s*(?:(?:virtual|inline|static|explicit)\s+)*{ret}{re.escape(f.name)}"
                        rf"\s*\((?P<params>[^;{{}}]*?)\)\s*(?:const\s*)?[;{{]", body, re.MULTILINE)
    return any((f.kind != "member" or resolve(m["ret"], aliases) == f.returns)
               and declared_params(m["params"], aliases) == [resolve(p, aliases) for p in f.params] for m in found)


def typedefs(body: str) -> dict[str, str]:
    """The body's 'typedef T Name;' and 'using Name = T;' aliases."""
    found = {m["name"]: m["type"] for m in re.finditer(r"\btypedef\s+(?P<type>[\w\s*&:<>]+?)\s*\b(?P<name>\w+)\s*;",
                                                         body)}
    found.update((m["name"], m["type"]) for m in re.finditer(r"\busing\s+(?P<name>\w+)\s*=\s*(?P<type>[^;]+);", body))
    return found


def resolve(type_: str, aliases: dict[str, str]) -> str:
    """A type with each alias word replaced by what it names (repeatedly), spelled as the demangler spells it:
    plain(), with a leading const moved after its type ('const char*' is 'char const*')."""
    for _ in range(8):
        replaced = re.sub(r"\b\w+\b", lambda m: aliases.get(m.group(), m.group()), type_)
        if replaced == type_:
            break
        type_ = replaced
    return re.sub(r"^const ((?:unsigned |signed )?[\w:]+(?:<[^>]*>)?)", r"\1 const", plain(type_))


def declared_params(text: str, aliases: dict[str, str]) -> list[str]:
    """A declaration's parameter types: names and default arguments dropped, aliases resolved."""
    text = text.strip()
    if text in {"", "void"}:
        return []
    types = []
    for written in split_params(text):
        declaration = written.split("=", 1)[0].strip()
        name = re.search(r"(?<![\w:])(\w+)$", declaration)  # a trailing identifier, not the end of A::B
        rest = declaration[:name.start()] if name else ""
        named = name and name[1] not in BUILTIN and (re.search(r"[*&>]\s*$", rest)
                                                     or set(re.findall(r"\w+", rest)) - {"const", "volatile"})
        types.append(resolve(rest if named else declaration, aliases))   # named: a type remains before the name
    return types


@dataclass(frozen=True)
class ForwardSpec:
    """What the spec says of one forward: the playable entry, if not kotor_fastcall_<address>, and the signature,
    for a callee symbols.csv records under other names only (identical code folded at its address)."""
    entry: str | None = None
    signature: str | None = None


@dataclass(frozen=True)
class Forward:
    """A callee of a bridged function, defined in the slice as a jump to a playable entry for its address."""
    function: Function
    symbol: str                      # the entry's name: kotor_fastcall_<address>, or the one the spec gives


def entry_stack(f: cpp_scope.Function) -> int | None:
    """Stack bytes a playable fastcall entry pops: its parameters after ECX and EDX, 64-bit ones counting two words."""
    if f.convention != "__fastcall" or len(f.params) < 2 or any(stack_size(p) != 4 for p in f.params[:2]):
        return None
    return sum(stack_size(p) or 4 for p in f.params[2:])


def compatible(callee: Function, entry: cpp_scope.Function) -> str | None:
    """Why the playable entry cannot stand in for the callee, or None: the same stack bytes and result register."""
    stack = entry_stack(entry)
    if stack is None:
        return f"{entry.name} is not a fastcall entry with ECX and EDX first"
    if callee.stack is None:
        return f"{callee.owner}::{callee.name} passes a class by value, whose size the bridge does not compute"
    if stack != callee.stack:
        return f"{entry.name} pops {stack} stack bytes, the original {callee.stack}"
    if callee.result != "void" and return_register(plain(entry.returns)) != callee.result:
        return f"{entry.name} returns {entry.returns} ({return_register(plain(entry.returns))}), " \
               f"the original's result is in {callee.result}"
    return None


def generate() -> dict[Path, str]:
    """{generated file: text} for the spec."""
    return {u.path: u.text() for u in units()}


def heap_calls(source: Path, body: str) -> list[str]:
    """The C heap functions body calls directly. The original's C heap is the game heap (006FA390 free, _nh_malloc),
    which src/ provides as live_malloc/live_free; anything without a game-heap counterpart is refused."""
    called = sorted(set(re.findall(r"(?<![\w.>:])(malloc|free|realloc|calloc|_msize|_?strdup)\s*\(", body)))
    unbound = [c for c in called if c not in GAME_HEAP]
    if unbound:
        raise ValueError(f"{source}: {', '.join(unbound)} has no game-heap counterpart; not bridged")
    return called


@dataclass(frozen=True)
class Plan:
    """A unit's slice and its bindings; text() renders it, bridge_gate.py checks it."""
    original: str                                       # the matched source
    body: str                                           # the slice, globals bound
    system: list[str]                                   # <system> includes the slice needs
    forwards: list[Forward]                             # callees defined as jumps to playable entries
    bound: list[tuple[str, Storage | Constant]]         # matched global name -> its binding
    unbound: list[tuple[str, str]]                      # matched global name -> why it is not bound
    tables: dict[int, Storage]                          # constructor/destructor address -> playable vtable
    heap: list[str]                                     # C heap functions the slice calls: rebound to the game heap

    @property
    def globals(self) -> list[Storage]:
        return list(dict.fromkeys(s for _, s in self.bound if isinstance(s, Storage)))


def units() -> list[Unit]:
    """The spec's units, with the tables and playable data they bind against."""
    spec = json.loads(SPEC.read_text(encoding="utf-8"))
    tables, data = Tables.load(), PlayableData(pinned_sources())
    functions: dict[str, list[Function]] = {}
    forwards: dict[str, dict[int, ForwardSpec]] = {}
    for item in spec["functions"]:
        address = int(item["address"], 16)
        row = tables.status.get(address)
        if not row or row["status"] != "MATCH":
            raise ValueError(f"{address:08X} is not a byte-identical (MATCH) function")
        f = Function.parse(address, tables.signature(address))
        if data.functions(f.entry):
            raise ValueError(f"{address:08X}: src/ already defines {f.entry}; bridging it would define it twice")
        functions.setdefault(row["file"], []).append(f)
        for forward in item.get("forwards", []):
            named = forward if isinstance(forward, dict) else {"address": forward}
            forwards.setdefault(row["file"], {})[int(named["address"], 16)] = \
                ForwardSpec(named.get("entry"), named.get("signature"))
    bridged = {f.address: f for members in functions.values() for f in members}
    return [Unit(ROOT / source, members, dict(sorted(forwards.get(source, {}).items())), bridged, tables, data)
            for source, members in sorted(functions.items())]


@dataclass
class Unit:
    """One generated unit: a matched file's slice with its bindings."""
    source: Path
    functions: list[Function]
    forward_entries: dict[int, ForwardSpec]   # callee address -> what the spec says of its forward
    bridged: dict[int, Function]              # every bridged function: a forward to one jumps to its generated entry
    tables: Tables
    data: PlayableData

    @property
    def path(self) -> Path:
        return OUT / f"{self.source.parent.name}_{self.source.stem}.cpp"

    @cached_property
    def plan(self) -> Plan:
        """The slice and everything it binds outside its unit."""
        original = self.source.read_text(encoding="utf-8")
        wanted = {f.address for f in self.functions}
        sliced, kept = select(original, wanted)
        if kept != wanted:
            raise ValueError(f"{self.source}: no annotated definition for "
                             f"{sorted(f'{a:08X}' for a in wanted - kept)}")
        system: list[str] = []
        body = flatten(inline_helpers(sliced), self.source, set(), system)
        forwards = [self.forward(a, spec, body) for a, spec in self.forward_entries.items()]
        body, bound, unbound = self.bind_globals(body)
        tables = {f.address: s for f in self.functions if f.kind != "member" and (s := self.vtable(f))}
        for owner in sorted({f.owner for f in self.functions if f.address in tables}):
            body = without_vtable(body, owner, self.source)
        return Plan(original, body, system, forwards, bound, unbound, tables, heap_calls(self.source, body))

    def text(self) -> str:
        p = self.plan
        system, tables = list(p.system), p.tables
        globals_ = p.globals
        storages = list(dict.fromkeys([*globals_, *tables.values()]))
        if any(f.kind != "member" or f.hidden for f in self.functions):
            system.append("<new>")
        if tables or globals_ or any(f.kind != "member" for f in self.functions):
            system.append("<type_traits>")
        if p.heap:
            system += ["<cstddef>", "<cstdint>"]
        if any("std::" in s.variable.type for s in storages):
            system.append("<cstdint>")
        namespace = f"kotor_bridge_{self.source.parent.name}_{self.source.stem}"
        original, body, forwards, bound, unbound = p.original, p.body, p.forwards, p.bound, p.unbound
        return "\n".join([
            f"// GENERATED by bridge_sources.py from {self.source.relative_to(ROOT).as_posix()} "
            f"(sha256 {hashlib.sha256(original.encode()).hexdigest()[:16]}). Do not edit:",
            "// change the matched source or src/bridge/bridge.json and regenerate. The byte-identical matched code",
            f'// lives in namespace {namespace}; only the extern "C" entries at its end are meant to be called.',
            *(f"// global {name} -> {s.qualified} ({s.address:08X})" if isinstance(s, Storage) else
              f"// constant {name} = {s.value} ({s.address:08X}, {s.agreement})" for name, s in bound),
            *(f"// vtable {f.owner} -> {s.qualified} ({s.address:08X})"
              for f in self.functions if (s := tables.get(f.address))),
            *(f"// unbound {name}: {why} (the link fails if it is used)" for name, why in unbound),
            *(f"#include {name}" for name in dict.fromkeys(system)),
            *([HELPERS] if globals_ else []),
            *(s.declaration() for s in storages),
            *(f'extern "C" {f.function.prototype(f.symbol)};' for f in forwards),
            *([GAME_HEAP_DECLARATIONS] if p.heap else []),
            f"namespace {namespace} {{",
            *(GAME_HEAP[name] for name in p.heap),
            f"#pragma warning(push)\n#pragma warning(disable: {' '.join(map(str, MATCHED_WARNINGS))})",
            body.rstrip(),
            "#pragma warning(pop)",
            "",
            *(f.function.forward_definition(f.symbol) for f in forwards),
            *(f.entry_definition(tables[f.address].qualified if tables.get(f.address) else None)
              for f in sorted(self.functions, key=lambda f: f.address)),
            f"}}  // namespace {namespace}",
            "",
        ])

    def forward(self, address: int, spec: ForwardSpec, body: str) -> Forward:
        """A callee of a bridged function, defined as a jump to the playable entry for it: a bridged function's
        generated entry, or the one extern "C" definition in src/ of the name, which must keep the original's frame."""
        callee = Function.parse(address, spec.signature or self.tables.signature(address, body))
        if spec.signature and not declared(callee, body):
            raise ValueError(f"{address:08X}: the slice declares no {spec.signature}")
        name = spec.entry
        if "<" in callee.owner:
            raise ValueError(f"{address:08X}: {callee.owner} is a template; its member cannot be forwarded")
        if callee.kind != "member" and (why := implicit_work(callee.owner, callee.kind, body)):
            raise ValueError(f"{address:08X}: {callee.kind} not forwarded: {why}")
        symbol = name or callee.entry
        if address in self.bridged and name is None:
            target = self.bridged[address]
            if (target.stack, target.result) != (callee.stack, callee.result) or callee.stack is None:
                raise ValueError(f"{address:08X}: this slice declares {callee.owner}::{callee.name} with "
                                 f"{callee.stack} stack bytes and its result in {callee.result}, the bridged entry "
                                 f"{target.stack} and {target.result}")
            return Forward(callee, symbol)
        found = self.data.functions(symbol)
        if len(found) != 1:
            raise ValueError(f"Forward target {symbol} needs exactly one extern \"C\" definition in src/, "
                             f"found {len(found)}")
        why = compatible(callee, found[0])
        if why:
            raise ValueError(f"{address:08X}: {why}")
        return Forward(callee, symbol)

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
                          f"{constant_type(v.type)} {v.name} = {target.value};" if isinstance(target, Constant) else
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
        # in .rdata the original cannot write it, whatever the matched declaration says; the copy is made const
        read_only = address < WRITABLE and not v.bounds
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
