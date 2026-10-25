"""Run a revision's build.py against this checkout with a simulated toolchain.

    python tests/build_harness.py BUILD_ROOT WORK_DIR [--dump COMMANDS.json]

BUILD_ROOT holds the build.py and local_math/ under test (this checkout, or
an export of another revision); the sources, manifest and math inputs always
come from this checkout. cl, clang-cl, ml, link and the generators are
replaced by fakes that record their command lines and write plausible
outputs, so the whole orchestration runs: validation, generation, math,
compilation, linking and the final checks. Prints the sha256 of the
normalized command lines and response file, plus their counts.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import threading
from pathlib import Path

CHECKOUT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CHECKOUT / "tests"))
import synthetic_pe  # noqa: E402

CLANG_VERSION = ("clang version 23.1.2 (https://github.com/llvm/llvm-project "
                 "85ac560262434c9ccfc0c183ec22d4138ed647fb)\nTarget: i686-pc-windows-msvc\n")
OBJECT = b"\x4c\x01" + bytes(62)


class Toolchain:
    """Fake tools: records argv, writes the files a real run would."""

    def __init__(self, work: Path, manifest: dict) -> None:
        self.work = work
        self.tools = work / "vs" / "VC" / "Tools" / "MSVC" / "14.44.35207"
        self.bin = self.tools / "bin" / "Hostx64" / "x86"
        self.bin.mkdir(parents=True)
        for name in ("cl.exe", "link.exe", "lib.exe", "ml.exe"):
            (self.bin / name).write_bytes(name.encode())
        self.clang = work / "llvm" / "clang-cl.exe"
        self.clang.parent.mkdir()
        self.clang.write_bytes(b"clang-cl")
        target = manifest["targets"]["game"]
        self.generated = sorted({u["source"] for u in target["units"] if u.get("source_kind") == "generated"}
                                | {h for u in target["units"] for h in u.get("generated_headers", [])}
                                | set(target.get("generated_link_inputs", [])))
        self.commands: list[list[str]] = []
        self.lock = threading.Lock()

    def environment(self) -> dict[str, str]:
        return {**os.environ, "VCToolsInstallDir": str(self.tools) + "\\",
                "VCToolsVersion": "14.44.35207", "WindowsSDKVersion": "10.0.26100.0\\"}

    def run(self, argv, env, log: Path) -> None:
        argv = [str(a) for a in argv]
        with self.lock:
            self.commands.append(argv)
        tool = Path(argv[0]).name.lower()
        script = next((Path(a).name for a in argv[1:3] if a.endswith(".py")), "")
        if script == "generate_local_resources.py":
            out = Path(argv[argv.index("--out") + 1])
            for relative in self.generated:
                (out / relative).parent.mkdir(parents=True, exist_ok=True)
                (out / relative).write_text(f"// synthetic {relative}\n", encoding="ascii")
        elif script == "gen_l1_asm.py":
            local = Path(argv[-1])
            local.mkdir(parents=True)
            (local / "kotor_vc7_math.asm").write_text("; synthetic\n")
            (local / "kotor_vc7_pow_tables.inc").write_text("; synthetic\n")
            (local / "l1_layout.json").write_text(json.dumps({"instructions": [0] * 2806, "runs": [[0, 0]] * 18}))
        elif tool in {"cl.exe", "clang-cl.exe", "ml.exe"}:
            obj = next(a[3:] for a in argv if a.startswith("/Fo"))
            Path(obj).write_bytes(OBJECT)
        elif tool == "link.exe":
            arguments = [line.strip('"') for line in Path(argv[1][1:]).read_text(encoding="utf-8").splitlines()]
            exe = next(a[5:] for a in arguments if a.startswith("/OUT:"))
            link_map = next(a[5:] for a in arguments if a.startswith("/MAP:"))
            Path(exe).write_bytes(b"MZ synthetic")
            providers = json.loads((CHECKOUT / "local_math" / "providers.json").read_text())
            Path(link_map).write_text("".join(f" 0001:00000000       {symbol}   00401000 f   {owner}\n"
                                              for symbol, owner in providers.items()))
        log.write_text("ok\n", encoding="utf-8")


def load_build(build_root: Path):
    spec = importlib.util.spec_from_file_location("build_under_test", build_root / "build.py")
    module = importlib.util.module_from_spec(spec)
    sys.path.insert(0, str(build_root))
    for name in [m for m in sys.modules if m == "local_math" or m.startswith("local_math.")]:
        del sys.modules[name]
    spec.loader.exec_module(module)
    module.ROOT = CHECKOUT
    return module


def run_build(build_root: Path, work: Path, jobs: int = 4) -> dict:
    work = work.resolve()
    manifest = json.loads((CHECKOUT / "build-manifest.json").read_text(encoding="utf-8"))
    toolchain = Toolchain(work, manifest)
    game = work / "game"
    game.mkdir()
    image = synthetic_pe.write_original(game / "swkotor.exe", CHECKOUT / "local_generators")
    module = load_build(build_root.resolve())
    pin = hashlib.sha256(image).hexdigest()
    for name in dir(module):           # whatever the revision calls its pinned hash
        if getattr(module, name) == "9c10e0450a6eeca417e036e3cde7474fed1f0a92aab018446d156944dea91435":
            setattr(module, name, pin)
    real_run = subprocess.run

    def fake_subprocess(argv, *args, **kwargs):
        if Path(str(argv[0])).name == "clang-cl.exe" and "--version" in argv:
            return subprocess.CompletedProcess(argv, 0, CLANG_VERSION, "")
        return real_run(argv, *args, **kwargs)

    patches = {"setup_msvc": lambda vs_root: toolchain.environment(), "run": toolchain.run}
    saved = {name: getattr(module, name) for name in patches if hasattr(module, name)}
    for name, value in patches.items():
        setattr(module, name, value)
    out = work / "build-out"
    argv = sys.argv
    sys.argv = ["build.py", "--target", "game", "--game-dir", str(game), "--clang-cl", str(toolchain.clang),
                "--out", str(out), "--jobs", str(jobs), "--vs-root", str(work / "vs")]
    subprocess.run = fake_subprocess
    try:
        module.main()
    finally:
        sys.argv, subprocess.run = argv, real_run
        for name, value in saved.items():
            setattr(module, name, value)
    tokens = {str(out): "<OUT>", str(CHECKOUT): "<ROOT>", str(game): "<GAME>", str(work): "<WORK>",
              sys.executable: "<PYTHON>"}

    def normalize(value: str) -> str:
        for path, token in sorted(tokens.items(), key=lambda kv: -len(kv[0])):
            for spelling in {path, path.replace("\\", "/")}:
                value = value.replace(spelling, token)
        return value

    commands = sorted(json.dumps([normalize(a) for a in c]) for c in toolchain.commands)
    response = normalize((out / "link.rsp").read_text(encoding="utf-8"))
    receipt = json.loads((out / "build-receipt.json").read_text(encoding="utf-8"))
    return {"commands": commands, "link.rsp": response,
            "receipt_keys": sorted(receipt), "sources": len(receipt["sources"])}


def summary(result: dict) -> dict:
    return {"commands": len(result["commands"]),
            "commands_sha256": hashlib.sha256("\n".join(result["commands"]).encode()).hexdigest(),
            "link_rsp_sha256": hashlib.sha256(result["link.rsp"].encode()).hexdigest(),
            "receipt_keys": result["receipt_keys"], "sources": result["sources"]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("build_root", type=Path)
    parser.add_argument("work", type=Path)
    parser.add_argument("--dump", type=Path, help="write the full normalized commands for diffing")
    args = parser.parse_args()
    result = run_build(args.build_root, args.work)
    if args.dump:
        args.dump.write_text(json.dumps(result, indent=1) + "\n", encoding="utf-8")
    print(json.dumps(summary(result), indent=1))
    shutil.rmtree(args.work, ignore_errors=True)


if __name__ == "__main__":
    main()
