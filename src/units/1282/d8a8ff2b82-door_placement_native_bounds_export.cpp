#include "311d0abb16-door_placement.hpp"
#include "220876d0e1-door_geometry.hpp"
#include "0c3674ffa8-door_area_bounds.hpp"
#include <new>
#include "50af928f4c-creature_area_access.hpp"
#include <cstring>
namespace kotor_reconstruction{struct ArrayMutations;}
namespace menu_list_growth{void __fastcall allocate_005e03c0(kotor_reconstruction::ArrayMutations*,void*,unsigned);}
extern "C"{
unsigned __fastcall recovered_mesh_four_corners_00596e40(void*,void*,void*,void*,void*,void*);
unsigned __fastcall recovered_door_bounds_00589980(void*,void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void*);
void* __fastcall recovered_object_area_004cb120(void*,void*);
void __fastcall recovered_object_position_004cd1a0(void*,void*,void*,unsigned,unsigned,unsigned);
unsigned __fastcall recovered_area_insert_0050dfd0(void*,void*,unsigned,unsigned);
__declspec(noreturn)void __cdecl kotor_unreachable_trap(unsigned long,const char*);
// CSWSDoor::GetMeshOutline: byte-identical matched code (src/bridge); its two Vectors pass by value, 12 bytes each.
int __fastcall kotor_fastcall_00588940(void*,void*,recovered_door_placement::Vector,recovered_door_placement::Vector,void*,void*,void*,void*);
}
// stale-stubs G3: ports rebound (0050DC40 claude_combat_spell_mine, 0066DCC0 W1 b7, 0058FEC0 claude_w1_route_bodies).
extern "C"{unsigned __fastcall kotor_fastcall_0050dc40(void*,void*,unsigned);unsigned __fastcall recovered_int_list_remove_0066dcc0(void*,void*,unsigned);unsigned __fastcall kotor_fastcall_0058fec0(void*,void*,unsigned);}
namespace{using U=unsigned;void*ptr(U a){return reinterpret_cast<void*>(a);}U word(void*p){return reinterpret_cast<U>(p);}
struct Native:recovered_door_placement::Ports,recovered_creature_area_access::Ports,recovered_door_area_bounds::Ports,recovered_door_geometry::Ports{
 U allocate(U n)override{return word(::operator new(n));}void release(U a)override{::operator delete(ptr(a));}
 U read(U a)override{if(a==0x74c394)return 0;U v;std::memcpy(&v,ptr(a),4);return v;}void write(U a,U v)override{std::memcpy(ptr(a),&v,4);}U invalid()override{return 0x7f000000;}
 U area(U a)override{return word(recovered_object_area_004cb120(ptr(a),nullptr));}
 void detach(U a,U id)override{(void)kotor_fastcall_0050dc40(ptr(a),nullptr,id);}void erase_id(U l,U v)override{(void)recovered_int_list_remove_0066dcc0(ptr(l),nullptr,v);}void remove_bounds(U a,U id)override{(void)kotor_fastcall_0058fec0(ptr(a),nullptr,id);}
 void set_area(U a,U b)override{recovered_creature_area_access::set(a,b,*this);}
 void position(U a,U b,U c,U d,U e)override{recovered_object_position_004cd1a0(ptr(a),nullptr,ptr(b),c,d,e);}
 void insert(U a,U b,U c)override{(void)recovered_area_insert_0050dfd0(ptr(a),nullptr,b,c);}
 void grow(U a,U b)override{menu_list_growth::allocate_005e03c0(static_cast<kotor_reconstruction::ArrayMutations*>(ptr(a)),nullptr,b);}
 U bounds(U a,recovered_door_placement::Vector v,recovered_door_placement::Vector d,U lo,U hi)override{return recovered_door_bounds_00589980(ptr(a),nullptr,v.x,v.y,v.z,d.x,d.y,d.z,ptr(lo),ptr(hi));}
 void add_bounds(U a,U id,U lo,U hi)override{(void)recovered_door_area_bounds::append(a,id,lo,hi,*this);}
 U geometry(U a,U b,U c,U d,U e)override{return recovered_mesh_four_corners_00596e40(ptr(a),nullptr,ptr(b),ptr(c),ptr(d),ptr(e));}
 void geometry(U a,recovered_door_placement::Vector v,recovered_door_placement::Vector o,U b,U c,U d,U e)override{(void)kotor_fastcall_00588940(ptr(a),nullptr,v,o,ptr(b),ptr(c),ptr(d),ptr(e));}
};}
extern "C" void __fastcall recovered_door_placement_0058b6d0(void*self,void*,unsigned area,unsigned x,unsigned y,unsigned z,unsigned flag){Native p;unsigned scratch[3];recovered_door_placement::place(word(self),area,x,y,z,flag,word(scratch),p);}

extern "C" unsigned __fastcall recovered_area_bounds_append_0058fd50(void*self,void*,unsigned id,void*lo,void*hi){Native p;return recovered_door_area_bounds::append(word(self),id,word(lo),word(hi),p);}
