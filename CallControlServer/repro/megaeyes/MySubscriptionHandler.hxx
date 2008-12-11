#if !defined(RESIP_MYSUBSCRIPTIONHANDLER_HXX)
#define RESIP_MYSUBSCRIPTIONHANDLER_HXX

#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"

#include <ext/hash_map>
#include <map>
#include <list>
#include <string>

namespace repro
{
    
class MyServerSubscriptionHandler: public resip::ServerSubscriptionHandler
{
public:
    MyServerSubscriptionHandler(resip::DialogUsageManager& dum)
	:resip::ServerSubscriptionHandler(),mDum(dum)
	{}
    virtual ~MyServerSubscriptionHandler()
	{}

    virtual void onNewSubscription( resip::ServerSubscriptionHandle handle, const resip::SipMessage& sub );
    virtual void onRefresh( resip::ServerSubscriptionHandle handle, const resip::SipMessage& sub );
    virtual void onExpired( resip::ServerSubscriptionHandle handle, resip::SipMessage& notify );
    virtual void onTerminated( resip::ServerSubscriptionHandle handle );    

    int NotifyUsers( std::string &devid, resip::Data &content );

private:
    typedef __gnu_cxx::hash_map<resip::Handled::Id, std::list<std::string> > ForwardMap;
    ForwardMap mForwardMap; //正向map,以用户为索引

    typedef std::map<std::string, std::list<resip::Handled::Id> > InvertMap;
    InvertMap mInvertMap; //反向map,以前端为索引

    resip::DialogUsageManager& mDum;
};

}
#endif

