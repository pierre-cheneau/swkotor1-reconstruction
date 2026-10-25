"""bridge_sources.py: signatures, slicing, header inlining, refusals, and the committed src/bridge/."""
from __future__ import annotations

import json
from pathlib import Path

import pytest

import bridge_sources as bridge

TYPICAL = "public: unsigned char __thiscall CResRef::GetLength(void)const "


def function(demangled: str, address: int = 0x00406040) -> bridge.Function:
    return bridge.Function.parse(address, demangled)


def test_committed_bridge_is_current() -> None:
    outputs = bridge.generate()
    assert sorted(outputs) == sorted(bridge.OUT.glob("*.cpp"))
    assert all(path.read_text(encoding="utf-8") == text for path, text in outputs.items())
    assert bridge.check_manifest(outputs) == 0


def test_signature_parts() -> None:
    f = function(TYPICAL)
    assert (f.owner, f.name, f.returns, f.params, f.const) == ("CResRef", "GetLength", "unsigned char", (), True)
    g = function("public: void __thiscall CExoArrayList<struct CSWSTagNode>::DelIndex(int)")
    assert (g.owner, g.params) == ("CExoArrayList<CSWSTagNode>", ("int",))


@pytest.mark.parametrize(("demangled", "prototype", "call"), [
    (TYPICAL, "unsigned char __fastcall kotor_fastcall_00406040(void* self, void*)",
     "return static_cast<CResRef*>(self)->GetLength();"),
    ("public: class CSWGuiPanel * __thiscall CClientExoApp::GetPlayModulePanel(void)",
     "void* __fastcall kotor_fastcall_00406040(void* self, void*)", "return static_cast<CClientExoApp*>(self)"),
    ("public: unsigned long * __thiscall CSWInventory::GetItemSlotPtr(unsigned long)",
     "unsigned long* __fastcall kotor_fastcall_00406040(void* self, void*, unsigned long a0)", "GetItemSlotPtr(a0)"),
    ("public: int __thiscall CExoKeyTable::GetTableIndex(unsigned long *,class CResRef const &,unsigned short)",
     "int __fastcall kotor_fastcall_00406040(void* self, void*, unsigned long* a0, void* a1, unsigned short a2)",
     "GetTableIndex(a0, *static_cast<CResRef const*>(a1), a2)"),
    ("public: void __thiscall CSWSObject::SetUpdateFlag(int &)",
     "void __fastcall kotor_fastcall_00406040(void* self, void*, int* a0)",
     "    static_cast<CSWSObject*>(self)->SetUpdateFlag(*a0);"),
])
def test_entries_carry_classes_as_void_pointers(demangled: str, prototype: str, call: str) -> None:
    f = function(demangled)
    assert f.prototype() == prototype
    assert call in f.entry_definition()


def test_forwards_define_the_member_through_the_playable_entry() -> None:
    f = function("public: int __thiscall CServerExoAppInternal::ExecuteLoadModule(class CExoString &)", 0x004BA820)
    assert f.forward_definition() == (
        "int CServerExoAppInternal::ExecuteLoadModule(CExoString& a0)\n{\n"
        "    return kotor_fastcall_004ba820(const_cast<void*>(static_cast<const void*>(this)), nullptr, "
        "const_cast<void*>(static_cast<const void*>(&a0)));\n}\n")
    const = function("public: class CGuiPanel * __thiscall CApp::Panel(void)const ", 0x00401000)
    assert "CGuiPanel* CApp::Panel() const" in const.forward_definition()
    assert "static_cast<CGuiPanel*>(kotor_fastcall_00401000(" in const.forward_definition()


@pytest.mark.parametrize("demangled", [
    "private: int __thiscall CApp::Hidden(void)",
    "public: static int __cdecl CApp::Static(void)",
    "int __cdecl FreeFunction(int)",
])
def test_only_public_thiscall_members_are_bridged(demangled: str) -> None:
    with pytest.raises(ValueError, match="only public __thiscall members"):
        function(demangled)


