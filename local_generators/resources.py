"""The original executable's resources as a .res file for the linker."""
from __future__ import annotations

import struct
from pathlib import Path

import pefile

from .original import OriginalImage

# Resource type -> memory flags of its .res entry: cursors, icons, their groups, version.
FLAGS = {1: 0x1010, 3: 0x1010, 12: 0x1030, 14: 0x1030, 16: 0x0030}
EMPTY_ENTRY = struct.pack("<II", 0, 32) + struct.pack("<HHHH", 0xFFFF, 0, 0xFFFF, 0) + bytes(16)


def original_resources(image: OriginalImage, output: Path) -> None:
    data = bytearray(EMPTY_ENTRY)
    with pefile.PE(data=image.data) as pe:
        for kind in pe.DIRECTORY_ENTRY_RESOURCE.entries:
            if kind.id not in FLAGS:
                continue
            for item in kind.directory.entries:
                if item.id is None or not 0 <= item.id <= 65535:
                    raise ValueError("Unsupported named/out-of-range original resource")
                for language in item.directory.entries:
                    record = language.data.struct
                    blob = pe.get_data(record.OffsetToData, record.Size)
                    if len(blob) != record.Size:
                        raise ValueError("Truncated original resource")
                    header = struct.pack("<HHHH", 0xFFFF, kind.id, 0xFFFF, item.id)
                    header += struct.pack("<IHHII", 0, FLAGS[kind.id], language.id, 0, 0)
                    entry = struct.pack("<II", len(blob), 8 + len(header)) + header + blob
                    data += entry + bytes(-len(entry) % 4)
    output.write_bytes(data)
