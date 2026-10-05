#include "MobileControls.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <thread>
#include <limits>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);auto folder=std::filesystem::path(argv[1]);std::filesystem::create_directories(folder);
 SRMobileLoad(folder.c_str());SRMobileReset();
 auto v=SRMobileStick(0,0,50,1,.1);assert(v.x==0&&v.y==0);
 v=SRMobileStick(4,0,50,1,.1);assert(v.x==0&&v.y==0);
 v=SRMobileStick(27.5,0,50,1,.1);assert(std::abs(v.x-.5)<1e-5&&v.y==0);
 v=SRMobileStick(100,100,50,1,.1);assert(std::abs(std::hypot(v.x,v.y)-1)<1e-5&&std::abs(v.x-v.y)<1e-5);
 v=SRMobileStick(2,2,0,1,.1);assert(v.x==0&&v.y==0);
 v=SRMobileStick(std::numeric_limits<float>::quiet_NaN(),2,50,1,.1);assert(v.x==0&&v.y==0);
 // A swipe totals the same displacement regardless of touch sampling/game cadence.
 float x=0,y=0;SRMobileClearLook();for(int i=0;i<60;++i)SRMobileAddLook(2,-1);SRMobileTakeLook(&x,&y);assert(x==120&&y==-60);
 SRMobileAddLook(120,-60);SRMobileTakeLook(&x,&y);assert(x==120&&y==-60);SRMobileTakeLook(&x,&y);assert(x==0&&y==0);
 SRMobileAddLook(9,8);SRMobileSetContext(SR_CONTEXT_VEHICLE);SRMobileTakeLook(&x,&y);assert(x==0&&y==0);
 SRMobileSetSetting(SR_OPACITY,5);SRMobileSetSetting(SR_CAMERA_SENSITIVITY,4);SRMobileSetPlacement(SR_FIRE,{.7f,.3f,1});
 SRMobileLoad(folder.c_str());assert(SRMobileGetSetting(SR_OPACITY)==5&&SRMobileGetSetting(SR_CAMERA_SENSITIVITY)==4);auto p=SRMobileGetPlacement(SR_FIRE);assert(p.hidden&&std::abs(p.x-.7f)<1e-5);
 SRMobileSetPlacement(SR_MOVE,{-1,2,0});p=SRMobileGetPlacement(SR_MOVE);assert(p.x==0&&p.y==1);
 SRMobileSetSetting(SR_OPACITY,999);assert(SRMobileGetSetting(SR_OPACITY)==SRMobileSettingChoiceCount(SR_OPACITY)-1);
 SRMobileBeginEdit();assert(SRMobileTakeEditRequest()==1&&SRMobileTakeEditRequest()==0);SRMobileSetEditing(1);assert(SRMobileIsEditing());SRMobileSetEditing(0);
 std::thread writer([]{for(int i=0;i<10000;++i)SRMobileAddLook(.01f,.01f);});
 for(int i=0;i<1000;++i){SRMobileTakeLook(&x,&y);assert(std::isfinite(x)&&std::isfinite(y));}writer.join();
 SRMobileReset();assert(SRMobileGetPlacement(SR_FIRE).hidden==0&&SRMobileGetSetting(SR_OPACITY)==2);
 puts("Mobile radial deadzone, displacement sampling, persistence, validation, reset and concurrent access passed.");
}
