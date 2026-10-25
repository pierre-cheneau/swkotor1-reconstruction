"""Behaviour locks for the tooling, recorded from upstream main (226424de).

Each test reruns a tool and compares with what that revision produced, so a
refactor that changes any output, command line or report fails here. To
accept a deliberate change, regenerate the golden file and say why in the
change description.
"""
from __future__ import annotations

import json
import shlex
import subprocess
import sys
from pathlib import Path

import build_harness
import generator_harness
import math_harness

ROOT = Path(__file__).resolve().parents[1]
GOLDEN = ROOT / "tests" / "golden"


def test_generators_match_golden(tmp_path: Path) -> None:
    expected = json.loads((GOLDEN / "generators.json").read_text(encoding="utf-8"))
    assert generator_harness.run_generators(ROOT, tmp_path) == expected


def test_build_commands_match_golden(tmp_path: Path) -> None:
    expected = json.loads((GOLDEN / "build.json").read_text(encoding="utf-8"))
    assert build_harness.summary(build_harness.run_build(ROOT, tmp_path)) == expected


def test_progress_tracker_matches_committed(tmp_path: Path) -> None:
    out = tmp_path / "progress.svg"
    subprocess.run([sys.executable, str(ROOT / "matching/tools/render_banner.py"), "--out", str(out)],
                   check=True, capture_output=True)
    assert out.read_bytes() == (ROOT / "matching/assets/progress.svg").read_bytes()


def test_todo_matches_golden() -> None:
    expected = (GOLDEN / "todo.txt").read_text(encoding="utf-8")
    sections = [block.split("\n", 1) for block in expected.split("### todo.py ")[1:]]
    for arguments, output in sections:
        run = subprocess.run([sys.executable, str(ROOT / "matching/tools/todo.py"), *shlex.split(arguments)],
                             cwd=ROOT, check=True, capture_output=True, text=True)
        assert run.stdout == output, arguments


def test_l1_math_transcription_matches_golden(tmp_path: Path) -> None:
    expected = json.loads((GOLDEN / "l1_math.json").read_text(encoding="utf-8"))
    assert math_harness.run_math(ROOT, tmp_path) == expected
