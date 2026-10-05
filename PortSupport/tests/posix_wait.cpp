#include <rex/thread.h>
#include <cassert>
#include <ctime>
#include <iostream>
using namespace std::chrono_literals;
int main() {
    using namespace rex::thread;
    auto a=Event::CreateAutoResetEvent(false), b=Event::CreateAutoResetEvent(false);
    WaitHandle* handles[]{a.get(),b.get()};
    auto start=std::chrono::steady_clock::now();auto cpu=std::clock();
    auto result=WaitAny(handles,2,true,100ms);
    auto elapsed=std::chrono::steady_clock::now()-start;
    assert(result.first==WaitResult::kTimeout && elapsed>=98ms);
    std::cout<<"Alertable 100 ms timeout: CPU "<<1000.0*(std::clock()-cpu)/CLOCKS_PER_SEC<<" ms\n";
    b->Set();result=WaitAny(handles,2,false,0ms);
    assert(result.first==WaitResult::kSuccess && result.second==1);
    assert(WaitAny(handles,2,false,0ms).first==WaitResult::kTimeout);
    a->Set();assert(WaitAll(handles,2,false,0ms)==WaitResult::kTimeout);
    b->Set();assert(WaitAll(handles,2,false,0ms)==WaitResult::kSuccess);
    assert(WaitAny(handles,2,false,0ms).first==WaitResult::kTimeout);
    std::thread signal([&]{std::this_thread::sleep_for(10ms);b->Set();});
    result=WaitAny(handles,2,true,100ms);signal.join();
    assert(result.first==WaitResult::kSuccess && result.second==1);
    std::cout<<"WaitAny/WaitAll, timeout, auto-reset and delayed signals passed\n";
}
