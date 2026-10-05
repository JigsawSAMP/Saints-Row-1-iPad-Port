// Signed AOT hooks: no runtime code patching or JIT is required.
#include "NativeCoop.h"
#include "NativeGame.h"
#include "NativeMobileGame.h"
#include "wml.h"
#include "saintsrow_config.h"
#include "saintsrow_init.h"
#include <rex/ppc/function.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/logging.h>
#include <array>
#include <bit>
#include <cstring>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
extern "C" int wml_mod_init(const WmlApi*, const WmlMod*);
extern "C" int WhompaysCoopState();
extern "C" const char* SRAppleCoopStatus();
extern "C" void WhompaysCoopHost();
extern "C" void WhompaysCoopJoin(const char*);
extern "C" void WhompaysCoopStop();
extern "C" int WhompaysCoopInfo(char*, int);
namespace {
rex::Runtime* runtime=nullptr;
uint8_t* memory=nullptr;
std::atomic<bool> initialized{false};
std::string folder;
WmlMod mod{};
std::mutex infoMutex;
std::string info="Co-op ready · direct IP / UDP 27015";
using Callback=std::pair<WmlFrameCallback,void*>;
std::vector<Callback> frameCallbacks,gameCallbacks;
#define SR_COOP_HOOK(A) extern "C" void __imp__sub_##A(PPCContext&,uint8_t*); WmlGuestFunction hook_##A=nullptr;
#include "CoopHooks.inc"
#undef SR_COOP_HOOK
int InstallHook(uint32_t address,WmlGuestFunction hook,WmlGuestFunction* original) {
    if(!hook || !original) return -1;
    switch(address) {
#define SR_COOP_HOOK(A) case 0x##A: *original=hook_##A ? hook_##A : reinterpret_cast<WmlGuestFunction>(__imp__sub_##A); hook_##A=hook; return 0;
#include "CoopHooks.inc"
#undef SR_COOP_HOOK
    default: REXLOG_ERROR("Unsupported iPad co-op hook {:08X}",address); return -1;
    }
}
template<class T> T Read(uint32_t a) { T v; memcpy(&v,memory+a,sizeof(v)); if constexpr(sizeof(T)>1) v=std::byteswap(v); return v; }
template<class T> void Write(uint32_t a,T v) { if constexpr(sizeof(T)>1) v=std::byteswap(v); memcpy(memory+a,&v,sizeof(v)); }
constexpr std::array<PPCRegister PPCContext::*,32> integerRegisters={
&PPCContext::r0,&PPCContext::r1,&PPCContext::r2,&PPCContext::r3,&PPCContext::r4,&PPCContext::r5,&PPCContext::r6,&PPCContext::r7,&PPCContext::r8,&PPCContext::r9,&PPCContext::r10,&PPCContext::r11,&PPCContext::r12,&PPCContext::r13,&PPCContext::r14,&PPCContext::r15,&PPCContext::r16,&PPCContext::r17,&PPCContext::r18,&PPCContext::r19,&PPCContext::r20,&PPCContext::r21,&PPCContext::r22,&PPCContext::r23,&PPCContext::r24,&PPCContext::r25,&PPCContext::r26,&PPCContext::r27,&PPCContext::r28,&PPCContext::r29,&PPCContext::r30,&PPCContext::r31};
constexpr std::array<PPCRegister PPCContext::*,32> floatRegisters={&PPCContext::f0,&PPCContext::f1,&PPCContext::f2,&PPCContext::f3,&PPCContext::f4,&PPCContext::f5,&PPCContext::f6,&PPCContext::f7,&PPCContext::f8,&PPCContext::f9,&PPCContext::f10,&PPCContext::f11,&PPCContext::f12,&PPCContext::f13,&PPCContext::f14,&PPCContext::f15,&PPCContext::f16,&PPCContext::f17,&PPCContext::f18,&PPCContext::f19,&PPCContext::f20,&PPCContext::f21,&PPCContext::f22,&PPCContext::f23,&PPCContext::f24,&PPCContext::f25,&PPCContext::f26,&PPCContext::f27,&PPCContext::f28,&PPCContext::f29,&PPCContext::f30,&PPCContext::f31};
const WmlApi api={
    WML_API_VERSION,sizeof(WmlApi),
    [](const WmlMod*,const char* text){ REXLOG_INFO("Co-op: {}",text?text:""); },
    Read<uint8_t>,Read<uint16_t>,Read<uint32_t>,
    [](uint32_t a){ return std::bit_cast<float>(Read<uint32_t>(a)); },
    Write<uint8_t>,Write<uint16_t>,Write<uint32_t>,
    [](uint32_t a,float v){ Write(a,std::bit_cast<uint32_t>(v)); },
    [](uint32_t a)->void* { return memory+a; },InstallHook,
    [](WmlContext* c,uint32_t a) { auto* fn=runtime->function_dispatcher()->GetFunction(a); if(!c || !fn) return -1; fn(*reinterpret_cast<PPCContext*>(c),memory); return 0; },
    [](WmlContext* c,int i)->uint64_t { return i>=0&&i<32 ? (reinterpret_cast<PPCContext*>(c)->*integerRegisters[i]).u64 : 0; },
    [](WmlContext* c,int i,uint64_t v){ if(i>=0&&i<32) (reinterpret_cast<PPCContext*>(c)->*integerRegisters[i]).u64=v; },
    [](WmlContext* c,int i)->double { return i>=0&&i<32 ? (reinterpret_cast<PPCContext*>(c)->*floatRegisters[i]).f64 : 0; },
    [](WmlContext* c,int i,double v){ if(i>=0&&i<32) (reinterpret_cast<PPCContext*>(c)->*floatRegisters[i]).f64=v; },
    [](WmlContext* c)->uint32_t { return reinterpret_cast<PPCContext*>(c)->lr; },
    [](WmlFrameCallback c,void* u){ if(c) frameCallbacks.emplace_back(c,u); },
    [](int){ return 0; },[](int){ return 0; },[](const char*){},
    [](WmlFrameCallback c,void* u){ if(c) gameCallbacks.emplace_back(c,u); },
    [](const float*,int){},[](const float*,int){}
};
}
#define SR_COOP_HOOK(A) PPC_FUNC(sub_##A) { if(hook_##A) hook_##A(reinterpret_cast<WmlContext*>(&ctx),base); else __imp__sub_##A(ctx,base); }
#include "CoopHooks.inc"
#undef SR_COOP_HOOK
void SRInitializeCoop(rex::Runtime* r,const std::filesystem::path& documents) {
    runtime=r; memory=r->memory()->virtual_membase();
    auto path=documents/"mods"/"WhompaysCoop";
    std::filesystem::create_directories(path); folder=path.string();
    // Each boot replaces diagnostics from prior launches.
    for(auto& e:std::filesystem::directory_iterator(path)) if(e.path().filename().string().starts_with("coop-")&&e.path().extension()==".log") { std::error_code error; std::filesystem::remove(e.path(),error); }
    mod={"Whompays Co-op",folder.c_str()};
    auto result=wml_mod_init(&api,&mod);
    initialized.store(result==0,std::memory_order_release);
    REXLOG_INFO("iPad co-op initialization: {}",result);
}
namespace wml {
void OnFrame() {
    if(!initialized.load(std::memory_order_acquire)) return;
    for(auto [callback,user]:frameCallbacks) callback(user);
    static auto last=std::chrono::steady_clock::now();
    auto now=std::chrono::steady_clock::now();
    if(now-last>=std::chrono::seconds(1)) {
        char text[1024]{}; WhompaysCoopInfo(text,sizeof(text));
        std::lock_guard lock(infoMutex); info=std::string(text)+" "+SRAppleCoopStatus(); last=now;
    }
}
void OnGameFrame() { SRMobileGameFrame(memory); SRNativeMenuPoll(memory); if(initialized.load(std::memory_order_acquire)) for(auto [callback,user]:gameCallbacks) callback(user); }
}
extern "C" int SRNativeCoopState() { return initialized.load() ? WhompaysCoopState() : -1; }
extern "C" void SRNativeCoopHost() { if(initialized.load()) WhompaysCoopHost(); }
extern "C" void SRNativeCoopJoin(const char* ip) { if(initialized.load()&&ip) WhompaysCoopJoin(ip); }
extern "C" void SRNativeCoopStop() { if(initialized.load()) WhompaysCoopStop(); }
extern "C" const char* SRNativeCoopInfo() { thread_local std::string copy; std::lock_guard lock(infoMutex); copy=initialized.load()?info:"Co-op initializing"; return copy.c_str(); }
