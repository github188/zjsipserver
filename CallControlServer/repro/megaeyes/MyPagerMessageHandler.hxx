#if !defined(RESIP_MYPAGERMESSAGEHANDLER_HXX)
#define RESIP_MYPAGERMESSAGEHANDLER_HXX

#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"

#include <string>

namespace repro
{

class MyServerPagerMessageHandler : public resip::ServerPagerMessageHandler
{
public:
    MyServerPagerMessageHandler(resip::DialogUsageManager& dum)
	:resip::ServerPagerMessageHandler(),mDum(dum)
	{}

    ~MyServerPagerMessageHandler()
	{}

    virtual void onMessageArrived( resip::ServerPagerMessageHandle, const resip::SipMessage& message);

private:
    int parseMessage( const resip::Data &xmlBody, std::string &msgType, std::string &msgSrcId );

private:
    resip::DialogUsageManager& mDum;    
};

}

#endif
