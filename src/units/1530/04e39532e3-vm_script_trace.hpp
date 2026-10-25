#pragma once
#include <cstdint>
namespace recovered_vm_script_trace {
using Word=std::uint32_t;
struct Ports {virtual ~Ports()=default;virtual Word read(Word)=0;virtual Word ai(Word)=0;virtual Word not_empty(Word)=0;
 // CServerAIMaster::AppendToScriptsRun 0052BFC0 (thiscall, the CExoString by reference, RET 4)
 virtual void append_scripts_run_0052bfc0(Word ai,Word script)=0;};
void command(Ports&,Word script);
}
