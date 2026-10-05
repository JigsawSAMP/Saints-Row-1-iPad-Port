#include "MobileControls.h"
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <sstream>

#include <vector>
namespace {
struct Setting { const char* name; std::vector<const char*> labels; std::vector<float> values; int initial; };
const std::array<Setting,SR_SETTING_COUNT> specs={{
 {"Touch Controls",{"Off","On"},{0,1},1},
 {"Control Opacity",{"25%","40%","55%","70%","85%","100%"},{.25f,.4f,.55f,.7f,.85f,1},2},
 {"Button Size",{"Small","Normal","Large","Extra Large"},{.8f,1,1.2f,1.4f},1},
 {"Touch Sensitivity",{"50%","75%","100%","125%","150%","200%"},{.5f,.75f,1,1.25f,1.5f,2},2},
 {"Camera Sensitivity",{"50%","75%","100%","125%","150%","200%"},{.5f,.75f,1,1.25f,1.5f,2},2},
 {"Aim Sensitivity",{"25%","50%","75%","100%","125%","150%"},{.25f,.5f,.75f,1,1.25f,1.5f},1},
 {"Virtual Stick Size",{"Small","Normal","Large","Extra Large"},{.8f,1,1.2f,1.4f},1},
 {"Stick Deadzone",{"0%","5%","10%","15%","20%","25%"},{0,.05f,.1f,.15f,.2f,.25f},2},
 {"Handed Layout",{"Right Handed","Left Handed"},{0,1},0},
 {"Rendering (restart)",{"Performance 640x360","Quality 1280x720","Balanced 960x540"},{0,1,2},0},
 {"Movement Stick",{"Floating","Fixed"},{0,1},0},
 {"FPS Display",{"Hidden","Shown"},{0,1},0}
}};
const std::array<const char*,SR_CONTROL_COUNT> names={"Pause","Movement / Steering","Fire / Attack","Secondary Attack","Reload / Pickup","Sprint","Jump","Crouch","Enter / Exit","Weapons","Handbrake","Accelerate","Brake / Reverse","Nitrous","Horn","Menu Up","Menu Down","Menu Left","Menu Right","Select (A)","Back (B)","Menu X","Menu Y","Left Shoulder","Right Shoulder","Left Trigger","Right Trigger","Map / Back"};
const std::array<SRMobilePlacement,SR_CONTROL_COUNT> defaults={{{.06f,.06f,0},{.16f,.77f,0},{.89f,.65f,0},{.76f,.65f,0},{.90f,.46f,0},{.78f,.88f,0},{.90f,.88f,0},{.64f,.88f,0},{.79f,.46f,0},{.66f,.46f,0},{.65f,.88f,0},{.89f,.76f,0},{.77f,.76f,0},{.89f,.50f,0},{.66f,.50f,0},{.16f,.62f,0},{.16f,.86f,0},{.10f,.74f,0},{.22f,.74f,0},{.88f,.77f,0},{.76f,.77f,0},{.82f,.64f,0},{.82f,.90f,0},{.12f,.09f,0},{.81f,.09f,0},{.23f,.09f,0},{.93f,.09f,0},{.48f,.06f,0}}};
std::mutex lock;
std::array<int,SR_SETTING_COUNT> choices;
std::array<SRMobilePlacement,SR_CONTROL_COUNT> placements=defaults;
std::string folder;
std::atomic<uint64_t> revision{1};
std::atomic<int> context{SR_CONTEXT_MENU};
std::atomic<bool> edit{false};
float lookX=0,lookY=0;
struct DefaultsInit { DefaultsInit(){ for(int i=0;i<SR_SETTING_COUNT;++i) choices[i]=specs[i].initial; } } init;
void Save() {
    if(folder.empty()) return;
    auto file=std::filesystem::path(folder)/"mobile-controls.cfg";
    auto temp=file; temp+=".tmp";
    std::ofstream output(temp,std::ios::trunc); if(!output) return;
    output<<"version 1\n";
    for(int i=0;i<SR_SETTING_COUNT;++i) output<<"setting "<<i<<' '<<choices[i]<<'\n';
    for(int i=0;i<SR_CONTROL_COUNT;++i) output<<"control "<<i<<' '<<placements[i].x<<' '<<placements[i].y<<' '<<placements[i].hidden<<'\n';
    output.close(); if(!output) return;
    std::error_code error; std::filesystem::rename(temp,file,error);
}
SRMobilePlacement Clamp(SRMobilePlacement p,int i) {
    if(!std::isfinite(p.x)||!std::isfinite(p.y)) return defaults[i];
    p.x=std::clamp(p.x,0.f,1.f); p.y=std::clamp(p.y,0.f,1.f); p.hidden=!!p.hidden; return p;
}
}
extern "C" void SRMobileLoad(const char* directory) {
 std::lock_guard guard(lock); folder=directory?directory:"";
 std::ifstream input(std::filesystem::path(folder)/"mobile-controls.cfg"); std::string line;
 while(std::getline(input,line)) { std::istringstream in(line); std::string key; int i,v; in>>key;
    if(key=="version") { if(!(in>>v)||v!=1) break; }
    else if(key=="setting" && (in>>i>>v) && i>=0&&i<SR_SETTING_COUNT) choices[i]=std::clamp(v,0,int(specs[i].labels.size())-1);
    else if(key=="control" && (in>>i) && i>=0&&i<SR_CONTROL_COUNT) { SRMobilePlacement p{}; if(in>>p.x>>p.y>>p.hidden) placements[i]=Clamp(p,i); }
 }
 ++revision;
}
extern "C" uint64_t SRMobileRevision(){return revision.load();}
extern "C" int SRMobileGetSetting(int key){std::lock_guard guard(lock);return key>=0&&key<SR_SETTING_COUNT?choices[key]:0;}
extern "C" void SRMobileSetSetting(int key,int choice){std::lock_guard guard(lock);if(key<0||key>=SR_SETTING_COUNT)return;choices[key]=std::clamp(choice,0,int(specs[key].labels.size())-1);lookX=lookY=0;Save();++revision;}
extern "C" float SRMobileSettingValue(int key){std::lock_guard guard(lock);return key>=0&&key<SR_SETTING_COUNT?specs[key].values[choices[key]]:0;}
extern "C" const char* SRMobileSettingName(int key){return key>=0&&key<SR_SETTING_COUNT?specs[key].name:"";}
extern "C" int SRMobileSettingChoiceCount(int key){return key>=0&&key<SR_SETTING_COUNT?int(specs[key].labels.size()):0;}
extern "C" const char* SRMobileSettingChoice(int key,int choice){return key>=0&&key<SR_SETTING_COUNT&&choice>=0&&choice<int(specs[key].labels.size())?specs[key].labels[choice]:"";}
extern "C" const char* SRMobileControlName(int control){return control>=0&&control<SR_CONTROL_COUNT?names[control]:"";}
extern "C" SRMobilePlacement SRMobileGetPlacement(int control){std::lock_guard guard(lock);return control>=0&&control<SR_CONTROL_COUNT?placements[control]:SRMobilePlacement{};}
extern "C" void SRMobileSetPlacement(int control,SRMobilePlacement p){std::lock_guard guard(lock);if(control<0||control>=SR_CONTROL_COUNT)return;placements[control]=Clamp(p,control);Save();++revision;}
extern "C" void SRMobileReset(){std::lock_guard guard(lock);for(int i=0;i<SR_SETTING_COUNT;++i)choices[i]=specs[i].initial;placements=defaults;lookX=lookY=0;Save();++revision;}
extern "C" void SRMobileSetContext(int value){if(context.exchange(value)!=value)SRMobileClearLook();}
extern "C" int SRMobileGetContext(){return context.load();}
extern "C" void SRMobileAddLook(float x,float y){if(!std::isfinite(x)||!std::isfinite(y))return;std::lock_guard guard(lock);lookX=std::clamp(lookX+x,-500.f,500.f);lookY=std::clamp(lookY+y,-500.f,500.f);}
extern "C" void SRMobileTakeLook(float* x,float* y){std::lock_guard guard(lock);if(x)*x=lookX;if(y)*y=lookY;lookX=lookY=0;}
extern "C" void SRMobileClearLook(){std::lock_guard guard(lock);lookX=lookY=0;}
extern "C" void SRMobileBeginEdit(){edit.store(true);}
extern "C" int SRMobileTakeEditRequest(){return edit.exchange(false);}
namespace {std::atomic<bool> isEditing{false};}
extern "C" void SRMobileSetEditing(int value){isEditing=value!=0;SRMobileClearLook();}
extern "C" int SRMobileIsEditing(){return isEditing.load();}
extern "C" SRMobileVector SRMobileStick(float x,float y,float radius,float sensitivity,float deadzone){
 if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(radius)||radius<=0||!std::isfinite(sensitivity)||!std::isfinite(deadzone))return {};
 float length=std::hypot(x,y)/radius,dead=std::clamp(deadzone,0.f,.9f);if(length<=dead)return {};
 float magnitude=std::clamp((std::min(length,1.f)-dead)/(1-dead)*std::clamp(sensitivity,.1f,4.f),0.f,1.f);
 auto divisor=std::hypot(x,y);return {x/divisor*magnitude,y/divisor*magnitude};
}
