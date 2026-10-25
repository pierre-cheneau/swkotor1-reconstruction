#include "2c667a082e-gff_write_complex.hpp"
#include "36abb9d59e-gff_write_fields.hpp"
#include "69de8440f8-gff_write_output.hpp"
#include "ee6caa2a71-gff_write_localized_getters.hpp"
#include "d63b2f9b4b-module_gff.hpp"
#include "abb3810cb7-gff_views.h"
#include <cstring>
#include <new>
#include <cstdint>
using namespace kotor::server_mainloop;
extern "C" {
void* __cdecl live_operator_new(std::uint32_t); void __cdecl live_free(void*); // game CRT heap: _live_operator_new 006FA7E6 / _live_free 006FA390 (decls as allocator_live.h)
extern const char* volatile gff_version_0078d3cc;
void __fastcall frame_LogStringCtor_005e5a90(void*,void*,const char*);void __fastcall frame_LogStringDtor_005e5c20(void*,void*);const char* __fastcall frame_LogCStr_005e5670(void*,void*);unsigned __fastcall async_string_length_005e5790(void*,void*);void* __fastcall async_assign_string_005e5c50(void*,void*,void*);
void* __fastcall c2da_resource_base_ctor_00406460(void*,void*);void* __fastcall gff_resource_deleting_destroy_00413010(void*,void*,unsigned);void __fastcall gff_resource_release_004108c0(void*,void*);unsigned __fastcall gff_resource_serviced_00410740(void*,void*);unsigned __fastcall gff_resource_freed_00410880(void*,void*);unsigned __stdcall c2da_resource_constant_word_00406490();
unsigned __fastcall gff_label_00411630(void*,void*,void*,const char*);unsigned __fastcall unresolved_list_next_005e9b90(void*,void*,void**);
void __fastcall frame_LogFileCtor_005e68d0(void*,void*,void*,unsigned,void*);void __fastcall frame_LogFileDtor_005e69e0(void*,void*);unsigned __fastcall frame_LogFileOpened_005e6a10(void*,void*);unsigned __fastcall kotor_fastcall_005ebc60(void*,void*,void*,unsigned,unsigned);
unsigned char __fastcall kotor_fastcall_00406040(void*,void*); // CResRef::GetLength: byte-identical matched code (src/bridge)
unsigned long __fastcall kotor_fastcall_005e9f00(void*,void*,int); // CExoLocString::GetStringLength: matched (src/bridge)
}
namespace {
void* ptr(Word v){return reinterpret_cast<void*>(v);}Word word(const void*p){return reinterpret_cast<Word>(p);}
struct M:Memory{Word read(Word a)override{if(a==0x78d3cc)return word(gff_version_0078d3cc);Word v;std::memcpy(&v,ptr(a),4);return v;}Word read8(Word a)override{return *static_cast<unsigned char*>(ptr(a));}Word read16(Word a)override{unsigned short v;std::memcpy(&v,ptr(a),2);return v;}void write(Word a,Word v)override{std::memcpy(ptr(a),&v,4);}void write16(Word a,Word v)override{unsigned short s=static_cast<unsigned short>(v);std::memcpy(ptr(a),&s,2);}};
struct Getters:LocalizedGetterPorts{Word next(Word a,Word b)override{return unresolved_list_next_005e9b90(ptr(a),nullptr,static_cast<void**>(ptr(b)));}void assign(Word a,Word b)override{async_assign_string_005e5c50(ptr(a),nullptr,ptr(b));}Word length(Word a)override{return async_string_length_005e5790(ptr(a),nullptr);}void byte(Word a,Word b)override{*static_cast<unsigned char*>(ptr(a))=static_cast<unsigned char>(b);}};
struct P:GffWriteComplexPorts{M&m;explicit P(M&v):m(v){}
Word allocate(Word a)override{return word(live_operator_new(a));}void free(Word a)override{live_free(ptr(a));}void release_004108c0(Word a)override{gff_resource_release_004108c0(ptr(a),nullptr);}void byte(Word a,Word b)override{*static_cast<unsigned char*>(ptr(a))=static_cast<unsigned char>(b);}
Word string_literal(Word a,Word b)override{frame_LogStringCtor_005e5a90(ptr(a),nullptr,static_cast<const char*>(ptr(b)));return a;}void string_destroy(Word a)override{frame_LogStringDtor_005e5c20(ptr(a),nullptr);}Word string_length(Word a)override{return async_string_length_005e5790(ptr(a),nullptr);}Word string_char(Word a,Word b)override{return static_cast<unsigned char>(frame_LogCStr_005e5670(ptr(a),nullptr)[b]);}Word string_text(Word a)override{return word(frame_LogCStr_005e5670(ptr(a),nullptr));}void string_construct(Word a)override{m.write(a,0);m.write(a+4,0);}
void owned(Word a)override{gff_owned_storage_00410aa0(m,*this,a);}Word add_struct(Word a,Word b)override{return gff_add_struct_00410d60(m,*this,a,b);}Word add_field(Word a,Word b,Word c,Word d)override{return gff_add_field_00411730(m,*this,a,b,c,d);}Word grow_list(Word a,Word b,Word c,Word d)override{return gff_grow_buffer(m,*this,a,b,c,d,2);}Word grow_data(Word a,Word b,Word c,Word d)override{return gff_grow_buffer(m,*this,a,b,c,d,0);}
Word field_index(Word a,Word b,Word c)override{return gff_label_00411630(ptr(a),nullptr,ptr(b),static_cast<const char*>(ptr(c)));}Word field(Word a,Word b,Word c)override{return word(static_cast<kotor_q05::CResGFF*>(ptr(a))->GetField(static_cast<kotor_q05::StructHandle*>(ptr(b)),c));}Word data(Word a,Word b,Word c)override{return static_cast<kotor_q05::CResGFF*>(ptr(a))->GetDataLayoutList(static_cast<kotor_q05::FieldRecord*>(ptr(b)),static_cast<Word*>(ptr(c)));}
Word ref_length(Word a)override{return kotor_fastcall_00406040(ptr(a),nullptr);}Word ref_text(Word a)override{return a;}
Word localized_count(Word a)override{return m.read(m.read(a)+4);}Word localized_length(Word a,Word b)override{return kotor_fastcall_005e9f00(ptr(a),nullptr,static_cast<int>(b));}Word localized_get(Word a,Word b,Word c,Word d,Word e)override{Getters p;Word scratch;return localized_get_index_005e9eb0(m,p,a,b,c,d,e,word(&scratch));}
};
struct O:GffOutputPorts{Word allocate(Word a)override{return word(live_operator_new(a));}void free(Word a)override{live_free(ptr(a));}void release_004108c0(Word a)override{gff_resource_release_004108c0(ptr(a),nullptr);}void byte(Word a,Word b)override{*static_cast<unsigned char*>(ptr(a))=static_cast<unsigned char>(b);}Word write(Word a,Word b,Word c,Word d)override{return kotor_fastcall_005ebc60(*static_cast<void**>(ptr(a)),nullptr,ptr(b),c,d);}void string_literal(Word a,Word)override{frame_LogStringCtor_005e5a90(ptr(a),nullptr,"wb");}void string_destroy(Word a)override{frame_LogStringDtor_005e5c20(ptr(a),nullptr);}void file_construct(Word a,Word b,Word c,Word d)override{frame_LogFileCtor_005e68d0(ptr(a),nullptr,ptr(b),c,ptr(d));}Word file_open(Word a)override{return frame_LogFileOpened_005e6a10(ptr(a),nullptr);}void file_destroy(Word a)override{frame_LogFileDtor_005e69e0(ptr(a),nullptr);}};
Word __fastcall constant(void*,void*){return c2da_resource_constant_word_00406490();}Word __fastcall zero(void*,void*){return 0;}
const void*const table[]={reinterpret_cast<void*>(&gff_resource_deleting_destroy_00413010),reinterpret_cast<void*>(&constant),reinterpret_cast<void*>(&zero),reinterpret_cast<void*>(&gff_resource_freed_00410880),reinterpret_cast<void*>(&gff_resource_serviced_00410740)};
struct Constructor:ModuleGffPorts{void base_00406460(Word a)override{c2da_resource_base_ctor_00406460(ptr(a),nullptr);}Word vtable(Word)override{return word(table);}void write_byte(Word a,Word b)override{*static_cast<unsigned char*>(ptr(a))=static_cast<unsigned char>(b);}};
}
extern "C" void* __fastcall recovered_gff_construct_004105a0(void*a,void*){M m;Constructor p;return ptr(gff_default_004105a0(m,p,word(a)));}
extern "C" Word __fastcall recovered_gff_00411260(void*a,void*,void*b,void*c,void*d){M m;P p(m);Word scratch[16];return gff_initialize_00411260(m,p,word(a),word(b),word(c),word(d),word(scratch));}
extern "C" Word __fastcall recovered_gff_00412450(void*a,void*,void*b,void*c,const char*d){M m;P p(m);return gff_add_list_00412450(m,p,word(a),word(b),word(c),word(d));}
extern "C" Word __fastcall recovered_gff_004124e0(void*a,void*,void*b,void*c,Word d){M m;P p(m);Word scratch;return gff_add_list_struct_004124e0(m,p,word(a),word(b),word(c),d,word(&scratch));}
#define SCALAR(addr,type) extern "C" Word __fastcall recovered_gff_##addr(void*a,void*,void*b,Word c,const char*d){M m;P p(m);return gff_scalar_write(m,p,word(a),word(b),c,word(d),type);}
SCALAR(00412620,0) SCALAR(00412670,1) SCALAR(004126c0,2) SCALAR(00412710,3) SCALAR(00412760,4) SCALAR(004127b0,5)
#undef SCALAR
extern "C" Word __fastcall recovered_gff_004128c0(void*a,void*,void*b,void*c,const char*d){M m;P p(m);return gff_write_resref_004128c0(m,p,word(a),word(b),word(c),word(d));}
extern "C" Word __fastcall recovered_gff_00412970(void*a,void*,void*b,void*c,const char*d){M m;P p(m);return gff_write_string_00412970(m,p,word(a),word(b),word(c),word(d));}
extern "C" Word __fastcall recovered_gff_00412a10(void*a,void*,void*b,void*c,const char*d){M m;P p(m);Word scratch[16];return gff_write_localized_00412a10(m,p,word(a),word(b),word(c),word(d),word(scratch));}
extern "C" Word __fastcall recovered_gff_00413030(void*a,void*,void*b,Word c){M m;O p;Word scratch[16];return gff_write_file_00413030(m,p,word(a),word(b),c,word(scratch));}
