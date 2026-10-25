// Original 004D4620 CSWSCombatRound::EndCombatRound plus the five combat-round
// callees that were still missing from the selected W1 provider set:
//   005B8010 CSWSCreature::ClearAttackerList
//   005B6980 CSWSCreature::CreateNewAttackActions
//   004FFAD0 CSWSCreature::MaxCleaveRange
//   005B48B0 CSWSCreature::CancelAllCombatModes
//   004D3E50 CSWSCombatRound::UpdateAttackTargetForAllAction
//
// Every memory access goes through the W1 Ports interface and every original
// direct call goes through its original address; the native binding maps those
// addresses to the selected real providers.  The bodies below reproduce the
// original control flow, including the cold flag != 0 branches and the original
// faults (no null guards, no fabricated state, no result constants).
//
// Also hosts the three authentic setup operations required by the reviewed
// case-47 plan (real faction defaults 0052B490, real combat-round constructor
// 004D5CB0, real client ObjectArray constructor 004D7D80).  The setup helpers
// are adapters around already-selected portable providers; they add no
// reconstructed original body.
#include "bd291ef956-w1_ports.hpp"
#include "2cb5482018-w1_x87.hpp"
#include "b4f8afa3dd-perception.hpp"
#include "54ec7b408a-module_factions.hpp"
#include "ea9b5e6c7f-faction_reputation.hpp"
#include "b0b49a4ad4-combat_constructor.hpp"
#include <cstdint>
#include <cstring>

namespace kotor_reconstruction {
struct StartupString;
struct ArrayMutations;
void __cdecl StartupFormat005e5680(StartupString*, const char*, ...);
}

