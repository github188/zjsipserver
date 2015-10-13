## 注册 ##



### 流程 ###

```
      CU or PU                      SipServer                        CMS
          |                             |                             |
          |                             |                             |
          |(1) SIP(Register)            |                             |
          |---------------------------->|                             |
          |                             |                             |
          |(2) SIP(401 Unauthorized)    |                             |
          |<----------------------------|                             |
          |                             |                             |
          |(3) SIP(Register)            |                             |
          |---------------------------->|                             |
          |                             |                             |
          |                             |(4) HTTP(POST)               |
          |                             |---------------------------->|
          |                             |                             |
          |                             |(5) HTTP(200OK)              |
          |                             |<----------------------------|
          |                             |                             |
          |(6) SIP(200 OK)              |                             |
          |<----------------------------|                             |
          |                             |                             |
          |                             |                             |
          |                             |                             |
```

### 说明 ###

(4)和(5)步只是在用户和设备从新注册时才会执行，即如果是注册刷新操作不执行。

### 协议 ###
```
*(1)*
REGISTER sip:telecom.com SIP/2.0 
Via: SIP/2.0/UDP 127.0.0.1:7000;rport;branch=z9hG4bK17245 
From: gzj <sip:gzj@telecom.com:7000>;tag=12103 
To: gzj <sip:gzj@telecom.com:7000> 
Call-ID: 6565@172.16.163.3 
CSeq: 1 REGISTER 
Contact: <sip:gzj@172.16.163.3:7000> 
Max-Forwards: 70 
User-Agent: TELECOM ncsip/1.0 
Expires: 3600 
Content-Length: 0 

*(2)*
SIP/2.0 401 Unauthorized 
Via:SIP/2.0/UDP 
127.0.0.1:7000;rport=7000;branch=z9hG4bK17245;received=172.16.163.3 
From: gzj <sip:gzj@telecom.com:7000>;tag=12103 
To: gzj <sip:gzj@telecom.com:7000>;tag=41 
Call-ID: 6565@172.16.163.3 
CSeq: 1 REGISTER 
Contact: <sip:gzj@172.16.163.3:7000> 
WWW-Authenticate:  Digest  realm="kedacom.com", 
nonce="00000000000000000000000000018467", 
opaque="00000000000000000000000000006334", qop="auth,auth-int" 
User-Agent: TELECOM ncsip/1.0 
Content-Length: 0

*(3)*
REGISTER sip:telecom.com SIP/2.0 
Via: SIP/2.0/UDP 127.0.0.1:7000; rport;branch=z9hG4bK28145 
From: gzj <sip:gzj@telecom.com:7000>;tag=12103 
To: gzj <sip:gzj@telecom.com:7000> 
Call-ID: 5705@172.16.163.3 
CSeq: 2 REGISTER 
Contact: <sip:gzj@172.16.163.3:7000> 
Authorization:  Digest  username="gzj",  realm="kedacom.com", 
nonce="00000000000000000000000000018467", 
opaque="00000000000000000000000000006334",  uri="sip:kedacom.com", 
qop=auth,  nc=00000001,  cnonce="0a4f113b", 
response="735d9440f5d7367539165ca932a07713" 
Max-Forwards: 70 
User-Agent: TELECOM ncsip/1.0 
Expires: 3600 
Content-Length: 0

*(6)*
SIP/2.0 200 OK 
Via:SIP/2.0/UDP 
127.0.0.1:7000;rport=7000;branch=z9hG4bK28145;received=172.16.163.3 
From: gzj <sip:gzj@telecom.com:7000>;tag=12103 
To: gzj <sip:gzj@telecom.com:7000>;tag=26500 
Call-ID: 5705@172.16.163.3 
CSeq: 2 REGISTER 
Contact: <sip:gzj@172.16.163.3:7000> 
User-Agent: TELECOM ncsip/1.0
Content-Length: 0
```