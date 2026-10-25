// lane-w3 unit w3-client-area-load:
//   00607610 CSWCArea::LoadArea(CAurCamera*, int, int)       (reads the Area/ClientArea record, builds the scene)
//   0064BAB0 CSWCMessage::HandleServerToPlayerArea(BYTE)     (major 0x04)
// Original: swkotor.exe GOG K1; statements cite original instructions. Stack slots are named by their entry-relative
// offset {E-xx} (lane-w3 tools/sdis.py; LoadArea frame depth 174h at every cited site).
#include "c39a01a95c-w3_client_area.h"

extern "C" void __fastcall kotor_fastcall_005ed770(void* app, void*);   // CClientExoApp::EmptyHitcheckIgnoreList (src/bridge)

using namespace kotor_w3;

namespace {
const char kMainScene_0074155c[] = "mainscene";                        // .rdata 0074155C
const char kGrass_0074efc0[] = "Grass";                                // .rdata 0074EFC0
const char kAnimLoop1_0074efb4[] = "animloop1";                        // .rdata 0074EFB4
const char kAnimLoop2_0074efa8[] = "animloop2";                        // .rdata 0074EFA8
const char kAnimLoop3_0074ef9c[] = "animloop3";                        // .rdata 0074EF9C
const char kSoundDummy_0074ef90[] = "sounddummy";                      // .rdata 0074EF90
const char kMgbNull_0074ef84[] = "MGB_null";                           // .rdata 0074EF84
const char kSun_0074ef80[] = "sun";                                    // .rdata 0074EF80
const char kGidySun_0074140c[] = "gidy_sun";                           // .rdata 0074140C
const char kBlackClear_0074ea80[] = "0 0 0";                           // .rdata 0074EA80
const char kBmpResRef_00745588[] = "BMPResRef";                        // .rdata 00745588
constexpr U kInv255_0073ede0 = 0x3B808081u;                            // .rdata 0073EDE0 (1/255)
constexpr U kTwo32_0073d770 = 0x4F800000u;                             // .rdata 0073D770
constexpr U kThousandth_0073d708 = 0x3A83126Fu;                        // .rdata 0073D708
constexpr U kGeomFade0_00741570 = 1, kGeomFade1_00741574 = 2, kGeomFade2_00741578 = 4;   // .rdata 00741570..78

// byte b (zero-extended, x87 fild + "+2^32 when negative" which never applies to a byte) * (1/255)
float byte_colour(U8 b) noexcept { return static_cast<float>(b) * fbits(kInv255_0073ede0); }
// x87 fsin / fcos of a float operand, rounded once to float by the original fstp dword.
float x87_sin(float value) noexcept { float result; __asm { fld value } __asm { fsin } __asm { fstp result } return result; }
float x87_cos(float value) noexcept { float result; __asm { fld value } __asm { fcos } __asm { fstp result } return result; }
// low byte of a DWORD colour field, same conversion
float low_colour(U word) noexcept { return byte_colour(static_cast<U8>(word & 0xFF)); }

// new CExoString[n] with the MSVC count prefix (operator new(n*8+4); eh_vector_constructor_iterator).
void* new_string_array(U n) noexcept(false) {
    void* const block = live_operator_new(n * 8 + 4);
    if (block == nullptr) return nullptr;
    st<U>(block, 0, n);
    unsigned char* const first = static_cast<unsigned char*>(block) + 4;
    for (U i = 0; i < n; ++i)
        kotor_menu20::ConstructString005b3190(reinterpret_cast<kotor_menu20::String*>(first + i * 8), nullptr);
    return first;
}
// delete[] of such an array: eh_vector_destructor_iterator (reverse order) then free(block).
void delete_string_array(void* first) noexcept(false) {
    if (first == nullptr) return;
    unsigned char* const block = static_cast<unsigned char*>(first) - 4;
    const I32 n = ld<I32>(block, 0);
    for (I32 i = n - 1; i >= 0; --i) frame_LogStringDtor_005e5c20(static_cast<unsigned char*>(first) + static_cast<U>(i) * 8, nullptr);
    live_free(block);
}
void* string_at(void* first, I32 i) noexcept { return static_cast<unsigned char*>(first) + static_cast<U>(i) * 8; }

U read_dword(void* m) noexcept { return kotor_fastcall_004d6780(m, nullptr, 0x20); }
U8 read_byte(void* m) noexcept { return kotor_fastcall_004d6720(m, nullptr, 8); }
float read_float(void* m) noexcept { return kotor_fastcall_004d67b0(m, nullptr, bitsf(1.0f), 0x20); }
U read_bool(void* m) noexcept { return kotor_fastcall_004d66f0(m, nullptr); }
void read_string_into(void* m, void* target) noexcept(false) {
    ExoString tmp;
    kotor_fastcall_004d67e0(m, nullptr, &tmp, 0x20);
    async_assign_string_005e5c50(target, nullptr, &tmp);
    frame_LogStringDtor_005e5c20(&tmp, nullptr);
}
void read_resref_into(void* m, void* target) noexcept(false) {
    ResRef read_tmp, copy_tmp;
    kotor_fastcall_004d6180(m, nullptr, &read_tmp, 0x10);
    c2da_copy_00406120(target, nullptr, &copy_tmp, &read_tmp);
}

// Append to the area's CExoArrayList<CSWCMapPin*> {+1C4h data, +1C8h count, +1CCh capacity} (inlined Add).
void append_map_pin(void* area, void* pin) noexcept(false) {
    if (ld<I32>(area, 0x1C8) == ld<I32>(area, 0x1CC)) {               // 006089D3..006089DF / 00608B72..00608B7E
        const I32 capacity = ld<I32>(area, 0x1CC);
        const I32 grown = capacity != 0 ? capacity * 2 : 0x10;        // 006089E1..006089E9
        void* const old = rp(area, 0x1C4);                            // 006089EE..006089FE
        st<I32>(area, 0x1CC, grown);                                  // 006089F4
        st<void*>(area, 0x1C4, live_operator_new(static_cast<U>(grown) * 4));   // 006089FA..00608A0D
        for (I32 i = 0; i < ld<I32>(area, 0x1C8); ++i)                // 00608A16..00608A39
            st<U>(rp(area, 0x1C4), static_cast<U>(i) * 4, rd32(old, static_cast<U>(i) * 4));
        if (old != nullptr) live_free(old);                           // 00608A3B..00608A44
    }
    st<void*>(rp(area, 0x1C4), static_cast<U>(ld<I32>(area, 0x1C8)) * 4, pin);   // 00608A4C..00608A5C
    st<I32>(area, 0x1C8, ld<I32>(area, 0x1C8) + 1);                   // 00608A5F..00608A67
}
}  // namespace

