#pragma once
#include <cstdint>
namespace recovered_combat_round {using U=std::uint32_t;struct Ports {virtual ~Ports()=default;virtual U read(U)=0;virtual void write(U,U)=0;virtual void byte(U,U)=0;virtual void half(U,U)=0;virtual void string(U)=0;virtual void empty(U)=0;virtual U allocate(U)=0;virtual void scheduled_list_004d2f30(U)=0;virtual void effect_destroy(U)=0;virtual void release(U)=0;};U attack(U,Ports&);void reset(U,Ports&);U construct(U,U,Ports&);}
