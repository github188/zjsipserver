
#include <exception>
#include <iostream>

#include "Logging.hxx"
#include "MediaProxy.hxx"

#include "resip/dum/megaeyes/MegaSipEntity.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

#define UPTOMAXCONN 1

bool MediaProxy::mNatHelper = true; //zhangjun change from false to true

void MediaProxy::setNatHelper(bool natHelper) 
{
    mNatHelper = natHelper;
}

MediaProxy::MediaProxy(MediaManager& mediaManager) 
    : mediaManager(mediaManager) 
{
    originalSdp = NULL;
    newSdp = NULL;
}

MediaProxy::MediaProxy( MediaManager& mediaManager, const resip::Data& callid )
    : mediaManager(mediaManager),
      iD_(callid)
{
    originalSdp = NULL;
    newSdp = NULL;
}

MediaProxy::~MediaProxy() 
{
    if(originalSdp != NULL)
	delete originalSdp;
    if(newSdp != NULL) 
	delete newSdp;
}

//!!!Transport have 2 types, 1 is c2t:client to terminal, 2 is c2v2t:client to vtdu to terminal
int MediaProxy::updateSdp ( const resip::SdpContents& sdp, const in_addr_t& msgSourceAddress ) 
{
    B2BUA_LOG_DEBUG( <<"B2BCall MediaProxy::updateSdp start!" );
    bool callerAsymmetric = true;
    bool calleeAsymmetric = true;
    if(originalSdp != NULL)
    {
	delete originalSdp;
    }

    B2BUA_LOG_DEBUG( <<"B2BCall MediaProxy::updateSdp clone sdp to originalsdp!" );
    originalSdp = (SdpContents *)sdp.clone();

    if(newSdp != NULL)
    {
	delete newSdp;
    }
    B2BUA_LOG_DEBUG( <<"B2BCall MediaProxy::updateSdp clone sdp to newsdp!" );
    newSdp = (SdpContents *)sdp.clone();

    // Process the Origin
    if(originalSdp->session().origin().getAddressType() != SdpContents::IP4) 
    {
	// FIXME - implement IPv6
	B2BUA_LOG_WARNING( <<"processing SDP origin, only IP4 is supported"); 
	return MM_SDP_BAD;
    }

    // Process the default connection
    if(originalSdp->session().connection().getAddressType() != SdpContents::IP4) 
    {
	// FIXME - implement IPv6
	B2BUA_LOG_WARNING( <<"processing SDP connection, only IP4 is supported");
	return MM_SDP_BAD;
    }
    
    //!!!below, begin change sdp infomations. But if transport Type is c2t, don't need change it
    //juage transport Type, need check maxconnnum and nat
    //first check nat 

    bool isNatTraversal = false;
    struct MediaProxy::EndPoint endpoint;
    endpoint.address = originalSdp->session().connection().getAddress();
    // Should we adjust the address because of NAT?

    if(mNatHelper) //whatever i should check nat 
    {
	in_addr_t sdpConnectionAddr = inet_addr(originalSdp->session().connection().getAddress().c_str());
	// Is the endpoint a private address?
	bool addressIsPrivate = isAddressPrivate(sdpConnectionAddr);
	if(addressIsPrivate)
	{
	    B2BUA_LOG_WARNING( <<"IP address in SDP is private: " << originalSdp->session().connection().getAddress().c_str() );
	}
	// Does the endpoint address not match the msg source address?
	bool matchesMsgSource = false;
	if(sdpConnectionAddr == msgSourceAddress)
	{
	    matchesMsgSource = true;
	}

	if(addressIsPrivate && !matchesMsgSource) 
	{
	    // use the msg source address instead of the address in the SDP
	    struct in_addr sa;
	    sa.s_addr = msgSourceAddress;
	    endpoint.address = Data( inet_ntoa(sa) );
	    callerAsymmetric = false;
	    isNatTraversal = true;
	    //if is nat, use proxy
	    // FIXME - set username also
	    newSdp->session().origin().setAddress(MediaManager::proxyAddress);     //change sdp's c=
	    newSdp->session().connection().setAddress(MediaManager::proxyAddress); //change sdp's c=

	    B2BUA_LOG_WARNING( <<"rewriting NAT address, was "<< originalSdp->session().connection().getAddress().c_str()
			       << "using "<< endpoint.address.c_str() );
	} 
    }

    //uri= 1000000111_1@192.168.0.1 ,but term id = 1000000111@192.168.0.1 NTD!!!
    B2BUA_LOG_DEBUG( <<"B2BCall MediaProxy::updateSdp this!" );    
    entity::Terminal* term = mediaManager.getTerminal();
    if ( !term )
    {
	return 405;//error
    }

    if ( isNatTraversal )
    {
	mediaManager.nTransType_ = entity::Terminal::C2V2T;
    }

    if ( -1 == mediaManager.nTransType_ ) //new create will handle this. add not
    {
	mediaManager.nTransType_ = term->getTransType();
    }
    
    if ( !isNatTraversal && (entity::Terminal::C2V2T == mediaManager.nTransType_) ) //!!!up to max conn number need fixme
    {
	callerAsymmetric = false;

	newSdp->session().origin().setAddress(MediaManager::proxyAddress);     //change sdp's c=
	newSdp->session().connection().setAddress(MediaManager::proxyAddress); //change sdp's c=

	B2BUA_LOG_INFO( << "Succed Max ConnNum, need use vtdu!");
    }
    
    if ( isNatTraversal || (entity::Terminal::C2V2T == mediaManager.nTransType_) )
    {
	newSdp->session().clearMedium();//clear all media info in sdp
	list<SdpContents::Session::Medium>::iterator i = originalSdp->session().media().begin();
	//!!!get media filed "m=" from originsdp, put it into newSdp after changing
	while(i != originalSdp->session().media().end()) 
	{
	    if ( allowProtocol((*i).protocol()) ) 
	    {
		if(newSdp->session().media().size() > 0) 
		{
		    // FIXME
		    B2BUA_LOG_WARNING( <<"only one medium definition supported");
		    return MM_SDP_BAD;
		}
		
		// Check for a connection spec
		if( (*i).getMediumConnections().size() > 1 ) 
		{
		    // FIXME - connection for each medium
		    B2BUA_LOG_WARNING( <<"multiple medium specific connections not supported");
		    return MM_SDP_BAD;
		}
		
		if( (*i).getMediumConnections().size() == 1 ) 
		{
		    const SdpContents::Session::Connection& mc = (*i).getMediumConnections().front();//mc only used to check address match
		    // FIXME - check address type, etc, or implement operator==
		    if( !(mc.getAddress() == originalSdp->session().connection().getAddress()) ) 
		    {
			B2BUA_LOG_WARNING( <<"medium specific connection doesn't match global connection");
			return MM_SDP_BAD;
		    }
		}
		// Get the old port, insert new port
		endpoint.originalPort = (*i).port(); //here, in fact it is a old port ,need change to new port? How?
		SdpContents::Session::Medium medium(*i);
		// FIXME - only needed until more detailed handling of medium specific
		// connections is implemented:
		//m.getMediumConnections().clear();
		medium.setConnection(newSdp->session().connection());
		
//		if(mediaManager.aLegProxy == this) 
		if(mediaManager.bLegProxy == this)
		{
		    // this must be B leg. !!!access vtdu,get vtdu media recv port, the port is used recv rtp from terminal
		    endpoint.proxyPort = mediaManager.rtpProxyUtil->setupCallee(endpoint.address.c_str(), endpoint.originalPort, 
										mediaManager.toTag.c_str(), calleeAsymmetric);
		    if(endpoint.proxyPort == 0)
			throw new exception;
		}
		else 
		{
		    // this must be A leg
		    if(mediaManager.rtpProxyUtil == NULL) 
		    {
			mediaManager.rtpProxyUtil = new RtpProxyUtil();
			mediaManager.rtpProxyUtil->setTimeoutListener(&mediaManager);
		    }
		    //!!!access vtdu,get vtdu media recv port, the port is used recv rtcp from client
		    endpoint.proxyPort = mediaManager.rtpProxyUtil->setupCaller(mediaManager.callId.c_str(), endpoint.address.c_str(), 
										endpoint.originalPort, mediaManager.fromTag.c_str(), callerAsymmetric);
		    if(endpoint.proxyPort == 0)
			throw new exception;
		} 

		medium.setPort(endpoint.proxyPort);
		//newMedia.push_back(m);
		newSdp->session().addMedium(medium);
		endpoints.push_back(endpoint);
	    } //end if protocol
	    else 
	    {
		B2BUA_LOG_WARNING( <<"media protocol "<< (*i).protocol().c_str() <<" not recognised, removed from SDP" );
	    }
	    i++;
	}//end while

	if(endpoints.size() == 0) 
	{
	    B2BUA_LOG_WARNING( <<"no acceptable media protocol found, try RTP/AVP or UDP");
	    return MM_SDP_BAD;
	}

    }

    return MM_SDP_OK;

}

