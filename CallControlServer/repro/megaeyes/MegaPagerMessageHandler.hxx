#if !defined(RESIP_MEGAPAGERMESSAGEHANDLER_HXX)
#define RESIP_MEGAPAGERMESSAGEHANDLER_HXX

#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"

#include <string>

namespace repro
{

class MegaServerPagerMessageHandler : public resip::ServerPagerMessageHandler
{
public:
    MegaServerPagerMessageHandler(resip::DialogUsageManager& dum)
	:resip::ServerPagerMessageHandler(),mDum(dum)
	{}

    ~MegaServerPagerMessageHandler()
	{}

    virtual void onMessageArrived( resip::ServerPagerMessageHandle, const resip::SipMessage& message);

private:
    int parseMessage( const resip::Data &xmlBody, std::string &msgType, std::string &msgSrcId );

private:
    resip::DialogUsageManager& mDum;    
};

class MegaClientPagerMessageHandler : public resip::ClientPagerMessageHandler
{
public:
    MegaClientPagerMessageHandler()
	:resip::ClientPagerMessageHandler()
	{}
    
    ~MegaClientPagerMessageHandler()
	{}
    
    virtual void onSuccess( resip::ClientPagerMessageHandle, const resip::SipMessage& status);

    virtual void onFailure( resip::ClientPagerMessageHandle, const resip::SipMessage& status, std::auto_ptr<resip::Contents> contents);

private:
//    int parseMessage( const resip::Data &xmlBody, std::string &msgType, std::string &msgSrcId );
};

}

#endif
