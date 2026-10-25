"""Find functions from a COFF static library (.lib) inside swkotor.exe.

For every code section in every member object, split it at its function
symbols, mask bytes covered by relocations, and search the exe's .text for
an exact masked match. Prints a CSV of (exe address, symbol, size, object).

usage: python matching/tools/libmatch.py <msvc7>/LIB/LIBCMT.LIB <swkotor.exe> out.csv

matching/crt_libcmt.csv keeps the hits at 0x006F0000 and up with a single
placement (LIBCMT, LIBCPMT, LIBCIMT). Functions under 6 bytes are skipped.
"""
import csv
import sys
from collections import defaultdict
from pathlib import Path

import pefile

from coff import CODE, DIR32, DIR32NB, EXTERNAL, I386, REL32, STATIC, CoffObject

MIN_SIZE = 6


def ar_members(data):
    assert data[:8] == b"!<arch>\n"
    pos, longnames = 8, b""
    while pos + 60 <= len(data):
        header = data[pos:pos + 60]
        name = header[:16].decode("latin-1").strip()
        size = int(header[48:58].decode().strip())
        body = data[pos + 60:pos + 60 + size]
        if name == "//":
            longnames = body
        elif name not in ("/", ""):
            if name.startswith("/") and name[1:].isdigit():
                off = int(name[1:])
                end = longnames.find(b"\0", off)
                name = longnames[off:end if end >= 0 else None].decode("latin-1").rstrip("/\n")
            yield name.rstrip("/"), body
        pos += 60 + size + (size & 1)


def coff_functions(obj):
    """[(symbol, code bytes, masked offset set)] for functions in code sections."""
    coff = CoffObject(obj)
    if coff.machine != I386 or not coff.symbols:
        return []
    per_section = defaultdict(list)
    for s in coff.symbols.values():
        # External symbols of any type, static ones only when they are functions.
        if s.section > 0 and (s.storage_class == EXTERNAL or (s.storage_class == STATIC and s.type >> 4 == 2)):
            per_section[s.section].append((s.value, s.name))
    out = []
    for number, symbols in per_section.items():
        section = coff.sections[number - 1]
        if not section.characteristics & CODE or not section.initialized:
            continue
        masked = {r.offset + k for r in section.relocations if r.type in (DIR32, DIR32NB, REL32) for k in range(4)}
        starts = sorted(set(symbols))
        for k, (value, name) in enumerate(starts):
            end = starts[k + 1][0] if k + 1 < len(starts) else len(section.data)
            if end - value >= MIN_SIZE:
                out.append((name, section.data[value:end], {m - value for m in masked if value <= m < end}))
    return out


def main():
    lib, exe, out = sys.argv[1:4]
    pe = pefile.PE(exe, fast_load=True)
    text = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
    base = pe.OPTIONAL_HEADER.ImageBase + text.VirtualAddress
    image = text.get_data()
    data = Path(lib).read_bytes()
    rows, total, objs = [], 0, 0
    for member, body in ar_members(data):
        objs += 1
        for name, code, masked in coff_functions(body):
            total += 1
            # anchor: longest unmasked run
            best, run_start = (0, 0), None
            for j in range(len(code) + 1):
                if j < len(code) and j not in masked:
                    run_start = j if run_start is None else run_start
                else:
                    if run_start is not None and j - run_start > best[0]:
                        best = (j - run_start, run_start)
                    run_start = None
            length, anchor = best
            if length < 4:
                continue
            needle = code[anchor:anchor + min(length, 32)]
            hits = []
            pos = image.find(needle)
            while pos >= 0 and len(hits) < 4:
                start = pos - anchor
                if start >= 0 and all(image[start + b] == code[b] for b in range(len(code))
                                      if b not in masked):
                    hits.append(start)
                pos = image.find(needle, pos + 1)
            for h in hits:
                rows.append((f"0x{base + h:08X}", name, len(code), member.split("\\")[-1],
                             len(hits)))
    rows.sort()
    with open(out, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["address", "symbol", "size", "object", "hits"])
        w.writerows(rows)
    print(f"{lib}: {objs} objects, {total} functions, {len(rows)} placements, "
          f"{len({r[0] for r in rows})} distinct exe addresses")


if __name__ == "__main__":
    main()
