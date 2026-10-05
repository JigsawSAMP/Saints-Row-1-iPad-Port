#include "NativeGame.h"
#include "NativeCoop.h"
#include "NativePresentation.h"
#include <rex/audio/sdl/sdl_audio_system.h>
#include <rex/audio/audio_system.h>
void SRSetFrameActivity(bool active);
extern "C" void SRNativeGpuSetActive(rex::system::IGraphicsSystem*, int);
#include <rex/system/xthread.h>
#include "AppleInputDriver.h"
#include "saintsrow_init.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/input/input_system.h>
#include <rex/input/device_assignment.h>
#include <rex/logging.h>
#include <rex/cvar.h>
#include <sys/mman.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>
#include <memory>
#include <cstring>
#include <cerrno>

namespace {
std::mutex status_mutex;
std::string status = "Waiting for game assets";
std::atomic<bool> started{false};
std::atomic<bool> ready{false};
std::unique_ptr<rex::Runtime> game;
std::string nativeLogPath;
void publish(std::string text) {
    std::lock_guard lock(status_mutex);
    status = std::move(text);
    fprintf(stderr, "[SR native] %s\n", status.c_str());
    fflush(stderr);
}
void boot(const std::filesystem::path& directory) {
    try {
        if (!std::filesystem::exists(directory / "default.xex")) {
            publish("Game files have not finished transferring. Relaunch after transfer.");
            return;
        }
        publish("Checking guest address space");
        // Check before SDK initialization, which otherwise asserts on failure.
        constexpr size_t span = 0x120000000ull;
        void* reservation = mmap(nullptr, span, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (reservation == MAP_FAILED) {
            publish(std::string("Guest address space unavailable: ") + strerror(errno));
            return;
        }
        munmap(reservation, span);
        auto writable = directory.parent_path();
        std::filesystem::current_path(writable);
        SRMobileLoad(writable.c_str());
        SRNativeSetPerformanceProfile(SRMobileGetSetting(SR_RENDER_PROFILE));
        // Keep diagnostics small on the device, including leftovers from
        // earlier builds with a larger rotation limit.
        for (const auto& entry : std::filesystem::directory_iterator(writable)) {
            auto name = entry.path().filename().string();
            if (name.starts_with("saintsrow-native.") && name.ends_with(".log") &&
                name != "saintsrow-native.log" && name != "saintsrow-native.1.log" && name != "saintsrow-native.2.log") {
                std::error_code ignored;
                std::filesystem::remove(entry.path(), ignored);
            }
        }
        rex::cvar::SetFlagByName("log_max_file_size_mb", "1");
        rex::cvar::SetFlagByName("log_max_files", "2");
        nativeLogPath = (writable / "saintsrow-native.log").string();
        rex::LogConfig logging;
        logging.log_file = nativeLogPath.c_str();
        logging.log_to_console = true;
        logging.category_levels["gpu"] = spdlog::level::err;
        rex::InitLogging(logging);
        publish("Initializing Xbox runtime memory and kernel");
        game = std::make_unique<rex::Runtime>(directory);
        rex::RuntimeConfig config;
        setenv("MVK_CONFIG_LOG_LEVEL", "1", 1);
        rex::cvar::SetFlagByName("vulkan_log_debug_messages", "false");
        // Match SaintsReborn's bounded, snapshotted command queue. Previously
        // this host skipped main.cpp and waited after every small GPU stream.
        rex::cvar::SetFlagByName("gpu_async_depth", "4");
        rex::cvar::SetFlagByName("gpu_async_max_lag_us", "4000");
        rex::cvar::SetFlagByName("present_letterbox", "false");
        rex::cvar::SetFlagByName("present_safe_area_x", "100");
        rex::cvar::SetFlagByName("present_safe_area_y", "100");
        publish("Initializing Metal graphics");
        config.graphics = SRPrepareGraphics();
        if (!config.graphics) { publish("Graphics initialization failed; inspect saintsrow-native.log"); return; }
        config.audio_factory = rex::audio::sdl::SDLAudioSystem::Create;
        config.kernel_init = rex::kernel::InitializeKernel;
        config.input_factory = [](bool) -> std::unique_ptr<rex::system::IInputSystem> {
            auto input = std::make_unique<rex::input::InputSystem>(nullptr);
            auto driver = std::make_unique<AppleInputDriver>();
            driver->Setup();
            input->AddDriver(std::move(driver));
            input->SetDeviceAssignment(std::make_unique<rex::input::SlotAssignment>());
            return input;
        };
        auto result = game->Setup(PPCImageConfig, std::move(config));
        if (XFAILED(result)) {
            publish("Xbox runtime initialization failed; inspect saintsrow-native.log");
            return;
        }
        publish("Loading Saints Row executable");
        result = game->LoadXexImage("game:\\default.xex");
        if (XFAILED(result)) {
            publish("Game executable loading failed; inspect saintsrow-native.log");
            return;
        }
        // Match SaintsReborn's Windows InitNullZeroRegion/InitNullObjectPage:
        // this title reads near-null objects during character/story startup.
        auto* base = game->memory()->virtual_membase();
        if (mprotect(base, 0x01000000, PROT_READ | PROT_WRITE) != 0 ||
            mprotect(base + 0x0F000000, 0x10000, PROT_READ | PROT_WRITE) != 0) {
            publish(std::string("Game compatibility memory initialization failed: ") + strerror(errno));
            return;
        }
        memset(base + 0x0F000000, 0, 0x10000);
        // Disable shadow passes in the initial iPad performance preset.
        const uint32_t shadows = __builtin_bswap32(0u);
        memcpy(base + 0x827D6C90u, &shadows, sizeof(shadows));
        SRConnectPresenter(game.get());
        *game->memory()->TranslateVirtual<uint32_t*>(0x827D7834) = 0;
        SRInitializeCoop(game.get(), writable);
        publish("Starting Saints Row");
        auto thread = game->PrepareModuleLaunch();
        if (!thread) { publish("Game launch preparation failed"); return; }
        thread->Resume();
        ready.store(true, std::memory_order_release);
        publish("Saints Row guest thread running");
    } catch (const std::exception& error) {
        publish(std::string("Native startup error: ") + error.what());
    }
}
}
extern "C" void SRNativeStart(const char* path) {
    if (!path || started.exchange(true)) return;
    std::filesystem::path directory(path);
    std::thread([directory] { boot(directory); }).detach();
}
extern "C" const char* SRNativeStatus() {
    thread_local std::string copy;
    std::lock_guard lock(status_mutex);
    copy = status;
    return copy.c_str();
}

extern "C" void SRNativeSetActive(int active) {
    SRSetFrameActivity(active != 0);
    if (!ready.load(std::memory_order_acquire)) return;
    SRNativeGpuSetActive(game->graphics_system(), active);
    auto* audio = dynamic_cast<rex::audio::AudioSystem*>(game->audio_system());
    if (audio) {
        if (active) audio->Resume(); else audio->Pause();
    }
}