MATCHED = """\
// a preamble the slice keeps
struct A { int x; };
inline int helper() { return 1; }

// FUNCTION: SWKOTOR 0x00401000
int A_first(A* a)
{
    // a comment with } and { braces
    const char* text = "a string with } and \\" quotes {";
    char close = '}';
    /* block } comment { */
    return a->x;
}

// FUNCTION: SWKOTOR 0x00401100
template void Unused<int>(int);

int between() { return 2; }

// FUNCTION: SWKOTOR 0x00401200 SYMBOL: ?Last@@YAHXZ
int Last() { return 3; } // trailing note
"""


def test_select_keeps_the_preamble_and_only_the_chosen_definitions() -> None:
    text, kept = bridge.select(MATCHED, {0x00401000, 0x00401200})
    assert kept == {0x00401000, 0x00401200}
    assert "inline int helper()" in text and "int between()" in text
    assert "return a->x;" in text and "int Last() { return 3; } // trailing note" in text
    assert "Unused" not in text


def test_select_drops_a_body_less_definition_at_its_semicolon() -> None:
    text, kept = bridge.select(MATCHED, set())
    assert kept == set()
    assert "A_first" not in text and "Unused" not in text and "Last" not in text
    assert text.count("int between()") == 1


def test_unterminated_definitions_are_refused() -> None:
    with pytest.raises(ValueError, match="Unterminated"):
        bridge.select("// FUNCTION: SWKOTOR 0x00401000\nint f() {\n", {0x00401000})


@pytest.fixture
def headers(tmp_path: Path, monkeypatch) -> Path:
    monkeypatch.setattr(bridge, "ROOT", tmp_path)
    monkeypatch.setattr(bridge, "INCLUDE", tmp_path / "include")
    (tmp_path / "include" / "a").mkdir(parents=True)
    (tmp_path / "include" / "a" / "base.h").write_text(
        "#ifndef A_BASE_H\n#define A_BASE_H\n#include <stddef.h>\nstruct Base {};\n#endif\n", encoding="utf-8")
    (tmp_path / "include" / "a" / "more.h").write_text(
        '#pragma once\n#include "a/base.h"\n#include <string.h>\nstruct More : Base {};\n', encoding="utf-8")
    return tmp_path


def test_headers_are_inlined_once_and_system_includes_hoisted(headers: Path) -> None:
    system: list[str] = []
    text = bridge.flatten('#include "a/more.h"\n#include "a/base.h"\nint use;\n', headers / "x.cpp", set(), system)
    assert text.count("struct Base {};") == 1 and "struct More : Base {};" in text
    assert "#include" not in text and "#pragma once" not in text
    assert system == ["<stddef.h>", "<string.h>"]


def test_conditional_includes_are_refused(headers: Path) -> None:
    with pytest.raises(ValueError, match="conditional block"):
        bridge.flatten("#ifdef DEBUG\n#include <stdio.h>\n#endif\n", headers / "x.cpp", set(), [])
    with pytest.raises(ValueError, match="not found"):
        bridge.flatten('#include "a/missing.h"\n', headers / "x.cpp", set(), [])


def test_only_matched_functions_are_bridged(tmp_path: Path, monkeypatch) -> None:
    spec = tmp_path / "bridge.json"
    spec.write_text(json.dumps({"functions": [{"address": "00400000"}]}), encoding="utf-8")   # no status row
    monkeypatch.setattr(bridge, "SPEC", spec)
    with pytest.raises(ValueError, match="not a byte-identical"):
        bridge.generate()


def test_forwards_must_match_the_playable_signature(monkeypatch) -> None:
    signatures = {0x004BA820: {"public: int __thiscall CServerExoAppInternal::ExecuteLoadModule(class CExoString &)"}}
    monkeypatch.setattr(bridge, "playable_entry", lambda name: (3, False))       # src/ declares it void
    with pytest.raises(ValueError, match="does not have the original's signature"):
        bridge.forward(0x004BA820, signatures)
    monkeypatch.setattr(bridge, "playable_entry", lambda name: (3, True))
    assert bridge.forward(0x004BA820, signatures).entry == "kotor_fastcall_004ba820"


def test_an_address_needs_exactly_one_signature() -> None:
    with pytest.raises(ValueError, match="2 signatures"):
        bridge.signature(0x00401000, {0x00401000: {"a", "b"}})
