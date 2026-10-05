// Exercise the real mod's direct-IP transport against a protocol peer.
#include "wml.h"
#include "AppleCoopPlatform.h"
#include <cassert>
#include <filesystem>
#include <cstdio>
extern "C" int wml_mod_init(const WmlApi*,const WmlMod*);
extern "C" void WhompaysCoopHost();
extern "C" void WhompaysCoopJoin(const char*);
extern "C" void WhompaysCoopStop();
extern "C" int WhompaysCoopState();
extern "C" int WhompaysCoopChatSend(const char*);
extern "C" int WhompaysCoopChatPoll(char*,int);
static WmlFrameCallback frame;
static void Original(WmlContext*,uint8_t*) {}
int main(int argc,char** argv) {
    assert(argc==2); std::filesystem::create_directories(argv[1]);
    WmlMod mod{"Transport test",argv[1]}; WmlApi api{};
    api.version=WML_API_VERSION; api.size=sizeof(api);
    api.read_u8=[](uint32_t)->uint8_t{return 0;};
    api.read_u16=[](uint32_t)->uint16_t{return 0;};
    api.read_u32=[](uint32_t)->uint32_t{return 0;};
    api.read_f32=[](uint32_t)->float{return 0;};
    api.guest_pointer=[](uint32_t)->void*{return nullptr;};
    api.log=[](const WmlMod*,const char*){};
    api.hook=[](uint32_t,WmlGuestFunction,WmlGuestFunction* out){ *out=Original; return 0; };
    api.on_frame=[](WmlFrameCallback callback,void*){ frame=callback; };
    api.on_game_frame=[](WmlFrameCallback,void*){};
    api.key_pressed=[](int){return 0;}; api.key_down=api.key_pressed;
    api.overlay_text=[](const char*){};
    assert(wml_mod_init(&api,&mod)==0 && frame);
    int peer=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); assert(peer>=0);
    timeval timeout{2,0}; setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    sockaddr_in address{}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=htons(27015);
    WhompaysCoopHost(); frame(nullptr); assert(WhompaysCoopState()&1);
    const char hello[]="WHOMPAYS_COOP_V3_HELLO", ack[]="WHOMPAYS_COOP_V3_ACK";
    assert(sendto(peer,hello,sizeof(hello)-1,0,reinterpret_cast<sockaddr*>(&address),sizeof(address))==sizeof(hello)-1);
    char packet[1500]; auto n=recv(peer,packet,sizeof(packet),0);
    assert(n==sizeof(ack)-1 && !memcmp(packet,ack,n));
    for(int i=0;i<100 && !(WhompaysCoopState()&4);++i) Sleep(5);
    assert(WhompaysCoopState()&4);
    assert(WhompaysCoopChatSend("iPad protocol test"));
    bool chat=false;
    for(int i=0;i<100 && !chat;++i) { auto count=recv(peer,packet,sizeof(packet),0); if(count==184 && !memcmp(packet,"WCH1",4)) { chat=true; assert(!strcmp(packet+24,"iPad protocol test")); } }
    assert(chat);
    WhompaysCoopStop(); frame(nullptr); assert(WhompaysCoopState()==0); close(peer);
    peer=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); assert(peer>=0);
    setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    assert(bind(peer,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
    WhompaysCoopJoin("127.0.0.1"); frame(nullptr); assert(WhompaysCoopState()&2);
    sockaddr_in client{}; socklen_t size=sizeof(client);
    n=recvfrom(peer,packet,sizeof(packet),0,reinterpret_cast<sockaddr*>(&client),&size);
    assert(n==sizeof(hello)-1 && !memcmp(packet,hello,n));
    sendto(peer,ack,sizeof(ack)-1,0,reinterpret_cast<sockaddr*>(&client),size);
    for(int i=0;i<100 && !(WhompaysCoopState()&4);++i) Sleep(5);
    assert(WhompaysCoopState()&4);
    WhompaysCoopStop(); frame(nullptr); assert(WhompaysCoopState()==0); close(peer);
    puts("Real co-op host/join, V3 handshake, chat wire packet and disconnect passed.");
}
