"""l1_bytegate.py passes the transcription ml.exe would assemble and catches a changed one."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

import l1_object
import math_harness

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def transcription(tmp_path_factory):
    work = tmp_path_factory.mktemp("l1")
    exe, code, out = math_harness.generate(ROOT, work / "math")
    obj, lst = l1_object.build(exe, out / "l1_layout.json", work)
    return exe, code, obj, lst, out / "l1_layout.json"


def gate(transcription, obj: Path, *extra: str) -> subprocess.CompletedProcess:
    exe, code, _, lst, layout = transcription
    return subprocess.run([sys.executable, "-B", str(code / "l1_bytegate.py"), str(exe), str(obj), str(lst),
                           str(layout), *extra], capture_output=True, text=True)


def test_gate_passes_the_faithful_transcription(transcription) -> None:
    run = gate(transcription, transcription[2])
    assert run.returncode == 0, run.stdout
    assert run.stdout.startswith("L1 BYTEGATE PASS: 26 instructions (62 bytes), 4 relocated fields, "
                                 "3 internal branches, 15372 data bytes; 0 failures")


def test_gate_catches_a_changed_instruction(transcription, tmp_path: Path) -> None:
    data = bytearray(transcription[2].read_bytes())
    position = data.index(bytes.fromhex("d8c1"))           # fadd st, st(1) -> fadd st, st(2)
    data[position + 1] = 0xC2
    mutant = tmp_path / "mutant.obj"
    mutant.write_bytes(data)
    run = gate(transcription, mutant)
    assert run.returncode == 1 and "byte 1 differs: d8c2 vs original d8c1" in run.stdout
    caught = gate(transcription, mutant, "--mutant")
    assert caught.returncode == 0 and "MUTANT CAUGHT" in caught.stdout


def test_gate_requires_its_four_inputs(transcription) -> None:
    exe, code, *_ = transcription
    run = subprocess.run([sys.executable, "-B", str(code / "l1_bytegate.py"), str(exe)], capture_output=True, text=True)
    assert run.returncode == 1 and "l1_bytegate.py <swkotor.exe> <obj> <lst> <l1_layout.json>" in run.stderr
