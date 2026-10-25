#pragma once
#include <cstdint>
namespace recovered_ai_construct {
struct Ports {virtual ~Ports()=default;virtual void* allocate(unsigned)=0;virtual void string_default(void*)=0;virtual const void* table1()=0;virtual const void* table2()=0;virtual void initialize_table1(void*)=0;virtual void initialize_table2(void*)=0;
 // CExoLinkedList<CServerAIEventNode>::CExoLinkedList 004AF260 (thiscall on the embedded list, RET 0)
 virtual void event_queue(void*)=0;};
void* construct_004b0780(void*,Ports&);
}
