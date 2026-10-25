"""D13 extractor: original .data/.rdata objects named by a renderer spec.

SPEC: [{"name": "arb_vertex_0078f600", "va": "0078f600", "kind": "cstring"|"bytes", "size": N?}]
  cstring: size = strlen+1 (NUL included), verified NUL inside the raw section.
  bytes:   exactly `size` bytes (must be raw-initialized, not loader-zero tail).
Writes OUT_DIR/<header> (contains original bytes; NEVER commit/publish) and
OUT_DIR/EXTRACT-MANIFEST.json (offsets, lengths, sha256 of each object; no data).
"""
from __future__ import annotations

import json
import os
import re
from pathlib import Path

from .original import OriginalImage, sha256, write_text

HEADER = "kotor_original_renderer_data.generated.h"
EXE_SIZE = 4042752


def validate(spec: list[dict]) -> None:
    """Check declarative names before they are emitted as C++ tokens."""
    for item in spec:
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item["name"]):
            raise ValueError("Invalid renderer symbol")
        if item["kind"] not in {"bytes", "cstring"} or not re.fullmatch(r"[0-9a-fA-F]{8}", item["va"]):
            raise ValueError("Invalid renderer extraction specification")
        if "size" in item and (type(item["size"]) is not int or not 0 < item["size"] <= EXE_SIZE):
            raise ValueError("Invalid extraction size")


def c_string_literal(data: bytes) -> str:
    parts = [chr(c) if 32 <= c < 127 and chr(c) not in '"\\?' else f"\\{c:03o}" for c in data]
    chunks = ['"' + "".join(parts[i:i + 256]) + '"' for i in range(0, len(parts), 256)] or ['""']
    return "\n    ".join(chunks)


def extract(image: OriginalImage, spec_path: Path, out_dir: Path, header: str = HEADER) -> None:
    spec = json.loads(spec_path.read_text(encoding="utf-8"))
    validate(spec)
    lines = ["// LOCAL GENERATED FILE (decision D13). Contains original swkotor.exe bytes.",
             "// Never commit or publish. Regenerate with tools/extract_original_data.py.",
             f"// exe sha256 {image.sha256}"]
    manifest = {"exe_path": str(image.path), "exe_sha256": image.sha256,
                "spec": os.path.abspath(spec_path), "objects": []}
    for item in spec:
        va = int(item["va"], 16)
        if item["kind"] == "cstring":
            data = image.cstring(va)
            if "size" in item and item["size"] != len(data):
                raise ValueError(f"{item['name']}: size {len(data)} != spec {item['size']}")
            lines.append(f'KOTOR_ORIGINAL_CSTRING({item["name"]}, {len(data)},\n    {c_string_literal(data[:-1])})')
        else:
            data = image.read(va, item["size"])
            lines.append(f'KOTOR_ORIGINAL_BYTES({item["name"]}, {len(data)}, {{{",".join(map(str, data))}}})')
        manifest["objects"].append({"name": item["name"], "va": f"{va:08X}", "section": image.section(va).name,
                                    "file_offset": f"0x{image.file_offset(va):X}", "length": len(data),
                                    "sha256": sha256(data), "kind": item["kind"]})
    out_dir.mkdir(parents=True, exist_ok=True)
    write_text(out_dir / header, "\n".join(lines) + "\n", "\n")
    write_text(out_dir / "EXTRACT-MANIFEST.json", json.dumps(manifest, indent=1), "\r\n")
