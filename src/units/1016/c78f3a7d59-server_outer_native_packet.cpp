// gfxapply 2026-10-20: one ten-slot CServerExoApp table 00744184 (was six slots here + a 0797 copy with four stubs).
#include "4edf4a86b3-server_outer.hpp"
#include <new>
#include <cstring>
#include <cstddef>
#if !defined(_M_IX86)
#error Native outer constructor bridge requires x86
#endif
extern "C" {
unsigned __fastcall recovered_server_packet_004b15d0(void*,void*,unsigned,void*,unsigned,unsigned);
void __fastcall recovered_server_event_004b78f0(void*,void*,unsigned,unsigned,unsigned);
unsigned __fastcall recovered_globalcat_initialize_004b1650(void*,void*);
void __fastcall recovered_server_options_004b63e0(void*,void*,unsigned);
void* __fastcall recovered_server_construct_004b55e0(void*,void*);
// Real rebuilt bodies (0399-routes-shutdown.obj); original thiscall no-arg RET0 (005ED310: mov ecx,[ecx+4]; jmp 004015A0;
// 004AE900: mov ecx,[ecx+4]; jmp 004B7C60; 004AEC70: ~CServerExoApp, ret; 00401440: SetWindowTitle, no ECX use, ret).
void __fastcall kotor_fastcall_005ed310(void*,void*);
void __fastcall kotor_fastcall_004ae900(void*,void*);
void __fastcall kotor_fastcall_004aec70(void*,void*);
void __cdecl kotor_cdecl_00401440();
void __fastcall kotor_fastcall_004015a0(void*,void*); // 004015A0: plain RET (vtable 00744184 slot 5)
// gfxapply: the remaining four 00744184 slots (6..9) bind existing real bodies.
void* __fastcall recovered_server_get_module_004b14f0(void*,void*);                 // CServerExoAppInternal::GetModule (w1_native_b7)
void __fastcall game_object_void_return_4_0060e760(void*,void*,std::uint32_t);     // 0060E760 `ret 4` (0797)
int __fastcall kotor_fastcall_004b4030(void*,void*);                                // CServerExoAppInternal::FUN_004b4030 (src/bridge)
}
namespace kotor_client_exo_app { unsigned __stdcall client_get_application_id_005ed990(); }  // 005ED990 (0580): [[007A39FC]+18h], ECX unused
namespace {
using namespace recovered_server_create;
void __fastcall slot_004ae870(void* self,void*,unsigned a,unsigned b,unsigned c){void* inner;std::memcpy(&inner,static_cast<unsigned char*>(self)+4,4);recovered_server_event_004b78f0(inner,nullptr,a,b,c);}
unsigned __fastcall slot_004ae840(void* self,void*,unsigned id,void* packet,unsigned length,unsigned flags){void* inner;std::memcpy(&inner,static_cast<unsigned char*>(self)+4,4);return recovered_server_packet_004b15d0(inner,nullptr,id,packet,length,flags);}
float __fastcall slot_004aec60(void*,void*){return 100.0f;}
unsigned __fastcall slot_004ae880(void* self,void*){void* inner;std::memcpy(&inner,static_cast<unsigned char*>(self)+4,4);return static_cast<unsigned>(kotor_fastcall_004b4030(inner,nullptr));}
void* __fastcall slot_004aeda0(void* self,void*){return network_getter_004aeda0(self);}
using Entry=void (__fastcall*)(void*,void*);
using EventEntry=void (__fastcall*)(void*,void*,unsigned,unsigned,unsigned);
using PacketEntry=unsigned (__fastcall*)(void*,void*,unsigned,void*,unsigned,unsigned);using FloatEntry=float (__fastcall*)(void*,void*);using FlagEntry=unsigned (__fastcall*)(void*,void*);
// 004AEE30 CServerExoApp::GetConnectionLib: mov eax,[ecx+4]; mov eax,[eax+10058h]; ret.
void* __fastcall slot_004aee30(void* self,void*){void* inner;std::memcpy(&inner,static_cast<unsigned char*>(self)+4,4);void* lib;std::memcpy(&lib,static_cast<unsigned char*>(inner)+0x10058,4);return lib;}
// 004AECA0 CServerExoApp::GetModuleDescription: GetModule([this+4]) 004B14F0; null -> 0, else GetModule([this+4]) (called
// again, [this+4] re-read) + 4Ch.
void* __fastcall slot_004aeca0(void* self,void*){
 void* inner;std::memcpy(&inner,static_cast<unsigned char*>(self)+4,4);
 if(!recovered_server_get_module_004b14f0(inner,nullptr))return nullptr;                       // 004AECA3..AD
 std::memcpy(&inner,static_cast<unsigned char*>(self)+4,4);                                     // 004AECAF
 return static_cast<unsigned char*>(recovered_server_get_module_004b14f0(inner,nullptr))+0x4C;  // 004AECB2..B7
}
}
// gfxapply: the single CServerExoApp table 00744184 with all ten original slots
// (004AE870 004AE840 004AEC60 004AE880 004AEDA0 004015A0 004AEE30 004AECA0 005ED990 0060E760), installed by the ctor
// (Native::vtable) and, under the same extern "C" name the 0797 copy had, by ~CServerExoApp 004AEC70 (0399 push).
namespace server_outer_vtable {
using namespace ::recovered_server_create;
struct Table {EventEntry a;PacketEntry b;FloatEntry c;FlagEntry d;void*(__fastcall* network)(void*,void*);Entry f;
 void*(__fastcall* connection_lib)(void*,void*);void*(__fastcall* module_description)(void*,void*);
 unsigned (__stdcall* application_id)();void (__fastcall* void_return_4)(void*,void*,std::uint32_t);};
static_assert(sizeof(Table)==40 && offsetof(Table,network)==16 && offsetof(Table,void_return_4)==36,"Original ten-entry vtable 00744184");
}
extern "C" const server_outer_vtable::Table kotor_vtable_cserverexoapp_00744184={slot_004ae870,slot_004ae840,slot_004aec60,slot_004ae880,
 slot_004aeda0,kotor_fastcall_004015a0,slot_004aee30,slot_004aeca0,&kotor_client_exo_app::client_get_application_id_005ed990,
 &game_object_void_return_4_0060e760};
namespace {
void* internal(void* s){void* p;std::memcpy(&p,static_cast<unsigned char*>(s)+4,4);return p;}
struct Native final:OuterPorts {
 void* allocate(std::uint32_t n)override{return ::operator new(n);}
 void free(void* p)override{::operator delete(p);}
 const void* vtable()override{return &kotor_vtable_cserverexoapp_00744184;}
 void* internal_construct_004b55e0(void* s)override{return recovered_server_construct_004b55e0(s,nullptr);}
 void shutdown_005ed310(void* s)override{kotor_fastcall_005ed310(s,nullptr);}
 void stop_004ae900(void* s)override{kotor_fastcall_004ae900(s,nullptr);}
 void destroy_004aec70(void* s)override{kotor_fastcall_004aec70(s,nullptr);}
 void cleanup_00401440()override{kotor_cdecl_00401440();}
 void set_options_004ae8f0(void* s,std::uint32_t n)override{recovered_server_options_004b63e0(internal(s),nullptr,n);}
 std::uint32_t initialize_004ae850(void* s)override{return recovered_globalcat_initialize_004b1650(internal(s),nullptr);}
};
}
extern "C" unsigned __fastcall recovered_create_server_00401380(void* manager,void*,unsigned options){Native p;return recovered_server_create::create_server_00401380(manager,options,p);}
