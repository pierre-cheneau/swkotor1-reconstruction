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


def image(tmp_path: Path, code: bytes, name: str, rdata: bytes = b"") -> gate.Image:
    """A synthetic executable whose .text starts with code, padded so 0x00401000..0x00403000 is mapped; rdata, if
    any, is its .rdata at 0x00403000."""
    path = tmp_path / f"{name}.exe"
    synthetic_pe.write_code_image(path, code + bytes(0x2000 - len(code)), rdata)
    return gate.Image.load(path.read_bytes())


def check(tmp_path: Path, original: bytes, rebuilt: bytes, rdata: tuple[bytes, bytes] = (b"", b""),
          **target) -> gate.Report:
    fields = {"name": "f", "original": TEXT, "rebuilt": TEXT, "params": (), "popped": 0, "mask": 0xFFFFFFFF, **target}
    return gate.check(gate.Target(**fields), image(tmp_path, original, "original", rdata[0]),
                      image(tmp_path, rebuilt, "rebuilt", rdata[1]), TRIALS)


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
    assert gate.entry(symbols, "kotor_fastcall_004b1740") == (0x008AD2E0, 4)
    assert gate.entry(symbols, "kotor_fastcall_005a4930") == (0x008AD200, 0)
    with pytest.raises(ValueError, match=r"0 link\.map symbols"):
        gate.entry(symbols, "kotor_fastcall_00401000")


def test_bound_storage_resolves_by_its_msvc_name(symbols, tmp_path: Path) -> None:
    (tmp_path / "cells.cpp").write_text(
        "namespace kotor_menu_window_cells { void* volatile manager_007a39fc = nullptr; }\n"
        'extern "C" const void* const kotor_vtable_cswinventory_0074a824[] = {nullptr};\n', encoding="utf-8")
    data = bridge_sources.PlayableData([tmp_path / "cells.cpp"])
    assert gate.storage_symbol(symbols, data.storage(0x007A39FC)) == 0x009C3124
    assert gate.storage_symbol(symbols, data.storage(0x0074A824)) == 0x0095D888


def member(demangled: str) -> bridge_sources.Function:
    return bridge_sources.Function.parse(0x00401000, demangled)


@pytest.mark.parametrize(("demangled", "mask"), [
    ("public: void __thiscall A::F(void)", None),
    ("public: unsigned char __thiscall A::F(void)", 0xFF),
    ("public: short __thiscall A::F(void)", 0xFFFF),
    ("public: int __thiscall A::F(void)", 0xFFFFFFFF),
    ("public: class CSWSItem * __thiscall A::F(void)", 0xFFFFFFFF),
    ("public: __thiscall A::A(void)", 0xFFFFFFFF),
    ("public: class CExoString __thiscall A::F(void)", 0xFFFFFFFF),      # the hidden pointer comes back
])
def test_return_masks(demangled: str, mask: int | None) -> None:
    assert gate.return_mask(member(demangled)) == mask


def test_float_returns_are_refused() -> None:
    with pytest.raises(ValueError, match="not compared"):
        gate.return_mask(member("public: double __thiscall A::F(void)"))


def test_stack_words_place_hidden_pointers_and_classes_by_value() -> None:
    assert gate.stack_words(member("public: class CExoString __thiscall A::F(int)"), 8) == ("void*", "int")
    mesh = member("public: int __thiscall A::F(struct Vector,struct Vector,int *)")
    assert gate.stack_words(mesh, 28) == ("word",) * 6 + ("int*",)
    assert gate.stack_words(member("public: void __thiscall A::F(double,char)"), 12) == ("double",) * 2 + ("char",)
    with pytest.raises(ValueError, match="cannot place"):
        gate.stack_words(mesh, 26)


def test_the_original_cleanup_is_read_through_thunks(tmp_path: Path) -> None:
    code = bytes.fromhex("33c0 c20800") + bytes(0x100 - 5)              # 00401000: xor eax,eax; ret 8
    code += bytes.fromhex("8b4904 e9f6feffff") + bytes(0x100 - 8)       # 00401100: thunk to 00401000
    code += bytes.fromhex("8b01 ff6008") + bytes(0x100 - 5)             # 00401200: jmp [eax+8]
    code += bytes.fromhex("c3")                                         # 00401300: ret
    original = image(tmp_path, code, "original")
    assert [gate.cleanup(original, a) for a in (0x00401000, 0x00401100, 0x00401200, 0x00401300)] == [8, 8, None, 0]


ALLOCATE = bytes.fromhex("6a18 e8f9000000 59 c3")             # operator new(0x18) at 00401100; return it
ALLOCATE_REBUILT = bytes.fromhex("31c9 6a18 e8f7010000 59 c3")   # ... at 00401200, with ECX cleared first
ALLOCATE_LESS = bytes.fromhex("6a14 e8f9000000 59 c3")
HEAP = (gate.Callee(0x00401100, 0x00401200, 0, 1),)


def test_heap_calls_compare_their_first_argument_only(tmp_path: Path) -> None:
    assert check(tmp_path, ALLOCATE, ALLOCATE_REBUILT, callees=HEAP).passed
    assert not check(tmp_path, ALLOCATE, ALLOCATE_LESS, callees=HEAP).passed


def test_a_defect_found_before_running_fails(tmp_path: Path) -> None:
    report = check(tmp_path, GETTER, GETTER, defect="forward x pops 4 bytes, the original 00401000 8")
    assert not report.passed and report.difference.startswith("forward x")


EXCEPTION_LIST = bytes.fromhex("64a100000000 c3")            # return fs:[0], the head of the SEH chain


def test_functions_with_exception_frames_run_on_a_thread_block(tmp_path: Path) -> None:
    report = check(tmp_path, EXCEPTION_LIST, EXCEPTION_LIST)
    assert report.passed and report.completed == TRIALS


SPIN = bytes.fromhex("ebfe")                                  # jmp $


def test_a_run_out_of_steps_is_not_compared(tmp_path: Path) -> None:
    report = check(tmp_path, GETTER, SPIN)
    assert (report.passed, report.difference, report.exhausted) == (False, None, TRIALS)


# Each passes the address of a local to the callee at 00401100 (which pops it), from different frame offsets.
LOCAL_AT_0 = bytes.fromhex("83ec08 8d0424 50 e8f4000000 83c408 c3")
LOCAL_AT_4 = bytes.fromhex("83ec08 8d442404 50 e8f3000000 83c408 c3")
THIS_INSTEAD = bytes.fromhex("83ec08 51 e8f6000000 83c408 c3")
CALLEE = (gate.Callee(0x00401100, 0x00401100, 4),)


def test_stack_temporaries_compare_by_slot_not_by_frame_offset(tmp_path: Path) -> None:
    assert check(tmp_path, LOCAL_AT_0, LOCAL_AT_4, callees=CALLEE).passed
    assert not check(tmp_path, LOCAL_AT_0, THIS_INSTEAD, callees=CALLEE).passed


PUSH_LITERAL_0 = bytes.fromhex("6800304000 e8f6000000 c3")    # callee(",") with "," at 00403000
PUSH_LITERAL_16 = bytes.fromhex("6810304000 e8f6000000 c3")   # ... with "," at 00403010


def test_literals_compare_by_their_text(tmp_path: Path) -> None:
    padded = b"padding padding!"
    assert check(tmp_path, PUSH_LITERAL_0, PUSH_LITERAL_16, (b",\0", padded + b",\0"), callees=CALLEE).passed
    assert not check(tmp_path, PUSH_LITERAL_0, PUSH_LITERAL_16, (b",\0", padded + b".\0"), callees=CALLEE).passed
