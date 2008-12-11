#ifndef COMMON_H
#define COMMON_H

//一些通用的算法和封装
#include "rutil/Data.hxx"

#include <string>


namespace common
{

    int Atoi( std::string snum, size_t maxlen=5, int defaultret=0, int radix=10 );

    int MakeXmlReponse( const char *req, std::string &resp );
}

#endif
