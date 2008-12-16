#ifndef COMMON_H
#define COMMON_H

//一些通用的算法和封装
#include "rutil/Data.hxx"
#include "resip/dum/DialogUsageManager.hxx"

#include <ext/hash_map>
#include <string>

namespace common
{

    int Atoi( std::string snum, size_t maxlen=5, int defaultret=0, int radix=10 );

    int MakeXmlReponse( const char *req, std::string &resp, int code=0 );

    typedef __gnu_cxx::hash_map<resip::Handled::Id, int > MsgMap;
    extern MsgMap msgmap; //正向map,以用户为索引
}

#endif
