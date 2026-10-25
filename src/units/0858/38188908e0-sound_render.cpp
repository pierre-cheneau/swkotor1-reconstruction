// Lane-sound: CExoSoundInternal::Render (005D9590-005DA6C8) and
// CExoSoundInternal::SetMusicVolume (005D6940). Render's tiny accessors
// (005D5FE0 005D5FF0 005D6000 005D6010 005D6030 005D6050 005D6070 005D6080
// 005D64F0 005D6500 005D6510 005D6520 005D6530 005D6670 005D6680 005D6690
// 005D66A0 005D66B0 005D66C0 005D69D0 005D69F0 00675DD0 00710E50 005A4C50
// 00404AE0 and list thunks 005D6CB0..005D6D20) are single loads/stores and
// are expressed as those loads/stores at each call site (cited).
// x87 extended intermediates are computed in double and rounded to float at
// every original FSTP DWORD (documented deviation: extended vs double).
#include "e2581310e3-sound_internal.h"
#include <cstring>

extern "C" std::uint32_t __stdcall unresolved_GetTickCount_005ec200();          // world-timer-os-imports.obj
extern "C" int __fastcall kotor_fastcall_005d6cc0(void*, void*, void*);       // CExoLinkedList<CExoSoundSourceInternal>::Contains: matched (src/bridge)
namespace add_key_lower { void __fastcall Remove005e9c10(kotor_list::Internal*, void*, void*); } // remove_position.obj
namespace sound_pause29_required { std::uint32_t __stdcall zero_0063e7f0(); }    // joined to 0063E7F0 body
namespace menu_resources { struct Ref; }
namespace sound_stream_play29 {
std::uint32_t __fastcall Play005dca30(void*, void*, menu_resources::Ref*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
}
namespace sound_stream_play29_required {
void __fastcall close_existing_005dc380(void*, void*);          // 005DC380 Stop
void __fastcall refresh_005dc560(void*, void*);                 // 005DC560 Update3DStream
void __fastcall attach_005d7ba0(void*, void*, void*);           // 005D7BA0 AddToNonPlayingStreamingLoopingList
std::uint32_t __fastcall attachment_query_005dc4d0(void*, void*); // 005DC4D0 GetWithinRangeOfListener
}
namespace sound_lower29_required { void __fastcall volume_005dc930(void*, void*, std::uint32_t, std::uint32_t, std::uint32_t); }
namespace kotor_sound_path_required {
// List-maintenance lowers whose EAX (next position) Render consumes.
std::uint32_t __fastcall remove_streaming_005d7ab0(void*, void*, void*);          // RET 4
std::uint32_t __fastcall remove_nonplaying_streaming_005d7a00(void*, void*, void*); // RET 4
std::uint32_t __fastcall remove_delayed_005d7a70(void*, void*, void*);           // RET 4
void __fastcall add_nonplaying_looping_005d7b80(void*, void*, void*);            // RET 4
std::uint32_t __fastcall source_in_range_005db050(void*, void*);                 // RET 0
void __fastcall source_set_volume_005db800(void*, void*, std::uint32_t, std::uint32_t, std::uint32_t); // RET 0Ch
void __fastcall apply_new_volume_005d8110(void*, void*, std::uint32_t);          // RET 4
void __fastcall listener_position_005d6600(void*, void*, void*);                 // RET 4
void __fastcall listener_orientation_005d6540(void*, void*, void*, void*);       // RET 8
// Boundary: OcclusionRender reaches shared world LOS (0050C330) - not authored here.
void __fastcall occlusion_render_005d88f0(void*, void*, std::uint32_t);          // RET 4
}

namespace {
using kotor_sound_internal::U;
unsigned char* at(void* p, unsigned off) { return static_cast<unsigned char*>(p) + off; }
U rd(void* p, unsigned off) { U v; std::memcpy(&v, at(p, off), 4); return v; }
void wr(void* p, unsigned off, U v) { std::memcpy(at(p, off), &v, 4); }
void* rp(void* p, unsigned off) { void* v; std::memcpy(&v, at(p, off), 4); return v; }
float rf(void* p, unsigned off) { float v; std::memcpy(&v, at(p, off), 4); return v; }
void wf(void* p, unsigned off, float v) { std::memcpy(at(p, off), &v, 4); }
unsigned char rb(void* p, unsigned off) { return *at(p, off); }
U fbits(float f) { U v; std::memcpy(&v, &f, 4); return v; }
U pbits(void* p) { U v; std::memcpy(&v, &p, 4); return v; }
void* bp(U v) { void* p; std::memcpy(&p, &v, 4); return p; }
// CExoLinkedList<T> member at self+off holds the internal pointer.
kotor_list::Internal* L(void* self, unsigned off) { return static_cast<kotor_list::Internal*>(rp(self, off)); }
void* head(void* self, unsigned off) { return rp(L(self, off), 0); }                      // 005D6CE0
void* at_head(void* self, unsigned off) { void* h = head(self, off); return h ? rp(h, 8) : nullptr; } // 005D6CD0
void* next(void* self, unsigned off, void** pos) { return bp(unresolved_list_next_005e9b90(L(self, off), nullptr, pos)); } // 005D6D20
void* at_pos(void* self, unsigned off, void* pos) { return bp(unresolved_list_at_pos_005e9b60(L(self, off), nullptr, pos)); } // 005D6D10
U elapsed(U now, U start) { return now >= start ? now - start : (0xffffffffu - start) + now; } // 005D9F8A-005D9FA6
void destroy_source(void* source) {                                // vtable slot 0, push 1 (scalar deleting dtor)
    using Deleting = void(__fastcall*)(void*, void*, U);
    Deleting del; std::memcpy(&del, rp(source, 0), 4); del(source, nullptr, 1);
}
U high_ms_low(void) {                                              // 005D9A2F-005D9A43: low DWORD / 1000 (32-bit DIV)
    void* base = kotor_menu_10_19::dependencies::base_007a39e0;
    auto* t = static_cast<kotor_menu_10_19::TimerDependency*>(rp(base, 4));
    return static_cast<U>(t->high_resolution_005e9260()) / 1000u;
}
// Fade factor clamp(el/den, 0, 1) as at 005D9A71-005D9BC5 / 005D9D2D-005D9E81.
float fade(U el, U den) {
    const double r = static_cast<double>(static_cast<unsigned long long>(el)) / static_cast<double>(static_cast<unsigned long long>(den));
    float low = (r >= 0.0 || r != r) ? static_cast<float>(r) : 0.0f;   // fcomp 0.0, JP
    if (1.0f >= low || low != low) {                                     // fcomp [0073D764]=1.0 vs low, JP
        return (r >= 0.0 || r != r) ? static_cast<float>(r) : 0.0f;
    }
    return 1.0f;
}
}

namespace kotor_sound_internal {
// 005D6940 CExoSoundInternal::SetMusicVolume(v, keep_base, apply_group), RET 0Ch.
void __fastcall SetMusicVolume005d6940(void* self, void*, float v, U keep_base, U apply_group) {
    if (keep_base == 0) wf(self, 0x78, v);                          // 005D6940-005D694C
    wf(self, 0x6c, v);                                              // 005D6958
    if (apply_group != 1) return;                                   // 005D694F / 005D695B
    const U group_bits = rd(self, 0x4c) + 0x30u;                    // 005D695E-005D6961
    if (group_bits == 0) return;                                    // 005D6964 (JE on ADD result)
    unsigned char* group = static_cast<unsigned char*>(bp(group_bits));
    const double x = static_cast<double>(v) * 127.0;                // 005D6966 fmul [0073EDDC]=127.0
    double chosen;
    if (!(0.0 <= x || x != x)) chosen = 0.0;                        // 005D6970-005D697D, 005D69A6-005D69B7
    else if (127.0 >= x || x != x) chosen = (0.0 <= x || x != x) ? x : 0.0; // 005D697F-005D6990 -> 005D69A6
    else chosen = 127.0;                                            // 005D6992-005D6994
    const long long t = (chosen != chosen) ? static_cast<long long>(0x8000000000000000ull) : static_cast<long long>(chosen); // __ftol2 006FAE8C
    group[7] = static_cast<unsigned char>(t);                       // 005D699F / 005D69C2
}

// 005D9590 CExoSoundInternal::Render, RET 0.
void __fastcall Render005d9590(void* self, void*) {
    void* base = kotor_menu_10_19::dependencies::base_007a39e0;    // 005D95A2
    auto* timer = static_cast<kotor_menu_10_19::TimerDependency*>(rp(base, 4));
    const U now = static_cast<U>(kotor_menu_10_19::dependencies::unsigned_divide_006fb030(timer->high_resolution_005e9260(), 1000)); // 005D95AA-005D95BD
    const U delta = now - rd(self, 0x114);                          // 005D95C0-005D95CF
    wr(self, 0x114, now);                                           // 005D95DB
    const double dt = static_cast<double>(static_cast<unsigned long long>(delta)) / 1000.0; // 005D95F4-005D95FA ([0073D6FC]=1000.0)
    const float step = (dt >= 0.1f || dt != dt) ? 0.1f : static_cast<float>(dt); // 005D9600-005D9644 (0.1f bits 3DCCCCCD)
    const U environment = rd(self, 0x20);                           // 005D964D (005D66C0)
    const float scale = rf(self, 0x24);                             // 005D965B (005D66B0)
    if (rb(self, 0x10) != (environment & 0xff))                     // 005D9663-005D9670
        kotor_sound_path_required::provider_preferences_005d66d0(self, nullptr,
            (pbits(self) & 0xffffff00u) | (environment & 0xffu));    // 005D9672-005D967C (MOV CL over ECX=self)
    if (!(scale == rf(self, 0x14))) {                               // 005D9681-005D9692
        const double moved = (static_cast<double>(scale) - rf(self, 0x18)) * (static_cast<double>(step) / 1.0) + rf(self, 0x14);
        wf(self, 0x14, static_cast<float>(moved));                  // 005D9698-005D96BE
        if (scale > rf(self, 0x18)) {                               // 005D96C1-005D96D2 (C0|C3 clear; NaN -> skip)
            const float cur = rf(self, 0x14);
            wf(self, 0x14, (cur > scale) ? scale : cur);            // 005D96D4-005D970D (C0|C3 or unordered -> cur)
        }
        if (scale < rf(self, 0x18)) {                               // 005D9710-005D9721 (JP: >= or unordered skips)
            const float cur = rf(self, 0x14);                       // 005D9723-005D975C
            wf(self, 0x14, (cur >= scale || cur != cur || scale != scale) ? cur : scale);
        }
    } else {
        wr(self, 0x18, rd(self, 0x14));                             // 005D9761-005D9770
    }
    // lane-m1-sound C2: fld [1C]; fcomp scale; test ah,44h; jnp 005D9798 skips the store when EQUAL: the ramp base [18]
    // is re-seeded only when the desired ambient scale changed (or compares unordered). M47..M54 inverted this.
    if (!(rf(self, 0x1c) == scale)) wr(self, 0x18, rd(self, 0x14)); // 005D9773-005D9795
    wf(self, 0x1c, scale);                                          // 005D9798-005D97A1

    // Streaming list (+44): remove the first non-playing stream when 00832890==0.
    void* pos = head(self, 0x44); void* s = at_head(self, 0x44);    // 005D97AB-005D97CA
    while (pos) {                                                   // 005D97CD
        if (!sound_streaming29::InternalIsPlaying005dc300(s, nullptr) && *sound_pause29_required::word_00832890 == 0) { // 005D97D3-005D97E6
            (void)kotor_sound_path_required::remove_streaming_005d7ab0(self, nullptr, s); // 005D97E8-005D97F2
            break;
        }
        s = next(self, 0x44, &pos);                                 // 005D97F9-005D980B
    }
    pos = head(self, 0x44); s = at_head(self, 0x44);                // 005D9810-005D982F
    while (pos) {                                                   // 005D9832
        if (rd(s, 0x58) && rd(s, 4) && !sound_stream_play29_required::attachment_query_005dc4d0(s, nullptr)) { // 005D983C-005D985E
            (void)next(self, 0x44, &pos);                           // 005D9860-005D986D (EAX discarded)
            sound_stream_play29_required::close_existing_005dc380(s, nullptr);   // 005D9875
            sound_stream_play29_required::attach_005d7ba0(self, nullptr, s);     // 005D987A-005D9884
            if (pos) s = at_pos(self, 0x44, pos);                   // 005D9889-005D98A1
            continue;
        }
        if (rd(s, 0x58)) sound_stream_play29_required::refresh_005dc560(s, nullptr); // 005D98A6-005D98B5
        s = next(self, 0x44, &pos);                                 // 005D98BA-005D98CC
    }
    // Non-playing streaming looping list (+3C): restart within voice/priority budget.
    pos = head(self, 0x3c); s = at_head(self, 0x3c);                // 005D98D4-005D98F3
    while (pos) {                                                   // 005D98FD
        unsigned char* group = static_cast<unsigned char*>(rp(self, 0x4c)) + static_cast<U>(rb(s, 8)) * 24u; // 005D9907-005D991B (005D5FF0, 005D6080)
        const U count = L(self, 0x44)->count;                       // 005D991E-005D992C (005D6D00)
        const std::int32_t n2 = rb(self, 0x28);                     // 005D9934-005D9939
        const std::int32_t limit = n2 - n2 / 2;                     // 005D9942-005D994F (CDQ/SUB/SAR)
        bool play;
        if (count >= static_cast<U>(limit)) play = false;           // 005D9951-005D9953 (JAE)
        else if (!group) play = true;                               // 005D9955-005D9959
        else play = static_cast<std::int32_t>(group[5]) < static_cast<std::int32_t>(group[4]); // 005D995B-005D9973
        if (!play) { s = next(self, 0x3c, &pos); continue; }        // 005D9975-005D998A
        if (rd(s, 4) && sound_stream_play29_required::attachment_query_005dc4d0(s, nullptr)) { // 005D998F-005D99A5
            sound_stream_play29_required::refresh_005dc560(s, nullptr);          // 005D99AA
            const U format = *reinterpret_cast<std::uint16_t*>(at(s, 0x44));    // 005D99B8 (005D6680, MOVZX word)
            (void)sound_stream_play29::Play005dca30(s, nullptr, reinterpret_cast<menu_resources::Ref*>(at(s, 9)), format, 0, 0, 0); // 005D99AF-005D99CA (005D6670 = s+9)
            pos = bp(kotor_sound_path_required::remove_nonplaying_streaming_005d7a00(self, nullptr, s)); // 005D99CF-005D99DE
            if (pos) s = at_pos(self, 0x3c, pos);                   // 005D99E1-005D99F9
            continue;
        }
        s = next(self, 0x3c, &pos);                                 // 005D9A01-005D9A13
    }
    if (rd(self, 0xf8) == 0) return;                                // 005D9A1B-005D9A2A (skips OcclusionRender)
    U now2 = high_ms_low();                                         // 005D9A2F-005D9A45
    if (rd(self, 0x54) != 0) {                                      // 005D9A48-005D9A52 fade-out
        const U el = now2 - rd(self, 0x50);                         // 005D9A58-005D9A64
        if (el != 0) {                                              // 005D9A67-005D9A6B (JBE)
            const float f = fade(el, rd(self, 0x54));               // [ebp-70h]
            const float dialog = rf(self, 0x64), sfx = rf(self, 0x7c), music = rf(self, 0x78); // 005D9BCE-005D9BFB
            wf(self, 0x74, static_cast<float>(static_cast<double>(dialog) - static_cast<double>(f) * dialog)); // 005D9BFE-005D9C13 (005D69F0, flag 1)
            wf(self, 0x70, static_cast<float>(static_cast<double>(sfx) - static_cast<double>(f) * sfx));       // 005D9C18-005D9C2D (005D69D0, flag 1)
            SetMusicVolume005d6940(self, nullptr, static_cast<float>(static_cast<double>(music) - static_cast<double>(f) * music), 1, 0); // 005D9C32-005D9C49
            kotor_sound_path_required::apply_new_volume_005d8110(self, nullptr, 1); // 005D9C4E-005D9C56
            if (fbits(f) == 0x3f800000u) {                          // 005D9C5B-005D9C62
                wr(self, 0x54, 0); wr(self, 0x50, 0);               // 005D9C68-005D9C7B
                pos = head(self, 0x44); s = at_head(self, 0x44);    // 005D9C82-005D9CA1
                while (pos) {                                       // 005D9CA4
                    if (rd(s, 0x2c)) {                              // 005D9CAA-005D9CB4 (00710E50)
                        pos = bp(kotor_sound_path_required::remove_streaming_005d7ab0(self, nullptr, s)); // 005D9CB6-005D9CC5
                        sound_stream_play29_required::close_existing_005dc380(s, nullptr); // 005D9CCB
                        if (pos) s = at_pos(self, 0x44, pos);       // 005D9CD0-005D9CE8
                    } else {
                        s = next(self, 0x44, &pos);                 // 005D9CED-005D9CFF
                    }
                }
            }
        }
    }
    if (rd(self, 0x5c) != 0) {                                      // 005D9D04-005D9D0E fade-in
        const U el = now2 - rd(self, 0x50);                         // 005D9D14-005D9D20
        if (el != 0) {
            const float f = fade(el, rd(self, 0x5c));               // [ebp-80h]
            const float dialog = rf(self, 0x64), sfx = rf(self, 0x7c), music = rf(self, 0x78); // 005D9E8A-005D9EBA
            wf(self, 0x74, static_cast<float>(static_cast<double>(f) * dialog)); // 005D9EBD-005D9ED2
            wf(self, 0x70, static_cast<float>(static_cast<double>(f) * sfx));    // 005D9ED7-005D9EE9
            SetMusicVolume005d6940(self, nullptr, static_cast<float>(static_cast<double>(f) * music), 1, 0); // 005D9EEE-005D9F02
            kotor_sound_path_required::apply_new_volume_005d8110(self, nullptr, 1); // 005D9F07-005D9F0F
            if (fbits(f) == 0x3f800000u) { wr(self, 0x5c, 0); wr(self, 0x50, 0); } // 005D9F14-005D9F30
        }
    }
    now2 = unresolved_GetTickCount_005ec200();                      // 005D9F37-005D9F45 (005E9250 -> 005EC200)
    // Delayed one-shot list (+34).
    pos = head(self, 0x34); void* src = at_head(self, 0x34);        // 005D9F48-005D9F67
    while (pos) {                                                   // 005D9F6A
        const U delay = rd(src, 0x5c), start = rd(src, 0x64);       // 005D9F74-005D9F87 (005D6500, 005D6510)
        const U el = elapsed(now2, start);
        if (el == 0 || el < delay) { src = next(self, 0x34, &pos); continue; } // 005D9FA9-005D9FB9, 005DA04F/005DA066
        pos = bp(kotor_sound_path_required::remove_delayed_005d7a70(self, nullptr, src)); // 005D9FBF-005D9FCE
        kotor_sound_path_required::source_play_005db4d0(src, nullptr, 0, 0); // 005D9FD1-005D9FD8
        if (rd(src, 0x2c)) { if (src) destroy_source(src); src = nullptr; } // 005D9FDD-005DA02B
        if (pos) src = at_pos(self, 0x40, pos);                     // 005DA032-005DA04A (list +40, as observed)
    }
    if (rd(self, 0x0c) == 0) {                                      // 005DA080-005DA08A stream fades (+44)
        pos = head(self, 0x44); s = at_head(self, 0x44);            // 005DA090-005DA0AF
        while (pos) {                                               // 005DA0B2
            if (!(rf(self, 0x14) == rf(self, 0x18)))                // 005DA0BC-005DA0D3
                sound_lower29_required::volume_005dc930(s, nullptr,
                    (pbits(self) & 0xffffff00u) | rb(s, 0x46), 0, rd(self, 0x14)); // 005DA0D5-005DA0ED (MOV AL over EAX=self)
            const U span = rd(s, 0x2c), start = rd(s, 0x34);        // 005DA0F2-005DA105
            if (span == 0) { s = next(self, 0x44, &pos); continue; } // 005DA108 / 005DA233
            const U el = elapsed(now2, start);                      // 005DA112-005DA12E
            if (el == 0) { s = next(self, 0x44, &pos); continue; }  // 005DA131 / 005DA21C
            if (el >= span) {                                       // 005DA13B-005DA141
                pos = bp(kotor_sound_path_required::remove_streaming_005d7ab0(self, nullptr, s)); // 005DA143-005DA152
                sound_stream_play29_required::close_existing_005dc380(s, nullptr); // 005DA158
                if (pos) s = at_pos(self, 0x44, pos);               // 005DA15D-005DA175
                continue;
            }
            const std::int32_t b = rb(s, 0x46);                     // 005DA17D-005DA185
            const double ratio = static_cast<double>(static_cast<unsigned long long>(el)) / static_cast<double>(static_cast<unsigned long long>(span)); // 005DA188-005DA1BC (FSTP QWORD)
            const double product = static_cast<double>(static_cast<std::int32_t>(rb(s, 0x46))) * ratio; // 005DA1C2-005DA1D9
            const U t = static_cast<U>(static_cast<long long>(product)); // 005DA1DF __ftol2 (truncation; EAX = low DWORD)
            const unsigned char v = static_cast<unsigned char>(b - static_cast<std::int32_t>(static_cast<signed char>(t & 0xffu))); // 005DA1E4-005DA1E9
            sound_lower29_required::volume_005dc930(s, nullptr, (t & 0xffffff00u) | v, 1, 0x3f800000u); // 005DA1EF-005DA200
            s = next(self, 0x44, &pos);                             // 005DA205-005DA217
        }
    }
    // Playing source list (+40).
    pos = head(self, 0x40); src = at_head(self, 0x40);              // 005DA24D-005DA26C
    while (pos) {                                                   // 005DA26F
        if (!sound_lower29::ActorPlaying005db0e0(src, nullptr)) {   // 005DA279-005DA283
            pos = bp(kotor_sound_path_required::remove_playing_005d78f0(self, nullptr, src)); // 005DA289-005DA298
            if (rd(src, 0x28)) {                                    // 005DA29B-005DA2A5 (005D64F0)
                if (!kotor_fastcall_005d6cc0(static_cast<char*>(self) + 0x48, nullptr, src) && src) destroy_source(src); // 005DA2A7-005DA2F5
                src = nullptr;                                      // 005DA2FF
            } else if (sound_pause29_required::zero_0063e7f0() == 0) { // 005DA308-005DA312 (ECX=src ignored)
                kotor_sound_path_required::shutdown_source_005db210(src, nullptr); // 005DA317
            }
            if (pos) src = at_pos(self, 0x40, pos);                 // 005DA31C-005DA334
            continue;
        }
        if (!kotor_sound_path_required::source_in_range_005db050(src, nullptr)) { // 005DA33C-005DA346
            pos = bp(kotor_sound_path_required::remove_playing_005d78f0(self, nullptr, src)); // 005DA34C-005DA35B
            if (rd(src, 0x28)) {                                    // 005DA35E-005DA368
                if (sound_lower29::ActorPlaying005db0e0(src, nullptr)) { // 005DA36E-005DA378
                    if (!kotor_fastcall_005d6cc0(static_cast<char*>(self) + 0x48, nullptr, src)) // 005DA37A-005DA38E
                        ini_addhead::AddHead005e9d70(L(self, 0x48), nullptr, src); // 005DA390-005DA39D (005D6CB0)
                } else if (!kotor_fastcall_005d6cc0(static_cast<char*>(self) + 0x48, nullptr, src) && src) { // 005DA3A4-005DA3D6
                    destroy_source(src);                            // 005DA3D8-005DA3E8
                }
                src = nullptr;                                      // 005DA3FC
            } else {
                kotor_sound_path_required::shutdown_source_005db210(src, nullptr); // 005DA405-005DA408
                if (rd(src, 0x24))                                  // 005DA40D-005DA417 (005A4C50)
                    kotor_sound_path_required::add_nonplaying_looping_005d7b80(self, nullptr, src); // 005DA419-005DA423
            }
            if (pos) src = at_pos(self, 0x40, pos);                 // 005DA428-005DA440
            continue;
        }
        if (!(rf(self, 0x14) == rf(self, 0x18)))                    // 005DA445-005DA45C
            kotor_sound_path_required::source_set_volume_005db800(src, nullptr,
                (rd(self, 0x14) & 0xffffff00u) | rb(src, 0x85), 0, rd(self, 0x14)); // 005DA45E-005DA476 (MOV AL over EAX=[self+14])
        src = next(self, 0x40, &pos);                               // 005DA47B-005DA48D
    }
    // Non-playing looping list (+38): restart.
    pos = head(self, 0x38); src = at_head(self, 0x38);              // 005DA495-005DA4B4
    while (pos) {                                                   // 005DA4B7
        (void)next(self, 0x38, &pos);                               // 005DA4BD-005DA4CA (EAX discarded)
        kotor_sound_path_required::source_play_005db4d0(src, nullptr, 1, 0); // 005DA4CF-005DA4D6
        if (pos) src = at_pos(self, 0x38, pos);                     // 005DA4DB-005DA4F3
    }
    // Listener: snapshot +98/+80/+8C, then zero them via Vector(0,0,0) (00404AE0).
    unsigned char position[12], up[12], forward[12];
    std::memcpy(position, at(self, 0x98), 12);                      // 005DA4F8-005DA512
    std::memcpy(up, at(self, 0x80), 12);                            // 005DA515-005DA52F
    std::memcpy(forward, at(self, 0x8c), 12);                       // 005DA532-005DA54B
    std::memset(at(self, 0x8c), 0, 12);                             // 005DA54E-005DA578
    std::memcpy(at(self, 0x80), at(self, 0x8c), 12);                // 005DA57B-005DA5A0
    std::memcpy(at(self, 0x98), at(self, 0x80), 12);                // 005DA5A3-005DA5C7
    kotor_sound_path_required::listener_position_005d6600(self, nullptr, position);        // 005DA5CA-005DA5D4
    kotor_sound_path_required::listener_orientation_005d6540(self, nullptr, up, forward);  // 005DA5D9-005DA5E7
    // Deferred-delete list (+48).
    pos = head(self, 0x48);                                         // 005DA5EC-005DA5FA
    while (pos) {                                                   // 005DA5FD
        src = at_pos(self, 0x48, pos);                              // 005DA607-005DA619
        if (!sound_lower29::ActorPlaying005db0e0(src, nullptr)) {   // 005DA61C-005DA626
            void* old = pos;                                        // 005DA628
            (void)next(self, 0x48, &pos);                           // 005DA631-005DA63E
            add_key_lower::Remove005e9c10(L(self, 0x48), nullptr, old); // 005DA643-005DA653 (005D6CF0)
            if (src) destroy_source(src);                           // 005DA658-005DA688
        } else {
            (void)next(self, 0x48, &pos);                           // 005DA69C-005DA6A9
        }
    }
    kotor_sound_path_required::occlusion_render_005d88f0(self, nullptr, delta); // 005DA6B3-005DA6BD
}
}

namespace kotor_sound_internal {
// 005D69D0 CExoSoundInternal::SetSoundEffectVolume(v, keep_base), RET 8.
void __fastcall SetSoundEffectVolume005d69d0(void* self, void*, float v, U keep_base) {
    if (keep_base == 0) wf(self, 0x7c, v);                          // 005D69DC-005D69DE
    wf(self, 0x70, v);                                              // 005D69E1 / 005D69E7
}
// 005D69F0 CExoSoundInternal::SetDialogVolume(v, keep_base), RET 8.
void __fastcall SetDialogVolume005d69f0(void* self, void*, float v, U keep_base) {
    if (keep_base == 0) wf(self, 0x64, v);                          // 005D69FC-005D69FE
    wf(self, 0x74, v);                                              // 005D6A01 / 005D6A07
}
}

namespace {
// Shared clamp of 005D5C59-005D5CA0 (and the 5CD9/5D59 copies):
// !(v < 1.0) incl. NaN -> 1.0 (JP); 0.0 > v -> 0.0; else v (re-test as observed).
float clamp_unit(float v) {
    if (!(v < 1.0f)) return 1.0f;                                   // fcomp [0073D764], TEST AH,5, JP
    if (!(0.0f <= v)) return 0.0f;                                  // fld [0073D700], fcomp v, TEST AH,41h
    if (!(v < 1.0f)) return 1.0f;                                   // 005D5C85-005D5C94
    return v;
}
}

namespace kotor_sound_path {
// 005D5C50 CExoSound::SetMusicVolume(float, U), RET 8. Null internal: no effect.
void __fastcall SoundSetMusicVolume005d5c50(void* self, void*, float volume, U apply_group) {
    void* internal = rp(self, 0);                                   // 005D5C53
    if (!internal) return;                                          // 005D5C57 -> 005D5CC2
    kotor_sound_internal::SetMusicVolume005d6940(internal, nullptr, clamp_unit(volume), 0, apply_group); // 005D5CA8-005D5CB4
    kotor_sound_path_required::apply_new_volume_005d8110(rp(self, 0), nullptr, 0); // 005D5CB9-005D5CBD (internal reloaded)
}
// 005D5CD0 CExoSound::SetSoundEffectVolume(float), RET 4.
void __fastcall SoundSetSoundEffectVolume005d5cd0(void* self, void*, float volume) {
    void* internal = rp(self, 0);
    if (!internal) return;                                          // 005D5CD7
    kotor_sound_internal::SetSoundEffectVolume005d69d0(internal, nullptr, clamp_unit(volume), 0); // 005D5D28-005D5D2F
    kotor_sound_path_required::apply_new_volume_005d8110(rp(self, 0), nullptr, 0); // 005D5D34-005D5D38
}
// 005D5D50 CExoSound::SetDialogVolume(float), RET 4.
void __fastcall SoundSetDialogVolume005d5d50(void* self, void*, float volume) {
    void* internal = rp(self, 0);
    if (!internal) return;                                          // 005D5D57
    kotor_sound_internal::SetDialogVolume005d69f0(internal, nullptr, clamp_unit(volume), 0); // 005D5DA8-005D5DAF
    kotor_sound_path_required::apply_new_volume_005d8110(rp(self, 0), nullptr, 0); // 005D5DB4-005D5DB8
}
}
