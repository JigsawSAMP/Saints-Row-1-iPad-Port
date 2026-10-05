// Compiled inside the GPU plugin, whose C++ implementation symbols are hidden.
#include <rex/graphics/graphics_system.h>
#include <rex/ui/vulkan/provider.h>
#include <rex/ui/vulkan/device.h>
extern "C" __attribute__((visibility("default"))) void SRNativeGpuSetActive(
    rex::system::IGraphicsSystem* system, int active) {
    auto* graphics = dynamic_cast<rex::graphics::GraphicsSystem*>(system);
    if (!graphics) return;
    if (active) {
        if (graphics->is_paused()) graphics->Resume();
    } else if (!graphics->is_paused()) {
        graphics->Pause();
        auto* provider = dynamic_cast<rex::ui::vulkan::VulkanProvider*>(graphics->provider());
        if (provider && provider->vulkan_device()) {
            auto* device = provider->vulkan_device();
            auto waitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
                device->vulkan_instance()->functions().vkGetDeviceProcAddr(device->device(), "vkDeviceWaitIdle"));
            if (waitIdle) waitIdle(device->device());
        }
    }
}
