// W5 t3: ProcessInput world picking (mouse-over target under the cursor), 00622D09..006236FD, and the callees it
// needs that have no implementation in any lane. Runs on every in-game frame once CSWCModule+40h (the scene) exists
// and the camera is not in mouse-look (W4's MainLoop analysis, MAINLOOP-PATHS.md; m48t process_input.cpp T-PI-3).
//
// Call-site contract for W4 (T-PI-3 in process_input.cpp): replace the trap with
//     w5_process_input_picking_00622d09(this, nullptr);
// The region reads no ProcessInput local that it does not write itself; it ends at 006236FD (the caller then frees
// the event list as it already does).
#include "9b5634a69a-w5_t2.hpp"
#include "a3b6729268-w5_server_env.hpp"
#include <cstdint>
#include <cstring>
#include <initializer_list>

using namespace w5;

namespace kotor_menu_window_cells { extern void* volatile input_007a39e4; }             // [007A39E4] CExoInput*  (0053)
namespace kotor_input_cells { extern void* volatile gui_manager_007a39f4; }             // [007A39F4] CSWGuiManager* (0392)
namespace kotor_input_cells_required { extern volatile int word_008338f4; }             // [008338F4] (0456)
namespace menu_tables_m41 {
unsigned __fastcall InputSetMouseCursor_005df5c0(void* input, void*, unsigned cursor, unsigned unused, unsigned adjust); // 0366
void __fastcall GuiSetSoftwareMouseCursor_0040a270(void* gui, void*, unsigned cursor, unsigned visible, unsigned adjust); // 0366
}
namespace menu_providers_40_49 {
struct Internal { __declspec(noinline) void __thiscall mouse_0061f980(unsigned, unsigned, unsigned); };  // 0026 CClientExoAppInternal::SetMouseCursor
}
namespace kotor_input_required {
void* __fastcall get_character_006346c0(void* party, void*, unsigned index);            // CSWParty::GetCharacter (0457)
void* __fastcall get_in_game_gui_005ed690(void* app, void*);                            // CClientExoApp::GetInGameGui (0457)
}
namespace kotor_reconstruction { struct ArrayMutations; }
namespace menu_list_growth { void __fastcall allocate_005e03c0(kotor_reconstruction::ArrayMutations*, void*, unsigned int); } // CExoArrayList::Allocate

