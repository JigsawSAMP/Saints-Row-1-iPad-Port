// Native frame timing and base-game services.
#include <rex/ppc/context.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>
#include <cstdint>
namespace { std::mutex activityMutex; std::condition_variable activityChanged; bool appActive = true; }
void SRSetFrameActivity(bool active) {
    { std::lock_guard lock(activityMutex); appActive = active; }
    activityChanged.notify_all();
}
namespace sr {
std::atomic<int> g_fps_cap{60};
std::atomic<uint64_t> g_game_frames{0};
void LimitFrameRate(double fps) {
    { std::unique_lock lock(activityMutex); activityChanged.wait(lock, [] { return appActive; }); }
    static thread_local auto last = std::chrono::steady_clock::now();
    if (fps > 0) {
        auto next = last + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(1.0/fps));
        std::this_thread::sleep_until(next);
    }
    last = std::chrono::steady_clock::now();
}
void PerfFrameBegin() {}
void PerfFrameEnd() {}
bool PerfWaitsOn() { return false; }
bool PerfTrackWaits() { return false; }
void PerfRecordWait(uint32_t, uint64_t) {}
void PerfRecordThreadWait(uint32_t, uint32_t, uint64_t) {}
enum PerfCounter { kPerfTilingExecutor, kPerfTilingReplay, kPerfCommandBuffers, kPerfCounterCount };
void PerfCount(PerfCounter) {}
namespace world_studio {
void OnPresent(::PPCContext&, uint8_t*) {}
void AfterFrameTimer(uint8_t*) {}
bool BeforeGameFrame(uint8_t*) { return true; }
}
}



// Custom SaintsReborn Gangs session is not initialized during base-game boot.
bool SrGangsActive() { return false; }

extern "C" uint64_t SRNativeFrameCount() { return sr::g_game_frames.load(std::memory_order_relaxed); }
