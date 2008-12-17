#ifndef COMMON_H
#define COMMON_H

//һЩͨ�õ��㷨�ͷ�װ
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
    extern MsgMap msgmap; //��baseusage��id����Ϊ����

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
    extern ResultMap resultmap; //��httpconnection�ı��Ϊ����
}

#endif
