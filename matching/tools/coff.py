"""Read i386 COFF objects as MSVC writes them: sections, relocations, symbols."""
from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

I386 = 0x14C
FUNCTION = 0x20                 # symbol type: function returning nothing special
EXTERNAL, STATIC = 2, 3         # storage classes
CODE = 0x20                     # section characteristic IMAGE_SCN_CNT_CODE
REL32, DIR32, DIR32NB = 0x14, 0x06, 0x07


@dataclass(frozen=True)
class Relocation:
    offset: int                 # within the section
    symbol: int                 # symbol table index
    type: int


@dataclass(frozen=True)
class Section:
    name: str
    data: bytes                 # zeros for uninitialized data
    relocations: tuple[Relocation, ...]
    characteristics: int
    initialized: bool           # data present in the file


@dataclass(frozen=True)
class Symbol:
    name: str
    value: int
    section: int                # 1-based section number; 0 undefined, negative special
    type: int
    storage_class: int

    @property
    def is_function(self) -> bool:
        return self.type == FUNCTION


class CoffObject:
    def __init__(self, data: bytes) -> None:
        self.machine, count, _, symbol_pointer, symbol_count, optional, _ = struct.unpack_from("<HHIIIHH", data, 0)
        strings = symbol_pointer + 18 * symbol_count

        def string(offset: int) -> str:
            start = strings + offset
            return data[start:data.index(b"\0", start)].decode("latin-1")

        def name(raw: bytes) -> str:
            if raw[:4] == b"\0\0\0\0":
                return string(struct.unpack_from("<I", raw, 4)[0])
            return raw.rstrip(b"\0").decode("latin-1")

        sections = []
        for i in range(count):
            header = 20 + optional + 40 * i
            raw_name = data[header:header + 8].rstrip(b"\0").decode("latin-1")
            size, pointer, relocation_pointer, _, relocation_count, _, characteristics = struct.unpack_from(
                "<IIIIHHI", data, header + 16)
            relocations = tuple(Relocation(*struct.unpack_from("<IIH", data, relocation_pointer + 10 * r))
                                for r in range(relocation_count))
            sections.append(Section(string(int(raw_name[1:])) if raw_name.startswith("/") else raw_name,
                                    data[pointer:pointer + size] if pointer else bytes(size),
                                    relocations, characteristics, pointer != 0))
        self.sections = sections
        self.symbols: dict[int, Symbol] = {}
        i = 0
        while i < symbol_count:
            entry = symbol_pointer + 18 * i
            value, section, kind, storage, aux = struct.unpack_from("<IhHBB", data, entry + 8)
            self.symbols[i] = Symbol(name(data[entry:entry + 8]), value, section, kind, storage)
            i += 1 + aux

    @classmethod
    def read(cls, path: Path) -> CoffObject:
        return cls(path.read_bytes())

    def section_of(self, symbol: Symbol) -> Section:
        return self.sections[symbol.section - 1]

    def defined_functions(self) -> list[Symbol]:
        """Function symbols defined in a section, external or static, in table order."""
        return [s for s in self.symbols.values()
                if s.is_function and s.section > 0 and s.storage_class in (EXTERNAL, STATIC)]
