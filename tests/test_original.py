"""OriginalImage addressing on a synthetic executable."""
from __future__ import annotations

import hashlib
from pathlib import Path

import pytest

import synthetic_pe
from local_generators import original

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def image(tmp_path_factory, monkeypatch_module) -> original.OriginalImage:
    path = tmp_path_factory.mktemp("game") / "swkotor.exe"
    data = synthetic_pe.write_original(path, ROOT / "local_generators")
    monkeypatch_module.setattr(original, "EXE_SHA256", hashlib.sha256(data).hexdigest())
    return original.OriginalImage(path)


@pytest.fixture(scope="module")
def monkeypatch_module():
    with pytest.MonkeyPatch.context() as patch:
        yield patch


def test_other_executables_are_refused(tmp_path: Path) -> None:
    (tmp_path / "swkotor.exe").write_bytes(b"MZ not the game")
    with pytest.raises(ValueError, match="supported GOG release"):
        original.OriginalImage(tmp_path / "swkotor.exe")


def test_sections_follow_the_original_layout(image) -> None:
    assert [(s.name, s.va, s.raw_offset, s.raw_size) for s in image.sections[:3]] == [
        (name.decode(), va, offset, raw) for (name, va, _, raw, _), offset
        in zip(synthetic_pe.ORIGINAL_SECTIONS, (0x1000, 0x33D000, 0x38D000))]


def test_addresses_map_to_file_offsets(image) -> None:
    assert image.file_offset(0x401000) == 0x1000
    assert image.file_offset(0x73D010) == 0x33D010
    assert image.read(0x78D000, 4) == image.data[0x38D000:0x38D004]


def test_loader_zero_tail_is_refused_or_zero_filled(image) -> None:
    tail = 0x78D000 + 0x17000                       # first .data byte the file does not hold
    with pytest.raises(ValueError, match="loader-zero"):
        image.file_offset(tail)
    with pytest.raises(ValueError, match="past the raw data"):
        image.read(tail - 2, 4)
    assert image.read_loaded(tail - 2, 4) == (image.data[0x3A3FFE:0x3A4000] + b"\0\0", 0x3A3FFE, "bss-zero")
    assert image.read_loaded(tail + 8, 4) == (b"\0" * 4, None, "bss-zero")
    assert image.read_loaded(0x78D000, 4)[2] == "file"


def test_addresses_outside_every_section_are_refused(image) -> None:
    with pytest.raises(ValueError, match="outside every section"):
        image.read(0x300000, 4)


def test_cstrings_stop_at_the_terminator_or_the_limit(image) -> None:
    upgrade = synthetic_pe.UPGRADE_STRINGS
    assert len(image.cstring(upgrade)) == 11 and image.cstring(upgrade).endswith(b"\0")
    with pytest.raises(ValueError, match="No terminator"):
        image.cstring(upgrade, limit=5)


def test_write_text_uses_the_requested_line_endings(tmp_path: Path) -> None:
    original.write_text(tmp_path / "a", "x\ny\n", "\r\n")
    original.write_text(tmp_path / "b", "x\ny\n", "\n")
    assert (tmp_path / "a").read_bytes() == b"x\r\ny\r\n"
    assert (tmp_path / "b").read_bytes() == b"x\ny\n"
