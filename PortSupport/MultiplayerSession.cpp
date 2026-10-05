#include "MultiplayerSession.h"
#include "NativeGame.h"
#include <arpa/inet.h>
namespace sr::multiplayer {
namespace {
class LegacyCoop final:public SessionBackend {
public:
 Capabilities capabilities() const override { return {true,true,false,false,2,"WhompaysCoop V3 / pinned 1.63"}; }
 int state() const override { return SRNativeCoopState(); }
 bool host(Mode mode) override { if(mode!=Mode::CampaignCoop || state()<0)return false;SRNativeCoopHost();return true; }
 bool join(const std::string& endpoint) override { in_addr ip{}; if(inet_pton(AF_INET,endpoint.c_str(),&ip)!=1||ip.s_addr==INADDR_ANY||ip.s_addr==INADDR_BROADCAST||state()<0)return false;SRNativeCoopJoin(endpoint.c_str());return true; }
 void stop() override {SRNativeCoopStop();}
};
}
SessionBackend& CampaignSession(){static LegacyCoop session;return session;}
}
