#if !defined(REPRO_MEGAHTTPCONNECTION_HXX)
#define REPRO_MEGAHTTPCONNECTION_HXX 

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/TransportType.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Mime.hxx"

#include "repro/UserStore.hxx"
#include "repro/megaeyes/MegaWebServer.hxx"
#include "repro/megaeyes/MarkupSTL.h"

#include <time.h>

namespace repro
{

    class MegaHttpConnection
    {
	friend class MegaWebServer;
      
    public:
	MegaHttpConnection( MegaWebServer& mws, resip::Socket pSock );
	~MegaHttpConnection();
      
	void buildFdSet( resip::FdSet& fdset );
	bool process( resip::FdSet& fdset );

	void sipReq( );
	void setResp( std::string &resp, const resip::Mime& pType );

    private:
	bool processSomeReads();
	bool processSomeWrites();
	void tryParse();
	void parsehead();
            
	MegaWebServer& mMws;
	const int mPageNumber;
	static int nextPageNumber;
            
	resip::Socket mSock;
	resip::Data mRxBuffer;
	resip::Data mTxBuffer;
	bool mParsedRequest;
	resip::Data mContent;

	std::string mMsgType;
	std::string mSN;
	std::string mSrcId;
	std::string mDstId;

	time_t mCtime;
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
