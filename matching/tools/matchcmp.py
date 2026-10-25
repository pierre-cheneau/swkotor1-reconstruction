"""Compile annotated matching sources with MSVC 7.0 and compare each function
byte-for-byte against the original swkotor.exe.

Annotate every function that should match the original with a marker comment
directly above its definition:

    // FUNCTION: SWKOTOR 0x00409460
    int CExoResMan::Leaf9460(unsigned long a, int b, int c) { ... }

Bytes covered by a relocation in the compiled object (call targets, global
addresses) are masked on both sides; everything else must be identical, and
the rest of the original function's address range must be padding.

Off Windows the MSVC tools run under Wine: KOTOR_WINE names the launcher
(default "wine"; may be a multi-word command) and paths are passed as Z:\\...

The compiler (Visual Studio .NET 2002, cl 13.00.9466) and the original
executable are not part of this repository; pass their locations with
--msvc/--exe or KOTOR_MSVC7/KOTOR_ORIGINAL_EXE.

usage: python matchcmp.py --msvc DIR --exe swkotor.exe [-v] FILE_OR_DIR...
"""
import argparse
import csv
import difflib
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import pefile

from coff import CoffObject

ROOT = Path(__file__).resolve().parents[2]
WINDOWS = os.name == "nt"
WINE =[] if WINDOWS else shlex.split(os.environ.get("KOTOR_WINE", "wine"))
DEFAULT_FLAGS = "-O2 -Gy -GX"  # dash form: Git Bash rewrites /O2 into a path
ANNOTATION = re.compile(r"//\s*FUNCTION:\s*SWKOTOR\s+(0x[0-9A-Fa-f]+)(?:\s+SYMBOL:\s*(\S+))?")
PADDING = {0x90, 0xCC}


def winpath(path):
    """Path as the MSVC tools see it (Wine maps Z: to the host root)."""
    return str(path) if WINDOWS else "Z:" + str(Path(path).resolve()).replace("/", "\\")


def run_tool(cmd, env=None):
    env = dict(os.environ if env is None else env)
    env.setdefault("WINEDEBUG", "-all")
    return subprocess.run([*WINE, *cmd], capture_output=True, text=True, env=env)


def start_wineserver():
    """Keep one wineserver alive across tool runs (~0.2 s each instead of ~4 s).

    The server and the prefix services (started by the first Wine process)
    outlive their parent holding its stdio, so both start here uncaptured;
    otherwise the first captured tool run would block until they exit."""
    launcher = shutil.which(WINE[0]) if WINE else None
    server = Path(launcher).resolve().parent / "wineserver" if launcher else None
    if server and server.exists():
        quiet = dict(stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                     stderr=subprocess.DEVNULL, env=dict(os.environ, WINEDEBUG="-all"))
        subprocess.run([str(server), "-p", "60"], **quiet)
        subprocess.run([*WINE, "wineboot"], **quiet)


