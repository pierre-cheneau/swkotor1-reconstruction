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


@pytest.mark.parametrize(("demangled", "head"), [
    ("public: int __thiscall CServerExoAppInternal::ExecuteLoadModule(class CExoString &)",
     "int CServerExoAppInternal::ExecuteLoadModule(CExoString&)"),
    ("public: class CGuiPanel * __thiscall CApp::Panel(void)const ", "CGuiPanel* CApp::Panel() const"),
    ("public: __thiscall CExoString::CExoString(char const *)", "CExoString::CExoString(char const*)"),
    ("public: __thiscall CExoString::~CExoString(void)", "CExoString::~CExoString()"),
    ("public: class CExoString __thiscall CExoString::operator+(class CExoString const &)const ",
     "CExoString CExoString::operator+(CExoString const&) const"),
])
def test_forwards_jump_to_the_playable_entry(demangled: str, head: str) -> None:
    """Every kind of member is one naked jump: thiscall and its fastcall entry share the frame."""
    assert function(demangled, 0x004BA820).forward_definition("some_entry_004ba820") == (
        f"__declspec(naked) {head}\n{{\n    __asm jmp some_entry_004ba820\n}}\n")


def test_a_class_returned_by_value_is_built_at_the_hidden_pointer() -> None:
    f = function("public: class CExoString __thiscall CServerExoApp::GetModuleName(void)", 0x004AE6C0)
    assert (f.hidden, f.result, f.stack) == (True, "eax", 4)
    assert f.prototype() == "void* __fastcall kotor_fastcall_004ae6c0(void* self, void*, void* out)"
    assert ("    ::new (out) CExoString(static_cast<CServerExoApp*>(self)->GetModuleName());\n"
            "    return out;\n") in f.entry_definition()
    pointer = function("public: class CExoString * __thiscall A::B(void)")
    assert not pointer.hidden and pointer.result == "eax"


@pytest.mark.parametrize(("demangled", "stack", "result"), [
    (TYPICAL, 0, "eax"),
    ("public: void __thiscall A::B(int,class CExoString const &,double,__int64)", 24, "void"),
    ("public: float __thiscall A::B(enum E)", 4, "st0"),
    ("public: __int64 __thiscall A::B(void)", 0, "edx:eax"),
    ("public: __thiscall A::A(int)", 4, "eax"),
    ("public: int __thiscall A::B(struct Vector,int)", None, "eax"),
])
def test_stack_bytes_and_result_register(demangled: str, stack: int | None, result: str) -> None:
    f = function(demangled)
    assert (f.stack, f.result) == (stack, result)


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
    return bridge.Unit(Path("x.cpp"), [], {}, {}, tables_, data)


def unit_with(tmp_path: Path, source: str, signature: str, bridged: str | None = None) -> bridge.Unit:
    """A unit whose callee 004BA820 has signature; bridged, when given, is the bridged function's own signature."""
    playable = tmp_path / "playable.cpp"
    playable.write_text(source, encoding="utf-8")
    return bridge.Unit(Path("x.cpp"), [], {}, {0x004BA820: function(bridged, 0x004BA820)} if bridged else {},
                       tables({0x004BA820: {signature}}), bridge.PlayableData([playable]))


LOAD = "public: int __thiscall CServerExoAppInternal::ExecuteLoadModule(class CExoString &)"


@pytest.mark.parametrize("source", [
    'extern "C" int __fastcall kotor_fastcall_004ba820(void* self, void*, void* name) { return 0; }',
    'extern "C" {\nunsigned __fastcall kotor_fastcall_004ba820(void*,void*,void*) noexcept(false) {return 1;}\n}',
])
def test_forwards_find_the_playable_entry_in_either_linkage_form(tmp_path: Path, source: str) -> None:
    forward = unit_with(tmp_path, source, LOAD).forward(0x004BA820, bridge.ForwardSpec(), "")
    assert (forward.symbol, forward.function.name) == ("kotor_fastcall_004ba820", "ExecuteLoadModule")


def test_forwards_may_name_their_entry(tmp_path: Path) -> None:
    u = unit_with(tmp_path, 'extern "C" int __fastcall recovered_load_004ba820(void*, void*, void*) {return 0;}', LOAD)
    assert u.forward(0x004BA820, bridge.ForwardSpec("recovered_load_004ba820"), "").symbol == "recovered_load_004ba820"
    with pytest.raises(ValueError, match="needs exactly one"):
        u.forward(0x004BA820, bridge.ForwardSpec(), "")


