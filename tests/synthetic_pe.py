"""Synthetic stand-ins for the original game files, for tests only.

The generators read the user's own swkotor.exe, Mss32.dll and binkw32.dll,
which this repository never contains. These builders write PE images with
the original's section layout and made-up contents that satisfy every check
the generators make: NUL terminators where a spec gives a string's size,
zero where a cell must be zero, 7-bit bytes where a literal must be ASCII.
No byte comes from the original.

Tests swap the pinned hashes for the synthetic image's own, so they prove
that the code paths produce the same output, not that the data is right.
"""
from __future__ import annotations

import json
import random
import re
import struct
from pathlib import Path

IMAGE_BASE = 0x400000
ALIGNMENT = 0x1000
# (name, virtual address, virtual size, raw size, characteristics) of the
# GOG 1.03 swkotor.exe; game_prepare.py and rules.py rely on these offsets.
ORIGINAL_SECTIONS = (
    (b".text", 0x401000, 0x33C000, 0x33C000, 0x60000020),
    (b".rdata", 0x73D000, 0x4FBFE, 0x50000, 0x40000040),
    (b".data", 0x78D000, 0xA8498, 0x17000, 0xC0000040),
)
RESOURCE_VA = 0x836000

# Addresses the generators hard-code rather than read from a spec.
DIAGNOSTIC_LITERALS = (0x0074E208, 0x0074E360, 0x0074E2A4, 0x0074E264)
TEXTURE_LITERAL = (0x0073EE34, 12)
UPGRADE_TABLE = 0x00756FB0
UPGRADE_STRINGS = 0x0078A000
ZERO_CELLS = (0x7B9274, 0x7A3C7C, 0x7A3C80, 0x7A3C64,      # game_prepare floats
              0x7A39F0, 0x7A3A0C, 0x7A3A28)                # game_prepare pointer cells
SHARED_EMPTY_STRING = 0x73D71C


class Layout:
    """Byte requirements on the image, checked for conflicts as they are added."""

    def __init__(self) -> None:
        self.fixed: dict[int, int] = {}
        self.no_nul: set[int] = set()

    def set(self, va: int, value: int) -> None:
        if self.fixed.get(va, value) != value or (value == 0 and va in self.no_nul):
            raise ValueError(f"conflicting synthetic requirement at {va:08X}")
        self.fixed[va] = value

    def string(self, va: int, size: int) -> None:
        """A C string of exactly size bytes, NUL included."""
        for a in range(va, va + size - 1):
            if self.fixed.get(a) == 0:
                raise ValueError(f"conflicting synthetic requirement at {a:08X}")
            self.no_nul.add(a)
        self.set(va + size - 1, 0)

    def word(self, va: int, value: int) -> None:
        for i, byte in enumerate(struct.pack("<I", value)):
            self.set(va + i, byte)


def _specs(generators: Path, *names: str) -> list[dict]:
    return [item for name in names
            for item in json.loads((generators / name).read_text(encoding="utf-8"))]


def original_layout(generators: Path) -> Layout:
    layout = Layout()
    for item in _specs(generators, "renderer-spec.json", "mesh-spec.json", "epoch-spec.json",
                       "window-spec.json", "vp-data-spec.json", "vp-rdata-spec.json"):
        if item["kind"] == "cstring" and "size" in item:
            layout.string(int(item["va"], 16), item["size"])
    for item in _specs(generators, "resource-pairs-spec.json"):
        address = int(item["original_address"], 16)
        if address != SHARED_EMPTY_STRING:
            layout.string(address, item["size"])
    layout.set(SHARED_EMPTY_STRING, 0)
    for va in DIAGNOSTIC_LITERALS:
        layout.string(va, 13)
    layout.string(*TEXTURE_LITERAL)
    for i in range(16):
        pointer = UPGRADE_STRINGS + 32 * i
        layout.word(UPGRADE_TABLE + 12 * i, (i - 1) & 0xFFFFFFFF)   # kind is signed; -1 included
        layout.word(UPGRADE_TABLE + 12 * i + 4, pointer)
        layout.word(UPGRADE_TABLE + 12 * i + 8, 1000 + i)
        layout.string(pointer, 11)
    for va in ZERO_CELLS:
        for a in range(va, va + 4):
            layout.set(a, 0)
    return layout


def _fill(va: int, raw_size: int, layout: Layout, rng: random.Random, text: bool) -> bytes:
    data = bytearray(rng.randbytes(raw_size)) if text else bytearray(
        rng.choice(b" !#$%&()*+,-./0123456789:;<=>@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}~\"\\?")
        for _ in range(raw_size))
    if not text:
        # Frequent terminators keep unsized strings short; never inside a sized one.
        for offset in range(0, raw_size, 97):
            if va + offset not in layout.no_nul:
                data[offset] = 0
        data[-1] = 0
    for offset in range(raw_size):
        value = layout.fixed.get(va + offset)
        if value is not None:
            data[offset] = value
        elif data[offset] == 0 and va + offset in layout.no_nul:
            data[offset] = 0x41
    return bytes(data)


def _resource_directory(rva: int, resources: dict[int, dict[int, dict[int, bytes]]]) -> bytes:
    """IMAGE_RESOURCE_DIRECTORY tree: type -> id -> language -> data, ids only."""
    tables = bytearray()
    leaves: list[tuple[int, bytes]] = []          # (entry slot to patch, data)

    def directory(children: dict, depth: int) -> int:
        position = len(tables)
        tables.extend(struct.pack("<IIHHHH", 0, 0, 0, 0, 0, len(children)))
        slots = len(tables)
        tables.extend(bytes(8 * len(children)))
        for k, (key, child) in enumerate(sorted(children.items())):
            slot = slots + 8 * k
            struct.pack_into("<I", tables, slot, key)
            if depth < 2:
                struct.pack_into("<I", tables, slot + 4, directory(child, depth + 1) | 0x80000000)
            else:
                leaves.append((slot + 4, child))
        return position

    directory(resources, 0)
    entries, blobs = bytearray(), bytearray()
    data_base = len(tables) + 16 * len(leaves)
    for k, (slot, blob) in enumerate(leaves):
        struct.pack_into("<I", tables, slot, len(tables) + 16 * k)
        entries += struct.pack("<IIII", rva + data_base + len(blobs), len(blob), 0, 0)
        blobs += blob + bytes(-len(blob) % 8)
    return bytes(tables + entries + blobs)