namespace w1 {
namespace combat_round {

using w1::Ports;
using w1::U;
using w1::S;
using w1::x87::X80;

namespace {

inline U word(void* p) { return static_cast<U>(reinterpret_cast<std::uintptr_t>(p)); }
inline void* ptr(U a) { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(a)); }
inline U bits(float f) { U u; std::memcpy(&u, &f, 4); return u; }

// CServerExoApp accessor (same inline original accessor spelling as w1_ports).
inline U server_of(Ports& p) { return p.read(p.global(kAppManager) + 8); }

// fld tbyte [range]; fadd dword 2.0f; fstp tbyte [out]
// Exactly the original 004FFAD0 tail: the extended argument is kept until the
// caller consumes it from ST0.
inline X80 add_two_float(const X80& range) {
    X80 out;
    const float two = 2.0f;
    // MS inline asm `mov eax, range.b` loads the array's first dword, not its
    // address; take the addresses in C++ first (same pattern as w1_x87.cpp).
    const unsigned char* in_ptr = range.b;
    unsigned char* out_ptr = out.b;
    __asm {
        mov eax, in_ptr
        fld tbyte ptr [eax]
        fadd two
        mov eax, out_ptr
        fstp tbyte ptr [eax]
    }
    return out;
}

// 004FFAD0: call 004FB0F0 then fadd [00741BE0] (=2.0f), result in ST0.
X80 max_cleave_range_core(U self, U a1, U a2, U a3, Ports& p) {
    alignas(8) unsigned char scratch[0x40] = {};
    X80 range = w1::perception::max_attack_range_004fb0f0(self, a1, a2, a3, word(scratch), p);
    return add_two_float(range);
}

// Original 005B6A8D..005B6AB6: after the MaxCleaveRange call, form
// ((z*z + y*y) + x*x) and compare r*r against it with fcompp; `jp` keeps the
// cleave result.  The exact instruction sequence is preserved.
bool cleave_range_kept(const X80& range, const float d[3]) {
    int keep = 1;
    // `d` is a pointer parameter: `[d + 8]` in MS inline asm addresses the
    // parameter slot, not d[2].  Load both addresses into registers first.
    const unsigned char* range_ptr = range.b;
    const float* d_ptr = d;
    __asm {
        mov eax, range_ptr
        mov ecx, d_ptr
        fld tbyte ptr [eax]
        fld dword ptr [ecx + 8]
        fmul dword ptr [ecx + 8]
        fld dword ptr [ecx + 4]
        fmul dword ptr [ecx + 4]
        faddp st(1), st(0)
        fld dword ptr [ecx]
        fmul dword ptr [ecx]
        faddp st(1), st(0)
        fld st(1)
        fmul st(2)
        fcompp
        fnstsw ax
        fstp st(0)
        test ah, 5
        jp done
        mov keep, 0
    done:
    }
    return keep != 0;
}

} // namespace

// 004FFAD0 (this=ECX, 3 stack arguments, RET 0xC).  Public so the dispatcher
// can route the original address and so the caller in 005B6980 can load the
// exact ten-byte extended result.
X80 max_cleave_range_004ffad0(U self, U a1, U a2, U a3, Ports& p) {
    return max_cleave_range_core(self, a1, a2, a3, p);
}

// 005B8010 CSWSCreature::ClearAttackerList (this=ECX, RET).
void clear_attacker_list_005b8010(U self, Ports& p) {
    U list = p.read(self + 0xa80);
    if (list) {
        U first = p.read(list);
        p.call(0x6fa390, 0, {first});
        p.write(list, 0);
        p.call(0x6fa390, 0, {list});
        p.write(self + 0xa80, 0);
        p.write(self + 0xa84, 0);
    }
}

// 005B48B0 CSWSCreature::CancelAllCombatModes (this=ECX, one stack argument,
// RET 4).  The argument is not read by the original body.
void cancel_all_combat_modes_005b48b0(U self, U, Ports& p) {
    p.call(0x4f34a0, self, {0x7f000000, 0});
    p.vcall(self, 0x7c, {0x2710});
    p.call(0x50ee80, self, {0, 1});
    p.call(0x4d4f60, p.read(self + 0x9c8), {});
}

// 004D3E50 CSWSCombatRound::UpdateAttackTargetForAllAction (this=ECX, one stack
// argument, RET 4).
void update_attack_target_for_all_action_004d3e50(U self, U target, Ports& p) {
    U wrapper = p.read(self + 0x9b0);
    if (!wrapper) return;
    U pos = p.read(p.read(wrapper));             // 004D3E5E..004D3E64: [esp+4] = head position
    if (!pos) return;
    for (;;) {
        // 004D3E75: the list pointer is re-read from [esi+0x9B0] every iteration.
        U node = p.call(0x5e9b60, p.read(p.read(self + 0x9b0)), {pos});
        if (p.read8(node + 0x10) == 1 && p.read(node + 0x18) == 1)
            p.write(node + 0x14, target);
        // 004D3E96..004D3EA8: GetNext(&pos) advances the same position cell in place.
        p.call(0x5e9b90, p.read(p.read(self + 0x9b0)), {word(&pos)});
        if (!pos) break;
    }
}

// 005B6980 CSWSCreature::CreateNewAttackActions (this=ECX, three stack
// arguments, RET 0xC).  arg1 = use current round target, arg2 = cleave flag,
// arg3 = target object id.
void create_new_attack_actions_005b6980(U self, U arg1, U arg2, U arg3, Ports& p) {
    U ebx = 0;
    U ebp;
    if (arg2) ebp = p.read(self + 0x524);   // original tests the second stack argument
    else ebp = p.read(p.read(self + 0x9c8) + 0x9cc);

    if (ebp != 0x7f000000) {
        goto target_path;
    }
    {
        U round = p.read(self + 0x9c8);
        U wrapper = p.read(round + 0x9b0);
        if (p.read(p.read(wrapper)) != 0) goto after_target;
        goto nearest_path;
    }

target_path:
    {
        U round = p.read(self + 0x9c8);
        U wrapper = p.read(round + 0x9b0);
        if (p.read(p.read(wrapper)) != 0) goto after_target;
        U obj = p.call(0x4ae750, server_of(p), {ebp});
        ebx = 1;
        if (!obj) {
            p.write(round + 0x9cc, 0x7f000000);
            ebx = 0;
            goto after_target;
        }
        U creature = p.vcall(obj, 0x30, {});
        if (!creature) goto after_target;
        creature = p.vcall(obj, 0x30, {});   // original casts twice
        if (p.vcall(creature, 0x94, {})) { ebx = 0; goto after_target; }
        if (p.call(0x4ef890, creature, {})) { ebx = 0; goto after_target; }
        if (p.vcall(self, 0x94, {})) { ebx = 0; goto after_target; }
        if (p.call(0x4ef890, self, {})) { ebx = 0; goto after_target; }
        U reputation = p.call(0x57cb80, self, {ebp, 0});
        if (static_cast<S>(reputation) > 10) { ebx = 0; goto after_target; }
        if (arg2 == 0) goto after_target;

        float d[3] = {};
        p.call(0x423bc0, 0, {word(d), creature + 0x90, self + 0x90});
        X80 range = max_cleave_range_core(self, p.read(creature + 4), 0, 1, p);
        if (!cleave_range_kept(range, d)) ebx = 0;
        goto after_target;   // both original exits (5B6AB6 jp / 5B6ABA jmp) go to 5B6AE3
    }

nearest_path:
    {
        U target = p.call(0x4ae770, server_of(p), {arg3});
        U nearest;
        if (target) {
            U self_id = p.read(self + 4);
            X80 range = max_cleave_range_core(self, self_id, 0, 1, p);
            U range_bits = x87::x80_to_float(range);
            nearest = p.call(0x4f2de0, target, {range_bits, self_id, self_id});
        } else {
            nearest = 0x7f000000;
        }
        if (nearest == 0x7f000000) {
            U self_id = p.read(self + 4);
            X80 range = max_cleave_range_core(self, self_id, 0, 1, p);
            U range_bits = x87::x80_to_float(range);
            nearest = p.call(0x4f2de0, self, {range_bits, self_id, 0x7f000000});
            if (nearest == 0x7f000000) goto cancel_all;
        }
        U obj = p.call(0x4ae750, server_of(p), {nearest});
        if (!p.vcall(obj, 0x30, {})) {
            p.call(0x5b48b0, self, {1});
            return;
        }
        p.call(0x4f34a0, self, {nearest, 0});
        p.write(self + 0x84, nearest);
        p.write(self + 0x50c, nearest);
        p.call(0x4d3e50, p.read(self + 0x9c8), {nearest});
        p.write(self + 0x4e8, 1);
        U client = p.read(p.global(kAppManager) + 4);
        U client_id = p.call(0x5eda50, client, {nearest, 1});
        p.call(0x5edd70, client, {client_id, 1});
        ebp = nearest;   // original 005B6BBC
        goto add_action;
    }

cancel_all:
    p.call(0x4f34a0, self, {0x7f000000, 0});
    p.vcall(self, 0x7c, {0x2710});
    p.call(0x50ee80, self, {0, 1});
    p.call(0x4d4f60, p.read(self + 0x9c8), {});
    return;

after_target:
    if (arg1 == 0) {
        if (ebx == 0) return;
        goto add_action;
    }
    goto nearest_path;

add_action:
    p.call(0x4d38b0, p.read(self + 0x9c8), {ebp, 0, 0, 0x5dc, 1});
    if (p.read(self + 0x510) == ebp) p.write(self + 0x510, 0x7f000000);
}

// 004D4620 CSWSCombatRound::EndCombatRound (this=ECX, one stack argument, RET 4).
U end_combat_round_004d4620(U self, U flag, Ports& p) {
    U owner = p.read(self + 0x9b4);
    p.write(self + 0x934, 0);
    p.write(self + 0x944, 0);
    p.write(self + 0x94c, 0);
    p.write(self + 0x958, 0);
    p.write(self + 0x95c, 0x7f000000);
    p.write(self + 0x960, 0);
    p.write(self + 0x9ac, 0x7f000000);
    p.write(self + 0x9a0, 0);
    p.write(self + 0x968, 0);
    if (owner && p.read(owner + 0xa9c)) {
        U stats = p.read(owner + 0xa74);
        U max_fp = p.call(0x4fd490, owner, {});
        p.write16(stats + 0x124, max_fp & 0xffff);
    }

    if (p.read(self + 0x9bc)) {
        U creature = p.call(0x4ae770, server_of(p), {p.read(self + 0x9c4)});
        if (creature) {
            U round = p.read(creature + 0x9c8);
            p.write(round + 0x958, 0);
            p.write(round + 0x95c, 0x7f000000);
            p.write(round + 0x964, 0);
            p.write(round + 0x960, 0);
            p.write(round + 0x9b8, 0);
            p.write(round + 0x9c0, 0x7f000000);
        } else {
            alignas(8) unsigned char text[8] = {};
            p.call(0x5b3190, word(text), {});
            kotor_reconstruction::StartupFormat005e5680(
                reinterpret_cast<kotor_reconstruction::StartupString*>(text),
                "CSWSCombatRound::EndCombatRound - %x Combat Slave (%x) not found!",
                p.read(owner + 4), p.read(self + 0x9c4));
            p.call(0x5e5c20, word(text), {});
        }
    }

    U valid_target = 0;
    U saved_target = 0x7f000000;
    if (p.read(self + 0x9cc) != 0x7f000000) {
        U obj = p.call(0x4ae750, server_of(p), {p.read(self + 0x9cc)});
        if (obj) {
            U creature = p.vcall(obj, 0x30, {});
            if (creature) {
                creature = p.vcall(obj, 0x30, {});   // original casts twice
                if (p.vcall(creature, 0x94, {}) || p.call(0x4ef890, creature, {})) {
                    p.write(self + 0x9cc, 0x7f000000);
                    valid_target = 1;
                    if (p.read(self + 0x938)) saved_target = p.read(owner + 0x524);
                    else saved_target = p.read(owner + 0x50c);
                }
            } else {
                bool invalid = false;
                U door = p.vcall(obj, 0x18, {});
                if (door) {
                    U door2 = p.vcall(obj, 0x18, {});   // original casts twice
                    if (p.read8(door2 + 0x2cc)) invalid = true;
                }
                if (!invalid) {
                    U placeable = p.vcall(obj, 0x48, {});
                    if (placeable) {
                        U placeable2 = p.vcall(obj, 0x48, {});   // original casts twice
                        if (p.read(placeable2 + 0x338)) invalid = true;
                    }
                }
                if (invalid) p.write(self + 0x9cc, 0x7f000000);
            }
        } else {
            p.write(self + 0x9cc, 0x7f000000);   // 004D4788 je 004D482B
        }
    }

    p.vcall(owner, 0x7c, {0x2711});
    for (S i = 0; i < 5; ++i)
        p.call(0x4d3010, self + 4 + static_cast<U>(i) * 0x14c, {});

    auto invalidate = [&](U offset) {
        U obj = p.call(0x4ae750, server_of(p), {p.read(owner + offset)});
        if (!obj) {
            p.write(owner + offset, 0x7f000000);
            return;
        }
        U as_object = p.vcall(obj, 0x10, {});
        if (as_object) {
            as_object = p.vcall(obj, 0x10, {});   // original casts twice
            if (p.vcall(as_object, 0x94, {}))
                p.write(owner + offset, 0x7f000000);
        }
    };
    invalidate(0x50c);
    invalidate(0x524);
    invalidate(0x504);
    invalidate(0x528);
    invalidate(0x520);

    p.call(0x5b8010, owner, {});

    U client = p.read(p.global(kAppManager) + 4);
    U party = p.call(0x5ed8b0, client, {});
    U player = p.call(0x635460, party, {});
    U controlled = 0;
    if (player) {
        U server_id = p.call(0x4aea30, server_of(p), {p.read(player + 4)});
        if (p.read(owner + 4) == server_id) controlled = 1;
        else p.call(0x50ee80, owner, {0, 1});
    } else {
        p.call(0x50ee80, owner, {0, 1});
    }

    U is_action = 1;
    U list = p.read(owner + 0xfc);
    U head = p.read(list);
    if (head) {
        if (flag != 0) {
            U node = p.call(0x5e9b60, list, {head});
            if (p.read(node) == 1) is_action = 0;
        }
    }

    if (flag != 0 && is_action != 0 && p.read(self + 0x93c) == 0 &&
        p.read(self + 0x940) == 0 && controlled == 0 &&
        p.vcall(owner, 0x94, {}) == 0 && p.call(0x4ef890, owner, {}) == 0) {
        p.call(0x5d0fc0, p.global(kVirtualMachine),
               {owner + 0x260, p.read(owner + 4), 1});
    }

    if (client) {
        if (p.call(0x5edc10, client, {}) == 0 && flag != 0 && controlled != 0) {
            U options = p.call(0x5ed700, client, {});
            if ((p.read(options + 0x14) & 0x800) && (p.read8(player + 0x440) & 1))
                p.call(0x5edee0, client, {1, 5});
        }
    }

    if (p.read(self + 0x940) == 0 && controlled != 0 && flag != 0) {
        p.call(0x5b6980, owner, {valid_target, p.read(self + 0x938), saved_target});
    }

    U owner_now = p.read(self + 0x9b4);
    p.write(owner_now + 0x53c, p.read(owner_now + 0x504));
    p.write(owner_now + 0x544, p.read(owner_now + 0x540));
    if (p.read(self + 0x938)) {
        p.write(owner_now + 0x54c, p.read(owner_now + 0x548));
        p.write(owner_now + 0x960, 0);
        p.write(self + 0x938, 0);
    }
    if (p.read(owner_now + 0x550) != 0)
        p.write(owner_now + 0x554, p.read(owner_now + 0x550));
    p.write(self + 0x984, 1);
    p.write(self + 0x988, 1);
    return 1;
}

} // namespace combat_round
} // namespace w1

