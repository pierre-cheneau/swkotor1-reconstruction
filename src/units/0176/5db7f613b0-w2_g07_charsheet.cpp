// W2 tier 4 G07 character sheet. See w2_g07_charsheet.hpp. Comments give original instruction addresses.
#include "04843df76a-w2_g07_charsheet.hpp"
#include "aa81aaf111-w2_g07_object.hpp"
#include "74be47154e-module_message.hpp"
#include <initializer_list>
namespace w2::g07 {
namespace sm = kotor::server_mainloop;
using namespace w2::msg;
namespace {
using I = std::int32_t;
I s8(Word v) { return static_cast<I>(static_cast<std::int8_t>(v & 0xffu)); }
I s16(Word v) { return static_cast<I>(static_cast<std::int16_t>(v & 0xffffu)); }
Word lo8(Word v) { return v & 0xffu; }
Word srv(Memory& m, w2::entry::Ports& p) { return m.read(p.app_007a39fc() + 8); }
// race-table column and stats byte per ability (STR DEX CON INT WIS CHA)
constexpr Word kRaceColumn[6] = {0x18, 0x19, 0x1d, 0x1a, 0x1c, 0x1b};
constexpr Word kStatsByte[6] = {0xe9, 0xeb, 0xed, 0xef, 0xf1, 0xf3};
// stack layout (a1 first): a1 kind 4 ability / 3 save, a2 0, a3 out slot, a4 0, a5 0, a6 save type, a7 0, a8 FFh,
// a9 ability index (abilities) or FFh (saves), a10 0
Word effect(Memory& m, CharSheetPorts& x, Word creature, Word a1, Word a6, Word a9, Word slot) {
    m.write(slot, 0);                                             // [esp+..] = 0 before every call
    return x.effect_bonus_004f3fe0(creature, a1, 0, slot, 0, 0, a6, 0, 0xff, a9, 0);
}
void wbyte(Memory& m, w2::entry::Ports& p, Word self, Word v, Word b = 8) { write_byte_004d62c0(m, p, self, v, b); }
void wword(Memory& m, w2::entry::Ports& p, Word self, Word v) { write_word16_004d6300(m, p, self, v, 0x10); }
void wbool(Memory& m, w2::entry::Ports& p, Word self, Word v) { write_bool_004d68e0(m, p, self, v); }
void put8(Memory& m, Word a, Word v) { m.write16(a, (m.read16(a) & 0xff00u) | (v & 0xffu)); }   // byte store
}

// ===================================================================== small stats bodies
Word ability_stat(Memory& m, CharSheetPorts& x, Word rules, Word stats, Word ability, Word s) {
    const Word creature = m.read(stats + 0x24);
    const Word row = m.read(rules + 0xb0) + (m.read16(stats + 0xdc) & 0xffffu) * 0x34;
    const I race = s8(m.read8(row + kRaceColumn[ability]));
    const I base = static_cast<I>(m.read8(stats + kStatsByte[ability]));
    const I first = race + s8(effect(m, x, creature, 4, 0, ability, s)) + base;
    if (first <= 3) return 3;                                     // 005A61EA
    const Word b2 = effect(m, x, creature, 4, 0, ability, s);
    const Word row2 = m.read(rules + 0xb0) + (m.read16(stats + 0xdc) & 0xffffu) * 0x34;
    return static_cast<Word>(s8(m.read8(row2 + kRaceColumn[ability])) + s8(b2) + static_cast<I>(m.read8(stats + kStatsByte[ability])));
}
Word total_bonus(Memory&, CharSheetPorts& x, Word stats, Word ability, Word) {
    return x.total_ability_bonus_005a58b0(stats, ability);
}
Word dex_mod_005a5330(Memory& m, CharSheetPorts& x, Word stats, Word flag) {
    Word cap = 0;                                                 // EBX
    bool limited = false;
    if (flag == 1 && x.is_debilitated_005b4880(m.read(stats + 0x24)) == 0) {
        const Word item = x.item_in_slot_005a4c20(m.read(m.read(stats + 0x24) + 0xa2c), 2);
        if (item != 0 && static_cast<I>(item_armor_class_00553cc0(m, x, item)) > 0) {
            cap = m.read8(x.item_base_item_005b4790(item) + 0xad);
            limited = true;                                       // 005A537D: movzx is never negative
        }
    }
    if (!limited && x.is_debilitated_005b4880(m.read(stats + 0x24)) == 0)
        return static_cast<Word>(s8(m.read8(stats + 0xec)));    // 005A539E
    const I mod = s8(m.read8(stats + 0xec));
    return mod < static_cast<I>(cap) ? static_cast<Word>(mod) : lo8(cap);
}
namespace {
Word save(Memory& m, CharSheetPorts& x, Word stats, Word flag, Word s, Word kind, Word slot_byte_off) {
    Word bonus = 0;
    if (flag == 0) bonus = effect(m, x, m.read(stats + 0x24), 3, kind, 0xff, s);
    return static_cast<Word>(bonus) + ((flag & 0xffffff00u) | m.read8(stats + slot_byte_off));   // arg slot, low byte replaced
}
}
Word fort_save_005ab810(Memory& m, CharSheetPorts& x, Word rules, Word stats, Word flag, Word s) {
    const Word partial = save(m, x, stats, flag, s, 1, 0x1a0);
    return lo8(base_save(m, x, rules, stats, 0)) + partial + m.read8(stats + 0xee);
}
Word will_save_005ab880(Memory& m, CharSheetPorts& x, Word rules, Word stats, Word flag, Word s) {
    const Word partial = save(m, x, stats, flag, s, 3, 0x1a1);
    return lo8(base_save(m, x, rules, stats, 1)) + partial + m.read8(stats + 0xf2);
}
Word reflex_save_005ab8f0(Memory& m, CharSheetPorts& x, Word rules, Word stats, Word flag, Word s) {
    Word bonus = 0;
    if (flag == 0) bonus = effect(m, x, m.read(stats + 0x24), 3, 2, 0xff, s);
    I dex = s8(m.read8(stats + 0xec));
    if (x.is_debilitated_005b4880(m.read(stats + 0x24)) != 0 && dex >= 0) dex = 0;   // 005AB93C: only a penalty applies
    const Word b = m.read8(stats + 0x1a2);                       // reflex uses only the low byte of the slot
    return lo8(base_save(m, x, rules, stats, 2)) + bonus + static_cast<Word>(dex) + b;
}
Word base_attack_bonus_005a60d0(Memory& m, CharSheetPorts& x, Word rules, Word stats) {
    Word slot = stats;                                            // PUSH ECX: local slot starts as `this`
    Word sum = 0;
    const Word count = m.read8(stats + 0x89);
    for (Word i = 0; i < count && i < m.read8(stats + 0x89); ++i) {
        const Word level = m.read8(stats + i * 40 + 0xa8);       // inlined GetClassLevel(i, 0)
        slot = (slot & 0xffffff00u) | level;
        const Word id = m.read8(stats + i * 40 + 0xa7);
        const Word cls = id * 0x1a0 + m.read(rules + 0xac);
        if (cls != 0) sum += lo8(x.class_attack_bonus_005bccb0(cls, slot));
    }
    const Word fixed = m.read8(stats + 0x102);
    return fixed != 0 ? fixed : sum;
}
Word use_monk_tables_005a4d00(Word, Word) {
    return 0;                                                     // 005A4D06: return_zero 0063E7F0 -> JE -> 0
}
Word can_use_skill_005af880(Memory& m, CharSheetPorts& x, Word rules, Word stats, Word skill) {
    const Word raw = lo8(skill);
    const Word s = (raw == 0x64 || raw == 0x65 || raw == 0x66) ? 1u : raw;
    if (s >= m.read8(rules + 0xab)) return 0;
    if ((m.read8(m.read(rules + 0xb4) + s * 32 + 0x1c) & 2u) != 0) {
        for (Word i = 0; i < m.read8(stats + 0x89); ++i) {
            const Word id = m.read8(stats + i * 40 + 0xa7);
            if (x.class_skill_useable_005be070(m.read(rules + 0xac) + id * 0x1a0, raw) != 0) return 1;   // original skill byte
        }
        return 0;
    }
    const I v = s8(m.read8(m.read(stats + 0x168) + s));
    if (v >= 0x7f) return 1;
    if (v <= -0x7f) return 0;
    return v > 0 ? 1u : 0u;
}
Word get_feat_005a6920(Memory& m, Word stats, Word index) {
    const Word i = index & 0xffffu;
    if (!(static_cast<I>(i) < static_cast<I>(m.read(stats + 4)))) return 0xffffu;
    return m.read16(m.read(stats) + 2 * i);
}
Word bonus_feat_005a6940(Memory& m, Word stats, Word index) {
    const Word i = index & 0xffffu;
    if (!(static_cast<I>(i) < static_cast<I>(m.read(stats + 0x1c)))) return 0xffffu;
    return m.read16(m.read(stats + 0x18) + 2 * i);
}
Word item_armor_class_00553cc0(Memory& m, CharSheetPorts& x, Word item) {
    if (m.read8(x.item_base_item_005b4790(item) + 0x18) != 1) return 0;
    return m.read8(x.item_base_item_005b4790(item) + 0x8a);
}
Word armor_class_004ed1d0(Memory& m, CharSheetPorts& x, Word rules, Word c) {
    Word ac = 10;                                                 // EBX; only the low 16 bits are returned
    for (Word i = 0; i < m.read8(m.read(c + 0xa74) + 0x89); ++i) {
        const Word id = lo8(x.get_class_005a4e90(m.read(c + 0xa74), i));
        const Word cls = id * 0x1a0 + m.read(rules + 0xac);
        const Word level = lo8(class_level_005a5090(m, m.read(c + 0xa74), i, 0));
        ac += static_cast<Word>(s8(x.class_armor_bonus_005be770(cls, level)));
    }
    const Word st = m.read(c + 0xa74);
    const Word t1 = static_cast<Word>(s8(m.read8(st + 0xfc) - m.read8(st + 0xfd) + m.read8(st + 0xf7)));
    const Word t2 = static_cast<Word>(s8(m.read8(st + 0xf8) - m.read8(st + 0xf9) + m.read8(st + 0xf6)));
    const Word t3 = static_cast<Word>(s8(m.read8(st + 0xfa) - m.read8(st + 0xfb)));
    const I diff = s8(m.read8(st + 0x100)) - s8(m.read8(st + 0x101));
    const Word capped = diff > 10 ? 10u : static_cast<Word>(diff);
    const Word natural = static_cast<Word>(s8(ac_natural_base_005a5a80(m, st) + m.read8(st + 0xfe) - m.read8(st + 0xff)));
    Word total = natural + t1 + ac + t2 + t3 + static_cast<Word>(s8(capped));
    total += static_cast<Word>(s8(dex_mod_005a5330(m, x, m.read(c + 0xa74), 1)));
    total += static_cast<Word>(s8(ac_feat_mod_005a9f50(m, x, m.read(c + 0xa74))));
    if (x.is_debilitated_005b4880(c) != 0) total -= 4;
    return total & 0xffffu;
}

// ===================================================================== base saves, AC helpers, combat information
namespace {
bool list_has(Memory& m, Word data, Word count, Word value) {          // inlined CExoArrayList<WORD> scan
    Word hits = 0;
    for (Word k = 0; static_cast<I>(k) < static_cast<I>(count); ++k) if (m.read16(data + 2 * k) == value) ++hits;
    return hits != 0;
}
}
Word base_save(Memory& m, CharSheetPorts& x, Word rules, Word stats, Word kind) {
    Word sum = 0;                                                 // byte accumulator [esp+0F]
    for (Word i = 0; i < m.read8(stats + 0x89); ++i) {
        const Word cls = m.read8(stats + i * 40 + 0xa7) * 0x1a0 + m.read(rules + 0xac);
        if (cls == 0) continue;
        // the level argument is a stack local whose upper three bytes are never initialised; the
        // CSWClass save getters read only the low byte (005BCCD0: MOV AL,[ESP+4])
        const Word level = m.read8(stats + i * 40 + 0xa8);
        const Word b = kind == 0 ? x.class_fort_bonus_005bccd0(cls, level)
                     : kind == 1 ? x.class_will_bonus_005bcd10(cls, level) : x.class_ref_bonus_005bccf0(cls, level);
        sum = lo8(sum + b);
    }
    if (list_has(m, m.read(stats), m.read(stats + 4), 0x16) || list_has(m, m.read(stats + 0x18), m.read(stats + 0x1c), 0x16)) return lo8(sum + 3);
    if (list_has(m, m.read(stats), m.read(stats + 4), 0x15) || list_has(m, m.read(stats + 0x18), m.read(stats + 0x1c), 0x15)) return lo8(sum + 2);
    return x.has_feat_005a6630(stats, 0x0d) != 0 ? lo8(sum + 1) : sum;
}
Word ac_natural_base_005a5a80(Memory& m, Word stats) {
    return m.read8(stats + 0xf5);                                 // 005A5A86: return_zero call, result unused
}
Word ac_feat_mod_005a9f50(Memory& m, CharSheetPorts& x, Word stats) {
    const Word right = x.item_in_slot_005a4c20(m.read(m.read(stats + 0x24) + 0xa2c), 0x10);
    const Word left = x.item_in_slot_005a4c20(m.read(m.read(stats + 0x24) + 0xa2c), 0x20);
    if (right == 0 || left != 0) return 0;
    const Word base = x.item_base_item_005b4790(right);
    if (base == 0) return 0;
    const Word type = m.read8(base + 8);
    if (type != 2 && type != 4) return 0;
    if (x.has_feat_005a6630(stats, 0x73) != 0) return 3;
    if (x.has_feat_005a6630(stats, 0x72) != 0) return 2;
    return x.has_feat_005a6630(stats, 0x71) != 0 ? 1u : 0u;
}
Word combat_node_ne_0054f280(Memory& m, Word a, Word b) {
    for (Word k = 0; k < 5; ++k) if (m.read8(a + k) != m.read8(b + k)) return 1;
    return 0;
}
Word combat_info_ne_005512d0(Memory& m, Word a, Word b) {
    for (Word k : {0u, 1u, 2u, 3u, 4u}) if (m.read8(a + k) != m.read8(b + k)) return 1;
    if (m.read(a + 0x0c) != m.read(b + 0x0c)) return 1;
    for (Word k : {5u, 6u, 7u, 8u, 9u, 0x0au, 0x0bu, 0x10u, 0x11u}) if (m.read8(a + k) != m.read8(b + k)) return 1;
    if (m.read(a + 0x30) != m.read(b + 0x30) || m.read(a + 0x2c) != m.read(b + 0x2c)) return 1;
    if (m.read(a + 0x18) != m.read(b + 0x18) || m.read(a + 0x24) != m.read(b + 0x24)) return 1;
    for (Word k = 0; static_cast<I>(k) < static_cast<I>(m.read(a + 0x18)); ++k)
        if (combat_node_ne_0054f280(m, m.read(m.read(a + 0x14) + 4 * k), m.read(m.read(b + 0x14) + 4 * k)) != 0) return 1;
    for (Word k = 0; static_cast<I>(k) < static_cast<I>(m.read(a + 0x24)); ++k)
        if (combat_node_ne_0054f280(m, m.read(m.read(a + 0x20) + 4 * k), m.read(m.read(b + 0x20) + 4 * k)) != 0) return 1;
    return 0;
}
// 005E03C0 CExoArrayList<ptr>::Allocate is an existing owner (menu_list_growth::allocate_005e03c0): port.
Word combat_info_assign_00552a00(Memory& m, CharSheetPorts& x, Word rules, Word d, Word s) {
    if (d == s) return d;
    for (Word k : {0u, 1u, 2u, 3u, 4u}) put8(m, d + k, m.read8(s + k));
    m.write(d + 0x0c, m.read(s + 0x0c));
    for (Word k : {5u, 6u, 7u, 8u, 9u, 0x0au, 0x0bu, 0x10u, 0x11u}) put8(m, d + k, m.read8(s + k));
    m.write(d + 0x30, m.read(s + 0x30)); m.write(d + 0x2c, m.read(s + 0x2c));
    // the old lists' storage is freed but their nodes are not (original leak)
    if (m.read(d + 0x14) != 0) { x.free_006fa390(m.read(d + 0x14)); m.write(d + 0x14, 0); m.write(d + 0x1c, 0); }
    m.write(d + 0x18, 0);
    if (m.read(d + 0x20) != 0) { x.free_006fa390(m.read(d + 0x20)); m.write(d + 0x20, 0); m.write(d + 0x28, 0); }
    m.write(d + 0x24, 0);
    auto clone = [&](Word from, Word fault_pc) {
        const Word n = x.allocate_006fa7e6(5);
        if (n == 0) x.trap(fault_pc, "W2: CCombatInformation::operator= node allocation failed (original writes through null)");
        for (Word k = 0; k < 4; ++k) put8(m, n + k, 0);
        put8(m, n + 4, m.read8(rules + 0xaa));
        if (from == 0) x.trap(fault_pc, "W2: CCombatInformation::operator= copies a null node");
        if (n != from) for (Word k = 0; k < 5; ++k) put8(m, n + k, m.read8(from + k));
        return n;
    };
    for (Word i = 0; static_cast<I>(i) < static_cast<I>(m.read(s + 0x18)); ++i) {
        const Word n = clone(m.read(m.read(s + 0x14) + 4 * i), 0x00552b00);
        if (m.read(d + 0x18) == m.read(d + 0x1c)) x.list_allocate_005e03c0(d + 0x14, m.read(d + 0x1c) != 0 ? m.read(d + 0x1c) * 2 : 0x10);
        m.write(m.read(d + 0x14) + 4 * m.read(d + 0x18), n); m.write(d + 0x18, m.read(d + 0x18) + 1);
    }
    for (Word i = 0; static_cast<I>(i) < static_cast<I>(m.read(s + 0x24)); ++i) {
        const Word n = clone(m.read(m.read(s + 0x20) + 4 * i), 0x00552bac);
        if (m.read(d + 0x24) == m.read(d + 0x28)) {                // 00552BCE: inlined growth
            const Word cap = m.read(d + 0x28) != 0 ? m.read(d + 0x28) * 2 : 0x10;
            const Word old = m.read(d + 0x20);
            m.write(d + 0x28, cap);
            m.write(d + 0x20, x.allocate_006fa7e6(cap * 4));
            for (Word k = 0; static_cast<I>(k) < static_cast<I>(m.read(d + 0x24)); ++k) m.write(m.read(d + 0x20) + 4 * k, m.read(old + 4 * k));
            if (old != 0) x.free_006fa390(old);
        }
        m.write(m.read(d + 0x20) + 4 * m.read(d + 0x24), n); m.write(d + 0x24, m.read(d + 0x24) + 1);
    }
    return d;
}

// ===================================================================== 005AE680
namespace {
void push_word(Memory& m, CharSheetPorts& x, Word list, Word value) {     // inlined CExoArrayList<WORD>::Add
    if (m.read(list + 4) == m.read(list + 8)) {
        const Word cap = m.read(list + 8) != 0 ? m.read(list + 8) * 2 : 0x10;
        const Word old = m.read(list);
        m.write(list + 8, cap);
        m.write(list, x.allocate_006fa7e6(cap * 2));
        for (Word k = 0; static_cast<I>(k) < static_cast<I>(m.read(list + 4)); ++k) m.write16(m.read(list) + 2 * k, m.read16(old + 2 * k));
        if (old != 0) x.free_006fa390(old);
    }
    m.write16(m.read(list) + 2 * m.read(list + 4), value);
    m.write(list + 4, m.read(list + 4) + 1);
}
}
void update_last_stats_005ae680(Memory& m, CharSheetPorts& x, Word rules, Word st, Word lo, Word hi, Word S, Word s) {
    (void)hi;                                                     // 64-bit flag word; every tested bit is in the low dword
    const Word slot3 = s, local = s + 4;                          // slot3: the snapshot argument slot (reused as a local)
    m.write(slot3, S);
    if ((lo & 1u) != 0) {
        for (Word a = 0; a < 6; ++a) put8(m, S + 0x2c + a, ability_stat(m, x, rules, st, a, local));
        const Word bytes[6] = {0xe9, 0xeb, 0xed, 0xef, 0xf1, 0xf3};
        for (Word k = 0; k < 6; ++k) put8(m, S + 0x32 + k, m.read8(st + bytes[k]));
        for (Word a = 0; a < 6; ++a) put8(m, S + 0x38 + a, effect(m, x, m.read(st + 0x24), 4, 0, a, slot3));
        I dex = s8(m.read8(st + 0xec));
        if (x.is_debilitated_005b4880(m.read(st + 0x24)) != 0 && dex >= 0) dex = 0;   // 005AE7F8
        put8(m, S + 0x3e, dex_mod_005a5330(m, x, st, 1) - static_cast<Word>(dex));
    }
    if ((lo & 2u) != 0) {
        const Word b = effect(m, x, m.read(st + 0x24), 3, 1, 0xff, local);
        const Word e = m.read8(st + 0xee), f = m.read8(st + 0x1a0);
        put8(m, slot3, f);                                        // 005AE85D: low byte of the argument slot
        put8(m, S + 0x3f, base_save(m, x, rules, st, 0) + b + f + e);
    }
    if ((lo & 4u) != 0) {
        const Word b = effect(m, x, m.read(st + 0x24), 3, 3, 0xff, local);
        const Word e = m.read8(st + 0xf2), f = m.read8(st + 0x1a1);
        put8(m, slot3, f);
        put8(m, S + 0x40, base_save(m, x, rules, st, 1) + b + f + e);
    }
    if ((lo & 8u) != 0) put8(m, S + 0x41, reflex_save_005ab8f0(m, x, rules, st, 0, local));
    if ((lo & 0x10u) != 0) m.write(S + 0x48, m.read(st + 0x68));
    if ((lo & 0x20u) != 0) { put8(m, S + 0x50, base_attack_bonus_005a60d0(m, x, rules, st)); m.write(S + 0x4c, 0); }   // return_zero -> 0
    if ((lo & 0x40u) != 0) combat_info_assign_00552a00(m, x, rules, m.read(S + 0x54), m.read(st + 0x118));   // 005CAEA0
    if ((lo & 0x400u) != 0) m.write16(S + 0x52, armor_class_004ed1d0(m, x, rules, m.read(st + 0x24)));
    if ((lo & 0x80u) != 0) {
        for (Word i = 0; i < m.read8(rules + 0xab); i = lo8(i + 1)) {
            put8(m, slot3, i);
            put8(m, m.read(S + 0x58) + i, x.skill_rank_005aa570(st, m.read(slot3), 0, 0));
            if (lo8(i + 1) == 0) break;
        }
    }
    if ((lo & 0x200u) != 0) {
        m.write(S + 0x18, m.read(m.read(st + 0x24) + 4));
        m.write(S + 4, 0);
        for (Word i = 0; (i & 0xffffu) < (m.read(st + 4) & 0xffffu); ++i) push_word(m, x, S, get_feat_005a6920(m, st, i));
        m.write(S + 0x10, 0);
        for (Word i = 0; (i & 0xffffu) < (m.read(st + 0x1c) & 0xffffu); ++i) push_word(m, x, S + 0x0c, bonus_feat_005a6940(m, st, i));
    }
}

// ===================================================================== 00561260
Word charsheet_required_00561260(Memory& m, w2::entry::Ports& p, CharSheetPorts& x, Word gui, Word player, Word s) {
    Word A = 0, B = 0;
    const Word c = p.creature_by_id_004ae770(srv(m, p), m.read(gui));
    if (c == 0) return 0;
    const Word o = p.game_object_by_id_004ae750(srv(m, p), m.read(player + 0x28));
    if (o != 0 && m.read8(o + 8) > 4) {                           // [00746633] = 4
        const Word o2 = p.game_object_by_id_004ae750(srv(m, p), m.read(player + 0x28));
        if (o2 == 0 || m.read8(o2 + 8) <= 4) x.trap(0x005612e1, "W2: original reads the id of a null game object (second lookup)");
        if (m.read(o2 + 4) != m.read(gui)) B = 1;
    }
    const I page = s8(m.read8(gui + 4));
    const Word st = m.read(c + 0xa74), S = m.read(gui + 8), rules = p.rules_007a3a28();
    switch (page) {
    case -1: return 0;
    case 0: {
        Word f = (lo8(ability_stat(m, x, rules, st, 0, s)) != m.read8(S + 0x2c)) ? 1u : 0u;
        f |= lo8(ability_stat(m, x, rules, st, 5, s)) != m.read8(S + 0x31) ? 1u : 0u;
        f |= lo8(ability_stat(m, x, rules, st, 4, s)) != m.read8(S + 0x30) ? 1u : 0u;
        f |= lo8(ability_stat(m, x, rules, st, 3, s)) != m.read8(S + 0x2f) ? 1u : 0u;
        f |= lo8(ability_stat(m, x, rules, st, 2, s)) != m.read8(S + 0x2e) ? 1u : 0u;
        f |= lo8(ability_stat(m, x, rules, st, 1, s)) != m.read8(S + 0x2d) ? 1u : 0u;
        const Word base_bytes[6] = {0xf3, 0xf1, 0xef, 0xed, 0xeb, 0xe9};
        const Word snap[6] = {0x37, 0x36, 0x35, 0x34, 0x33, 0x32};
        for (int k = 0; k < 6; ++k) f |= m.read8(st + base_bytes[k]) != m.read8(S + snap[k]) ? 1u : 0u;
        A = f;
        const I d = s8(dex_mod_005a5330(m, x, st, 1)) - s8(dex_mod_005a5330(m, x, st, 0));
        if (d != s8(m.read8(S + 0x3e))) A |= 1;
        Word bonus = lo8(total_bonus(m, x, st, 5, s)) != m.read8(S + 0x3d) ? 1u : 0u;
        bonus |= lo8(total_bonus(m, x, st, 4, s)) != m.read8(S + 0x3c) ? 1u : 0u;
        bonus |= lo8(total_bonus(m, x, st, 3, s)) != m.read8(S + 0x3b) ? 1u : 0u;
        bonus |= lo8(total_bonus(m, x, st, 2, s)) != m.read8(S + 0x3a) ? 1u : 0u;
        bonus |= lo8(total_bonus(m, x, st, 1, s)) != m.read8(S + 0x39) ? 1u : 0u;
        bonus |= lo8(total_bonus(m, x, st, 0, s)) != m.read8(S + 0x38) ? 1u : 0u;
        const Word xp = m.read(st + 0x68);
        A |= bonus;
        Word saves = lo8(reflex_save_005ab8f0(m, x, rules, st, 0, s)) != m.read8(S + 0x41) ? 8u : 0u;
        saves |= lo8(will_save_005ab880(m, x, rules, st, 0, s)) != m.read8(S + 0x40) ? 4u : 0u;
        saves |= lo8(fort_save_005ab810(m, x, rules, st, 0, s)) != m.read8(S + 0x3f) ? 2u : 0u;
        saves |= xp != m.read(S + 0x48) ? 0x10u : 0u;
        A |= saves;
        if (base_attack_bonus_005a60d0(m, x, rules, st) != m.read8(S + 0x50) || use_monk_tables_005a4d00(st, 0) != m.read(S + 0x4c)) A |= 0x20;
        if (combat_info_ne_005512d0(m, m.read(st + 0x118), m.read(S + 0x54)) != 0) A |= 0x40;
        if (B == 1 && armor_class_004ed1d0(m, x, rules, c) != m.read16(S + 0x52)) return A | 0x400;   // AC only when B (original)
        return A;
    }
    case 1: {
        m.write(S + 0x1c, 0);
        Word slot = player;                                        // [esp+2C] = player argument, low byte reused as index
        for (Word i = 0; i < m.read8(p.rules_007a3a28() + 0xab); i = lo8(i + 1)) {
            slot = (slot & 0xffffff00u) | i;
            const Word old = m.read8(m.read(S + 0x58) + i);
            if (lo8(x.skill_rank_005aa570(st, slot, 0, 0)) != old) m.write(S + 0x1c, m.read(S + 0x1c) | (1u << (i & 31u)));
            if (lo8(i + 1) == 0) break;                            // byte counter (defensive: count is a byte, cannot exceed 255)
        }
        return m.read(S + 0x1c) != 0 ? 0x80u : A;
    }
    case 2: {
        B = m.read(c + 4) != m.read(S + 0x18) ? 1u : 0u;
        const Word count = m.read(st + 4);
        if ((count & 0xffffu) != m.read(S + 4)) B = 1;
        bool final_check = false;
        if ((m.read16(st + 0x1c) & 0xffffu) != m.read(S + 0x10)) final_check = true;     // 005616FF -> 005617B8
        else if (B != 0) final_check = B == 1;
        else {
            if ((count & 0xffffu) != 0) {
                Word i = 0;
                do {
                    const Word f = get_feat_005a6920(m, st, i);
                    bool found = false;
                    if (static_cast<I>(m.read(S + 4)) > 0)
                        for (Word k = 0; k < m.read(S + 4); ++k) if (m.read16(m.read(S) + 2 * k) == (f & 0xffffu)) found = true;
                    if (!found) B = 1;
                    ++i;
                } while ((i & 0xffffu) < (m.read16(m.read(c + 0xa74) + 4) & 0xffffu));
            }
            if (B != 0) final_check = B == 1;
            else {
                const Word st2 = m.read(c + 0xa74);
                if ((m.read16(st2 + 0x1c) & 0xffffu) == 0) return A;
                Word i = 0;
                do {
                    const Word f = bonus_feat_005a6940(m, m.read(c + 0xa74), i);
                    bool found = false;
                    if (static_cast<I>(m.read(S + 0x10)) > 0)
                        for (Word k = 0; k < m.read(S + 0x10); ++k) if (m.read16(m.read(S + 0x0c) + 2 * k) == (f & 0xffffu)) found = true;
                    if (!found) B = 1;
                    ++i;
                } while ((i & 0xffffu) < (m.read16(m.read(c + 0xa74) + 0x1c) & 0xffffu));
                final_check = B == 1;
            }
        }
        if (!final_check) return A;
        const Word o3 = p.game_object_by_id_004ae750(srv(m, p), m.read(player + 0x28));
        if (o3 != 0 && m.read8(o3 + 8) > 4 && m.read(p.game_object_00561080(player) + 4) != m.read(c + 4)) A = 0x200;
        return A;
    }
    default: return A;
    }
}

// ===================================================================== 0056F490
void write_charsheet_0056f490(Memory& m, w2::entry::Ports& p, CharSheetPorts& x, Word self, Word player, Word flags, Word s) {
    const Word gui = m.read(player + 0x50);
    const Word S = m.read(gui + 8);
    const Word c = p.creature_by_id_004ae770(srv(m, p), m.read(gui));
    if (c == 0) x.trap(0x0056f4ba, "W2: original reads the stats of a null creature (WriteGameObjUpdate_CharacterSheet)");
    const Word st = m.read(c + 0xa74), rules = p.rules_007a3a28();
    wbyte(m, p, self, 0x47); wbyte(m, p, self, 0x53);
    write_object_id_00578570(m, p, self, m.read(c + 4));
    sm::message_word_004d6340(m, p, self, flags, 0x20);
    if ((flags & 1u) != 0) {
        const Word order[6] = {0, 1, 2, 3, 4, 5};
        for (Word a : order) wbyte(m, p, self, ability_stat(m, x, rules, st, a, s));
        for (Word a : order) wbyte(m, p, self, total_bonus(m, x, st, a, s));
        for (Word off : {0xe9u, 0xebu, 0xedu, 0xefu, 0xf1u, 0xf3u}) wbyte(m, p, self, m.read8(st + off));
        const Word mod1 = lo8(dex_mod_005a5330(m, x, st, 1));
        wbyte(m, p, self, lo8(mod1 - lo8(dex_mod_005a5330(m, x, st, 0))));
    }
    if ((flags & 2u) != 0) wbyte(m, p, self, fort_save_005ab810(m, x, rules, st, 0, s));
    if ((flags & 4u) != 0) wbyte(m, p, self, will_save_005ab880(m, x, rules, st, 0, s));
    if ((flags & 8u) != 0) wbyte(m, p, self, reflex_save_005ab8f0(m, x, rules, st, 0, s));
    if ((flags & 0x10u) != 0) sm::message_word_004d6340(m, p, self, m.read(st + 0x68), 0x20);
    if ((flags & 0x20u) != 0) { wbyte(m, p, self, base_attack_bonus_005a60d0(m, x, rules, st)); wbool(m, p, self, use_monk_tables_005a4d00(st, 0)); }
    Word slot = player;                                           // [esp+1C]: the player argument slot
    if ((flags & 0x40u) != 0) {
        const Word ci = m.read(st + 0x118);
        wbyte(m, p, self, m.read8(ci), 3); wbyte(m, p, self, m.read8(ci + 1)); wbyte(m, p, self, m.read8(ci + 2));
        wbyte(m, p, self, m.read8(ci + 5)); wbyte(m, p, self, m.read8(ci + 6), 7); wbyte(m, p, self, m.read8(ci + 7), 5);
        wbyte(m, p, self, m.read8(ci + 8), 5); wbyte(m, p, self, m.read8(ci + 9), 5); wbyte(m, p, self, m.read8(ci + 0x0a), 4);
        wbyte(m, p, self, m.read8(ci + 0x0b), 3);
        wbool(m, p, self, m.read(ci + 0x0c));
        if (m.read(ci + 0x0c) == 1) {
            wbyte(m, p, self, m.read8(ci + 3)); wbyte(m, p, self, m.read8(ci + 4));
            wbyte(m, p, self, m.read8(ci + 0x10), 4); wbyte(m, p, self, m.read8(ci + 0x11), 3);
        }
        auto list = [&](Word head, bool second) {             // CExoArrayList at ci+14h / ci+20h (pointer never null)
            wbyte(m, p, self, m.read8(head + 4));
            if (!second) slot = 0;                                // 0056F7EB: index lives in the player slot
            for (Word i = 0; static_cast<I>(i) < static_cast<I>(m.read(head + 4)); ++i) {
                if (!second) slot = i;
                const Word e = m.read(m.read(head) + 4 * i);
                wbyte(m, p, self, m.read8(e));
                if (second) wbyte(m, p, self, m.read8(m.read(m.read(head) + 4 * i) + 1), 4);
                wbyte(m, p, self, m.read8(m.read(m.read(head) + 4 * i) + 2), 3);
                if (m.read8(m.read(m.read(head) + 4 * i) + 4) != m.read8(rules + 0xaa)) {
                    wbool(m, p, self, 1); wbyte(m, p, self, m.read8(m.read(m.read(head) + 4 * i) + 4));
                } else wbool(m, p, self, 0);
                if (m.read8(m.read(m.read(head) + 4 * i) + 3) != 0) {
                    wbool(m, p, self, 1); wbyte(m, p, self, m.read8(m.read(m.read(head) + 4 * i) + 3), 3);
                } else wbool(m, p, self, 0);
                if (!second) slot = i + 1;
            }
        };
        list(ci + 0x14, false);
        list(ci + 0x20, true);
    }
    if ((flags & 0x400u) != 0) wword(m, p, self, armor_class_004ed1d0(m, x, rules, c));
    if ((flags & 0x80u) != 0) {
        sm::message_word_004d6340(m, p, self, m.read(S + 0x1c), m.read8(p.rules_007a3a28() + 0xab));
        for (Word i = 0; i < m.read8(p.rules_007a3a28() + 0xab); i = lo8(i + 1)) {
            slot = (slot & 0xffffff00u) | i;
            if ((m.read(S + 0x1c) & (1u << (i & 31u))) != 0) {
                wbyte(m, p, self, x.skill_rank_005aa570(st, slot, 0, 0));
                wbool(m, p, self, can_use_skill_005af880(m, x, rules, st, slot));
            }
            if (lo8(i + 1) == 0) break;
        }
    }
    if ((flags & 0x200u) != 0) {
        wword(m, p, self, m.read16(m.read(c + 0xa74) + 4));
        for (Word i = 0; (i & 0xffffu) < (m.read16(m.read(c + 0xa74) + 4) & 0xffffu); ++i) wword(m, p, self, get_feat_005a6920(m, m.read(c + 0xa74), i));
        wword(m, p, self, m.read16(m.read(c + 0xa74) + 0x1c));
        for (Word i = 0; (i & 0xffffu) < (m.read16(m.read(c + 0xa74) + 0x1c) & 0xffffu); ++i) wword(m, p, self, bonus_feat_005a6940(m, m.read(c + 0xa74), i));
    }
    update_last_stats_005ae680(m, x, rules, m.read(c + 0xa74), flags, 0, S, s);
}
}
