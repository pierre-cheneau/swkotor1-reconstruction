extern "C" void __fastcall kotor_fastcall_0052bfc0(void*,void*,void*);  // CServerAIMaster::AppendToScriptsRun: matched (src/bridge)
struct NativeVmScriptTrace:recovered_vm_script_trace::Ports{
 NativeMemory&m;explicit NativeVmScriptTrace(NativeMemory&v):m(v){}
 Word read(Word a)override{return m.read(a);}Word ai(Word a)override{NativeModuleComplete p(m);return p.ai(a);}Word not_empty(Word a)override{return static_cast<Word>(client_movies::StringNotEqual_005e5390(static_cast<options_video::String*>(ptr(a)),nullptr,""));}
 void append_scripts_run_0052bfc0(Word a,Word s)override{kotor_fastcall_0052bfc0(ptr(a),nullptr,ptr(s));}
};
extern "C" void __fastcall recovered_vm_script_trace_0052c100(void*,void*,void*script){NativeMemory m;NativeVmScriptTrace p(m);recovered_vm_script_trace::command(p,word(script));}
