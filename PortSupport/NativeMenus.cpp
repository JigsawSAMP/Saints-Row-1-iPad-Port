// Extend the game's guest lists, using its own selectors, draw functions and sounds.
// Guest object layout/entry points are documented by SaintsReborn's MIT-licensed
// options_menu.cpp and coop_menu.cpp. No custom settings sheet is used.
#include "NativeMenus.h"
#include "MobileControls.h"
#include "MultiplayerSession.h"
#include "NativeGame.h"
#include "saintsrow_config.h"
#include "saintsrow_init.h"
#include <rex/ppc/function.h>
#include <rex/system/kernel_state.h>
#include <rex/logging.h>
#include <algorithm>
#include <arpa/inet.h>
#include <bit>

#include <atomic>
#include <chrono>
#include <cstring>
#include <array>
#include <fstream>
#include <string>
#include <vector>
#define DECL(A) extern "C" void __imp__sub_##A(PPCContext&,uint8_t*);
DECL(8228BAB0) DECL(8228CAD8) DECL(8228BE98) DECL(822903E8)
DECL(822852B0) DECL(82348638) DECL(822B7328) DECL(822B7D68) DECL(82349018) DECL(822861F0)
DECL(82348750) DECL(8228F878) DECL(8228B9D0) DECL(8216EE80)
DECL(8216ED48) DECL(82288750) DECL(8216FCA0) DECL(8228CBA0)
DECL(8228CC90) DECL(82292CC8) DECL(82292D68) DECL(82287FC0)
#undef DECL
namespace {
constexpr uint32_t current=0x8370DDAC,requested=0x8370DDB0,listPointer=0x8370DDC4,table=0x827B05F8;
constexpr uint32_t optionsId=43,multiplayerId=46,mobileId=47;
uint32_t R(uint8_t* b,uint32_t a){uint32_t v;memcpy(&v,b+a,4);return __builtin_bswap32(v);}
void W(uint8_t* b,uint32_t a,uint32_t v){v=__builtin_bswap32(v);memcpy(b+a,&v,4);}
uint16_t R16(uint8_t* b,uint32_t a){return uint16_t(b[a])<<8|b[a+1];}
void W16(uint8_t* b,uint32_t a,uint16_t v){b[a]=v>>8;b[a+1]=v;}
uint32_t Allocate(uint8_t* b,size_t bytes){auto a=REX_KERNEL_STATE()->memory()->SystemHeapAlloc(uint32_t(bytes));if(a)memset(b+a,0,bytes);return a;}
void Text(uint8_t* b,uint32_t a,const std::string& s,size_t capacity=128){size_t n=std::min(s.size(),capacity/2-1);for(size_t i=0;i<n;++i){b[a+i*2]=0;b[a+i*2+1]=uint8_t(s[i]);}W16(b,a+uint32_t(n*2),0);}
void Call(PPCContext& c,uint8_t* b,PPCFunc* fn,uint64_t value=0){rex::CallFrame f(c);f.ctx.r3.u64=value;fn(f.ctx,b);}
bool Test(PPCContext& c,uint8_t* b,PPCFunc* fn,uint32_t code){rex::CallFrame f(c);f.ctx.r3.u64=code;fn(f.ctx,b);return (f.ctx.r3.u64&0xff)!=0;}
bool Ready(PPCContext& c,uint8_t* b){return Test(c,b,__imp__sub_82288750,0);}
bool Confirm(PPCContext& c,uint8_t* b){return Ready(c,b)&&Test(c,b,__imp__sub_8216EE80,55);}
void Add(PPCContext& c,uint8_t* b,uint32_t text,uint32_t item=0,bool enabled=true){rex::CallFrame f(c);f.ctx.r3.u64=text;f.ctx.r4.u64=item;f.ctx.r5.u64=0;f.ctx.r6.u64=~0ull;f.ctx.r7.u64=enabled;f.ctx.r8.u64=0;__imp__sub_8228BAB0(f.ctx,b);}
uint32_t textTouch=0,textMultiplayer=0;
uint32_t optionsList=0;int optionsRow=-1,mainRow=-1;uint32_t mainObject=0;
uint32_t parentMobile=optionsId,parentMultiplayer=0;
bool openedFromMain=false;
struct Row{std::string label;std::vector<std::string> choices;int setting=-1,control=-1,action=0;bool enabled=true;uint32_t text=0,item=0;};
std::vector<Row> rows;
uint32_t customList=0;int page=0;
uint64_t shownRevision=0;
bool editingAddress=false;
std::array<unsigned,4> addressParts{192,168,0,1};int addressPart=0;
std::string Address(){return std::to_string(addressParts[0])+"."+std::to_string(addressParts[1])+"."+std::to_string(addressParts[2])+"."+std::to_string(addressParts[3]);}
std::string AddressLabel(){std::string s="IP: ";for(int i=0;i<4;++i){if(i)s+=".";if(i==addressPart)s+="[";s+=std::to_string(addressParts[i]);if(i==addressPart)s+="]";}return s+"  D-pad edit, A join, B back";}

std::string pendingAddress;int pendingSession=0;
std::chrono::steady_clock::time_point playingSince{};
std::string lastIP="";
void RegisterPages(uint8_t* b){
 if(!textTouch){textTouch=Allocate(b,128);textMultiplayer=Allocate(b,128);Text(b,textTouch,"iPad Display");Text(b,textMultiplayer,"MULTIPLAYER");}
 for(auto id:{mobileId,multiplayerId})for(uint32_t i=0;i<4;++i)W(b,table+id*16+i*4,R(b,table+optionsId*16+i*4));
}
void Selector(uint8_t* b,Row& row,int choice){
 memset(b+row.item,0,1024);W(b,row.item,0x82068024);
 auto colour=R(b,0x82816CF0);uint32_t at=row.item+1024;
 for(uint32_t i=0;i<64;++i){W(b,row.item+272+i*4,~0u);W(b,row.item+528+i*4,colour);}
 for(size_t i=0;i<row.choices.size();++i){Text(b,at,row.choices[i]);W(b,row.item+16+uint32_t(i)*4,at);at+=128;}
 W(b,row.item+784,uint32_t(row.choices.size()));W(b,row.item+788,std::clamp(choice,0,int(row.choices.size())-1));b[row.item+792]=1;
}
void Build(PPCContext& c,uint8_t* b,int kind){
 page=kind;rows.clear();
 if(kind==1){
  Row row;row.label=SRMobileSettingName(SR_RENDER_PROFILE);row.setting=SR_RENDER_PROFILE;
  for(int n=0;n<SRMobileSettingChoiceCount(SR_RENDER_PROFILE);++n)row.choices.emplace_back(SRMobileSettingChoice(SR_RENDER_PROFILE,n));
  rows.push_back(std::move(row));

 }else{
  auto& session=sr::multiplayer::CampaignSession();auto caps=session.capabilities();auto state=session.state();
  rows.push_back({"Campaign Co-op - Host",{},-1,-1,3,caps.campaignCoop&&!(state&3)});
  rows.push_back({"Direct Connect - Co-op",{},-1,-1,4,caps.directConnect&&!(state&3)});
  rows.push_back({"Disconnect",{},-1,-1,5,(state&3)!=0});
  rows.push_back({"Server Browser (unavailable)",{},-1,-1,0,caps.discovery});
  rows.push_back({"Host Server (unavailable)",{},-1,-1,0,caps.dedicatedServer});
  rows.push_back({"Co-op uses direct IPv4 / UDP 27015",{},-1,-1,0,false});
  if(openedFromMain)rows.push_back({"Choose a story save to host or join",{},-1,-1,0,false});
 }
 rows.push_back({"Back",{},-1,-1,6});
 Call(c,b,__imp__sub_82348750);Call(c,b,__imp__sub_8228F878);
 W(b,0x82FFE3E0,0x8203C19C);b[0x82FFE3DC]=0;
 {rex::CallFrame f(c);f.ctx.r3.u64=0;f.ctx.r4.u64=0;f.ctx.r5.u64=0;f.ctx.r6.u64=1;f.ctx.r7.u64=0;__imp__sub_8228B9D0(f.ctx,b);}
 customList=R(b,listPointer);
 for(auto& row:rows){
  // Reuse a pool on each page rebuild rather than allocating per navigation.
  static std::vector<uint32_t> blocks;auto index=size_t(&row-rows.data());
  if(blocks.size()<=index)blocks.push_back(Allocate(b,2048));row.item=blocks[index];row.text=row.item+1920;
  if(!row.item){REXLOG_ERROR("Mobile menu guest allocation failed");continue;}
  Text(b,row.text,row.label);
  if(!row.choices.empty()){
   auto choice=row.setting>=0?SRMobileGetSetting(row.setting):SRMobileGetPlacement(row.control).hidden;
   Selector(b,row,choice);
   // Text follows the choices (at most six); keep it outside selector storage.
   Text(b,row.text,row.label);
  }
  Add(c,b,row.text,row.choices.empty()?0:row.item,row.enabled);
 }
 {rex::CallFrame f(c);f.ctx.r3.u64=~0ull;f.ctx.r4.u64=~0ull;f.ctx.r6.u64=1;f.ctx.f1.f64=std::bit_cast<float>(R(b,0x820875EC));__imp__sub_8228CAD8(f.ctx,b);}
 Call(c,b,__imp__sub_8228BE98,0);W(b,0x8370DDCC,1);W(b,0x83710690+88,0);W(b,0x83710690+96,0);
 for(uint32_t o:{32u,33u,34u,35u,36u,37u,72u,73u,74u,75u,76u,77u})b[0x83710690+o]=1;
 shownRevision=SRMobileRevision();
 REXLOG_INFO("Native {} list built: {} rows",kind==1?"iPad Display":"Multiplayer",rows.size());
}
void LoadForSession(PPCContext& c,uint8_t* b,int action,const std::string& address){
 pendingSession=action;pendingAddress=address;playingSince={};
 if(openedFromMain){W(b,0x827AF780,0);W(b,0x8300FD3C,4);W(b,0x8300FD40,4);W(b,requested,44);}
 else{
  // Request Resume by the game's normal pause close flag, as common tab update does.
  // The pause menu closes through the same request handler used by Start/B.
  // Input-table pointer 0x82340610 is what sub_822C06B8 passes to 82348638.
  Call(c,b,__imp__sub_82348638,0x82340610);
 }
}
void Update(PPCContext& c,uint8_t* b){
 auto menu=R(b,listPointer);if(menu!=customList||!menu)return;
 if(editingAddress){
  if(!Ready(c,b))return;
  if(Test(c,b,__imp__sub_8216EE80,56)){editingAddress=false;W(b,requested,multiplayerId);return;}
  if(Test(c,b,__imp__sub_8216ED48,39))addressPart=(addressPart+3)%4;
  else if(Test(c,b,__imp__sub_8216ED48,40))addressPart=(addressPart+1)%4;
  if(Test(c,b,__imp__sub_8216ED48,37))addressParts[addressPart]=(addressParts[addressPart]+1)%256;
  else if(Test(c,b,__imp__sub_8216ED48,38))addressParts[addressPart]=(addressParts[addressPart]+255)%256;
  if(rows.size()>1)Text(b,rows[1].text,AddressLabel());
  if(Test(c,b,__imp__sub_8216EE80,55)){
   auto address=Address();in_addr ip{};
   if(inet_pton(AF_INET,address.c_str(),&ip)==1 && ip.s_addr!=INADDR_ANY && ip.s_addr!=INADDR_BROADCAST){
    lastIP=address;std::ofstream("coop-last-ip.txt",std::ios::trunc)<<lastIP<<"\n";
    editingAddress=false;LoadForSession(c,b,2,lastIP);
   }
  }
  return;
 }
 auto cursor=int(R16(b,menu+62));if(cursor<0||cursor>=int(rows.size()))return;
 auto& row=rows[size_t(cursor)];
 if(Ready(c,b)){
  if(Test(c,b,__imp__sub_8216EE80,56)){W(b,requested,page==1?parentMobile:parentMultiplayer);return;}
  bool left=Test(c,b,__imp__sub_8216ED48,39),right=!left&&Test(c,b,__imp__sub_8216ED48,40);
  if(!row.choices.empty()&&(left||right)){
   rex::CallFrame f(c);f.ctx.r3.u64=row.item;f.ctx.r4.u64=1;(left?__imp__sub_82292CC8:__imp__sub_82292D68)(f.ctx,b);
   int choice=int(R(b,row.item+788));
   if(choice>=0&&choice<int(row.choices.size())){
    if(row.setting>=0)SRMobileSetSetting(row.setting,choice);
    else{auto p=SRMobileGetPlacement(row.control);p.hidden=choice;SRMobileSetPlacement(row.control,p);}
   }
  }else if(row.enabled&&Confirm(c,b)){
   switch(row.action){
    case 1:SRMobileBeginEdit();break;
    case 2:SRMobileReset();W(b,requested,mobileId);break;
    case 3:LoadForSession(c,b,1,{});break;
    case 4:{
      if(lastIP.empty()){std::ifstream saved("coop-last-ip.txt");saved>>lastIP;}
      in_addr ip{};if(inet_pton(AF_INET,lastIP.c_str(),&ip)==1){auto host=ntohl(ip.s_addr);for(int i=0;i<4;++i)addressParts[i]=(host>>(24-i*8))&255;}
      addressPart=0;editingAddress=true;Text(b,rows[1].text,AddressLabel());break;
    }
    case 5:sr::multiplayer::CampaignSession().stop();W(b,requested,multiplayerId);break;
    case 6:W(b,requested,page==1?parentMobile:parentMultiplayer);break;
   }
  }
  if(Test(c,b,__imp__sub_8216ED48,37))Call(c,b,__imp__sub_8228CBA0,1);
  else if(Test(c,b,__imp__sub_8216ED48,38))Call(c,b,__imp__sub_8228CC90,1);
 }
 if(page==1&&shownRevision!=SRMobileRevision()){
  for(auto& r:rows)if(!r.choices.empty())W(b,r.item+788,r.setting>=0?SRMobileGetSetting(r.setting):SRMobileGetPlacement(r.control).hidden);
  shownRevision=SRMobileRevision();
 }
}
}
PPC_FUNC(sub_8228CAD8){
 auto lr=uint32_t(ctx.lr);auto menu=R(base,listPointer);
 if(lr==0x822861B8 && menu){RegisterPages(base);mainRow=int(R16(base,menu+58));Add(ctx,base,textMultiplayer);}
 if((lr==0x822B7C04||lr==0x82348F50)&&menu){RegisterPages(base);optionsList=menu;optionsRow=int(R16(base,menu+58));Add(ctx,base,textTouch);}
 __imp__sub_8228CAD8(ctx,base);
}
PPC_FUNC(sub_822903E8){
 auto lr=uint32_t(ctx.lr),id=ctx.r5.u32;__imp__sub_822903E8(ctx,base);
 if(id==optionsId&&lr==0x822BFCB0&&!base[0x8370E9F6]){RegisterPages(base);rex::CallFrame f(ctx);f.ctx.r3.u64=textMultiplayer;f.ctx.r4.u64=0;f.ctx.r5.u64=multiplayerId;__imp__sub_822903E8(f.ctx,base);}
}
PPC_FUNC(sub_822B7328){auto id=R(base,requested);if(id==mobileId||id==multiplayerId){Build(ctx,base,id==mobileId?1:2);return;}__imp__sub_822B7328(ctx,base);}
bool ExtendedOptions(PPCContext& c,uint8_t* b){auto menu=R(b,listPointer);if(menu==optionsList&&optionsRow>=0&&R16(b,menu+62)==optionsRow&&Confirm(c,b)){RegisterPages(b);parentMobile=R(b,current);W(b,requested,mobileId);return true;}return false;}
PPC_FUNC(sub_822B7D68){auto id=R(base,current);if(id==mobileId||id==multiplayerId){if(id==multiplayerId&&!openedFromMain)parentMultiplayer=optionsId;Update(ctx,base);return;}if(!ExtendedOptions(ctx,base))__imp__sub_822B7D68(ctx,base);}
PPC_FUNC(sub_82349018){if(!ExtendedOptions(ctx,base))__imp__sub_82349018(ctx,base);}
PPC_FUNC(sub_822852B0){
 auto object=ctx.r3.u32;bool restore=object&&R16(base,object+8)==0xffff;
 if(restore)W16(base,object+8,0);
 __imp__sub_822852B0(ctx,base);
 if(restore&&mainRow>=0){Call(ctx,base,__imp__sub_8228BE98,uint32_t(mainRow));W16(base,object+8,0xffff);}
}
PPC_FUNC(sub_822861F0){
 mainObject=ctx.r3.u32;auto menu=R(base,listPointer);auto cursor=menu?R16(base,menu+62):uint16_t(0xffff);
 if(menu&&mainRow>=0&&cursor==mainRow){
  W16(base,mainObject+8,0xffff);
  if(Ready(ctx,base)&&Test(ctx,base,__imp__sub_8216FCA0,0)){RegisterPages(base);openedFromMain=true;parentMultiplayer=R(base,current);W16(base,mainObject+8,0);W(base,requested,multiplayerId);ctx.r3.u64=1;return;}
 }else if(menu)W16(base,mainObject+8,cursor);
 __imp__sub_822861F0(ctx,base);
 if(menu&&mainRow>=0&&R16(base,menu+62)==mainRow)W16(base,mainObject+8,0xffff);
}
void SRNativeMenuPoll(uint8_t* b){
 if(R(b,current)==optionsId)openedFromMain=false;
 if(!pendingSession)return;
 bool playing=R(b,0x8309ABEC)&&R(b,0x839E0DF8)==0&&!b[0x8370D991]&&!b[0x8370D990];
 if(!playing){playingSince={};return;}
 auto now=std::chrono::steady_clock::now();if(playingSince.time_since_epoch().count()==0)playingSince=now;
 if(now-playingSince<std::chrono::seconds(3))return;
 int action=pendingSession;pendingSession=0;auto& session=sr::multiplayer::CampaignSession();
 bool ok=action==1?session.host(sr::multiplayer::Mode::CampaignCoop):session.join(pendingAddress);
 REXLOG_INFO("Campaign session request {}: {}",action,ok);
}
