#include "AppleInputDriver.h"
#include "SRInput.h"
#include <cassert>
int main() {
    using namespace rex;
    using namespace rex::input;
    AppleInputDriver driver;
    assert(driver.Setup() == X_STATUS_SUCCESS);
    std::vector<DeviceInfo> devices;
    driver.EnumerateDevices(devices);
    assert(devices.size() == 1);
    auto id = devices[0].id;
    SRInputReset();
    SRInputState touch{}; touch.buttons=0x1000; touch.leftX=-32768; touch.leftY=32767; touch.rightTrigger=255;
    SRInputSetTouch(touch);
    X_INPUT_STATE state{};
    assert(driver.GetDeviceState(id, &state)==X_ERROR_SUCCESS);
    assert(uint16_t(state.gamepad.buttons)==0x1000 && int16_t(state.gamepad.thumb_lx)==-32768);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&state.gamepad);
    assert(bytes[0]==0x10 && bytes[1]==0 && bytes[3]==255 && bytes[4]==0x80 && bytes[5]==0);
    driver.set_is_active_callback([] { return false; });
    assert(driver.GetDeviceState(id, &state)==X_ERROR_SUCCESS && uint16_t(state.gamepad.buttons)==0);
    assert(driver.GetDeviceState(DeviceId::kInvalid, &state)==X_ERROR_DEVICE_NOT_CONNECTED);
    X_INPUT_CAPABILITIES caps{};
    assert(driver.GetDeviceCapabilities(id, 0, &caps)==X_ERROR_SUCCESS && uint16_t(caps.flags)==0);
    assert(driver.GetDeviceKeystroke(id, 0, nullptr)==X_ERROR_EMPTY);
}
