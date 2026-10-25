// W2 tier 4 native binding + exports for the G07 object-update writers (all w2_g07_*.cpp logic files).
// Every port of every t4 logic file is implemented here once; t4 files call each other's bodies directly,
// so each original address has exactly one body. Providers: the 1122 map, W1 tier4 / W4 / W5 lane exports
// (link dependencies, see INTEGRATION.md), w2x_* 0886 wrappers, or explicit traps.
#if !defined(_M_IX86)
#error W2 native bindings require the Windows x86 route
#endif
#include "6d05e68233-w2_entry_native.hpp"
#include "aa81aaf111-w2_g07_object.hpp"
#include "d34a525424-w2_g07_compute.hpp"
#include "d5bc14c6f5-w2_g07_last.hpp"
#include "51971a08a7-w2_g07_appearance.hpp"
#include "c94ea11535-w2_g07_panels.hpp"
#include "28b3146bfb-w2_g07_party_update.hpp"
#include "5ada6db8dd-w2_g07_player.hpp"
#include "4002af1e06-w2_g07_playerlast.hpp"
#include "04843df76a-w2_g07_charsheet.hpp"
#include "6e34a1e9b9-w2_g07_effect_save.hpp"
#include "788ec0865b-w2_client_send.hpp"
#include <cstdint>
#include <cstring>
namespace kotor_reconstruction { struct StartupString; struct ArrayMutations; }
kotor_reconstruction::StartupString* __fastcall GuiConcat005e5d10(kotor_reconstruction::StartupString*, void*, kotor_reconstruction::StartupString*, const kotor_reconstruction::StartupString*);
void* __fastcall exo_array_list_construct_005a76d0(void*, void*);
namespace directory_list { struct TimeList; void __fastcall AllocateTimes004c4f50(TimeList*, void*, int); }
namespace menu_list_growth { void __fastcall allocate_005e03c0(kotor_reconstruction::ArrayMutations*, void*, unsigned); }
namespace lane_config_required { void* __fastcall GetPartyTable_004aee70(void*, void*); }
namespace kotor_rules { void* __fastcall kotor_fastcall_00550c00(void*, void*, unsigned short); }
namespace kotor::server_mainloop {
Word creature_faction_00513fc0(Memory&, Word);
Word creature_get_class_005a4e90(Memory&, Word, Word);
Word creature_force_class_005be4a0(Word);
}
extern "C" {
extern void* kotor_cell_rules_007a3a28;
void* __fastcall kotor_fastcall_00561080(void*, void*);
float __fastcall recovered_creature_spot_distance_004efc70(void*, void*);
float __fastcall recovered_creature_listen_distance_004efd20(void*, void*);
unsigned __fastcall recovered_creature_invisible_00501950(void*, void*, void*);
void* __fastcall recovered_creature_visible_element_00517a20(void*, void*, unsigned);
unsigned __fastcall exo_array_list_count_004af240(void*, void*, unsigned);
void __fastcall exo_array_list_destruct_0056b6f0(void*, void*);
void __fastcall c2da_005e9d70(void*, void*, void*);
unsigned __fastcall recovered_object_animation_stationary_004cae60(void*, void*, unsigned);
unsigned short __fastcall w1x_creature_max_fp_004fd490(void*, void*);
unsigned char __fastcall recovered_creature_detect_mode_004edae0(void*, void*);
void* __fastcall recovered_equipment_item_005a4c20(void*, void*, unsigned);
void* __fastcall kotor_m15b_base_item_array_get_base_item_005b31a0(void*, void*, int);
void* __fastcall kotor_m15b_item_get_base_item_005b4790(void*, void*);
unsigned char __fastcall recovered_stats_level_005a5fd0(void*, void*, unsigned);
unsigned __fastcall recovered_item_property_has_00555c90(void*, void*, unsigned, unsigned);
unsigned __fastcall recovered_item_property_query_005539c0(void*, void*, void*, unsigned, unsigned);
unsigned __fastcall recovered_stats_has_feat_005a6630(void*, void*, unsigned);
void __fastcall recovered_creature_compute_ai_state_004f1460(void*, void*);
int __fastcall gui_load_resref_not_equal_00406dc0(void*, void*, void*);
void* __fastcall server_get_module_004ae6b0(void*, void*);
void* __fastcall kotor_fastcall_004ae910(void*, void*, unsigned);
void __fastcall gui_load_array_add_00671c00(void*, void*, unsigned);
unsigned __fastcall exo_locstring_get_005ea130(void*, void*, unsigned, void*, unsigned);
void __fastcall GuiAssign005e5140(void*, void*, const char*);
unsigned __fastcall frame_LogIsEmpty_005e58a0(void*, void*);
void* __fastcall recovered_creature_inventory_004ef770(void*, void*, unsigned);
void* __fastcall recovered_item_lookup_004ae760(void*, void*, unsigned);
unsigned __fastcall recovered_effect_integer_00503690(void*, void*, unsigned);
void* __fastcall frame_R4_005ed690(void*, void*);
unsigned __fastcall frame_ContainsPanel_0040bd70(void*, void*, void*);
const char* __fastcall c2da_ref_00405fe0(void*, void*);
unsigned __fastcall recovered_creature_incapacitated_004ef890(void*, void*);
unsigned char __fastcall w1x_creature_skill_rank_005aa570(void*, void*, unsigned, unsigned, unsigned);
unsigned char __fastcall kotor_m15b_class_get_attack_bonus_005bccb0(void*, void*, unsigned);
unsigned __fastcall kotor_m15b_class_is_skill_useable_005be070(void*, void*, unsigned);
unsigned char __fastcall kotor_m15b_class_get_armor_bonus_005be770(void*, void*, unsigned);
unsigned char __fastcall kotor_m15b_class_get_fort_save_bonus_005bccd0(void*, void*, unsigned);
unsigned char __fastcall kotor_m15b_class_get_will_save_bonus_005bcd10(void*, void*, unsigned);
unsigned char __fastcall kotor_m15b_class_get_ref_save_bonus_005bccf0(void*, void*, unsigned);
void __fastcall kotor_m15_array_u16_allocate_004d2fa0(void*, void*, unsigned);
unsigned __cdecl aurora_vector_equal_004aa980(const void*, const void*);
void* __fastcall recovered_last_update_object_00561820(void*, void*, unsigned);   // W2 t2
unsigned __fastcall recovered_npc_object_00564700(void*, void*, unsigned, unsigned, unsigned);   // W2 t3
unsigned __fastcall w2x_creature_gold_004edd60(void*, void*);                        // W2 0886 wrapper (t3)
unsigned __fastcall w2x_creature_effect_skill_004f3fe0(void*, void*, unsigned*, unsigned);   // W2 0886 wrapper (t4b)
unsigned __fastcall w2x_creature_effect_ability_004f3fe0(void*, void*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);   // W2 0886 wrapper (t4)
unsigned __fastcall w5x_creature_is_debilitated_005b4880(void*, void*);              // W5 0886 wrapper (w5_0886_exports.inc)
// lane providers not in 1122 (link dependencies, see INTEGRATION.md)
void* __fastcall recovered_combat_round_attack_004d2b50(void*, void*, unsigned);    // W1 tier4
float __fastcall w5_creature_get_walk_rate_004f1b20(void*, void*);                   // W5
float __fastcall w5_creature_get_run_rate_004f1be0(void*, void*);                    // W5
unsigned __fastcall w5_door_get_is_linked_005890d0(void*, void*);                    // W5
void __fastcall kotor_fastcall_005e9bd0(void*, void*, void*);                         // W4 (CExoLinkedListInternal::GetPrev)
unsigned __fastcall recovered_server_faction_of_object_004b40c0(void*, void*, std::uint32_t, std::uint32_t*);   // W7 t4
unsigned __fastcall recovered_object_ai_state_reputation_0057cd90(void*, void*, unsigned);   // W1 tier6 (name/@12 confirmed t4b); AL consumed
void* __fastcall recovered_server_info_004aec90(void*, void*);
void __fastcall recovered_creature_set_last_instant_equip_004efe00(void*, void*, unsigned, unsigned);
void* __fastcall recovered_sla_construct_006475c0(void*, void*);                                       // W3 (t6b ruling)
std::int32_t __fastcall recovered_gui_in_game_get_current_equip_screen_npc_0062fea0(void*, void*);     // W3 (t6b ruling)   // W1 tier7 (ruling)                               // W1 tier6 004AEC90 GetServerInfo
// W1 tier6 reputation set (t4b ruling: W1 owns 0057CB80, 0052B280, 0052B420, 00513FF0 and cell 007A2068)
unsigned __fastcall recovered_object_get_reputation_0057cb80(void*, void*, unsigned, unsigned);
unsigned __fastcall recovered_faction_is_npc_0052b280(void*, void*, unsigned);
// CSWSCreatureStats::GetTotal{STR,DEX,CON,INT,WIS,CHA}Bonus: byte-identical matched code (src/bridge)
char __fastcall kotor_fastcall_005a58b0(void*, void*);
char __fastcall kotor_fastcall_005a58e0(void*, void*);
char __fastcall kotor_fastcall_005a5910(void*, void*);
char __fastcall kotor_fastcall_005a5940(void*, void*);
char __fastcall kotor_fastcall_005a5970(void*, void*);
char __fastcall kotor_fastcall_005a59a0(void*, void*);
unsigned __fastcall recovered_creature_total_effect_bonus_004f3fe0(void*, void*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);   // W2 single 004F3FE0 entry (t4b)
}

