
#ifndef __CallHandle_h
#define __CallHandle_h

#include <iostream>
#include <list>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/SharedPtr.hxx"

#include "resip/dum/megaeyes/MegaSipEntity.hxx"

namespace b2bua
{

#define CC_PERMITTED 0				// Call is permitted	
#define CC_AUTH_REQUIRED 1			// Auth is required
#define CC_PAYMENT_REQUIRED 2			// Payment is required
#define CC_IP_DENIED 3				// IP is blocked/unrecognised
#define CC_ERROR 4				// Server error 
#define CC_INVALID 5				// Request is invalid
#define CC_PENDING 6				// Waiting for server
#define CC_DESTINATION_INCOMPLETE 7		// Destination number too short
#define CC_DESTINATION_INVALID 8		// Destination invalid
#define CC_TIMEOUT 9				// Timeout waiting for server
#define CC_USER_UNKNOWN 10			// User unknown
#define CC_REALM_UNKNOWN 11			// Realm unknown
#define CC_REQUEST_DENIED 12			// Request denied for 
						// administrative reason

class CallRoute 
{
public:
  virtual ~CallRoute();
  virtual const resip::Data& getAppRef1() = 0;
  virtual const resip::Data& getAppRef2() = 0;
  virtual const resip::Data& getAuthRealm() = 0;
  virtual const resip::Data& getAuthUser() = 0;
  virtual const resip::Data& getAuthPass() = 0;
  virtual const resip::NameAddr& getSourceAddr() = 0;
  virtual const resip::NameAddr& getDestinationAddr() = 0;
  virtual const resip::Uri& getOutboundProxy() = 0;
  virtual const bool isSrvQuery() = 0;
};

class MegaCallRoute : public CallRoute
{
public:
    MegaCallRoute( const resip::NameAddr& sourceAddr, const resip::Uri& destinationAddr, const resip::Data& authRealm, 
		   const resip::Data& authUser, const resip::Data& authPass, const resip::Data& srcIp, 
		   const resip::Data& contextId, const resip::Data& accountId, const resip::Data& baseIp, 
		   const resip::Data& controlId, time_t startTime)
	: sourceAddr(sourceAddr), 
	  destinationAddr(destinationAddr),
	  authRealm(authRealm), 
	  authUser(authUser), 
	  authPassword(authPassword), 
	  srcIp(srcIp), 
	  contextId(contextId), 
	  accountId(accountId), 
	  baseIp(baseIp), 
	  controlId(controlId),
	  startTime_(startTime)
	{}

    virtual ~MegaCallRoute()
	{}
    virtual const resip::Data& getAppRef1() 
	{
	    return appRef1;
	}
    virtual const resip::Data& getAppRef2()
	{
	    return appRef2;
	}
    virtual const resip::Data& getAuthRealm()
	{
	    return authRealm;
	}
    virtual const resip::Data& getAuthUser()
	{
	    return authUser;
	}
    virtual const resip::Data& getAuthPass()
	{
	    return authPassword;
	}
    virtual const resip::NameAddr& getSourceAddr()
	{
	    return sourceAddr;
	}
    virtual const resip::NameAddr& getDestinationAddr()
	{
	    return destinationAddr;
	}
    virtual const resip::Uri& getOutboundProxy()
	{
	    return proxy;
	}
    virtual const bool isSrvQuery()
	{
	    return false;
	}

private:
    resip::NameAddr sourceAddr;
    resip::NameAddr destinationAddr;
    resip::Data authRealm;
    resip::Data authUser;
    resip::Data authPassword;
    resip::Data srcIp;
    resip::Data contextId;
    resip::Data accountId;
    resip::Data baseIp;
    resip::Data controlId;

    resip::Data appRef1;
    resip::Data appRef2;

    resip::Uri proxy;

    time_t startTime_;
};

class CallHandle 
{
public:
    virtual ~CallHandle();
    virtual int getAuthResult() = 0; 
    virtual const resip::Data& getRealm() = 0;	// Realm for client auth,
						// if we require further auth
    virtual time_t getHangupTime() = 0;		// The time to hangup,
						// 0 for no limit
    virtual bool mustHangup() = 0;
    virtual void connect(time_t *connectTime) = 0; // The call connected
    virtual void fail(time_t *finishTime) = 0;	// The attempt failed, or
						// the caller gave up
    virtual void finish(time_t *finishTime) = 0;	// The call hungup
    virtual std::list<CallRoute *>& getRoutes() = 0;
};

class MegaCallHandle : public CallHandle
{
public:
    MegaCallHandle( const resip::NameAddr& sourceAddr, const resip::Uri& destinationAddr, const resip::Data& authRealm, 
		    const resip::Data& authUser, const resip::Data& authPass, const resip::Data& srcIp, 
		    const resip::Data& contextId, const resip::Data& accountId, const resip::Data& baseIp, 
		    const resip::Data& controlId, time_t startTime )
	: realm_(authRealm)
	{
	    try
	    {
		resip::Uri newuri = updateDestAddr(destinationAddr);
		MegaCallRoute *mcr = new MegaCallRoute(sourceAddr, newuri, authRealm, authUser, 
						       authPass, srcIp, contextId, accountId, baseIp, 
						       controlId, startTime);
		routelist_.push_front(mcr);
	    }
	    catch(...)
	    {
		//if terminal not be here, no push route
	    }
	}

    virtual ~MegaCallHandle() 
	{
	    while( !routelist_.empty() )
	    {
		CallRoute *item = routelist_.front();
		delete item;
		routelist_.pop_front();
	    }
	}

    virtual int getAuthResult() 
	{
	    return 0;
	} 

    virtual const resip::Data& getRealm()  // Realm for client auth, if we require further auth
	{
	    return realm_;
	}
    virtual time_t getHangupTime()	   // The time to hangup, 0 for no limit
	{
	    return 0;
	}
    virtual bool mustHangup()
	{
	    return false; //no hangUp
	}
    virtual void connect(time_t *connectTime)  // The call connected
	{
	    if ( connectTime )
		connectTime_ = *connectTime;
	}
    virtual void fail(time_t *finishTime)	// The attempt failed, or the caller gave up
	{ }
    virtual void finish(time_t *finishTime)	// The call hungup
	{
	    if ( finishTime )
		finishTime_ = *finishTime;
	}
    virtual std::list<CallRoute *>& getRoutes()
	{
	    return routelist_;
	}

private:
     resip::Uri updateDestAddr(const resip::Uri& destinationAddr)
	{
	    resip::Uri newUri = destinationAddr;
	    newUri.user() = newUri.user().substr(0,DEVIDLEN); 

	    entity::Terminal *term = entity::getTerminal( newUri.user() );

	    if ( term )
	    {
		newUri.host() = term->host_;
		newUri.port() = term->port_;
		return newUri;
	    }
	    else
	    {
		throw new std::exception;
	    }
	}

private:
    time_t connectTime_;
    time_t finishTime_;
    resip::Data realm_;
    std::list<CallRoute *> routelist_;
};

}

#endif

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