resip::SdpContents& MediaProxy::getSdp() 
{
    return *newSdp;
}

bool MediaProxy::allowProtocol(const resip::Data& protocol) 
{
    if(protocol == Data("RTP/AVP") || protocol == Data("UDP") || protocol == Data("udp") || protocol == Data("udptl")) 
    {
	return true;
    }
    return false;
}

// 10.0.0.0/8 - 10.255.255.255
// 172.16.0.0/12 - 172.31.255.255
// 192.168.0.0/16 - 192.168.255.255
bool MediaProxy::isAddressPrivate(const in_addr_t& subj_addr) 
{
    //in_addr_t subj_addr = inet_addr(address.c_str());
    if(subj_addr == INADDR_NONE) 
    {
	B2BUA_LOG_WARNING( <<"subject address is invalid: INADDR_NONE");
	return false;
    }
 
    uint32_t subj_addr1 = ntohl(subj_addr);
    uint32_t priv1 = (10 << 24);
    uint32_t nm1 = 0xff000000;
    uint32_t priv2 = (172 << 24) + (16 << 16);
    uint32_t nm2 = 0xfff00000;
    uint32_t priv3 = (192 << 24) + (168 << 16);
    uint32_t nm3 = 0xffff0000;

    if(((subj_addr1 & nm1) == priv1) ||
       ((subj_addr1 & nm2) == priv2) ||
       ((subj_addr1 & nm3) == priv3))
	return true;
 
    return false;
}
 
/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

