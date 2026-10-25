// Lane-sound: CExoSoundInternal::CExoSoundInternal (005D86E0) and
// CExoSoundInternal::Initialize (005DA8A0) with their small container helpers.
// Object = 0x14C raw bytes (allocation at 005D5BE8); offsets are observed.
#include "e2581310e3-sound_internal.h"
#include "63225f2644-allocator_live.h"
#include <cstring>
#include <new>

namespace {
using kotor_sound_internal::U;
using String = kotor_reconstruction::StartupString;
unsigned char* at(void* p, unsigned off) { return static_cast<unsigned char*>(p) + off; }
U rd(void* p, unsigned off) { U v; std::memcpy(&v, at(p, off), 4); return v; }
void wr(void* p, unsigned off, U v) { std::memcpy(at(p, off), &v, 4); }
void* rp(void* p, unsigned off) { void* v; std::memcpy(&v, at(p, off), 4); return v; }
void wp(void* p, unsigned off, void* v) { std::memcpy(at(p, off), &v, 4); }
unsigned char rb(void* p, unsigned off) { return *at(p, off); }
void wb(void* p, unsigned off, unsigned char v) { *at(p, off) = v; }
void* ptr(U v) { void* p; std::memcpy(&p, &v, 4); return p; }
kotor_list::Internal* list_internal(void* list_object) {         // CExoLinkedList = {internal*}
    return static_cast<kotor_list::Internal*>(rp(list_object, 0));
}
// Original head/data read: MOV EAX,[list]; MOV ECX,[EAX]; data = ECX ? [ECX+8] : 0.
void* first(void* list_object, void** position) {
    void* internal = rp(list_object, 0);
    void* head = rp(internal, 0);
    *position = head;
    return head ? rp(head, 8) : nullptr;
}
const U one_float = 0x3f800000u;                                 // 1.0f bit pattern
}

