#include "repro/megaeyes/B2buaThread.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

B2buaThread::B2buaThread(b2bua::B2BUA& b2b)
    :mB2BUA(b2b)   
{}

void
B2buaThread::thread()
{
    mB2BUA.run();
}

void
B2buaThread::shutdown()
{
    mB2BUA.stop();
}


