"""pin_sources.py on a scratch checkout."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

import pytest

import pin_sources


@pytest.fixture
def checkout(tmp_path: Path, monkeypatch) -> Path:
    for name, text in {"src/a/one.cpp": "1", "src/a/two.h": "2", "src/b/three.cpp": "3"}.items():
        (tmp_path / name).parent.mkdir(parents=True, exist_ok=True)
        (tmp_path / name).write_text(text)
    files = [{"path": p, "sha256": hashlib.sha256((tmp_path / p).read_bytes()).hexdigest()}
             for p in ("src/a/one.cpp", "src/b/three.cpp")]
    manifest = {"game_complete": True, "targets": {"game": {"units": [], "files": files}}}
    monkeypatch.setattr(pin_sources, "ROOT", tmp_path)
    monkeypatch.setattr(pin_sources, "MANIFEST", tmp_path / "build-manifest.json")
    pin_sources.write_manifest(manifest)
    monkeypatch.chdir(tmp_path)
    return tmp_path


def files(checkout: Path) -> list[dict]:
    return json.loads((checkout / "build-manifest.json").read_bytes())["targets"]["game"]["files"]


def run(monkeypatch, *argv: str) -> int:
    monkeypatch.setattr("sys.argv", ["pin_sources.py", *argv])
    return pin_sources.main()


def test_manifest_keeps_its_committed_format(checkout: Path) -> None:
    raw = (checkout / "build-manifest.json").read_bytes()
    assert raw.endswith(b"}\r\n") and b"\r\n  \"targets\"" in raw and b"\n\n" not in raw


def test_check_reports_and_changes_nothing(checkout: Path, monkeypatch) -> None:
    before = (checkout / "build-manifest.json").read_bytes()
    assert run(monkeypatch, "--check") == 0
    (checkout / "src/a/one.cpp").write_text("changed")
    assert run(monkeypatch, "--check") == 1
    assert (checkout / "build-manifest.json").read_bytes() == before


def test_changed_files_are_repinned_and_new_ones_join_their_directory(checkout: Path, monkeypatch) -> None:
    (checkout / "src/a/one.cpp").write_text("changed")
    assert run(monkeypatch, "src/a/two.h") == 0
    assert [f["path"] for f in files(checkout)] == ["src/a/one.cpp", "src/a/two.h", "src/b/three.cpp"]
    assert files(checkout)[0]["sha256"] == hashlib.sha256(b"changed").hexdigest()
    assert run(monkeypatch, "--check") == 0


def test_missing_and_outside_files_are_errors(checkout: Path, monkeypatch, tmp_path_factory) -> None:
    outside = tmp_path_factory.mktemp("elsewhere") / "x.cpp"
    outside.write_text("x")
    with pytest.raises(SystemExit, match="inside the checkout"):
        run(monkeypatch, str(outside))
    (checkout / "src/b/three.cpp").unlink()
    with pytest.raises(SystemExit, match="missing"):
        run(monkeypatch)
