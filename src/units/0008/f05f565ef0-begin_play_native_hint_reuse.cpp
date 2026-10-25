#include "7765cc0e84-begin_play.hpp"
#include "a5ce01f027-net_connect.hpp"
#include "d1bb495762-client_begin_leaves.hpp"
#include "c43966139d-loading_lookup.hpp"
#include "a75d7ae91a-module_query.hpp"
#include "13b5e26d41-module_mount.hpp"
#include "00a0946121-resources.h"
#include "97e489d1ec-table_contract.h"
#include "b1d8b1b4c3-load.h"
#include <cstring>
#include <new>
namespace kotor_menu_window_cells {extern void* volatile input_007a39e4;}
namespace kotor_menu_10_19::dependencies {extern void* base_007a39e0;}
namespace menu_resources {Manager* __cdecl BorrowCanonicalManager007a39e8();}
kotor_reconstruction::StartupString* __fastcall GuiConcat005e5d10(kotor_reconstruction::StartupString*,void*,kotor_reconstruction::StartupString*,const kotor_reconstruction::StartupString*);
namespace kotor_reconstruction {void __cdecl StartupFormat005e5680(StartupString*,const char*,...);}
extern "C" {
extern void* kotor_cell_rules_007a3a28;
void __fastcall kotor_fastcall_004088d0(void*,void*,void*);void __fastcall kotor_fastcall_005df580(void*,void*);void* __fastcall kotor_fastcall_005e6890(void*,void*,void*,unsigned);int __fastcall kotor_fastcall_00414110(void*,void*,void*,void*,unsigned*);
void __fastcall recovered_save_chargen_006123e0(void*,void*);
void __fastcall frame_LogStringCtor_005e5a90(void*,void*,const char*);void __fastcall frame_LogStringDtor_005e5c20(void*,void*);const char* __fastcall frame_LogCStr_005e5670(void*,void*);unsigned __fastcall async_string_length_005e5790(void*,void*);void __fastcall GuiAssign005e5140(void*,void*,const char*);void* __fastcall async_assign_string_005e5c50(void*,void*,void*);void* __fastcall WindowCopy005e5b00(void*,void*,void*);void* __fastcall async_ref_construct_string_00406d60(void*,void*,void*);
unsigned __fastcall net_layer_send_message_to_player_005d4cb0(void*,void*,unsigned,const void*,unsigned,unsigned);
void __fastcall kotor_fastcall_005eda60(void*,void*,unsigned,unsigned);void __fastcall kotor_m15_client_set_load_screen_by_module_005edcf0(void*,void*,const void*);void __fastcall kotor_m15_client_show_load_screen_005eda80(void*,void*,unsigned,unsigned,unsigned);void __fastcall kotor_m15_client_set_load_screen_hint_005edf00(void*,void*,unsigned);
int __fastcall kotor_m15_c2da_get_string_entry_00413de0(void*,void*,void*,void*,void*);int __fastcall c2da_current_get_int_00413660(void*,void*,unsigned,void*,unsigned*);
}