@pytest.mark.parametrize(("source", "why"), [
    ('extern "C" int __fastcall kotor_fastcall_004ba820(void*, void*) {return 0;}', "pops 0 stack bytes, the original"),
    ('extern "C" void __fastcall kotor_fastcall_004ba820(void*, void*, void*) {}', "the original's result is in eax"),
    ('extern "C" int __cdecl kotor_fastcall_004ba820(void*, void*, void*) {return 0;}', "not a fastcall entry"),
    ('namespace { extern "C" int __fastcall kotor_fastcall_004ba820(void*, void*, void*) {return 0;} }',
     "needs exactly one"),
    ('extern "C" int __fastcall kotor_fastcall_004ba820(void*, void*, void*);', "needs exactly one"),
])
def test_forwards_keep_the_original_frame(tmp_path: Path, source: str, why: str) -> None:
    with pytest.raises(ValueError, match=why):
        unit_with(tmp_path, source, LOAD).forward(0x004BA820, bridge.ForwardSpec(), "")


def test_a_forward_to_a_bridged_function_jumps_to_its_generated_entry(tmp_path: Path) -> None:
    """The callers' view may differ from the defining file's (int vs unsigned long), never the frame."""
    defining = "public: unsigned long __thiscall CServerExoAppInternal::ExecuteLoadModule(class CExoString const &)"
    forward = unit_with(tmp_path, "", LOAD, defining).forward(0x004BA820, bridge.ForwardSpec(), "")
    assert forward.symbol == "kotor_fastcall_004ba820"
    other = "public: int __thiscall CServerExoAppInternal::ExecuteLoadModule(class CExoString &,int)"
    with pytest.raises(ValueError, match="with 4 stack bytes and its result in eax, the bridged entry 8 and eax"):
        unit_with(tmp_path, "", LOAD, other).forward(0x004BA820, bridge.ForwardSpec(), "")


def test_a_bridged_function_takes_its_defining_files_signature() -> None:
    defines = "public: int __thiscall CExoLocStringInternal::GetStringLength(unsigned long)"
    calls = "public: unsigned long __thiscall CExoLocStringInternal::GetStringLength(int)"
    t = bridge.Tables({}, {0x005ECA20: {defines, calls}}, {}, {}, {0x005ECA20: {defines}})
    assert t.signature(0x005ECA20) == defines
    assert t.signature(0x005ECA20, "class CExoLocStringInternal { unsigned long GetStringLength(int); };") == calls


def test_template_members_are_not_forwarded(tmp_path: Path) -> None:
    with pytest.raises(ValueError, match="is a template"):
        unit_with(tmp_path, "", "public: void __thiscall CExoArrayList<int>::Add(int)").forward(
            0x004BA820, bridge.ForwardSpec(), "")


def test_an_entry_src_already_defines_is_not_bridged_again(tmp_path: Path, monkeypatch) -> None:
    playable = tmp_path / "entry.cpp"
    playable.write_text('extern "C" unsigned char __fastcall kotor_fastcall_00406040(void* self, void*) {return 0;}',
                        encoding="utf-8")
    spec = tmp_path / "bridge.json"
    spec.write_text(json.dumps({"functions": [{"address": "00406040"}]}), encoding="utf-8")
    monkeypatch.setattr(bridge, "SPEC", spec)
    monkeypatch.setattr(bridge, "pinned_sources", lambda: [playable])
    with pytest.raises(ValueError, match="already defines kotor_fastcall_00406040"):
        bridge.units()


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


INSTANTIATED = """\
template <class T> struct List { int Contains(T* p) { return p != 0; } int Count() { return 0; } };
// The members below are identical for every T; the linker folded them.
// FUNCTION: SWKOTOR 0x005D6CC0 SYMBOL: ?Contains@?$List@H@@QAEHPAH@Z
// FUNCTION: SWKOTOR 0x005D6D00 SYMBOL: ?Count@?$List@H@@QAEHXZ
template class List<int>;

// FUNCTION: SWKOTOR 0x005D78F0
int After() { return 2; }
"""


def test_members_of_an_explicit_instantiation_come_from_the_template() -> None:
    """The instantiation line goes either way: it would emit every member, the unselected ones' callees included."""
    text, kept = bridge.select(INSTANTIATED, {0x005D6CC0})
    assert kept == {0x005D6CC0} and "template class" not in text and "After" not in text
    assert text.startswith("template <class T> struct List")
    assert bridge.select(INSTANTIATED, {0x005D78F0})[1] == {0x005D78F0}
    stray = INSTANTIATED.replace("// FUNCTION: SWKOTOR 0x005D6D00", "// a note\n// FUNCTION: SWKOTOR 0x005D6D00")
    with pytest.raises(ValueError, match="compiler-generated"):
        bridge.select(stray, {0x005D6CC0})       # not a plain run of annotations over the line: not guessed


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