// ---------------------------------------------------------------------------
// Authentic setup adapters.  These call the selected portable providers in
// 0896/0964/0019; the adapters only supply the original memory/port plumbing.
// ---------------------------------------------------------------------------

namespace kotor_menu_window_cells { extern void* volatile manager_007a39fc; }
namespace menu_list_growth { void __fastcall allocate_005e03c0(kotor_reconstruction::ArrayMutations*, void*, unsigned); }
namespace ini_comparison { int __fastcall CompareNC005e58c0(kotor_reconstruction::StartupString*, void*, const char*); }

namespace unready47_setup {

using Word = std::uint32_t;

inline void* ptr(Word a) { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(a)); }
inline Word word(void* p) { return static_cast<Word>(reinterpret_cast<std::uintptr_t>(p)); }

extern "C" void* __cdecl live_operator_new(unsigned);
extern "C" void __cdecl live_free(void*);
extern "C" void* __fastcall kotor_fastcall_004d2f30(void*, void*);   // CExoLinkedList<CSWSCombatRoundAction> ctor: matched (src/bridge)
extern "C" void* kotor_cell_rules_007a3a28;

extern "C" unsigned __fastcall gff_list_count_00411940(void*, void*, void*);
extern "C" unsigned __fastcall gff_list_element_00411990(void*, void*, void*, void*, unsigned);
extern "C" unsigned __fastcall gui_load_gff_dword_00411c20(void*, void*, void*, const char*, unsigned*, unsigned);
extern "C" unsigned __fastcall gui_load_gff_string_00411ec0(void*, void*, void*, void*, const char*, unsigned*, void*);
extern "C" unsigned short __fastcall recovered_gff_word_00411b40(void*, void*, void*, const char*, unsigned*, unsigned short);
extern "C" void __fastcall async_assign_string_005e5c50(void*, void*, void*);
extern "C" void __fastcall frame_LogStringCtor_005e5a90(void*, void*, const char*);
extern "C" void __fastcall frame_LogStringDtor_005e5c20(void*, void*);
extern "C" void __fastcall GuiAssign005e5140(void*, void*, const char*);
extern "C" void __fastcall recovered_effect_destroy_00503590(void*, void*);
extern "C" void __fastcall c2da_current_get_string_00413270(void*, void*, int, void*, void*);
extern "C" unsigned __fastcall c2da_current_get_int_entry_00413510(void*, void*, int, int, int*);

