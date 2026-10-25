"""Run a checkout's original-data generators on synthetic game files.

    python tests/generator_harness.py CODE_ROOT WORK_DIR [--write GOLDEN.json]

CODE_ROOT is a checkout (or an export of one revision) holding
generate_local_resources.py, local_generators/ and the rules input. The
generator files are copied to WORK_DIR with the pinned original hashes
replaced by those of the synthetic files, so the same treatment applies to
any revision. lib.exe is simulated in-process; everything else runs as in
a build. Prints {output path: sha256}, with absolute paths in the JSON
receipts replaced by placeholders and logs left out.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import runpy
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import synthetic_pe  # noqa: E402

EXE_SHA = "9c10e0450a6eeca417e036e3cde7474fed1f0a92aab018446d156944dea91435"
DLL_SHA = {"Mss32.dll": "858d6066bab4445f56c2d1713f16bfa742fb43a149c6afcb83460efe5bdec7da",
           "binkw32.dll": "2d0ae23a6175dc7b635c402a5e7e9542e923c0d1c376a8c5ef876ca0d5959d23"}
RULES = Path("src/generated/rules-input")
# Spec files that pin the hash of the original span they read.
SPAN_HASHES = {"resource-pairs-spec.json": ("original_address", "size", "span_sha256"),
               "pow-constants-spec.json": ("va", "size", "sha256")}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def make_game(generators: Path, game: Path) -> dict[str, str]:
    """Synthetic install; returns {original hash: synthetic hash}."""
    game.mkdir(parents=True)
    swaps = {EXE_SHA: sha(synthetic_pe.write_original(game / "swkotor.exe", generators))}
    swaps[DLL_SHA["Mss32.dll"]] = sha(synthetic_pe.write_dll(game / "Mss32.dll", synthetic_pe.MSS32_EXPORTS, 11))
    swaps[DLL_SHA["binkw32.dll"]] = sha(synthetic_pe.write_dll(game / "binkw32.dll", synthetic_pe.BINK_EXPORTS, 12))
    return swaps


def repin_spans(generators: Path, image: bytes) -> None:
    import pefile
    pe = pefile.PE(data=image, fast_load=True)
    for name, (address_key, size_key, hash_key) in SPAN_HASHES.items():
        path = generators / name
        raw = path.read_bytes()
        items = json.loads(raw)
        for item in items:
            address = item[address_key]
            address = int(address, 16) if isinstance(address, str) else address
            item[hash_key] = sha(pe.get_data(address - pe.OPTIONAL_HEADER.ImageBase, item[size_key]))
        indent = 2 if b"\n  " in raw else 1
        path.write_text(json.dumps(items, indent=indent) + "\n", encoding="utf-8")


def stage(code_root: Path, work: Path) -> tuple[Path, Path, dict[str, str]]:
    """Copy the generator code and inputs to work/code with the synthetic pins."""
    code = work / "code"
    shutil.copytree(code_root / "local_generators", code / "local_generators")
    shutil.copy2(code_root / "generate_local_resources.py", code)
    shutil.copytree(code_root / RULES, code / RULES)
    game = work / "game"
    swaps = make_game(code / "local_generators", game)
    for path in [code / "generate_local_resources.py", *(code / "local_generators").glob("*.py")]:
        text = path.read_bytes()
        for old, new in swaps.items():
            text = text.replace(old.encode(), new.encode())
        path.write_bytes(text)
    repin_spans(code / "local_generators", (game / "swkotor.exe").read_bytes())
    return code, game, swaps


def fake_librarian(real_run):
    def run(argv, *args, **kwargs):
        if isinstance(argv, list) and Path(str(argv[0])).name.lower() == "lib.exe":
            definition = next(a[5:] for a in argv if a.startswith("/def:"))
            library = next(a[5:] for a in argv if a.startswith("/out:"))
            Path(library).write_bytes(b"synthetic import library\n" + Path(definition).read_bytes())
            return subprocess.CompletedProcess(argv, 0, "", "")
        return real_run(argv, *args, **kwargs)
    return run


def run_generators(code_root: Path, work: Path) -> dict[str, str]:
    code, game, _ = stage(code_root.resolve(), work.resolve())
    out = work.resolve() / "out"
    librarian = work.resolve() / "msvc" / "lib.exe"
    librarian.parent.mkdir()
    librarian.write_bytes(b"")
    for name in [m for m in sys.modules if m == "local_generators" or m.startswith("local_generators.")]:
        del sys.modules[name]            # each revision imports its own copy
    argv, real_run = sys.argv, subprocess.run
    sys.argv = ["generate_local_resources.py", "--game-dir", str(game), "--out", str(out),
                "--rules-src", str(code / RULES), "--lib-exe", str(librarian)]
    subprocess.run = fake_librarian(real_run)
    try:
        runpy.run_path(str(code / "generate_local_resources.py"), run_name="__main__")
    except SystemExit as stop:
        if stop.code:
            raise
    finally:
        sys.argv, subprocess.run = argv, real_run
    return digest_tree(out, {str(game): "<GAME>", str(out): "<OUT>", str(code): "<CODE>",
                             str(librarian): "<LIB>"})


def normalize(text: str, paths: dict[str, str]) -> str:
    for path, token in sorted(paths.items(), key=lambda kv: -len(kv[0])):
        for spelling in {path, path.replace("\\", "/"), json.dumps(path)[1:-1]}:
            text = text.replace(spelling, token)
    return text


def digest_tree(out: Path, paths: dict[str, str]) -> dict[str, str]:
    result = {}
    for path in sorted(p for p in out.rglob("*") if p.is_file()):
        relative = path.relative_to(out).as_posix()
        if path.suffix == ".log":
            continue
        data = path.read_bytes()
        if path.suffix == ".json":
            document = json.loads(data)
            if relative == "generation-receipt.json":
                document.pop("generators")        # hashes of the generator code itself
                # Raw hashes of files holding absolute paths; those are digested here normalized.
                document["files"] = {k: v for k, v in document["files"].items()
                                     if not k.endswith((".log", ".json"))}
            data = normalize(json.dumps(document, indent=1, sort_keys=True), paths).encode()
        result[relative] = sha(data)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("code_root", type=Path)
    parser.add_argument("work", type=Path)
    parser.add_argument("--write", type=Path, help="save the digests as a golden file")
    args = parser.parse_args()
    digests = run_generators(args.code_root, args.work)
    text = json.dumps(digests, indent=1, sort_keys=True) + "\n"
    if args.write:
        args.write.write_text(text, encoding="utf-8", newline="\n")
    sys.stdout.write(text)
    if os.environ.get("HARNESS_KEEP") is None:
        shutil.rmtree(args.work, ignore_errors=True)


if __name__ == "__main__":
    main()
