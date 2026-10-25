#pragma once
#include <cstdint>
namespace recovered_module_tag{using U=std::uint32_t;struct Ports{virtual ~Ports()=default;virtual U read(U)=0;virtual U byte(U)=0;virtual void write(U,U)=0;virtual void put_byte(U,U)=0;virtual U allocate(U)=0;virtual void release(U)=0;virtual void string_copy(U,U)=0;virtual U text(U)=0;virtual U equal(U,U)=0;virtual void destroy(U)=0;
// CExoArrayList<CSWSTagNode>::Insert 004C6520 (thiscall on the list, the 40-byte node by value, RET 2Ch)
virtual void insert_004c6520(U list,U node,U index)=0;};U lower_bound(U,U,Ports&);void grow(U,U,Ports&);void insert(U,U,U,Ports&);U add(U,U,U,U,Ports&);}
