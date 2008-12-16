#include "repro/megaeyes/Common.hxx"
#include "repro/megaeyes/MarkupSTL.h"
#include <stdio.h>
#include <assert.h>
#include <map>

namespace common
{

MsgMap msgmap;

std::map<std::string,std::string> msgTypeMap;

int initMap()
{
    //报警
    msgTypeMap["MSG_SUBSCRIBLE_ALARM_REQ"] = "MSG_SUBSCRIBLE_ALARM_RESP";
    msgTypeMap["MSG_ALARM_TRANS_REQ"] = "MSG_ALARM_TRANS_RESP";
    //参数设置和查询
    msgTypeMap["MSG_SET_PU_SERIAL_PORT_REQ"] = "MSG_SET_PU_SERIAL_PORT_RESP";
    msgTypeMap["MSG_GET_PU_SERIAL_PORT_REQ"] = "MSG_GET_PU_SERIAL_PORT_RESP";
    msgTypeMap["MSG_SET_PU_INPUT_OUTPUT_REQ"] = "MSG_SET_PU_INPUT_OUTPUT_RESP";
    msgTypeMap["MSG_GET_PU_INPUT_OUTPUT_REQ"] = "MSG_GET_PU_INPUT_OUTPUT_RESP";
    msgTypeMap["MSG_SET_PU_TIME_REQ"] = "MSG_SET_PU_TIME_RESP";
    msgTypeMap["MSG_GET_PU_TIME_REQ"] = "MSG_GET_PU_TIME_RESP";
    msgTypeMap["MSG_SET_PU_IMAGE_ENCODE_PARA_REQ"] = "MSG_SET_PU_IMAGE_ENCODE_PARA_RESP";
    msgTypeMap["MSG_GET_PU_IMAGE_ENCODE_PARA_REQ"] = "MSG_GET_PU_IMAGE_ENCODE_PARA_RESP";
    msgTypeMap["MSG_SET_PU_IMAGE_DISPLAY_PARA_REQ"] = "MSG_SET_PU_IMAGE_DISPLAY_PARA_RESP";
    msgTypeMap["MSG_GET_PU_IMAGE_DISPLAY_PARA_REQ"] = "MSG_GET_PU_IMAGE_DISPLAY_PARA_RESP";
    msgTypeMap["MSG_SET_PU_IMAGE_TEXT_PARA_REQ"] = "MSG_SET_PU_IMAGE_TEXT_PARA_RESP";
    msgTypeMap["MSG_GET_PU_IMAGE_TEXT_PARA_REQ"] = "MSG_GET_PU_IMAGE_TEXT_PARA_RESP";
    msgTypeMap["MSG_SET_PU_ALARM_CONFIGURATION_REQ"] = "MSG_SET_PU_ALARM_CONFIGURATION_RESP";
    msgTypeMap["MSG_GET_PU_ALARM_CONFIGURATION_REQ"] = "MSG_GET_PU_ALARM_CONFIGURATION_RESP";
    msgTypeMap["MSG_GET_PU_STATE_REQ"] = "MSG_GET_PU_STATE_RESP";
    msgTypeMap["MSG_GET_PU_LOG_REQ"] = "MSG_GET_PU_LOG_RESP";
    msgTypeMap["MSG_SET_PRESET_PTZ_REQ"] = "MSG_SET_PRESET_PTZ_RESP";
    msgTypeMap["MSG_GET_PRESET_PTZ_REQ"] = "MSG_GET_PRESET_PTZ_RESP";
    msgTypeMap["MSG_DELETE_PRESET_PTZ_REQ"] = "MSG_DELETE_PRESET_PTZ_RESP";
    msgTypeMap["MSG_SET_CRUISE_TRACK_REQ"] = "MSG_SET_CRUISE_TRACK_RESP";
    msgTypeMap["MSG_GET_CRUISE_TRACK_REQ"] = "MSG_GET_CRUISE_TRACK_RESP";
    msgTypeMap["MSG_COM_TRANSPARENT_REQ"] = "MSG_COM_TRANSPARENT_RESP";
    msgTypeMap["MSG_SET_NPU_ADDR_REQ"] = "MSG_SET_NPU_ADDR_RESP";
    msgTypeMap["MSG_GET_NPU_ADDR_REQ"] = "MSG_GET_NPU_ADDR_RESP";
    msgTypeMap["MSG_SET_CAMERA_INFO_REQ"] = "MSG_SET_CAMERA_INFO_RESP";
    msgTypeMap["MSG_GET_CAMERA_INFO_REQ"] = "MSG_GET_CAMERA_INFO_RESP";
    //控制云台
    msgTypeMap["MSG_PTZ_SET_REQ"] = "MSG_PTZ_SET_RESP";
    //其他!!!

    return 0;
}

int nInitMap = initMap();

int Atoi( std::string snum, size_t maxlen, int defaultret, int radix )
{
    if ( 10 == radix )
    {
	if ( !snum.empty() && snum.size()<=maxlen )
	    return atoi( snum.c_str() );
    }
    else if ( 16 == radix )
    {
	if ( !snum.empty() && snum.size()<=maxlen )
	{
	    int num=0;
	    sscanf( snum.c_str(), "%x", &num );
	    return num;
	}
    }

    return defaultret;
}

int MakeXmlReponse( const char *req, std::string &resp, int code )
{
    assert( req );

    if ( req )
    {
	CMarkupSTL ReqXML;

        ReqXML.SetDoc( req );
	
        if ( ReqXML.FindElem() )
        {
	    ReqXML.IntoElem();
	    while ( ReqXML.FindElem() )
	    {
		std::string sTag = ReqXML.GetTagName();

		if ( sTag == "IE_HEADER"  )
		{
		    std::string msgType  = ReqXML.GetAttrib( "MessageType" );
		    ReqXML.SetAttrib( "MessageType", msgTypeMap[msgType].c_str() );

                    //std::string msgseq = ReqXML.GetAttrib( "SequenceNumber" );
		    std::string msgSrcId = ReqXML.GetAttrib( "SourceID" );
		    ReqXML.SetAttrib( "SourceID" , ReqXML.GetAttrib( "DestinationID" ).c_str() );
		    ReqXML.SetAttrib( "DestinationID", msgSrcId.c_str() );
		}
		else
		{
		    ReqXML.RemoveElem();
		}
	    }
	    
	    //<IE_RESULT Value = "操作结果" ErrorCode = "错误码" /> 
	    ReqXML.AddElem( "IE_RESULT" );
	    ReqXML.SetAttrib( "Value", "Success" );
	    ReqXML.SetAttrib( "ErrorCode", code );

	    resp = ReqXML.GetDoc();

	    return 0;
        }
    }

    return -1;
}

}