def test_storage_the_build_generates_binds_like_a_definition(playable) -> None:
    rules = playable.storage(0x007A3A28)                  # local_generators/game_prepare.py's cell
    assert rules.qualified == "::kotor_cell_rules_007a3a28" and rules.size == 4
    assert rules.declaration() == 'extern "C" void* kotor_cell_rules_007a3a28;'


def test_constants_need_every_definition_to_agree(playable) -> None:
    assert playable.constant(0x0074666C) == ("0", 2)
    assert playable.constant(0x00746640) is None          # 1 and 2 disagree
    assert playable.constant(0x007A39FC) is None          # nullptr is not a literal


MATCHED_GLOBALS = """\
class CAppManager;
extern CAppManager* g_pAppManager;                  // 0x007A39FC
extern float g_vFogColor[3];
extern const unsigned char GAME_OBJECT_VALID;
extern unsigned char g_nDoorStateClosed;             // declared writable, but in the original's .rdata
extern int g_unused;
extern HWND g_window;
int use() { return g_pAppManager != 0 && g_vFogColor[0] > 0 && GAME_OBJECT_VALID == g_nDoorStateClosed && g_window; }
"""


def test_used_globals_bind_to_storage_or_agreed_constants(playable) -> None:
    t = tables(globals_={"g_pAppManager": {0x007A39FC}, "g_vFogColor": {0x0078DB24}, "GAME_OBJECT_VALID": {0x0074666C},
                         "g_nDoorStateClosed": {0x0074666C}, "g_unused": {0x00700000},
                         "g_window": {0x007A39D8, 0x007A39DC}})
    body, bound, unbound = unit(t, playable).bind_globals(MATCHED_GLOBALS)
    assert [(name, type(target).__name__) for name, target in bound] == [
        ("g_pAppManager", "Storage"), ("GAME_OBJECT_VALID", "Constant"), ("g_nDoorStateClosed", "Constant")]
    assert unbound == [
        ("g_vFogColor", "0078DB24: 12 bytes, the playable ::kotor_menu_window_cells::fog_0078db24 holds 4"),
        ("g_window", "2 original addresses in symbols.csv")]
    assert "kotor_bridge::ref<CAppManager*> g_pAppManager = reinterpret_cast<kotor_bridge::ref<CAppManager*>>" \
           "(const_cast<kotor_bridge::storage<decltype(::kotor_menu_window_cells::manager_007a39fc)>&>" \
           "(::kotor_menu_window_cells::manager_007a39fc));                  // 0x007A39FC" in body
    assert 'static_assert(sizeof(CAppManager*) <= sizeof(::kotor_menu_window_cells::manager_007a39fc)' in body
    assert "const unsigned char GAME_OBJECT_VALID = 0;" in body
    assert "unsigned char const g_nDoorStateClosed = 0;" in body       # a write would not compile
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


def test_direct_c_heap_calls_are_rebound_to_the_game_heap() -> None:
    body = "void f(void* p) { free(p); char* q = (char*)malloc(4); x.free(q); ::y->malloc(1); }"
    assert bridge.heap_calls(Path("x.cpp"), body) == ["free", "malloc"]
    assert bridge.heap_calls(Path("x.cpp"), "int g() { return freed(1); }") == []


@pytest.mark.parametrize("call", ["realloc(p, 8)", "calloc(1, 4)", "_msize(p)", "_strdup(s)", "strdup(s)"])
def test_heap_calls_without_a_game_heap_counterpart_are_refused(call: str) -> None:
    with pytest.raises(ValueError, match="no game-heap counterpart"):
        bridge.heap_calls(Path("x.cpp"), f"void f() {{ {call}; }}")


@pytest.mark.parametrize(("declaration", "params"), [
    ("void*& pos", ["void*&"]),
    ("CExoLinkedListPosition& pos", ["void*&"]),          # through the body's typedef
    ("unsigned long", ["unsigned long"]),
    ("unsigned long nIndex, int n = 0", ["unsigned long", "int"]),
    ("const CExoString& s", ["CExoString const&"]),
    ("const char* text", ["char const*"]),
    ("Space::Type", ["Space::Type"]),
    ("void", []),
])
def test_declared_parameters_are_spelled_as_the_demangler_spells_them(declaration: str, params: list[str]) -> None:
    assert bridge.declared_params(declaration, {"CExoLinkedListPosition": "void*"}) == params


def test_overloads_resolve_by_their_parameters() -> None:
    node = "public: void * __thiscall CExoLinkedListInternal::GetNext(struct CExoLinkedListNode * &)"
    void = "public: void * __thiscall CExoLinkedListInternal::GetNext(void * &)"
    body = ("typedef void* CExoLinkedListPosition;\nclass CExoLinkedListInternal {\npublic:\n"
            "    void* GetNext(CExoLinkedListPosition& pos);   // 0x005E9B90\n};")
    assert bridge.Tables({}, {0x005E9B90: {node, void}}, {}, {}).signature(0x005E9B90, body) == void


