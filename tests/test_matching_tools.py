"""The matching tools end to end, with MSVC 7.0 replaced by a fake.

The fake "compiles" a source into a COFF object holding one function with a
call relocation; the synthetic exe holds the same function at 0x00401000
with a real call displacement. Everything else is the tools' own code.
"""
from __future__ import annotations

import subprocess
from pathlib import Path

import pefile
import pytest

import coff
import libmatch
import matchcmp
import symbolmap
import synthetic_coff
import synthetic_pe

#            push ebp; mov ebp,esp; call _callee  ; pop ebp; ret ; padding
COMPILED = bytes.fromhex("55 8bec e8 00000000 5d c3 cccccccccccc")
CALLEE = 0x401100
ORIGINAL = COMPILED[:4] + (CALLEE - 0x401008).to_bytes(4, "little") + COMPILED[8:]
SOURCE = "// FUNCTION: SWKOTOR 0x00401000\nvoid __cdecl Foo(void)\n{\n    callee();\n}\n"
MSVC = Path("C:/msvc7")


def compiled_object() -> bytes:
    return synthetic_coff.write(
        [synthetic_coff.SectionSpec(".text$mn", COMPILED, [(4, 3, synthetic_coff.REL32)])],    # symbol 3: _callee
        [synthetic_coff.SymbolSpec(".text$mn", 0, 1, 0, 3, aux=bytes(18)),     # section symbol + aux
         synthetic_coff.SymbolSpec("_Foo", 0, 1),
         synthetic_coff.SymbolSpec("_callee", 0, 0)])


def fake_tool(cmd, env=None):
    tool = Path(cmd[0]).name
    if tool == "CL.EXE":
        Path(next(a[3:] for a in cmd if a.startswith("/Fo"))).write_bytes(compiled_object())
        return subprocess.CompletedProcess(cmd, 0, "", "")
    assert tool == "UNDNAME.EXE"
    names = {"_Foo": "void __cdecl Foo(void)", "_callee": "_callee"}
    return subprocess.CompletedProcess(cmd, 0, "".join(
        f'Undecoration of :- "{n}"\nis :- "{names.get(n, n)}"\n\n' for n in cmd[1:]), "")


@pytest.fixture
def setup(tmp_path: Path, monkeypatch):
    monkeypatch.setattr(matchcmp, "run_tool", fake_tool)
    source = tmp_path / "foo.cpp"
    source.write_text(SOURCE)

    def image(code: bytes) -> pefile.PE:
        path = tmp_path / f"swkotor-{len(list(tmp_path.glob('*.exe')))}.exe"
        synthetic_pe.write_code_image(path, code + bytes.fromhex("90" * 16))
        return pefile.PE(str(path), fast_load=True)
    return source, image


def test_annotations_name_the_function_below(tmp_path: Path) -> None:
    lines = ["// FUNCTION: SWKOTOR 0x00401000", "// a note", "int __thiscall",
             "CExoResMan::Leaf9460(unsigned long a)", "{", "// FUNCTION: SWKOTOR 0x00402000 SYMBOL: ??_GX@@UAEPAXI@Z"]
    path = tmp_path / "annotated.cpp"
    path.write_text("\n".join(lines))
    assert matchcmp.annotated_functions(path) == [(0x401000, "CExoResMan::Leaf9460"), (0x402000, "??_GX@@UAEPAXI@Z")]


def test_identical_function_matches(setup) -> None:
    source, image = setup
    [(address, name, ok, bad, line)] = matchcmp.check_source(
        source, matchcmp.DEFAULT_FLAGS, image(ORIGINAL), {0x401000: 16}, False, MSVC)
    assert (address, name, ok, bad) == (0x401000, "Foo", True, 0)
    assert line.startswith("OK 0x00401000 Foo")


def test_different_byte_is_reported_with_a_side_by_side(setup) -> None:
    source, image = setup
    changed = ORIGINAL[:9] + b"\xc2" + ORIGINAL[10:]          # ret -> ret imm16
    [(_, _, ok, bad, line)] = matchcmp.check_source(
        source, matchcmp.DEFAULT_FLAGS, image(changed), {0x401000: 16}, True, MSVC)
    assert not ok and bad > 0
    assert "first diff @+0x9" in line and "\n   !! " in line


def test_masked_relocations_and_trailing_padding(setup) -> None:
    original, compiled = bytes(range(16)), bytes(range(4)) + bytes(4) + bytes(range(8, 16))
    assert matchcmp.compare(original, compiled, set(range(4, 8)), 16) == ([], True)
    assert matchcmp.compare(original, compiled, set(), 16)[0] == [4, 5, 6, 7]
    assert matchcmp.compare(original + b"\xcc" * 16, compiled[:12], set(range(4, 8)), 32)[1] is False


def test_symbol_lookup_by_name_parameters_or_decoration() -> None:
    readable = {"?A@@YAXH@Z": "void __cdecl A(int)", "?AB@@YAXXZ": "void __cdecl AB(void)", "_C": "_C"}
    assert matchcmp.symbols_named("A", readable) == ["?A@@YAXH@Z"]
    assert matchcmp.symbols_named("_C", readable) == ["_C"]
    assert matchcmp.symbols_named("?AB@@YAXXZ", readable) == ["?AB@@YAXXZ"]


def test_symbolmap_recovers_definitions_and_callees(setup) -> None:
    source, image = setup
    found = symbolmap.scan(source, matchcmp.DEFAULT_FLAGS, {0x401000}, image(ORIGINAL), MSVC)
    assert found == [("_Foo", 0x401000, "defines"), ("_callee", CALLEE, "calls")]


def test_coff_reader_resolves_long_names_aux_records_and_bss() -> None:
    data = synthetic_coff.write(
        [synthetic_coff.SectionSpec(".text$yd_long_section", b"\xc3" * 8, [(1, 3, synthetic_coff.DIR32)]),
         synthetic_coff.SectionSpec(".bss", bytes(12), characteristics=0xC0300080, uninitialized=True)],
        [synthetic_coff.SymbolSpec(".text$yd", 0, 1, 0, 3, aux=bytes(36)),
         synthetic_coff.SymbolSpec("?LongFunctionName@CClass@@QAEXXZ", 2, 1),
         synthetic_coff.SymbolSpec("_data", 0, 2, 0, 2)])
    obj = coff.CoffObject(data)
    assert [s.name for s in obj.sections] == [".text$yd_long_section", ".bss"]
    assert not obj.sections[1].initialized and obj.sections[1].data == bytes(12)
    assert sorted(obj.symbols) == [0, 3, 4]                     # aux records take indices 1 and 2
    assert [s.name for s in obj.defined_functions()] == ["?LongFunctionName@CClass@@QAEXXZ"]
    assert obj.sections[0].relocations == (coff.Relocation(1, 3, coff.DIR32),)
    # From the symbol's value to the section end, the DIR32 at offset 1 masked relative to it.
    assert libmatch.coff_functions(data) == [("?LongFunctionName@CClass@@QAEXXZ", b"\xc3" * 6, {0, 1, 2})]
