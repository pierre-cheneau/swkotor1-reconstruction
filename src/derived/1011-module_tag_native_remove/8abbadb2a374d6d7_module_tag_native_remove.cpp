#include "c109ae92e469d9e9_module_tag.hpp"
#include <cstring>
#include <new>
extern "C" {void* __fastcall WindowCopy005e5b00(void*,void*,void*);const char* __fastcall WindowCStr005e5670(void*,void*);void __fastcall frame_LogStringDtor_005e5c20(void*,void*);}
// CExoArrayList<CSWSTagNode>::Insert: byte-identical matched code (src/bridge); its CSWSTagNode passes by value.
namespace bridged_tag { struct Node { unsigned char bytes[40]; }; }
extern "C" void __fastcall kotor_fastcall_004c6520(void*, void*, bridged_tag::Node, int);
namespace kotor_reconstruction{struct StartupString{int __thiscall BorrowedEquals005e5290(const char*);};}
namespace{using U=unsigned;void*ptr(U a){return reinterpret_cast<void*>(a);}U word(const void*p){return reinterpret_cast<U>(p);}struct Native:recovered_module_tag::Ports{U read(U a)override{U v;std::memcpy(&v,ptr(a),4);return v;}U byte(U a)override{return *static_cast<unsigned char*>(ptr(a));}void write(U a,U v)override{std::memcpy(ptr(a),&v,4);}void put_byte(U a,U v)override{*static_cast<unsigned char*>(ptr(a))=static_cast<unsigned char>(v);}U allocate(U n)override{return word(::operator new(n));}void release(U a)override{::operator delete(ptr(a));}void string_copy(U a,U b)override{WindowCopy005e5b00(ptr(a),nullptr,ptr(b));}U text(U a)override{return word(WindowCStr005e5670(ptr(a),nullptr));}U equal(U a,U b)override{return static_cast<unsigned>(static_cast<kotor_reconstruction::StartupString*>(ptr(a))->BorrowedEquals005e5290(static_cast<const char*>(ptr(b))));}void destroy(U a)override{frame_LogStringDtor_005e5c20(ptr(a),nullptr);}void insert_004c6520(U list,U node,U index)override{bridged_tag::Node n;std::memcpy(&n,ptr(node),sizeof n);kotor_fastcall_004c6520(ptr(list),nullptr,n,static_cast<int>(index));}};}
extern "C" unsigned __fastcall recovered_module_register_tag_004c7de0(void*self,void*,void*tag,unsigned id){Native p;alignas(8)unsigned char scratch[48];return recovered_module_tag::add(word(self),word(tag),id,word(scratch),p);}
#include "61584bab4587649a_module_tag_remove.hpp"
extern "C" unsigned __fastcall recovered_module_remove_tag_004c5ef0(void*self,void*,void*tag,unsigned id){Native p;unsigned scratch[2];return recovered_module_tag::remove(word(self),word(tag),id,word(scratch),p);}
