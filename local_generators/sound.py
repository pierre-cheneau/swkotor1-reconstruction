"""D13: the original .rdata items the sound donors require, as a local C++ unit.

DATA-RECEIPT.json records each item's address, file offset, length and
sha256. Neither file is ever committed.
"""
from __future__ import annotations

import json
import struct
from pathlib import Path

from .original import EXE_SHA256, OriginalImage, byte_list, sha256, write_text

STRINGS = (('sound_stream_play29_required', 'format_0074c304', 0x74C304),
           ('sound_stream_play29_required', 'format_0074a808', 0x74A808),
           ('sound_stream_play29_required', 'format_0074a7f4', 0x74A7F4),
           ('sound_stream_play29_required', 'format_0074d33c', 0x74D33C))
FLOATS = (('sound_volume29_required', 'one_0073d764', 0x73D764),
          ('sound_volume29_required', 'scale_0073ee00', 0x73EE00))
BLOBS = (('kotor_sound_data', 'eax_presets_0074c610', 0x74C610, 24 * 0x70),)


def sound(image: OriginalImage, out: Path) -> None:
    lines = ["// GENERATED LOCALLY from the user's swkotor.exe (sha256 %s) by gen_sound_data.py." % EXE_SHA256,
             '// D13: original .rdata bytes; do not commit or publish.', '']
    receipt = {'exe': str(image.path), 'exe_sha256': EXE_SHA256, 'items': []}

    def item(namespace, name, va, raw, **extra):
        receipt['items'].append({'symbol': '%s::%s' % (namespace, name), 'va': '%08X' % va,
                                 'file_offset': image.file_offset(va), 'length': len(raw),
                                 'sha256': sha256(raw), **extra})

    for namespace, name, va in STRINGS:
        raw = image.cstring(va)
        lines.append('namespace %s { extern const char %s[%d] = { %s }; }' % (namespace, name, len(raw), byte_list(raw)))
        item(namespace, name, va, raw, text=raw[:-1].decode('latin1'))
    for namespace, name, va in FLOATS:
        raw = image.read(va, 4)
        bits, value = struct.unpack('<I', raw)[0], struct.unpack('<f', raw)[0]
        lines.append('namespace %s { extern const float %s = %rf; static_assert(sizeof(float) == 4, "x86"); } '
                     '// bits %08X' % (namespace, name, value, bits))
        item(namespace, name, va, raw, bits='%08X' % bits)
    for namespace, name, va, size in BLOBS:
        raw = image.read(va, size)
        lines.append('namespace %s { alignas(4) extern const unsigned char %s[%d] = { %s }; }'
                     % (namespace, name, size, byte_list(raw)))
        item(namespace, name, va, raw)
    out.mkdir(parents=True, exist_ok=True)
    write_text(out / 'sound_data.cpp', '\n'.join(lines) + '\n', '\r\n')
    write_text(out / 'DATA-RECEIPT.json', json.dumps(receipt, indent=1), '\r\n')
