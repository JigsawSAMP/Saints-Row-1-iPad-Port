#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum SRMobileSetting { SR_TOUCH_ENABLED, SR_OPACITY, SR_BUTTON_SIZE, SR_TOUCH_SENSITIVITY, SR_CAMERA_SENSITIVITY, SR_AIM_SENSITIVITY, SR_STICK_SIZE, SR_STICK_DEADZONE, SR_LEFT_HANDED, SR_RENDER_PROFILE, SR_FIXED_STICK, SR_SHOW_FPS, SR_SETTING_COUNT };
enum SRMobileControl { SR_PAUSE, SR_MOVE, SR_FIRE, SR_AIM, SR_RELOAD, SR_SPRINT, SR_JUMP, SR_CROUCH, SR_ENTER, SR_WEAPON, SR_HANDBRAKE, SR_ACCELERATE, SR_BRAKE, SR_NITRO, SR_HORN, SR_UP, SR_DOWN, SR_LEFT, SR_RIGHT, SR_ACCEPT, SR_CANCEL, SR_MENU_X, SR_MENU_Y, SR_LB, SR_RB, SR_LT, SR_RT, SR_BACK, SR_CONTROL_COUNT };
enum SRMobileContext { SR_CONTEXT_MENU, SR_CONTEXT_FOOT, SR_CONTEXT_VEHICLE, SR_CONTEXT_CUTSCENE };
typedef struct { float x,y; int hidden; } SRMobilePlacement;
void SRMobileLoad(const char* directory);
uint64_t SRMobileRevision(void);
int SRMobileGetSetting(int key);
void SRMobileSetSetting(int key,int choice);
float SRMobileSettingValue(int key);
const char* SRMobileSettingName(int key);
int SRMobileSettingChoiceCount(int key);
const char* SRMobileSettingChoice(int key,int choice);
const char* SRMobileControlName(int control);
SRMobilePlacement SRMobileGetPlacement(int control);
void SRMobileSetPlacement(int control,SRMobilePlacement placement);
void SRMobileReset(void);
void SRMobileSetContext(int context);
int SRMobileGetContext(void);
void SRMobileAddLook(float dx,float dy);
void SRMobileTakeLook(float* dx,float* dy);
void SRMobileClearLook(void);
void SRMobileBeginEdit(void);
int SRMobileTakeEditRequest(void);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {float x,y;} SRMobileVector;
SRMobileVector SRMobileStick(float x,float y,float radius,float sensitivity,float deadzone);
void SRMobileSetEditing(int editing);
int SRMobileIsEditing(void);
#ifdef __cplusplus
}
#endif
