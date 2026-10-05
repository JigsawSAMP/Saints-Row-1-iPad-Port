#ifndef SR_INPUT_H
#define SR_INPUT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Host-order Xbox controller state. The ReXGlue adapter converts to guest order.
typedef struct {
    uint32_t packet;
    uint16_t buttons;
    uint8_t leftTrigger, rightTrigger;
    int16_t leftX, leftY, rightX, rightY;
} SRInputState;
void SRInputSetTouch(SRInputState state);
void SRInputSetController(SRInputState state);
void SRInputSetControllerActive(int active);
SRInputState SRInputRead(void);
void SRInputReset(void);
#ifdef __cplusplus
}
#endif
#endif