namespace {
using namespace w2::native;
namespace g = w2::g07;
using w2::entry::Word;
inline float f32(Word b) { float v; std::memcpy(&v, &b, 4); return v; }
inline Word bits(float v) { Word b; std::memcpy(&b, &v, 4); return b; }
Word rules() { return word(kotor_cell_rules_007a3a28); }
Word app() { return word(kotor_menu_window_cells::manager_007a39fc); }
extern "C" unsigned __fastcall recovered_gui_in_game_get_party_access_panel_up_0062b4d0(void*, void*, unsigned);   // CGuiInGame::GetPartyAccessPanelUp (RET 4)
[[noreturn]] void ntrap(Word a, const char* w) { kotor_unreachable_trap(a, w); }

struct Bundle;
// One port-implementation struct per t4 logic file; each forwards into the Bundle.
#define W2_BUNDLE_PORT(T) Bundle& b; explicit T(Bundle& v) : b(v) {}

struct NObject final : g::ObjectPorts {
    Word is_npc_faction_0052b280(Word fm, Word id) override { return recovered_faction_is_npc_0052b280(ptr(fm), nullptr, id); }   // W1 tier6
    W2_BUNDLE_PORT(NObject)
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    Word server_module_004ae6b0(Word s) override { return word(server_get_module_004ae6b0(ptr(s), nullptr)); }
    Word client_object_by_id_004ae910(Word s, Word id) override { return word(kotor_fastcall_004ae910(ptr(s), nullptr, id)); }
    Word spot_bits_004efc70(Word c) override { return bits(recovered_creature_spot_distance_004efc70(ptr(c), nullptr)); }
    Word listen_bits_004efd20(Word c) override { return bits(recovered_creature_listen_distance_004efd20(ptr(c), nullptr)); }
    Word invisible_00501950(Word c, Word o) override { return recovered_creature_invisible_00501950(ptr(c), nullptr, ptr(o)); }
    Word visible_element_00517a20(Word c, Word id) override { return word(recovered_creature_visible_element_00517a20(ptr(c), nullptr, id)); }
    Word reputation_0057cb80(Word o, Word id, Word f) override;
    Word ai_state_reputation_0057cd90(Word c, Word id) override { return recovered_object_ai_state_reputation_0057cd90(ptr(c), nullptr, id) & 0xffu; }
    Word array_count_004af240(Word l, Word v) override { return exo_array_list_count_004af240(ptr(l), nullptr, v); }
    Word object_lookup_004d8230(Word a, Word id, Word out) override { return kotor_fastcall_004d8230(ptr(a), nullptr, id, static_cast<void**>(ptr(out))); }
    Word list_at_pos_005e9b60(Word l, Word pos) override { return word(unresolved_list_at_pos_005e9b60(ptr(l), nullptr, ptr(pos))); }
    void list_next_005e9b90(Word l, Word slot) override { (void)unresolved_list_next_005e9b90(ptr(l), nullptr, static_cast<void**>(ptr(slot))); }
    void list_remove_005e9c10(Word l, Word pos) override { c2da_005e9c10(ptr(l), nullptr, ptr(pos)); }
    void last_update_destroy_005ca6a0(Word last) override;
    Word allocate_006fa7e6(Word n) override { return word(live_operator_new(n)); }
    void free_006fa390(Word a) override { live_free(ptr(a)); }
    void list_allocate8_004c4f50(Word l, Word cap) override { directory_list::AllocateTimes004c4f50(static_cast<directory_list::TimeList*>(ptr(l)), nullptr, static_cast<int>(cap)); }
    Word animation_stationary_004cae60(Word o, Word a) override { return recovered_object_animation_stationary_004cae60(ptr(o), nullptr, a); }
    Word walk_rate_bits_004f1b20(Word c) override { return bits(w5_creature_get_walk_rate_004f1b20(ptr(c), nullptr)); }
    Word run_rate_bits_004f1be0(Word c) override { return bits(w5_creature_get_run_rate_004f1be0(ptr(c), nullptr)); }
    Word max_force_points_004fd490(Word c) override { return w1x_creature_max_fp_004fd490(ptr(c), nullptr); }
    Word faction_00513fc0(Word c) override;
    Word detect_mode_004edae0(Word c) override { return recovered_creature_detect_mode_004edae0(ptr(c), nullptr); }
    Word get_class_005a4e90(Word s, Word i) override;
    Word combat_attack_004d2b50(Word r, Word i) override { return word(recovered_combat_round_attack_004d2b50(ptr(r), nullptr, i)); }
    Word item_in_slot_005a4c20(Word inv, Word slot) override { return word(recovered_equipment_item_005a4c20(ptr(inv), nullptr, slot)); }
    void update_appearance_00572070(Word self, Word o, Word last, Word f) override;
    Word create_new_last_00577000(Word self, Word pl, Word o, Word fo, Word ao) override;
    Word compute_update_0056b950(Word self, Word pl, Word o, Word last, Word same) override;
    Word compute_appearance_00567100(Word self, Word o, Word last) override;
    void update_last_00575c80(Word self, Word pl, Word o, Word last, Word f) override;
    void update_last_appearance_00568480(Word o, Word last, Word a) override;
};
struct NCompute final : g::ComputePorts {
    W2_BUNDLE_PORT(NCompute)
    Word app_007a39fc() override { return app(); }
    Word vcall0(Word o, Word slot) override;
    Word vcall1(Word o, Word slot, Word a) override;
    void compute_ai_state_004f1460(Word c) override { recovered_creature_compute_ai_state_004f1460(ptr(c), nullptr); }
    Word game_object_00561080(Word pl) override { return word(kotor_fastcall_00561080(ptr(pl), nullptr)); }
    Word ai_state_reputation_0057cd90(Word c, Word id) override { return recovered_object_ai_state_reputation_0057cd90(ptr(c), nullptr, id) & 0xffu; }
    Word is_beam_004ca9d0(Word e) override;
    Word item_in_slot_005a4c20(Word inv, Word slot) override { return word(recovered_equipment_item_005a4c20(ptr(inv), nullptr, slot)); }
    Word get_class_005a4e90(Word s, Word i) override;
    Word get_class_level_005a5090(Word s, Word i, Word f) override;
    Word string_ne_005e5310(Word a, Word c) override { return async_string_equal_005e5200(ptr(a), nullptr, ptr(c)) == 0 ? 1u : 0u; }   // as 0886 NativeModuleLoad::compare_005e5310
    Word animation_stationary_004cae60(Word o, Word a) override { return recovered_object_animation_stationary_004cae60(ptr(o), nullptr, a); }
    Word object_area_004cb120(Word o) override;
    Word walk_rate_bits_004f1b20(Word c) override { return bits(w5_creature_get_walk_rate_004f1b20(ptr(c), nullptr)); }
    Word run_rate_bits_004f1be0(Word c) override { return bits(w5_creature_get_run_rate_004f1be0(ptr(c), nullptr)); }
    Word faction_00513fc0(Word c) override;
    Word max_force_points_004fd490(Word c) override { return w1x_creature_max_fp_004fd490(ptr(c), nullptr); }
    Word client_object_004ae910(Word s, Word id) override { return word(kotor_fastcall_004ae910(ptr(s), nullptr, id)); }
    Word singleton_party_005bf690(Word f) override;
    Word leader_005bf5c0(Word f) override;
    Word module_004ae6b0(Word s) override { return word(server_get_module_004ae6b0(ptr(s), nullptr)); }
    Word array_count_004af240(Word l, Word v) override { return exo_array_list_count_004af240(ptr(l), nullptr, v); }
    Word visible_element_00517a20(Word c, Word id) override { return word(recovered_creature_visible_element_00517a20(ptr(c), nullptr, id)); }
    Word damage_level_004cb020(Word o) override;
    Word resref_ne_00406dc0(Word a, Word c) override { return static_cast<Word>(gui_load_resref_not_equal_00406dc0(ptr(a), nullptr, ptr(c))); }
    Word creature_by_id_004ae770(Word s, Word id) override { return word(kotor_fastcall_004ae770(ptr(s), nullptr, id)); }
};
struct NLast final : g::LastPorts {
    W2_BUNDLE_PORT(NLast)
    w2::msg::Ports& msg() override;
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    Word app_007a39fc() override { return app(); }
    Word allocate_006fa7e6(Word n) override { return word(live_operator_new(n)); }
    void free_006fa390(Word a) override { live_free(ptr(a)); }
    void ref_default_00405ed0(Word r) override;
    void loc_construct_005e9e10(Word l) override;
    void loc_destroy_005ea000(Word l) override;
    void str_empty_005b3190(Word s) override;
    void str_destroy_005e5c20(Word s) override;
    void update_appearance_equipped_004ed620(Word c) override;
    Word game_object_00561080(Word pl) override { return word(kotor_fastcall_00561080(ptr(pl), nullptr)); }
    Word vcall0(Word o, Word slot) override;
    Word vcall1(Word o, Word slot, Word a) override;
    void write_locstring_005785c0(Word self, Word loc, Word gender) override;
    void add_item_appearance_00567090(Word self, Word item) override;
    void add_active_item_properties_00566f20(Word self, Word item, Word creature) override;
    Word reputation_0057cb80(Word o, Word id, Word f) override;
    Word door_is_linked_005890d0(Word d) override { return w5_door_get_is_linked_005890d0(ptr(d), nullptr); }
    void list_add_head_005e9d70(Word l, Word node) override { c2da_005e9d70(ptr(l), nullptr, ptr(node)); }
    Word object_area_004cb120(Word o) override;
    void resref_assign_00406120(Word self, Word result, Word source) override;
    Word ai_state_reputation_0057cd90(Word c, Word id) override { return recovered_object_ai_state_reputation_0057cd90(ptr(c), nullptr, id) & 0xffu; }
    Word animation_stationary_004cae60(Word o, Word a) override { return recovered_object_animation_stationary_004cae60(ptr(o), nullptr, a); }
    Word walk_rate_bits_004f1b20(Word c) override { return bits(w5_creature_get_walk_rate_004f1b20(ptr(c), nullptr)); }
    Word run_rate_bits_004f1be0(Word c) override { return bits(w5_creature_get_run_rate_004f1be0(ptr(c), nullptr)); }
    Word damage_level_004cb020(Word o) override;
    Word max_force_points_004fd490(Word c) override { return w1x_creature_max_fp_004fd490(ptr(c), nullptr); }
    Word faction_00513fc0(Word c) override;
    Word singleton_party_005bf690(Word f) override;
    Word faction_leader_005bf5c0(Word f) override;
    Word module_004ae6b0(Word s) override { return word(server_get_module_004ae6b0(ptr(s), nullptr)); }
    Word is_beam_004ca9d0(Word e) override;
    Word detect_mode_004edae0(Word c) override { return recovered_creature_detect_mode_004edae0(ptr(c), nullptr); }
    w2::msg::LocPorts& loc_ports() override;
    Word loc_get_string_005ea130(Word l, Word lang, Word out, Word gender) override { return exo_locstring_get_005ea130(ptr(l), nullptr, lang, ptr(out), gender); }
    void str_assign_text_005e5140(Word s, Word t) override { GuiAssign005e5140(ptr(s), nullptr, static_cast<const char*>(ptr(t))); }
    Word str_concat_005e5d10(Word self, Word result, Word rhs) override {
        return word(GuiConcat005e5d10(static_cast<kotor_reconstruction::StartupString*>(ptr(self)), nullptr, static_cast<kotor_reconstruction::StartupString*>(ptr(result)), static_cast<const kotor_reconstruction::StartupString*>(ptr(rhs))));
    }
    Word space_text_0073d5d8() override { return word(" "); }
    Word array_count_004af240(Word l, Word v) override { return exo_array_list_count_004af240(ptr(l), nullptr, v); }
    Word visible_list_element_00517a20(Word c, Word id) override { return word(recovered_creature_visible_element_00517a20(ptr(c), nullptr, id)); }
    void str_assign_005e5c50(Word d, Word s) override;
    Word creature_class_005a4e90(Word s, Word i) override;
    Word class_level_005a5090(Word s, Word i, Word f) override;
    void loc_assign_005e9e70(Word d, Word s) override;
    Word str_is_empty_005e58a0(Word s) override { return frame_LogIsEmpty_005e58a0(ptr(s), nullptr); }
};
struct NAppearance final : g::AppearancePorts {
    W2_BUNDLE_PORT(NAppearance)
    Word server_info_004aec90(Word s) override { return word(recovered_server_info_004aec90(ptr(s), nullptr)); }   // W1 tier6
    void set_last_instant_equip_004efe00(Word c, Word sl, Word v) override { recovered_creature_set_last_instant_equip_004efe00(ptr(c), nullptr, sl, v); }   // W1 tier7
    Word item_in_slot_005a4c20(Word inv, Word slot) override { return word(recovered_equipment_item_005a4c20(ptr(inv), nullptr, slot)); }
    Word base_item_005b31a0(Word a, Word t) override { return word(kotor_m15b_base_item_array_get_base_item_005b31a0(ptr(a), nullptr, static_cast<int>(t))); }
    Word rules_007a3a28() override { return rules(); }
    Word item_base_item_005b4790(Word i) override { return word(kotor_m15b_item_get_base_item_005b4790(ptr(i), nullptr)); }
    Word stats_level_005a5fd0(Word s, Word f) override { return recovered_stats_level_005a5fd0(ptr(s), nullptr, f); }
    Word item_property_exists_00555c90(Word i, Word t, Word st) override { return recovered_item_property_has_00555c90(ptr(i), nullptr, t, st); }
    Word item_property_by_type_005539c0(Word i, Word out, Word t, Word st) override { return recovered_item_property_query_005539c0(ptr(i), nullptr, ptr(out), t, st); }
    Word stats_has_feat_005a6630(Word s, Word f) override { return recovered_stats_has_feat_005a6630(ptr(s), nullptr, f); }
    Word rules_feat_00550c00(Word r, Word f) override { return word(kotor_rules::kotor_fastcall_00550c00(ptr(r), nullptr, static_cast<unsigned short>(f))); }
    Word stats_class_005a4e90(Word s, Word i) override;
    void list_add_00671c00(Word l, Word v) override { gui_load_array_add_00671c00(ptr(l), nullptr, v); }
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
};
struct NNpcFaction final : g::NpcFactionPort {
    Word is_npc_faction_0052b280(Word fm, Word id) override { return recovered_faction_is_npc_0052b280(ptr(fm), nullptr, id); }
};
struct NPanels final : g::PanelPorts {
    W2_BUNDLE_PORT(NPanels)
    Word server_info_004aec90(Word s) override { return word(recovered_server_info_004aec90(ptr(s), nullptr)); }
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    Word allocate_006fa7e6(Word n) override { return word(live_operator_new(n)); }
    void free_006fa390(Word a) override { live_free(ptr(a)); }
    Word last_update_object_00561820(Word pl, Word id) override { return word(recovered_last_update_object_00561820(ptr(pl), nullptr, id)); }
    void player_last_destroy_005cad70(Word o) override;
    Word player_last_construct_005cac80(Word s) override;
    void clear_spell_uses_005ca060(Word o) override;
    void gui_known_spells_00570930(Word self, Word c, Word plast) override;
    void test_player_differences_00572a20(Word self, Word pl, Word plast, Word last, Word fo) override;
    void player_update_00572dc0(Word self, Word pl, Word plast, Word last, Word f) override;
    void store_player_values_00574400(Word self, Word pl, Word plast, Word last, Word f) override;
    void repository_update_0056e360(Word self, Word pl, Word o, Word repo, Word luo, Word a, Word kind) override;
    Word object_lookup_004d8230(Word a, Word id, Word out) override { return kotor_fastcall_004d8230(ptr(a), nullptr, id, static_cast<void**>(ptr(out))); }
    Word in_game_gui_005ed690(Word c) override { return word(frame_R4_005ed690(ptr(c), nullptr)); }
    Word party_access_panel_up_0062b4d0(Word gui, Word flag) override { return recovered_gui_in_game_get_party_access_panel_up_0062b4d0(ptr(gui), nullptr, flag); }   // closeA: w3_gui body
    Word equip_screen_npc_0062fea0(Word gui) override;
    Word party_table_004aee70(Word s) override { return word(lane_config_required::GetPartyTable_004aee70(ptr(s), nullptr)); }
    Word npc_object_00564700(Word t, Word i, Word c, Word a) override { return recovered_npc_object_00564700(ptr(t), nullptr, i, c, a); }
    Word item_in_slot_005a4c20(Word inv, Word slot) override { return word(recovered_equipment_item_005a4c20(ptr(inv), nullptr, slot)); }
    void list_add_00671c00(Word l, Word v) override { gui_load_array_add_00671c00(ptr(l), nullptr, v); }
    void item_appearance_00567090(Word self, Word item) override;
    void active_item_properties_00566f20(Word self, Word item, Word c) override;
    Word item_repository_004ef770(Word c, Word f) override { return word(recovered_creature_inventory_004ef770(ptr(c), nullptr, f)); }
    Word faction_00513fc0(Word c) override;
    void test_party_differences_0056e850(Word self, Word pl, Word c, Word lo, Word fo) override;
    void update_object_00574a10(Word self, Word pl, Word o, Word last, Word f, Word af) override;
    void store_party_values_00567cb0(Word self, Word c, Word last, Word member, Word f) override;
    Word list_at_pos_005e9b60(Word l, Word pos) override { return word(unresolved_list_at_pos_005e9b60(ptr(l), nullptr, ptr(pos))); }
    void list_next_005e9b90(Word l, Word slot) override { (void)unresolved_list_next_005e9b90(ptr(l), nullptr, static_cast<void**>(ptr(slot))); }
    void list_remove_005e9c10(Word l, Word pos) override { c2da_005e9c10(ptr(l), nullptr, ptr(pos)); }
};
struct NPartyUpdate final : g::PartyUpdatePorts {
    Word is_npc_faction_0052b280(Word fm, Word id) override { return recovered_faction_is_npc_0052b280(ptr(fm), nullptr, id); }   // W1 tier6
    W2_BUNDLE_PORT(NPartyUpdate)
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    Word allocate_006fa7e6(Word n) override { return word(live_operator_new(n)); }
    void free_006fa390(Word a) override { live_free(ptr(a)); }
    void compute_ai_state_004f1460(Word c) override { recovered_creature_compute_ai_state_004f1460(ptr(c), nullptr); }
    Word ai_state_reputation_0057cd90(Word c, Word id) override { return recovered_object_ai_state_reputation_0057cd90(ptr(c), nullptr, id) & 0xffu; }
    Word max_force_points_004fd490(Word c) override { return w1x_creature_max_fp_004fd490(ptr(c), nullptr); }
    Word faction_00513fc0(Word c) override;
    void ref_default_00405ed0(Word r) override;
    Word resref_ne_00406dc0(Word a, Word c) override { return static_cast<Word>(gui_load_resref_not_equal_00406dc0(ptr(a), nullptr, ptr(c))); }
    Word resref_assign_00406120(Word self, Word r, Word s) override;
    Word list_at_pos_005e9b60(Word l, Word pos) override { return word(unresolved_list_at_pos_005e9b60(ptr(l), nullptr, ptr(pos))); }
    void list_next_005e9b90(Word l, Word slot) override { (void)unresolved_list_next_005e9b90(ptr(l), nullptr, static_cast<void**>(ptr(slot))); }
    void list_prev_005e9bd0(Word l, Word slot) override { kotor_fastcall_005e9bd0(ptr(l), nullptr, ptr(slot)); }
    // 005E9C10 returns the removed node's data (EDI); the bound provider is declared void, so the data is read first via GetAtPos
    Word list_remove_005e9c10(Word l, Word pos) override { const Word d = word(unresolved_list_at_pos_005e9b60(ptr(l), nullptr, ptr(pos))); c2da_005e9c10(ptr(l), nullptr, ptr(pos)); return d; }
    void list_add_head_005e9d70(Word l, Word d) override { c2da_005e9d70(ptr(l), nullptr, ptr(d)); }
    Word item_by_id_004ae760(Word s, Word id) override { return word(recovered_item_lookup_004ae760(ptr(s), nullptr, id)); }
    Word item_cost_00554000(Word item) override;
    void item_appearance_00567090(Word self, Word item) override;
    void active_item_properties_00566f20(Word self, Word item, Word c) override;
    Word panel_exists_0040bd70(Word mgr, Word panel) override { return frame_ContainsPanel_0040bd70(ptr(mgr), nullptr, ptr(panel)); }
};
struct NCharSheet final : g::CharSheetPorts {
    W2_BUNDLE_PORT(NCharSheet)
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    Word effect_bonus_004f3fe0(Word c, Word a1, Word a2, Word a3, Word a4, Word a5, Word a6, Word a7, Word a8, Word a9, Word a10) override;
    Word total_ability_bonus_005a58b0(Word stats, Word ability) override;
    Word is_debilitated_005b4880(Word c) override { return w5x_creature_is_debilitated_005b4880(ptr(c), nullptr); }
    Word item_in_slot_005a4c20(Word inv, Word slot) override { return word(recovered_equipment_item_005a4c20(ptr(inv), nullptr, slot)); }
    Word item_base_item_005b4790(Word i) override { return word(kotor_m15b_item_get_base_item_005b4790(ptr(i), nullptr)); }
    Word class_fort_bonus_005bccd0(Word cls, Word lvl) override { return kotor_m15b_class_get_fort_save_bonus_005bccd0(ptr(cls), nullptr, lvl); }
    Word class_will_bonus_005bcd10(Word cls, Word lvl) override { return kotor_m15b_class_get_will_save_bonus_005bcd10(ptr(cls), nullptr, lvl); }
    Word class_ref_bonus_005bccf0(Word cls, Word lvl) override { return kotor_m15b_class_get_ref_save_bonus_005bccf0(ptr(cls), nullptr, lvl); }
    Word has_feat_005a6630(Word s, Word f) override { return recovered_stats_has_feat_005a6630(ptr(s), nullptr, f); }
    Word class_attack_bonus_005bccb0(Word cls, Word lvl) override { return kotor_m15b_class_get_attack_bonus_005bccb0(ptr(cls), nullptr, lvl); }
    Word class_skill_useable_005be070(Word cls, Word skill) override { return kotor_m15b_class_is_skill_useable_005be070(ptr(cls), nullptr, skill); }
    Word class_armor_bonus_005be770(Word cls, Word lvl) override { return kotor_m15b_class_get_armor_bonus_005be770(ptr(cls), nullptr, lvl); }
    Word skill_rank_005aa570(Word s, Word sk, Word a, Word c) override { return w1x_creature_skill_rank_005aa570(ptr(s), nullptr, sk, a, c); }
    Word get_class_005a4e90(Word s, Word i) override;
    Word allocate_006fa7e6(Word n) override { return word(live_operator_new(n)); }
    void free_006fa390(Word a) override { live_free(ptr(a)); }
    void list_allocate_005e03c0(Word l, Word cap) override { menu_list_growth::allocate_005e03c0(static_cast<kotor_reconstruction::ArrayMutations*>(ptr(l)), nullptr, cap); }
};
struct NPlayer final : g::PlayerPorts {
    W2_BUNDLE_PORT(NPlayer)
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    Word armor_class_004ed1d0(Word c) override;
    Word gold_004edd60(Word c) override { return w2x_creature_gold_004edd60(ptr(c), nullptr); }
    Word skill_rank_005aa570(Word s, Word sk, Word a, Word c) override { return w1x_creature_skill_rank_005aa570(ptr(s), nullptr, sk, a, c); }
    Word can_use_skill_005af880(Word s, Word sk) override;
    Word stats_level_005a5fd0(Word s, Word f) override { return recovered_stats_level_005a5fd0(ptr(s), nullptr, f); }
    Word server_info_004aec90(Word s) override;
    Word dead_temp_004ef890(Word c) override { return recovered_creature_incapacitated_004ef890(ptr(c), nullptr); }
    Word detect_mode_004edae0(Word c) override { return recovered_creature_detect_mode_004edae0(ptr(c), nullptr); }
    Word get_class_005a4e90(Word s, Word i) override;
    Word is_jedi_005be4a0(Word cls) override { return kotor::server_mainloop::creature_force_class_005be4a0(cls); }
    Word get_feat_005a6920(Word s, Word i) override;
    Word bonus_feat_005a6940(Word s, Word i) override;
    Word feat_remaining_uses_005a6680(Word s, Word f) override;
    Word str_stat_005a6190(Word s) override; Word dex_stat_005a6550(Word s) override; Word con_stat_005a6250(Word s) override;
    Word int_stat_005a6310(Word s) override; Word wis_stat_005a63d0(Word s) override; Word cha_stat_005a6490(Word s) override;
    Word will_save_005ab880(Word s, Word f) override; Word reflex_save_005ab8f0(Word s, Word f) override; Word fort_save_005ab810(Word s, Word f) override;
    Word resref_str_00405fe0(Word r) override { return word(c2da_ref_00405fe0(ptr(r), nullptr)); }
    Word max_force_points_004fd490(Word c) override { return w1x_creature_max_fp_004fd490(ptr(c), nullptr); }
    Word compare_spell_like_0056c430(Word self, Word c, Word plast) override;
};
struct NPlayerLast final : g::PlayerLastPorts {
    Word sla_construct_006475c0(Word self) override { return word(recovered_sla_construct_006475c0(ptr(self), nullptr)); }   // W3 (t6b)
    W2_BUNDLE_PORT(NPlayerLast)
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    void write8(Word a, Word v) override { const auto x = static_cast<std::uint8_t>(v); std::memcpy(ptr(a), &x, 1); }
    Word allocate_006fa7e6(Word n) override { return word(live_operator_new(n)); }
    void free_006fa390(Word a) override { live_free(ptr(a)); }
    void array_construct_005a76d0(Word l) override { (void)exo_array_list_construct_005a76d0(ptr(l), nullptr); }
    void array_destruct_0056b6f0(Word l) override { exo_array_list_destruct_0056b6f0(ptr(l), nullptr); }
    void list_allocate_005e03c0(Word l, Word cap) override { menu_list_growth::allocate_005e03c0(static_cast<kotor_reconstruction::ArrayMutations*>(ptr(l)), nullptr, cap); }
    void list_u16_allocate_004d2fa0(Word l, Word cap) override { kotor_m15_array_u16_allocate_004d2fa0(ptr(l), nullptr, cap); }
    Word rules_007a3a28() override { return rules(); }
    Word gui_skills_00567430(Word c) override;
    Word player_state_00567600(Word c) override;
    Word known_spell_uses_005ca070(Word plast, Word cls, Word lvl) override;
    Word spells_per_day_left_005a5650(Word s, Word cls, Word lvl) override;
    Word can_level_up_005a6810(Word s) override;
    Word get_class_005a4e90(Word s, Word i) override;
    Word is_jedi_005be4a0(Word cls) override { return kotor::server_mainloop::creature_force_class_005be4a0(cls); }
    Word gold_004edd60(Word c) override { return w2x_creature_gold_004edd60(ptr(c), nullptr); }
    Word armor_class_004ed1d0(Word c) override;
    Word stats_level_005a5fd0(Word s, Word f) override { return recovered_stats_level_005a5fd0(ptr(s), nullptr, f); }
    Word str_stat_005a6190(Word s) override; Word dex_stat_005a6550(Word s) override; Word con_stat_005a6250(Word s) override;
    Word int_stat_005a6310(Word s) override; Word wis_stat_005a63d0(Word s) override; Word cha_stat_005a6490(Word s) override;
    Word skill_rank_005aa570(Word s, Word sk, Word a, Word c) override { return w1x_creature_skill_rank_005aa570(ptr(s), nullptr, sk, a, c); }
    Word resref_ne_00406dc0(Word a, Word c) override { return static_cast<Word>(gui_load_resref_not_equal_00406dc0(ptr(a), nullptr, ptr(c))); }
    void resref_assign_00406120(Word self, Word r, Word s) override;
    Word max_force_points_004fd490(Word c) override { return w1x_creature_max_fp_004fd490(ptr(c), nullptr); }
    Word get_feat_005a6920(Word s, Word i) override;
    Word bonus_feat_005a6940(Word s, Word i) override;
    Word vector_equal_004aa980(Word a, Word c) override { return aurora_vector_equal_004aa980(ptr(a), ptr(c)); }
};

struct NEffectSave final : g::EffectSavePorts {
    W2_BUNDLE_PORT(NEffectSave)
    [[noreturn]] void trap(Word a, const char* w) override { ntrap(a, w); }
    Word app_007a39fc() override { return app(); }
    Word rules_007a3a28() override { return rules(); }
    Word combat_attack_004d2b50(Word r, Word i) override { return word(recovered_combat_round_attack_004d2b50(ptr(r), nullptr, i)); }
    Word effect_integer_00503690(Word e, Word i) override { return recovered_effect_integer_00503690(ptr(e), nullptr, i); }
    Word item_by_id_004ae760(Word s, Word id) override { return word(recovered_item_lookup_004ae760(ptr(s), nullptr, id)); }
};
struct Bundle {
    Mem m; EntryPorts p{m};
    NObject object{*this}; NCompute compute{*this}; NLast last{*this}; NAppearance appearance{*this};
    NNpcFaction npcf; NPanels panels{*this}; NPartyUpdate party{*this}; NCharSheet sheet{*this};
    NPlayer player{*this}; NPlayerLast plast{*this}; NEffectSave esave{*this};
    std::uint64_t scratch[0x40] = {};                         // 0x200 bytes, 8-aligned
    Word s() { std::memset(scratch, 0, sizeof scratch); return word(scratch); }
    // one-body helpers shared by several port structs
    Word faction(Word c) { return kotor::server_mainloop::creature_faction_00513fc0(m, c); }
    Word klass(Word st, Word i) { return kotor::server_mainloop::creature_get_class_005a4e90(m, st, i); }
    Word stat(Word st, Word ability) { Bundle nb; return g::ability_stat(nb.m, nb.sheet, rules(), st, ability, nb.s()); }
};
// a fresh Bundle per nested call keeps each callee's scratch private (the original's stack frames)
#define FRESH Bundle nb
// ---- NObject
Word NObject::reputation_0057cb80(Word o, Word id, Word f) { return recovered_object_get_reputation_0057cb80(ptr(o), nullptr, id, f); }   // W1 tier6
void NObject::last_update_destroy_005ca6a0(Word last) { g::last_update_destroy_005ca6a0(b.m, b.last, last); }
Word NObject::faction_00513fc0(Word c) { return b.faction(c); }
Word NObject::get_class_005a4e90(Word s, Word i) { return b.klass(s, i); }
void NObject::update_appearance_00572070(Word self, Word o, Word last, Word f) { FRESH; g::update_appearance_00572070(nb.m, nb.p, nb.appearance, self, o, last, f, nb.s()); }
Word NObject::create_new_last_00577000(Word self, Word pl, Word o, Word fo, Word ao) { FRESH; return g::create_new_last_00577000(nb.m, nb.last, self, pl, o, fo, ao, nb.s()); }
Word NObject::compute_update_0056b950(Word self, Word pl, Word o, Word last, Word same) { FRESH; return g::compute_update_required_0056b950(nb.m, nb.compute, self, pl, o, last, same, nb.s()); }
Word NObject::compute_appearance_00567100(Word self, Word o, Word last) { return g::compute_appearance_update_required_00567100(b.m, b.compute, self, o, last); }
void NObject::update_last_00575c80(Word self, Word pl, Word o, Word last, Word f) { FRESH; g::update_last_00575c80(nb.m, nb.last, self, pl, o, last, f, nb.s()); }
void NObject::update_last_appearance_00568480(Word o, Word last, Word a) { g::update_last_appearance_00568480(b.m, b.last, o, last, a); }
// ---- NCompute
Word NCompute::vcall0(Word o, Word slot) { return b.p.vcall0(o, slot); }
Word NCompute::vcall1(Word o, Word slot, Word a) { return b.p.vcall1(o, slot, a); }
Word NCompute::is_beam_004ca9d0(Word e) { return g::is_beam_004ca9d0(b.m, e); }
Word NCompute::get_class_005a4e90(Word s, Word i) { return b.klass(s, i); }
Word NCompute::get_class_level_005a5090(Word s, Word i, Word f) { return g::class_level_005a5090(b.m, s, i, f); }
Word NCompute::object_area_004cb120(Word o) { return b.p.object_area_004cb120(o); }
Word NCompute::faction_00513fc0(Word c) { return b.faction(c); }
Word NCompute::singleton_party_005bf690(Word f) { return g::singleton_party_005bf690(b.m, b.p, f); }
Word NCompute::leader_005bf5c0(Word f) { return g::leader_005bf5c0(b.m, b.p, b.npcf, f); }
Word NCompute::damage_level_004cb020(Word o) { return g::damage_level_004cb020(b.m, b.p, o); }
// ---- NLast
w2::msg::Ports& NLast::msg() { return b.p; }
void NLast::ref_default_00405ed0(Word r) { b.p.ref_default_00405ed0(r); }
void NLast::loc_construct_005e9e10(Word l) { b.p.loc_construct_005e9e10(l); }
void NLast::loc_destroy_005ea000(Word l) { b.p.loc_destroy_005ea000(l); }
void NLast::str_empty_005b3190(Word s) { b.p.str_empty_005b3190(s); }
void NLast::str_destroy_005e5c20(Word s) { b.p.str_destroy_005e5c20(s); }
void NLast::update_appearance_equipped_004ed620(Word c) { g::update_appearance_equipped_004ed620(b.m, b.compute, c); }
Word NLast::vcall0(Word o, Word slot) { return b.p.vcall0(o, slot); }
Word NLast::vcall1(Word o, Word slot, Word a) { return b.p.vcall1(o, slot, a); }
void NLast::write_locstring_005785c0(Word self, Word loc, Word gender) { FRESH; w2::msg::write_locstring_005785c0(nb.m, nb.p, self, loc, gender, nb.s()); }
void NLast::add_item_appearance_00567090(Word self, Word item) { g::item_appearance_00567090(b.m, b.p, b.appearance, self, item); }
void NLast::add_active_item_properties_00566f20(Word self, Word item, Word c) { FRESH; g::active_item_properties_00566f20(nb.m, nb.p, nb.appearance, self, item, c, nb.s()); }
Word NLast::reputation_0057cb80(Word o, Word id, Word f) { return recovered_object_get_reputation_0057cb80(ptr(o), nullptr, id, f); }   // W1 tier6
Word NLast::object_area_004cb120(Word o) { return b.p.object_area_004cb120(o); }
void NLast::resref_assign_00406120(Word self, Word r, Word s) { b.p.ref_assign_lower_00406120(self, r, s); }
Word NLast::damage_level_004cb020(Word o) { return g::damage_level_004cb020(b.m, b.p, o); }
Word NLast::faction_00513fc0(Word c) { return b.faction(c); }
Word NLast::singleton_party_005bf690(Word f) { return g::singleton_party_005bf690(b.m, b.p, f); }
Word NLast::faction_leader_005bf5c0(Word f) { return g::leader_005bf5c0(b.m, b.p, b.npcf, f); }
Word NLast::is_beam_004ca9d0(Word e) { return g::is_beam_004ca9d0(b.m, e); }
w2::msg::LocPorts& NLast::loc_ports() { return b.p; }
void NLast::str_assign_005e5c50(Word d, Word s) { b.p.str_assign_005e5c50(d, s); }
Word NLast::creature_class_005a4e90(Word s, Word i) { return b.klass(s, i); }
Word NLast::class_level_005a5090(Word s, Word i, Word f) { return g::class_level_005a5090(b.m, s, i, f); }
void NLast::loc_assign_005e9e70(Word d, Word s) { b.p.loc_assign_005e9e70(d, s); }
// ---- NAppearance
Word NAppearance::stats_class_005a4e90(Word s, Word i) { return b.klass(s, i); }
// ---- NPanels
void NPanels::player_last_destroy_005cad70(Word o) { g::player_last_destroy_005cad70(b.m, b.plast, o); }
Word NPanels::player_last_construct_005cac80(Word s) { return g::player_last_construct_005cac80(b.m, b.plast, s); }
void NPanels::clear_spell_uses_005ca060(Word o) { g::clear_known_spell_uses_005ca060(b.m, o); }
void NPanels::gui_known_spells_00570930(Word self, Word c, Word plast) { (void)g::gui_known_spells_00570930(b.m, b.plast, self, c, plast); }
void NPanels::test_player_differences_00572a20(Word self, Word pl, Word plast, Word last, Word fo) { FRESH; g::test_player_differences_00572a20(nb.m, nb.p, nb.plast, self, pl, plast, last, fo, nb.s()); }
void NPanels::player_update_00572dc0(Word self, Word pl, Word plast, Word last, Word f) { FRESH; g::player_update_00572dc0(nb.m, nb.p, nb.player, self, pl, plast, last, f, nb.s()); }
void NPanels::store_player_values_00574400(Word self, Word pl, Word plast, Word last, Word f) { FRESH; g::store_player_values_00574400(nb.m, nb.p, nb.plast, self, pl, plast, last, f, nb.s()); }
void NPanels::repository_update_0056e360(Word self, Word pl, Word o, Word repo, Word luo, Word a, Word kind) { FRESH; g::repository_update_0056e360(nb.m, nb.p, nb.party, self, pl, o, repo, luo, a, kind, nb.s()); }
Word NPanels::equip_screen_npc_0062fea0(Word gui) { return static_cast<Word>(recovered_gui_in_game_get_current_equip_screen_npc_0062fea0(ptr(gui), nullptr)); }   // W3 (t6b)
void NPanels::item_appearance_00567090(Word self, Word item) { g::item_appearance_00567090(b.m, b.p, b.appearance, self, item); }
void NPanels::active_item_properties_00566f20(Word self, Word item, Word c) { FRESH; g::active_item_properties_00566f20(nb.m, nb.p, nb.appearance, self, item, c, nb.s()); }
Word NPanels::faction_00513fc0(Word c) { return b.faction(c); }
void NPanels::test_party_differences_0056e850(Word self, Word pl, Word c, Word lo, Word fo) { FRESH; g::test_party_differences_0056e850(nb.m, nb.p, nb.party, self, pl, c, lo, fo, nb.s()); }
void NPanels::update_object_00574a10(Word self, Word pl, Word o, Word last, Word f, Word af) { FRESH; g::update_object_00574a10(nb.m, nb.p, nb.object, self, pl, o, last, f, af, nb.s()); }
void NPanels::store_party_values_00567cb0(Word self, Word c, Word last, Word member, Word f) { FRESH; g::store_party_values_00567cb0(nb.m, nb.p, nb.party, self, c, last, member, f, nb.s()); }
// ---- NPartyUpdate
Word NPartyUpdate::faction_00513fc0(Word c) { return b.faction(c); }
void NPartyUpdate::ref_default_00405ed0(Word r) { b.p.ref_default_00405ed0(r); }
Word NPartyUpdate::resref_assign_00406120(Word self, Word r, Word s) { b.p.ref_assign_lower_00406120(self, r, s); return r; }
Word NPartyUpdate::item_cost_00554000(Word item) { return g::item_cost_00554000(b.m, b.appearance, item); }
void NPartyUpdate::item_appearance_00567090(Word self, Word item) { g::item_appearance_00567090(b.m, b.p, b.appearance, self, item); }
void NPartyUpdate::active_item_properties_00566f20(Word self, Word item, Word c) { FRESH; g::active_item_properties_00566f20(nb.m, nb.p, nb.appearance, self, item, c, nb.s()); }
// ---- NCharSheet: 004F3FE0 GetTotalEffectBonus is bound per case (only the owned specialisations exist)
Word NCharSheet::effect_bonus_004f3fe0(Word c, Word a1, Word a2, Word a3, Word a4, Word a5, Word a6, Word a7, Word a8, Word a9, Word a10) {
    return recovered_creature_total_effect_bonus_004f3fe0(ptr(c), nullptr, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);   // the single W2 entry
}
Word NCharSheet::total_ability_bonus_005a58b0(Word stats, Word ability) {
    using Getter = char(__fastcall*)(void*, void*);
    static constexpr Getter kGetters[] = {kotor_fastcall_005a58b0, kotor_fastcall_005a58e0, kotor_fastcall_005a5910,
                                          kotor_fastcall_005a5940, kotor_fastcall_005a5970, kotor_fastcall_005a59a0};
    if (ability >= 6) ntrap(0x005a58b0, "GetTotal*Bonus: ability index");
    return static_cast<unsigned char>(kGetters[ability](ptr(stats), nullptr));
}
Word NCharSheet::get_class_005a4e90(Word s, Word i) { return b.klass(s, i); }
// ---- NPlayer / NPlayerLast: stat getters are w2_g07_charsheet bodies
Word NPlayer::armor_class_004ed1d0(Word c) { return g::armor_class_004ed1d0(b.m, b.sheet, rules(), c); }
Word NPlayer::can_use_skill_005af880(Word s, Word sk) { return g::can_use_skill_005af880(b.m, b.sheet, rules(), s, sk); }
Word NPlayer::server_info_004aec90(Word s) { return word(recovered_server_info_004aec90(ptr(s), nullptr)); }
Word NPlayer::get_class_005a4e90(Word s, Word i) { return b.klass(s, i); }
Word NPlayer::get_feat_005a6920(Word s, Word i) { return g::get_feat_005a6920(b.m, s, i); }
Word NPlayer::bonus_feat_005a6940(Word s, Word i) { return g::bonus_feat_005a6940(b.m, s, i); }
Word NPlayer::feat_remaining_uses_005a6680(Word s, Word f) { return g::feat_remaining_uses_005a6680(b.m, b.appearance, s, f); }
Word NPlayer::str_stat_005a6190(Word s) { return b.stat(s, 0); }
Word NPlayer::dex_stat_005a6550(Word s) { return b.stat(s, 1); }
Word NPlayer::con_stat_005a6250(Word s) { return b.stat(s, 2); }
Word NPlayer::int_stat_005a6310(Word s) { return b.stat(s, 3); }
Word NPlayer::wis_stat_005a63d0(Word s) { return b.stat(s, 4); }
Word NPlayer::cha_stat_005a6490(Word s) { return b.stat(s, 5); }
Word NPlayer::will_save_005ab880(Word s, Word f) { FRESH; return g::will_save_005ab880(nb.m, nb.sheet, rules(), s, f, nb.s()); }
Word NPlayer::reflex_save_005ab8f0(Word s, Word f) { FRESH; return g::reflex_save_005ab8f0(nb.m, nb.sheet, rules(), s, f, nb.s()); }
Word NPlayer::fort_save_005ab810(Word s, Word f) { FRESH; return g::fort_save_005ab810(nb.m, nb.sheet, rules(), s, f, nb.s()); }
Word NPlayer::compare_spell_like_0056c430(Word self, Word c, Word plast) { return g::compare_spell_like_0056c430(b.m, self, c, plast); }
Word NPlayerLast::gui_skills_00567430(Word c) { return g::gui_skills_00567430(b.m, b.player, c); }
Word NPlayerLast::player_state_00567600(Word c) { return g::player_state_00567600(b.m, b.player, c); }
Word NPlayerLast::known_spell_uses_005ca070(Word plast, Word cls, Word lvl) { return g::known_spell_uses_005ca070(b.m, plast, cls, lvl); }
Word NPlayerLast::spells_per_day_left_005a5650(Word s, Word cls, Word lvl) { return g::spells_per_day_left_005a5650(b.m, s, cls, lvl); }
Word NPlayerLast::can_level_up_005a6810(Word s) { return g::can_level_up_005a6810(b.m, b.p, b.player, s); }
Word NPlayerLast::get_class_005a4e90(Word s, Word i) { return b.klass(s, i); }
Word NPlayerLast::armor_class_004ed1d0(Word c) { return g::armor_class_004ed1d0(b.m, b.sheet, rules(), c); }
Word NPlayerLast::str_stat_005a6190(Word s) { return b.stat(s, 0); }
Word NPlayerLast::dex_stat_005a6550(Word s) { return b.stat(s, 1); }
Word NPlayerLast::con_stat_005a6250(Word s) { return b.stat(s, 2); }
Word NPlayerLast::int_stat_005a6310(Word s) { return b.stat(s, 3); }
Word NPlayerLast::wis_stat_005a63d0(Word s) { return b.stat(s, 4); }
Word NPlayerLast::cha_stat_005a6490(Word s) { return b.stat(s, 5); }
void NPlayerLast::resref_assign_00406120(Word self, Word r, Word s) { b.p.ref_assign_lower_00406120(self, r, s); }
Word NPlayerLast::get_feat_005a6920(Word s, Word i) { return g::get_feat_005a6920(b.m, s, i); }
Word NPlayerLast::bonus_feat_005a6940(Word s, Word i) { return g::bonus_feat_005a6940(b.m, s, i); }
}

