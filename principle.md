## SIP协议的统一要求 ##

  1. 以rfc3261文档为主要参考
  1. 消息头部统一采用SIP，消息体分别采用SDP和XML进行描述
  1. 必须支持如下的SIP Method：
    * INVITE，ACK，CANCEL、BYE：音视频会话的建立和释放
    * INFO：会话中的云镜控制
    * SUBSCRIBE，NOTIFY：告警订阅和告警通知
    * REGISTER：注册
    * MESSAGE：sip server 和 PU 之间业务参数配置和查询，PU到SipServer的告警上报
  1. Request-URI采用 “统一编号@GlobalEye.com[:port]” 格式(域名可以用ip地址替换)
  1. Via中的branch参数必须以 z9hG4bK为前缀，后续是一个随机字符串
  1. 传输方式恒定为 UDP
  1. Content-Type在建立会话中使用application/sdp。对于一些应用相关的消息体类型如云台控制、参数设置等，可以扩展使用自定义类型 application/Globe\_Eye30
  1. SIP响应：不需要支持180和183号响应，但要支持100号响应，防止重复发送请求
  1. SDP需支持如下必选参数：
    * Session description：
```
v=    (protocol version) 
o=    (owner/creator and session identifier). 
s=    (session name) 
c=* (connection information - not required if included in all media) 
Time description： 
t=    (time the session is active) 
Media description 
m=    (media name and transport address) 
c=* (connection information - optional if included at session-level) 
b=* (bandwidth information) 
a=* (zero or more media attribute lines) 
示例：
v=0 
o=mhandley 2890844526 2890842807 IN IP4 126.16.64.4 
s=SDP Seminar 
i=A Seminar on the session description protocol 
u=http://www.cs.ucl.ac.uk/staff/M.Handley/sdp.03.ps 
e=mjh@isi.edu (Mark Handley) 
c=IN IP4 224.2.17.12/127 
t=2873397496 2873404696 
a=recvonly 
m=audio 49170 RTP/AVP 0 
m=video 51372 RTP/AVP 31
MPEG4示例：
m=video 49170/2 RTP/AVP 98 
a=rtpmap:98 MP4V-ES/90000 
a=fmtp:98 profile-level-id=1;config=000001B001000001B5090000010000000120008440FA282C2090A21F 
H264示例：
m=video 49170 RTP/AVP 98
a=rtpmap:98 H264/90000 
a=fmtp:98 profile-level-id=42A01E; packetization-mode=0;sprop-parameter-sets=Z0IACpZTBYmI,aMljiA==
```