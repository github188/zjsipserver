#include <fstream>
#include "rutil/Logger.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Subsystem.hxx"
#include "rutil/SysLogStream.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

std::ostream* GenericLogImpl::mLogger=0;
unsigned int GenericLogImpl::mLineCount=0;
unsigned int GenericLogImpl::MaxLineCount = 0; // no limit by default

std::ostream&
GenericLogImpl::Instance()
{
   switch (Log::_type)
   {
      case Log::Syslog:
         if (mLogger == 0)
         {
            std::cerr << "Creating a syslog stream" << std::endl;
            mLogger = new SysLogStream;
         }
         return *mLogger;

      case Log::Cerr:
         return std::cerr;

      case Log::Cout:
         return std::cout;

      case Log::File:
         if (mLogger == 0 || (MaxLineCount && mLineCount > MaxLineCount))
         {
            std::cerr << "Creating a file logger" << std::endl;
            if (mLogger)
            {
               delete mLogger;
            }
            if (Log::mLogFileName != "")
            {
               mLogger = new std::ofstream(mLogFileName.c_str(), std::ios_base::out | std::ios_base::trunc);
               mLineCount = 0;
            }
            else
            {
               mLogger = new std::ofstream("resiprocate.log", std::ios_base::out | std::ios_base::trunc);
               mLineCount = 0;
            }
         }
         mLineCount++;
         return *mLogger;
      default:
         assert(0);
         return std::cout;
   }
}

void 
GenericLogImpl::reset()
{
   delete mLogger;
   mLogger = 0;
}

bool
GenericLogImpl::isLogging(Log::Level level, const resip::Subsystem& sub)
{
   if (sub.getLevel() != Log::None)
   {
      return level <= sub.getLevel();
   }
   else
   {
      return (level <= Log::mLevel);
   }
}

void
GenericLogImpl::OutputToWin32DebugWindow(const Data& result)
{
#ifdef WIN32
   const char *text = result.c_str();
#ifdef UNDER_CE
   LPWSTR lpwstrText = resip::ToWString(text);
	OutputDebugStringW(lpwstrText);
	FreeWString(lpwstrText);
#else
	OutputDebugStringA(text);
#endif
#endif
}

bool
genericLogCheckLevel(resip::Log::Level level, const resip::Subsystem& sub)
{
   //?dcm? should we just remove threadsetting? Does it still work?
   const resip::Log::ThreadSetting* setting = resip::Log::getThreadSetting();
   return ((setting && resip::GenericLogImpl::isLogging(setting->level, sub) ||
            (!setting && resip::GenericLogImpl::isLogging(level, sub))));
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000-2005
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