const char* faction_literal(Word a) {
    switch (a) {
    case 0x73d71c: return "";
    case 0x748610: return "LABEL";
    case 0x748638: return "FactionName";
    case 0x748628: return "FactionParentID";
    case 0x748618: return "FactionGlobal";
    case 0x748608: return "player";
    case 0x7485fc: return "hostile_1";
    case 0x7485f0: return "friendly_1";
    case 0x7485e4: return "hostile_2";
    case 0x7485d8: return "friendly_2";
    case 0x7485d0: return "neutral";
    case 0x7485c8: return "insane";
    default: return "";
    }
}

struct SetupMemory : kotor::server_mainloop::Memory {
    Word read(Word a) override {
        if (a == 0x7a39fc) return word(::kotor_menu_window_cells::manager_007a39fc);
        if (a == 0x7a3a28) return word(kotor_cell_rules_007a3a28);
        Word v;
        std::memcpy(&v, ptr(a), 4);
        return v;
    }
    void write(Word a, Word v) override { std::memcpy(ptr(a), &v, 4); }
    Word read16(Word a) override { unsigned short v; std::memcpy(&v, ptr(a), 2); return v; }
    void write16(Word a, Word v) override { unsigned short x = static_cast<unsigned short>(v); std::memcpy(ptr(a), &x, 2); }
    Word read8(Word a) override { return *static_cast<unsigned char*>(ptr(a)); }
};

