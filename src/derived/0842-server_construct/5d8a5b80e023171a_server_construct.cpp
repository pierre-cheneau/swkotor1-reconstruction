#include "a5884536589ccff6_server_construct.hpp"
#include <cstring>
#include <cstddef>
#include <initializer_list>
namespace recovered_server_create {
namespace {
void w32(unsigned char* p, std::size_t o, std::uint32_t v) { std::memcpy(p+o,&v,4); }
void wptr(unsigned char* p, std::size_t o, void* v) { w32(p,o,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(v))); }
void* list(const Providers& p) { void* v=p.allocate(4); return v ? p.list_construct(v) : nullptr; }
}
void* construct_internal_004b55e0(void* object,const Providers& p) {
 auto* s=static_cast<unsigned char*>(object);
 // Preserve original write/call order and untouched bytes; do not zero the whole object.
 w32(s,0x10020,0); w32(s,0x10024,0); w32(s,0x1001c,0);
 p.string_construct(s+0x10084); p.string_construct(s+0x1008c); p.string_construct(s+0x10094);
 w32(s,0x100f0,0); w32(s,0x100f4,0); w32(s,0x100ec,0);
 p.embedded_construct_0052b170(s+0x100fc); p.embedded_construct_00563d20(s+0x1b770);
 p.string_construct(s+0x1b908); p.string_construct(s+0x1b93c);
 w32(s,0x10028,0); w32(s,0x1002c,0); w32(s,0x1006c,0); w32(s,0x10004,0);
 s[0x10008]=0; s[0x10009]=0;
 w32(s,0x10060,p.invalid_object);
 wptr(s,0x10064,list(p)); wptr(s,0x10068,list(p));
 w32(s,0x10014,0); w32(s,0x10010,0); p.publish_internal(s);
 auto* players=static_cast<unsigned char*>(p.allocate(0x40)); wptr(s,0x10018,players);
 // The original dereferences this allocation without a null guard.
 for(std::size_t o=0;o<0x40;o+=4) w32(players,o,p.invalid_player);
 s[0x10078]=0; w32(s,0x1007c,0); w32(s,0x10080,0); p.string_assign(s+0x10084,"");
 w32(s,0x100b4,0); w32(s,0x100b8,0); w32(s,0x100bc,0); w32(s,0x100c0,0);
 wptr(s,0x10070,list(p)); wptr(s,0x10074,list(p));
 w32(s,0x100c8,0); w32(s,0x100f8,0); p.string_assign(s+0x1b908,"");
 w32(s,0x1b910,0); w32(s,0x1b914,0); w32(s,0x100c4,0); w32(s,0x100d8,0);
 w32(s,0x1b918,0x7f000000); w32(s,0x1b91c,0); w32(s,0x1b920,0x7f000000);
 w32(s,0x1b924,0); w32(s,0x100b0,0); w32(s,0x100ac,0); w32(s,0x1b930,0);
 w32(s,0x1b928,0); w32(s,0x1b92c,0); w32(s,0x1b938,0); w32(s,0x1b934,0);
 p.string_assign(s+0x1b93c,""); return s;
}
}
namespace recovered_server_create {
void* construct_list_internal_005e9aa0(void* object) {
 auto* s=static_cast<unsigned char*>(object);
 w32(s,4,0); w32(s,0,0); w32(s,8,0); return s;
}
void* construct_list_outer_004b25d0(void* object,void* (*allocate)(std::uint32_t)) {
 void* storage=allocate(12);
 void* internal=storage ? construct_list_internal_005e9aa0(storage) : nullptr;
 wptr(static_cast<unsigned char*>(object),0,internal); return object;
}
}
namespace recovered_server_create {
void* construct_embedded_0052b170(void* object,void (*string_construct)(void*)) {
 auto* s=static_cast<unsigned char*>(object);
 // 100 ordinary 004CA7A0 element constructors, then five strings.
 for(std::size_t i=0;i<100;++i)for(std::size_t j=0;j<24;j+=4)w32(s,0xa8d0+i*24+j,0);
 for(std::size_t i=0;i<5;++i)string_construct(s+0xb230+i*8);
 for(std::uint32_t i=0;i<256;++i){std::uint32_t v=i;for(int bit=0;bit<8;++bit)v=(v>>1)^((v&1)?0xedb88320u:0u);w32(s,0xb268+i*4,v);}
 std::memset(s,0,0x299a*4);
 for(auto o:{0xb258,0xb25c,0xb260,0xb264,0xb668,0xb66c,0xb670})w32(s,o,0);
 std::memset(s+0xa668,0,0x71);std::memset(s+0xa6d9,0,0x1f4);
 std::memset(s+0xa8d0,0,0x960);std::memset(s+0xb230,0,0x28);
 return s;
}
}
namespace recovered_server_create {
void* construct_embedded_00563d20(void* object) {
 auto* s=static_cast<unsigned char*>(object);
 // 00563D20 clears the two owned pointers before calling00563200.
 // Consequently its two destructor branches are unreachable on this constructor path.
 w32(s,0x118,0);w32(s,0x11c,0);
 w32(s,0xfc,0);w32(s,0xf4,0);w32(s,0xf0,0xffffffff);w32(s,0xf8,0);
 w32(s,0x100,1);w32(s,0x104,0x7db3);w32(s,0x108,0xa488);w32(s,0x114,0);w32(s,0x10c,0);
 for(std::size_t o=0;o<64;o+=4){w32(s,0x60+o,0);w32(s,0xa0+o,0);}
 w32(s,0xe0,0xffffffff);w32(s,0x190,0);w32(s,0x194,0);
 w32(s,0,0);w32(s,0xe4,0);w32(s,0xe8,0);w32(s,0xec,0xffffffff);w32(s,0x110,0);
 for(std::size_t i=0;i<9;++i){w32(s,0x30+i*4,0);s[0x54+i]=1;w32(s,0xc+i*4,0x7f000000);}
 for(std::size_t o=0x120;o<=0x130;o+=4)w32(s,o,2);
 std::memset(s+0x134,0,0x34);for(std::size_t o=0x168;o<0x190;o+=4)w32(s,o,0xffffffff);
 return s;
}
}
// CExoLinkedListInternal::CExoLinkedListInternal, which the bridged CExoLinkedList<T> constructors forward to.
extern "C" void* __fastcall kotor_fastcall_005e9aa0(void* self, void*) {
 return recovered_server_create::construct_list_internal_005e9aa0(self);
}
