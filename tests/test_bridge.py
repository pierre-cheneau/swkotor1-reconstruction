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


def tables(signatures=None, globals_=None, vftables=None) -> bridge.Tables:
    return bridge.Tables({}, signatures or {}, globals_ or {}, vftables or {})


def unit(tables_: bridge.Tables, data: bridge.PlayableData) -> bridge.Unit:
    return bridge.Unit(Path("x.cpp"), [], [], tables_, data)


def test_forwards_must_match_the_playable_signature(monkeypatch) -> None:
    load = "public: int __thiscall CServerExoAppInternal::ExecuteLoadModule(class CExoString &)"
    u = unit(tables({0x004BA820: {load}}), bridge.PlayableData([]))
    monkeypatch.setattr(bridge, "playable_entry", lambda name, data: (3, False))     # src/ declares it void
    with pytest.raises(ValueError, match="does not have the original's signature"):
        u.forward(0x004BA820, "")
    monkeypatch.setattr(bridge, "playable_entry", lambda name, data: (3, True))
    assert u.forward(0x004BA820, "").entry == "kotor_fastcall_004ba820"


def test_several_signatures_resolve_to_the_one_the_slice_declares() -> None:
    signed = "public: char __thiscall CGameObjectArray::GetGameObject(unsigned long,class CGameObject * *)"
    plain_ = "public: unsigned char __thiscall CGameObjectArray::GetGameObject(unsigned long,class CGameObject * *)"
    t = tables({0x004D8230: {signed, plain_}})
    body = ("class CGameObjectArray {\npublic:\n"
            "    unsigned char GetGameObject(unsigned long id, CGameObject** out);\n};")
    assert t.signature(0x004D8230, body) == plain_
    with pytest.raises(ValueError, match="2 matching signatures"):
        t.signature(0x004D8230)
    with pytest.raises(ValueError, match="0 matching signatures"):
        t.signature(0x004D8230, "class CGameObjectArray {};")


# ---------------------------------------------------------------- constructors, destructors and vtables

def test_constructors_and_destructors_are_recognized() -> None:
    ctor = function("public: __thiscall CSWInventory::CSWInventory(void)", 0x005A4930)
    dtor = function("public: virtual __thiscall CAurPart::~CAurPart(void)", 0x0043E2E0)
    member = function(TYPICAL)
    assert (ctor.kind, ctor.returns, dtor.kind, dtor.returns, member.kind) == (
        "constructor", "void*", "destructor", "void", "member")
    with pytest.raises(ValueError, match="unexpected signature shape"):
        function("public: int __thiscall CSWInventory::CSWInventory(void)")


def test_a_bound_constructor_rebinds_the_vtable_and_returns_this() -> None:
    entry = function("public: __thiscall CSWInventory::CSWInventory(void)", 0x005A4930).entry_definition(
        "::kotor_vtable_cswinventory_0074a824")
    assert entry.splitlines()[2:6] == [
        '    static_assert(std::is_polymorphic<CSWInventory>::value, "a vtable is bound");',
        "    ::new (self) CSWInventory();",
        "    *static_cast<const void**>(self) = static_cast<const void*>(::kotor_vtable_cswinventory_0074a824);"
        "  // the playable build's table",
        "    return self;"]


def test_an_unbound_destructor_insists_the_class_has_no_vtable() -> None:
    entry = function("public: __thiscall CThing::~CThing(void)", 0x00401000).entry_definition()
    assert "static_assert(!std::is_polymorphic<CThing>::value" in entry
    assert "static_cast<CThing*>(self)->CThing::~CThing();" in entry and "return" not in entry


def test_compiler_generated_markers_own_only_their_line() -> None:
    text = ("// FUNCTION: SWKOTOR 0x005A4C00 SYMBOL: ??_GCSWInventory@@UAEPAXI@Z\n\n"
            "// FUNCTION: SWKOTOR 0x005A4C20\nint GetItemInSlot() { return 1; }\n")
    assert bridge.select(text, {0x005A4C20}) == (text[text.index("\n") + 1:], {0x005A4C20})   # all but the marker
    assert "GetItemInSlot" not in bridge.select(text, set())[0]
    with pytest.raises(ValueError, match="compiler-generated"):
        bridge.select(text, {0x005A4C00})


# ---------------------------------------------------------------- the data bridge

@pytest.mark.parametrize(("text", "value"), [
    ("0", 0), ("0x00u", 0), ("6", 6), ("{0}", 0), ("-3", -3), ("010", 8), ("0x10UL", 16),
    ("1.5f", 1.5), ("0x1p+0f", 1.0), (".5", 0.5), ("nullptr", None), ("kOther", None), ("1 + 1", None),
])
def test_literal_values(text, value) -> None:
    assert bridge.literal(text) == value


@pytest.mark.parametrize(("type_", "bounds", "size"), [
    ("void* volatile", "", 4), ("std::uint32_t volatile", "", 4), ("const unsigned char", "", 1), ("float", "[3]", 12),
    ("CAppManager*", "", 4), ("unsigned", "", 4), ("double", "[2][2]", 32), ("CExoString", "", None),
    ("float", "[]", None), ("long long", "", None),
])
def test_sizes(type_, bounds, size) -> None:
    assert bridge.size_of(type_, bounds) == size


