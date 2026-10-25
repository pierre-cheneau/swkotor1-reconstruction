"""Import libraries for the original middleware DLLs, built with the user's lib.exe."""
from __future__ import annotations

import os
import re
import subprocess
from pathlib import Path

import pefile

from .original import sha256, write_text

DLLS = {
    "Mss32.dll": "858d6066bab4445f56c2d1713f16bfa742fb43a149c6afcb83460efe5bdec7da",
    "binkw32.dll": "2d0ae23a6175dc7b635c402a5e7e9542e923c0d1c376a8c5ef876ca0d5959d23",
}
TOOL_OVERRIDES = {"CL", "_CL_", "LINK", "_LINK_", "LIB_FLAGS"}


def definition(filename: str, dll: Path) -> str:
    """The .def text naming every export; stdcall names keep their decoration."""
    with pefile.PE(str(dll)) as pe:
        if pe.FILE_HEADER.Machine != 0x14C:
            raise ValueError("Expected x86 middleware DLL")
        names = [s.name.decode("ascii") for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name]
    if not names or len(names) != len(set(names)):
        raise ValueError("Empty or duplicate DLL exports")
    lines = [f"LIBRARY {filename.lower()}", "EXPORTS"]
    for name in names:
        if not re.fullmatch(r"[A-Za-z_?@$][A-Za-z0-9_?@$]*", name):
            raise ValueError("Unsupported DLL export name")
        stdcall = re.fullmatch(r"_([A-Za-z0-9_]+@\d+)", name)
        lines.append(f"  {stdcall[1]} EXPORTAS {name}" if stdcall else f"  {name}")
    return "\n".join(lines) + "\n"


def import_libraries(game: Path, output: Path, librarian: Path) -> list[dict[str, str]]:
    if librarian.name.lower() != "lib.exe" or not librarian.is_file():
        raise ValueError("Pass an installed MSVC x86 lib.exe via --lib-exe")
    if any(k.upper() in TOOL_OVERRIDES and v for k, v in os.environ.items()):
        raise ValueError("Inherited tool option overrides are not permitted")
    librarian_sha256 = sha256(librarian.read_bytes())
    results = []
    for filename, expected in DLLS.items():
        dll = (game / filename).resolve(strict=True)
        if not dll.is_relative_to(game) or sha256(dll.read_bytes()) != expected:
            raise ValueError(f"Unsupported or redirected original DLL: {filename}")
        stem = Path(filename).stem.lower()
        def_file, library = output / f"{stem}.def", output / f"{stem}.lib"
        write_text(def_file, definition(filename, dll), "\r\n")
        command = [str(librarian), "/nologo", f"/def:{def_file}", "/machine:x86", f"/out:{library}"]
        run = subprocess.run(command, cwd=output, capture_output=True, text=True, timeout=120, check=False)
        (output / f"{stem}.log").write_text(run.stdout + run.stderr, encoding="utf-8")
        if run.returncode or not library.is_file() or library.stat().st_size == 0:
            raise RuntimeError(f"Import library generation failed for {filename}")
        results.append({"dll": filename, "dll_sha256": expected,
                        "library_sha256": sha256(library.read_bytes()), "lib_exe_sha256": librarian_sha256})
    return results
