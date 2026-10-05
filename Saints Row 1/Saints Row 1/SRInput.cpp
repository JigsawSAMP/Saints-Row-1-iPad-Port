#ifndef SR_INPUT_BRIDGE_EXTERNAL
#include "SRInput.h"
#include <mutex>
namespace {
std::mutex guard;
SRInputState touch{}, controller{}, merged{};
bool controllerActive = true;
void update() {
    SRInputState next{};
    next.buttons = touch.buttons | controller.buttons;
    next.leftTrigger = touch.leftTrigger > controller.leftTrigger ? touch.leftTrigger : controller.leftTrigger;
    next.rightTrigger = touch.rightTrigger > controller.rightTrigger ? touch.rightTrigger : controller.rightTrigger;
    // Keep each stick's two axes together, preserving its intended direction.
    auto magnitude = [](int16_t x, int16_t y) { return int64_t(x)*x + int64_t(y)*y; };
    bool leftTouch = magnitude(touch.leftX, touch.leftY) >= magnitude(controller.leftX, controller.leftY);
    bool rightTouch = magnitude(touch.rightX, touch.rightY) >= magnitude(controller.rightX, controller.rightY);
    next.leftX = leftTouch ? touch.leftX : controller.leftX;
    next.leftY = leftTouch ? touch.leftY : controller.leftY;
    next.rightX = rightTouch ? touch.rightX : controller.rightX;
    next.rightY = rightTouch ? touch.rightY : controller.rightY;
    bool changed = next.buttons != merged.buttons || next.leftTrigger != merged.leftTrigger ||
        next.rightTrigger != merged.rightTrigger || next.leftX != merged.leftX || next.leftY != merged.leftY ||
        next.rightX != merged.rightX || next.rightY != merged.rightY;
    next.packet = merged.packet + (changed ? 1u : 0u);
    merged = next;
}
}
extern "C" void SRInputSetTouch(SRInputState state) { std::lock_guard lock(guard); touch = state; update(); }
extern "C" void SRInputSetController(SRInputState state) { std::lock_guard lock(guard); controller = controllerActive ? state : SRInputState{}; update(); }
extern "C" void SRInputSetControllerActive(int active) { std::lock_guard lock(guard); controllerActive = active != 0; if (!controllerActive) { controller = {}; update(); } }
extern "C" SRInputState SRInputRead() { std::lock_guard lock(guard); return merged; }
extern "C" void SRInputReset() { std::lock_guard lock(guard); touch = {}; controller = {}; update(); }

#endif