namespace kotor_menu_window_cells {extern void* volatile manager_007a39fc;}
namespace kotor_game_prepare {void __fastcall SetGlobalUnpauseableState0062b410(void*,void*,unsigned);}
extern "C" {
 unsigned __fastcall recovered_create_server_00401380(void*,void*,unsigned);
 void __fastcall kotor_m15c_main_chargen_remove_sub_panels_006ea830(void*,void*);
 void __fastcall kotor_fastcall_005ee120(void*,void*);
 int __fastcall kotor_fastcall_004aebe0(void*,void*,void*);
 void* __fastcall frame_R4_005ed690(void*,void*);
 __declspec(noreturn) void __cdecl kotor_unreachable_trap(unsigned long,const char*);
}
#include "65283e0453-post_save_native.inc"
namespace {
using namespace recovered_begin_play;
struct Native final:Ports {
void* manager() override {return kotor_menu_window_cells::manager_007a39fc;}
void remove_subpanels_006ea830(void* p) override{kotor_m15c_main_chargen_remove_sub_panels_006ea830(p,nullptr);}
void movie_cleanup_005ee120(void* p) override{kotor_fastcall_005ee120(p,nullptr);}
void* getter_005ed690(void* p) override{return frame_R4_005ed690(p,nullptr);}
void unpauseable_0062b410(void* p,std::uint32_t n) override{kotor_game_prepare::SetGlobalUnpauseableState0062b410(p,nullptr,n);}
void string_construct_005b3190(String* p) override{p->data=0;p->length=0;}
void string_literal_005e5a90(String*s,const char*t) override {frame_LogStringCtor_005e5a90(s,nullptr,t);}
void string_destroy_005e5c20(String*s) override {frame_LogStringDtor_005e5c20(s,nullptr);}
void create_server_00401380(void* m,std::uint32_t n) override {recovered_create_server_00401380(m,nullptr,n);}
void save_character_006123e0(void*s) override {recovered_save_chargen_006123e0(s,nullptr);}
String* getter_string_005ed490(void*c,String*out) override {ClientPorts p;return reinterpret_cast<String*>(recovered_client_begin::name_005ed490(c,reinterpret_cast<recovered_client_begin::String*>(out),p));}
void* net_layer_slot10(void*c) override {return post_network(c);}
void net_start_005d5040(void*n,unsigned a,const String*b,const String*c,unsigned d,unsigned e) override {NetPorts p;recovered_net_connect::start_005d5040(n,a,reinterpret_cast<const recovered_net_connect::String*>(b),reinterpret_cast<const recovered_net_connect::String*>(c),d,e,p);}
void* getter_005ed6f0(void*c) override {ClientPorts p;return recovered_client_begin::message_005ed6f0(c,p);}
void ui_begin_006775e0(void*) override {ClientPorts p;recovered_client_begin::begin_006775e0(p);}
void format_005e5680(String*out,const char*f,unsigned c,const char*a,const char*b,unsigned d,unsigned e) override {kotor_reconstruction::StartupFormat005e5680(reinterpret_cast<kotor_reconstruction::StartupString*>(out),f,c,a,b,d,e);}
unsigned length_005e5790(String*s) override {return async_string_length_005e5790(s,nullptr);}
const char* cstr_005e5670(String*s) override {return frame_LogCStr_005e5670(s,nullptr);}
void net_send_005d4cb0(void*n,unsigned r,const char*d,unsigned size,unsigned f) override {net_layer_send_message_to_player_005d4cb0(n,nullptr,r,d,size,f);}
void set_temp_005edaa0(void*c,const String*s) override {ClientPorts p;recovered_client_begin::temp_005edaa0(c,reinterpret_cast<const recovered_client_begin::String*>(s),p);}
void set_flags_005eda60(void*c,unsigned a,unsigned b) override {kotor_fastcall_005eda60(c,nullptr,a,b);}
void set_module_005edcf0(void*c,const String*s) override {kotor_m15_client_set_load_screen_by_module_005edcf0(c,nullptr,s);}
String* get_module_005edd00(void*,String*out,const String*s) override {LoadingPorts p;return reinterpret_cast<String*>(recovered_loading_lookup::name_005f3650(reinterpret_cast<recovered_loading_lookup::String*>(out),reinterpret_cast<const recovered_loading_lookup::String*>(s),p));}
void use_module_005edd30(void*c,const String*s) override {ClientPorts p;recovered_client_begin::screen_005edd30(c,reinterpret_cast<const recovered_client_begin::String*>(s),p);}
void connect_flags_005eda80(void*c,unsigned a,unsigned b,unsigned d) override {kotor_m15_client_show_load_screen_005eda80(c,nullptr,a,b,d);}
unsigned get_connect_005ee090(void*c,const String*s,unsigned m) override {LoadingPorts p;return recovered_loading_lookup::hint_005f4760(post_ptr(c,4),reinterpret_cast<const recovered_loading_lookup::String*>(s),m,p);}
void connect_005edf00(void*c,unsigned h) override {kotor_m15_client_set_load_screen_hint_005edf00(c,nullptr,h);}
};
}
extern "C" void __fastcall kotor_fastcall_006eb320(void* self,void*){Native p;recovered_begin_play::main_begin_006eb320(self,p);}

extern "C" unsigned __fastcall recovered_save_allowed_004b20e0(void*,void*,const recovered_loading_lookup::String* module){LoadingPorts p;return recovered_module_query::save_allowed_004b20e0(module,p);}

// CServerExoAppInternal::GetModulePreviouslyVisited, which the bridged CServerExoApp thunk 004AEBE0 forwards to.
extern "C" unsigned __fastcall kotor_fastcall_004b51a0(void*,void*,const recovered_loading_lookup::String* module){LoadingPorts p;return recovered_module_query::query_004b51a0(module,p);}

extern "C" unsigned __fastcall recovered_loading_hint_005f4760(void* inner,void*,const recovered_loading_lookup::String* module,unsigned mode){LoadingPorts p;return recovered_loading_lookup::hint_005f4760(inner,module,mode,p);}

// Support export only: the original005D5040 body remains in0861 and ports in0841.
extern "C" unsigned __fastcall recovered_support_net_start_005d5040(
    void* net,void*,unsigned first,const recovered_net_connect::String* name,
    const recovered_net_connect::String* password,unsigned fourth,unsigned mode){
    NetPorts ports;
    return recovered_net_connect::start_005d5040(net,first,name,password,fourth,mode,ports);
}

// Support export only: genuine name copy uses existing0862 with existing0841 ports.
extern "C" recovered_client_begin::String* __fastcall recovered_support_client_name_005ed490(
    void* client,void*,recovered_client_begin::String* output){
    ClientPorts ports;
    return recovered_client_begin::name_005ed490(client,output,ports);
}

// Support getter: returns the existing borrowed client message object.
extern "C" void* __fastcall recovered_support_client_message_005ed6f0(
    void* client,void*){
    ClientPorts ports;
    return recovered_client_begin::message_005ed6f0(client,ports);
}

// Original RequestStatus ignores its message receiver and has no stack arguments.
extern "C" unsigned __fastcall recovered_support_request_status_006775e0(
    void*,void*){
    ClientPorts ports;
    return recovered_client_begin::begin_006775e0(ports);
}