PLAYABLE = {
    "cells.cpp": "namespace kotor_menu_window_cells {\nvoid* volatile manager_007a39fc = nullptr;\n"
                 "volatile float fog_0078db24 = 1.0f;\n}\n",
    "alias.cpp": "namespace other { void*& app_007a39fc = kotor_menu_window_cells::manager_007a39fc; }\n"
                 "namespace kotor_menu_window_cells { extern void* volatile manager_007a39fc; }\n",
    "constants_a.cpp": "namespace { constexpr unsigned char k_found_0074666c = 0x00u; }\n"
                       "constexpr unsigned k_mixed_00746640 = 1;\n",
    "constants_b.cpp": "const unsigned char kOk_0074666c = 0;\nconstexpr unsigned k_mixed_00746640 = 2;\n",
    "twice_a.cpp": "unsigned word_00831fdc = 0;\n",
    "twice_b.cpp": 'extern "C" { void* recovered_00831fdc = nullptr; }\n',
    "window.cpp": "HWND volatile render_007a39d8 = nullptr;\n",
    "table.cpp": 'extern "C" const void* const kotor_vtable_cswinventory_0074a824[] = {nullptr};\n',
}


@pytest.fixture
def playable(tmp_path: Path) -> bridge.PlayableData:
    for name, text in PLAYABLE.items():
        (tmp_path / name).write_text(text, encoding="utf-8")
    return bridge.PlayableData(sorted(tmp_path.glob("*.cpp")))


def test_storage_is_the_one_external_definition(playable) -> None:
    storage = playable.storage(0x007A39FC)
    assert storage.qualified == "::kotor_menu_window_cells::manager_007a39fc" and storage.size == 4
    assert storage.declaration() == "namespace kotor_menu_window_cells { extern void* volatile manager_007a39fc; }"
    table = playable.storage(0x0074A824)
    assert table.declaration() == 'extern "C" const void* const kotor_vtable_cswinventory_0074a824[];'
    assert table.qualified == "::kotor_vtable_cswinventory_0074a824"


@pytest.mark.parametrize(("address", "reason"), [
    (0x00831FDC, "2 external playable definitions: ::recovered_00831fdc, ::word_00831fdc"),
    (0x0074666C, "0 external playable definitions (2 internal)"),
    (0x007A39D8, "unsupported playable type HWND volatile"),
    (0x00700000, "0 external playable definitions"),
])
def test_storage_refusals_say_why(playable, address, reason) -> None:
    assert playable.storage(address) == reason


def test_constants_need_every_definition_to_agree(playable) -> None:
    assert playable.constant(0x0074666C) == ("0", 2)
    assert playable.constant(0x00746640) is None          # 1 and 2 disagree
    assert playable.constant(0x007A39FC) is None          # nullptr is not a literal


MATCHED_GLOBALS = """\
class CAppManager;
extern CAppManager* g_pAppManager;                  // 0x007A39FC
extern float g_vFogColor[3];
extern const unsigned char GAME_OBJECT_VALID;
extern int g_unused;
extern HWND g_window;
int use() { return g_pAppManager != 0 && g_vFogColor[0] > 0 && GAME_OBJECT_VALID == 0 && g_window != 0; }
"""


def test_used_globals_bind_to_storage_or_agreed_constants(playable) -> None:
    t = tables(globals_={"g_pAppManager": {0x007A39FC}, "g_vFogColor": {0x0078DB24}, "GAME_OBJECT_VALID": {0x0074666C},
                         "g_unused": {0x00700000}, "g_window": {0x007A39D8, 0x007A39DC}})
    body, bound, unbound = unit(t, playable).bind_globals(MATCHED_GLOBALS)
    assert [(name, type(target).__name__) for name, target in bound] == [
        ("g_pAppManager", "Storage"), ("GAME_OBJECT_VALID", "Constant")]
    assert unbound == [
        ("g_vFogColor", "0078DB24: 12 bytes, the playable ::kotor_menu_window_cells::fog_0078db24 holds 4"),
        ("g_window", "2 original addresses in symbols.csv")]
    assert "kotor_bridge::ref<CAppManager*> g_pAppManager = reinterpret_cast<kotor_bridge::ref<CAppManager*>>" \
           "(const_cast<kotor_bridge::storage<decltype(::kotor_menu_window_cells::manager_007a39fc)>&>" \
           "(::kotor_menu_window_cells::manager_007a39fc));                  // 0x007A39FC" in body
    assert 'static_assert(sizeof(CAppManager*) <= sizeof(::kotor_menu_window_cells::manager_007a39fc)' in body
    assert "const unsigned char GAME_OBJECT_VALID = 0;" in body
    assert "extern int g_unused;" in body and "extern float g_vFogColor[3];" in body     # untouched


def test_a_class_with_several_vftables_is_refused(playable) -> None:
    ctor = function("public: __thiscall CBoth::CBoth(void)", 0x00401000)
    t = tables(vftables={"CBoth": {0x00741000, 0x00741010}})
    with pytest.raises(ValueError, match="several vftables"):
        unit(t, playable).vtable(ctor)
    with pytest.raises(ValueError, match="no playable table"):
        unit(tables(vftables={"CBoth": {0x00741000}}), playable).vtable(ctor)
    assert unit(tables(vftables={"CBoth": {0x0074A824}}), playable).vtable(ctor).variable.name == \
        "kotor_vtable_cswinventory_0074a824"
