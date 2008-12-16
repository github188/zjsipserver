#include <cassert>

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"

#include "resip/dum/ClientPagerMessage.hxx" //zhangjun add

#include "repro/ReproVersion.hxx"
#include "repro/megaeyes/MegaWebServer.hxx"
#include "repro/megaeyes/MegaHttpConnection.hxx"
#include "repro/megaeyes/Common.hxx"
#include "repro/megaeyes/XmlContents.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

int MegaHttpConnection::nextPageNumber=1;


MegaHttpConnection::MegaHttpConnection( MegaWebServer& base, Socket pSock ):
   mMws( base ),
   mPageNumber(nextPageNumber++),
   mSock(pSock),
   mParsedRequest(false),
   mCtime(time(NULL))
    
{
    assert( mSock > 0 );
}


MegaHttpConnection::~MegaHttpConnection()
{
   assert( mSock > 0 );
#ifdef WIN32
   closesocket(mSock); mSock=0;
#else
   close(mSock); mSock=0;
#endif
}

      
void 
MegaHttpConnection::buildFdSet(FdSet& fdset)
{
   if ( !mTxBuffer.empty() )
   {
      fdset.setWrite(mSock);
   }
   fdset.setRead(mSock);
}

bool 
MegaHttpConnection::process(FdSet& fdset)
{
    if ( 30 < time(NULL) - mCtime )
	return false;

    if ( fdset.hasException(mSock) )
    {
	int errNum = 0;
	int errNumSize = sizeof(errNum);
	getsockopt(mSock,SOL_SOCKET,SO_ERROR,(char *)&errNum,(socklen_t *)&errNumSize);
	InfoLog (<< "Exception reading from socket " 
		 << (int)mSock << " code: " << errNum << "; closing connection");
	return false;
    }
   
    if ( fdset.readyToRead( mSock ) )
    {
	bool ok = processSomeReads();
	if ( !ok )
	{
	    return false;//表示读出错
	}
    }

    if ( (!mTxBuffer.empty()) && fdset.readyToWrite( mSock ) )
    {
	bool ok = processSomeWrites();
	if ( !ok )
	{
	    return false;//表示写完成或写出错
	}
    }

    return true;
}

bool
MegaHttpConnection::processSomeReads()
{
   const int bufSize = 8000;
   char buf[bufSize];
   
 
#if defined(WIN32)
   int bytesRead = ::recv(mSock, buf, bufSize, 0);
#else
   int bytesRead = ::read(mSock, buf, bufSize);
#endif

   if (bytesRead == INVALID_SOCKET)
   {
      int e = getErrno();
      switch (e)
      {
         case EAGAIN:
            InfoLog (<< "No data ready to read");
            return true;
         case EINTR:
            InfoLog (<< "The call was interrupted by a signal before any data was read.");
            break;
         case EIO:
            InfoLog (<< "I/O error");
            break;
         case EBADF:
            InfoLog (<< "fd is not a valid file descriptor or is not open for reading.");
            break;
         case EINVAL:
            InfoLog (<< "fd is attached to an object which is unsuitable for reading.");
            break;
         case EFAULT:
            InfoLog (<< "buf is outside your accessible address space.");
            break;
         default:
            InfoLog (<< "Some other error");
            break;
      }
      InfoLog (<< "Failed read on " << (int)mSock << " " << strerror(e));
      return false;
   }
   else if (bytesRead == 0)
   {
      InfoLog (<< "Connection closed by remote " );
      return false;
   }

   //DebugLog (<< "MegaHttpConnection::processSomeReads() " 
   //          << " read=" << bytesRead);            

   mRxBuffer += Data( buf, bytesRead );
   
   tryParse();
   
   return true;
}


void 
MegaHttpConnection::tryParse()
{
    //DebugLog (<< "parse " << mRxBuffer );
    ParseBuffer pb(mRxBuffer);

    //const char* start = pb.skipNoWhitespace(); //position http start
   
    //pb.skipToChar(Symbols::SPACE[0]);
    //const char* start = pb.skipWhitespace();
    //pb.skipToChar(Symbols::SPACE[0]);
   
    const char* start = pb.start();
    pb.skipToChars(Symbols::CRLFCRLF);
    pb.skipWhitespace();
    if (pb.eof())
    {
	// parse failed - just return 
	return;
    }
   
    int headlen = pb.position() - pb.start();

    pb.reset(start);
    Data contlen;
    
    pb.skipToChars( "Content-Length:" );
    pb.skipToChar(Symbols::SPACE[0]);
    const char* begin = pb.skipWhitespace();
    pb.skipToChars(Symbols::CRLF);
    pb.data( contlen, begin );

    int bodylen = contlen.convertInt(); 
    InfoLog (<< "Http Content-length is " << contlen <<" ." );

    if ( (mRxBuffer.size() - headlen) < bodylen )
    {
	return;
    }

    pb.skipToChars(Symbols::CRLFCRLF);
    const char* body = pb.skipWhitespace();
    pb.skipToEnd();

    pb.data( mContent, body );

    InfoLog (<< "Http Content is " << mContent );
 
    mParsedRequest = true;

    //解析消息体
    CMarkupSTL ParaXML;
    if ( mContent.size() > 0 )
    {
        ParaXML.SetDoc( mContent.data() );
        if ( ParaXML.FindElem() )
        {
	    ParaXML.IntoElem();
	    while ( ParaXML.FindElem() )
	    {
		std::string sTag = ParaXML.GetTagName();

		if ( sTag == "IE_HEADER"  )
		{
		    mMsgType  = ParaXML.GetAttrib( "MessageType" );
		    mSN = ParaXML.GetAttrib( "SequenceNumber" );
		    mSrcId = ParaXML.GetAttrib( "SourceID" );
		    mDstId = ParaXML.GetAttrib( "DestinationID" );
		    InfoLog ( << "Recv Message body: " << " MessageType: " <<  mMsgType << " SequenceNumber " << mSN 
			      << " SourceID " << mSrcId << " DestinationID " << mDstId  );
		}
	    }
        }
	else
	{
	    InfoLog ( << "Recv Message XML: " << mContent );
	}
    }

    /* 接下来的工作
    1 查询通讯设备目的id对应的sip uri
    2 调用dum接口,把请求和本connection标志传给dum,通过dum发送Message请求
    
    响应处理
    3 dum收到响应后,通过ClientPageMessageHandler把响应传给MegaWebServerThread,最终交到本connection
    4 调用setPage返回响应
    */
//    std::string resp;
//    setResp ( resp, resip::Mime("text","xml") );

    sipReq( );
}

