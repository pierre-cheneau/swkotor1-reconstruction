extern "C" void __cdecl live_free(void*); // 1118 single heap: CRT free -> game CRT heap _live_free 006FA390
extern "C" const void* const kotor_vtable_cswinventory_0074a824[]; // CSWInventory's table, defined after the providers
extern "C" void* __fastcall kotor_fastcall_005a4930(void*,void*);       // CSWInventory::CSWInventory (src/bridge)
extern "C" void __fastcall recovered_item_ai_update_0055cb60(void*,void*);
#include "e1b8864aa7-object_destruct.hpp"
#include "d44276b3a8-object_cleanup_arrays.hpp"
#include "87b27e2d24-m50_vtables.h"
#include "a89e7d9860-base_ini.h"
#include "158a6c4f90-startup_string.h"
#include <cstdlib>
extern "C" void __fastcall recovered_item_destroy_0055ec70(void*,void*);
extern "C" unsigned char __fastcall recovered_object_array_remove_004d81a0(void*,void*,unsigned,void**);
extern "C" void __fastcall exo_locstring_destroy_005ea000(void*,void*);
extern "C" void __stdcall c2da_destroy_array_006fa852(void*,unsigned,unsigned,void(__fastcall*)(kotor_reconstruction::StartupString*,void*));
namespace base_ini_reconstruction {void __fastcall destroy_list_005e9ab0(List*,void*);void* __fastcall remove_head_005e9c80(List*,void*);}
#include "c9562a31ae-placeable_constructor.hpp"
extern "C" void* __fastcall recovered_placeable_mesh_construct_005cdaa0(void*,void*);
#include "bcd202555c-sound_constructor.hpp"
extern "C" void __fastcall void_return_14_005c8650(void*,void*,unsigned,unsigned,unsigned,unsigned,unsigned);
extern "C" unsigned __fastcall recovered_area_update_00506fe0(void*,void*,void*);
#include "9efd7cef8e-waypoint_constructor.hpp"
extern "C" void __fastcall kotor_m15e_slot090_void0_004015a0(void*,void*);
#include "5856371129-trigger_constructor.hpp"
namespace c2da_current{struct Table;}
namespace kotor_reconstruction{struct StartupString;}
extern "C" {float recovered_trigger_primary_range_007a206c=1.0f;int __fastcall c2da_current_get_float_00413430(c2da_current::Table*,void*,int,kotor_reconstruction::StartupString*,float*);void __fastcall frame_LogStringCtor_005e5a90(void*,void*,const char*);void __fastcall frame_LogStringDtor_005e5c20(void*,void*);}
extern "C" void __fastcall recovered_object_portrait_004cad20(void*,void*,unsigned);
extern "C" void __fastcall recovered_object_portrait_ref_004d01f0(void*,void*,void*);
#include "8c730e5e2e-object_state.hpp"
#include "1c184649aa-door_constructor.hpp"
extern "C" unsigned __stdcall c2da_resource_constant_word_00406490();
extern "C" void __fastcall recovered_creature_priority_004f0d70(void*,void*,unsigned);
extern "C" void __fastcall recovered_creature_effect_index_004f14f0(void*,void*);
extern "C" void __fastcall recovered_creature_effect_added_004ffaf0(void*,void*,void*);
#include "6d65c1a47f-item_constructor.hpp"
extern "C" {void __fastcall recovered_creature_portrait_004ed4c0(void*,void*,unsigned);void __fastcall recovered_creature_portrait_ref_004f83c0(void*,void*,void*);}
#include "8a9e548803-constructor_tail.hpp"
extern "C" {unsigned __fastcall kotor_fastcall_004aea40(void*,void*);void* __fastcall kotor_fastcall_004ae910(void*,void*,unsigned);unsigned __fastcall unresolved_list_at_pos_005e9b60(void*,void*,void*);unsigned __fastcall unresolved_list_next_005e9b90(void*,void*,void**);}
#include "40aa5f3766-creature_children.hpp"
#include "4cfa2b9bcf-party_health.hpp"
extern "C" void* __fastcall kotor_fastcall_005ed580(void*,void*,unsigned);
#include "7836e2e673-creature_health.hpp"
extern "C" float recovered_health_threshold_007a1b38=0.2f;
#include "59b497b5f7-creature_priority.hpp"
#include "ebbe124c2a-menu_targets.h"
#include "75b72a3cd0-ai_registration.hpp"
#include "ee59127831-combat_constructor.hpp"
#include "5d49ae355f-creature_constructor.hpp"
#include "8f31e5bb5b-object_constructor.hpp"
#include "0b067cb742-stats_constructor.hpp"
#include "63ba54e5a5-object_registration.hpp"
#include <cstring>
#include <new>
extern "C" {__declspec(noreturn) void __cdecl kotor_unreachable_trap(unsigned long,const char*);void* __fastcall exo_locstring_construct_005e9e10(void*,void*);void* __fastcall c2da_default_ref_00405ed0(void*,void*);void* __fastcall GuiAssign005e5140(void*,void*,const char*);unsigned __cdecl aurora_vector_equal_004aa980(void*,void*);unsigned char __fastcall kotor_fastcall_004d8230(void*,void*,unsigned,void**);void* __fastcall kotor_fastcall_004aede0(void*,void*);void __fastcall frame_GetWorldTime_004ade40(void*,void*,unsigned*,unsigned*);int __cdecl crt_rand_006fc447();}
extern "C" {extern void* kotor_cell_rules_007a3a28;void* __fastcall unresolved_ref_assign_00406290(void*,void*,void*,const char*);void* __fastcall kotor_m15_combat_information_construct_00552280(void*,void*);}
extern "C" void* __fastcall recovered_collision_construct_005d0ce0(void*,void*);
extern "C" {unsigned __fastcall kotor_fastcall_004b1f70(void*,void*,unsigned);void* __fastcall kotor_fastcall_005ed8b0(void*,void*);void* __fastcall frame_GetCharacter_006346c0(void*,void*,unsigned);unsigned __fastcall kotor_fastcall_005eda50(void*,void*,unsigned);}
namespace kotor_reconstruction{struct ArrayMutations;}
namespace menu_list_growth{void __fastcall allocate_005e03c0(kotor_reconstruction::ArrayMutations*,void*,unsigned);}
namespace kotor_menu_window_cells{extern void* volatile manager_007a39fc;}
namespace menu_resources{struct Ref{Ref* construct_00406d80(const char*);};} // 00406D80 CResRef::CResRef(char const*) (0030-service-resource-13)
namespace frame_control{extern unsigned resource_manager_007a39e8;} // 007A39E8 CExoResMan* (0151-manager-cell-owner)
namespace table29_required{void* __fastcall gui_game_object_004d7d60(void*,void*,unsigned,unsigned);}
namespace recovered_server_create{void* construct_list_internal_005e9aa0(void*);}
namespace{using U=unsigned;void* ptr(U a){return reinterpret_cast<void*>(a);}U word(const void*p){return reinterpret_cast<U>(p);}__declspec(noreturn)void required(U a){kotor_unreachable_trap(a,"Server creature constructor required provider");}
struct M:kotor::server_mainloop::Memory{U read(U a)override{U v;std::memcpy(&v,ptr(a),4);return v;}void write(U a,U v)override{std::memcpy(ptr(a),&v,4);}U read16(U a)override{unsigned short v;std::memcpy(&v,ptr(a),2);return v;}void write16(U a,U v)override{unsigned short n=static_cast<unsigned short>(v);std::memcpy(ptr(a),&n,2);}U read8(U a)override{return *static_cast<unsigned char*>(ptr(a));}void byte(U a,U v){*static_cast<unsigned char*>(ptr(a))=static_cast<unsigned char>(v);}U server(){return read(word(kotor_menu_window_cells::manager_007a39fc)+8);}};
struct Alloc:kotor::server_mainloop::ModuleAllocation{U allocate(U n)override{return word(::operator new(n));}};
void* __fastcall constant_zero(void*,void*){return nullptr;}void* __fastcall constant_self(void*s,void*){return s;}U __fastcall constant_one(void*,void*){return 1;}
unsigned __fastcall object_state_004ccf60(void*self,void*,unsigned value){M m;return recovered_object_state::set(word(self),value,m);}
unsigned __fastcall current_hp(void*,void*,unsigned);
unsigned short __fastcall maximum_hp(void*,void*,unsigned);
unsigned __fastcall creature_dead(void*,void*);
#include "0e5e408797-claude_0963_bodies_decl.inc"
#include "88d1914f94-constructor_vtables_object_portrait.inc"
struct Object:recovered_server_object::Ports{M m;U read(U a)override{return m.read(a);}void write(U a,U v)override{m.write(a,v);}void byte(U a,U v)override{m.byte(a,v);}void half(U a,U v)override{m.write16(a,v);}U cell(U a)override{if(a==0x746644)return 0x7f000000;required(a);}U table()override{return word(object_table);}void object(U a,U b,U c)override{table29_required::gui_game_object_004d7d60(ptr(a),nullptr,b,c);}void localized(U a)override{exo_locstring_construct_005e9e10(ptr(a),nullptr);}void string(U a)override{write(a,0);write(a+4,0);}void ref(U a)override{c2da_default_ref_00405ed0(ptr(a),nullptr);}void queue(U a)override{void*p=::operator new(12);write(a,p?word(recovered_server_create::construct_list_internal_005e9aa0(p)):0);}void location(U a)override{(void)kotor::server_mainloop::module_flags_0059afe0(m,a);}U equal(U a,U b)override{return aurora_vector_equal_004aa980(ptr(a),ptr(b));}U array()override{return read(read(m.server()+4)+0x1005c);}void lookup(U a,U id,U out)override{(void)kotor_fastcall_004d8230(ptr(a),nullptr,id,static_cast<void**>(ptr(out)));}U slot(U a,U o)override{using F=void*(__thiscall*)(void*);return word(reinterpret_cast<F>(read(read(a)+o))(ptr(a)));}void notify(U a,U b)override{(void)recovered_area_update_00506fe0(ptr(a),nullptr,ptr(b));}void register_object(U a,U b,U c,U d)override{Alloc p;(void)kotor::server_mainloop::object_register_004d80e0(m,p,a,b,c,d);}void insert(U a,U id,U object)override{(void)body_004d7e90(ptr(a),nullptr,id,ptr(object));}void literal(U a,const char*s)override{GuiAssign005e5140(ptr(a),nullptr,s);}U allocate(U n)override{return word(::operator new(n));}U timer()override{return word(kotor_fastcall_004aede0(ptr(m.server()),nullptr));}void time(U a,U b,U c)override{frame_GetWorldTime_004ade40(ptr(a),nullptr,static_cast<unsigned*>(ptr(b)),static_cast<unsigned*>(ptr(c)));}};
#include "60122381fc-stats_native.inc"
#include "9f18bd8ddf-combat_native.inc"
#include "6031202393-ai_native.inc"
#include "9be34dc5f2-health_native.inc"
#include "35d832c838-party_health_native.inc"
#include "437ca5550b-priority_native_party_health.inc"
#include "5858ece5b1-children_native.inc"
#include "17a07744f7-tail_native.inc"
#include "b0c1d9d504-item_native_ai_update.inc"
struct Creature:recovered_server_creature::Ports{M m;U read(U a)override{return m.read(a);}void write(U a,U v)override{m.write(a,v);}void byte(U a,U v)override{m.byte(a,v);}void half(U a,U v)override{m.write16(a,v);}U cell(U a)override{if(a==0x746634)return 5;if(a==0x746644)return 0x7f000000;required(a);}U vtable()override{return word(creature_table);}U allocate(U n)override{return word(::operator new(n));}void base(U a,U type,U id,U flag)override{Object p;alignas(8)unsigned char scratch[16];(void)recovered_server_object::construct(a,type,id,flag,word(scratch),p);}void string_empty(U a)override{write(a,0);write(a+4,0);}void action_empty(U a)override{write(a+12,0);write(a+16,0);write(a+20,0);half(a,0xffff);half(a+2,0xffff);half(a+4,0xffff);write(a+12,0);write(a+16,0);write(a+20,0);write(a+24,0x7f000000);write(a+8,0xffffffff);}U child(U address,U self,U owner,U extra)override{if(address==0x5aca80){Stats p;alignas(8)unsigned char scratch[16];return recovered_server_stats::construct(self,owner,word(scratch),p);}if(address==0x5a4930)return word(kotor_fastcall_005a4930(ptr(self),nullptr));if(address==0x55d290){Children p;return recovered_creature_children::inventory(self,owner,extra,p);}if(address==0x4d5cb0){Combat p;return recovered_combat_round::construct(self,owner,p);}if(address==0x5d0ce0)return word(recovered_collision_construct_005d0ce0(ptr(self),nullptr));required(address);}U server(U address)override{if(address==0x4aede0)return word(kotor_fastcall_004aede0(ptr(m.server()),nullptr));if(address==0x4aed80)return read(read(m.server()+4)+0x10044);required(address);}void register_ai(U a,U b,U c)override{Ai p;(void)recovered_ai_registration::add(a,b,c,p);}void priority(U a,U v)override{Priority p;recovered_creature_priority::set(a,v,p);}void literal(U a,const char*s)override{GuiAssign005e5140(ptr(a),nullptr,s);}int random()override{return crt_rand_006fc447();}void time(U a,U b,U c)override{frame_GetWorldTime_004ade40(ptr(a),nullptr,static_cast<unsigned*>(ptr(b)),static_cast<unsigned*>(ptr(c)));}void effect(U s,U id,U a,U b,U c,U d)override{Children p;recovered_creature_children::perception(s,id,a,b,c,d,p);}void update(U self,U value)override{Tail p;if(value==0x1f40){unsigned scratch;recovered_constructor_tail::update_timer(self,value,word(&scratch),p);}else recovered_constructor_tail::enable(self,value,p);}};
}
extern "C" const void* const* const kotor_recovered_creature_vtable_004f8410 = creature_table;
// CSWInventory's vtable, original 0074A824: one slot, the scalar deleting destructor. Named after its original
// address, like the other playable tables, so matched code (src/bridge) binds its constructor to it.
extern "C" const void* const kotor_vtable_cswinventory_0074a824[] = {reinterpret_cast<void*>(&equipment_delete)};
extern "C" void* __fastcall recovered_server_creature_construct_004f7a10(void*s,void*,unsigned id,unsigned flag){Creature p;alignas(8)unsigned char scratch[8];return ptr(recovered_server_creature::construct(word(s),id,flag,word(scratch),p));}

