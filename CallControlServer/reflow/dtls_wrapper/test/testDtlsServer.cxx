#include <ctype.h>
#include <iostream>

#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <openssl/ssl.h>
#include <openssl/srtp.h>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "TestTimerContext.hxx"
#include "TestDtlsUdp.hxx"
#include "CreateCert.hxx"

using namespace std;
using namespace dtls;
using namespace resip;

int main(int argc,char **argv)
{
   X509 *serverCert;
   EVP_PKEY *serverKey;

   resip::initNetwork();
   srtp_init();

   assert(argc==2);

   createCert(resip::Data("sip:server@example.com"),365,1024,serverCert,serverKey);

   TestTimerContext *ourTimer=new TestTimerContext();
   DtlsFactory *serverFactory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(ourTimer),serverCert,serverKey);

   cout << "Created the factory\n";

   Socket fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == -1 )
   {
      assert(0);
   }

   // Make the UDP socket context
   int port=atoi(argv[1]);
   struct sockaddr_in myaddr;
   memset((char*) &(myaddr),0, sizeof((myaddr)));
   myaddr.sin_family = AF_INET;
   myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   myaddr.sin_port = htons(port);
   int r=bind( fd,(struct sockaddr*)&myaddr, sizeof(myaddr));
   assert(r==0);

   cout << "Made our UDP socket\n";

   cout << "Entering wait loop\n";
   TestDtlsUdpSocketContext *sockContext=0;
   DtlsSocket *dtlsSocket;

   while(1)
   {
      FdSet fdset;
      unsigned char buffer[4096];
      struct sockaddr_in src;
      socklen_t srclen;
      int r;

      fdset.setRead(fd);

      UInt64 towait=ourTimer->getRemainingTime();
      // cerr << "Invoking select for time " << towait << endl;
      int toread=fdset.selectMilliSeconds(towait);
      ourTimer->updateTimer();

      if (toread >= 0) 
      {
         if (fdset.readyToRead(fd))
         {
            srclen=sizeof(src);
            r=recvfrom(fd, (char*)buffer, sizeof(buffer), 0, (sockaddr *)&src,&srclen);
            assert(r>=0);

            // The first packet initiates the association
            if(!sockContext)
            {
               sockContext=new TestDtlsUdpSocketContext(fd,&src);

               cout << "Made the socket context\n";          

               dtlsSocket=serverFactory->createServer(std::auto_ptr<DtlsSocketContext>(sockContext));

               cout << "Made the DTLS socket\n";
            }

            switch(DtlsFactory::demuxPacket(buffer,r))
            {
            case DtlsFactory::dtls:
               dtlsSocket->handlePacketMaybe(buffer,r);
               break;
            case DtlsFactory::rtp:
               unsigned char buf2[4096];
               unsigned int buf2l;

               sockContext->recvRtpData(buffer,r,buf2,&buf2l,sizeof(buf2));

               cout << "Read RTP data of length " << buf2l << endl;
               cout << buf2 << endl;

               for(unsigned int i=0;i<buf2l;i++)
               {
                  buf2[i]=toupper(buf2[i]);
               }

               // Now echo it back
               sockContext->sendRtpData((const unsigned char *)buf2,buf2l);

               break;
            default:
               break;
            }
         }
      }
   }

   exit(0);
}


/* ====================================================================

 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
