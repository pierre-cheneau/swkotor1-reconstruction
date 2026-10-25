// W7 tier t4: the spawn-path object/creature queries and setters, reconstructed from the original swkotor.exe
// (read-only, capstone 5.0.7). Native x86 translation unit. MEMORY: only live_operator_new / live_free.
// Reused existing owners (1123): CServerExoApp::GetCreatureByGameObjectID 004AE770 (0595), GetGameObject 004AE750
// (0530), GetModule 004AE6B0 (0530), CGameObjectArray 004D8230 (0457), CSWSCreatureStats::GetLevel 005A5FD0 (0963),
// GetClass 005A4E90 (0982 namespace function), CSWSObject::GetArea 004CB120 (1060), vector_magnitude 004AB100 (0464),
// _rand 006FC447 (0479), CSWSpellArray::GetSpell 0059B6D0 / C2DA::GetFLOATEntry 00413350 (0820), rules cells (0431),
// AddEventDeltaTime 004B08D0 (1017), the 1010 module tag owner, CExoString providers.
// Imports with named-trap fallbacks (w7_missing_t4.cpp): CSWSCreature::SetLockOrientationToObject 004F34A0 (W5 t3
// export w5_creature_set_lock_orientation_to_object_004f34a0).
// Inlined (no exported owner, or owned in another lane's unintegrated namespace code): CSWSCreatureStats::GetClassLevel
// 005A5090 (W2 t4 export @recovered_stats_class_level_005a5090@16, hard import since t6), CServerExoApp::GetFactionOfObject 004AE950
// forwarder, GetServerAIMaster 004AED80.
#include "73dbba025f-vm_actions_t1.hpp"
#include "d09f202cd1-vm_actions_t4.hpp"
#include "4d916f71f6-module_tag.hpp"
#include "d2fbe17f4b-server_mainloop.hpp"
#include <cstring>
#if !defined(_M_IX86)
#error W7 ACTION bodies are native x86 (32-bit guest pointers)
#endif
namespace kotor_reconstruction { struct StartupString { int BorrowedEquals005e5290(const char*); }; }
namespace kotor_menu20 { struct String; String* __fastcall ConstructString005b3190(String*, void*); }
namespace frame_control { extern unsigned message_manager_007a3a00; }
namespace kotor_menu_window_cells { extern void* volatile manager_007a39fc; }
namespace kotor::server_mainloop { Word creature_get_class_005a4e90(Memory&, Word stats, Word index); }   // 0982
extern "C" unsigned __fastcall recovered_stats_class_level_005a5090(void*, void*, unsigned, unsigned);   // W2 t4 (hard import, t6)
// CExoArrayList<CSWSTagNode>::Insert: byte-identical matched code (src/bridge); its CSWSTagNode passes by value.
namespace bridged_tag { struct Node { unsigned char bytes[40]; }; }
extern "C" void __fastcall kotor_fastcall_004c6520(void*, void*, bridged_tag::Node, int);
extern "C" {
void* __cdecl live_operator_new(std::uint32_t);
void __cdecl live_free(void*);
__declspec(noreturn) void __cdecl kotor_unreachable_trap(unsigned long address, const char* reason);
extern void* kotor_cell_rules_007a3a28;                                               // g_pRules
extern unsigned kotor_rules_cell32_007a2184, kotor_rules_cell32_007a2188;             // cached 2DA column indices
unsigned __fastcall recovered_vm_outer_push_integer_005d1010(void*, void*, unsigned);
unsigned __fastcall recovered_vm_outer_push_object_005d10d0(void*, void*, unsigned);
unsigned __fastcall recovered_vm_outer_pop_object_005d10c0(void*, void*, void*);
unsigned __fastcall recovered_vm_push_float_005d1880(void*, void*, unsigned);         // 1196
unsigned char __fastcall kotor_fastcall_004d8230(void*, void*, unsigned, void**);
void* __fastcall kotor_fastcall_004ae750(void*, void*, unsigned);
void* __fastcall kotor_fastcall_004ae770(void*, void*, unsigned);
void* __fastcall server_get_module_004ae6b0(void*, void*);
unsigned char __fastcall recovered_stats_level_005a5fd0(void*, void*, unsigned);
void* __fastcall recovered_object_area_004cb120(void*, void*);
double __cdecl vector_magnitude_004ab100(const float*);
int __cdecl crt_rand_006fc447();
void* __fastcall kotor_m15b_spell_array_get_spell_0059b6d0(void*, void*, int);
int __fastcall kotor_m15b_c2da_get_float_entry_00413350(void*, void*, int, unsigned, float*);
unsigned __fastcall recovered_event_enqueue_004b08d0(void*, void*, unsigned, unsigned, unsigned, unsigned, unsigned, void*);
void __fastcall frame_LogStringDtor_005e5c20(void*, void*);
const char* __fastcall WindowCStr005e5670(void*, void*);
void* __fastcall WindowCopy005e5b00(void*, void*, const void*);
void* __fastcall async_assign_string_005e5c50(void*, void*, const void*);
void* __fastcall lower_construct_005e6080(void*, void*, void*);
void __fastcall w5_creature_set_lock_orientation_to_object_004f34a0(void*, void*, unsigned, unsigned);
void __fastcall w7_missing_set_lock_orientation_004f34a0(void*, void*, unsigned, unsigned);
}
#pragma comment(linker, "/alternatename:@w5_creature_set_lock_orientation_to_object_004f34a0@16=@w7_missing_set_lock_orientation_004f34a0@16")

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
U rd16(U a) { std::uint16_t v; std::memcpy(&v, ptr(a), 2); return v; }
void* vm() { return ptr(frame_control::message_manager_007a3a00); }
U app() { return word(kotor_menu_window_cells::manager_007a39fc); }
U server() { return rd(app() + 8); }
U object_array_004aed70(U s) { return rd(rd(s + 4) + 0x1005c); }
U vcall(U object, U slot) { using F = U(__thiscall*)(void*); return reinterpret_cast<F>(rd(rd(object) + slot))(ptr(object)); }
I push_int(U v) { return recovered_vm_outer_push_integer_005d1010(vm(), nullptr, v) ? 0 : kPushFailed; }
bool pop_object(U& id) { return recovered_vm_outer_pop_object_005d10c0(vm(), nullptr, &id) != 0; }
bool pop_int(U& v) { return recovered_vm_outer_pop_integer_005d1000(vm(), nullptr, &v) != 0; }
// CGameObjectArray::GetGameObject == 0 ([0074666C]); the original uses the object without a null check
bool lookup(U id, U& object) { void* out = nullptr; const bool ok = kotor_fastcall_004d8230(ptr(object_array_004aed70(server())), nullptr, id, &out) == 0; object = word(out); return ok; }
U creature_by_id(U id) { return word(kotor_fastcall_004ae770(ptr(server()), nullptr, id)); }
struct Exo { char* data = nullptr; U allocated = 0; };
void exo_default(Exo* s) { (void)kotor_menu20::ConstructString005b3190(reinterpret_cast<kotor_menu20::String*>(s), nullptr); }
void exo_destroy(void* s) { frame_LogStringDtor_005e5c20(s, nullptr); }
struct Mem final : kotor::server_mainloop::Memory {
  U read(U a) override { return rd(a); }
  void write(U a, U v) override { wr(a, v); }
  U read16(U a) override { return rd16(a); }
  void write16(U a, U v) override { const std::uint16_t x = static_cast<std::uint16_t>(v); std::memcpy(ptr(a), &x, 2); }
  U read8(U a) override { return rd8(a); }
};
U get_class_005a4e90(U stats, U index) { Mem m; return kotor::server_mainloop::creature_get_class_005a4e90(m, stats, index) & 0xffu; }
// 005A5090 CSWSCreatureStats::GetClassLevel(index, flag): W2 t4 export (t6: the internal W7 copy is removed)
U class_level_005a5090(U stats, U index, U flag) { return recovered_stats_class_level_005a5090(ptr(stats), nullptr, index, flag); }
struct TagPorts final : recovered_module_tag::Ports {
  U read(U a) override { return rd(a); }
  U byte(U a) override { return rd8(a); }
  void write(U a, U v) override { wr(a, v); }
  void put_byte(U a, U v) override { *static_cast<unsigned char*>(ptr(a)) = static_cast<unsigned char>(v); }
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
// 005D1030: mov ecx,[ecx]; jmp CVirtualMachineInternal::StackPushFloat
unsigned __fastcall recovered_vm_outer_push_float_005d1030(void* v, void*, unsigned bits) { return recovered_vm_push_float_005d1880(ptr(rd(word(v))), nullptr, bits); }

// 0059B8A0 CSWSpell::GetAdjustedForcePoints(level, kind): base cost +14E; kind != 0 scales it by the spells.2da
// column for the spell's school (+14F 'G' or 'E'), via C2DA::GetFLOATEntry(level/10 ... see below)
int __fastcall recovered_spell_adjusted_force_points_0059b8a0(void* spell, void*, U level, U kind) {
  const U s = word(spell);
  const U row = static_cast<U>((static_cast<unsigned long long>(level) * 0xcccccccdull) >> 35);   // mul/shr 3: level / 10
  const I base = static_cast<I>(rd8(s + 0x14e));
  if (!kind) return base;
  float scale = 1.0f;
  const U school = rd8(s + 0x14f);
  const U table = rd(rd(word(kotor_cell_rules_007a3a28) + 0xb8) + 0xd8);
  if (school == 0x47) (void)kotor_m15b_c2da_get_float_entry_00413350(ptr(table), nullptr, static_cast<int>(row), kotor_rules_cell32_007a2184, &scale);
  else if (school == 0x45) (void)kotor_m15b_c2da_get_float_entry_00413350(ptr(table), nullptr, static_cast<int>(row), kotor_rules_cell32_007a2188, &scale);
  // FILD/FMUL (exact in double), then 006FAE8C __ftol2: truncation, low dword of the int64; NaN/inf/out of int64 -> 0
  const double v = static_cast<double>(base) * static_cast<double>(scale);
  if (!(v >= -9223372036854775808.0 && v < 9223372036854775808.0)) return 0;
  return static_cast<int>(static_cast<U>(static_cast<std::uint64_t>(static_cast<std::int64_t>(v))));
}
// 005A6E70 CSWSCreatureStats::HasSpell(first class byte, spell, check force points)
int __fastcall recovered_stats_has_spell_005a6e70(void* stats, void*, U first, U spell, U check) {
  const U self = word(stats);
  int found = 0;
  const U classes = rd8(self + 0x89);
  if (classes) {
    U entry = self + 0x8c + (first & 0xffu) * 12;   // lea ecx,[c*3]; shl 2 (the caller passes 0)
    for (U k = 0; k < classes; ++k, entry += 0x28) {
      const U count = rd16(entry + 4), list = rd(entry);
      for (U i = 0; i < count; ++i) if (rd(list + 4 * i) == spell) { found = 1; break; }
    }
  }
  if (!check) return found;
  if (!found) return 0;
  void* s = kotor_m15b_spell_array_get_spell_0059b6d0(ptr(rd(word(kotor_cell_rules_007a3a28) + 0x8c)), nullptr, static_cast<int>(spell));
  if (!s) kotor_unreachable_trap(0x005a6f14, "HasSpell: CSWSpellArray::GetSpell returned null (original calls through it)");
  const I cost = recovered_spell_adjusted_force_points_0059b8a0(s, nullptr, static_cast<U>(static_cast<I>(static_cast<std::int16_t>(rd16(self + 0x17e)))), rd(rd(self + 0x24) + 0xa88));
  const I points = static_cast<std::int16_t>(static_cast<std::uint16_t>(rd16(self + 0x126) + rd16(self + 0x124)));
  return cost > points ? 0 : 1;
}
// 004B40C0 CServerExoAppInternal::GetFactionOfObject(id, out): creature stats +78, door +2B8, trigger(5C) +22C,
// placeable +23C, encounter(40) +2B8; returns 1 when written
int __fastcall recovered_server_faction_of_object_004b40c0(void* internal, void*, U id, U* out) {
  void* found = nullptr;
  if (kotor_fastcall_004d8230(ptr(rd(word(internal) + 0x1005c)), nullptr, id, &found) != 0 || !found) return 0;
  const U o = word(found);
  if (vcall(o, 0x30)) { *out = rd(rd(vcall(o, 0x30) + 0xa74) + 0x78); return 1; }
  if (vcall(o, 0x18)) { *out = rd(vcall(o, 0x18) + 0x2b8); return 1; }
  if (vcall(o, 0x5c)) { *out = rd(vcall(o, 0x5c) + 0x22c); return 1; }
  if (vcall(o, 0x48)) { *out = rd(vcall(o, 0x48) + 0x23c); return 1; }
  if (vcall(o, 0x40)) { *out = rd(vcall(o, 0x40) + 0x2b8); return 1; }
  return 0;
}
// 004C60B0 CSWSModule::FindObjectByTagTypeOrdinal(tag, type, nth)
unsigned __fastcall recovered_module_find_object_by_tag_type_004c60b0(void* module, void*, const void* tag, U type, I nth) {
  const U self = word(module);
  Exo lower; (void)lower_construct_005e6080(const_cast<void*>(tag), nullptr, &lower);
  TagPorts p;
  U index = recovered_module_tag::lower_bound(self, word(WindowCStr005e5670(&lower, nullptr)), p);
  U objects = 0;
  for (; static_cast<I>(index) < static_cast<I>(rd(self + 0x134)); ++index) {
    const U record = rd(self + 0x130) + index * 40;
    if (!reinterpret_cast<StartupString*>(&lower)->BorrowedEquals005e5290(static_cast<const char*>(ptr(record)))) break;
    if (!objects) objects = object_array_004aed70(server());
    void* found = nullptr;
    const bool ok = kotor_fastcall_004d8230(ptr(objects), nullptr, rd(record + 0x24), &found) == 0;
    if (ok && !found) kotor_unreachable_trap(0x004c6158, "FindObjectByTagTypeOrdinal: object array returned success with a null object");
    if (ok && rd8(word(found) + 8) == type) {
      if (nth == 0) { const U id = rd(record + 0x24); exo_destroy(&lower); return id; }
      --nth;
    }
  }
  exo_destroy(&lower);
  return kInvalidObject;
}
// 004C6E00 CSWSModule::GetWaypoint(tag): type [0074663B] = 12
unsigned __fastcall recovered_module_get_waypoint_004c6e00(void* module, void*, const void* tag) { return recovered_module_find_object_by_tag_type_004c60b0(module, nullptr, tag, 12, 0); }

// ============================================================================================ ACTIONs
// 00539440 GetHasSpell(int spell, object creature = OBJECT_SELF)
int __fastcall recovered_vm_get_has_spell_00539440(void*, void*, int, unsigned) {
  U spell = 0, id = 0, result = 0;
  if (!pop_int(spell) || !pop_object(id)) return kPopFailed;
  const U c = creature_by_id(id);
  if (c) result = static_cast<U>(recovered_stats_has_spell_005a6e70(ptr(rd(c + 0xa74)), nullptr, 0, spell, 1));
  return push_int(result);
}
// 0053DF00 GetTag(object): module AsModule()+1F8, area AsArea()+158, otherwise object+18; empty when unknown
int __fastcall recovered_vm_get_tag_0053df00(void*, void*, int, unsigned) {
  Exo tag; exo_default(&tag);
  U id = 0, object = 0;
  if (!pop_object(id)) { exo_destroy(&tag); return kPopFailed; }
  if (lookup(id, object)) {
    const U type = rd8(object + 8);
    const U source = type == 3 ? vcall(object, 0x20) + 0x1f8 : type == 4 ? vcall(object, 0x28) + 0x158 : object + 0x18;
    (void)async_assign_string_005e5c50(&tag, nullptr, ptr(source));
  }
  const U pushed = recovered_vm_outer_push_string_005d1090(vm(), nullptr, &tag);
  exo_destroy(&tag);
  return pushed ? 0 : kPushFailed;
}
// 0053E2B0 GetWaypointByTag(string)
int __fastcall recovered_vm_get_waypoint_by_tag_0053e2b0(void*, void*, int, unsigned) {
  Exo tag; exo_default(&tag);
  U id = kInvalidObject;
  if (!recovered_vm_outer_pop_string_005d1080(vm(), nullptr, &tag)) { exo_destroy(&tag); return kPopFailed; }
  if (app() && server()) {
    void* module = server_get_module_004ae6b0(ptr(server()), nullptr);
    if (module) { Exo lower; (void)lower_construct_005e6080(&tag, nullptr, &lower); id = recovered_module_get_waypoint_004c6e00(module, nullptr, &lower); exo_destroy(&lower); }
  }
  const U pushed = recovered_vm_outer_push_object_005d10d0(vm(), nullptr, id);
  exo_destroy(&tag);
  return pushed ? 0 : kPushFailed;
}
// 0053CBA0 GetRacialType(object): creature (type byte 5) stats +DC (word), else 0
int __fastcall recovered_vm_get_racial_type_0053cba0(void*, void*, int, unsigned) {
  U id = 0, object = 0, result = 0;
  if (!pop_object(id)) return kPopFailed;
  if (lookup(id, object)) result = rd8(object + 8) == 5 ? rd16(rd(vcall(object, 0x30) + 0xa74) + 0xdc) : 0;
  return push_int(result);
}
// 00545D80 GetStandardFaction(object): -1 without a server internal or faction manager (+10054)
int __fastcall recovered_vm_get_standard_faction_00545d80(void*, void*, int, unsigned) {
  U id = 0, faction = 0xffffffffu;
  if (!pop_object(id)) return kPopFailed;
  const U internal = rd(server() + 4);
  if (internal && rd(internal + 0x10054)) {
    (void)recovered_server_faction_of_object_004b40c0(ptr(internal), nullptr, id, &faction);   // 004AE950 forwarder
    // 00545DE4: `if (f < 1 && f > 6) f = -1` -- unsatisfiable in the original, kept as written
    if (static_cast<I>(faction) < 1 && static_cast<I>(faction) > 6) faction = 0xffffffffu;
  }
  return push_int(faction);
}
// 00546D90 GetSubRace(object): stats +E8 (byte); for a non-creature the popped id itself is pushed (original)
int __fastcall recovered_vm_get_subrace_00546d90(void*, void*, int, unsigned) {
  U id = 0;
  if (!pop_object(id)) return kPopFailed;
  const U c = creature_by_id(id);
  U result = id;
  if (c && rd(c + 0xa74)) result = rd8(rd(c + 0xa74) + 0xe8);
  return push_int(result);
}
// 005419D0 Random(int n): rand() % n, negated for negative n; 0 for n == 0
int __fastcall recovered_vm_random_005419d0(void*, void*, int, unsigned) {
  U n = 0;
  if (!pop_int(n)) return kPopFailed;
  I result = 0;
  if (n) {
    const I d = static_cast<I>(n);
    result = crt_rand_006fc447() % d;    // IDIV: rand() >= 0, so the remainder is >= 0 (n == INT_MIN gives rand())
    if (d < 0) result = -result;
  }
  return push_int(static_cast<U>(result));
}
// 00539570 GetHitDice(object): creatures (type > [00746633]) with AsCreature: GetLevel(0) (byte)
int __fastcall recovered_vm_get_hit_dice_00539570(void*, void*, int, unsigned) {
  U id = 0, object = 0, result = 0;
  if (!pop_object(id)) return kPopFailed;
  if (lookup(id, object) && rd8(object + 8) > 4 && vcall(object, 0x30))
    result = recovered_stats_level_005a5fd0(ptr(rd(vcall(object, 0x30) + 0xa74)), nullptr, 0);
  return push_int(result);
}
// 00539C10 GetIsEncounterCreature(object): creature +A24 != OBJECT_INVALID
int __fastcall recovered_vm_get_is_encounter_creature_00539c10(void*, void*, int, unsigned) {
  U id = 0;
  if (!pop_object(id)) return kPopFailed;
  const U c = creature_by_id(id);
  return push_int(c && rd(c + 0xa24) != kInvalidObject ? 1u : 0u);
}
// 00545F90 SetMinOneHP(object, int): AsSWSObject()+200 = (value != 0)
int __fastcall recovered_vm_set_min_one_hp_00545f90(void*, void*, int, unsigned) {
  U id = 0, value = 0;
  if (!pop_object(id) || !pop_int(value)) return kPopFailed;
  const U o = word(kotor_fastcall_004ae750(ptr(server()), nullptr, id));
  if (o && vcall(o, 0x10)) wr(vcall(o, 0x10) + 0x200, value ? 1u : 0u);
  return 0;
}
// 00537960 GetDistanceToObject2D(object): -1.0 unless the script owner and the target share an area
int __fastcall recovered_vm_get_distance_to_object_2d_00537960(void* self, void*, int, unsigned) {
  U id = 0, owner = 0, target = 0;
  float result = -1.0f;
  if (!pop_object(id)) return kPopFailed;
  const U s = word(self);
  if (rd(s + 4) == 1 && lookup(rd(s + 8), owner)) {
    if (!owner) kotor_unreachable_trap(0x005379c6, "GetDistanceToObject2D: owner lookup succeeded with a null object");
    float a[3]; std::memcpy(a, ptr(owner + 0x90), 12);
    const U area = word(recovered_object_area_004cb120(ptr(owner), nullptr));
    if (lookup(id, target) && target && rd8(target + 8) > 4) {
      float b[3]; std::memcpy(b, ptr(target + 0x90), 12);
      if (word(recovered_object_area_004cb120(ptr(target), nullptr)) == area) {
        const float d[3] = {b[0] - a[0], b[1] - a[1], 0.0f};   // FLD/FSUB/FSTP m32 (rounded to float), z = 0
        result = static_cast<float>(vector_magnitude_004ab100(d));
      }
    }
  }
  U bits; std::memcpy(&bits, &result, 4);
  return recovered_vm_outer_push_float_005d1030(vm(), nullptr, bits) ? 0 : kPushFailed;
}
// 005426C0 SetCommandable(int, object = OBJECT_SELF): the object defaults to the script owner (this+8)
int __fastcall recovered_vm_set_commandable_005426c0(void* self, void*, int, unsigned argc) {
  U id = rd(word(self) + 8), value = 0, object = 0;
  if (!pop_int(value)) return kPopFailed;
  if (static_cast<I>(argc) >= 2 && !pop_object(id)) return kPopFailed;
  if (lookup(id, object) && rd8(object + 8) > 4) wr(object + 0xe8, value ? 1u : 0u);
  return 0;
}
// 00543240 SetLocked(object, int): door AsDoor()+2C4 or placeable AsPlaceable()+260 (raw value)
// (lane-w3 / successors lock-actions candidate set_locked_00543240, re-verified against the original)
int __fastcall recovered_vm_set_locked_00543240(void*, void*, int, unsigned) {
  U id = 0, locked = 0, object = 0;
  if (!pop_object(id) || !pop_int(locked)) return kPopFailed;
  if (lookup(id, object)) {
    if (!object) kotor_unreachable_trap(0x005432a9, "SetLocked: object array returned success with a null object");
    if (vcall(object, 0x18)) wr(vcall(object, 0x18) + 0x2c4, locked);
    else if (vcall(object, 0x48)) wr(vcall(object, 0x48) + 0x260, locked);
  }
  return 0;
}
// 00542DC0 SetIsDestroyable(int destroyable, int raiseable = TRUE, int selectable = FALSE) on the script owner
int __fastcall recovered_vm_set_is_destroyable_00542dc0(void* self, void*, int, unsigned argc) {
  U destroyable = 0, raiseable = 1, selectable = 0, object = 0;
  if (!pop_int(destroyable)) return kPopFailed;
  if (static_cast<I>(argc) >= 2 && (!pop_int(raiseable) || !pop_int(selectable))) return kPopFailed;
  if (lookup(rd(word(self) + 8), object) && rd8(object + 8) > 4) {
    wr(object + 0xec, destroyable ? 1u : 0u);
    if (rd(object + 0xec) == 1) {
      const U c = vcall(object, 0x30);
      if (c && vcall(c, 0x94) && rd(c + 0xa88) == 0) {   // dead creature that is not the player's: start decay
        w5_creature_set_lock_orientation_to_object_004f34a0(ptr(c), nullptr, kInvalidObject, 0);
        if (rd(c + 0x9d4) == 0)
          (void)recovered_event_enqueue_004b08d0(ptr(rd(rd(server() + 4) + 0x10044)), nullptr, 0, 3000, rd(c + 0x154), rd(c + 4), 11, nullptr);   // [007441E0] = 11
      }
    }
    wr(object + 0xf0, raiseable);
    wr(object + 0xf4, selectable);
  }
  return 0;
}
// 00536CE0 GetClassByPosition (341) / GetLevelByPosition (342) / GetLevelByClass (343)
int __fastcall recovered_vm_get_class_information_00536ce0(void*, void*, int routine, unsigned) {
  U a = 0, id = 0, result = 0;
  if (!pop_int(a) || !pop_object(id)) return kPopFailed;
  const U c = creature_by_id(id);
  if (c) {
    const U stats = rd(c + 0xa74);
    if (routine == 0x155) {
      result = 0xff;
      if (static_cast<I>(a) >= 1 && static_cast<I>(a) <= static_cast<I>(rd8(stats + 0x89))) {
        (void)get_class_005a4e90(stats, (a - 1) & 0xffu);
        if (get_class_005a4e90(stats, (a - 1) & 0xffu) <= 0x20) result = get_class_005a4e90(stats, (a - 1) & 0xffu);
      }
    } else if (routine == 0x156) {
      if (static_cast<I>(a) >= 1 && static_cast<I>(a) <= static_cast<I>(rd8(stats + 0x89))) result = class_level_005a5090(stats, (a - 1) & 0xffu, 0);
    } else if (routine == 0x157) {
      for (U i = 0; i < 2; ++i) if (get_class_005a4e90(stats, i) == a) result = class_level_005a5090(stats, i, 0);
    }
  }
  return push_int(result);
}
}
