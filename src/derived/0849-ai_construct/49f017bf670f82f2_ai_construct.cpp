#include "2fa59ec2bac40639_ai_construct.hpp"
#include <cstring>
#include <initializer_list>
namespace recovered_ai_construct {
namespace {void word(void*p,unsigned o,unsigned v){std::memcpy((char*)p+o,&v,4);}void put(void*p,unsigned o,const void*v){word(p,o,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(v)));}}
void* construct_004b0780(void*self,Ports&p){
 for(unsigned n=0;n<5;++n){void*e=(char*)self+4+n*16;for(unsigned o:{4U,8U,0U,12U})word(e,o,0);}
 p.event_queue((char*)self+0x54);
 p.string_default((char*)self+0x60);
 void*first=p.allocate(12);if(first)put(first,0,p.table1());put(self,0x58,first);p.initialize_table1(first);
 void*second=p.allocate(12);if(second)put(second,0,p.table2());put(self,0x5c,second);p.initialize_table2(second);
 for(unsigned o:{0x80U,0x84U,0x88U,0x8cU})word(self,o,0);
 return self;
}
}
