#pragma once
#include <cstdint>
namespace recovered_global_catalog {
struct String {std::uint32_t data,length;};
struct Ports {virtual ~Ports()=default;virtual int length(String*)=0;virtual signed char character(String*,int)=0;virtual int uppercase(int)=0;virtual const char* text(String*)=0;virtual int equal_nc(String*,const char*)=0;};
unsigned hash_00528e50(void*,String*,Ports&);
int insert_00528ec0(void*,String*,unsigned,unsigned,Ports&);
int register_category(void*,String*,unsigned,Ports&);
int find_00529030(void*,String*,Ports&);
}
namespace recovered_global_catalog {
struct LoadPorts:Ports {
 virtual void table_construct(void*,const void*,unsigned)=0;virtual int table_load(void*)=0;virtual void table_destroy(void*)=0;virtual int table_get(void*,int,String*,String*)=0;
 virtual void string_default(String*)=0;virtual void string_literal(String*,const char*)=0;virtual void string_destroy(String*)=0;
 virtual void diagnostic(String*,const char*,const char*,const char*,int)=0;
 // CSWGlobalVariableTable::Add{Boolean,Number,Location,String} 00528F70 + 0x30 * category (thiscall, RET 4)
 virtual int add_category(void*,String*,unsigned)=0;
};
void load_0052ae50(void*,const void*,LoadPorts&);
}
