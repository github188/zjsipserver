

#include <syslog.h>

#include "B2BCallManager.hxx"
#include "Logging.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

B2BCallManager::B2BCallManager(resip::DialogUsageManager& dum, 
			       AuthorizationManager *authorizationManager, 
			       CDRHandler& cdrHandler) 
    : dum(dum), 
      authorizationManager(authorizationManager), 
      cdrHandler(cdrHandler) 
{
    stopping = false;
    mustStopCalls = false;
    time(&statisticsStartTime);
    statisticsIntervalSecs = 60;
}

B2BCallManager::~B2BCallManager() 
{
}

void 
B2BCallManager::setAuthorizationManager(AuthorizationManager *authorizationManager) 
{
    this->authorizationManager = authorizationManager;
}

TaskManager::TaskResult 
B2BCallManager::doTaskProcessing() 
{
//    B2BUA_LOG_NOTICE( <<"B2BCallManager is running!");
    if(mustStopCalls) 
    {
	B2BUA_LOG_NOTICE( <<"notifying calls to stop");

	B2BCallMap::iterator i = callmaps.begin();
	for ( ; i!=callmaps.end(); ++i ) //say stop to every b2bcall
	{
	    B2BCallList::iterator j = (i->second).begin();
	    while( j != (i->second).end() )
	    {
		(*j)->onStopping();
		j++;
	    }
	}

	mustStopCalls = false;
    }

    time_t now;
    time(&now);
    int B2BCallNumber=0;

    B2BCallMap::iterator i = callmaps.begin();
    for ( ; i!=callmaps.end(); ++i ) //say stop to every b2bcall
    {
	B2BCallList::iterator j = (i->second).begin();//j is std::list's iterator
	while(  j!= (i->second).end() )
	{
	    ++B2BCallNumber;
	    (*j)->checkProgress(now, stopping);
	    if( (*j)->isComplete() )
	    {
		B2BCall *call = *j;
		j++;
		(i->second).remove(call); //from b2blist delete it
		delete call;
	    }
	    else
		j++;
	}
    }

    if ( now - statisticsStartTime > statisticsIntervalSecs )
    {
	B2BUA_LOG_NOTICE( <<"B2BCall have "<<B2BCallNumber <<" !" );
	statisticsStartTime = now;
    }

//    if(stopping && calls.begin() == calls.end()) 
    if(stopping && this->empty() )
    {
	B2BUA_LOG_NOTICE( <<"no (more) calls in progress");
	return TaskManager::TaskComplete;
    }

    return TaskManager::TaskNotComplete;
}

bool 
B2BCallManager::empty()
{
    B2BCallMap::iterator i = callmaps.begin();
    for ( ; i!=callmaps.end(); ++i ) 
    {
	if ( !(i->second).empty() )
	{
	    return false;
	}
    }

    return true;
}

void 
B2BCallManager::stop() 
{
    B2BUA_LOG_NOTICE( <<"B2BCallManager is stopped!");
    stopping = true;
    mustStopCalls = true;

    B2BCallMap::iterator i = callmaps.begin();
    for ( ; i!=callmaps.end(); ++i )
    {
	B2BCallList::iterator j = (i->second).begin();//j is std::list's iterator
        while(  j!= (i->second).end() )
	{
	    B2BUA_LOG_DEBUG( <<"B2BCall "<< (*j)->aFirstLegAppDialogCallId << " State is " << (*j)->B2BCallStateName[(*j)->callState] );
	    j++;
	}
    }
}

bool 
B2BCallManager::isStopping() 
{
    return stopping;
}


B2BCallList*
B2BCallManager::getB2BCall( const resip::Uri &destUri )
{
    if ( callmaps.find( destUri )!= callmaps.end() )
	return &(callmaps[destUri]);
    else 
    {
	B2BCallList bclist;
	callmaps[destUri] = bclist;
	//throw new exception;
	return &(callmaps[destUri]);
    }
}

void 
B2BCallManager::onNewCall(MyAppDialog *aLegDialog, const resip::NameAddr& sourceAddr, 
			       const resip::Uri& destinationAddr, const resip::Data& authRealm, 
			       const resip::Data& authUser, const resip::Data& authPassword, 
			       const resip::Data& srcIp, const resip::Data& contextId, 
			       const resip::Data& accountId, const resip::Data& baseIp, 
			       const resip::Data& controlId) 
{

    B2BCall *call = new B2BCall(cdrHandler, dum, *authorizationManager, 
				aLegDialog, sourceAddr, destinationAddr, 
				authRealm, authUser, authPassword, srcIp, 
				contextId, accountId, baseIp, controlId);

//  calls.push_back(call);
    callmaps[destinationAddr].push_back(call);
}

void 
B2BCallManager::logStats() 
{
    int preDial = 0, dialing = 0, connected = 0, finishing = 0, unknown = 0;

    B2BCallMap::iterator i = callmaps.begin();
    for ( ; i!=callmaps.end(); ++i ) //say stop to every b2bcall
    {
	B2BCallList::iterator j = (i->second).begin();//j is std::list's iterator
	while(  j!= (i->second).end() )
	{
	    switch((*j)->getStatus())
	    {
	    case B2BCall::PreDial:
		preDial++;
		break;
	    case B2BCall::Dialing:
		dialing++;
		break;
	    case B2BCall::Connected:
		connected++;
		break;
	    case B2BCall::Finishing:
		finishing++;
		break;
	    default:
		unknown++;
		break;
	    }
	    j++;
	}
    }

/*
    list<B2BCall *>::iterator call = calls.begin();
    while(call != calls.end()) 
    {
	switch((*call)->getStatus()) 
	{
	case B2BCall::PreDial:
	    preDial++;
	    break;
	case B2BCall::Dialing:
	    dialing++;
	    break;
	case B2BCall::Connected:
	    connected++;
	    break;
	case B2BCall::Finishing:
	    finishing++;
	    break;
	default:
	    unknown++;
	    break;
	}
	call++;
    }
*/

    B2BUA_LOG_NOTICE( <<"call info: preDial = "<< preDial<<" dialing = "<<dialing <<" connected = "<< connected<<" finishing = "
		      << finishing<<" unknown = "<< unknown<<" total = " << (preDial + dialing + connected + finishing + unknown) );
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

