#pragma once
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <cstring>
#include <algorithm>
using SOCKET = int;
constexpr int INVALID_SOCKET = -1;
struct WSADATA {};
inline int WSAStartup(int, WSADATA*) { return 0; }
inline void WSACleanup() {}
inline int MAKEWORD(int a, int b) { return a | (b << 8); }
inline int closesocket(int s) { return close(s); }
inline int ioctlsocket(int s, unsigned long c, unsigned long* v) { int flag = int(*v); return ioctl(s, c, &flag); }
inline uint64_t GetTickCount64() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline unsigned GetCurrentProcessId() { return getpid(); }
inline void Sleep(unsigned ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline bool DeleteFileA(const char* path) { return unlink(path) == 0; }
constexpr unsigned INVALID_FILE_ATTRIBUTES = ~0u;
inline unsigned GetFileAttributesA(const char* path) { return access(path, F_OK) == 0 ? 0 : INVALID_FILE_ATTRIBUTES; }
constexpr int VK_F6=0x75, VK_F7=0x76, VK_F8=0x77, VK_F9=0x78;
inline std::string SRIniValue(const char* section, const char* key, const char* fallback, const char* path) {
    std::ifstream file(path); std::string line, current;
    auto trim=[](std::string s) { auto first=s.find_first_not_of(" \t\r"); if(first==s.npos) return std::string(); return s.substr(first,s.find_last_not_of(" \t\r")-first+1); };
    while(std::getline(file,line)) {
        line=trim(line); if(line.empty() || line[0]==';' || line[0]=='#') continue;
        if(line.front()=='[' && line.back()==']') { current=trim(line.substr(1,line.size()-2)); continue; }
        auto equal=line.find('=');
        if(current==section && equal!=line.npos && trim(line.substr(0,equal))==key) return trim(line.substr(equal+1));
    }
    return fallback;
}
inline unsigned GetPrivateProfileStringA(const char* s,const char* k,const char* d,char* out,unsigned size,const char* p) {
    auto value=SRIniValue(s,k,d,p); if(!size) return 0; auto n=std::min<size_t>(size-1,value.size()); memcpy(out,value.data(),n); out[n]=0; return unsigned(n);
}
inline unsigned GetPrivateProfileIntA(const char* s,const char* k,int d,const char* p) {
    auto v=SRIniValue(s,k,"",p); if(v.empty()) return d; char* end; auto n=strtol(v.c_str(),&end,0); return end==v.c_str()?d:unsigned(n);
}
inline std::string SRCoopLocalIp() {
    ifaddrs* interfaces=nullptr; std::string result="127.0.0.1";
    if(getifaddrs(&interfaces)!=0) return result;
    for(auto* i=interfaces;i;i=i->ifa_next) {
        if(!i->ifa_addr || i->ifa_addr->sa_family!=AF_INET || !(i->ifa_flags&IFF_UP) || (i->ifa_flags&IFF_LOOPBACK)) continue;
        char text[INET_ADDRSTRLEN]{}; inet_ntop(AF_INET,&reinterpret_cast<sockaddr_in*>(i->ifa_addr)->sin_addr,text,sizeof(text));
        result=text; if(std::string(i->ifa_name)=="en0") break;
    }
    freeifaddrs(interfaces); return result;
}
#include <strings.h>
#define _stricmp strcasecmp
struct LARGE_INTEGER { int64_t QuadPart=0; };
inline bool QueryPerformanceFrequency(LARGE_INTEGER* value) { value->QuadPart=1000000000; return true; }
inline bool QueryPerformanceCounter(LARGE_INTEGER* value) { value->QuadPart=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); return true; }
