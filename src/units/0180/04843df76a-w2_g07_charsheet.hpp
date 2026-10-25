#pragma once
// W2 tier 4 (G07 character sheet): CSWSPlayerCharSheetGUI::ComputeCharacterSheetUpdateRequired 00561260,
// CSWSMessage::WriteGameObjUpdate_CharacterSheet 0056F490 ('G','S' record) and their small unowned
// CSWSCreatureStats / CSWSCreature / CSWSItem callees. Logic over original 32-bit address words.
//
// CSWSPlayerCharSheetGUI (player+50h points at it): +00 creature id, +04 signed byte page
// (-1 none, 0 stats, 1 skills, 2 feats), +08 S = last-sent stats snapshot. S fields read here:
//   +00/+04 feat list (WORD*, count)      +0C/+10 bonus-feat list (WORD*, count)   +18 creature id
//   +1C skill change mask (written by 00561260 page 1)
//   +2C..+31 STR DEX CON INT WIS CHA stat  +32..+37 stats bytes E9/EB/ED/EF/F1/F3
//   +38..+3D STR DEX CON INT WIS CHA total bonus  +3E DEX-mod difference
//   +3F fort  +40 will  +41 reflex  +48 XP dword  +4C monk-table dword  +50 BAB byte  +52 AC word
//   +54 CCombatInformation (compared by 005512D0)  +58 skill-rank byte array pointer
#include "72239a78a7-w2_entry.hpp"
namespace w2::g07 {
using w2::entry::Word;
using w2::entry::Memory;

struct CharSheetPorts {
    virtual ~CharSheetPorts() = default;
    [[noreturn]] virtual void trap(Word address, const char* what) = 0;
    // CSWSCreature::GetTotalEffectBonus 004F3FE0 (thiscall, 10 stack args, RET 28h); EAX returned raw
    virtual Word effect_bonus_004f3fe0(Word creature, Word a1, Word a2, Word a3, Word a4, Word a5, Word a6, Word a7, Word a8, Word a9, Word a10) = 0;
    // CSWSCreatureStats::GetTotal{STR,DEX,CON,INT,WIS,CHA}Bonus 005A58B0 + 0x30 * ability (thiscall, RET 0); AL returned
    virtual Word total_ability_bonus_005a58b0(Word stats, Word ability) = 0;
    virtual Word is_debilitated_005b4880(Word creature) = 0;
    virtual Word item_in_slot_005a4c20(Word inventory, Word slot) = 0;
    virtual Word item_base_item_005b4790(Word item) = 0;
    virtual Word class_fort_bonus_005bccd0(Word cls, Word level) = 0;     // AL; @kotor_m15b_class_get_fort_save_bonus_005bccd0@12
    virtual Word class_will_bonus_005bcd10(Word cls, Word level) = 0;     // AL; @kotor_m15b_class_get_will_save_bonus_005bcd10@12
    virtual Word class_ref_bonus_005bccf0(Word cls, Word level) = 0;      // AL; @kotor_m15b_class_get_ref_save_bonus_005bccf0@12
    virtual Word has_feat_005a6630(Word stats, Word feat) = 0;            // @recovered_stats_has_feat_005a6630@12
    virtual Word class_attack_bonus_005bccb0(Word cls, Word level_slot) = 0;   // AL
    virtual Word class_skill_useable_005be070(Word cls, Word skill) = 0;
    virtual Word class_armor_bonus_005be770(Word cls, Word level) = 0;    // AL
    virtual Word skill_rank_005aa570(Word stats, Word skill, Word a, Word b) = 0;   // AL
    virtual Word get_class_005a4e90(Word stats, Word index) = 0;          // AL
    virtual Word allocate_006fa7e6(Word bytes) = 0;                       // live_operator_new
    virtual void free_006fa390(Word block) = 0;
    virtual void list_allocate_005e03c0(Word list, Word capacity) = 0;   // menu_list_growth::allocate_005e03c0 (existing owner)                           // live_free
};

// small CSWSCreatureStats / CSWSItem / CSWSCreature bodies (unowned)
// ability: 0 STR, 1 DEX, 2 CON, 3 INT, 4 WIS, 5 CHA. GetSTRStat 005A6190, GetDEXStat 005A6550, GetCONStat 005A6250,
// GetINTStat 005A6310, GetWISStat 005A63D0, GetCHAStat 005A6490 (RET 0). scratch >= 4 (effect-bonus out slot).
Word ability_stat(Memory&, CharSheetPorts&, Word rules, Word stats, Word ability, Word scratch);
// GetTotal{STR,DEX,CON,INT,WIS,CHA}Bonus 005A58B0 / 58E0 / 5910 / 5940 / 5970 / 59A0 (RET 0)
Word total_bonus(Memory&, CharSheetPorts&, Word stats, Word ability, Word scratch);
Word dex_mod_005a5330(Memory&, CharSheetPorts&, Word stats, Word flag);                 // RET 4
Word fort_save_005ab810(Memory&, CharSheetPorts&, Word rules, Word stats, Word flag, Word scratch);  // RET 4
Word will_save_005ab880(Memory&, CharSheetPorts&, Word rules, Word stats, Word flag, Word scratch);  // RET 4
Word reflex_save_005ab8f0(Memory&, CharSheetPorts&, Word rules, Word stats, Word flag, Word scratch);// RET 4
Word base_attack_bonus_005a60d0(Memory&, CharSheetPorts&, Word rules, Word stats);      // RET 0
Word use_monk_tables_005a4d00(Word stats, Word flag);                                    // RET 4
Word can_use_skill_005af880(Memory&, CharSheetPorts&, Word rules, Word stats, Word skill); // RET 4
Word get_feat_005a6920(Memory&, Word stats, Word index);                                 // RET 4 (AX)
Word bonus_feat_005a6940(Memory&, Word stats, Word index);                               // RET 4 (AX)
Word item_armor_class_00553cc0(Memory&, CharSheetPorts&, Word item);                     // RET 0
Word armor_class_004ed1d0(Memory&, CharSheetPorts&, Word rules, Word creature);          // RET 0 (AX)
// GetBase{Fort,Will,Reflex}SavingThrow 005AA1B0 / 005AA2F0 / 005AA430 (RET 0, AL). kind: 0 fort, 1 will, 2 reflex
Word base_save(Memory&, CharSheetPorts&, Word rules, Word stats, Word kind);
Word ac_natural_base_005a5a80(Memory&, Word stats);                                      // RET 0 (AL)
Word ac_feat_mod_005a9f50(Memory&, CharSheetPorts&, Word stats);                         // RET 0 (AL)
Word combat_node_ne_0054f280(Memory&, Word a, Word b);                                   // RET 4
Word combat_info_ne_005512d0(Memory&, Word info, Word other);                            // RET 4
// 005CAEA0 CSWCreatureStatsUpdate::SetCombatInformation -> 00552A00 CCombatInformation::operator= (RET 4)
Word combat_info_assign_00552a00(Memory&, CharSheetPorts&, Word rules, Word dst, Word src);
// 005AE680 UpdateLastStatsObject(flags64 lo, hi, snapshot) RET 0Ch. scratch >= 8
void update_last_stats_005ae680(Memory&, CharSheetPorts&, Word rules, Word stats, Word flags_lo, Word flags_hi, Word snapshot, Word scratch);

// 00561260 ComputeCharacterSheetUpdateRequired(player) RET 4; this = CharSheetGUI. scratch >= 4
Word charsheet_required_00561260(Memory&, w2::entry::Ports&, CharSheetPorts&, Word gui, Word player, Word scratch);
// 0056F490 WriteGameObjUpdate_CharacterSheet(player, flags) RET 8. scratch >= 4
void write_charsheet_0056f490(Memory&, w2::entry::Ports&, CharSheetPorts&, Word self, Word player, Word flags, Word scratch);
}