struct SetupFactionPorts : kotor::server_mainloop::FactionPorts {
    SetupMemory& m;
    explicit SetupFactionPorts(SetupMemory& v) : m(v) {}
    void empty(Word a) override { m.write(a, 0); m.write(a + 4, 0); }
    void literal(Word a, Word b) override { frame_LogStringCtor_005e5a90(ptr(a), nullptr, faction_literal(b)); }
    void assign(Word a, Word b) override { async_assign_string_005e5c50(ptr(a), nullptr, ptr(b)); }
    void destroy(Word a) override { frame_LogStringDtor_005e5c20(ptr(a), nullptr); }
    Word count(Word a, Word b) override { return gff_list_count_00411940(ptr(a), nullptr, ptr(b)); }
    void element(Word a, Word b, Word c, Word d) override {
        (void)gff_list_element_00411990(ptr(a), nullptr, ptr(b), ptr(c), d);
    }
    Word string(Word a, Word b, Word c, Word d, Word e, Word f) override {
        return gui_load_gff_string_00411ec0(ptr(a), nullptr, ptr(b), ptr(c),
                                            faction_literal(d), static_cast<unsigned*>(ptr(e)), ptr(f));
    }
    Word dword(Word a, Word b, Word c, Word d, Word e) override {
        return gui_load_gff_dword_00411c20(ptr(a), nullptr, ptr(b),
                                           faction_literal(c), static_cast<unsigned*>(ptr(d)), e);
    }
    Word short_value(Word a, Word b, Word c, Word d, Word e) override {
        return recovered_gff_word_00411b40(ptr(a), nullptr, ptr(b), faction_literal(c),
                                           static_cast<unsigned*>(ptr(d)), static_cast<unsigned short>(e));
    }
    Word allocate(Word n) override { return word(live_operator_new(n)); }
    void grow_005e03c0(Word a, Word b) override {
        ::menu_list_growth::allocate_005e03c0(
            reinterpret_cast<kotor_reconstruction::ArrayMutations*>(ptr(a)), nullptr, b);
    }
    Word compare_005e58c0(Word a, Word b) override {
        return ::ini_comparison::CompareNC005e58c0(
            reinterpret_cast<kotor_reconstruction::StartupString*>(ptr(a)), nullptr, faction_literal(b));
    }
    void table_string_00413270(Word a, Word b, Word c, Word d) override {
        c2da_current_get_string_00413270(ptr(a), nullptr, static_cast<int>(b), ptr(c), ptr(d));
    }
};