extern "C" {
// ---- existing providers in build/1122 (exact names) ----
void* __fastcall packed_file_get_async_file_00644760(void* self, void*);                // GetProperty0x18 -> CClientExoAppInternal+18 (module)
void* __fastcall kotor_fastcall_00641980(void* module, void*);                          // CSWCModule::GetMiniGame
void* __fastcall kotor_fastcall_005f1420(void* internal, void*);                        // CClientExoAppInternal::GetPlayerCreature
void* __fastcall kotor_fastcall_005f10f0(void* internal, void*, U id);                  // CClientExoAppInternal::GetGameObject
void* __fastcall kotor_fastcall_005ed580(void* app, void*, U id);                       // CClientExoApp::GetGameObject
unsigned char __fastcall kotor_fastcall_00684ed0(void* menu, void*, void* control);     // CSWGuiTargetActionMenu::IsTargetActionMenuControl (0769)
void* __fastcall aurora_hit_info_construct_004578f0(void* self, void*, U a, U b, U c);  // CHitInfo::CHitInfo (0478)
void __fastcall kotor_fastcall_005f6410(void* internal, void*);                         // EmptyHitcheckIgnoreList (0399)
int __fastcall kotor_fastcall_005df730(void* input, void*);                             // CExoInput::GetLastSetMouseDownState (src/bridge)
U __fastcall c2da_current_get_int_entry_00413510(void* table, void*, U row, U column, I* out); // C2DA::GetINTEntry(int, int, int*) (0390)
extern U kotor_rules_cell32_007a225c;                                                   // [007A225C] cached column index (0431)
U16 __fastcall kotor_m15a_object_get_current_animation_0063c060(void* object, void*);   // CSWCObject::GetCurrentAnimation (0817)
void* __fastcall kotor_m15e_item_get_equipped_by_00646630(void* item, void*);           // CSWCItem::GetEquippedBy (0835)
U __fastcall kotor_m15_client_get_player_creature_id_005ed550(void* app, void*);        // CClientExoApp::GetPlayerCreatureId (0813)
void* __fastcall kotor_m15_client_get_item_by_id_005ed5a0(void* app, void*, U id);      // CClientExoApp::GetItemByGameObjectID (0813)
U __fastcall kotor_m15a_creature_get_equipped_item_id_0060ca80(void* creature, void*, U slot); // CSWCCreature::GetEquippedItemID (0816)
void* __fastcall kotor_m15b_base_item_array_get_base_item_005b31a0(void* array, void*, U id);  // CSWBaseItemArray::GetBaseItem (0820)
void __fastcall int_list_add_00444fe0(void* list, void*, void* value);                  // CExoArrayList<int>::Add (0095)
void __fastcall exo_array_list_remove_00445080(void* list, void*, void* value_ref);     // CExoArrayList::Remove (0314)
// ---- other lanes (trap fallback via /alternatename in w5_t3_foreign_traps.cpp) ----
U __fastcall kotor_fastcall_00617ad0(void* creature, void*, void* target);              // CSWCCreature::CanSee (W4)
U __fastcall kotor_fastcall_00604bc0(void* area, void*, U id);                          // CSWCArea::GetIsObjectCorpse (W4)
void* __fastcall kotor_fastcall_00682e00(void* placeable, void*);                       // CSWCPlaceable::GetServerPlaceable (W4 t1 party/selection.cpp)
void __fastcall recovered_client_internal_add_to_hitcheck_ignore_list_005f6340(void* internal, void*, U id); // W3 t2

// ---- W5 t3 exports ----
void __fastcall w5_process_input_picking_00622d09(void* ci, void*);
void __fastcall kotor_fastcall_00620620(void* ci, void*);                                  // GetDefaultActions (W5 t4)
U __fastcall kotor_fastcall_0061faa0(void* ci, void*, U action, U multiple);              // action -> cursor (W5 t4)
void __fastcall kotor_fastcall_006222f0(void* ci, void*, U id, const Vector* position, U flag);   // HandleMouseOver, RET 0Ch
void __fastcall kotor_fastcall_005f94d0(void* ci, void*, void* array);                  // SetHitcheckIgnoreList, RET 4
I __fastcall kotor_fastcall_005f4fb0(void* list, void*, U key, int(__cdecl* compare)(U, U, U, U)); // CExoArrayList<CNearestStruct>::IndexOf, RET 8
int __cdecl kotor_cdecl_0061fa90(U key, U id, U b, U c);                                // NearestObjectCompare
U __fastcall kotor_fastcall_0061f790(void* door, void*);                                // CSWCDoor::GetIsOpen
}

namespace {
constexpr U k_none = 0x7f000000u;
const float k_far_00007455bc = 900.0f;            // .rdata 007455BC
inline void* in_game_main(void* ci) { return at<void*>(at<void*>(ci, 0x40), 0x90); }   // CGuiInGame+90 main interface
inline void* module_of(void* ci) { return packed_file_get_async_file_00644760(ci, nullptr); }
inline void* scene_of(void* ci) { return at<void*>(module_of(ci), 0x40); }
inline Vector& vec_at(void* p, U off) { return *reinterpret_cast<Vector*>(static_cast<unsigned char*>(p) + off); }
// inline CExoArrayList<void*>::Add (data/count/alloc), as at 005F957A..9E and 005F9734..59
inline void array_add(void* array, void* value) {
    if (at<I>(array, 4) == at<I>(array, 8)) {
        const I alloc = at<I>(array, 8);
        menu_list_growth::allocate_005e03c0(static_cast<kotor_reconstruction::ArrayMutations*>(array), nullptr,
                                            static_cast<unsigned>(alloc != 0 ? alloc + alloc : 0x10));
    }
    at<void**>(array, 0)[at<I>(array, 4)] = value;
    at<I>(array, 4) += 1;
}
}  // namespace

