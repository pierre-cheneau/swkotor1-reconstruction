"""cpp_scope.py: namespace-scope statements and their variables."""
from __future__ import annotations

import pytest

import cpp_scope

SOURCE = r'''#include <x.h>
  #define A 1 \
     continued { not code
namespace kotor_menu_window_cells {
// a comment with } brace
void* volatile manager_007a39fc = nullptr;
std::uint32_t volatile height_0078d1d8 = 600;
static const int hidden_007a0000 = 1;
int f_00401000(int a) { return a; }
struct S { int m; } s_007a1000;
const char* text_007a4000 = "a } brace";
char quote_007a5000 = '}';
}
namespace { int anon_007a2000 = 0; }
namespace a::b { long nested_007a6000 = 0x10; }
extern "C" { void* c_block_00831fdc=nullptr; extern int declared_007a3000; }
extern "C" const void* const table_0074a824[]={reinterpret_cast<void*>(&f)};
extern CAppManager* g_pAppManager;                  // 0x007A39FC
float g_vFogColor[3];
int& alias_007a39fc = x;
int x = 1, y = 2;
constexpr unsigned char k_found_0074666c = 0x00u;
'''


@pytest.fixture(scope="module")
def found() -> dict[str, cpp_scope.Variable]:
    return {v.name: v for v in map(cpp_scope.variables, cpp_scope.statements(SOURCE)) if v}


def test_only_plain_variables_are_read(found) -> None:
    assert "f_00401000" not in found and "s_007a1000" not in found      # a function, a class-typed declarator
    assert "x" not in found and "y" not in found                         # several declarators


@pytest.mark.parametrize(("name", "namespace", "c_linkage", "type_", "bounds", "defined", "external", "reference"), [
    ("manager_007a39fc", ("kotor_menu_window_cells",), False, "void* volatile", "", True, True, False),
    ("height_0078d1d8", ("kotor_menu_window_cells",), False, "std::uint32_t volatile", "", True, True, False),
    ("hidden_007a0000", ("kotor_menu_window_cells",), False, "const int", "", True, False, False),
    ("text_007a4000", ("kotor_menu_window_cells",), False, "const char*", "", True, True, False),
    ("quote_007a5000", ("kotor_menu_window_cells",), False, "char", "", True, True, False),
    ("anon_007a2000", (), False, "int", "", True, False, False),
    ("nested_007a6000", ("a", "b"), False, "long", "", True, True, False),
    ("c_block_00831fdc", (), True, "void*", "", True, True, False),
    ("declared_007a3000", (), True, "int", "", False, True, False),
    ("table_0074a824", (), True, "const void* const", "[]", True, True, False),
    ("g_pAppManager", (), False, "CAppManager*", "", False, True, False),
    ("g_vFogColor", (), False, "float", "[3]", True, True, False),
    ("alias_007a39fc", (), False, "int&", "", True, True, True),
    ("k_found_0074666c", (), False, "constexpr unsigned char", "", True, False, False),
])
def test_variables_carry_scope_linkage_and_type(found, name, namespace, c_linkage, type_, bounds, defined, external,
                                                reference) -> None:
    v = found[name]
    assert (v.statement.namespace, v.statement.c_linkage, v.type, v.bounds) == (namespace, c_linkage, type_, bounds)
    assert (v.defined, v.external, v.reference) == (defined, external, reference)


def test_initializers_keep_their_source(found) -> None:
    assert found["k_found_0074666c"].initializer == "0x00u"
    assert found["table_0074a824"].initializer == "{reinterpret_cast<void*>(&f)}"
    assert found["g_pAppManager"].initializer == ""


def test_spans_locate_the_statement_in_the_source(found) -> None:
    s = found["g_pAppManager"].statement
    assert SOURCE[s.start:s.end] == s.text == "extern CAppManager* g_pAppManager;"


@pytest.mark.parametrize(("words", "const"), [
    (["const", "int"], True), (["int", "const"], True), (["char", "*", "const"], True),
    (["const", "char", "*"], False), (["char", "const", "*"], False), (["constexpr", "int"], True), (["int"], False),
])
def test_top_level_const(words, const) -> None:
    assert cpp_scope.top_level_const(words) is const


def test_unbalanced_braces_are_refused() -> None:
    with pytest.raises(ValueError, match="Unbalanced"):
        list(cpp_scope.statements("int f() { return 1;\n"))