extern "C" unsigned char __fastcall recovered_stats_level_005a5fd0(void*s,void*,unsigned arg){Health p;return static_cast<unsigned char>(recovered_creature_health::level(word(s),arg,p));}
extern "C" void* __fastcall recovered_stats_level_info_005a6170(void*s,void*,unsigned i){Health p;return ptr(recovered_creature_health::level_info(word(s),i,p));}
extern "C" unsigned __fastcall recovered_stats_has_feat_005a6630(void*s,void*,unsigned id){Health p;return recovered_creature_health::has_feat(word(s),id,p);}

extern "C" void* __fastcall recovered_item_construct_005530a0(void*self,void*,unsigned id){Item p;return ptr(recovered_item_constructor::construct(word(self),id,p));}

extern "C" unsigned __fastcall recovered_creature_incapacitated_004ef890(void*self,void*){PartyHealth p;return recovered_party_health::incapacitated(word(self),p);}

extern "C" void __fastcall recovered_creature_priority_004f0d70(void*self,void*,unsigned state){Priority p;recovered_creature_priority::set(word(self),state,p);}

namespace{
#include "c31522be87-door_native_portrait.inc"
}
extern "C" void* __fastcall recovered_door_construct_00589ee0(void*self,void*,unsigned id){Door p;return ptr(recovered_door_constructor::construct(word(self),id,p));}