struct SetupReputationPorts : kotor::server_mainloop::ReputationPorts {
    SetupMemory& m;
    explicit SetupReputationPorts(SetupMemory& v) : m(v) {}
    Word allocate(Word n) override { return word(live_operator_new(n)); }
    void free(Word a) override { live_free(ptr(a)); }
    Word table_int_00413510(Word a, Word b, Word c, Word d) override {
        return c2da_current_get_int_entry_00413510(ptr(a), nullptr, static_cast<int>(b),
                                                   static_cast<int>(c), static_cast<int*>(ptr(d)));
    }
    Word count_00411940(Word a, Word b) override { return gff_list_count_00411940(ptr(a), nullptr, ptr(b)); }
    void element_00411990(Word a, Word b, Word c, Word d) override {
        (void)gff_list_element_00411990(ptr(a), nullptr, ptr(b), ptr(c), d);
    }
    Word dword_00411c20(Word a, Word b, Word c, Word d, Word e) override {
        return gui_load_gff_dword_00411c20(ptr(a), nullptr, ptr(b),
                                           faction_literal(c), static_cast<unsigned*>(ptr(d)), e);
    }
};

struct SetupCombatMemory : recovered_combat_round::Ports {
    Word read(Word a) override { Word v; std::memcpy(&v, ptr(a), 4); return v; }
    void write(Word a, Word v) override { std::memcpy(ptr(a), &v, 4); }
    void byte(Word a, Word v) override { *static_cast<unsigned char*>(ptr(a)) = static_cast<unsigned char>(v); }
    void half(Word a, Word v) override { unsigned short x = static_cast<unsigned short>(v); std::memcpy(ptr(a), &x, 2); }
    void string(Word a) override { write(a, 0); write(a + 4, 0); }
    void empty(Word a) override;
    Word allocate(Word n) override { return word(live_operator_new(n)); }
    void scheduled_list_004d2f30(Word a) override { kotor_fastcall_004d2f30(ptr(a), nullptr); }
    void effect_destroy(Word a) override;
    void release(Word a) override { live_free(ptr(a)); }
};