extern "C" {

// 0061FA90 NearestObjectCompare(key, CNearestStruct by value): key == id
int __cdecl kotor_cdecl_0061fa90(U key, U id, U, U) { return key == id ? 1 : 0; }

// 005F4FB0 CExoArrayList<CNearestStruct>::IndexOf(key, compare): 12-byte elements passed by value
I __fastcall kotor_fastcall_005f4fb0(void* list, void*, U key, int(__cdecl* compare)(U, U, U, U)) {
    if (compare == nullptr) return -1;                                              // 005F4FB0..BC
    for (I i = 0; i < at<I>(list, 4); ++i) {                                        // 005F4FBE..FE
        const U* e = at<U*>(list, 0) + i * 3;
        if (compare(key, e[0], e[1], e[2]) != 0) return i;                          // 005F4FD0..F3
    }
    return -1;                                                                       // 005F5000
}

// 0061F790 CSWCDoor::GetIsOpen: current animation 2742h or 2743h
U __fastcall kotor_fastcall_0061f790(void* door, void*) {
    if (kotor_m15a_object_get_current_animation_0063c060(door, nullptr) == 0x2742) return 1;
    return kotor_m15a_object_get_current_animation_0063c060(door, nullptr) == 0x2743 ? 1u : 0u;
}

// 005F94D0 CClientExoAppInternal::SetHitcheckIgnoreList(CExoArrayList<Gob*>* array): models of every ignored
// object (and of a creature's head, weapons and +21C item) plus the area's +204 list.
void __fastcall kotor_fastcall_005f94d0(void* ci, void*, void* array) {
    void* position = *at<void**>(ci, 0x190);                                         // 005F94D5..DD ([esp+8])
    void* data = position ? at<void*>(position, 8) : nullptr;                       // 005F94E1..E9
    while (position != nullptr) {                                                    // 005F94EC..F2 / 005F96EA..F0
        if (data != nullptr) {
            void* found = nullptr;                                                   // over the argument slot
            const unsigned char status = static_cast<unsigned char>(kotor_fastcall_004d8230(at<void*>(ci, 0x14), nullptr, *static_cast<U*>(data), &found)); // 005F9500..13
            void* o = (status == 0 && found != nullptr) ? vcall<void*>(found, 0x0c) : nullptr; // 005F9518..35
            if (o != nullptr) {
                void* creature = vcall<void*>(o, 0x2c);                              // 005F953F..46
                (void)vcall<void*>(o, 0x34);                                         // 005F9548..4C (result unused)
                if (vcall<void*>(o, 0x98, 0xffu, 1u) != nullptr)                     // 005F954F..62
                    array_add(array, vcall<void*>(o, 0x98, 0xffu, 1u));              // 005F9564..9E
                if (creature != nullptr) {
                    if (void* extra = at<void*>(creature, 0x21c)) {                  // 005F95A9..B1
                        if (void* item = kotor_m15_client_get_item_by_id_005ed5a0(client_app(), nullptr, at<U>(extra, 0x10))) { // 005F95B3..C7
                            void* gob = static_cast<unsigned char*>(item) + 0x10;
                            if (vcall<void*>(gob, 0x98, 0xffu, 1u) != nullptr)
                                int_list_add_00444fe0(array, nullptr, vcall<void*>(gob, 0x98, 0xffu, 1u)); // 005F95C9..F5
                        }
                        if (vcall<void*>(creature, 0x98, 0xfeu, 1u) != nullptr)      // 005F95FA..0D
                            int_list_add_00444fe0(array, nullptr, vcall<void*>(creature, 0x98, 0xfeu, 1u)); // 005F960F..23
                    }
                    for (U slot : {0x10u, 0x20u}) {                                   // 005F9628..D5
                        void* item = kotor_m15_client_get_item_by_id_005ed5a0(client_app(), nullptr,
                            kotor_m15a_creature_get_equipped_item_id_0060ca80(creature, nullptr, slot));
                        if (item == nullptr) continue;
                        if (slot == 0x10)
                            (void)kotor_m15b_base_item_array_get_base_item_005b31a0(at<void*>(kotor_cell_rules_007a3a28, 0x34), nullptr, at<U>(item, 0xc)); // 005F9647..54
                        void* gob = static_cast<unsigned char*>(item) + 0x10;
                        if (vcall<void*>(gob, 0x98, 0xffu, 1u) != nullptr)
                            int_list_add_00444fe0(array, nullptr, vcall<void*>(gob, 0x98, 0xffu, 1u));
                    }
                }
            }
        }
        data = reinterpret_cast<void*>(static_cast<std::uintptr_t>(
            unresolved_list_next_005e9b90(at<void*>(ci, 0x190), nullptr, &position)));  // 005F96DA..E5 (EAX = data)
    }
    void* module = at<void*>(ci, 0x18);                                              // 005F96F6
    if (module == nullptr || at<void*>(module, 0x48) == nullptr) return;              // 005F96F9..702
    void** list_holder = &at<void*>(at<void*>(module, 0x48), 0x204);                 // 005F9704..0E (ESI)
    void* pos = *static_cast<void**>(*list_holder);                                  // [esp+18]
    void* item = pos ? at<void*>(pos, 8) : nullptr;                                  // 005F9718..23
    if (pos == nullptr) return;                                                      // 005F9727
    for (;;) {
        if (item != nullptr) array_add(array, item);                                 // 005F9730..59
        item = reinterpret_cast<void*>(static_cast<std::uintptr_t>(
            unresolved_list_next_005e9b90(*list_holder, nullptr, &pos)));            // 005F975C..68
        if (pos == nullptr) return;                                                  // 005F976A..70
    }
}

// 006222F0 CClientExoAppInternal::HandleMouseOver(id, position*, flag)
void __fastcall kotor_fastcall_006222f0(void* ci, void*, U id, const Vector* position, U flag) {
    copy_vector(static_cast<unsigned char*>(ci) + 0x4a8, position);                  // 006222F3..0A
    at<U>(ci, 0x4a4) = id;                                                           // 00622317
    void* in_game = at<void*>(ci, 0x40);
    if (in_game != nullptr && (at<U>(in_game, 0x30) != 0 || at<I>(in_game, 0xb4) == 1 ||
                               at<U8>(at<void*>(ci, 4), 0x6d) == 5)) {               // 0062230D..36
        void* input = kotor_menu_window_cells::input_007a39e4;
        U cursor = 1;
        if (input != nullptr) cursor = menu_tables_m41::InputSetMouseCursor_005df5c0(input, nullptr, 1, 0, 0); // 00622338..4C
        if (at<void*>(ci, 0x274) == nullptr) return;                                 // 00622351..59
        menu_tables_m41::GuiSetSoftwareMouseCursor_0040a270(at<void*>(ci, 0x274), nullptr, cursor, 0, 0); // 0062235F..75 (tail jump)
        return;
    }
    void* const over = at<void*>(kotor_input_cells::gui_manager_007a39f4, 8);        // 0062237A..80 (EBX)
    bool defaults = flag != 0;                                                       // 00622383..8A
    if (!defaults && id != k_none && at<U>(ci, 0x2b4) != k_none &&
        id != kotor_m15_client_get_player_creature_id_005ed550(client_app(), nullptr) && over == nullptr &&
        at<U>(ci, 0x2b4) == at<U>(ci, 0x4a4))                                        // 00622390..C6
        defaults = true;
    if (defaults) {                                                                  // 00622446 (t4)
        kotor_fastcall_00620620(ci, nullptr);                                        // 0062244F GetDefaultActions
        void* first = nullptr; U multiple = 0;                                       // EDI, ECX
        if (at<I>(ci, 0x4cc) > 0) { first = at<void*>(ci, 0x4c8); multiple = at<I>(ci, 0x4cc) > 1 ? 1u : 0u; }   // 00622454..68
        const int down = kotor_input_cells_required::word_008338f4;                  // 00622476 (EBP)
        const U hov = at<U>(ci, 0x4a4);
        if (hov != k_none && hov != at<U>(at<void*>(at<void*>(ci, 0x40), 0x90), 0x64)) {   // 0062246B..8A
            reinterpret_cast<menu_providers_40_49::Internal*>(ci)->mouse_0061f980(0x2d, 0, 0);   // -> 006223EF
            return;
        }
        U cursor = 1;                                                                // EBX
        if (first != nullptr) cursor = kotor_fastcall_0061faa0(ci, nullptr, at<U>(first, 8), multiple);  // 00622490..A0
        void* input = kotor_menu_window_cells::input_007a39e4;
        U shown = cursor;
        if (input != nullptr) shown = menu_tables_m41::InputSetMouseCursor_005df5c0(input, nullptr, cursor, 0, static_cast<unsigned>(down)); // 006224A2..B2
        if (at<void*>(ci, 0x274) == nullptr) return;                                 // 006224B7..BF
        menu_tables_m41::GuiSetSoftwareMouseCursor_0040a270(at<void*>(ci, 0x274), nullptr, shown, 0, static_cast<unsigned>(down)); // 006224C1..C5
        return;
    }
    const U hovered = at<U>(ci, 0x4a4);                                              // 006223C8
    if (hovered != k_none && kotor_fastcall_005f4fb0(static_cast<unsigned char*>(ci) + 0x2a8, nullptr, hovered, &kotor_cdecl_0061fa90) != -1 &&
        over == nullptr) {                                                           // 006223CE..ED
        reinterpret_cast<menu_providers_40_49::Internal*>(ci)->mouse_0061f980(0x2d, 0, 0); // 006223EF..F7
        return;
    }
    void* input = kotor_menu_window_cells::input_007a39e4;
    const U down = static_cast<U>(kotor_fastcall_005df730(input, nullptr));         // 00622403..09 GetLastSetMouseDownState (no null test)
    U cursor = 1;
    if (input != nullptr) cursor = menu_tables_m41::InputSetMouseCursor_005df5c0(input, nullptr, 1, 0, down); // 0062240E..23
    if (at<void*>(ci, 0x274) == nullptr) return;                                     // 00622428..30
    menu_tables_m41::GuiSetSoftwareMouseCursor_0040a270(at<void*>(ci, 0x274), nullptr, cursor, 0, down); // 00622436..3A
}

// 00622D09..006236FD (inside CClientExoAppInternal::ProcessInput): world picking
void __fastcall w5_process_input_picking_00622d09(void* ci, void*) {
    void* const gui = at<void*>(ci, 0x274);
    if (void* over = at<void*>(gui, 8)) {                                            // 00622D09..14
        const unsigned char k = kotor_fastcall_00684ed0(static_cast<unsigned char*>(in_game_main(ci)) + 0xbc, nullptr, over); // 00622D16..26
        if (k != 0) {                                                                // 00622D2B..2D
            Vector zero{0, 0, 0};                                                    // [esp+30]
            if (k == 1) kotor_fastcall_006222f0(ci, nullptr, at<U>(ci, 0x2b4), &zero, 1); // 00622D4D..69
            else kotor_fastcall_006222f0(ci, nullptr, k_none, &zero, 0);             // 00622D5D..69
            at<U>(in_game_main(ci), 0x5cac) = at<U>(ci, 0x2b4);                      // 00622D6E..7D
            return;                                                                  // 00622D83 -> 006236FD
        }
    }
    alignas(8) unsigned char hit[0x64];                                              // [esp+C0] CHitInfo
    aurora_hit_info_construct_004578f0(hit, nullptr, 0, 0xbfffffffu, 0);             // 00622D88..98
    hit[0x60] = 1;                                                                   // 00622DA2
    Vector pos{0, 0, 0};                                                             // [esp+70]
    void* const gm = kotor_input_cells::gui_manager_007a39f4;                        // 00622D9D
    const I y = static_cast<I>(at<std::int16_t>(gm, 0x6e)) - at<I>(gm, 4);           // 00622DC2..CB (EBX)
    const I x = at<I>(gm, 0);                                                        // [esp+1C]
    kotor_fastcall_005f6410(ci, nullptr);                                            // 00622DDB EmptyHitcheckIgnoreList
    recovered_client_internal_add_to_hitcheck_ignore_list_005f6340(ci, nullptr, at<U>(ci, 0x20)); // 00622DE0..E6 the player
    void* o = nullptr;                                                               // EDI
    for (;;) {                                                                       // 00622DF0
        kotor_fastcall_005f94d0(ci, nullptr, hit + 8);                               // 00622DF0..02
        vcall<void>(scene_of(ci), 0x30, static_cast<U>(x), static_cast<U>(y), &pos, static_cast<void*>(hit)); // 00622E07..26
        void* h = at<void*>(hit, 0x38);
        if (h == nullptr || vcall<void*>(h, 0x70) == nullptr) goto scan;            // 00622E29..3F
        const U type = vcall<U>(h, 0x78);                                            // 00622E45..4E
        switch (type) {                                                              // table 00624498 (types 5..10)
        case 5:                                                                      // 0062316E creature
            o = vcall<void*>(vcall<void*>(h, 0x70), 0x0c);
            goto use;
        case 6:                                                                      // 006231E0 item: subobject at +10
            o = vcall<void*>(static_cast<unsigned char*>(vcall<void*>(h, 0x70)) + 0x10, 0x0c);
            goto use;
        case 7: {                                                                    // 006231F8 trigger
            o = vcall<void*>(vcall<void*>(h, 0x70), 0x0c);
            if (o == nullptr) goto scan;
            if (vcall<void*>(o, 0x3c) == nullptr || at<U>(vcall<void*>(o, 0x3c), 0x108) == 0) goto use;  // 00623215..2F
            if (kotor_m15a_object_get_current_animation_0063c060(vcall<void*>(o, 0x3c), nullptr) != 0x27a0) goto scan; // 00623231..43
            goto use;
        }
        case 9: {                                                                    // 00622E64 placeable
            o = vcall<void*>(vcall<void*>(h, 0x70), 0x0c);
            if (o == nullptr) goto scan;
            if (vcall<void*>(o, 0x4c) == nullptr) goto use;                          // 00622E81..8A
            void* plc = vcall<void*>(o, 0x4c);                                       // EBP
            void* sp = kotor_fastcall_00682e00(plc, nullptr);                        // 00622E99..9B
            if (sp == nullptr) goto scan;
            if (at<U>(plc, 0x128) != 0 && at<U>(sp, 0x398) == 0) goto use;           // 00622EA4..B6
            I ignore = 0;                                                            // [esp+20]
            c2da_current_get_int_entry_00413510(at<void*>(at<void*>(kotor_cell_rules_007a3a28, 0xb8), 0x2c), nullptr,
                at<U8>(plc, 0x10c), kotor_rules_cell32_007a225c, &ignore);           // 00622EBC..E9
            if (ignore != 0) {                                                       // 00622EEE..F4
                recovered_client_internal_add_to_hitcheck_ignore_list_005f6340(ci, nullptr, at<U>(plc, 4)); // 00622EF6..01
                continue;                                                            // EDI = 1 -> 00622DF0
            }
            goto scan;                                                               // EDI = [esp+24] = 0
        }
        case 10: {                                                                   // 00623188 door
            o = vcall<void*>(vcall<void*>(h, 0x70), 0x0c);
            if (o == nullptr) goto scan;
            if (vcall<void*>(o, 0x14) == nullptr) goto use;                          // 006231A5..AE
            if (at<U>(vcall<void*>(o, 0x14), 0x138) != 0) goto scan;                 // 006231B4..C3
            if (kotor_fastcall_0061f790(vcall<void*>(o, 0x14), nullptr) == 0) goto use; // 006231C9..D9
            goto scan;
        }
        default:                                                                     // 8 and outside 5..10
            goto scan;
        }
    }
use:                                                                                 // 00623249
    if (o == nullptr) goto scan;
    if (vcall<void*>(o, 0x2c) != nullptr) {                                          // 00623251..5A AsCreature
        const U rider = at<U>(vcall<void*>(o, 0x2c), 0x3dc);                         // 00623260..67
        if (rider != k_none) {                                                       // 0062326F..77
            void* g = kotor_fastcall_005ed580(client_app(), nullptr, at<U>(vcall<void*>(o, 0x2c), 0x3dc)); // 00623279..91
            if (g != nullptr && vcall<void*>(g, 0x0c) != nullptr) o = vcall<void*>(g, 0x0c); // 00623293..AF
        } else if (vcall<void*>(o, 0x2c) != nullptr) {                               // 006232B3..B8
            void* sc = kotor_m15a_creature_get_server_creature_0060fb20(vcall<void*>(o, 0x2c), nullptr);
            bool alive = false;
            if (sc != nullptr && vcall<U>(kotor_m15a_creature_get_server_creature_0060fb20(vcall<void*>(o, 0x2c), nullptr), 0x94) == 0 &&
                recovered_creature_incapacitated_004ef890(kotor_m15a_creature_get_server_creature_0060fb20(vcall<void*>(o, 0x2c), nullptr), nullptr) == 0)
                alive = true;                                                        // 006232C3..FF
            if (!alive && kotor_fastcall_00604bc0(at<void*>(module_of(ci), 0x48), nullptr, at<U>(vcall<void*>(o, 0x2c), 4)) != 0)
                goto scan;                                                           // 00623301..1F corpse
        }
    }
    if (o == nullptr) goto scan;                                                     // 00623325
    if (vcall<void*>(o, 0x34) != nullptr) {                                          // 0062332D..36 AsItem
        if (void* by = kotor_m15e_item_get_equipped_by_00646630(vcall<void*>(o, 0x34), nullptr)) o = vcall<void*>(by, 0x0c); // 00623338..51
    }
    if (o == nullptr) goto scan;                                                     // 00623353
    if (kotor_fastcall_005f1420(ci, nullptr) == nullptr) goto scan;                  // 0062335B..64
    {
        Vector cam;                                                                  // [esp+7C]
        vcall<void>(scene_of(ci), 0x20, &cam);                                       // 0062336A..7B
        Vector p; p.x = 0; p.y = 0; p.z = 0;                                         // [esp+3C] (z never read)
        if (kotor_fastcall_005f1420(ci, nullptr) != nullptr) {                       // 00623390..A0
            copy_vector(&p, static_cast<unsigned char*>(kotor_fastcall_005f1420(ci, nullptr)) + 0x24);
        } else if (at<void*>(ci, 0x270) != nullptr && kotor_input_required::get_character_006346c0(at<void*>(ci, 0x270), nullptr, 0) != nullptr) {
            copy_vector(&p, static_cast<unsigned char*>(kotor_input_required::get_character_006346c0(at<void*>(ci, 0x270), nullptr, 0)) + 0x24); // 006233A2..D7
        }
        Vector q; copy_vector(&q, static_cast<unsigned char*>(vcall<void*>(o, 0x0c)) + 0x24); // 006233DB..14
        const float pcy = static_cast<float>(static_cast<double>(p.y) - cam.y);      // [esp+24]
        const float pcx = static_cast<float>(static_cast<double>(p.x) - cam.x);      // [esp+30]
        const float qcy = static_cast<float>(static_cast<double>(q.y) - cam.y);      // [esp+1C]
        const float qcx = static_cast<float>(static_cast<double>(q.x) - cam.x);      // [esp+9C]
        const float qpy = static_cast<float>(static_cast<double>(q.y) - p.y);        // [esp+20]
        const float qpx = static_cast<float>(static_cast<double>(q.x) - p.x);        // [esp+4C]
        (void)vcall<U>(vcall<void*>(o, 0x0c), 0x138);                                // 0062344E (result unused)
        const double far2 = static_cast<double>(qpx) * qpx + static_cast<double>(qpy) * qpy;   // 00623454..64
        const bool far = far2 > k_far_00007455bc;                                    // 00623466..7A (test ah,41h)
        const double front = static_cast<double>(qcx) * pcx + static_cast<double>(qcy) * pcy;  // 0062347C..8F
        const bool behind = front < 0.0 || front != front;                          // 00623491..9C (test ah,1)
        if (behind || far) at<U>(in_game_main(ci), 0x5cac) = k_none;                // 006234CE..D7
        else if (at<void*>(gui, 8) != nullptr) at<U>(in_game_main(ci), 0x5cac) = k_none; // 006234A2..B8
        else at<U>(in_game_main(ci), 0x5cac) = at<U>(o, 4);                          // 006234BA..C6
        kotor_fastcall_006222f0(ci, nullptr, at<U>(o, 4), &pos, 0);                   // 006234E1..EE
    }
    goto done;
scan:                                                                                // 00622F14: nearest visible object
    if (kotor_fastcall_00641980(module_of(ci), nullptr) != nullptr || kotor_fastcall_005f1420(ci, nullptr) == nullptr) { // 00622F14..33
        kotor_fastcall_006222f0(ci, nullptr, k_none, &pos, 0);                       // 0062364F..5D
        at<U>(in_game_main(ci), 0x5cac) = k_none;                                    // 00623662..6B
        goto done;
    }
    {
        Vector screen{0, 0, 0};                                                      // [esp+58]
        const float mx = static_cast<float>(at<I>(gm, 0));                           // [esp+4C] fild/fstp
        const float my = static_cast<float>(at<I>(gm, 4));                           // [esp+50]
        void* best = nullptr;                                                        // [esp+10]
        float best_d = flt(0x7f7fffffu);                                             // [esp+24] FLT_MAX
        void* pc = kotor_fastcall_005f1420(ci, nullptr);                             // [esp+20]
        for (I i = 0; i < at<I>(ci, 0x2ac); ++i) {                                   // 00622F8C..0062355E
            unsigned char* entry = at<unsigned char*>(ci, 0x2a8) + i * 0xc;
            void* g = kotor_fastcall_005f10f0(ci, nullptr, at<U>(entry, 0));        // 00622FA0..AC
            if (g == nullptr || vcall<void*>(g, 0x0c) == nullptr) continue;          // 00622FB1..C4
            entry = at<unsigned char*>(ci, 0x2a8) + i * 0xc;                         // 00622FCA (reloaded)
            if (entry[4] == 0xff) entry[4] = static_cast<U8>(kotor_fastcall_00617ad0(pc, nullptr, vcall<void*>(g, 0x0c))); // 00622FD0..EC
            if (at<U8>(at<unsigned char*>(ci, 0x2a8) + i * 0xc, 4) == 0) continue;   // 00622FEF..FB
            vcall<void>(vcall<void*>(g, 0x0c), 0x13c, &screen);                      // 00623001..11
            const double sdx = static_cast<double>(screen.x) - mx, sdy = static_cast<double>(screen.y) - my;
            const float d = static_cast<float>((sdx * sdx + sdy * sdy) + static_cast<double>(screen.z) * screen.z); // 00623017..3D ([esp+2C])
            Vector cam;                                                              // [esp+30]
            vcall<void>(scene_of(ci), 0x20, &cam);                                   // 00623047..56
            Vector p; copy_vector(&p, static_cast<unsigned char*>(pc) + 0x24);       // [esp+7C]
            Vector q; copy_vector(&q, static_cast<unsigned char*>(vcall<void*>(g, 0x0c)) + 0x24); // [esp+3C]
            const float pcy = static_cast<float>(static_cast<double>(p.y) - cam.y);  // [esp+18]
            const float pcx = static_cast<float>(static_cast<double>(p.x) - cam.x);  // [esp+A8]
            const float qcy = static_cast<float>(static_cast<double>(q.y) - cam.y);  // [esp+90]
            const float qcx = static_cast<float>(static_cast<double>(q.x) - cam.x);  // [esp+B4]
            const float qpy = static_cast<float>(static_cast<double>(q.y) - p.y);    // [esp+14]
            const float qpx = static_cast<float>(static_cast<double>(q.x) - p.x);    // [esp+9C]
            (void)vcall<U>(vcall<void*>(g, 0x0c), 0x138);                            // 006230F6..FD (result unused)
            const bool far = static_cast<double>(qpx) * qpx + static_cast<double>(qpy) * qpy > k_far_00007455bc; // 00623103..30
            const double front = static_cast<double>(qcx) * pcx + static_cast<double>(qcy) * pcy;               // 00623138..51
            const bool ahead = !(front < 0.0 || front != front);                     // 00623153..69 (EBP)
            const double reach = vcall<double>(vcall<void*>(g, 0x0c), 0x144);        // 006234FA..05 (ST0)
            if (reach < d || reach != reach) continue;                               // 0062350B..14 (test ah,1)
            if (!(static_cast<double>(d) < static_cast<double>(best_d))) continue;   // 00623516..23 (test ah,5; jp)
            if (!ahead || far) continue;                                             // 00623525..2F
            best = vcall<void*>(g, 0x0c); best_d = d;                                // 00623531..40
        }
        U shown;
        if (best != nullptr && at<U>(best, 4) == at<U>(ci, 0x2b4)) {                 // 0062355E..6F
            void* main_if = at<void*>(kotor_input_required::get_in_game_gui_005ed690(client_app(), nullptr), 0x90);
            if (at<I>(main_if, 0x78) > 0 &&
                at<U>(at<void*>(kotor_input_required::get_in_game_gui_005ed690(client_app(), nullptr), 0x90), 0x74) != 0 &&
                at<void*>(gui, 8) == nullptr)                                        // 00623571..B1
                shown = at<U>(best, 4);                                              // 006235B3..C3
            else
                shown = k_none;                                                      // 006235C5..D8
        } else if (at<void*>(gui, 8) != nullptr) {                                   // 006235DA..E5
            shown = k_none;
        } else {
            shown = best ? at<U>(best, 4) : k_none;                                  // 006235FC..0E
        }
        at<U>(in_game_main(ci), 0x5cac) = shown;                                     // 00623617 / 006235CE / 006235F0
        kotor_fastcall_006222f0(ci, nullptr, best ? at<U>(best, 4) : k_none, &pos, 0); // 0062361D..48
    }
done:                                                                                // 00623675: ~CHitInfo (inline)
    if (void* h = at<void*>(hit, 0x38)) exo_array_list_remove_00445080(static_cast<unsigned char*>(h) + 4, nullptr, hit + 0x38); // 00623675..93
    if (void* data = at<void*>(hit, 8)) live_free(data);                             // 00623698..A9
}
}  // extern "C"
