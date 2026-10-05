#pragma once
#include <stdint.h>
#include "MobileControls.h"
#include "NativeMenus.h"
#ifdef __cplusplus
extern "C" {
#endif
void SRNativeAttachView(void* view);
void SRNativePumpEvents(void);
void SRNativeSetActive(int active);
uint64_t SRNativeFrameCount(void);
void SRNativeSetPerformanceProfile(int profile);
void SRNativeStart(const char* game_directory);
const char* SRNativeStatus(void);
int SRNativeCoopState(void);
void SRNativeCoopHost(void);
void SRNativeCoopJoin(const char* ip);
void SRNativeCoopStop(void);
const char* SRNativeCoopInfo(void);
#ifdef __cplusplus
}
#endif
