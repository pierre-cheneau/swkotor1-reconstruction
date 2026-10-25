"""Read the playable build's src/: which original addresses it defines or cites.

src/ names each reconstructed function after its original address
(kotor_<cc>_<address>, or any identifier ending in _<address>) and cites
addresses in comments and calls. todo.py and render_banner.py both use this.
"""
from __future__ import annotations

import re
from collections.abc import Iterator
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_SUFFIXES = {".cpp", ".h", ".hpp", ".inc"}
# A definition: identifier ending in _<address>, a parameter list, then the body.
DEFINITION = re.compile(
    r"\b[A-Za-z_]\w*?_(00[4-7][0-9a-fA-F]{5})\s*\(([^;{}()]|\([^()]*\))*\)\s*"
    r"(?:const\s*)?(?:noexcept\s*)?(?:->\s*[\w:<>*& ]+)?\{")
# Where a definition can end its identifier: the address, then "(".
CANDIDATE = re.compile(r"_00[4-7][0-9a-fA-F]{5}\s*\(")
IDENTIFIER_START = re.compile(r"[A-Za-z_]")
CITED = re.compile(r"(?<![0-9A-Fa-fx])(00[4-7][0-9A-Fa-f]{5})(?![0-9A-Fa-f])")


def definitions(text: str) -> Iterator[re.Match[str]]:
    """Exactly the matches of DEFINITION.finditer(text), found much faster.

    A match starts at a word boundary and runs through word characters to
    its address, which "(" follows, so it can only start where the word
    holding a CANDIDATE begins (there is no boundary inside a word). Scanning
    for the literal candidates and matching only there replaces trying every
    word in the file; like finditer, no match starts inside an earlier one.
    """
    end = 0
    for candidate in CANDIDATE.finditer(text):
        start = candidate.start()
        while start > 0 and (text[start - 1].isalnum() or text[start - 1] == "_"):   # re's \w
            start -= 1
        if start < end or not IDENTIFIER_START.match(text[start]):
            continue
        m = DEFINITION.match(text, start)
        if m:
            end = m.end()
            yield m


@dataclass(frozen=True)
class Scan:
    path: Path
    cited: frozenset[int]
    definitions: tuple[tuple[int, int], ...]      # (address, line), in file order


def scan_file(path: Path) -> Scan:
    text = path.read_text(encoding="utf-8", errors="replace")
    # src/bridge/ holds generated slices of matched sources: only their kotor_fastcall_ entries define
    # functions, and the addresses the matched code mentions are not playable citations.
    bridge = path.parent.name == "bridge" and path.parent.parent.name == "src"
    found, line, counted = [], 1, 0
    for m in definitions(text):
        line += text.count("\n", counted, m.start())
        counted = m.start()
        if not bridge or m.group(0).startswith("kotor_fastcall_"):
            found.append((int(m.group(1), 16), line))
    cited = frozenset() if bridge else frozenset(int(a, 16) for a in set(CITED.findall(text)))
    return Scan(path, cited, tuple(found))


def scan(root: Path = ROOT) -> list[Scan]:
    """Every src/ source, in sorted path order, scanned in parallel."""
    paths = sorted(p for p in (root / "src").rglob("*") if p.suffix in SOURCE_SUFFIXES)
    with ProcessPoolExecutor() as pool:
        return list(pool.map(scan_file, paths, chunksize=128))


def definition_index(scans: list[Scan]) -> dict[int, tuple[Path, int]]:
    """address -> (file, line) of the first src/ definition."""
    found: dict[int, tuple[Path, int]] = {}
    for s in scans:
        for address, line in s.definitions:
            found.setdefault(address, (s.path, line))
    return found


def coverage(scans: list[Scan]) -> dict[int, int]:
    """Original address -> 2 if src/ defines a function under that address,
    1 if src/ only cites it (inlined, folded into a caller, or shared with an
    identical function)."""
    result = dict.fromkeys(set().union(*(s.cited for s in scans)), 1)
    result.update((address, 2) for s in scans for address, _ in s.definitions)
    return result
