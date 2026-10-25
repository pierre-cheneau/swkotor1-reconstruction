#include "0f2c9decb2-string_priority.h"
#include "1095ffa0e5-concat_copy_lifetime.h"
#include "ce00e26aa5-native_strings.h" // Borrow task20's genuine copy-constructor provider.
#include "8997362651-allocator_live.h" // Borrow the one task23 live heap contract.
#include <new>
using kotor_reconstruction::StartupString;
using Word=std::uint32_t;
static Word length(const char* input) {
    const volatile unsigned char* first=reinterpret_cast<const volatile unsigned char*>(input);
    const volatile unsigned char* p=first;while(*p)++p;
    return static_cast<Word>(p-first);
}
static void copy_zero(char* output,const char* input) {
    volatile unsigned char* d=reinterpret_cast<volatile unsigned char*>(output);
    const volatile unsigned char* s=reinterpret_cast<const volatile unsigned char*>(input);
    unsigned char b;do{b=*s++;*d++=b;}while(b);
}
extern "C" StartupString* __fastcall GuiAssign005e5140(StartupString* self,void*,const char* input) {
    char* previous=self->data;
    if(previous && (!input || length(input)+1u>self->capacity)) {
        self->capacity=0;live_free(previous);self->data=nullptr;
    }
    if(input && static_cast<std::int32_t>(length(input))>0) {
        if(!self->data) {
            Word count=length(input)+1u;



            self->capacity=count;

            self->data=static_cast<char*>(live_operator_new(count));
        }
        copy_zero(self->data,input);
    } else {
        char* current=self->data;
        if(current) {
            live_free(current);self->data=nullptr;self->capacity=0;
        }



    }
    return self;
}
StartupString* __fastcall GuiConcat005e5d10(StartupString* self,void*,StartupString* output,const StartupString* right) {
    // Original temporary fields are zeroed inline. Borrowing the accepted
    // default constructor has the same normal stores, without another owner.
    alignas(StartupString) unsigned char storage[sizeof(StartupString)];
    StartupString* temporary=::new(storage) StartupString;
    // Fixed /EHsc treats direct extern-C calls as nonthrowing. Volatile typed
    // function-pointer carriers preserve allocator/free bindings and the
    // explicit owned-copy lifetime join; all keep the genuine machine ABI and
    // require normal C++ exception state around the three opaque calls.
    void* (__cdecl * volatile allocate)(Word)=&live_operator_new;
    void (__cdecl * volatile release)(void*)=&live_free;
    StartupString* (__fastcall * volatile copy)(StartupString*,void*,const StartupString* volatile)=&base_ini_string_joins::ConstructFreshConcatOutput;
    bool copy_complete=false;char* allocation=nullptr;
    try {
    Word r=right->data?length(right->data):0u;
    Word l=self->data?length(self->data):0u;
    if(l || r) {
        Word count=l+r+1u;temporary->capacity=count;
        allocation=static_cast<char*>(allocate(count));temporary->data=allocation;
        if(!r)copy_zero(allocation,self->data);
        else if(!l)copy_zero(allocation,right->data);
        else {
            copy_zero(allocation,self->data);
            const char* source=right->data;
            Word count_right=length(source)+1u;
            volatile unsigned char* target=reinterpret_cast<volatile unsigned char*>(allocation);
            while(*target)++target;
            const volatile unsigned char* bytes=reinterpret_cast<const volatile unsigned char*>(source);
            for(Word i=0;i<count_right;++i)target[i]=bytes[i];
        }
    }
    (void)copy(output,nullptr,temporary);
    copy_complete=true;
    // Original frees cached ESI/EBP, not a potentially modified temporary field.



    if(allocation)release(allocation);

    } catch(...) {










        // Original unwind map77509C: state1 destroys current temporary;
        // after copy returns, state0 destroys constructed output only.
        if(copy_complete)output->~StartupString();
        else temporary->~StartupString();

        throw;
    }



    return output;

}
// CExoString::operator+ for bridged code (src/bridge): the result is built at the caller's hidden pointer.
extern "C" void* __fastcall kotor_fastcall_005e5d10(void* self,void*,void* output,const void* right) {
    return GuiConcat005e5d10(static_cast<StartupString*>(self),nullptr,static_cast<StartupString*>(output),
                             static_cast<const StartupString*>(right));
}