namespace{
#include "60c99db603-trigger_native.inc"
}
extern "C" void* __fastcall recovered_trigger_construct_0058eae0(void*self,void*,unsigned id){Trigger p;alignas(4)unsigned scratch[2];return ptr(recovered_trigger_constructor::construct(word(self),id,word(scratch),p));}

namespace{
#include "1c9497abbf-waypoint_native.inc"
}
extern "C" void* __fastcall recovered_waypoint_construct_005c7e70(void*self,void*,unsigned id){Waypoint p;return ptr(recovered_waypoint_constructor::construct(word(self),id,p));}

namespace{
#include "489859dc96-sound_native.inc"
}
extern "C" void* __fastcall recovered_sound_construct_005c8f30(void*self,void*,unsigned id){Sound p;return ptr(recovered_sound_constructor::construct(word(self),id,p));}

namespace{
#include "5ffa66856c-placeable_native.inc"
}
extern "C" void* __fastcall recovered_placeable_construct_005877e0(void*self,void*,unsigned id){Placeable p;unsigned scratch[4];return ptr(recovered_placeable_constructor::construct(word(self),id,word(scratch),p));}
#include "ec54558280-object_destruct_native.inc"

extern "C" void __fastcall recovered_object_array_erase_004d8290(void*self,void*,unsigned id){ObjectDestruct p;p.remove(word(self),id);}

