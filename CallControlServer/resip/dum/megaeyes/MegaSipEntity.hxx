#ifndef MEGASIPENTITY_HXX
#define MEGASIPENTITY_HXX

#include <string>
#include <ext/hash_map>
#include "rutil/Data.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/NameAddr.hxx"

#include "rutil/resipfaststreams.hxx"

namespace __gnu_cxx
{

template<> struct hash< std::string >
{
    size_t operator()( const std::string& x ) const
	{
	    return hash< const char* >()( x.c_str() );
	}
};

}

/*
resip class have implement hash function. e.g. Uri class
HashValueImp(resip::Uri, resip::Data::from(data).hash());
In fact, invoke resip::Data::from(resip::Uri), this will return Data object which has hash function 
You may look up rutil/HashMap.hxx file, it use MACRO implement hash function of most class.
*/

namespace entity
{

#define DEVIDLEN 18
#define CLIENTIDLEN 14
#define USERIDLEN 18    

class SipEntity //base class for all sip device  
{
public:
    SipEntity()
	{}

    virtual ~SipEntity()
	{}

protected:
    resip::Uri uri_;
};

class Terminal : public SipEntity 
{
public:
    enum TransType { FULL, C2T, C2V2T };

public:
    Terminal()
	:port_(0),mMaxConnNum_(16),mCameraNum_(4),mCurConnNum_(0)
	{}
    
    virtual ~Terminal()
	{}

    TransType getTransType()
	{
	    if ( mCurConnNum_ >= mMaxConnNum_ )
	    {
		return FULL;
	    }
	    else if ( mCurConnNum_ < mMaxConnNum_ - mCameraNum_ )
	    {
		return C2T;
	    }
	    else
	    {
		return C2V2T;
	    }
	}

public:
    resip::Data id_;
    resip::Data host_;
    int port_;
    int mMaxConnNum_;
    int mCameraNum_;
    int mCurConnNum_;
};

#ifndef  RESIP_USE_STL_STREAMS
EncodeStream& 
operator<<(EncodeStream& strm, const Terminal& term);
#endif

std::ostream& 
operator<<(std::ostream& strm, const Terminal& term);

extern __gnu_cxx::hash_map<resip::Data, Terminal*> Terminals;//!!!when register, need add terminal to Terminals

Terminal* getTerminal( const resip::Data &id );

void setTerminal( const resip::Uri &termUri, const resip::NameAddr &termContact );

enum DevType { CLIENT,USER,TERMINAL,STORE,DISPATCH,SIPPROXY,DISPLAY };

DevType getDevType( const resip::Uri& devUri );

void storeEntity( const resip::Uri &devUri, const resip::NameAddr &devContact );

}
#endif

