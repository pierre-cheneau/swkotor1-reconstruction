"""build.py's refusals: everything it must reject before running a tool."""
from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

import build

UNIT = "src/units/0000/bc50cf2708-bootstrap_scopes.cpp"


def make_build(tmp_path: Path, **target) -> build.Build:
    tools = tmp_path / "msvc"
    (tools / "bin/Hostx64/x86").mkdir(parents=True)
    for name in ("cl.exe", "link.exe"):
        (tools / "bin/Hostx64/x86" / name).write_bytes(b"tool")
    out = tmp_path / "out"
    out.mkdir()
    return build.Build({"files": [], "units": [], **target}, tmp_path / "game", out,
                       {"VCToolsInstallDir": str(tools)}, None, {UNIT}, {"commands": []})


@pytest.mark.parametrize("path", ["C:/Windows/x.h", "/etc/passwd", "src/../../x.h", "..\\x.h"])
def test_inside_rejects_paths_outside_the_root(path: str) -> None:
    with pytest.raises(ValueError):
        build.inside(build.ROOT, path)


def test_inside_resolves_relative_paths() -> None:
    assert build.inside(build.ROOT, UNIT) == (build.ROOT / UNIT).resolve()


@pytest.mark.parametrize("flag", ["/Ox", "/DBAD=a b", "/FIinjected.h", "/clang:-Xclang", "/D1X"])
def test_unreviewed_compiler_flags_are_refused(tmp_path: Path, flag: str) -> None:
    with pytest.raises(ValueError, match="Unreviewed compiler flag"):
        build.compile_unit(make_build(tmp_path), 0, {"source": UNIT, "flags": ["/nologo", flag]})


@pytest.mark.parametrize("flag", ["/DNDEBUG", "/D_CRT_SECURE_NO_WARNINGS", "/DLEVEL=2", "/clang:-Wno-invalid-offsetof"])
def test_reviewed_compiler_flags_reach_the_compiler(tmp_path: Path, monkeypatch, flag: str) -> None:
    seen = []
    monkeypatch.setattr(build, "run", lambda argv, env, log: seen.append(argv))
    with pytest.raises(RuntimeError, match="did not produce"):
        build.compile_unit(make_build(tmp_path), 0, {"source": UNIT, "flags": [flag]})
    assert flag in seen[0]


def test_unpinned_and_unknown_sources_are_refused(tmp_path: Path) -> None:
    b = make_build(tmp_path)
    with pytest.raises(ValueError, match="hash-pinned"):
        build.compile_unit(b, 0, {"source": "src/units/0000/unpinned.cpp", "flags": []})
    with pytest.raises(ValueError, match="Unknown source kind"):
        build.compile_unit(b, 0, {"source": UNIT, "source_kind": "url", "flags": []})
    with pytest.raises(ValueError, match="not produced"):
        build.compile_unit(b, 0, {"source": "rules/x.cpp", "source_kind": "generated", "flags": []})
    with pytest.raises(ValueError, match="compiler selector"):
        build.compile_unit(b, 0, {"source": UNIT, "compiler": "gcc", "flags": []})


@pytest.mark.parametrize("name", ["../escape.h", "a/b.h", "x.cpp"])
def test_alias_headers_must_be_plain_names_of_pinned_files(tmp_path: Path, name: str) -> None:
    with pytest.raises(ValueError, match="alias header"):
        build.alias_includes(make_build(tmp_path), 0, {name: UNIT})
    with pytest.raises(ValueError, match="alias header"):
        build.alias_includes(make_build(tmp_path / "second"), 1, {"ok.h": "src/units/0000/unpinned.h"})


@pytest.mark.parametrize("target, message", [
    ({"executable": "a.exe", "libraries": ["evil.lib"]}, "Library names"),
    ({"executable": "a.exe", "link_flags": ["/FORCE"]}, "Unreviewed linker flag"),
    ({"executable": "a.exe", "link_aliases": {"_a": "_b"}}, "Unreviewed linker alias"),
    ({"executable": "a.exe", "generated_link_inputs": ["imports/other.lib"]}, "generated link input"),
    ({"executable": "../a.exe"}, "relative"),
])
def test_link_refuses_unreviewed_inputs(tmp_path: Path, target: dict, message: str) -> None:
    with pytest.raises(ValueError, match=message):
        build.link(make_build(tmp_path, **target), [])


def test_clang_must_be_the_tested_release(tmp_path: Path, monkeypatch) -> None:
    clang = tmp_path / "clang-cl.exe"
    clang.write_bytes(b"")
    target = {"units": [{"compiler": "clang-cl"}]}
    monkeypatch.setattr(subprocess, "run",
                        lambda *a, **k: subprocess.CompletedProcess(a, 0, "clang version 18.1.0\n", ""))
    with pytest.raises(ValueError, match=r"LLVM 23\.1\.2"):
        build.verify_clang(target, clang, {})
    with pytest.raises(ValueError, match="--clang-cl"):
        build.verify_clang(target, None, {})
    build.verify_clang({"units": [{"compiler": "msvc"}]}, None, {})     # not needed: no error


def test_overrides_in_the_environment_are_refused() -> None:
    with pytest.raises(ValueError, match="overrides"):
        build.reject_overrides({"_cl_": "/FIevil.h"})
    build.reject_overrides({"CL": ""})


def test_runtime_preparation_stays_out_of_the_checkout(tmp_path: Path) -> None:
    game = tmp_path / "game"
    game.mkdir()
    with pytest.raises(ValueError, match="outside and separate"):
        build.prepare_runtime(build.ROOT / "src", game)
    runtime = tmp_path / "runtime"
    runtime.mkdir()
    (runtime / "saves").write_bytes(b"")
    with pytest.raises(ValueError, match="occupied by a file"):
        build.prepare_runtime(runtime, game)
    (runtime / "saves").unlink()
    build.prepare_runtime(runtime, game)
    assert all((runtime / name).is_dir() for name in build.RUNTIME_DIRECTORIES)
