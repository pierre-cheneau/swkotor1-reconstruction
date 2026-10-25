"""bridge_gate.py on hand-assembled functions: equivalent pairs agree, each kind of mutant is caught.

The original executable never enters the tests: each "original" and "rebuilt"
image is a synthetic PE whose .text, at 0x00401000, holds a few instructions.
"""
from __future__ import annotations

import faulthandler
import sys
from pathlib import Path

import pytest

import bridge_gate as gate
import bridge_sources
import synthetic_pe

TEXT = 0x00401000
TRIALS = 40


@pytest.fixture(autouse=True)
def quiet_emulated_faults():
    """On Windows, Unicorn turns each emulated fault into a host access violation it handles itself;
    faulthandler would print every one. Pause it for these tests only."""
    enabled = faulthandler.is_enabled()
    faulthandler.disable()
    yield
    if enabled:
        faulthandler.enable(file=sys.__stderr__)


def image(tmp_path: Path, code: bytes, name: str) -> gate.Image:
    """A synthetic executable whose .text starts with code, padded so 0x00401000..0x00402000 is mapped."""
    path = tmp_path / f"{name}.exe"
    synthetic_pe.write_code_image(path, code + bytes(0x2000 - len(code)))
    return gate.Image.load(path.read_bytes())


def check(tmp_path: Path, original: bytes, rebuilt: bytes, **target) -> gate.Report:
    fields = {"name": "f", "original": TEXT, "rebuilt": TEXT, "params": (), "popped": 0, "mask": 0xFFFFFFFF, **target}
    return gate.check(gate.Target(**fields), image(tmp_path, original, "original"),
                      image(tmp_path, rebuilt, "rebuilt"), TRIALS)


GETTER = bytes.fromhex("8b4104 8b4050 c3")                 # return this->m_p4->m_x50
GETTER_OTHER_REGISTERS = bytes.fromhex("8b5104 8b4250 c3")
GETTER_WRONG_FIELD = bytes.fromhex("8b4104 8b4054 c3")


def test_equivalent_functions_agree(tmp_path: Path) -> None:
    report = check(tmp_path, GETTER, GETTER_OTHER_REGISTERS)
    assert report.passed and report.agreed == TRIALS and report.completed > 0


def test_a_different_result_is_caught(tmp_path: Path) -> None:
    report = check(tmp_path, GETTER, GETTER_WRONG_FIELD)
    assert not report.passed and report.difference.startswith("return value differ")


SETTER = bytes.fromhex("8b442404 894108 c20400")           # this->m_8 = a0; ret 4
SETTER_OTHER_REGISTERS = bytes.fromhex("8b542404 895108 c20400")
SETTER_WITHOUT_POP = bytes.fromhex("8b442404 894108 c3")


def test_void_setters_compare_memory_and_stack(tmp_path: Path) -> None:
    assert check(tmp_path, SETTER, SETTER_OTHER_REGISTERS, params=("int",), popped=4, mask=None).passed
    report = check(tmp_path, SETTER, SETTER_WITHOUT_POP, params=("int",), popped=4, mask=None)
    assert not report.passed and "stack cleanup" in report.difference


TAIL_CALL = bytes.fromhex("8b4904 e9f8000000")             # jmp to the callee at 0x00401100 with this->m_p4
CALL = bytes.fromhex("8b4904 e8f8010000 c3")               # call the callee at 0x00401200, then return
CALL_WRONG_THIS = bytes.fromhex("8b4908 e8f8010000 c3")
FORWARD = (gate.Callee(0x00401100, 0x00401200, 0),)


def test_forwards_are_answered_alike_and_their_calls_compared(tmp_path: Path) -> None:
    assert check(tmp_path, TAIL_CALL, CALL, callees=FORWARD).passed
    report = check(tmp_path, TAIL_CALL, CALL_WRONG_THIS, callees=FORWARD)
    assert not report.passed and "calls" in report.difference


READ_GLOBAL = bytes.fromhex("a100184000 c3")                # return the global at 0x00401800
READ_REBUILT_GLOBAL = bytes.fromhex("a100194000 c3")        # ... which the rebuilt image keeps at 0x00401900
READ_WRONG_GLOBAL = bytes.fromhex("a104194000 c3")


