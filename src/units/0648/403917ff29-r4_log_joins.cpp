// lane-r4-lowers: R4 active_log (MainLoop 00603FE3) CExoString/CExoFile lowers.
// Not reached on frame 1: CExoString 008328D8 is .bss {0,0}, so IsEmpty 005E58A0
// returns 1 at 00603FDB and the log block is skipped. They must link; each R4 name
// forwards to the existing M41 genuine provider of the same original address
// (no new credit), or is authored here when M41 has none (005E6A40, 005E6A70, 006FE4A7).
// frame_LogFormat_005e5680 (variadic) is NOT defined here: see PACKET.json
// /alternatename join to M41 kotor_reconstruction::StartupFormat005e5680 (D1 limit).
#include "e6446a7354-r4_lowers.h"
#include "96c5832fd3-startup_string.h"
#include <new>
#include <cstring>
#include <windows.h>

using namespace r4_lowers;
using kotor_reconstruction::StartupString;
static_assert(sizeof(StartupString) == sizeof(frame_log::String), "CExoString view is the M41 StartupString representation");

namespace tlk_file { struct File; }
namespace file_internal {
__declspec(noinline) tlk_file::File* __fastcall exofile_construct_005e68d0(tlk_file::File*, void*, StartupString*, unsigned int, StartupString*); // M41 m39-constructors.obj
}
extern "C" {
void __fastcall file_destroy_005e69e0(void*, void*);              // M41 resource-successor-file-destructor-file_destroy.obj
std::uint32_t __fastcall file_opened_005e6a10(void*, void*);      // M41 same object
std::uint32_t __fastcall exofile_size_005e6950(void*, void*);     // M41 m39-size-seek.obj
char* __fastcall WindowCStr005e5670(StartupString*, void*);       // M41 window-native-strings.obj
StartupString* __fastcall GuiAssign005e5140(StartupString*, void*, const char*); // M41 ini-string-priority.obj
std::uint32_t __fastcall async_string_length_005e5790(StartupString*, void*); // M41 task10-canonical-ref-string-carriers.obj
std::uint32_t __fastcall kotor_fastcall_005ebc60(void* file_internal, void*, const void* data, std::uint32_t size, std::uint32_t count); // CExoFileInternal::Write (424) MISSING
}

namespace {
StartupString* as_string(frame_log::String* s) { return reinterpret_cast<StartupString*>(s); }
}

extern "C" void __fastcall frame_LogStringCtor_005e5a90(frame_log::String* s, void*, const char* text) noexcept(false) {
    ::new (static_cast<void*>(s)) StartupString(text);            // 005E5A90 CExoString(const char*)
}
// The same constructor returning the object, as thiscall constructors do: the entry bridged code forwards to.
extern "C" void* __fastcall kotor_fastcall_005e5a90(frame_log::String* s, void*, const char* text) noexcept(false) {
    frame_LogStringCtor_005e5a90(s, nullptr, text);
    return s;
}
extern "C" void __fastcall frame_LogStringDtor_005e5c20(frame_log::String* s, void*) {
    as_string(s)->~StartupString();                                // 005E5C20 ~CExoString
}
extern "C" void __fastcall frame_LogFileCtor_005e68d0(frame_log::File* file, void*, frame_log::String* name, unsigned type, frame_log::String* mode) noexcept(false) {
    file_internal::exofile_construct_005e68d0(reinterpret_cast<tlk_file::File*>(file), nullptr, as_string(name), type, as_string(mode)); // 005E68D0
}
extern "C" void __fastcall frame_LogFileDtor_005e69e0(frame_log::File* file, void*) { file_destroy_005e69e0(file, nullptr); }        // 005E69E0
extern "C" unsigned __fastcall frame_LogFileOpened_005e6a10(frame_log::File* file, void*) noexcept(false) { return file_opened_005e6a10(file, nullptr); } // 005E6A10
extern "C" unsigned __fastcall frame_LogFileSize_005e6950(frame_log::File* file, void*) noexcept(false) { return exofile_size_005e6950(file, nullptr); }  // 005E6950
extern "C" const char* __fastcall frame_LogCStr_005e5670(frame_log::String* s, void*) noexcept(false) { return WindowCStr005e5670(as_string(s), nullptr); } // 005E5670
extern "C" void __fastcall frame_LogAssign_005e5140(frame_log::String* s, void*, const char* text) noexcept(false) { GuiAssign005e5140(as_string(s), nullptr, text); } // 005E5140

// 005E6A40 CExoFile::Write(const char*), RET 4: internal->Write(text, 1, strlen(text)).
extern "C" void __fastcall frame_LogWriteText_005e6a40(frame_log::File* file, void*, const char* text) noexcept(false) {
    void* internal = pw(file, 0);                                  // 005E6A40
    const U32 length = static_cast<U32>(std::strlen(text));        // 005E6A47..005E6A57
    kotor_fastcall_005ebc60(internal, nullptr, text, 1, length);   // 005E6A59..005E6A5D
}
// 005E6A70 CExoString overload, RET 4: GetLength first, then CStr.
extern "C" void __fastcall frame_LogWriteString_005e6a70(frame_log::File* file, void*, frame_log::String* s) noexcept(false) {
    void* internal = pw(file, 0);                                  // 005E6A71
    const U32 length = async_string_length_005e5790(as_string(s), nullptr); // 005E6A78..005E6A7A
    const char* text = WindowCStr005e5670(as_string(s), nullptr);  // 005E6A82..005E6A84
    kotor_fastcall_005ebc60(internal, nullptr, text, 1, length);   // 005E6A7F..005E6A8C
}
// 006FE4A7 _strdate (static CRT): "MM/DD/YY" from GetLocalTime (IAT 0073D134).
extern "C" char* __cdecl frame_LogDate_006fe4a7(char* buffer) noexcept(false) {
    SYSTEMTIME now;
    GetLocalTime(&now);                                            // 006FE4B4
    const int year = now.wYear % 100, month = now.wMonth, day = now.wDay; // 006FE4BA..006FE4D0
    buffer[8] = 0; buffer[5] = '/'; buffer[2] = '/';               // 006FE4D6..006FE4DE
    buffer[0] = static_cast<char>('0' + month / 10); buffer[1] = static_cast<char>('0' + month % 10); // 006FE4E6..006FE4EE
    buffer[3] = static_cast<char>('0' + day / 10); buffer[4] = static_cast<char>('0' + day % 10);     // 006FE4F4..006FE501
    buffer[6] = static_cast<char>('0' + year / 10); buffer[7] = static_cast<char>('0' + year % 10);   // 006FE505..006FE50F
    return buffer;                                                 // 006FE512
}
