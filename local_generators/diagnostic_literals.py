"""Original diagnostic literals; the generated output must stay private."""
from __future__ import annotations

from pathlib import Path

from .original import OriginalImage, write_text

# (symbol, address, longest the literal may be, NUL included)
LITERALS = (
    ("diagnostic_source_0074e208", 0x0074E208, 92),
    ("diagnostic_empty_0074e360", 0x0074E360, 128),
    ("diagnostic_position_0074e2a4", 0x0074E2A4, 128),
    ("diagnostic_at_pos_0074e264", 0x0074E264, 64),
)


def diagnostic_literals(image: OriginalImage, output: Path) -> None:
    lines = ["// Generated original game data: keep local and ignored.",
             "namespace base_ini_reconstruction {"]
    for symbol, address, bound in LITERALS:
        data = image.cstring(address, limit=bound)
        if len(data) < 2 or any(byte >= 128 for byte in data):
            raise ValueError("Invalid bounded original diagnostic literal")
        lines.append(f"extern const char {symbol}[] = {{{','.join(map(str, data))}}};")
    lines.append("}")
    write_text(output, "\n".join(lines) + "\n", "\r\n")
