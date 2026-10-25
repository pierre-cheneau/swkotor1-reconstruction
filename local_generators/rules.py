"""D13: the original strings and cells the lane-rules sources name.

Scans the rules sources for kotor_rules_str_XXXXXXXX (original .rdata/.data
C strings) and kotor_rules_cellNN_XXXXXXXX (.data cells) and writes:
  rules_image_data.h    declarations only (no original bytes)
  rules_image_data.cpp  definitions with original bytes (NEVER COMMIT)
  IMAGE-CHECK.json      address, file offset, length, sha256 per value
"""
from __future__ import annotations

import json
import re
from pathlib import Path

from .original import OriginalImage, sha256, write_text

NAME = re.compile(r'kotor_rules_str_([0-9a-f]{8})')
CELL = re.compile(r'kotor_rules_cell(8|16|32)_([0-9a-f]{8})')
CELL_TYPES = {8: 'std::uint8_t', 16: 'std::uint16_t', 32: 'std::uint32_t'}


def references(src: Path) -> tuple[dict[int, set[str]], dict[tuple[int, int], set[str]]]:
    """{string address: files}, {(cell address, bits): files} named in the sources."""
    strings: dict[int, set[str]] = {}
    cells: dict[tuple[int, int], set[str]] = {}
    for path in sorted(src.iterdir(), key=lambda p: p.name):
        if path.suffix not in ('.cpp', '.h') or path.name == 'rules_image_data.h':
            continue
        text = path.read_text(encoding='utf-8')
        for m in NAME.finditer(text):
            strings.setdefault(int(m.group(1), 16), set()).add(path.name)
        for m in CELL.finditer(text):
            cells.setdefault((int(m.group(2), 16), int(m.group(1))), set()).add(path.name)
    return strings, cells


def rules(image: OriginalImage, src: Path, out: Path) -> None:
    strings, cells = references(src)
    check, declarations, definitions = [], [], []
    for va in sorted(strings):
        if image.section(va).name not in ('.rdata', '.data'):
            raise ValueError('%08X not in .rdata/.data' % va)
        data = image.cstring(va)
        consumers = sorted(strings[va])
        check.append({'name': 'kotor_rules_str_%08x' % va, 'address': '%08X' % va,
                      'file_offset': '%X' % image.file_offset(va), 'length': len(data),
                      'sha256': sha256(data), 'consumers': consumers})
        declarations.append('extern const char kotor_rules_str_%08x[%d];  // %s' % (va, len(data), ', '.join(consumers)))
        definitions.append('const char kotor_rules_str_%08x[%d] = {%s};'
                           % (va, len(data), ', '.join('0x%02X' % c for c in data)))
    for (va, bits), files in sorted(cells.items()):
        if image.section(va).name != '.data':
            raise ValueError('%08X not in .data' % va)
        data, offset, source = image.read_loaded(va, bits // 8)
        name = 'kotor_rules_cell%d_%08x' % (bits, va)
        check.append({'name': name, 'address': '%08X' % va, 'file_offset': None if offset is None else '%X' % offset,
                      'length': len(data), 'source': source, 'sha256': sha256(data), 'consumers': sorted(files)})
        declarations.append('extern %s %s;  // %s' % (CELL_TYPES[bits], name, ', '.join(sorted(files))))
        definitions.append('%s %s = 0x%Xu;' % (CELL_TYPES[bits], name, int.from_bytes(data, 'little')))
    out.mkdir(parents=True, exist_ok=True)
    write_text(out / 'rules_image_data.h',
               '#pragma once\n// GENERATED declarations (no original bytes) by tools/gen_rules_image_data.py.\n'
               '// Definitions exist ONLY in the locally generated, never-committed rules_image_data.cpp (D13).\n'
               '#include <cstdint>\nextern "C" {\n' + '\n'.join(declarations) + '\n}\n', '\n')
    write_text(out / 'rules_image_data.cpp',
               '// GENERATED - NEVER COMMIT (D13). tools/gen_rules_image_data.py from swkotor.exe sha256 %s.\n'
               '#include "rules_image_data.h"\nextern "C" {\n%s\n}\n' % (image.sha256, '\n'.join(definitions)), '\n')
    write_text(out / 'IMAGE-CHECK.json', json.dumps({'exe_sha256': image.sha256, 'values': check}, indent=1), '\r\n')
    write_text(out / 'NEVER-COMMIT', 'D13 generated original bytes\n', '\r\n')
