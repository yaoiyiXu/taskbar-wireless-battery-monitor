#include "../src/drivers/drivers.h"
#include <cassert>
#include <cstdio>

static std::vector<BYTE> frame;
static int writes;
bool HidDev::Open(const HidColl&) { return true; }
void HidDev::Close() {}
void HidDev::Flush() {}
bool HidDev::Write(const std::vector<BYTE>& request, int) {
    assert(request.size() == 20 && request[0] == 0x13 && request[1] == 0x4A && request[2] == 1);
    for (size_t i = 3; i < request.size(); ++i) assert(request[i] == 0);
    ++writes;
    return true;
}
int HidDev::Read(std::vector<BYTE>& out, int) {
    if (frame.empty()) return -1;
    out = frame;
    frame.clear();
    return (int)out.size();
}
static void Checksum(std::vector<BYTE>& f) {
    unsigned sum=0;
    for (int i=0; i<19; ++i) sum+=f[i];
    f[19]=(BYTE)sum;
}
int main() {
    HidColl c;
    c.vid=0x41E4; c.pid=0x2001;
    c.caps.UsagePage=0xFF02; c.caps.Usage=2;
    c.caps.InputReportByteLength=c.caps.OutputReportByteLength=20;
    std::vector<BYTE> captured={0x13,0x4A,1,0,2,0x38,1,0,0,0,0,0,0,0,0,0,0,0,0,0x99};
    PollContext ctx; ctx.colls={c}; frame=captured;
    PollMchose(ctx);
    assert(ctx.out.size()==1 && ctx.out[0].percent==56);
    assert(ctx.out[0].kind==DeviceKind::Keyboard && ctx.IsClaimed(c.vid,c.pid));
    int before=writes; PollMchose(ctx); assert(writes==before);
    for (int test=0; test<5; ++test) {
        ctx=PollContext{}; ctx.colls={c}; frame=captured;
        if(test==0) frame.resize(6);
        if(test==1) frame[19]^=1;
        if(test==2) { frame[5]=101; Checksum(frame); }
        if(test==3) { frame[1]=0x0A; Checksum(frame); }
        if(test==4) frame.clear();
        PollMchose(ctx); assert(ctx.out.empty() && ctx.claimed.empty());
    }
    for (int level : {0,100}) {
        ctx=PollContext{}; ctx.colls={c}; frame=captured;
        frame[5]=(BYTE)level; Checksum(frame); PollMchose(ctx);
        assert(ctx.out.size()==1 && ctx.out[0].percent==level);
    }
    ctx=PollContext{}; c.pid=0x2002; ctx.colls={c};
    before=writes; PollMchose(ctx); assert(writes==before);
    puts("MCHOSE tests passed: captured reply, bounds, malformed replies, timeout, identity, deduplication.");
}
