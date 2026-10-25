"""Generate src/bridge/: byte-identical matched functions compiled into the playable build.

    uv run bridge_sources.py            regenerate src/bridge/ and its manifest units and pins
    uv run bridge_sources.py --check    exit 1 if a generated file or bridge unit is missing, differs or is stale

This is phase 2 of STRUCTURE.md. For every address the spec names, the matched
source (matching/src, status MATCH) is sliced to the preamble its file declares
plus the selected functions, with its matching/include headers inlined. The
slice sits in a namespace of its own, so none of its classes, inline functions
or vtables can merge with another unit's copy. It adds one extern "C" entry per
function, named kotor_fastcall_<address> as the playable build names original
thiscall functions. A bridged function's calls outside its unit are the forwards
its spec entry names. The generator checks each against the playable definition
it calls: parameter count and whether a value is returned. Any other outside
function or global the slice uses stays unresolved and fails the link.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

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
SIGNATURE = re.compile(r"^public: (?:virtual )?(?P<ret>.+?) __thiscall (?P<owner>.+)::(?P<name>\w+)"
                       r"\((?P<params>.*)\)(?P<const>const)?\s*$")
ANNOTATION = re.compile(r"^// FUNCTION: SWKOTOR 0x([0-9A-Fa-f]{8})\b")
DIRECTIVE = re.compile(r"^\s*#\s*(?P<word>\w+)(?P<rest>.*)$")
BUILTIN = {"void", "bool", "char", "short", "int", "long", "float", "double", "__int64", "unsigned", "signed", "const"}


@dataclass(frozen=True)
class Function:
    """A public thiscall member function, from its demangled MSVC signature."""
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
        params = found["params"].strip()
        return cls(address, plain(found["owner"]), found["name"], plain(found["ret"]),
                   () if params == "void" else tuple(map(plain, split_params(params))), bool(found["const"]))

    @property
    def entry(self) -> str:
        return f"kotor_fastcall_{self.address:08x}"

    @property
    def returns_value(self) -> bool:
        return self.returns != "void"

    def prototype(self) -> str:
        """The extern "C" entry: thiscall as fastcall with the unused EDX, as the playable build declares it."""
        params = ", ".join(["void* self", "void*", *(f"{abi(t)} a{i}" for i, t in enumerate(self.params))])
        return f"{abi(self.returns)} __fastcall {self.entry}({params})"

    def entry_definition(self) -> str:
        args = ", ".join(argument(t, f"a{i}") for i, t in enumerate(self.params))
        call = f"static_cast<{self.owner}*>(self)->{self.name}({args})"
        return f'extern "C" {self.prototype()}\n{{\n    {"return " if self.returns_value else ""}{call};\n}}\n'

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
        end = definition_end(text, offsets[i + 1])
        j = next(k for k in range(i + 1, len(offsets)) if offsets[k] >= end)
        if int(found[1], 16) in keep:
            out.extend(lines[i:j])
            kept.add(int(found[1], 16))
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


# ---------------------------------------------------------------- generation

def playable_entry(name: str) -> tuple[int, bool]:
    """(parameter count, returns a value) of the playable build's one definition of an extern "C" entry."""
    pattern = re.compile(r'extern\s+"C"\s+(?P<ret>[\w:<>* ]+?)\s+__fastcall\s+' + name
                         + r"\s*\((?P<params>[^;{}]*)\)\s*\{")
    found = [m for p in (ROOT / "src").rglob("*.cpp") if OUT not in p.parents
             for m in pattern.finditer(p.read_text(encoding="utf-8", errors="replace"))]
    if len(found) != 1:
        raise ValueError(f"Forward target {name} needs exactly one playable definition, found {len(found)}")
    params = found[0]["params"].strip()
    return (0 if params in {"", "void"} else len(split_params(params))), found[0]["ret"].strip() != "void"


def forward(address: int, signatures: dict[int, set[str]]) -> Function:
    """A callee of a bridged function, defined by calling the playable build's entry for it."""
    callee = Function.parse(address, signature(address, signatures))
    if playable_entry(callee.entry) != (len(callee.params) + 2, callee.returns_value):
        raise ValueError(f"{callee.entry}: the playable definition does not have the original's signature")
    return callee


def signature(address: int, signatures: dict[int, set[str]]) -> str:
    """The one demangled signature symbols.csv knows for an address."""
    known = signatures.get(address, set())
    if len(known) != 1:
        raise ValueError(f"{address:08X}: symbols.csv has {len(known)} signatures, the bridge needs exactly one")
    return next(iter(known))


def generate() -> dict[Path, str]:
    """{generated file: text} for the spec."""
    spec = json.loads(SPEC.read_text(encoding="utf-8"))
    with (MATCHING / "status.csv").open(encoding="utf-8") as rows:
        status = {int(r["address"], 16): r for r in csv.DictReader(rows)}
    signatures: dict[int, set[str]] = {}
    with (MATCHING / "symbols.csv").open(encoding="utf-8") as rows:
        for r in csv.DictReader(rows):
            if r["role"] in {"defines", "calls"}:
                signatures.setdefault(int(r["address"], 16), set()).add(r["demangled"])
    functions: dict[str, list[Function]] = {}
    forwards: dict[str, dict[int, Function]] = {}
    for item in spec["functions"]:
        address = int(item["address"], 16)
        row = status.get(address)
        if not row or row["status"] != "MATCH":
            raise ValueError(f"{address:08X} is not a byte-identical (MATCH) function")
        functions.setdefault(row["file"], []).append(Function.parse(address, signature(address, signatures)))
        for callee in (int(a, 16) for a in item.get("forwards", [])):
            forwards.setdefault(row["file"], {})[callee] = forward(callee, signatures)
    return {OUT / f"{Path(source).parent.name}_{Path(source).stem}.cpp":
            unit(ROOT / source, members, list(forwards.get(source, {}).values()))
            for source, members in sorted(functions.items())}


def unit(source: Path, functions: list[Function], forwards: list[Function]) -> str:
    text = source.read_text(encoding="utf-8")
    wanted = {f.address for f in functions}
    sliced, kept = select(text, wanted)
    if kept != wanted:
        raise ValueError(f"{source}: no annotated definition for {sorted(f'{a:08X}' for a in wanted - kept)}")
    system: list[str] = []
    body = flatten(sliced, source, set(), system)
    namespace = f"kotor_bridge_{source.parent.name}_{source.stem}"
    return "\n".join([
        f"// GENERATED by bridge_sources.py from {source.relative_to(ROOT).as_posix()} "
        f"(sha256 {hashlib.sha256(text.encode()).hexdigest()[:16]}). Do not edit:",
        "// change the matched source or src/bridge/bridge.json and regenerate. The byte-identical matched code",
        f'// lives in namespace {namespace}; only the extern "C" entries at its end are meant to be called.',
        *(f"#include {name}" for name in dict.fromkeys(system)),
        *(f'extern "C" {f.prototype()};' for f in forwards),
        f"namespace {namespace} {{",
        body.rstrip(),
        "",
        *(f.forward_definition() for f in forwards),
        *(f.entry_definition() for f in sorted(functions, key=lambda f: f.address)),
        f"}}  // namespace {namespace}",
        "",
    ])


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
