"""Write i386 COFF objects shaped like MSVC's, for tests of the COFF readers."""
from __future__ import annotations

import random
import struct
from dataclasses import dataclass, field

IMAGE_SCN_CNT_CODE = 0x20
REL32, DIR32, DIR32NB, SECREL = 0x14, 0x06, 0x07, 0x0B


@dataclass
class SectionSpec:
    name: str
    data: bytes
    relocations: list[tuple[int, int, int]] = field(default_factory=list)   # (offset, symbol, type)
    characteristics: int = 0x60501020                                     # code, COMDAT, execute/read
    uninitialized: bool = False


@dataclass
class SymbolSpec:
    name: str
    value: int
    section: int               # 1-based; 0 undefined, -1 absolute
    type: int = 0x20           # function
    storage_class: int = 2     # external
    aux: bytes = b""           # auxiliary records, 18 bytes each


def _name_field(name: str, strings: bytearray) -> bytes:
    raw = name.encode("latin-1")
    if len(raw) <= 8:
        return raw.ljust(8, b"\0")
    offset = 4 + len(strings)
    strings += raw + b"\0"
    return struct.pack("<II", 0, offset)


def write(sections: list[SectionSpec], symbols: list[SymbolSpec], machine: int = 0x14C) -> bytes:
    strings = bytearray()
    headers, bodies = bytearray(), bytearray()
    data_start = 20 + 40 * len(sections)
    for spec in sections:
        name = spec.name.encode("latin-1")
        if len(name) > 8:
            name = f"/{4 + len(strings)}".encode()
            strings += spec.name.encode("latin-1") + b"\0"
        raw_pointer = 0 if spec.uninitialized else data_start + len(bodies)
        if not spec.uninitialized:
            bodies += spec.data
        relocation_pointer = data_start + len(bodies) if spec.relocations else 0
        for offset, symbol, kind in spec.relocations:
            bodies += struct.pack("<IIH", offset, symbol, kind)
        headers += struct.pack("<8sIIIIIIHHI", name.ljust(8, b"\0"), 0, 0, len(spec.data), raw_pointer,
                               relocation_pointer, 0, len(spec.relocations), 0, spec.characteristics)
    table = bytearray()
    count = 0
    for spec in symbols:
        assert len(spec.aux) % 18 == 0
        table += _name_field(spec.name, strings) + struct.pack("<IhHBB", spec.value, spec.section, spec.type,
                                                               spec.storage_class, len(spec.aux) // 18)
        table += spec.aux
        count += 1 + len(spec.aux) // 18
    symbol_pointer = data_start + len(bodies)
    header = struct.pack("<HHIIIHH", machine, len(sections), 0x3E000000, symbol_pointer, count, 0, 0)
    return bytes(header + headers + bodies + table + struct.pack("<I", 4 + len(strings)) + strings)


def random_object(seed: int) -> bytes:
    """An object with several code and data sections, long names, aux records,
    static and external symbols, and every relocation type the readers meet."""
    rng = random.Random(seed)
    sections, symbols = [], []
    for k in range(rng.randint(1, 6)):
        code = rng.random() < 0.7
        size = rng.randint(8, 120)
        name = rng.choice([".text", ".text$mn", ".data", ".rdata", ".bss", ".text$yd_long_name"])
        sections.append(SectionSpec(name, rng.randbytes(size),
                                    characteristics=(0x60501020 if code else 0xC0300040),
                                    uninitialized=name == ".bss"))
        symbols.append(SymbolSpec(name, 0, k + 1, 0, 3, aux=rng.randbytes(18)))
        for j in range(rng.randint(0, 3)):
            value = 0 if j == 0 else rng.randrange(0, size - 4)
            long = rng.random() < 0.5
            symbols.append(SymbolSpec(("?Function%d@CLongClassName@@QAEXH@Z" if long else "_f%d") % (10 * k + j),
                                      value, k + 1, 0 if name == ".bss" else rng.choice([0x20, 0x20, 0x00, 0x24]),
                                      rng.choice([2, 2, 3, 6])))
    for _ in range(rng.randint(0, 4)):
        symbols.append(SymbolSpec(rng.choice(["_extern_call", "??_7CVtable@@6B@", "$LN12", "__except_list"]),
                                  0, 0, rng.choice([0x20, 0]), 2))
    indices, index = [], 0                     # table indices: auxiliary records take slots too
    for spec in symbols:
        indices.append(index)
        index += 1 + len(spec.aux) // 18
    for spec in sections:
        for _ in range(rng.randint(0, 6)):
            offset = rng.randrange(0, max(1, len(spec.data) - 4))
            spec.relocations.append((offset, rng.choice(indices), rng.choice([REL32, DIR32, DIR32NB, SECREL])))
    return write(sections, symbols)