def annotated_functions(path):
    """[(address, qualified name)] for each annotation in a source file."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    found = []
    for i, line in enumerate(lines):
        m = ANNOTATION.search(line)
        if not m:
            continue
        if m.group(2):  # explicit mangled or undecorated symbol
            found.append((int(m.group(1), 16), m.group(2)))
            continue
        signature = ""
        for follow in lines[i + 1:i + 8]:
            if follow.strip().startswith("//"):
                continue
            signature += " " + follow.strip()
            if "(" in signature:
                break
        head = signature.split("(")[0].split()
        found.append((int(m.group(1), 16), head[-1].lstrip("*&") if head else "?"))
    return found


def compile_source(path, flags, outdir, msvc):
    """Compile one source with the MSVC 7.0 installation at msvc; returns the object."""
    obj = Path(outdir) / (path.stem + ".obj")
    env = dict(os.environ)
    env["PATH"] = str(msvc / "BIN") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(msvc / "INCLUDE")
    # Under Wine the SDK headers go on the command line instead (searched
    # after matching/include, as INCLUDE would be), so any launcher works.
    sdk = [] if WINDOWS else ["/I", winpath(msvc / "INCLUDE")]
    cmd = [winpath(msvc / "BIN" / "CL.EXE"), "/nologo", "/c", *flags.split(),
           "/I", winpath(ROOT / "matching" / "include"), *sdk,
           f"/Fo{winpath(obj)}", winpath(path)]
    run = run_tool(cmd, env)
    if run.returncode != 0:
        sys.exit(f"compile failed: {path}\n{run.stdout}{run.stderr}")
    return obj


def read_coff(obj):
    """{symbol name: (code bytes, set of masked offsets)} for function COMDATs."""
    coff = CoffObject.read(obj)
    funcs = {}
    for symbol in coff.defined_functions():
        section = coff.section_of(symbol)
        masked = {r.offset + k for r in section.relocations for k in range(4)}
        funcs[symbol.name] = (section.data[symbol.value:], masked)
    return funcs


def demangle(names, msvc):
    """Undecorated names, via UNDNAME in chunks (the command line caps at ~32K)."""
    result = {}
    for start in range(0, len(names), 100):
        chunk = names[start:start + 100]
        run = run_tool([winpath(msvc / "BIN" / "UNDNAME.EXE"), *chunk])
        out = re.findall(r'is :- "(.*)"', run.stdout)
        result.update(zip(chunk, out) if len(out) == len(chunk) else ((n, n) for n in chunk))
    return result


def symbols_named(qualname, readable):
    """The compiled symbols an annotation names, from {symbol: undecorated}:
    by undecorated name (followed by its parameters, or as a suffix) or by
    the decorated symbol itself."""
    return [s for s, text in readable.items()
            if f"{qualname}(" in text or text.endswith(qualname) or s == qualname]


def call_targets(pe):
    """Targets of direct calls (E8 rel32) in the original .text that follow
    padding or a ret, i.e. plausible function starts."""
    base = pe.OPTIONAL_HEADER.ImageBase
    text = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
    start = base + text.VirtualAddress
    code = text.get_data()
    end = start + len(code)
    targets = set()
    for i in range(len(code) - 4):
        if code[i] != 0xE8:
            continue
        target = start + i + 5 + struct.unpack_from("<i", code, i + 1)[0]
        j = target - start
        if start < target < end and (code[j - 1] in PADDING or code[j - 1] == 0xC3
                                     or (j >= 3 and code[j - 3] == 0xC2)):
            targets.add(target)
    return targets


def function_extents(pe, table=None):
    """Distance from each function start to the next. Starts are every
    address annotated under matching/src, the original's direct call
    targets and, when given, a CSV function table with an "address" column."""
    starts = call_targets(pe)
    if table:
        with open(table, encoding="utf-8") as rows:
            starts |= {int(r["address"], 16) for r in csv.DictReader(rows)}
    for src in (ROOT / "matching" / "src").rglob("*.cpp"):
        text = src.read_text(encoding="utf-8", errors="replace")
        starts.update(int(m.group(1), 16) for m in ANNOTATION.finditer(text))
    starts = sorted(starts)
    return {a: b - a for a, b in zip(starts, starts[1:])}


def compare(original, compiled, masked, extent):
    """(unmasked offsets that differ, whether the original ends where ours does)."""
    n = len(compiled)
    diffs = [i for i in range(n) if i not in masked and original[i] != compiled[i]]
    # The function table can miss the next function's start; padding followed
    # by more code still counts as the end of this function.
    tail = original[n:extent]
    pad = len(tail) - len(tail.lstrip(bytes(PADDING)))
    tail_ok = pad == len(tail) or (pad > 0 and (n + pad) % 16 == 0)
    if not tail_ok and pad == 0 and n % 16 == 0:
        # Compiled COMDATs are padded to 16; if ours ends in ret/jmp, the
        # original's following bytes start the next (untabled) function.
        insns = disassemble(compiled.rstrip(bytes(PADDING)))
        tail_ok = bool(insns) and insns[-1][2].split()[0] in ("ret", "jmp")
    return diffs, tail_ok


def disassemble(code):
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs
    return [(i.address, i.size, f"{i.mnemonic} {i.op_str}".strip())
            for i in Cs(CS_ARCH_X86, CS_MODE_32).disasm(code, 0)]


def count_bad_instructions(original, compiled, masked):
    """Instructions that differ after aligning by mnemonic sequence (robust to
    a single size change shifting all later offsets)."""
    if not any(i not in masked and original[i] != compiled[i] for i in range(len(compiled))):
        return 0

    def normalized(code):
        return [re.sub(r"0x[0-9a-f]+", "N", text) for _, _, text in disassemble(code)]

    a, b = normalized(original[:len(compiled) + 16]), normalized(compiled)
    sm = difflib.SequenceMatcher(None, a[:len(b)], b, autojunk=False)
    return len(b) - sum(block.size for block in sm.get_matching_blocks())


def side_by_side(original, compiled, masked):
    """Original and compiled instructions in two columns, diffs marked !!."""
    left, right = disassemble(original), disassemble(compiled)
    lines = []
    for k in range(max(len(left), len(right))):
        a = left[k] if k < len(left) else (0, 0, "")
        b = right[k] if k < len(right) else (0, 0, "")
        same = (a[1] == b[1] and all(
            i in masked or original[a[0] + i - b[0]] == compiled[i]
            for i in range(b[0], b[0] + b[1]))) if a[2] and b[2] else False
        lines.append(f"   {'  ' if same else '!!'} {a[0]:04x} {a[2]:<38} | {b[0]:04x} {b[2]}")
    return lines


def check_source(src, flags, pe, extents, verbose, msvc):
    """Compile one source and return [(address, name, ok, diff insns, line)]."""
    wanted = annotated_functions(src)
    if not wanted:
        return []
    with tempfile.TemporaryDirectory() as tmp:
        funcs = read_coff(compile_source(src, flags, tmp, msvc))
    readable = demangle(list(funcs), msvc) if funcs else {}
    results = []
    for address, qualname in wanted:
        hits = symbols_named(qualname, readable)
        if len(hits) != 1:
            results.append((address, qualname, False, -1,
                            f"?? {address:#010x} {qualname}: {len(hits)} symbol matches"))
            continue
        code, masked = funcs[hits[0]]
        extent = extents.get(address, len(code))
        original = pe.get_data(address - pe.OPTIONAL_HEADER.ImageBase,
                               max(extent, len(code)))
        diffs, tail_ok = compare(original, code, masked, extent)
        ok = not diffs and tail_ok
        bad = 0 if ok else count_bad_instructions(original, code, masked)
        line = (f"{'OK' if ok else '--'} {address:#010x} {qualname:<48} "
                f"{bad:3d} diff insns {len(code)}/{extent} bytes"
                + ("" if ok else f" first diff @+{diffs[0]:#x}" if diffs
                   else " (length differs)"))
        if verbose and not ok:
            line += "\n" + "\n".join(side_by_side(original[:max(len(code), extent)], code, masked)).rstrip()
        results.append((address, qualname, ok, bad, line))
    return results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--flags", default=DEFAULT_FLAGS)
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--report", help="write a CSV of every annotated function")
    parser.add_argument("-q", "--quiet", action="store_true", help="only print misses")
    parser.add_argument("--msvc", default=os.environ.get("KOTOR_MSVC7"),
                        help="MSVC 7.0 root containing BIN and INCLUDE")
    parser.add_argument("--exe", default=os.environ.get("KOTOR_ORIGINAL_EXE"),
                        help="original swkotor.exe (GOG 1.03)")
    parser.add_argument("--function-table",
                        default=os.environ.get("KOTOR_FUNCTION_TABLE", ROOT / "matching" / "functions.csv"),
                        help="CSV of function starts (address column); default matching/functions.csv")
    args = parser.parse_args()
    if not args.msvc or not args.exe:
        sys.exit("pass --msvc and --exe (or set KOTOR_MSVC7 and KOTOR_ORIGINAL_EXE)")
    msvc = Path(args.msvc)

    sources = []
    for p in (Path(x).resolve() for x in args.paths):
        sources += sorted(p.rglob("*.cpp")) if p.is_dir() else [p]
    pe = pefile.PE(args.exe, fast_load=True)
    table = args.function_table if args.function_table and Path(args.function_table).exists() else None
    extents = function_extents(pe, table)
    start_wineserver()
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        per_file = list(pool.map(
            lambda s: (s, check_source(s, args.flags, pe, extents, args.verbose, msvc)),
            sources))
    rows, seen, total, matched = [], {}, 0, 0
    for src, results in per_file:
        for address, name, ok, bad, line in results:
            total += 1
            matched += ok
            if not (args.quiet and ok):
                print(line)
            if address in seen:
                print(f"!! {address:#010x} annotated twice: {seen[address]} and {src}")
            seen[address] = src
            rows.append((f"0x{address:08X}", name, "MATCH" if ok else "NONMATCH",
                         bad, src.relative_to(ROOT).as_posix()
                         if src.is_relative_to(ROOT) else str(src)))
    if args.report:
        with open(args.report, "w", newline="") as out:
            writer = csv.writer(out, lineterminator="\n")
            writer.writerow(["address", "name", "status", "diff_insns", "file"])
            writer.writerows(sorted(rows))
    print(f"\n{matched}/{total} functions match byte-for-byte")
    return 0 if matched == total else 1


if __name__ == "__main__":
    sys.exit(main())
