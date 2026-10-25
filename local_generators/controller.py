"""D13: the original .rdata tables the controller bodies use, as a local header.

The header is never committed or published; controller_rdata.json beside it
records each table's offset, length and sha256.
"""
from __future__ import annotations

import json
from pathlib import Path

from .original import EXE_SHA256, OriginalImage, sha256, write_text

WORD_TABLES = (("cosine_lut_00743fe8", 0x743FE8, 32), ("sine_lut_00744070", 0x744070, 32),
               ("lowpass_kernel_0078e450", 0x78E450, 9))
BYTE_TABLES = (("lowpass_shift_0078e474_bytes", 0x78E474, 9),)


def controller(image: OriginalImage, out: Path) -> None:
    lines = ["#pragma once",
             f"// GENERATED LOCALLY (D13) from the user's swkotor.exe sha256 {EXE_SHA256}. Do not commit or publish.",
             "#include <cstdint>"]
    record = {"exe": str(image.path), "exe_sha256": EXE_SHA256, "tables": []}
    for name, va, count, width in [(*t, 4) for t in WORD_TABLES] + [(*t, 1) for t in BYTE_TABLES]:
        raw = image.read(va, width * count)
        if width == 4:
            words = [int.from_bytes(raw[i:i + 4], "little") for i in range(0, len(raw), 4)]
            lines.append(f"constexpr std::uint32_t {name}_bits[{count}]={{{','.join(f'0x{w:08X}u' for w in words)}}};")
        else:
            lines.append(f"constexpr std::uint8_t {name}[{count}]={{{','.join(f'0x{b:02X}u' for b in raw)}}};")
        record["tables"].append({"name": name, "va": f"{va:08X}", "file_offset": image.file_offset(va),
                                 "length": len(raw), "sha256": sha256(raw)})
    out.parent.mkdir(parents=True, exist_ok=True)
    write_text(out, "\n".join(lines) + "\n", "\r\n")
    write_text(out.parent / "controller_rdata.json", json.dumps(record, indent=1), "\r\n")
