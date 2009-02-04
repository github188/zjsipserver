
#include "rutil/Data.hxx"
#include "resip/stack/Helper.hxx"

#include "MediaManager.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

Data MediaManager::proxyAddress;

void MediaManager::setProxyAddress(const Data& proxyAddress) 
{
    MediaManager::proxyAddress = proxyAddress;
};

MediaManager::MediaManager(B2BCall& b2BCall) 
    : b2BCall(b2BCall) 
{
    //aLegProxy = NULL;
    bLegProxy = NULL;
    rtpProxyUtil = NULL;
    nTransType_ = 0;
};

MediaManager::MediaManager(B2BCall& b2BCall, 
			   const Data& callId, 
			   const Data& fromTag, 
			   const Data& toTag) 
    : b2BCall(b2BCall), 
      callId(callId), 
      fromTag(fromTag), 
      toTag(toTag) 
{
//    aLegProxy = NULL;
    bLegProxy = NULL;
    rtpProxyUtil = NULL;
    nTransType_ = 0;
};

MediaManager::MediaManager(B2BCall& b2BCall,
                           const Data& fromTag,
                           const Data& toTag)
    : b2BCall(b2BCall),
      fromTag(fromTag),
      toTag(toTag)
{
    callId = resip::Helper::computeCallId();//get a unique callid
    //aLegProxy = NULL;
    bLegProxy = NULL;
    rtpProxyUtil = NULL;
    nTransType_ = -1;
};


MediaManager::~MediaManager() 
{
//    if(aLegProxy != NULL)
//    {
//	delete aLegProxy;
//    }

    std::map<resip::Data, MediaProxy *>::iterator i = aLegProxys.begin();
    for(; i!=aLegProxys.end(); ++i)
    {
	MediaProxy *alegmp = i->second;
	delete alegmp;
    }

    if(bLegProxy != NULL)
    {
	delete bLegProxy;
    }

    if(rtpProxyUtil != NULL)
    {
	delete rtpProxyUtil;
    }
};

void 
MediaManager::setFromTag(const Data& fromTag) 
{
    this->fromTag = fromTag;
};

void 
MediaManager::setToTag(const Data& toTag) 
{
    this->toTag = toTag;
};

entity::Terminal *
MediaManager::getTerminal( )
{
    resip::Data termid;

    resip::Data userName = b2BCall.destinationAddr.user();
    if ( userName.size() < DEVIDLEN )
    {
	termid = userName;
    }
    else
    {
	termid = userName.substr(0,DEVIDLEN);
    }

    return entity::getTerminal( termid );
}

void 
MediaManager::updateTermStatus()
{
    entity::Terminal* term = getTerminal();
    if ( term )
    {
	switch( b2BCall.getStatus() ) 
	{
	case B2BCall::Connected:
	    term->mCurConnNum_++;
	    break;
	case B2BCall::Finishing:
	    term->mCurConnNum_--;
	    break;
	default:
	    break;
	}
    }
}

int 
MediaManager::erase( const resip::Data& callid )
{
    if ( aLegProxys.find(callid) != aLegProxys.end() )
    {
	MediaProxy *alegmp = NULL;
	alegmp = aLegProxys[callid];
	if ( alegmp )
	{
	    delete alegmp;
	    return 0;
	}
    }

    return -1;
}

int 
MediaManager::setALegSdp(const resip::Data& callid, const SdpContents& sdp, const in_addr_t& msgSourceAddress) 
{
    MediaProxy *alegmp = NULL;

    if ( aLegProxys.find(callid) == aLegProxys.end() )
    {
	alegmp = new MediaProxy(*this,callid);
	aLegProxys[callid] = alegmp;
    }
    else
    {
	alegmp = aLegProxys[callid];
    }

    return alegmp->updateSdp(sdp, msgSourceAddress);
};

SdpContents& 
MediaManager::getALegSdp( const resip::Data& callid ) 
{
//    if(aLegProxy == NULL) 
//    {
//	throw new exception;
//    }
    if ( aLegProxys.find(callid) == aLegProxys.end() )
    {
	throw new exception;
    }

    return (aLegProxys[callid])->getSdp();
};

int 
MediaManager::setBLegSdp(const resip::Data& callid, const SdpContents& sdp, const in_addr_t& msgSourceAddress) 
{
    if(bLegProxy == NULL)
    {
	bLegProxy = new MediaProxy(*this,callid);
    }
    return bLegProxy->updateSdp(sdp, msgSourceAddress);
};

SdpContents& 
MediaManager::getBLegSdp() 
{
    if(bLegProxy == NULL)
	throw new exception;

    return bLegProxy->getSdp();
};

void MediaManager::onMediaTimeout() 
{
    b2BCall.onMediaTimeout();
};

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