def test_globals_are_seeded_at_both_addresses(tmp_path: Path) -> None:
    globals_ = ((0x00401800, 0x00401900),)
    assert check(tmp_path, READ_GLOBAL, READ_REBUILT_GLOBAL, globals=globals_).passed
    assert not check(tmp_path, READ_GLOBAL, READ_WRONG_GLOBAL, globals=globals_).passed


CONSTRUCT = bytes.fromhex("c70124a87400 89c8 c3")           # *this = &vftable 0074A824; return this
CONSTRUCT_REBUILT = bytes.fromhex("c701001a4000 89c8 c3")   # *this = the playable table at 0x00401A00


def test_vtable_pointers_are_compared_through_the_binding(tmp_path: Path) -> None:
    assert check(tmp_path, CONSTRUCT, CONSTRUCT_REBUILT, vtables=((0x0074A824, 0x00401A00),)).passed
    report = check(tmp_path, CONSTRUCT, CONSTRUCT_REBUILT)
    assert not report.passed and "memory" in report.difference


def test_a_function_that_never_completes_is_inconclusive(tmp_path: Path) -> None:
    always_faults = bytes.fromhex("a100000010 c3")          # reads 0x10000000, which is never mapped
    report = check(tmp_path, always_faults, always_faults)
    assert report.agreed == TRIALS and report.completed == 0 and not report.passed


def test_only_one_side_faulting_is_a_difference(tmp_path: Path) -> None:
    report = check(tmp_path, GETTER, bytes.fromhex("a100000010 c3"))
    assert not report.passed and "faulted" in report.difference


# ---------------------------------------------------------------- symbols

LINK_MAP = """\
  Address         Publics by Value              Rva+Base       Lib:Object
 0001:004ac200       @kotor_fastcall_005a4930@8 008ad200 f   1924.obj
 0001:004ac2e0       @kotor_fastcall_004b1740@12 008ad2e0 f   1926.obj
 0002:0004a888       _kotor_vtable_cswinventory_0074a824 0095d888     1534.obj
 0003:0001b124       ?manager_007a39fc@kotor_menu_window_cells@@3RAXA 009c3124     0256.obj
 0003:0001b128       ?manager_007a39fc_copy@other@@3PAXA 009c3128     0257.obj
"""


@pytest.fixture
def symbols(tmp_path: Path) -> dict[str, int]:
    (tmp_path / "link.map").write_text(LINK_MAP, encoding="utf-8")
    return gate.link_symbols(tmp_path / "link.map")


def test_entries_resolve_with_their_stack_bytes(symbols) -> None:
    assert gate.entry(symbols, 0x004B1740) == (0x008AD2E0, 4)
    assert gate.entry(symbols, 0x005A4930) == (0x008AD200, 0)
    with pytest.raises(ValueError, match=r"0 link\.map symbols"):
        gate.entry(symbols, 0x00401000)


def test_bound_storage_resolves_by_its_msvc_name(symbols, tmp_path: Path) -> None:
    (tmp_path / "cells.cpp").write_text(
        "namespace kotor_menu_window_cells { void* volatile manager_007a39fc = nullptr; }\n"
        'extern "C" const void* const kotor_vtable_cswinventory_0074a824[] = {nullptr};\n', encoding="utf-8")
    data = bridge_sources.PlayableData([tmp_path / "cells.cpp"])
    assert gate.storage_symbol(symbols, data.storage(0x007A39FC)) == 0x009C3124
    assert gate.storage_symbol(symbols, data.storage(0x0074A824)) == 0x0095D888


@pytest.mark.parametrize(("type_", "mask"), [
    ("void", None), ("unsigned char", 0xFF), ("short", 0xFFFF), ("int", 0xFFFFFFFF), ("CSWSItem*", 0xFFFFFFFF),
    ("unsigned long*", 0xFFFFFFFF),
])
def test_return_masks(type_, mask) -> None:
    assert gate.return_mask(type_) == mask


def test_float_returns_are_refused() -> None:
    with pytest.raises(ValueError, match="not compared"):
        gate.return_mask("double")
