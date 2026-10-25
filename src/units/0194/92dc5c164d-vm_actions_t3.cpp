// W7 tier t3: global variables (578-581), GetObjectByTag (200), GetPCSpeaker (238) and the conversation pause /
// resume ACTIONs (205/206), reconstructed from the original swkotor.exe (read-only, capstone 5.0.7).
// Native x86 translation unit. MEMORY: only live_operator_new / live_free (via the reused string/catalog owners).
// Reused existing owners: CSWGlobalVariableTable::FindIdentifierIndex 00529030 (0855 recovered_global_catalog),
// CSWSModule::FindTagPositionInTable 004C5E60 (1010 recovered_module_tag::lower_bound), CServerExoApp::GetModule
// 004AE6B0 (0530), GetPlayerCreatureId 004AEA40 / GetClientObjectByObjectId 004AE910 (0595), GetDeadTemp 004EF890
// (0963), return_zero 0063E7F0 (0296), CExoString 0077/0078/0125/0154/0188/0262/0288/0310/0459.
// CSWSObject::AddAction 004CEA20 (server action queue) is an import with a named-trap fallback.
#include "73dbba025f-vm_actions_t1.hpp"
#include "1675e588f9-vm_actions_t3.hpp"
#include "9f6f5e8482-global_catalog.hpp"   // recovered_global_catalog (0855), read-only include
#include "4d916f71f6-module_tag.hpp"       // recovered_module_tag (1010), read-only include
#include <cctype>
#include <cstring>
#if !defined(_M_IX86)
#error W7 ACTION bodies are native x86 (32-bit guest pointers)
#endif
namespace kotor_reconstruction {
struct StartupString { int BorrowedEquals005e5290(const char*); };   // 0077 CExoString::operator==(const char*)
void __cdecl StartupFormat005e5680(StartupString*, const char*, ...);  // 0125 CExoString::Format
}
namespace ini_comparison { int __fastcall CompareNC005e58c0(kotor_reconstruction::StartupString*, void*, const char*); }   // 0154
namespace directory_list { char __fastcall At005e5600(kotor_reconstruction::StartupString*, void*, int); }                 // 0288
namespace kotor_menu20 { struct String; String* __fastcall ConstructString005b3190(String*, void*); }                     // 0262
namespace frame_control { extern unsigned message_manager_007a3a00; }
namespace kotor_menu_window_cells { extern void* volatile manager_007a39fc; }
// CExoArrayList<CSWSTagNode>::Insert: byte-identical matched code (src/bridge); its CSWSTagNode passes by value.
namespace bridged_tag { struct Node { unsigned char bytes[40]; }; }
extern "C" void __fastcall kotor_fastcall_004c6520(void*, void*, bridged_tag::Node, int);
extern "C" {
void* __cdecl live_operator_new(std::uint32_t);
void __cdecl live_free(void*);
__declspec(noreturn) void __cdecl kotor_unreachable_trap(unsigned long address, const char* reason);
unsigned __fastcall recovered_vm_outer_push_integer_005d1010(void*, void*, unsigned);   // 1196
unsigned __fastcall recovered_vm_outer_push_object_005d10d0(void*, void*, unsigned);    // 1196
unsigned char __fastcall kotor_fastcall_004d8230(void*, void*, unsigned, void**);       // 0457
void* __fastcall server_get_module_004ae6b0(void*, void*);                              // 0530 CServerExoApp::GetModule
unsigned __fastcall kotor_fastcall_004aea40(void*, void*);                              // 0595 GetPlayerCreatureId
void* __fastcall kotor_fastcall_004ae910(void*, void*, unsigned);                       // 0595 GetClientObjectByObjectId
int __fastcall recovered_creature_incapacitated_004ef890(void*, void*);                 // 0963 CSWSCreature::GetDeadTemp
unsigned __fastcall gui_load_slot_zero_0063e7f0(void*, void*);                          // 0296 return_zero
void __fastcall frame_LogStringDtor_005e5c20(void*, void*);                             // 0459
const char* __fastcall WindowCStr005e5670(void*, void*);                                // 0078
void* __fastcall WindowCopy005e5b00(void*, void*, const void*);                         // 0078
unsigned __fastcall async_string_length_005e5790(void*, void*);                         // 0188
void* __fastcall lower_construct_005e6080(void*, void*, void*);                         // 0310
// CSWSObject::AddAction 004CEA20: thiscall, 28 stack words, RET 70h (server action queue owner)
int __fastcall recovered_object_add_action_004cea20(void*, void*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
    unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
    unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
int __fastcall w7_missing_add_action_004cea20(void*, void*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
    unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
    unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
}
#pragma comment(linker, "/alternatename:@recovered_object_add_action_004cea20@120=@w7_missing_add_action_004cea20@120")

namespace {
using U = std::uint32_t;
using I = std::int32_t;
using kotor_reconstruction::StartupString;
constexpr U kInvalidObject = 0x7f000000u;
constexpr I kPopFailed = -2001, kPushFailed = -2000;
void* ptr(U a) { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(a)); }
U word(const void* p) { return static_cast<U>(reinterpret_cast<std::uintptr_t>(p)); }
U rd(U a) { U v; std::memcpy(&v, ptr(a), 4); return v; }
void wr(U a, U v) { std::memcpy(ptr(a), &v, 4); }
U rd8(U a) { return *static_cast<const unsigned char*>(ptr(a)); }
void wr8(U a, U v) { *static_cast<unsigned char*>(ptr(a)) = static_cast<unsigned char>(v); }
void* vm() { return ptr(frame_control::message_manager_007a3a00); }
U server() { return rd(word(kotor_menu_window_cells::manager_007a39fc) + 8); }
U object_array_004aed70(U s) { return rd(rd(s + 4) + 0x1005c); }
U global_table_004aee60(U s) { return rd(s + 4) + 0x100fc; }   // mov eax,[ecx+4]; add eax,100FC (inlined; no exported owner)
U vcall(U object, U slot) { using F = U(__thiscall*)(void*); return reinterpret_cast<F>(rd(rd(object) + slot))(ptr(object)); }
struct Exo { char* data = nullptr; U allocated = 0; };
void exo_default(Exo* s) { (void)kotor_menu20::ConstructString005b3190(reinterpret_cast<kotor_menu20::String*>(s), nullptr); }
void exo_destroy(void* s) { frame_LogStringDtor_005e5c20(s, nullptr); }
// Format a diagnostic into a temporary and discard it, as the original does (the result is never used)
void diagnostic(const char* format, void* name) {
  Exo out; exo_default(&out);
  kotor_reconstruction::StartupFormat005e5680(reinterpret_cast<StartupString*>(&out), format, WindowCStr005e5670(name, nullptr));
  exo_destroy(&out);
}
// Ports for the 0855 catalogue owner, bound to the same live providers its own native adapter uses
struct CatalogPorts final : recovered_global_catalog::Ports {
  int length(recovered_global_catalog::String* s) override { return static_cast<int>(async_string_length_005e5790(s, nullptr)); }
  signed char character(recovered_global_catalog::String* s, int i) override { return static_cast<signed char>(directory_list::At005e5600(reinterpret_cast<StartupString*>(s), nullptr, i)); }
  int uppercase(int c) override { return std::toupper(c); }   // 00528E7C _toupper (C locale)
  const char* text(recovered_global_catalog::String* s) override { return WindowCStr005e5670(s, nullptr); }
  int equal_nc(recovered_global_catalog::String* s, const char* t) override { return ini_comparison::CompareNC005e58c0(reinterpret_cast<StartupString*>(s), nullptr, t); }
};
int find_00529030(U table, void* name) { CatalogPorts p; return recovered_global_catalog::find_00529030(ptr(table), static_cast<recovered_global_catalog::String*>(name), p); }
U encoded(U table, int slot) { return rd(table + static_cast<U>(slot) * 24 + 0x16) & 0xffffu; }   // movzx ax,[esi+slot*24+16]
// Ports for the 1010 module tag owner (only read/byte are reached by lower_bound)
struct TagPorts final : recovered_module_tag::Ports {
  U read(U a) override { return rd(a); }
  U byte(U a) override { return rd8(a); }
  void write(U a, U v) override { wr(a, v); }
  void put_byte(U a, U v) override { wr8(a, v); }
  U allocate(U n) override { return word(live_operator_new(n)); }
  void release(U a) override { live_free(ptr(a)); }
  void string_copy(U a, U b) override { (void)WindowCopy005e5b00(ptr(a), nullptr, ptr(b)); }
  U text(U a) override { return word(WindowCStr005e5670(ptr(a), nullptr)); }
  U equal(U a, U b) override { return static_cast<U>(static_cast<StartupString*>(ptr(a))->BorrowedEquals005e5290(static_cast<const char*>(ptr(b)))); }
  void destroy(U a) override { exo_destroy(ptr(a)); }
  void insert_004c6520(U list, U node, U index) override { bridged_tag::Node n; std::memcpy(&n, ptr(node), sizeof n); kotor_fastcall_004c6520(ptr(list), nullptr, n, static_cast<int>(index)); }
};
}

extern "C" {
// ============================================================================================ CSWGlobalVariableTable
// layout: 1775 x 24-byte slots {char name[22]; u16 (type<<14 | index)}, booleans MSB-first at +A668, numbers at +A6D9
void __fastcall recovered_global_set_bit_005290c0(void*, void*, void* bits, int index, int value) {
  const U cell = word(bits) + static_cast<U>(index >> 3);   // cdq/and 7/add/sar 3
  const U bit = (1u << ((static_cast<U>((index >> 3) * 8 - index + 7)) & 7u)) & 0xffu;
  wr8(cell, value ? (rd8(cell) | bit) : (rd8(cell) & ~bit & 0xffu));
}
void __fastcall recovered_global_get_boolean_00529110(void* t, void*, void* name, int* out) {
  const U self = word(t);
  const int slot = find_00529030(self, name);
  *out = 0;
  if (slot < 0) { diagnostic("Script var BOOLEAN '%s' not in catalogue!", name); return; }
  const U e = encoded(self, slot);
  if (e & 0xc000u) { diagnostic("Script var '%s' is not BOOLEAN!", name); return; }
  const U index = e & 0x3fffu;
  *out = static_cast<int>((rd8(self + 0xa668 + (index >> 3)) >> (7u - (index & 7u))) & 1u);
}
void __fastcall recovered_global_get_number_00529240(void* t, void*, void* name, signed char* out) {
  const U self = word(t);
  const int slot = find_00529030(self, name);
  *out = 0;
  if (slot < 0) { diagnostic("Script var NUMBER '%s' not in catalogue!", name); return; }
  const U e = encoded(self, slot);
  if ((e & 0xc000u) != 0x4000u) { diagnostic("Script var '%s' not a NUMBER!", name); return; }
  *out = static_cast<signed char>(rd8(self + 0xa6d9 + (e & 0x3fffu)));
}
void __fastcall recovered_global_set_boolean_00529570(void* t, void*, void* name, int value) {
  const U self = word(t);
  const int slot = find_00529030(self, name);
  if (slot < 0) { diagnostic("Script var BOOLEAN '%s' not in catalogue!", name); return; }
  const U e = encoded(self, slot);
  if (e & 0xc000u) { diagnostic("Script var '%s' not a BOOLEAN!", name); return; }
  recovered_global_set_bit_005290c0(t, nullptr, ptr(self + 0xa668), static_cast<int>(e & 0x3fffu), value);
}
void __fastcall recovered_global_set_number_00529680(void* t, void*, void* name, int value) {
  const U self = word(t);
  const int slot = find_00529030(self, name);
  if (slot < 0) { diagnostic("Script var NUMBER '%s' not in catalogue!", name); return; }
  const U e = encoded(self, slot);
  if ((e & 0xc000u) != 0x4000u) { diagnostic("Script var '%s' not a NUMBER!", name); return; }
  wr8(self + 0xa6d9 + (e & 0x3fffu), static_cast<U>(value) & 0xffu);
}

// ============================================================================================ ACTIONs
// 00538DC0 GetGlobalBoolean (578) / GetGlobalNumber (580). A failed string pop returns -2000 (original).
int __fastcall recovered_vm_get_global_value_00538dc0(void*, void*, int routine, unsigned) {
  Exo name; exo_default(&name);
  signed char result = 0;
  if (!recovered_vm_outer_pop_string_005d1080(vm(), nullptr, &name)) { exo_destroy(&name); return kPushFailed; }
  if (routine == 0x244) recovered_global_get_number_00529240(ptr(global_table_004aee60(server())), nullptr, &name, &result);
  else if (routine == 0x242) { int b = 0; recovered_global_get_boolean_00529110(ptr(global_table_004aee60(server())), nullptr, &name, &b); result = b != 0; }
  const U pushed = recovered_vm_outer_push_integer_005d1010(vm(), nullptr, static_cast<U>(static_cast<I>(result)));   // movsx
  exo_destroy(&name);
  return pushed ? 0 : kPushFailed;
}
// 00542B60 SetGlobalBoolean (579) / SetGlobalNumber (581). Pop failures return -2000 (original).
int __fastcall recovered_vm_set_global_value_00542b60(void*, void*, int routine, unsigned) {
  Exo name; exo_default(&name);
  U value = 0;
  if (!recovered_vm_outer_pop_string_005d1080(vm(), nullptr, &name) || !recovered_vm_outer_pop_integer_005d1000(vm(), nullptr, &value)) {
    exo_destroy(&name); return kPushFailed;
  }
  if (routine == 0x245) recovered_global_set_number_00529680(ptr(global_table_004aee60(server())), nullptr, &name, static_cast<int>(value));
  else if (routine == 0x243) recovered_global_set_boolean_00529570(ptr(global_table_004aee60(server())), nullptr, &name, value != 0);
  exo_destroy(&name);
  return 0;
}
// 004C5FE0 CSWSModule::FindObjectByTagOrdinal(const CExoString& tag, int nth)
unsigned __fastcall recovered_module_find_object_by_tag_004c5fe0(void* module, void*, const void* tag, int nth) {
  const U self = word(module);
  Exo copy; (void)WindowCopy005e5b00(&copy, nullptr, tag);
  TagPorts p;
  const U position = recovered_module_tag::lower_bound(self, word(WindowCStr005e5670(&copy, nullptr)), p) + static_cast<U>(nth);
  U id = kInvalidObject;
  if (static_cast<I>(position) < static_cast<I>(rd(self + 0x134))) {
    const U record = rd(self + 0x130) + position * 40;
    if (reinterpret_cast<StartupString*>(&copy)->BorrowedEquals005e5290(static_cast<const char*>(ptr(record)))) id = rd(record + 0x24);
  }
  exo_destroy(&copy);
  return id;
}
// 0053C280 GetObjectByTag(string, int nth): the tag is lowercased first
int __fastcall recovered_vm_get_object_by_tag_0053c280(void*, void*, int, unsigned) {
  Exo tag; exo_default(&tag);
  U nth = 0;
  if (!recovered_vm_outer_pop_string_005d1080(vm(), nullptr, &tag) || !recovered_vm_outer_pop_integer_005d1000(vm(), nullptr, &nth)) {
    exo_destroy(&tag); return kPopFailed;
  }
  Exo lower; (void)lower_construct_005e6080(&tag, nullptr, &lower);
  void* module = server_get_module_004ae6b0(ptr(server()), nullptr);
  if (!module) kotor_unreachable_trap(0x0053c303, "GetObjectByTag: no module (original calls FindObjectByTagOrdinal on null)");
  const U id = recovered_module_find_object_by_tag_004c5fe0(module, nullptr, &lower, static_cast<int>(nth));
  exo_destroy(&lower);
  const U pushed = recovered_vm_outer_push_object_005d10d0(vm(), nullptr, id);
  exo_destroy(&tag);
  return pushed ? 0 : kPushFailed;
}
// 0053C9C0 GetPCSpeaker: the player creature's client object, its +38 (no null check in the original)
int __fastcall recovered_vm_get_pc_speaker_0053c9c0(void*, void*, int, unsigned) {
  const U id = kotor_fastcall_004aea40(ptr(server()), nullptr);
  const U client = word(kotor_fastcall_004ae910(ptr(server()), nullptr, id));
  if (!client) kotor_unreachable_trap(0x0053c9dc, "GetPCSpeaker: no client object for the player creature (original reads [null+38])");
  return recovered_vm_outer_push_object_005d10d0(vm(), nullptr, rd(client + 0x38)) ? 0 : kPushFailed;
}
// 0052D330 ActionPauseConversation (no stack arguments)
int __fastcall recovered_vm_action_pause_conversation_0052d330(void* self, void*, int, unsigned) {
  const U s = word(self);
  if (rd(s + 4) != 1) return 0;
  void* found = nullptr;
  if (kotor_fastcall_004d8230(ptr(object_array_004aed70(server())), nullptr, rd(s + 8), &found) != 0) return 0;
  const U object = word(found);
  if (!object) kotor_unreachable_trap(0x0052d36c, "ActionPauseConversation: object array returned success with a null object");
  if (!vcall(object, 0x10)) return 0;                               // AsSWSObject
  if (!rd(vcall(object, 0x10) + 0xe8)) return 0;
  U dead = 0;
  if (vcall(object, 0x30)) dead = static_cast<U>(recovered_creature_incapacitated_004ef890(ptr(vcall(object, 0x30)), nullptr));
  const U o = vcall(object, 0x10);
  if (vcall(o, 0x94) == 1 || gui_load_slot_zero_0063e7f0(ptr(vcall(object, 0x10)), nullptr) == 1 || dead == 1) {
    using F = void(__thiscall*)(void*, U);
    const U so = vcall(object, 0x10);
    reinterpret_cast<F>(rd(rd(so) + 0xc0))(ptr(so), kInvalidObject);
    return 0;
  }
  wr(vcall(object, 0x10) + 0x50, 1);
  if (vcall(object, 0x30)) { const U c = vcall(object, 0x30); wr(c + 0xa00, rd(c + 0xa00) | 4u); }
  return 0;
}
// 0052D520 ActionResumeConversation: AddAction(0x20, 0xFFFF, 0 x26) on the commandable script owner
int __fastcall recovered_vm_action_resume_conversation_0052d520(void* self, void*, int, unsigned) {
  const U s = word(self);
  if (rd(s + 4) != 1) return 0;
  void* found = nullptr;
  if (kotor_fastcall_004d8230(ptr(object_array_004aed70(server())), nullptr, rd(s + 8), &found) != 0) return 0;
  if (!found) kotor_unreachable_trap(0x0052d552, "ActionResumeConversation: object array returned success with a null object");
  if (!rd(word(found) + 0xe8)) return 0;
  (void)recovered_object_add_action_004cea20(found, nullptr, 0x20, 0xffff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  return 0;
}
}