// ===================================================================== exports (thiscall exported as __fastcall, unused EDX)
extern "C" {
// G07 stream (bound from w2::native::EntryPorts)
unsigned __fastcall recovered_test_object_visible_0056dc60(void* self, void*, void* object, void* observer) { Bundle b; return g::test_object_visible_0056dc60(b.m, b.p, b.object, word(self), word(object), word(observer)); }
void __fastcall recovered_delete_for_object_0056e190(void* self, void*, void* player, unsigned id) { Bundle b; g::delete_for_object_0056e190(b.m, b.p, b.object, word(self), word(player), id, b.s()); }
void __fastcall recovered_update_object_00574a10(void* self, void*, void* player, void* object, void* last, unsigned flags, unsigned aflags) {
    Bundle b; g::update_object_00574a10(b.m, b.p, b.object, word(self), word(player), word(object), word(last), flags, aflags, b.s());
}
void __fastcall recovered_test_update_differences_00577fa0(void* self, void*, void* player, void* object, void* last_out, void* flags_out, void* aflags_out) {
    Bundle b; g::test_update_differences_00577fa0(b.m, b.p, b.object, word(self), word(player), word(object), word(last_out), word(flags_out), word(aflags_out), b.s());
}
void __fastcall recovered_store_values_00576fc0(void* self, void*, void* player, void* last, void* object, unsigned flags, unsigned aflags) {
    Bundle b; g::store_values_00576fc0(b.object, word(self), word(player), word(last), word(object), flags, aflags);
}
void __fastcall recovered_major_gui_panels_00577670(void* self, void*, void* player) { Bundle b; g::major_gui_panels_00577670(b.m, b.p, b.panels, word(self), word(player), b.s()); }
unsigned __fastcall recovered_party_ai_state_005779f0(void* self, void*, void* player) { Bundle b; return g::party_ai_state_005779f0(b.m, b.p, b.panels, word(self), word(player), b.s()); }
unsigned __fastcall recovered_charsheet_required_00561260(void* gui, void*, void* player) { Bundle b; return g::charsheet_required_00561260(b.m, b.p, b.sheet, word(gui), word(player), b.s()); }
void __fastcall recovered_write_charsheet_0056f490(void* self, void*, void* player, unsigned flags) { Bundle b; g::write_charsheet_0056f490(b.m, b.p, b.sheet, word(self), word(player), flags, b.s()); }
// last-update objects and their per-family writers
void* __fastcall recovered_create_new_last_00577000(void* self, void*, void* player, void* object, void* fo, void* ao) { Bundle b; return ptr(g::create_new_last_00577000(b.m, b.last, word(self), word(player), word(object), word(fo), word(ao), b.s())); }
unsigned __fastcall recovered_compute_update_required_0056b950(void* self, void*, void* player, void* object, void* last, unsigned flag) { Bundle b; return g::compute_update_required_0056b950(b.m, b.compute, word(self), word(player), word(object), word(last), flag, b.s()); }
unsigned __fastcall recovered_compute_appearance_update_00567100(void* self, void*, void* object, void* last) { Bundle b; return g::compute_appearance_update_required_00567100(b.m, b.compute, word(self), word(object), word(last)); }
void __fastcall recovered_update_last_00575c80(void* self, void*, void* player, void* object, void* last, unsigned flags) { Bundle b; g::update_last_00575c80(b.m, b.last, word(self), word(player), word(object), word(last), flags, b.s()); }
void __fastcall recovered_update_last_appearance_00568480(void*, void*, void* object, void* last, unsigned aflags) { Bundle b; g::update_last_appearance_00568480(b.m, b.last, word(object), word(last), aflags); }
void* __fastcall recovered_last_update_construct_005caa50(void* self, void*) { Bundle b; return ptr(g::last_construct_005caa50(b.m, b.last, word(self))); }
void __fastcall recovered_last_update_destroy_005ca6a0(void* self, void*) { Bundle b; g::last_update_destroy_005ca6a0(b.m, b.last, word(self)); }
void __fastcall recovered_update_appearance_00572070(void* self, void*, void* object, void* last, unsigned flags) { Bundle b; g::update_appearance_00572070(b.m, b.p, b.appearance, word(self), word(object), word(last), flags, b.s()); }
void __fastcall recovered_player_update_00572dc0(void* self, void*, void* player, void* plast, void* last, unsigned flags) { Bundle b; g::player_update_00572dc0(b.m, b.p, b.player, word(self), word(player), word(plast), word(last), flags, b.s()); }
void __fastcall recovered_test_player_differences_00572a20(void* self, void*, void* player, void* plast, void* last, void* fo) { Bundle b; g::test_player_differences_00572a20(b.m, b.p, b.plast, word(self), word(player), word(plast), word(last), word(fo), b.s()); }
void __fastcall recovered_store_player_values_00574400(void* self, void*, void* player, void* plast, void* last, unsigned flags) { Bundle b; g::store_player_values_00574400(b.m, b.p, b.plast, word(self), word(player), word(plast), word(last), flags, b.s()); }
void* __fastcall recovered_player_last_construct_005cac80(void* self, void*) { Bundle b; return ptr(g::player_last_construct_005cac80(b.m, b.plast, word(self))); }
void __fastcall recovered_player_last_destroy_005cad70(void* self, void*) { Bundle b; g::player_last_destroy_005cad70(b.m, b.plast, word(self)); }
void __fastcall recovered_test_party_differences_0056e850(void* self, void*, void* player, void* c, void* lo, void* fo) { Bundle b; g::test_party_differences_0056e850(b.m, b.p, b.party, word(self), word(player), word(c), word(lo), word(fo), b.s()); }
void __fastcall recovered_store_party_values_00567cb0(void* self, void*, void* pc, void* last, void* member, unsigned flags) { Bundle b; g::store_party_values_00567cb0(b.m, b.p, b.party, word(self), word(pc), word(last), word(member), flags, b.s()); }
void __fastcall recovered_repository_update_0056e360(void* self, void*, void* player, void* object, void* repo, void* luo, unsigned which, unsigned letter) {
    Bundle b; g::repository_update_0056e360(b.m, b.p, b.party, word(self), word(player), word(object), word(repo), word(luo), which, letter, b.s());
}
// endar-attachment clears (names kept from the t2 pending stubs, which are now removed)
void __fastcall recovered_last_update_clear_actions_005ca0d0(void* self, void*) { Bundle b; g::clear_action_queue_005ca0d0(b.m, word(self)); }
void __fastcall recovered_last_update_clear_spells_005cab20(void* self, void*) { Bundle b; g::clear_known_spells_005cab20(b.m, b.plast, word(self)); }
void __fastcall recovered_last_update_clear_spell_uses_005ca060(void* self, void*) { Bundle b; g::clear_known_spell_uses_005ca060(b.m, word(self)); }
void __fastcall recovered_last_update_clear_spell_lists_005cac30(void* self, void*) { Bundle b; g::clear_spell_add_delete_005cac30(b.m, b.plast, word(self)); }
void __fastcall recovered_last_update_clear_feats_005cab60(void* self, void*) { Bundle b; g::clear_feats_005cab60(b.m, b.plast, word(self)); }
// shared creature / faction / object helpers other lanes need
unsigned __fastcall recovered_can_level_up_005a6810(void* stats, void*) { Bundle b; return g::can_level_up_005a6810(b.m, b.p, b.player, word(stats)); }
unsigned __fastcall recovered_object_damage_level_004cb020(void* object, void*) { Bundle b; return g::damage_level_004cb020(b.m, b.p, word(object)); }
unsigned __fastcall recovered_faction_singleton_party_005bf690(void* faction, void*) { Bundle b; return g::singleton_party_005bf690(b.m, b.p, word(faction)); }
unsigned __fastcall recovered_faction_leader_005bf5c0(void* faction, void*) { Bundle b; return g::leader_005bf5c0(b.m, b.p, b.npcf, word(faction)); }
unsigned __fastcall recovered_stats_class_level_005a5090(void* stats, void*, unsigned index, unsigned flag) { Bundle b; return g::class_level_005a5090(b.m, word(stats), index, flag); }
unsigned __fastcall recovered_effect_is_beam_004ca9d0(void* effect, void*) { Bundle b; return g::is_beam_004ca9d0(b.m, word(effect)); }
unsigned __fastcall recovered_item_cost_00554000(void* item, void*) { Bundle b; return g::item_cost_00554000(b.m, b.appearance, word(item)); }
unsigned __fastcall recovered_creature_can_use_item_0051b440(void* c, void*, void* item, unsigned unused) { Bundle b; return g::can_use_item_0051b440(b.m, b.p, b.appearance, word(c), word(item), unused, b.s()); }
unsigned __fastcall recovered_stats_reflex_save_005ab8f0(void* s, void*, unsigned f) { Bundle b; return g::reflex_save_005ab8f0(b.m, b.sheet, rules(), word(s), f, b.s()); }   // t6 (SaveStats)
unsigned __fastcall recovered_stats_base_fort_save_005aa1b0(void* s, void*) { Bundle b; return g::base_save(b.m, b.sheet, rules(), word(s), 0); }   // t6
unsigned __fastcall recovered_stats_base_will_save_005aa2f0(void* s, void*) { Bundle b; return g::base_save(b.m, b.sheet, rules(), word(s), 1); }   // t6
// t6: single-function exports of CSWSCreatureStats::Get<Ability>Stat (thiscall, no args, AL) for W6
unsigned char __fastcall recovered_creature_stats_str_005a6190(void* s, void*) { Bundle b; return static_cast<unsigned char>(b.stat(word(s), 0)); }
unsigned char __fastcall recovered_creature_stats_dex_005a6550(void* s, void*) { Bundle b; return static_cast<unsigned char>(b.stat(word(s), 1)); }
unsigned char __fastcall recovered_creature_stats_con_005a6250(void* s, void*) { Bundle b; return static_cast<unsigned char>(b.stat(word(s), 2)); }
unsigned char __fastcall recovered_creature_stats_int_005a6310(void* s, void*) { Bundle b; return static_cast<unsigned char>(b.stat(word(s), 3)); }
unsigned char __fastcall recovered_creature_stats_wis_005a63d0(void* s, void*) { Bundle b; return static_cast<unsigned char>(b.stat(word(s), 4)); }
unsigned char __fastcall recovered_creature_stats_cha_005a6490(void* s, void*) { Bundle b; return static_cast<unsigned char>(b.stat(word(s), 5)); }
unsigned __fastcall recovered_stats_can_use_skill_005af880(void* s, void*, unsigned skill) { Bundle b; return g::can_use_skill_005af880(b.m, b.sheet, rules(), word(s), skill); }   // t6: W7 286 GetHasSkill
unsigned __fastcall recovered_creature_armor_class_004ed1d0(void* c, void*) { Bundle b; return g::armor_class_004ed1d0(b.m, b.sheet, rules(), word(c)); }
unsigned __cdecl recovered_get_locstring_server_00578870(void* player, void* loc, void* loc2, void* out, unsigned gender) {
    Bundle b; return g::get_locstring_server_00578870(b.m, b.last, word(player), word(loc), word(loc2), word(out), gender, b.s());
}
// 004F3FE0 save case (W2 t4) and the client -> server senders assigned from the W4 review
// 004F3FE0 CSWSCreature::GetTotalEffectBonus: the single native entry (t4b). thiscall, 10 args, RET 28h -> @48.
//   kind = a1 low byte; 0 / > 5: original default arm, handled by the save body (prefix, then EAX 0)
//   3: W2 effect_save body (null target; non-null target traps 004F405E)
//   4: 1122 creature_effect_ability via 0886 (only a2=0,a4..a7=0,a8=FFh,a9<6,a10=0; anything else traps 004F3FE0)
//   5: 1122 creature_effect_skill via 0886 (only a2=0,a4..a7=0,a9=FFh,a10=0; non-empty effect list traps 004F5A8F)
//   1 (004F410D attack), 2 (004F4491 damage): explicit traps.
// closeA: the complete 004F3FE0 body (all kinds incl. 1 attack / 2 damage / targets) is owned by
// claude_combat_attack_effectbonus.cpp (@kotor_fastcall_004f3fe0@48); this W2 export forwards to it so the W2 callers
// (w2_save_stats_native and the G07 writers) reach the real body instead of the partial arms that trapped.
unsigned __fastcall kotor_fastcall_004f3fe0(void*, void*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
unsigned __fastcall recovered_creature_total_effect_bonus_004f3fe0(void* c, void*, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5, unsigned a6, unsigned a7, unsigned a8, unsigned a9, unsigned a10) {
    return kotor_fastcall_004f3fe0(c, nullptr, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}
unsigned __fastcall recovered_client_send_00677410(void* self, void*, unsigned major, unsigned minor, void* data, unsigned size) {
    Bundle b; return w2::client_send::send_00677410(b.m, b.p, word(self), b.m.read(app() + 4), major, minor, word(data), size);
}
unsigned __fastcall recovered_client_gui_inventory_status_00677a20(void* self, void*, unsigned status) {
    Bundle b; return w2::client_send::gui_inventory_status_00677a20(b.m, b.p, word(self), b.m.read(app() + 4), status, b.s());
}
unsigned __fastcall recovered_client_journal_quest_open_00677820(void*, void*) { Bundle b; return w2::client_send::journal_quest_screen_open_00677820(b.m, b.p, b.m.read(app() + 4)); }
}
