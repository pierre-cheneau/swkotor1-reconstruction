"""The supported original executable, opened read-only and never executed."""
from __future__ import annotations

import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path

EXE_SHA256 = "9c10e0450a6eeca417e036e3cde7474fed1f0a92aab018446d156944dea91435"
SPECS = Path(__file__).resolve().parent


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_spec(name: str):
    return json.loads((SPECS / name).read_text(encoding="utf-8"))


def write_text(path: Path, text: str, newline: str) -> None:
    """Write ASCII/UTF-8 text with explicit line endings, whatever the host OS."""
    path.write_bytes(text.replace("\n", newline).encode("utf-8"))


def byte_list(data: bytes, form: str = "0x{:02x}", separator: str = ", ") -> str:
    return separator.join(form.format(b) for b in data)


@dataclass(frozen=True)
class Section:
    name: str
    va: int                # absolute virtual address
    virtual_size: int
    raw_offset: int
    raw_size: int

    def contains(self, va: int) -> bool:
        return self.va <= va < self.va + max(self.virtual_size, self.raw_size)


class OriginalImage:
    """swkotor.exe bytes, verified against the pinned hash, addressed by VA."""

    def __init__(self, path: Path) -> None:
        self.path = path
        self.data = path.read_bytes()
        self.sha256 = sha256(self.data)
        if self.sha256 != EXE_SHA256:
            raise ValueError("The original executable does not match the supported GOG release")
        header = struct.unpack_from("<I", self.data, 0x3C)[0]
        count, optional = struct.unpack_from("<H", self.data, header + 6)[0], struct.unpack_from("<H", self.data, header + 20)[0]
        self.image_base = struct.unpack_from("<I", self.data, header + 52)[0]
        table = header + 24 + optional
        self.sections = []
        for i in range(count):
            entry = table + 40 * i
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", self.data, entry + 8)
            name = self.data[entry:entry + 8].rstrip(b"\0").decode("ascii")
            self.sections.append(Section(name, self.image_base + rva, virtual_size, raw_offset, raw_size))

    def verify_unchanged(self) -> None:
        if sha256(self.path.read_bytes()) != EXE_SHA256:
            raise ValueError("Original executable changed during generation")

    def section(self, va: int) -> Section:
        for section in self.sections:
            if section.contains(va):
                return section
        raise ValueError(f"{va:08X} lies outside every section")

    def file_offset(self, va: int) -> int:
        """Offset of a byte the file initializes (not a loader-zero tail)."""
        section = self.section(va)
        relative = va - section.va
        if relative >= section.raw_size:
            raise ValueError(f"{va:08X} lies in the loader-zero tail of {section.name}")
        return section.raw_offset + relative

    def read(self, va: int, size: int) -> bytes:
        """size bytes the file initializes, all in one section."""
        section = self.section(va)
        if va + size > section.va + section.raw_size:
            raise ValueError(f"{va:08X}+{size:#x} extends past the raw data of {section.name}")
        start = self.file_offset(va)
        return self.data[start:start + size]

    def read_loaded(self, va: int, size: int) -> tuple[bytes, int | None, str]:
        """size bytes as the loader maps them: (bytes, file offset or None, 'file'/'bss-zero')."""
        section = self.section(va)
        relative = va - section.va
        initialized = max(0, min(size, section.raw_size - relative))
        start = section.raw_offset + relative
        data = self.data[start:start + initialized] + bytes(size - initialized)
        return (data, start if relative < section.raw_size else None,
                "file" if initialized == size else "bss-zero")

    def cstring(self, va: int, limit: int | None = None) -> bytes:
        """The NUL-terminated string at va, NUL included, within its section's raw data."""
        section = self.section(va)
        start = self.file_offset(va)
        end = section.raw_offset + section.raw_size
        if limit is not None:
            end = min(end, start + limit)
        nul = self.data.find(b"\0", start, end)
        if nul < 0:
            raise ValueError(f"No terminator for the string at {va:08X}")
        return self.data[start:nul + 1]

    def u32(self, va: int) -> int:
        return struct.unpack("<I", self.read(va, 4))[0]
