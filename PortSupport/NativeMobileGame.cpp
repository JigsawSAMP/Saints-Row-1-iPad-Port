#include "NativeMobileGame.h"
#include "MobileControls.h"
#include "SRInput.h"
#include "saintsrow_config.h"
#include "saintsrow_init.h"
#include <rex/ppc/function.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include <cmath>

namespace {
uint32_t Read(uint8_t* base,uint32_t a){uint32_t v;memcpy(&v,base+a,4);return __builtin_bswap32(v);}
float Float(uint8_t* base,uint32_t a){return std::bit_cast<float>(Read(base,a));}
void Store(uint8_t* base,uint32_t a,float f){auto v=__builtin_bswap32(std::bit_cast<uint32_t>(f));memcpy(base+a,&v,4);}
bool PlayerVehicle(uint8_t* base,uint32_t p){
 if(!p || Read(base,p+3456)!=0)return false;
 auto handle=Read(base,p+2496),state=Read(base,p+508);
 if(!handle || (base[p+2569]&0x10) || state==9||state==12)return false;
 auto index=handle&0xffff;if(index>=4096)return false;
 auto object=Read(base,0x830866C8+12+index*16);
 return object && Read(base,object+68)==handle && Read(base,object+72)==5;
}
}
void SRMobileGameFrame(uint8_t* base){
 auto player=Read(base,0x8309ABEC),screen=Read(base,0x839E0DF8);
 int mode=SR_CONTEXT_MENU;
 if(player && !screen && Read(base,0x827B04D4)!=1) mode=(base[0x8370D991]||base[0x8370D990])?SR_CONTEXT_CUTSCENE:PlayerVehicle(base,player)?SR_CONTEXT_VEHICLE:SR_CONTEXT_FOOT;
 SRMobileSetContext(mode);
}
extern "C" void __imp__sub_8210D518(PPCContext&,uint8_t*);
// Controller-only build: use the original game's right-stick camera path.
// No pointer deltas or temporary camera sensitivity overrides are applied.
PPC_FUNC(sub_8210D518){ __imp__sub_8210D518(ctx,base); }
