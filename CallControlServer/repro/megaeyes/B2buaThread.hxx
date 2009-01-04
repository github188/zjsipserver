#ifndef REPRO_B2BUATHREAD__HXX
#define REPRO_B2BUATHREAD__HXX

#include "b2bua/B2BUA.hxx"
#include "rutil/ThreadIf.hxx"

namespace repro
{

class B2buaThread : public resip::ThreadIf
{
public:
    B2buaThread(b2bua::B2BUA& b2b);
    
    virtual void thread();
    virtual void shutdown();
protected:
    //  virtual void buildFdSet(FdSet& fdset);
    //  virtual unsigned int getTimeTillNextProcessMS() const;

private:
    b2bua::B2BUA& mB2BUA;
};

}

#endif