// Original005CB1B0 is already owned by0962::level_info; expose existing0963 Stats.
extern "C" void __fastcall exo_array_list_destruct_0056b6f0(void*,void*);
static void __fastcall history_array_destroy_0056b6f0(kotor_reconstruction::StartupString* a,void*) { exo_array_list_destruct_0056b6f0(a,nullptr); }
extern "C" void* __fastcall recovered_history_level_ctor_005cb1b0(void* object,void*) {
 Stats p;
 try { return ptr(recovered_server_stats::level_info(word(object),p)); }
 catch(...) {
  // Existing alias initialized all three arrays before its sole throwing skill allocation.
  exo_array_list_destruct_0056b6f0(static_cast<unsigned char*>(object)+0x18,nullptr);
  c2da_destroy_array_006fa852(static_cast<unsigned char*>(object)+0x0c,12,1,history_array_destroy_0056b6f0);
  c2da_destroy_array_006fa852(object,12,1,history_array_destroy_0056b6f0);
  throw;
 }
}

// Issue #87 fixture transport: expose the existing file-local Stats ports and
// selected 005ACA80 construct. This adds no reconstructed address or behavior.
extern "C" void* __fastcall recovered_stats_construct_005aca80(void* self,void*,void* owner){
 Stats p;
 alignas(8) unsigned char scratch[16];
 return ptr(recovered_server_stats::construct(word(self),word(owner),word(scratch),p));
}

// Issue #88 fixture transport: expose the already-selected, file-local GetDead
// body defined by party_health_native.inc above. That body is the faithful
// 004EF820 -> 004CB810 chain (client-party membership, then base_dead through the
// object's own +30/+9C virtuals). This facade adds no body, address or behavior.
extern "C" unsigned __fastcall recovered_creature_dead_004ef820(void* self,void*){
 return creature_dead(self,nullptr);
}

// 0963-gates: faithful bodies for the former required() vtable/port gates (see claude_0963_bodies_decl.inc).
namespace{
#include "5a47792f2b-claude_0963_bodies.inc"
}
