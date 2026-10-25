"""Run a revision's local_math/gen_l1_asm.py on the synthetic executable.

    python tests/math_harness.py CODE_ROOT WORK_DIR [--keep]

Prints {output file: sha256}. The generator is copied with the pinned hash
replaced by the synthetic image's, as generator_harness does.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import l1_fixture
import synthetic_pe

CHECKOUT = Path(__file__).resolve().parents[1]
EXE_SHA = "9c10e0450a6eeca417e036e3cde7474fed1f0a92aab018446d156944dea91435"


def generate(code_root: Path, work: Path) -> tuple[Path, Path, Path]:
    """(synthetic swkotor.exe, pinned copy of local_math/, generator output)."""
    work = work.resolve()
    game = work / "game"
    game.mkdir(parents=True)
    image = synthetic_pe.write_original(game / "swkotor.exe", CHECKOUT / "local_generators",
                                        patches=l1_fixture.patches())
    code = work / "code" / "local_math"
    code.mkdir(parents=True)
    for path in (code_root / "local_math").glob("*.py"):
        (code / path.name).write_bytes(path.read_bytes().replace(EXE_SHA.encode(),
                                                                 hashlib.sha256(image).hexdigest().encode()))
    out = work / "out"
    subprocess.run([sys.executable, "-B", str(code / "gen_l1_asm.py"), str(game / "swkotor.exe"), str(out)],
                   check=True, capture_output=True, text=True)
    return game / "swkotor.exe", code, out


def run_math(code_root: Path, work: Path) -> dict[str, str]:
    _, _, out = generate(code_root, work)
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.iterdir())}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("code_root", type=Path)
    parser.add_argument("work", type=Path)
    parser.add_argument("--keep", action="store_true", help="keep WORK_DIR for inspection")
    args = parser.parse_args()
    print(json.dumps(run_math(args.code_root, args.work), indent=1))
    if not args.keep:
        shutil.rmtree(args.work, ignore_errors=True)


if __name__ == "__main__":
    main()
