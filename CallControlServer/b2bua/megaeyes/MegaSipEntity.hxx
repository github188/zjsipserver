#ifndef MEGASIPENTITY_HXX
#define MEGASIPENTITY_HXX

#include <string>
#include <ext/hash_map>
#include "rutil/Data.hxx"
#include "resip/stack/Uri.hxx"

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
    SipEntity();

protected:
    resip::Uri uri_;
};

class Terminal : public SipEntity 
{
public:
    enum TransType { FULL, C2T, C2V2T };

public:
    Terminal();
    
    TransType getTransType()
	{
	    if ( mCurConnNum >= mMaxConnNum )
	    {
		return FULL;
	    }
	    else if ( mCurConnNum < mMaxConnNum - mCameraNum )
	    {
		return C2T;
	    }
	    else
	    {
		return C2V2T;
	    }
	}

public:
    int mMaxConnNum;
    int mCameraNum;
    int mCurConnNum;
};

extern __gnu_cxx::hash_map<resip::Data, Terminal*> Terminals;//!!!when register, need add terminal to Terminals

}
#endif

