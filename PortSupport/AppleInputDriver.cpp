#include "AppleInputDriver.h"
#include "SRInput.h"
#include <cstring>
#include <cstdio>

using namespace rex;
using namespace rex::input;
namespace { constexpr DeviceId pad = static_cast<DeviceId>(0x4150504C450001); }
X_STATUS AppleInputDriver::Setup() { return X_STATUS_SUCCESS; }
void AppleInputDriver::EnumerateDevices(std::vector<DeviceInfo>& out) {
    DeviceInfo device;
    device.id = pad;
    device.name = "iPad touch / Game Controller";
    device.guid = "saintsrow.apple.merged-pad";
    device.synthetic = true;
    out.push_back(device);
}
X_RESULT AppleInputDriver::GetDeviceState(DeviceId id, X_INPUT_STATE* out) {
    if (id != pad) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (out) {
        auto state = is_active() ? SRInputRead() : SRInputState{};
        static thread_local uint16_t last_buttons = 0;
        if (state.buttons != last_buttons) {
            fprintf(stderr, "[SR input] Guest buttons %04X packet %u\n", state.buttons, state.packet);
            last_buttons = state.buttons;
        }
        out->packet_number = state.packet;
        out->gamepad.buttons = state.buttons;
        out->gamepad.left_trigger = state.leftTrigger;
        out->gamepad.right_trigger = state.rightTrigger;
        out->gamepad.thumb_lx = state.leftX;
        out->gamepad.thumb_ly = state.leftY;
        out->gamepad.thumb_rx = state.rightX;
        out->gamepad.thumb_ry = state.rightY;
    }
    return X_ERROR_SUCCESS;
}
X_RESULT AppleInputDriver::GetDeviceCapabilities(DeviceId id, uint32_t, X_INPUT_CAPABILITIES* out) {
    if (id != pad) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (out) {
        std::memset(out, 0, sizeof(*out));
        out->type = 1; out->sub_type = 1;
        out->gamepad.buttons = 0xF3FF;
        out->gamepad.left_trigger = out->gamepad.right_trigger = 255;
        out->gamepad.thumb_lx = out->gamepad.thumb_ly = out->gamepad.thumb_rx = out->gamepad.thumb_ry = 32767;
    }
    return X_ERROR_SUCCESS;
}
X_RESULT AppleInputDriver::SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION*) {
    // No force feedback advertised until a physical-controller haptics bridge exists.
    return id == pad ? X_ERROR_SUCCESS : X_ERROR_DEVICE_NOT_CONNECTED;
}
X_RESULT AppleInputDriver::GetDeviceKeystroke(DeviceId id, uint32_t, X_INPUT_KEYSTROKE*) {
    return id == pad ? X_ERROR_EMPTY : X_ERROR_DEVICE_NOT_CONNECTED;
}
