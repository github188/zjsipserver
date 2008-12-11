#include "TlsServer.hxx"
#include <boost/bind.hpp>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

namespace reTurn {

TlsServer::TlsServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port)
: mIOService(ioService),
  mAcceptor(ioService),
  mContext(ioService, asio::ssl::context::tlsv1),
  mConnectionManager(),
  mRequestHandler(requestHandler)
{
   // Set Context options - TODO make into configuration settings
   asio::error_code ec;
   mContext.set_options(asio::ssl::context::default_workarounds | 
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);
   mContext.set_password_callback(boost::bind(&TlsServer::getPassword, this));
#define SERVER_CERT_FILE "server.pem"
#define TMP_DH_FILE "dh512.pem"
   mContext.use_certificate_chain_file(SERVER_CERT_FILE, ec);
   if(ec)
   {
      ErrLog(<< "Unable to load server cert chain file: " << SERVER_CERT_FILE << ", error=" << ec.value() << "(" << ec.message() << ")");
   }
   mContext.use_private_key_file(SERVER_CERT_FILE, asio::ssl::context::pem, ec);
   if(ec)
   {
      ErrLog(<< "Unable to load server private key file: " << SERVER_CERT_FILE << ", error=" << ec.value() << "(" << ec.message() << ")");
   }
   mContext.use_tmp_dh_file(TMP_DH_FILE, ec);
   if(ec)
   {
      ErrLog(<< "Unable to load temporary Diffie-Hellman parameters file: " << TMP_DH_FILE << ", error=" << ec.value() << "(" << ec.message() << ")");
   }

   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   asio::ip::tcp::endpoint endpoint(address, port);

   mAcceptor.open(endpoint.protocol());
   mAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
   mAcceptor.bind(endpoint);
   mAcceptor.listen();

   InfoLog(<< "TlsServer started.  Listening on " << address << ":" << port);
}

void
TlsServer::start()
{
   mNewConnection.reset(new TlsConnection(mIOService, mConnectionManager, mRequestHandler, mContext));
   mAcceptor.async_accept(((TlsConnection*)mNewConnection.get())->socket(), boost::bind(&TlsServer::handleAccept, this, asio::placeholders::error));
}

std::string 
TlsServer::getPassword() const
{
   return "test";  // TODO configuration
}

void 
TlsServer::handleAccept(const asio::error_code& e)
{
   if (!e)
   {
      mConnectionManager.start(mNewConnection);

      mNewConnection.reset(new TlsConnection(mIOService, mConnectionManager, mRequestHandler, mContext));
      mAcceptor.async_accept(((TlsConnection*)mNewConnection.get())->socket(), boost::bind(&TlsServer::handleAccept, this, asio::placeholders::error));
   }
   else
   {
      ErrLog(<< "Error in handleAccept: " << e.value() << "-" << e.message());
   }
}

}

/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
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