void SetupCombatMemory::empty(Word a) { GuiAssign005e5140(ptr(a), nullptr, ""); }
void SetupCombatMemory::effect_destroy(Word a) { recovered_effect_destroy_00503590(ptr(a), nullptr); }

} // namespace unready47_setup

using namespace unready47_setup;

extern "C" int unready47_setup_factions(void* manager, unsigned extend) {
    SetupMemory m;
    SetupFactionPorts p(m);
    alignas(8) unsigned char scratch[0x40] = {};
    return static_cast<int>(kotor::server_mainloop::faction_defaults_0052b490(
        m, p, word(manager), extend, word(scratch)));
}

// Original module-load order: faction_check_0052B490 then faction_a_0052B9E0
// (CFactionManager reputation defaults). The latter fills mgr+0x20 with the
// N*N reputation matrix; without it GetReputation's npc_faction_reputation
// indexes a null table. This is the same genuine owner/provider as 0052B490.
extern "C" int unready47_setup_reputation(void* manager) {
    SetupMemory m;
    SetupReputationPorts p(m);
    alignas(8) unsigned char scratch[0x80] = {};
    return static_cast<int>(kotor::server_mainloop::reputation_defaults_0052b9e0(
        m, p, word(manager), word(scratch)));
}

extern "C" void* unready47_construct_combat_round(void* storage, void* owner) {
    SetupCombatMemory p;
    return ptr(recovered_combat_round::construct(word(storage), word(owner), p));
}

namespace kotor_menu_10_19 {
struct ObjectArray {
    ObjectArray* construct_004d7d80(int flag);
};
}

extern "C" void* unready47_construct_client_object_array(void* storage) {
    return static_cast<kotor_menu_10_19::ObjectArray*>(storage)->construct_004d7d80(1);
}


