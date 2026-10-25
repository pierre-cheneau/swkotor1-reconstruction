"""Power-function constants from the hash-pinned original; output stays local and ignored."""
from __future__ import annotations

from pathlib import Path

from .original import OriginalImage, load_spec, sha256, write_text

SPEC = 'pow-constants-spec.json'


def pow_constants(image: OriginalImage, output: Path) -> None:
    records = load_spec(SPEC)
    if len(records) != 12:
        raise ValueError('Expected 12 constant objects')
    lines = ['// Generated original game data. Keep local and ignored; never publish.']
    for record in records:
        symbol, address, size = record['symbol'], record['va'], record['size']
        if symbol != f'task8_pow_const_{address:08x}' or not 0 < size <= 16000:
            raise ValueError('Invalid constant specification')
        payload = image.read(address, size)
        if sha256(payload) != record['sha256']:
            raise ValueError('Original constant bytes do not match specification')
        qualifier = 'const ' if record['const'] else ''
        lines.append(f'extern "C" __declspec(align(16)) {qualifier}unsigned char {symbol}[{size}]={{')
        lines.append(','.join(f'0x{byte:02x}' for byte in payload))
        lines.append('};\n')
    write_text(output, '\n'.join(lines), '\n')
