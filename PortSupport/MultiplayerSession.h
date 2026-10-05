#pragma once
#include <string>
#include <cstdint>
namespace sr::multiplayer {
enum class Mode { CampaignCoop, CustomServer };
struct Capabilities { bool campaignCoop, directConnect, discovery, dedicatedServer; unsigned maxPeers; const char* protocol; };
// Future backends must negotiate new modes; legacy campaign packets stay intact.
class SessionBackend {
public:
 virtual ~SessionBackend()=default;
 virtual Capabilities capabilities() const=0;
 virtual int state() const=0;
 virtual bool host(Mode mode)=0;
 virtual bool join(const std::string& endpoint)=0;
 virtual void stop()=0;
};
SessionBackend& CampaignSession();
}