HELPERS = """\
CWorldJournalEntry* CopyWorldJournalEntry(const CWorldJournalEntry& e)
{
    return new CWorldJournalEntry(e);
}
static int Clamp(int n) { return n < 0 ? 0 : n; }
inline int Twice(int n) { return 2 * n; }
static inline int Thrice(int n) { return 3 * n; }
template <class T> T Same(T t) { return t; }
int table[2] = {1, 2};

// FUNCTION: SWKOTOR 0x005F7DC0
// a note between the annotation and its definition
int CThing::Selected(int n)
{
    return Clamp(n);
}
"""


def test_unannotated_helpers_become_inline_and_selected_definitions_stay_verbatim() -> None:
    text = bridge.inline_helpers(HELPERS)
    assert "inline CWorldJournalEntry* CopyWorldJournalEntry(" in text
    assert "inline static int Clamp(" in text
    assert "inline int Twice(" in text and "inline inline" not in text and "static inline int Thrice(" in text
    assert "inline template" not in text and "inline int table" not in text
    assert "// a note between the annotation and its definition\nint CThing::Selected(int n)" in text


CLASSES = """\
struct Vector { Vector() {} float x, y, z; };
struct Plain { int a; char* b; };
enum Kind { KIND_A };
typedef unsigned long OBJECT_ID;
class CExoLinkedListInternal { public: CExoLinkedListInternal(); void* pHead; void* pTail; int nCount; };
class CHolder { public: CHolder(); Plain plain; Kind kind; OBJECT_ID id; static int s_count; };
class CMesh { public: CMesh(unsigned short a, unsigned short b); virtual ~CMesh(); int n; };
class CDoor : public CMesh { public: CDoor(); };
class CPoint { public: CPoint(); Vector v; };
class CDefault { public: CDefault(); int n = 3; };
class CEntry { public: ~CEntry(); char pad[12]; };
"""


@pytest.mark.parametrize(("owner", "kind", "why"), [
    ("CExoLinkedListInternal", "constructor", None),
    ("CHolder", "constructor", None),                    # scalar, enum and typedef members; statics ignored
    ("CEntry", "destructor", None),
    ("CMesh", "constructor", "CMesh has a table pointer"),
    ("CMesh", "destructor", "CMesh has a table pointer"),
    ("CDoor", "constructor", "CDoor has bases 'public CMesh'"),
    ("CPoint", "constructor", "CPoint::v is a Vector, whose default constructor runs first"),
    ("CDefault", "constructor", "CDefault::n has a default member initializer"),
    ("CAbsent", "constructor", "CAbsent's definition is not in the slice"),
])
def test_naked_special_member_forwards_need_no_implicit_work(owner: str, kind: str, why: str | None) -> None:
    """A naked constructor still runs the compiler's table, base and member construction, with no frame."""
    found = bridge.implicit_work(owner, kind, CLASSES)
    assert found == why if why is None else found.startswith(why)


def test_a_bridged_constructor_class_gets_no_table_of_its_own() -> None:
    body = "class CSWInventory : public CBase {\npublic:\n    CSWInventory();\n};\nvoid f(CSWInventory* p);\n"
    assert bridge.without_vtable(body, "CSWInventory", Path("x.cpp")).startswith(
        "class __declspec(novtable) CSWInventory : public CBase {")
    for constructs in ("CSWInventory* p = new CSWInventory;", "void g() { CSWInventory local; }",
                       "struct H { CSWInventory member; };"):
        with pytest.raises(ValueError, match="cannot construct one itself"):
            bridge.without_vtable(body + constructs, "CSWInventory", Path("x.cpp"))


def test_a_forward_may_give_the_signature_symbols_csv_lacks(tmp_path: Path) -> None:
    """~CExoString shares 005E5C20 with folded destructors: symbols.csv names those only."""
    u = unit_with(tmp_path, 'extern "C" void __fastcall frame_LogStringDtor_005e5c20(void*, void*) {}', LOAD)
    spec = bridge.ForwardSpec("frame_LogStringDtor_005e5c20", "public: __thiscall CExoString::~CExoString(void)")
    body = "class CExoString {\npublic:\n    ~CExoString();                 // 0x005E5C20\n    char* m_sString;\n};"
    forward = u.forward(0x004BA820, spec, body)
    assert (forward.function.name, forward.symbol) == ("~CExoString", "frame_LogStringDtor_005e5c20")
    with pytest.raises(ValueError, match="the slice declares no"):
        u.forward(0x004BA820, spec, "class CExoString { public: char* m_sString; };")