def _export_directory(rva: int, dll_name: str, names: list[str], function_rva: int) -> bytes:
    names = sorted(names)
    header = 40
    functions = header
    name_pointers = functions + 4 * len(names)
    ordinals = name_pointers + 4 * len(names)
    strings = ordinals + 2 * len(names)
    pool = bytearray()
    name_offsets = []
    for name in [dll_name, *names]:
        name_offsets.append(strings + len(pool))
        pool += name.encode("ascii") + b"\0"
    body = bytearray(struct.pack("<IIHHIIIIIII", 0, 0, 0, 0, rva + name_offsets[0], 1, len(names),
                                 len(names), rva + functions, rva + name_pointers, rva + ordinals))
    body += b"".join(struct.pack("<I", function_rva + 16 * i) for i in range(len(names)))
    body += b"".join(struct.pack("<I", rva + o) for o in name_offsets[1:])
    body += b"".join(struct.pack("<H", i) for i in range(len(names)))
    return bytes(body + pool)


def _pe(sections: list[tuple[bytes, int, int, bytes, int]], directories: dict[int, tuple[int, int]],
        dll: bool, image_base: int = IMAGE_BASE) -> bytes:
    """sections: (name, virtual address, virtual size, raw data, characteristics)."""
    headers = bytearray(ALIGNMENT)
    headers[:2] = b"MZ"
    struct.pack_into("<I", headers, 0x3C, 0x80)
    headers[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", headers, 0x84, 0x14C, len(sections), 0, 0, 0, 0xE0,
                     0x2102 if dll else 0x0102)
    last = sections[-1]
    size_of_image = last[1] - image_base + -(-last[2] // ALIGNMENT) * ALIGNMENT
    struct.pack_into("<HBBIIIIIIIIIHHHHHHIIIIHHIIIIII", headers, 0x98, 0x10B, 7, 0,
                     sections[0][2], 0, 0, sections[0][1] - image_base, sections[0][1] - image_base, 0,
                     image_base, ALIGNMENT, ALIGNMENT, 4, 0, 0, 0, 4, 0, 0, size_of_image, ALIGNMENT,
                     0, 2, 0, 0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    for index, (rva, size) in directories.items():
        struct.pack_into("<II", headers, 0x98 + 96 + 8 * index, rva, size)
    body = bytearray()
    raw_pointer = ALIGNMENT
    for k, (name, va, vsize, raw, characteristics) in enumerate(sections):
        raw = raw + bytes(-len(raw) % ALIGNMENT)
        struct.pack_into("<8sIIIIIIHHI", headers, 0x178 + 40 * k, name, vsize, va - image_base,
                         len(raw), raw_pointer, 0, 0, 0, 0, characteristics)
        body += raw
        raw_pointer += len(raw)
    return bytes(headers + body)


def original_resources() -> dict[int, dict[int, dict[int, bytes]]]:
    rng = random.Random(3)
    return {kind: {item: {1033: rng.randbytes(9 + 7 * item)} for item in (1, 2, 7)}
            for kind in (1, 3, 12, 14, 16, 24)}      # 24 (manifest) is not copied


def write_original(path: Path, generators: Path, seed: int = 1) -> bytes:
    """Write a synthetic swkotor.exe and return its bytes."""
    layout = original_layout(generators)
    rng = random.Random(seed)
    sections = [(name, va, vsize, _fill(va, raw_size, layout, rng, name == b".text"), flags)
                for name, va, vsize, raw_size, flags in ORIGINAL_SECTIONS]
    resource = _resource_directory(RESOURCE_VA - IMAGE_BASE, original_resources())
    sections.append((b".rsrc", RESOURCE_VA, len(resource), resource, 0x40000040))
    image = _pe(sections, {2: (RESOURCE_VA - IMAGE_BASE, len(resource))}, dll=False)
    path.write_bytes(image)
    return image


def write_dll(path: Path, exports: list[str], seed: int) -> bytes:
    """Write a synthetic x86 DLL exporting the given names."""
    rng = random.Random(seed)
    text_va, export_va = 0x10001000, 0x10002000
    image_base = 0x10000000
    code = rng.randbytes(16 * len(exports) + 16)
    table = _export_directory(export_va - image_base, path.name, exports, text_va - image_base)
    image = _pe([(b".text", text_va, len(code), code, 0x60000020),
                 (b".edata", export_va, len(table), table, 0x40000040)],
                {0: (export_va - image_base, len(table))}, dll=True, image_base=image_base)
    path.write_bytes(image)
    return image


def write_code_image(path: Path, code: bytes) -> None:
    """An executable whose .text, at the original's 00401000, holds code."""
    path.write_bytes(_pe([(b".text", 0x401000, len(code), code, 0x60000020)], {}, dll=False))


MSS32_EXPORTS = ["_AIL_quick_handles@12", "_AIL_startup@0", "AIL_debug_printf", "_AIL_set_preference@8"]
BINK_EXPORTS = ["_BinkOpen@8", "_BinkClose@4", "_BinkDoFrame@4", "_BinkSetVolume@12"]
