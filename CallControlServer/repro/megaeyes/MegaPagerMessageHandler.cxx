#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "repro/megaeyes/MegaPagerMessageHandler.hxx"
#include "resip/dum/ServerPagerMessage.hxx"
#include "repro/megaeyes/MegaSubscriptionHandler.hxx"
#include "rutil/Logger.hxx"
#include "repro/megaeyes/XmlContents.hxx"
#include "repro/megaeyes/MarkupSTL.h"
#include "repro/megaeyes/Common.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

void
MegaServerPagerMessageHandler::onMessageArrived( resip::ServerPagerMessageHandle handle, const resip::SipMessage& message )
{
    Data xmlReqBody = message.getContents()->getBodyData();
    
    //���ͱ���Ϣ����Ӧ
    std::string respXmlBody;
    common::MakeXmlReponse( xmlReqBody.data(), respXmlBody );
    Data xmlRespBody( resip::Data::Share, respXmlBody.c_str(), respXmlBody.size() );
    std::auto_ptr<Contents> respbody( new XmlContents(xmlRespBody) ); 
    resip::SharedPtr<SipMessage> resp = handle->accept(200);
    resp->setContents( respbody );
    handle->send( resp );//!!!ע��ServerPagerMessage::send������ִ��delete this,��֮�µ���䲻���ٵ���handle

    //���ͱ�����Ϣ
    std::string MsgType;
    std::string MsgSrcId;

    int ret = parseMessage( xmlReqBody, MsgType, MsgSrcId );
    if ( !ret && MsgType=="MSG_ALARM_TRANS_REQ" )
    {
	MegaServerSubscriptionHandler *subhandler = dynamic_cast<MegaServerSubscriptionHandler*>( mDum.getServerSubscriptionHandler("alarm") );
	if ( subhandler )
	{
	    subhandler->NotifyUsers( MsgSrcId, xmlReqBody );
	    InfoLog( << "Terminal : " << MsgSrcId << " Alarm Begin!!!" );
	}
    }
}

int 
MegaServerPagerMessageHandler::parseMessage( const resip::Data &xmlBody, std::string &msgType, std::string &msgSrcId )
{
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
		    msgType  = SubXML.GetAttrib( "MessageType" );
		    std::string msgseq = SubXML.GetAttrib( "SequenceNumber" );
		    msgSrcId = SubXML.GetAttrib( "SourceID" );
		    std::string msgdstid = SubXML.GetAttrib( "DestinationID" );
		    InfoLog ( << "Recv Message body: " << " MessageType: " <<  msgType << " SequenceNumber " << msgseq 
			      << " SourceID " << msgSrcId << " DestinationID " << msgdstid  );
		}
	    }

	    return 0;
        }
	else
	{
	    InfoLog ( << "Recv Message XML: " << xmlBody );
	}
    }

    return -1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

void MegaClientPagerMessageHandler::onSuccess( resip::ClientPagerMessageHandle, const resip::SipMessage& status )
{
    //!!!������proxy�򱾵�ǰ�˷���Messageָ��,�յ���Ӧ�󷵻ظ�http server
    return ;
}

void MegaClientPagerMessageHandler::onFailure( resip::ClientPagerMessageHandle handle, const resip::SipMessage& status, std::auto_ptr<resip::Contents> contents)
{
    int sn = common::msgmap[handle.getId()];

    InfoLog( << "Recv fail Message Resp: " << status.brief() << " contents: " << contents->getBodyData() << " pagenumber is "<< sn ) ;

    

    return ;
}

