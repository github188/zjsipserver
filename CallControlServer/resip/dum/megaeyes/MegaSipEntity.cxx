#include "resip/dum/megaeyes/MegaSipEntity.hxx"
#include "rutil/RWMutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

namespace entity
{

__gnu_cxx::hash_map<resip::Data, Terminal*> Terminals;

DevType getDevType( const resip::Uri& devUri )
{
    return TERMINAL;
}

void storeEntity( const resip::Uri &devUri, const resip::NameAddr &devContact )
{
    if ( TERMINAL == getDevType( devUri ) )
    {
	setTerminal( devUri, devContact );
    }
}

resip::RWMutex rwTermMutex;

Terminal* getTerminal( const resip::Data& id )
{
    resip::Lock lock( rwTermMutex, resip::VOCAL_READLOCK );

    if ( Terminals.find(id) != Terminals.end() )
    {
	return Terminals[id];
    }
    else
    {
	return NULL;
    }
}

void setTerminal( const resip::Uri &termUri, const resip::NameAddr &termContact )
{
    resip::Lock lock( rwTermMutex, resip::VOCAL_WRITELOCK );

    Terminal* term = new Terminal();
    term->id_   = termUri.user();
    term->host_ = termContact.uri().host();
    term->port_ = termContact.uri().port();

    Terminals[term->id_] = term;

    InfoLog (<< "Register a terminal: " << *term );
}

#ifndef  RESIP_USE_STL_STREAMS
EncodeStream& 
operator<<(EncodeStream& strm, const Terminal& term)
{
    strm << "id= " << term.id_;
    strm << " host= " << term.host_;
    strm << " port= " << term.port_;
    strm << " maxconn= " << term.mMaxConnNum_;
    strm << " camnum= " << term.mCameraNum_;
    strm << " currnum= " << term.mCurConnNum_;
    
    return strm;
}
#endif

std::ostream& 
operator<<(std::ostream& strm, const Terminal& term)
{
    strm << "id= " << term.id_;
    strm << " host= " << term.host_;
    strm << " port= " << term.port_;
    strm << " maxconn= " << term.mMaxConnNum_;
    strm << " camnum= " << term.mCameraNum_;
    strm << " currnum= " << term.mCurConnNum_;
    
    return strm;
}


}
