// Native subset of SaintsReborn/project/src/render_fixes.cpp's single-pass
// rendering fix. All tiling scenarios must be changed, including shop and
// character menus; selecting only scenario zero does not cover those paths.
#include "saintsrow_config.h"
#include "saintsrow_init.h"
#include <rex/ppc/function.h>
#include <rex/logging.h>
#include <cstdint>
#include <atomic>
#include <bit>
namespace { std::atomic<int> profile{0}; }
extern "C" void SRNativeSetPerformanceProfile(int value) { profile.store(value>=0 && value<=2 ? value : 0); }
// Selected before the guest starts; restart is required to change frame buffers.
int SRNativeRenderWidth() { return profile.load()==1 ? 1280 : profile.load()==2 ? 960 : 640; }
int SRNativeRenderHeight() { return profile.load()==1 ? 720 : profile.load()==2 ? 540 : 360; }
extern "C" void __imp__sub_82183008(PPCContext&, uint8_t*);
PPC_FUNC(sub_82183008) {
    constexpr uint32_t configs = 0x827D6D38;
    const uint32_t width = SRNativeRenderWidth();
    const uint32_t height = SRNativeRenderHeight();
    for (uint32_t index = 0; index < 9; ++index) {
        const uint32_t entry = configs + index * 156;
        PPC_STORE_U32(entry, 0);       // 1x, no MSAA
        PPC_STORE_U32(entry + 12, 1);  // one full-screen tile
        PPC_STORE_U32(entry + 20, 0);
        PPC_STORE_U32(entry + 24, 0);
        PPC_STORE_U32(entry + 28, width);
        PPC_STORE_U32(entry + 32, height);
    }
    REXLOG_INFO("iPad single-pass render scenarios: {}x{}", width, height);
    __imp__sub_82183008(ctx, base);
}

extern "C" void __imp__sub_8263D8D8(PPCContext&, uint8_t*);
PPC_FUNC(sub_8263D8D8) {
    const auto setup=ctx.r3.u32;
    if(PPC_LOAD_U32(setup)==1280 && PPC_LOAD_U32(setup+4)==720) {
        PPC_STORE_U32(setup,SRNativeRenderWidth());
        PPC_STORE_U32(setup+4,SRNativeRenderHeight());
        PPC_STORE_U32(setup+16,std::bit_cast<uint32_t>(16.0f/9.0f));
        REXLOG_INFO("iPad frame buffers: {}x{}",SRNativeRenderWidth(),SRNativeRenderHeight());
    }
    __imp__sub_8263D8D8(ctx,base);
}
