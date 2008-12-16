#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "repro/megaeyes/MegaSubscriptionHandler.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "repro/megaeyes/XmlContents.hxx"
#include "rutil/Logger.hxx"
#include "repro/megaeyes/MarkupSTL.h"
#include <memory>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;

void 
MegaServerSubscriptionHandler::onNewSubscription( resip::ServerSubscriptionHandle handle, const resip::SipMessage& sub)
{
    Data xmlBody = sub.getContents()->getBodyData();

    CMarkupSTL SubXML;
    if ( xmlBody.size() > 0 )
    {
        SubXML.SetDoc( xmlBody.data() );
        if ( SubXML.FindElem() )
        {
	    SubXML.IntoElem();
	    while ( SubXML.FindElem() )
	    {
		std::string sTag = SubXML.GetTagName();

		if ( sTag == "IE_HEADER"  )
		{
		    std::string msgid  = SubXML.GetAttrib( "MessageType" );
		    std::string msgseq = SubXML.GetAttrib( "SequenceNumber" );
		    std::string msgsrcid = SubXML.GetAttrib( "SourceID" );
		    std::string msgdstid = SubXML.GetAttrib( "DestinationID" );
		    InfoLog ( << "Recv Subscribe body: " << " MessageType: " <<  msgid << " SequenceNumber " << msgseq 
			      << " SourceID " << msgsrcid << " DestinationID " << msgdstid  );
		
		    //�����û�������
		    ForwardMap::iterator i = mForwardMap.find( handle.getId() );
		    if ( i == mForwardMap.end() )
		    {
			std::list<std::string> devlist;
			devlist.push_front( msgdstid );
			mForwardMap[handle.getId()] = devlist;
		    }
		    else
		    {
			(i->second).push_front(msgdstid);
		    }

		    //����ǰ��������
		    InvertMap::iterator j = mInvertMap.find( msgdstid );
		    if ( j == mInvertMap.end() )
                    {
			std::list<Handled::Id> usrlist;
                        usrlist.push_front( handle.getId() );
                        mInvertMap[msgdstid] = usrlist;
                    }
                    else
                    {
                        (j->second).push_front( handle.getId() );
                    }
		}
		else if ( sTag == "IE_PU" )
		{
		}
	    }
        }
	else
	    InfoLog ( << "Recv Subscribe XML: " << xmlBody );
    }
  
    std::auto_ptr<Contents> respbody( new XmlContents(xmlBody) ); 

    resip::SharedPtr<SipMessage> resp = handle->accept(200);
    resp->setContents( respbody );

    handle->send( resp );
}

void
MegaServerSubscriptionHandler::onRefresh( resip::ServerSubscriptionHandle handle, const resip::SipMessage& sub)
{
}
 
void 
MegaServerSubscriptionHandler::onExpired( resip::ServerSubscriptionHandle handle, resip::SipMessage& notify)
{

}

void 
MegaServerSubscriptionHandler::onTerminated( resip::ServerSubscriptionHandle handle )
{
    //�����û�������
    ForwardMap::iterator i = mForwardMap.find( handle.getId() );
    if ( i != mForwardMap.end() )
    {
	while ( !(i->second).empty() )
	{
	    //�������б�����ǰ��
	    std::string devid = (i->second).front();
	    InvertMap::iterator j = mInvertMap.find( devid );
	    if ( j != mInvertMap.end() )
	    {
		(j->second).remove(handle.getId()); //�Ѷ��Ĵ�ǰ�˶����û���ɾ��
	    }
	    (i->second).pop_front();
	}
	
	mForwardMap.erase(i);//��������ļ�¼ɾ��
    }
}

int MegaServerSubscriptionHandler::NotifyUsers( std::string &devid, resip::Data &xmlBody )
{
    InvertMap::iterator i = mInvertMap.find( devid );

    if ( i != mInvertMap.end() )
    {
	InfoLog( << "Terminal : " << devid << " Alarm Subscribe Relation is Founded!!!" );	

	//�������Ĵ�ǰ�˵�����subscriber
	std::list<resip::Handled::Id>::iterator h = (i->second).begin();
	for ( ; h!=(i->second).end() ;++h )
	{
	    InfoLog( << "Terminal : " << devid << " Alarm Subscriber is "<< *h );	

	    ServerSubscriptionHandle handle(mDum, *h);//��Ҫ���쳣����!!!

	    std::auto_ptr<Contents> reqbody( new XmlContents(xmlBody) ); 
	    resip::SharedPtr<SipMessage> req = handle->update( reqbody.get() );
	    handle->send( req );
	}
    }
    else
    {
	InfoLog( << "Terminal : " << devid << " Alarm Subscribe is Not Founded!!!" );
    }
    
    return 0;
}

