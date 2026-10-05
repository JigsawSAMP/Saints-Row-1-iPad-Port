#pragma once
#include <rex/input/input_driver.h>

// Registered in the native iPad runtime InputSystem.
// One logical Xbox pad merges touchscreen and Apple's physical controllers.
class AppleInputDriver final : public rex::input::InputDriver {
public:
    AppleInputDriver() : InputDriver(nullptr, 0) {}
    rex::X_STATUS Setup() override;
    void EnumerateDevices(std::vector<rex::input::DeviceInfo>& out) override;
    rex::X_RESULT GetDeviceState(rex::input::DeviceId id, rex::input::X_INPUT_STATE* out) override;
    rex::X_RESULT GetDeviceCapabilities(rex::input::DeviceId id, uint32_t flags, rex::input::X_INPUT_CAPABILITIES* out) override;
    rex::X_RESULT SetDeviceVibration(rex::input::DeviceId id, rex::input::X_INPUT_VIBRATION* vibration) override;
    rex::X_RESULT GetDeviceKeystroke(rex::input::DeviceId id, uint32_t flags, rex::input::X_INPUT_KEYSTROKE* out) override;
};
