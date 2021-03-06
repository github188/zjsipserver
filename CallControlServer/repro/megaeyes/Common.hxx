#ifndef COMMON_H
#define COMMON_H

//一些通用的算法和封装
#include "rutil/Data.hxx"
#include "resip/dum/DialogUsageManager.hxx"

#include <map>
#include <string>

namespace common
{

    int Atoi( std::string snum, size_t maxlen=5, int defaultret=0, int radix=10 );

    int MakeXmlReponse( const char *req, std::string &resp, int code=0 );
    int MakeXmlReponse( const char *req, resip::Data &resp, int code=0 );

    typedef std::map<resip::Handled::Id, int > MsgMap;
    extern MsgMap msgmap; //以baseusage的id对象为索引

    class Result
    {
    public:
	Result()
	    :end(false)
	    {}

	int code;
	resip::Data data;
	bool end;
    };

    typedef std::map<int, Result> ResultMap;
    extern ResultMap resultmap; //以httpconnection的编号为索引
}

#endif
