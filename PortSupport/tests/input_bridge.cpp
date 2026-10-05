#include "SRInput.h"
#include <cassert>
#include <thread>
int main() {
    SRInputReset();
    auto zero = SRInputRead();
    SRInputState t{}; t.buttons=0x1000; t.leftX=12000; t.leftY=16000; t.rightTrigger=255;
    SRInputSetTouch(t);
    auto a = SRInputRead(); assert(a.buttons==0x1000 && a.packet==zero.packet+1 && a.rightTrigger==255);
    SRInputSetTouch(t); assert(SRInputRead().packet==a.packet);
    SRInputState c{}; c.buttons=0x2000; c.leftX=-24000; c.leftY=0; c.leftTrigger=128;
    SRInputSetController(c);
    a=SRInputRead(); assert(a.buttons==0x3000 && a.leftX==-24000 && a.leftY==0 && a.leftTrigger==128);
    SRInputSetController({}); a=SRInputRead(); assert(a.buttons==0x1000 && a.leftX==12000 && a.leftY==16000);
    // A hardware callback already queued when the app suspends must not
    // resurrect buttons after input has been reset.
    SRInputReset(); SRInputSetController(c); SRInputSetControllerActive(0);
    SRInputSetController(c); a=SRInputRead(); assert(a.buttons==0 && a.leftX==0);
    SRInputSetControllerActive(1); SRInputSetController(c);
    a=SRInputRead(); assert(a.buttons==0x2000 && a.leftX==-24000);
    SRInputReset();
    std::thread writer([&] { for(int i=0;i<10000;++i) SRInputSetTouch(i%2 ? t : SRInputState{}); });
    for(int i=0;i<10000;++i) { a=SRInputRead(); assert(a.buttons==0 || a.buttons==0x1000); }
    writer.join(); SRInputReset(); a=SRInputRead(); assert(a.buttons==0 && a.leftX==0 && a.rightTrigger==0);
}
