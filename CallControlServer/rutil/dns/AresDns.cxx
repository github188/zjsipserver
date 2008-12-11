#if !defined(WIN32)
#include <sys/types.h>
#endif

#include "rutil/dns/AresDns.hxx"
#include "rutil/GenericIPAddress.hxx"

#include "ares.h"
#include "ares_dns.h"
#include "ares_private.h"

#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/WinLeakCheck.hxx"

#if !defined(USE_ARES)
#error Must have ARES
#endif

#if !defined(WIN32)
#if !defined(__CYGWIN__)
#include <arpa/nameser.h>
#endif
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

int 
AresDns::init(const std::vector<GenericIPAddress>& additionalNameservers,
              AfterSocketCreationFuncPtr socketfunc,
              int timeout,
              int tries,
              unsigned int features)
{
	mAdditionalNameservers = additionalNameservers;
	mFeatures = features;

	int ret = internalInit(additionalNameservers,
              socketfunc,
              features,
				  &mChannel);

	if(ret != Success)
		return ret;

   if (timeout > 0)
   {
      mChannel->timeout = timeout;
   }

   if (tries > 0)
   {
      mChannel->tries = tries;
   }

   return Success;      
}

int 
AresDns::internalInit(const std::vector<GenericIPAddress>& additionalNameservers,
              AfterSocketCreationFuncPtr socketfunc,
              unsigned int features,
				  ares_channeldata** channel
)
{
#ifdef USE_IPV6
   int requiredCap = ARES_CAP_IPV6;
#else
   int requiredCap = 0;
#endif

	if(*channel)
	{
		ares_destroy_suppress_callbacks(*channel);
		*channel = 0;
	}


   int cap = ares_capabilities(requiredCap);
   if (cap != requiredCap)
   {
      ErrLog (<< "Build mismatch (ipv4/ipv6) problem in ares library"); // !dcm!
      return BuildMismatch;      
   }
   
   int status;
   ares_options opt;
   int optmask = 0;

   memset(&opt, '\0', sizeof(opt));
   if ((features & ExternalDns::TryServersOfNextNetworkUponRcode3))
   {
      optmask |= ARES_OPT_FLAGS;
      opt.flags |= ARES_FLAG_TRY_NEXT_SERVER_ON_RCODE3;
   }

   if (additionalNameservers.empty())
   {
      status = ares_init_options_with_socket_function(channel, &opt, optmask, socketfunc);
   }
   else
   { 
      optmask |= ARES_OPT_SERVERS;
      opt.nservers = additionalNameservers.size();
      
#ifdef USE_IPV6
      opt.servers = new multiFamilyAddr[additionalNameservers.size()];
      for (size_t i =0; i < additionalNameservers.size(); i++)
      {
         if (additionalNameservers[i].isVersion4())
         {
            opt.servers[i].family = AF_INET;            
            opt.servers[i].addr = additionalNameservers[i].v4Address.sin_addr;
         }
         else
         {
            opt.servers[i].family = AF_INET6;            
            opt.servers[i].addr6 = additionalNameservers[i].v6Address.sin6_addr;
         }                  
      }
#else
      opt.servers = new in_addr[additionalNameservers.size()];
      for (size_t i =0; i < additionalNameservers.size(); i++)
      {
         opt.servers[i] = additionalNameservers[i].v4Address.sin_addr;
      }
#endif
      status = ares_init_options_with_socket_function(channel, &opt, optmask, socketfunc);
      delete [] opt.servers;
      opt.servers = 0;
   }
	if (status != ARES_SUCCESS)
   {
      ErrLog (<< "Failed to initialize DNS library (status=" << status << ")");
      return status;
   }
   else
   {
      InfoLog(<< "DNS initialization: found  " << (*channel)->nservers << " name servers");
      for (int i = 0; i < (*channel)->nservers; ++i)
      {
         InfoLog(<< " name server: " << DnsUtil::inet_ntop((*channel)->servers[i].addr));
      }
      return Success;      
   }
}

bool AresDns::checkDnsChange()
{
	//return 'true' if there are changes in the list of DNS servers
	struct ares_channeldata* channel = 0;
	bool bRet = false;
	int result = internalInit(mAdditionalNameservers, 0, mFeatures, &channel);
	if(result != Success || channel == 0)
	{
		bRet = true;
	}
	else if(channel->nservers != mChannel->nservers)
	{
		bRet = true;
	}
	else
	{
		for (int i = 0; i < mChannel->nservers; i++)
		{
			if (channel->servers[i].addr.s_addr != mChannel->servers[i].addr.s_addr)
			{
				bRet = true;
				break;
			}
		}
	}
	ares_destroy_suppress_callbacks(channel);

	if(!bRet)
	{
		InfoLog(<< " No changes in DNS server list");
	}
	else
	{
		InfoLog(<< " DNS server list changed");
	}

	return bRet;
}

AresDns::~AresDns()
{
   ares_destroy_suppress_callbacks(mChannel);
}

bool AresDns::hostFileLookup(const char* target, in_addr &addr)
{
   assert(target);

   hostent *hostdata = 0;
   int status = hostfile_lookup(target, &hostdata);

   if (status != ARES_SUCCESS)
   {
      return false;
   }
   sockaddr_in saddr;
   memset(&saddr,0,sizeof(saddr));  /* Initialize sockaddr fields. */
   saddr.sin_family = AF_INET;
   memcpy((char *)&(saddr.sin_addr.s_addr),(char *)hostdata->h_addr_list[0], (size_t)hostdata->h_length);
   addr = saddr.sin_addr;
   
   return true;
}

ExternalDnsHandler* 
AresDns::getHandler(void* arg)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   ExternalDnsHandler *thisp = reinterpret_cast<ExternalDnsHandler*>(p->first);
   return thisp;
}

ExternalDnsRawResult 
AresDns::makeRawResult(void *arg, int status, unsigned char *abuf, int alen)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   void* userArg = reinterpret_cast<void*>(p->second);
   
   if (status != ARES_SUCCESS)
   {
      return ExternalDnsRawResult(status, abuf, alen, userArg);
   }
   else
   {
      return ExternalDnsRawResult(abuf, alen, userArg);
   }
}
      
bool 
AresDns::requiresProcess()
{
   return true; 
}

void 
AresDns::buildFdSet(fd_set& read, fd_set& write, int& size)
{
   int newsize = ares_fds(mChannel, &read, &write);
   if ( newsize > size )
   {
      size = newsize;
   }
}

void 
AresDns::process(fd_set& read, fd_set& write)
{
   ares_process(mChannel, &read, &write);
}

char* 
AresDns::errorMessage(long errorCode)
{
   const char* aresMsg = ares_strerror(errorCode);

   int len = strlen(aresMsg);
   char* errorString = new char[len+1];

   strncpy(errorString, aresMsg, len);
   errorString[len] = '\0';
   return errorString;
}

void
AresDns::lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData)
{
   ares_query(mChannel, target, C_IN, type, AresDns::aresCallback, new Payload(handler, userData));
}

void
AresDns::aresCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   getHandler(arg)->handleDnsRaw(makeRawResult(arg, status, abuf, alen));
   Payload* p = reinterpret_cast<Payload*>(arg);
   delete p;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