void 
MegaHttpConnection::sipReq( )
{
    //这里假设dstid已经是一个合法的sip uri,实际还不是!!!
    std::string uri = "<sip:" + mDstId + "@192.168.5.232" + ">";//低效的做法 稍后重构!!!
    resip::NameAddr to( uri.c_str() );
    
    InfoLog ( << "Target is " << to << " pagenumber is "<< mPageNumber );

#if 1
    ClientPagerMessageHandle cpmh = mMws.mDum->makePagerMessage(to);

    common::msgmap[cpmh.getId()] = mPageNumber;

    std::auto_ptr<Contents> req( new XmlContents( mContent ) );
    cpmh->page(req);
#else
    std::string resp;
    setResp ( resp, resip::Mime("text","xml") );
#endif
}

void 
MegaHttpConnection::setResp( std::string &resp, const Mime& pType)
{
    //request
    if ( mContent.size() > 0 && resp.empty() )
    {
	common::MakeXmlReponse( mContent.data(), resp );
    }

    mTxBuffer += "HTTP/1.0 200 OK" ; mTxBuffer += Symbols::CRLF;
    
    Data len;
    {
	DataStream s(len);
	s << resp.size();
	s.flush();
    }
    
    mTxBuffer += "Server: Repro Proxy "; 
    mTxBuffer += Data(VersionUtils::instance().displayVersion());
    mTxBuffer += Symbols::CRLF;
    mTxBuffer += "Mime-version: 1.0 " ; mTxBuffer += Symbols::CRLF;
    mTxBuffer += "Pragma: no-cache " ; mTxBuffer += Symbols::CRLF;
    mTxBuffer += "Content-Length: "; mTxBuffer += len; mTxBuffer += Symbols::CRLF;

    mTxBuffer += "Content-Type: "  ;
    mTxBuffer += pType.type() ;
    mTxBuffer += "/"  ;
    mTxBuffer += pType.subType() ; mTxBuffer += Symbols::CRLF;
   
    mTxBuffer += Symbols::CRLF;
   
    mTxBuffer += resp.c_str();
}

void
MegaHttpConnection::parsehead()
{
    ParseBuffer pb(mRxBuffer);

    while( !pb.eof() )
    {//zhangjun test
	const char* begin = pb.skipWhitespace();
	pb.skipToChar(Symbols::COLON[0]);
	Data key;
	pb.data( key, begin );
       
	pb.skipToChar(Symbols::SPACE[0]);
	begin = pb.skipWhitespace();
	pb.skipToChars(Symbols::CRLF);
       
	Data value;
	pb.data( value, begin );

	DebugLog (<< "parse " << key <<"?" <<value );
    }
}

bool
MegaHttpConnection::processSomeWrites()
{
   if ( mTxBuffer.empty() )
   {
      return true;
   }
   
   //DebugLog (<< "Writing " << mTxBuffer );

#if defined(WIN32)
   int bytesWritten = ::send( mSock, mTxBuffer.data(), mTxBuffer.size(), 0);
#else
   int bytesWritten = ::write(mSock, mTxBuffer.data(), mTxBuffer.size() );
#endif

   if (bytesWritten == INVALID_SOCKET)
   {
      int e = getErrno();
      InfoLog (<< "MegaHttpConnection failed write on " << mSock << " " << strerror(e));

      return false;
   }
   
   if (bytesWritten == (int)mTxBuffer.size() )
   {
      DebugLog (<< "Wrote it all" );
      mTxBuffer = Data::Empty;

      return false; // return false causes connection to close and clean up
   }
   else
   {
      Data rest = mTxBuffer.substr(bytesWritten);
      mTxBuffer = rest;
      DebugLog( << "Wrote " << bytesWritten << " bytes - still need to do " << mTxBuffer );
   }
   
   return true;
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