namespace kotor_sound_internal {

// 005D6C40 CExoLinkedList<T>::CExoLinkedList: internal = new(12) -> 005E9AA0.
// EH FuncInfo 00774614 state 0 frees the allocation (00724030).
void* __fastcall ListConstruct005d6c40(void* list, void*) {
    void* raw = live_operator_new(12);                           // 005D6C57-005D6C5B
    if (!raw) { wp(list, 0, nullptr); return list; }             // 005D6C8E-005D6C94
    struct Guard { void* p; ~Guard() { if (p) live_free(p); } } guard{raw};
    kotor_list::Internal* internal = static_cast<kotor_list::Internal*>(raw)->Construct(); // 005D6C75
    guard.p = nullptr;
    wp(list, 0, internal);                                       // 005D6C7A
    return list;
}

// 005D86C0 UnwindLinkedList: p=[list]; if p {005E9AB0(p); free(p)}.
void __fastcall ListUnwind005d86c0(void* list, void*) {
    kotor_list::Internal* p = list_internal(list);
    if (!p) return;
    base_ini_reconstruction::destroy_list_005e9ab0(p, nullptr);  // 005D86C9
    live_free(p);                                                // 005D86CF
}

// 005E9D40 CExoLinkedListInternal::Contains(data): walk next (+4), compare data (+8).
U __fastcall ListContains005e9d40(void* internal, void*, void* data) {
    for (void* node = rp(internal, 0); node; node = rp(node, 4))   // 005E9D40-005E9D5A
        if (rp(node, 8) == data) return 1;                      // 005E9D50 / 005E9D61
    return 0;                                                    // 005E9D5C
}

// 005E9530 CExoStringList::CExoStringList: five zero DWORDs, EAX=this.
void* __fastcall StringListConstruct005e9530(void* self, void*) {
    for (unsigned off = 0; off < 0x14; off += 4) wr(self, off, 0);
    return self;
}
// 005E95E0 CExoStringList::GetString(i) = ([this])[i].
void* __fastcall StringListGet005e95e0(void* self, void*, U index) {
    return rp(rp(self, 0), index * 4);
}
// 005E9860 CExoStringList::~CExoStringList (unwind action 00724160 only).
void __fastcall StringListDestroy005e9860(void* self, void*) {
    if (!rd(self, 0)) return;                                    // 005E9863
    for (std::int32_t i = 0; i < static_cast<std::int32_t>(rd(self, 4)); ++i) { // 005E986C-005E9892 (count reloaded)
        String* s = static_cast<String*>(rp(rp(self, 0), static_cast<unsigned>(i) * 4));
        if (s) { s->~StartupString(); live_free(s); }           // 005E987E / 005E9884
    }
    live_free(rp(self, 0));                                      // 005E9898 (006FB7B2; 006FA390 is JMP to it)
}
// 0056B6F0 CExoArrayList::~CExoArrayList: free([this]); [this]=0.
// [M47 compose] 0056B6F0 CExoArrayList dtor: single body = existing provider @exo_array_list_destruct_0056b6f0@8 (3d-render scene3d, M45) (definition removed; declaration kept)
void __fastcall ArrayListDestroy0056b6f0(void* self, void*);

// 005D86E0 CExoSoundInternal::CExoSoundInternal. EH FuncInfo 0077478C,
// unwind states 0..9 (00724110..0072417C) reproduced by Unwind below.
void* __fastcall Construct005d86e0(void* self, void*) {
    struct Unwind {
        void* self; int state;
        ~Unwind() {
            if (state < 0) return;
            if (state >= 9) ArrayListDestroy0056b6f0(at(self, 0x108), nullptr);  // 0072417C
            if (state >= 8) ArrayListDestroy0056b6f0(at(self, 0xec), nullptr);   // 0072416E
            if (state >= 7) StringListDestroy005e9860(at(self, 0xd8), nullptr);  // 00724160
            if (state >= 6) reinterpret_cast<String*>(at(self, 0xd0))->~StartupString(); // 00724152
            static const unsigned lists[] = {0x34, 0x38, 0x3c, 0x40, 0x44, 0x48};
            for (int s = state < 5 ? state : 5; s >= 0; --s)    // 00724147..00724110
                ListUnwind005d86c0(at(self, lists[s]), nullptr);
        }
    } unwind{self, -1};
    ListConstruct005d6c40(at(self, 0x34), nullptr); unwind.state = 0; // 005D8702
    ListConstruct005d6c40(at(self, 0x38), nullptr); unwind.state = 1; // 005D8710
    ListConstruct005d6c40(at(self, 0x3c), nullptr); unwind.state = 2; // 005D871D
    ListConstruct005d6c40(at(self, 0x40), nullptr); unwind.state = 3; // 005D872A
    ListConstruct005d6c40(at(self, 0x44), nullptr); unwind.state = 4; // 005D8737
    ListConstruct005d6c40(at(self, 0x48), nullptr);                  // 005D8744
    for (unsigned off = 0x80; off <= 0x94; off += 4) wr(self, off, 0); // 005D8749-005D8767
    unwind.state = 5;                                                // 005D8773
    wr(self, 0x98, 0); wr(self, 0x9c, 0); wr(self, 0xa0, 0);         // 005D8778-005D8784
    ::new (static_cast<void*>(at(self, 0xd0))) String();             // 005D878A (005B3190)
    unwind.state = 6;
    StringListConstruct005e9530(at(self, 0xd8), nullptr);            // 005D879A
    wr(self, 0xf0, 0); wr(self, 0xf4, 0); wr(self, 0xec, 0);         // 005D879F-005D87AB
    wr(self, 0x10c, 0); wr(self, 0x110, 0); wr(self, 0x108, 0);      // 005D87B1-005D87BD
    unwind.state = 9;                                                // 005D87C5
    wr(self, 0x30, 0); wr(self, 0x2c, 0); wr(self, 0xf8, 0); wr(self, 8, 0); // 005D87CA-005D87D6
    wr(self, 0x4c, 0); wb(self, 0x2a, 0); wr(self, 0x58, 0);         // 005D87D9-005D87DF
    kotor_sound_path_required::load_priority_groups_005d7bd0(self, nullptr); // 005D87E2
    wr(self, 0x64, one_float); wr(self, 0x7c, one_float); wr(self, 0x78, one_float); // 005D87EC-005D87F2
    wr(self, 0x74, one_float); wr(self, 0x70, one_float); wr(self, 0x6c, one_float); // 005D87F5-005D87FB
    wr(self, 0x5c, 0); wr(self, 0x54, 0); wr(self, 0x50, 0); wr(self, 0x0c, 0);     // 005D87FE-005D8807
    wr(self, 0x60, one_float); wb(self, 0x29, 0); wb(self, 0x28, 0);              // 005D880A-005D8810
    wr(self, 0x68, 0x3f19999au);                                     // 005D8813 (0.6f)
    wr(self, 0, 1); wr(self, 4, 1); wb(self, 0x10, 0);               // 005D881A-005D882B
    (void)AIL_set_preference(1, 0x40);                               // 005D8827-005D882E
    wr(self, 0x104, 0);                                              // 005D8834
    wr(self, 0x14, one_float); wr(self, 0x18, one_float); wr(self, 0x1c, one_float); // 005D883A-005D8840
    wr(self, 0x20, 0); wr(self, 0x24, one_float);                    // 005D8843-005D8846
    void* base = kotor_menu_10_19::dependencies::base_007a39e0;      // 005D8849
    auto* timer = static_cast<kotor_menu_10_19::TimerDependency*>(rp(base, 4)); // 005D884E
    const kotor_menu_10_19::Wide now = timer->high_resolution_005e9260();      // 005D8851
    wr(self, 0x114, static_cast<U>(kotor_menu_10_19::dependencies::unsigned_divide_006fb030(now, 1000))); // 005D8856-005D8863
    wr(self, 0xa4, 0); wr(self, 0xac, 0); wr(self, 0xb0, 0); wr(self, 0xb4, 0);    // 005D8869-005D887B
    wr(self, 0xb8, 0); wr(self, 0xbc, 0); wr(self, 0xc4, 0); wr(self, 0xc8, 0);    // 005D8881-005D8893
    wr(self, 0xcc, 0); wr(self, 0xa8, 1); wr(self, 0xc0, 3);         // 005D8899-005D88A9
    const U mode0 = *sound_mode29_required::word_0074c5ec;           // 005D88B3
    *sound_pause29_required::word_00832890 = 0;                      // 005D88B9
    wr(self, 0x118, mode0);                                          // 005D88BF
    wr(self, 0x140, 0); wr(self, 0x144, 0); wr(self, 0x148, 0);      // 005D88C9-005D88D5
    // D2 (value-initialization deviation): the original ctor never stores +100h (EAX level of the selected provider;
    // only writer 005DAB64 in Initialize, reached only when a provider matches). When no provider is selected (no Miles
    // device: AIL_quick_startup fails 005DAAC7 / no 3D provider matches 005DAB03..005DABA3), InitializeSoundOptions
    // still reads it through CExoSound::GetEAX 005D6450 (005F257B, 005F2621) and writes it to swkotor.ini 'EAX' and
    // ClientOptions+49h. The original reads indeterminate heap bytes there; reconstructed C++ value-initializes to 0.
    wr(self, 0x100, 0);                                              // D2 label: no original store (005D86E0..005D88EA)
    unwind.state = -1;
    return self;                                                     // 005D88DC
}

// 005DA8A0 CExoSoundInternal::Initialize(n2D, n3D, eax, hardware), RET 10h, EAX 0/1.
U __fastcall Initialize005da8a0(void* self, void*, U voices2d, U voices3d, U eax_mode, U hardware) {
    if (*sound_lifecycle29_required::word_00832894 == 1) return 0;   // 005DA8AE-005DA8DC (== 1 exactly)
    unsigned char local_list[4];                                     // [esp+14h] CExoLinkedList<CExoSoundSourceInternal>
    ListConstruct005d6c40(local_list, nullptr);                      // 005DA8E5
    struct Unwind { unsigned char* list; bool live; ~Unwind() { if (live) ListUnwind005d86c0(list, nullptr); } } // 0077489C state 0 -> 00724220
        unwind{local_list, true};
    if (rd(self, 0xf8) != 0) {                                       // 005DA8EA-005DA8F8 re-initialize
        void* pos; void* stream = first(at(self, 0x44), &pos);       // 005DA8FE-005DA916
        if (pos) {
            do {
                if (sound_streaming29::InternalIsPlaying005dc300(stream, nullptr))  // 005DA922
                    (void)sound_pause29_required::stream_pause_005dc3c0(stream, nullptr); // 005DA92D
                if (void* h = rp(stream, 0)) { AIL_close_stream(h); wp(stream, 0, nullptr); } // 005DA92F-005DA93B
                stream = ptr(unresolved_list_next_005e9b90(rp(self, 0x44), nullptr, &pos)); // 005DA93D-005DA94A
            } while (pos);                                           // 005DA94C
        }
        void* spos; void* source = first(at(self, 0x40), &spos);     // 005DA952-005DA966
        if (spos) {
            for (;;) {
                while (!source) { /* 005DA970: CMP EDI,EBX / JE 005DA970 (original spins) */
                    static volatile int spin; spin = spin; }
                const U next = kotor_sound_path_required::remove_playing_005d78f0(self, nullptr, source); // 005DA977
                if (!sound_lower29::ActorPlaying005db0e0(source, nullptr)) {        // 005DA980-005DA98A
                    if (rd(source, 0x28) != 0) {                                     // 005DA98C
                        if (!ListContains005e9d40(rp(self, 0x48), nullptr, source)) { // 005DA991-005DA996
                            using Deleting = void(__fastcall*)(void*, void*, U);    // vtable slot 0, RET4
                            Deleting del; std::memcpy(&del, rp(source, 0), 4);      // 005DA99B
                            del(source, nullptr, 1);                                // 005DA99D-005DA9A1
                        }
                    } else {
                        kotor_sound_path_required::shutdown_source_005db210(source, nullptr); // 005DA9A7
                    }
                } else {
                    kotor_sound_path_required::store_restart_005db380(source, nullptr);    // 005DA9B0
                    kotor_sound_path_required::shutdown_source_005db210(source, nullptr);  // 005DA9B7
                    ini_addhead::AddHead005e9d70(list_internal(local_list), nullptr, source); // 005DA9BC-005DA9C1
                }
                if (!next) break;                                    // 005DA9C6
                source = ptr(unresolved_list_at_pos_005e9b60(rp(self, 0x40), nullptr, ptr(next))); // 005DA9CA-005DA9D3
            }
        }
        if (rp(self, 0x30)) {                                        // 005DA9D7 3D voices
            for (U i = 0; i < rb(self, 0x29); ++i) {                 // 005DA9DC-005DAA23 (count reloaded)
                if (void* h = rp(rp(self, 0x30), i * 16 + 4)) {
                    AIL_end_3D_sample(h);                            // 005DA9FC
                    AIL_release_3D_sample_handle(rp(rp(self, 0x30), i * 16 + 4)); // 005DAA06
                    wp(rp(self, 0x30), i * 16 + 4, nullptr);         // 005DAA0F
                }
            }
            live_free(rp(self, 0x30)); wp(self, 0x30, nullptr);      // 005DAA25-005DAA31
        }
        if (rp(self, 0x2c)) {                                        // 005DAA34 2D voices
            for (U i = 0; i < rb(self, 0x28); ++i) {                 // 005DAA3C-005DAA83
                if (void* h = rp(rp(self, 0x2c), i * 16 + 4)) {
                    AIL_end_sample(h);                               // 005DAA5C
                    AIL_release_sample_handle(rp(rp(self, 0x2c), i * 16 + 4)); // 005DAA66
                    wp(rp(self, 0x2c), i * 16 + 4, nullptr);
                }
            }
            live_free(rp(self, 0x2c)); wp(self, 0x2c, nullptr);      // 005DAA85-005DAA91
        }
        AIL_close_3D_provider(rp(self, 0xf8));                       // 005DAA94-005DAA9B
        wr(self, 0xf8, 0);                                           // 005DAAA1
    } else {
        (void)AIL_set_redist_directory("Miles");                     // 005DAAA9-005DAAAE (0074D2DC)
        if (AIL_quick_startup(1, 0, 44100, 16, 2) == 0)              // 005DAAB4-005DAAC7
            *sound_lifecycle29_required::word_00832894 = 1;          // 005DAAC9
        kotor_sound_path_required::enumerate_providers_005da6d0(self, nullptr); // 005DAAD1
    }
    String* provider_name = reinterpret_cast<String*>(at(self, 0xd0)); // 005DAAD6
    wr(self, 0xf8, 0); wr(self, 8, 0);                               // 005DAAE1-005DAAE7
    GuiAssign005e5140(provider_name, nullptr, base_ini_reconstruction::warning_empty_0073d71c); // 005DAAEE
    U want_hw = hardware;                                            // 005DAAF3 EBP
    U want_eax = want_hw ? eax_mode : 0;                             // 005DAAF7-005DAAFF EBX
    if (rd(self, 0xf8) == 0) {                                       // 005DAB03
        do {
            void* table = rp(self, 0x108);                           // 005DAB1D (loaded once per pass)
            for (std::int32_t i = 0; i < static_cast<std::int32_t>(rd(self, 0x10c)); ++i) { // 005DAB11-005DAB82
                void* entry = rp(table, static_cast<unsigned>(i) * 4);
                if (rd(entry, 8) != want_hw || rd(entry, 0xc) != want_eax) continue; // 005DAB26-005DAB2E
                const U index = rd(entry, 0);                        // 005DAB30
                wr(self, 8, 1);                                      // 005DAB38
                wr(self, 0xf8, rd(rp(self, 0xec), index * 4));       // 005DAB32-005DAB42
                String* name = static_cast<String*>(StringListGet005e95e0(at(self, 0xd8), nullptr, index)); // 005DAB4F
                provider_name->Assign005e5c50(name);                 // 005DAB59
                table = rp(self, 0x108);                             // 005DAB5E
                wr(self, 0x100, want_eax); wr(self, 0xfc, want_hw);  // 005DAB64-005DAB6A
                wr(self, 0x60, rd(rp(table, static_cast<unsigned>(i) * 4), 4)); // 005DAB70-005DAB76
            }
            if (rd(self, 0xf8) != 0) break;                          // 005DAB84-005DAB8C
            if (static_cast<std::int32_t>(want_eax) > 0) --want_eax; // 005DAB8E-005DAB93
            else if (want_hw == 0) break;                            // 005DAB95-005DAB97
            else want_hw = 0;                                        // 005DAB99
        } while (rd(self, 0xf8) == 0);                               // 005DAB9B-005DABA3
    }
    bool ok = false;
    void* provider = rp(self, 0xf8);                                 // 005DABA9
    if (provider && AIL_open_3D_provider(provider) == 0) {           // 005DABB1-005DABC0
        U n3 = voices3d & 0xff;                                      // 005DABC6
        wb(self, 0x29, static_cast<unsigned char>(n3));
        if (n3 >= 0x40) n3 = 0x40;                                   // 005DABCD-005DABD4
        wb(self, 0x29, static_cast<unsigned char>(n3));
        if (n3 <= 0x10) n3 = 0x10;                                   // 005DABDC-005DABE3
        wb(self, 0x29, static_cast<unsigned char>(n3));              // 005DABEF
        wp(self, 0x30, live_operator_new((n3 & 0xff) << 4));         // 005DABEB-005DABF7
        U n2 = voices2d & 0xff;                                      // 005DABFA
        wb(self, 0x28, static_cast<unsigned char>(n2));
        if (n2 >= 0x40) n2 = 0x40;                                   // 005DAC04-005DAC0B
        wb(self, 0x28, static_cast<unsigned char>(n2));
        if (n2 <= 0x18) n2 = 0x18;                                   // 005DAC13-005DAC1A
        wb(self, 0x28, static_cast<unsigned char>(n2));              // 005DAC1F
        void* voices2 = live_operator_new((n2 & 0xff) << 4);         // 005DAC22-005DAC29
        void* digital = nullptr;                                     // [esp+1Ch]
        wp(self, 0x2c, voices2);                                     // 005DAC3A
        AIL_quick_handles(&digital, nullptr, nullptr);               // 005DAC31-005DAC3D
        std::int32_t i = 0, j = 0;                                   // EBX / [esp+34h]
        for (;;) {                                                   // 005DAC50
            unsigned char a = rb(self, 0x29), c = rb(self, 0x28);
            if (a > c) c = a;                                        // 005DAC56-005DAC5A
            if (i >= static_cast<std::int32_t>(c)) break;            // 005DAC5F-005DAC61
            if (j < static_cast<std::int32_t>(a)) {                  // 005DAC67-005DAC70
                void* h = AIL_allocate_3D_sample_handle(rp(self, 0xf8)); // 005DAC72-005DAC79
                void* arr = rp(self, 0x30);
                wp(arr, i * 16 + 4, h); wr(arr, i * 16, 0); wr(arr, i * 16 + 8, 0); wr(arr, i * 16 + 0xc, 0); // 005DAC7F-005DAC96
                if (rd(rp(self, 0x30), i * 16 + 4) == 0) wb(self, 0x29, static_cast<unsigned char>(j)); // 005DAC9D-005DACA7
            }
            if (i < static_cast<std::int32_t>(rb(self, 0x28))) {     // 005DACAA-005DACB0
                void* h = AIL_allocate_sample_handle(digital);       // 005DACB2-005DACB7
                void* arr = rp(self, 0x2c);
                wp(arr, i * 16 + 4, h); wr(arr, i * 16, 0); wr(arr, i * 16 + 8, 0); wr(arr, i * 16 + 0xc, 0); // 005DACBD-005DACD4
                void* got = rp(rp(self, 0x2c), i * 16 + 4);          // 005DACD8-005DACDB
                if (got) AIL_init_sample(got);                       // 005DACE3-005DACE4
                else wb(self, 0x28, static_cast<unsigned char>(i));  // 005DACFC
            }
            ++i; ++j;                                                // 005DACEE-005DACF3 / 005DAD03-005DAD08
        }
        const U count2 = rb(self, 0x28);                             // 005DAD11
        for (U k = count2 >> 1; k < rb(self, 0x28); ++k) {           // 005DAD15-005DAD53 (count reloaded; [28] kept)
            if (void* h = rp(rp(self, 0x2c), k * 16 + 4)) {
                AIL_release_sample_handle(h);                        // 005DAD3C
                wp(rp(self, 0x2c), k * 16 + 4, nullptr);             // 005DAD41
            }
        }
        if (void* p = rp(self, 0xf8)) AIL_set_3D_rolloff_factor(p, 1.0f); // 005DAD57-005DAD67
        kotor_sound_path_required::provider_preferences_005d66d0(self, nullptr, 0); // 005DAD6D-005DAD70
        kotor_list::Internal* restart = list_internal(local_list);   // 005DAD75
        while (restart->count != 0) {                                // 005DAD79 / 005DAD92
            void* s = base_ini_reconstruction::remove_head_005e9c80(restart, nullptr); // 005DAD82
            kotor_sound_path_required::source_play_005db4d0(s, nullptr, 1, 1);        // 005DAD87-005DAD8D
        }
        void* pos; void* stream = first(at(self, 0x44), &pos);       // 005DAD97-005DADAF
        if (pos) {
            do {
                sound_stream_play29_required::open_005dbd30(stream, nullptr);          // 005DADB5
                (void)sound_pause29_required::stream_resume_005dcdb0(stream, nullptr); // 005DADBC
                stream = ptr(unresolved_list_next_005e9b90(rp(self, 0x44), nullptr, &pos)); // 005DADC1-005DADCE
            } while (pos);                                           // 005DADD0
        }
        unwind.live = false;                                         // 005DADD8 state -1
        base_ini_reconstruction::destroy_list_005e9ab0(restart, nullptr); // 005DADE0 (no null test, as observed)
        live_free(restart);                                          // 005DADE6
        ok = true;                                                   // 005DADEE
    } else {
        AIL_quick_shutdown();                                        // 005DADF5
        wr(self, 0xf8, 0); wr(self, 8, 0);                           // 005DAE06-005DAE0C
        GuiAssign005e5140(provider_name, nullptr, base_ini_reconstruction::warning_empty_0073d71c); // 005DAE01-005DAE0F
        unwind.live = false;                                         // 005DAE1A
        if (kotor_list::Internal* l = list_internal(local_list)) {   // 005DAE14-005DAE22
            base_ini_reconstruction::destroy_list_005e9ab0(l, nullptr); // 005DAE26
            live_free(l);                                            // 005DAE2C
        }
    }
    return ok ? 1u : 0u;
}
}

// The entry the bridged CExoLinkedList<CExoSoundSourceInternal>::Contains (005D6CC0) forwards to.
extern "C" std::uint32_t __fastcall kotor_fastcall_005e9d40(void* internal, void*, void* data) {
    return kotor_sound_internal::ListContains005e9d40(internal, nullptr, data);
}
