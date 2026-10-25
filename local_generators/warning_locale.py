"""Warning and locale tables, read only from the supported, locally supplied game."""
from __future__ import annotations

import struct
from pathlib import Path

from .original import OriginalImage, load_spec, write_text

SPEC = "warning-locale-spec.json"


def warning_locale(image: OriginalImage, output: Path) -> None:
    groups = {"warning": ["namespace base_ini_reconstruction {"], "locale": ["namespace kotor_locale {"]}
    for item in load_spec(SPEC):
        count, width = item["count"], item["width"]
        data = bytes(count * width) if item["bss"] else image.read(item["address"], count * width)
        values = struct.unpack("<" + ("H" if width == 2 else "B") * count, data)
        prefix = ("extern " if item["group"] == "warning" else "") + item["prefix"]
        groups[item["group"]].append(f"{prefix} {item['name']}[{count}] = {{{','.join(map(str, values))}}};")
    output.mkdir(parents=True, exist_ok=True)
    for group, lines in groups.items():
        extension = "cpp" if group == "warning" else "inc"
        write_text(output / f"{group}_data.{extension}", "\n".join([*lines, "}"]) + "\n", "\r\n")
