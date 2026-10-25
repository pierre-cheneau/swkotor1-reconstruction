#include "8c5a390f6f3039f4_vm_script_trace.hpp"
namespace recovered_vm_script_trace {
void command(Ports&p,Word script){const Word app=p.read(0x7a39fc);const Word server=p.read(app+8);const Word master=p.ai(server);if(p.not_empty(script))p.append_scripts_run_0052bfc0(master,script);}
}