extern "C" {
// 00607610 CSWCArea::LoadArea(CAurCamera* camera, int, int) RET 0Ch. EAX = 1 on success, 0 on failure.
U __fastcall recovered_client_area_load_area_00607610(void* self, void*, void* camera, U, U) noexcept(false) {
    void* const message = client_message();                           // 00607631..00607643 GetSWCMessage
    if (message == nullptr) return 0;                                 // 00607645..00607647
    st<U>(self, 0x104, kotor_fastcall_00692630(message, nullptr));   // 0060764D..0060765B area id
    {
        ResRef name_8c;                                               // {E-0x8c}
        kotor_fastcall_00405ed0(&name_8c, nullptr);                   // 00607654..00607661
        ResRef read_e8, copy_ac;                                      // {E-0xe8} / {E-0xac}
        kotor_fastcall_004d6180(message, nullptr, &read_e8, 0x10);   // 00607666..00607672
        c2da_copy_00406120(&name_8c, nullptr, &copy_ac, &read_e8);    // 00607677..00607687
        c2da_copy_00406120(at(self, 0x10C), nullptr, &read_e8, &name_8c);   // 0060768C..006076A2 [+10Ch] area resref
        kotor_fastcall_00405ed0(&read_e8, nullptr);                   // 006076A7..006076B0
        ExoString name_13c;                                           // {E-0x13c}
        kotor_fastcall_00692660(message, nullptr, &name_13c, &read_e8, nullptr);   // 006076B5..006076BD
        async_assign_string_005e5c50(at(self, 0x17C), nullptr, &name_13c);   // 006076C2..006076D4 [+17Ch] name
        frame_LogStringDtor_005e5c20(&name_13c, nullptr);             // 006076DC..006076E7
    }
    st<U>(self, 0x1A8, read_dword(message));                          // 006076EC..006076F9
    st<U>(self, 0x1AC, read_dword(message));                          // 006076F5..00607708
    st<U>(self, 0x1B0, read_dword(message));                          // 00607704..00607717
    st<U>(self, 0x004, read_dword(message));                          // 00607713..00607726
    st<U>(self, 0x0B8, read_dword(message));                          // 00607722..0060773A
    read_resref_into(message, at(self, 0xBC));                        // 0060772E..00607754
    if (read_byte(message) != 0) {                                    // 00607759..00607764 mini-game area
        void* const memory = live_operator_new(0xF0);                 // 00607766..0060776B
        void* game = nullptr;
        if (memory != nullptr) game = recovered_client_minigame_construct_00671c40(memory, nullptr, self);   // 00607789..0060778C
        st<void*>(self, 0x264, game);                                 // 00607798 / 006077AE
        recovered_aur_disable_interleaved_animations_0044f770();      // 0060779E / 006077B4
    } else {
        recovered_aur_enable_interleaved_animations_0044f760();       // 006077BB
    }
    st<U>(self, 0x68, read_dword(message));                           // 006077C0..006077CD
    st<U>(self, 0x6C, read_dword(message));                           // 006077C9..006077D9
    st<U8>(self, 0x7C, read_byte(message));                           // 006077D5..006077E5
    st<U>(self, 0x70, read_dword(message));                           // 006077E1..006077F6
    st<float>(self, 0x74, read_float(message));                       // 006077ED..006077FE
    st<float>(self, 0x78, read_float(message));                       // 00607801..0060780F
    st<U>(self, 0x80, read_bool(message));                            // 00607812..0060781D
    st<U>(self, 0x84, read_dword(message));                           // 00607819..0060782C
    st<U>(self, 0x88, read_dword(message));                           // 00607828..0060783B
    st<U8>(self, 0x98, read_byte(message));                           // 00607837..0060784A
    st<U>(self, 0x8C, read_dword(message));                           // 00607846..0060785E
    st<float>(self, 0x90, read_float(message));                       // 00607855..00607869
    st<float>(self, 0x94, read_float(message));                       // 0060786F..0060787D
    st<U>(self, 0x9C, read_bool(message));                            // 00607883..0060788E
    st<U>(self, 0xA0, read_byte(message));                            // 0060788A..006078A0 (movzx)
    st<U>(self, 0xA4, read_byte(message));                            // 0060789C..006078AE (movzx)
    st<U>(self, 0xAC, read_dword(message));                           // 006078B4..006078C1
    st<U>(self, 0xB0, read_byte(message));                            // 006078BD..006078D3 (movzx)
    st<U8>(self, 0xB4, read_byte(message));                           // 006078CF..006078DE
    recovered_client_ambient_sound_load_from_message_0068ee80(rp(self, 0x1DC), nullptr, message);   // 006078E4..006078EB
    void* const module = kotor_fastcall_005ed530(client_app(), nullptr);   // 006078F0..00607901 {E-0xb4}
    const U8 time_state = read_byte(message);                         // 006078FD..00607911 {E-0xc0}
    const U world_ms = read_dword(message);                           // 0060790D..00607921 {E-0xc8}
    const U module_b0 = read_byte(message);                           // 0060791D..00607930 {E-0xf0}
    const U module_ac = read_byte(message);                           // 00607937..00607947 {E-0xb0}
    const U module_a8 = read_byte(message);                           // 00607943..0060795A {E-0xbc}
    const U module_a4 = read_dword(message);                          // 00607956..0060796A {E-0xb8}
    const U8 module_c0 = read_byte(message);                          // 00607966..0060797A {E-0x125}
    const U8 module_c1 = read_byte(message);                          // 00607976..0060798F {E-0x126}
    read_resref_into(message, at(self, 0xEC));                        // 00607983..006079A9 [+ECh] grass texture
    st<float>(self, 0xD4, read_float(message));                       // 006079AE..006079BC
    st<float>(self, 0xD8, read_float(message));                       // 006079C2..006079D0
    st<U>(self, 0xD0, read_dword(message));                           // 006079D6..006079E3
    st<U>(self, 0xCC, read_dword(message));                           // 006079DF..006079F4
    for (unsigned o = 0xDC; o < 0xDC + 16; o += 4) st<float>(self, o, read_float(message));   // 006079EE..00607A25 (4 probabilities)
    // 00607A27..00607B46: CAurGrassInfo {E-0x6c}: name = grass texture, +20h/+24h density/quad size,
    // +28h diffuse (+D0h bytes), +34h ambient (+CCh bytes), +40h..+4Ch probabilities.
    unsigned char grass[0x54];
    recovered_aur_grass_info_construct_004a6870(grass, nullptr);      // 00607A27..00607A2E
    gui_load_resref_copy_to_string_00405fb0(at(self, 0xEC), nullptr, grass);   // 00607A33..00607A41
    st<float>(grass, 0x20, ld<float>(self, 0xD4));                    // 00607A4C / 00607A67
    st<float>(grass, 0x24, ld<float>(self, 0xD8));                    // 00607A52 / 00607A6E
    st<float>(grass, 0x34, low_colour(rd32(self, 0xCC)));             // 00607A46..00607A91
    st<float>(grass, 0x38, byte_colour(ld<U8>(self, 0xCD)));          // 00607A83..00607ABB
    st<float>(grass, 0x3C, byte_colour(ld<U8>(self, 0xCE)));          // 00607A8A..00607ACC
    st<float>(grass, 0x28, low_colour(rd32(self, 0xD0)));             // 00607A98..00607AF3
    std::memcpy(grass + 0x40, at(self, 0xDC), 16);                    // 00607AFA..00607B2C
    st<float>(grass, 0x2C, byte_colour(ld<U8>(self, 0xD1)));          // 00607AE5..00607B33
    st<float>(grass, 0x30, byte_colour(ld<U8>(self, 0xD2)));          // 00607AEC..00607B46
    // {E-0xc4} Grass output: ONE frame slot shared by all 32 iterations (the original never stores to it besides
    // C2DA::GetINTEntry, so a failed lookup leaves the value of the previous row in place). Its value before the first
    // row is uninitialised in the original; it is zeroed once here (documented deviation, receipt), never per row.
    unsigned grass_flag = 0;
    for (I32 surface = 0; surface < 0x20; ++surface) {                // 00607B3A..00607BC1
        ExoString column;                                             // {E-0x160}
        frame_LogStringCtor_005e5a90(&column, nullptr, kGrass_0074efc0);   // 00607B50..00607B59
        c2da_current_get_int_00413660(rp(rp(kotor_cell_rules_007a3a28, 0xB8), 0x14), nullptr,
                                      static_cast<unsigned>(surface), &column, &grass_flag);   // 00607B5E..00607B88 surfacemat
        frame_LogStringDtor_005e5c20(&column, nullptr);               // 00607B8D..00607B9C
        if (grass_flag != 0) recovered_aur_grass_add_info_004a74f0(surface, grass);   // 00607BA1..00607BB5
    }
    st<U8>(self, 0xAA, read_byte(message));                           // 00607BC3..00607BD0
    st<U8>(self, 0xAB, read_byte(message));                           // 00607BCC..00607BDF
    const U8 weather_value = read_byte(message);                      // 00607BDB..00607BEC {E-0x127}
    const U weather_stopped = read_bool(message);                     // 00607BEA..00607BFE {E-0x14c}
    st<float>(self, 0xFC, read_float(message));                       // 00607BF5..00607C07
    // environment-audio table {E-0xcc} count, {E-0x11c} names, {E-0x104} env ids, {E-0x100} levels
    void* env_names = nullptr; U* env_ids = nullptr; U* env_levels = nullptr;   // 00607C0D..00607C1B
    const U env_count = read_dword(message);                          // 00607C0F..00607C28
    if (env_count != 0) {                                             // 00607C26..00607C2F
        env_names = static_cast<U*>(new_string_array(env_count));     // 00607C35..00607C7C
        env_ids = static_cast<U*>(live_operator_new(env_count * 4));  // 00607C80..00607C98
        env_levels = static_cast<U*>(live_operator_new(env_count * 4));   // 00607C9C..00607CAE
        for (I32 i = 0; i < static_cast<I32>(env_count); ++i) {       // 00607CAC..00607D45 (signed jle)
            read_string_into(message, string_at(env_names, i));       // 00607CD7..00607D09
            env_ids[i] = read_dword(message);                         // 00607D0E..00607D1D
            st<float>(&env_levels[i], 0, read_float(message));        // 00607D1B..00607D2C
        }
    }
    // room-attached sounds {E-0xec} count: room names {E-0x110}, flags {E-0x164}, parts {E-0x114},
    // events {E-0x108}, resrefs {E-0x148}
    const U sound_count = read_dword(message);                        // 00607D47..00607D5A
    void* const sound_rooms = new_string_array(sound_count);          // 00607D52..00607DA2
    U* const sound_flags = static_cast<U*>(live_operator_new(sound_count * 4));   // 00607DAA..00607DC2
    void* const sound_parts = new_string_array(sound_count);          // 00607DC6..00607E0F
    void* const sound_events = new_string_array(sound_count);         // 00607E17..00607E6B
    void* sound_refs = nullptr;                                       // 00607E73..00607EDA
    if (void* const refs = live_operator_new(sound_count * 16)) {
        for (I32 i = 0; i < static_cast<I32>(sound_count); ++i)       // 00607EA1..00607ECE
            kotor_fastcall_00405ed0(static_cast<unsigned char*>(refs) + static_cast<U>(i) * 16, nullptr);
        sound_refs = refs;
    }
    for (I32 i = 0; i < static_cast<I32>(sound_count); ++i) {         // 00607EE2..0060801E
        read_string_into(message, string_at(sound_rooms, i));         // 00607F20..00607F55
        sound_flags[i] = read_bool(message);                          // 00607F5A..00607F69
        read_string_into(message, string_at(sound_parts, i));         // 00607F6C..00607F9C
        read_string_into(message, string_at(sound_events, i));        // 00607FA1..00607FD6
        read_resref_into(message, static_cast<unsigned char*>(sound_refs) + static_cast<U>(i) * 16);   // 00607FDB..00607FF9
    }
    (void)read_dword(message);                                        // 00608024..00608028 (discarded)
    st<U>(self, 0x08, read_dword(message));                           // 0060802D..0060803A
    st<U>(self, 0x0C, read_dword(message));                           // 00608036..00608046
    if (recovered_player_message::overrun_004d6230(message) != 0) return 0;   // 00608042..00608050 (temporaries are leaked as in the original)
    st<U>(self, 0x1EC, 0x1770);                                       // 0060805C
    st<U8>(self, 0x1F1, weather_stopped != 0 ? weather_value : static_cast<U8>(0));   // 00608056..00608074
    void* const scene = new_aur_scene_00458e70(kMainScene_0074155c);  // 0060807B..00608080
    st<void*>(self, 0x184, scene);                                    // 0060808A
    if (scene == nullptr) return 0;                                   // 00608088..00608090
    // 00608096..006080AB scene vtable +20h (void_return_14 in the original table) with (+8h, +Ch, 5, 1, 10.0f)
    vslot<void (__fastcall*)(void*, void*, U, U, U, U, U)>(scene, 0x20)(scene, nullptr, rd32(self, 8), rd32(self, 0xC), 5, 1,
                                                                        0x41200000u);
    void* const layout = recovered_bridge_layout_new_005de770(nullptr, nullptr);   // 006080AE..006080D8 {E-0x124}
    recovered_bridge_layout_load_005df140(layout, nullptr, at(self, 0x10C));       // 006080E0..006080F8
    const U room_count = recovered_bridge_layout_room_count_005a4c50(layout, nullptr);   // 006080FD..006080FF
    st<U>(self, 0x25C, room_count);                                   // 00608108
    if (room_count != 0) {                                            // 00608106..0060810E
        void* rooms = nullptr;
        if (void* const block = live_operator_new(room_count * 0x48 + 4)) {   // 00608110..0060811B
            st<U>(block, 0, room_count);                              // 00608141
            rooms = static_cast<unsigned char*>(block) + 4;           // 00608143
            for (U i = 0; i < room_count; ++i)                        // 0060814D eh_vector_constructor_iterator
                recovered_client_room_construct_00646080(static_cast<unsigned char*>(rooms) + i * 0x48, nullptr);
        }
        st<void*>(self, 0x260, rooms);                                // 00608165
    }
    texture_make_current_00421ee0();                                  // 0060816B
    U* const stats = recovered_client_area_texture_stats_00832c74;     // [00832C74 + 4*k]
    stats[0] = texture_cells::accounting_007a46d8;                     // 00608170..0060817E [00832C74]
    stats[3] = 0;                                                      // 00608184 [00832C80]
    stats[5] = 0;                                                      // 00608189 [00832C88]
    stats[7] = aurora_part_count_007a8a10;                             // 00608176..0060818E [00832C90]
    I32 sequence = 0;                                                  // {E-0xfc}
    for (I32 i = 0; i < ld<I32>(self, 0x25C); ++i) {                   // 00608194..0060858B ({E-0x158} index, {E-0x118} offset)
        if (i % 5 == 0 || i == ld<I32>(self, 0x25C) - 1) {             // 006081B0..006081CB
            void* const app = client_app();                            // 006081D1..006081D6
            const U step2 = client_load_step(app, 2);                  // 006081D9..006081EB
            // 006081EF..00608208: x87 (53-bit) (i+1)/count * step2, stored as float
            float progress = static_cast<float>(static_cast<double>(static_cast<I32>(step2)) *
                (static_cast<double>(i + 1) / static_cast<double>(ld<I32>(self, 0x25C))));
            progress = static_cast<float>(static_cast<double>(static_cast<I32>(client_load_step(app, 0))) + progress);   // 006081F8..00608224
            progress = static_cast<float>(static_cast<double>(static_cast<I32>(client_load_step(app, 1))) + progress);   // 00608218..00608240
            const double total = static_cast<double>(static_cast<I32>(client_load_step(app, 3))) + progress;   // 00608234..00608254
            kotor_m15_client_set_load_bar_progress_005edd40(app, nullptr, static_cast<I32>(total), 1);   // 00608258..00608260 __ftol2 (truncate)
            kotor_cdecl_00401c10(fbits(0x3D088889u), 0, 0);           // 00608265..0060826E UpdateScreen(1/30, 0, 0)
        }
        void* const room = static_cast<unsigned char*>(rp(self, 0x260)) + static_cast<U>(i) * 0x48;
        Vector3 position_ac;                                           // {E-0xac}
        void* const position = recovered_bridge_layout_room_position_005de4a0(layout, nullptr, &position_ac, static_cast<U>(i));   // 0060827A..00608287
        ResRef name_9c;                                                // {E-0x9c}
        void* const name = recovered_bridge_layout_room_name_005de470(layout, nullptr, &name_9c, static_cast<U>(i));   // 0060828C..0060829A
        recovered_room_base_assign_005c7d70(room, nullptr, name, position);   // 0060829F..006082AC SetRoomInfo
        recovered_room_set_env_audio_props_005c7dc0(room, nullptr, static_cast<I32>(env_count), env_names, env_ids,
                                                    env_levels);       // 006082B1..006082D0
        if (rp(room, 0x38) != nullptr) {                               // 006082D5..006082E3
            st<U>(room, 0x44, 0xFFFFFFFFu);                            // 006082E5
        } else {
            st<I32>(room, 0x44, sequence);                             // 006082EE..006082F2
            ++sequence;                                                // 006082F5..006082F6
        }
        void* const gob = recovered_client_room_spawn_room_006460c0(room, nullptr, rp(self, 0x184));   // 006082FA..0060830D {E-0x120}
        if (gob != nullptr) {                                          // 00608314..0060831A
            using Play = U (__fastcall*)(void*, void*, const char*, U, U, U);
            vslot<Play>(gob, 0x18)(gob, nullptr, kAnimLoop1_0074efb4, 0x3F800000u, 8, 0);   // 0060831C..0060832E
            vslot<Play>(gob, 0x18)(gob, nullptr, kAnimLoop2_0074efa8, 0x3F800000u, 8, 0);   // 00608331..00608343
            vslot<Play>(gob, 0x18)(gob, nullptr, kAnimLoop3_0074ef9c, 0x3F800000u, 8, 0);   // 00608346..00608358
        }
        ResRef room_ref_e8;                                            // {E-0xe8}
        std::memcpy(&room_ref_e8, static_cast<unsigned char*>(rp(self, 0x260)) + static_cast<U>(i) * 0x48 + 0x20, 16);   // 0060835B..0060838F
        ExoString room_name_13c;                                       // {E-0x13c}
        frame_LogStringCtor_005e5a90(&room_name_13c, nullptr, c2da_ref_00405fe0(&room_ref_e8, nullptr));   // 00608388..006083A0
        for (I32 k = 0; k < static_cast<I32>(sound_count); ++k) {      // 006083A5..00608555
            void* const part_name = string_at(sound_parts, k);        // ebx
            if (async_string_equal_005e5200(string_at(sound_rooms, k), nullptr, &room_name_13c) == 0) continue;   // 006083F0..0060840E
            if (gob == nullptr) continue;                              // 00608414..0060841A
            void* const sound_gob = aurora_new_aur_object_00449cc0(kMgbNull_0074ef84, kSoundDummy_0074ef90, nullptr, nullptr);   // 00608420..0060842E {E-0x144}
            void* const part = vslot<void* (__fastcall*)(void*, void*, const char*)>(gob, 0x10C)(
                gob, nullptr, async_string_cstr_005e5670(part_name, nullptr));   // 00608437..00608454 GetPart
            if (part == nullptr) continue;                             // 0060845C..0060845E
            void* behavior = nullptr;                                  // 006084B4
            if (void* const memory = live_operator_new(0x50)) {        // 00608464..00608474
                ResRef sound_7c;                                       // {E-0x7c}
                reinterpret_cast<menu_resources::Ref*>(&sound_7c)->construct_00406d80(
                    c2da_ref_00405fe0(static_cast<unsigned char*>(sound_refs) + static_cast<U>(k) * 16, nullptr));   // 0060847E..0060848F
                behavior = recovered_behavior_attach_sound_construct_00690ef0(memory, nullptr, sound_gob, gob, part, &sound_7c);   // 00608494..006084AB
            }
            vslot<U (__fastcall*)(void*, void*, void*)>(sound_gob, 0x58)(sound_gob, nullptr, behavior);   // 006084B6..006084C9 SetBehavior
            vslot<U (__fastcall*)(void*, void*, void*)>(sound_gob, 0x4C)(sound_gob, nullptr, rp(self, 0x184));   // 006084CC..006084D9 AttachToScene
            vslot<U (__fastcall*)(void*, void*)>(sound_gob, 0x130)(sound_gob, nullptr);   // 006084DC..006084E2 EnableAlwaysRender
            void* const event = string_at(sound_events, k);
            if (frame_LogIsEmpty_005e58a0(event, nullptr) == 0) {      // 006084E8..006084F3
                using Register = U (__fastcall*)(void*, void*, void (__cdecl*)(void*, void*, void*), const char*, void*, U, U);
                vslot<Register>(gob, 0x28)(gob, nullptr, &recovered_play_room_sound_event_00604b50,
                                           async_string_cstr_005e5670(event, nullptr), behavior, 0x461C3C00u, 0);   // 006084F5..0060851E
            } else {
                recovered_behavior_attach_sound_play_00691000(behavior, nullptr);   // 00608523..00608525
            }
            c2da_005e9d70(rp(self, 0x280), nullptr, behavior);         // 0060852A..00608531 AddHead
        }
        frame_LogStringDtor_005e5c20(&room_name_13c, nullptr);         // 0060855B..0060856A
    }
    delete_string_array(sound_rooms);                                  // 00608591..006085B3
    live_free(sound_flags);                                            // 006085B6..006085BB
    delete_string_array(sound_parts);                                  // 006085C0..006085E5
    delete_string_array(sound_events);                                 // 006085E8..0060860A
    live_free(sound_refs);                                             // 0060860D..00608612
    texture_make_current_00421ee0();                                   // 0060861A
    stats[1] = texture_cells::accounting_007a46d8;                     // 0060861F..00608630 [00832C78]
    stats[2] = stats[1] - stats[0];                                    // 00608636..0060863E [00832C7C]
    stats[8] = aurora_part_count_007a8a10;                             // 0060862B..0060864A [00832C94]
    stats[9] = stats[8] - stats[7];                                    // 0060864F..00608657 [00832C98]
    stats[4] = stats[3];                                               // 00608651..00608662 [00832C84]
    stats[6] = stats[5];                                               // 00608638..00608668 [00832C8C]
    delete_string_array(env_names);                                    // 0060865C..0060868A
    if (env_ids != nullptr) live_free(env_ids);                        // 0060868D..0060869B
    if (env_levels != nullptr) live_free(env_levels);                  // 0060869E..006086AC
    if (rp(self, 0x264) != nullptr) {                                  // 006086AF..006086BB mini-game layout content
        const I32 tracks = static_cast<I32>(recovered_layout_track_count_005d64f0(layout, nullptr));   // 006086C1..006086CA
        for (I32 t = 0; t < tracks; ++t) {                             // 006086CE..00608748
            ExoString text; ResRef ref; Vector3 where; ResRef name_tmp;
            gui_load_resref_copy_to_string_00405fb0(recovered_bridge_layout_track_name_005de4d0(layout, nullptr, &name_tmp,
                                                    static_cast<U>(t)), nullptr, &text);   // 006086E0..006086FB
            reinterpret_cast<menu_resources::Ref*>(&ref)->construct_00406d80(reinterpret_cast<const char*>(&text));   // 00608700..0060870C
            recovered_client_minigame_add_track_006720c0(rp(self, 0x264), nullptr, &ref,
                recovered_bridge_layout_track_position_005de500(layout, nullptr, &where, static_cast<U>(t)));   // 00608711..00608734
        }
        const I32 obstacles = static_cast<I32>(c2da_current_resource_size_00710e50(layout, nullptr));   // 0060874A..00608753
        for (I32 t = 0; t < obstacles; ++t) {                          // 00608757..006087D8
            ExoString text; ResRef ref; Vector3 where; ResRef name_tmp;
            gui_load_resref_copy_to_string_00405fb0(recovered_bridge_layout_obstacle_name_005de530(layout, nullptr, &name_tmp,
                                                    static_cast<U>(t)), nullptr, &text);   // 00608770..0060878B
            reinterpret_cast<menu_resources::Ref*>(&ref)->construct_00406d80(reinterpret_cast<const char*>(&text));   // 00608790..0060879C
            recovered_client_minigame_add_obstacle_00672240(rp(self, 0x264), nullptr, &ref,
                recovered_bridge_layout_obstacle_position_005de560(layout, nullptr, &where, static_cast<U>(t)));   // 006087A1..006087C4
        }
        if (recovered_client_minigame_load_006723d0(rp(self, 0x264), nullptr) == 0) {   // 006087DA..006087E7
            if (layout != nullptr) delete_object(layout);              // 006087E9..006087F3
            void* const rooms = rp(self, 0x260);                       // 006087F5
            if (rooms != nullptr) {                                    // 006087FB..006087FD
                if (rd32(static_cast<unsigned char*>(rooms) - 4, 0) != 0) delete_object_flags(rooms, 3);   // 006087FF..0060880D
                else live_free(static_cast<unsigned char*>(rooms) - 4);   // 0060881E..0060881F
            }
            st<U>(self, 0x260, 0);                                     // 0060880F / 00608827
            return 0;                                                  // 006093F3
        }
    }
    recovered_bridge_layout_unload_005de450(layout, nullptr);          // 00608836..00608838 CLYT::UnloadLayout
    if (layout != nullptr) delete_object(layout);                      // 0060883D..00608847
    {
        void* const sc = rp(self, 0x184);
        // 00608849..00608863 scene vtable +94h LoadVisibility(resref) ; +7Ch SetCurrentRoom(0) ; +88h / +8Ch
        if (vslot<U (__fastcall*)(void*, void*, const char*)>(sc, 0x94)(sc, nullptr, c2da_ref_00405fe0(at(self, 0x10C), nullptr)) != 0) {
            vslot<U (__fastcall*)(void*, void*, U)>(rp(self, 0x184), 0x7C)(rp(self, 0x184), nullptr, 0);   // 00608875..00608877
            vslot<U8 (__fastcall*)(void*, void*)>(rp(self, 0x184), 0x88)(rp(self, 0x184), nullptr);      // 0060887A..00608882
        } else {
            vslot<void (__fastcall*)(void*, void*)>(rp(self, 0x184), 0x8C)(rp(self, 0x184), nullptr);     // 0060888A
        }
    }
    recovered_aur_texture_suspend_updates_00420490();                  // 00608890
    (void)frame_R4_005ed690(client_app(), nullptr);                    // 00608895..0060889D GetInGameGui (unused)
    for (I32 n = static_cast<I32>(read_dword(message)); n > 0; --n) {  // 006088A2..00608A71 map pins (TLK/inline note)
        void* pin = nullptr;
        if (void* const memory = live_operator_new(0x110)) pin = recovered_client_map_pin_construct_00692540(memory, nullptr);   // 006088E0..0060890B
        st<U>(pin, 0x108, kotor_fastcall_00692630(message, nullptr));   // 0060890D..00608928
        Vector3 where;                                                 // {E-0xd8}
        where.x = read_float(message); where.y = read_float(message); where.z = read_float(message);   // 0060891F..0060895D
        vslot<void (__fastcall*)(void*, void*, const Vector3*)>(pin, 0x8C)(pin, nullptr, &where);   // 00608964..00608970 SetPosition
        st<U>(pin, 0xFC, read_bool(message));                          // 00608976..00608986
        ResRef sound; kotor_fastcall_00405ed0(&sound, nullptr);        // 0060897D..0060898C
        ExoString text;
        kotor_fastcall_00692660(message, nullptr, &text, &sound, nullptr);   // 00608991..00608999
        async_assign_string_005e5c50(at(pin, 0x100), nullptr, &text);  // 0060899E..006089B0
        frame_LogStringDtor_005e5c20(&text, nullptr);                  // 006089B5..006089C4
        st<U>(pin, 0x10C, 0);                                          // 006089C9
        append_map_pin(self, pin);                                     // 006089D3..00608A67
    }
    for (I32 n = static_cast<I32>(read_dword(message)); n > 0; --n) {  // 00608A77..00608C11 map pins (plain text)
        void* pin = nullptr;
        if (void* const memory = live_operator_new(0x110)) pin = recovered_client_map_pin_construct_00692540(memory, nullptr);   // 00608A90..00608ABB
        st<U>(pin, 0x108, read_dword(message));                        // 00608ABD..00608AD1
        read_string_into(message, at(pin, 0x100));                     // 00608AD7..00608B0B
        Vector3 where;
        where.x = read_float(message); where.y = read_float(message); where.z = read_float(message);   // 00608B10..00608B48
        vslot<void (__fastcall*)(void*, void*, const Vector3*)>(pin, 0x8C)(pin, nullptr, &where);   // 00608B4F..00608B5B
        st<U>(pin, 0xFC, 1);                                           // 00608B61..00608B66
        st<U>(pin, 0x10C, 1);                                          // 00608B6C
        append_map_pin(self, pin);                                     // 00608B72..00608C07
    }
    void* const objects = kotor_fastcall_005ed570(client_app(), nullptr);   // 00608C17..00608C28
    for (U n = kotor_fastcall_004d6750(message, nullptr, 0x10); n != 0; --n) {   // 00608C24..00608C91 sound objects
        void* sound = nullptr;
        if (void* const memory = live_operator_new(0x16C)) sound = recovered_client_sound_object_construct_0066b200(memory, nullptr);   // 00608C40..00608C6B
        recovered_client_sound_object_load_from_message_0066b420(sound, nullptr, message);   // 00608C6D..00608C7B
        recovered_client_sound_object_add_to_area_0066b8a0(sound, nullptr, self, objects);   // 00608C80..00608C88
    }
    {
        const I32 n = kotor_fastcall_004d6750(message, nullptr, 0x10);   // 00608C93..00608CDF placeable lights
        for (I32 i = 0; i < n; ++i) {                                  // 00608CE3..00608D7E
            const U id = kotor_fastcall_00692630(message, nullptr);   // 00608CF0..00608CF2
            const U8 kind = read_byte(message);                        // 00608CF7..00608D0B
            const float x = read_float(message), y = read_float(message), z = read_float(message);   // 00608D02..00608D38
            if (recovered_player_message::overrun_004d6230(message) != 0) break;   // 00608D3C..00608D47
            recovered_client_area_add_placeable_light_00606e30(self, nullptr, id, kind, x, y, z);   // 00608D49..00608D6A
        }
    }
    {
        const I32 n = kotor_fastcall_004d6750(message, nullptr, 0x10);   // 00608D84..00608D9A placeable statics
        for (I32 i = 0; i < n; ++i) {                                  // 00608D9E..00608E87
            const U id = kotor_fastcall_00692630(message, nullptr);   // 00608DA4..00608DA6
            const U8 kind = read_byte(message);                        // 00608DAB..00608DBF
            const float px = read_float(message), py = read_float(message), pz = read_float(message);   // 00608DB6..00608DEC
            const float ox = read_float(message), oy = read_float(message), oz = read_float(message);   // 00608DF0..00608E22
            if (recovered_player_message::overrun_004d6230(message) != 0) break;   // 00608E29..00608E34
            recovered_client_area_add_placeable_static_006072d0(self, nullptr, id, kind, px, py, pz, ox, oy, oz);   // 00608E36..00608E73
        }
    }
    recovered_aur_texture_resume_updates_004204a0();                   // 00608E8D
    bool fail = recovered_player_message::overrun_004d6230(message) != 0 ||   // 00608E92..00608E9D
                recovered_player_message::underrun_004d6240(message) != 0;  // 00608EA3..00608EAE
    void* sun = nullptr;
    if (!fail) {
        sun = aurora_new_aur_object_00449cc0(kGidySun_0074140c, kSun_0074ef80, nullptr, nullptr);   // 00608EB4..00608EC0
        st<void*>(self, 0x188, sun);                                   // 00608ECA
        fail = sun == nullptr;                                         // 00608EC8..00608ED0
    }
    if (fail) {                                                        // 006093D9
        if (rp(self, 0x184) != nullptr) delete_object(rp(self, 0x184));   // 006093D9..006093E7
        st<U>(self, 0x184, 0);                                         // 006093E9
        return 0;                                                      // 006093FC
    }
    {
        Vector3 out_104;                                               // {E-0x70} (lea ecx,[esp+104h] after the 0Ch vector)
        const Vector3 sun_position = {fbits(0x457A0000u), fbits(0x458CA000u), fbits(0x45DAC000u)};   // 00608EDD..00608F06 (4000, 4500, 7000)
        vslot<Vector3* (__fastcall*)(void*, void*, Vector3*, Vector3)>(sun, 0x5C)(sun, nullptr, &out_104, sun_position);   // 00608F09..00608F13 SetPosition
    }
    vslot<U (__fastcall*)(void*, void*, void*)>(rp(self, 0x188), 0x4C)(rp(self, 0x188), nullptr, rp(self, 0x184));   // 00608F16..00608F25 AttachToScene
    vslot<void (__fastcall*)(void*, void*, U)>(rp(self, 0x184), 0x04)(rp(self, 0x184), nullptr, ld<U8>(self, 0x98));   // 00608F28..00608F38 SetFog(bool)
    beam_alpha_0078e6a0 = fbits(0x3D4CCCCDu);                          // 00608F3B
    // 00608F45..00609172: byte colours -> float colour vectors (low byte of each DWORD then its two following bytes)
    st<float>(self, 0x140, low_colour(rd32(self, 0x70)));
    st<float>(self, 0x144, byte_colour(ld<U8>(self, 0x71)));
    st<float>(self, 0x148, byte_colour(ld<U8>(self, 0x72)));
    st<float>(self, 0x128, low_colour(rd32(self, 0x68)));
    st<float>(self, 0x12C, byte_colour(ld<U8>(self, 0x69)));
    st<float>(self, 0x130, byte_colour(ld<U8>(self, 0x6A)));
    st<float>(self, 0x134, low_colour(rd32(self, 0x6C)));
    st<float>(self, 0x138, byte_colour(ld<U8>(self, 0x6D)));
    st<float>(self, 0x13C, byte_colour(ld<U8>(self, 0x6E)));
    st<float>(self, 0x164, low_colour(rd32(self, 0x8C)));
    st<float>(self, 0x168, byte_colour(ld<U8>(self, 0x8D)));
    st<float>(self, 0x16C, byte_colour(ld<U8>(self, 0x8E)));
    st<float>(self, 0x14C, low_colour(rd32(self, 0x84)));
    st<float>(self, 0x150, byte_colour(ld<U8>(self, 0x85)));
    st<float>(self, 0x154, byte_colour(ld<U8>(self, 0x86)));
    st<float>(self, 0x158, low_colour(rd32(self, 0x88)));
    st<float>(self, 0x15C, byte_colour(ld<U8>(self, 0x89)));
    st<float>(self, 0x160, byte_colour(ld<U8>(self, 0x8A)));
    if (rd32(self, 0xA4) != 0) recovered_client_area_set_as_night_006059c0(self, nullptr);   // 00609150..00609176
    else recovered_client_area_set_as_day_006058c0(self, nullptr);                          // 0060917D
    st<float>(self, 0x170, low_colour(rd32(self, 0xAC)));              // 00609182..006091B4
    st<float>(self, 0x174, byte_colour(ld<U8>(self, 0xAD)));           // 006091A6..006091D1
    st<float>(self, 0x178, byte_colour(ld<U8>(self, 0xAE)));           // 006091AD..006091E1
    recovered_aur_scene_set_ambient_0044fcd0(rp(self, 0x184), nullptr, ld<float>(self, 0x170), ld<float>(self, 0x174),
                                             ld<float>(self, 0x178));  // 006091E7..00609207
    vslot<void (__fastcall*)(void*, void*)>(rp(self, 0x184), 0x58)(rp(self, 0x184), nullptr);   // 0060920C..00609214
    vslot<U (__fastcall*)(void*, void*)>(rp(self, 0x184), 0x3C)(rp(self, 0x184), nullptr);      // 00609217..0060921F ComputeStaticLighting
    vslot<I32 (__fastcall*)(void*, void*, void*)>(camera, 0x14)(camera, nullptr, rp(self, 0x184));   // 00609222..0060922E camera AttachToScene
    vslot<U (__fastcall*)(void*, void*, U)>(self, 0x10)(self, nullptr, ld<U8>(self, 0xAB));     // 00609231..0060923E SetWind
    recovered_client_module_set_time_of_day_state_00640640(module, nullptr, time_state, 0);    // 00609241..00609254
    {   // 00609259..006092BF module clock fields
        double ms = static_cast<double>(static_cast<I32>(world_ms));
        if (static_cast<I32>(world_ms) < 0) ms += static_cast<double>(fbits(kTwo32_0073d770));
        st<U>(module, 0xA4, module_a4);                                // 0060928C
        st<float>(module, 0xB8, static_cast<float>(ms * static_cast<double>(fbits(kThousandth_0073d708))));   // 00609259..00609292
        st<U>(module, 0xA8, module_a8);                                // 0060929F
        st<U>(module, 0xAC, module_ac);                                // 006092A9
        st<U>(module, 0xB0, module_b0);                                // 006092B3
        st<U8>(module, 0xC0, module_c0);                               // 006092B9
        st<U8>(module, 0xC1, module_c1);                               // 006092BF
    }
    {
        const I32 n = ld<I32>(self, 0x1D4);                            // 006092C5..006092CF
        for (I32 i = 0; i < n; ++i) {                                  // 006092D1..006092F7
            void* const sound = recovered_client_get_sound_object_005ed620(client_app(), nullptr,
                rd32(rp(self, 0x1D0), static_cast<U>(i) * 4));
            if (sound != nullptr) recovered_client_sound_object_update_time_of_day_0066ae90(sound, nullptr);
        }
    }
    recovered_client_ambient_sound_update_time_of_day_0068d690(rp(self, 0x1DC), nullptr);   // 006092F9..006092FF
    void* const options = kotor_fastcall_005ed700(client_app(), nullptr);   // 00609304..0060930D
    if (rp(self, 0x184) != nullptr) {                                  // 00609312..0060931A
        U fade;
        switch (ld<U8>(options, 0x6B)) {                               // 0060931C..00609329
        case 0: fade = kGeomFade0_00741570; break;                     // 00609339
        case 1: fade = kGeomFade1_00741574; break;                     // 00609332
        case 2: fade = kGeomFade2_00741578; break;                     // 0060932B
        default: fade = module_b0; break;                              // 00609340 ({E-0xf0})
        }
        vslot<U (__fastcall*)(void*, void*, U)>(rp(self, 0x184), 0x4C)(rp(self, 0x184), nullptr, fade);   // 00609347..0060934A SetGeomFadeProperties
    }
    if (client_get_player_creature_005ed540(client_app(), nullptr) != nullptr) {   // 0060934D..0060935C
        void* const player = client_get_player_creature_005ed540(client_app(), nullptr);   // 0060935E..00609367
        vslot<U (__fastcall*)(void*, void*, U)>(player, 0xC0)(player, nullptr, 0);   // 0060936C..00609372 RefreshVisualEffects(0)
    }
    for (I32 i = 0; i < ld<I32>(self, 0x210); ++i) {                   // 00609378..006093BB
        void* const creature = frame_R4_005ed5b0(client_app(), nullptr, rd32(rp(self, 0x20C), static_cast<U>(i) * 4));   // 00609390..006093A2
        if (creature != nullptr) recovered_client_creature_update_area_id_0060ed90(creature, nullptr);   // 006093A7..006093AD
    }
    return 1;                                                          // 006093BD
}

// 0064BAB0 CSWCMessage::HandleServerToPlayerArea(BYTE minor). ECX this, RET 4. EAX = handled.
U __fastcall recovered_client_handle_area_0064bab0(void* self, void*, U minor) noexcept(false) {
    if (recovered_player_message::overrun_004d6230(self) != 0) return 0;   // 0064BAD1..0064BADC
    switch (static_cast<U8>(minor)) {                                 // 0064BAE2..0064BAF7
    case 4: {                                                         // 0064BAFD weather start/stop
        const U8 value = read_byte(self);                             // 0064BAFD..0064BB08
        const U stop = read_bool(self);                               // 0064BB06..0064BB11
        void* const module = kotor_fastcall_005ed530(client_app(), nullptr);   // 0064BB13..0064BB1B
        if (module == nullptr || rp(module, 0x48) == nullptr) return 0;   // 0064BB20..0064BB2D
        if (stop != 0) recovered_client_area_stop_weather_00605380(rp(module, 0x48), nullptr, value);   // 0064BB33..0064BB3C
        else recovered_client_area_start_weather_006053a0(rp(module, 0x48), nullptr, value);            // 0064BB5A..0064BB5F
        return 1;
    }
    case 2: {                                                         // 0064BB7D fire-and-forget visual effect
        const U16 effect = kotor_fastcall_004d6750(self, nullptr, 0x10);   // 0064BB7D..0064BB81
        const float x = read_float(self), y = read_float(self), z = read_float(self);   // 0064BB86..0064BBBA
        void* const module = kotor_fastcall_005ed530(client_app(), nullptr);   // 0064BBBE..0064BBC7
        if (module == nullptr) return 0;                              // 0064BBCC..0064BBCE
        recovered_client_area_apply_fire_and_forget_visual_effect_00604c90(rp(module, 0x48), nullptr, effect, x, y, z);   // 0064BBD4..0064BBF1
        return 1;
    }
    case 1: break;                                                    // 0064BC0F client area
    default: return 0;                                                // 0064BF15
    }
    void* const module = kotor_fastcall_005ed530(client_app(), nullptr);   // 0064BC0F..0064BC18
    if (module == nullptr) { kotor_fastcall_004d6170(self, nullptr); return 0; }   // 0064BC1D..0064BC1F -> 0064BF0E
    void* const area = rp(module, 0x48);                              // 0064BC25
    void* const camera = rp(module, 0x40);                            // 0064BC2A..0064BC2D {E-0x58}
    if (area == nullptr || camera == nullptr) { kotor_fastcall_004d6170(self, nullptr); return 0; }   // 0064BC28..0064BC39
    {
        void* removed = nullptr;                                      // {E-0x40}
        recovered_object_array_remove_004d81a0(kotor_fastcall_005ed570(client_app(), nullptr), nullptr,
                                               rd32(area, 0x104), &removed);   // 0064BC3F..0064BC5B CGameObjectArray::Delete(old id)
    }
    ExoString load_screen;                                            // {E-0x3c}
    kotor_menu20::ConstructString005b3190(reinterpret_cast<kotor_menu20::String*>(&load_screen), nullptr);   // 0064BC60..0064BC64
    U8 screen_index = read_byte(self);                                // 0064BC69..0064BC7A (bl)
    if (screen_index == 1) {                                          // 0064BC7C..0064BC7F
        ExoString tmp;
        kotor_fastcall_004d67e0(self, nullptr, &tmp, 0x20);   // 0064BC81..0064BC8A
        async_assign_string_005e5c50(&load_screen, nullptr, &tmp);    // 0064BC8F..0064BC98
        frame_LogStringDtor_005e5c20(&tmp, nullptr);                  // 0064BC9D..0064BCA6
    }
    ExoString screen_name;                                            // {E-0x4c}
    kotor_menu20::ConstructString005b3190(reinterpret_cast<kotor_menu20::String*>(&screen_name), nullptr);   // 0064BCAB..0064BCAF
    if (screen_index == 0) {                                          // 0064BCB4..0064BCBB random load screen
        const I32 rows = ld<I32>(rp(rp(kotor_cell_rules_007a3a28, 0xB8), 0x18), 0x24);   // 0064BCBD..0064BCCC loadscreens 2DA rows
        const I32 r = crt_rand_006fc447();                            // 0064BCCF
        screen_index = static_cast<U8>(r % (rows - 2) + 2);           // 0064BCD4..0064BCDC (idiv; dl + 2)
    }
    void* chosen;
    if (screen_index == 1) {                                          // 0064BCDF..0064BCE2
        chosen = &load_screen;                                        // 0064BCE4..0064BCE8
    } else {
        ExoString column;                                             // {E-0x40}
        frame_LogStringCtor_005e5a90(&column, nullptr, kBmpResRef_00745588);   // 0064BCEA..0064BCF3
        c2da_current_get_string_00413270(rp(rp(kotor_cell_rules_007a3a28, 0xB8), 0x18), nullptr, screen_index, &column,
                                         &screen_name);               // 0064BCF8..0064BD1B
        frame_LogStringDtor_005e5c20(&column, nullptr);               // 0064BD20..0064BD29
        chosen = &screen_name;                                        // 0064BD2E
    }
    // 0064BD32..0064BD4F: ECX = [[app]+4] (client internal), push 1 / CStr(chosen) / 2, call 0066B360 return_zero_0xc
    // ("xor eax,eax; ret 0Ch": no side effect; runtime1115 has it only inside a private port adapter) -> EBX = 0.
    (void)async_string_cstr_005e5670(chosen, nullptr);
    const U loading = 0;
    kotor_fastcall_00644650(kotor_fastcall_005ed530(client_app(), nullptr), nullptr, fbits(0x322BCC77u));   // 0064BD51..0064BD65 CSWCModule::Update(1e-8) (no result)
    recovered_set_clear_color_00401690(kBlackClear_0074ea80);         // 0064BD6A..0064BD7D
    (void)recovered_bridge_exo_timers_high_resolution_005e9260(rp(kotor_menu_10_19::dependencies::base_007a39e0, 4), nullptr);   // 0064BD74..0064BD80
    (void)kotor_input_required::get_snapshot_time_difference_004adc80(frame_GetWorldTimer_005ed4f0(client_app(), nullptr), nullptr);   // 0064BD85..0064BD95
    frame_LogStringDtor_005e5c20(&screen_name, nullptr);              // 0064BD9A..0064BDA3
    kotor_fastcall_005ed770(client_app(), nullptr);                   // 0064BDA8..0064BDB0 EmptyHitcheckIgnoreList
    kotor_cdecl_00401c10(fbits(0x3D4CCCCDu), 0, 0);                   // 0064BDB5..0064BDC3 UpdateScreen(0.05, 0, 0)
    recovered_client_area_unload_area_00609410(area, nullptr);        // 0064BDC6..0064BDC8
    const float px = read_float(self), py = read_float(self), pz = read_float(self);   // 0064BDCD..0064BDFF {E-0x48..}
    const float facing = read_float(self);                            // 0064BE03..0064BE11 {E-0x58}
    (void)read_bool(self);                                            // 0064BE15..0064BE17
    if (recovered_player_message::overrun_004d6230(self) != 0 ||      // 0064BE1C..0064BE27
        recovered_client_area_load_area_00607610(area, nullptr, camera, loading, 1) == 0) {   // 0064BE29..0064BE3A
        kotor_fastcall_004d6170(self, nullptr);         // 0064BE3C..0064BE3E
        frame_LogStringDtor_005e5c20(&load_screen, nullptr);          // 0064BE43..0064BE4F
        return 0;                                                     // 0064BF15
    }
    void* const player = client_get_player_creature_005ed540(client_app(), nullptr);   // 0064BE59..0064BE62
    if (player != nullptr) {                                          // 0064BE67..0064BE6B
        Vector3 position = {px, py, pz};                              // {E-0x2c}
        vslot<void (__fastcall*)(void*, void*, const Vector3*)>(player, 0x8C)(player, nullptr, &position);   // 0064BE6D..0064BE8E SetPosition
        st<float>(player, 0x1DC, position.x); st<float>(player, 0x1E0, position.y); st<float>(player, 0x1E4, position.z);   // 0064BE9A..0064BEB1
        // 0064BE94..0064BECF: orientation (sin, cos, 0) of the facing angle (x87 fsin / fcos on the float)
        Vector3 orientation;
        orientation.x = x87_cos(facing);                              // 0064BEC5..0064BECB fcos -> fstp dword
        orientation.y = x87_sin(facing);                              // 0064BE94..0064BECF fsin -> fstp dword
        orientation.z = 0.0f;                                         // 0064BEBD
        vslot<void (__fastcall*)(void*, void*, const Vector3*)>(player, 0x88)(player, nullptr, &orientation);   // 0064BED3 SetOrientation
        recovered_client_creature_set_way_points_0060c790(player, nullptr, 0, 0);   // 0064BED9..0064BEDF
    }
    frame_LogStringDtor_005e5c20(&load_screen, nullptr);              // 0064BEE4..0064BEF0
    return 1;                                                         // 0064BEF5
}
}  // extern "C"
