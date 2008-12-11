#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "repro/megaeyes/XmlContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const XmlContents XmlContents::Empty;

static bool invokeXmlContentsInit = XmlContents::init();

bool
XmlContents::init()
{
   static resip::ContentsFactory<XmlContents> factory;
   (void)factory;
   return true;
}

XmlContents::XmlContents()
   : resip::Contents(getStaticType()),
     mText()
{
}

XmlContents::XmlContents(const resip::Data& txt)
   : Contents(getStaticType()),
     mText(txt)
{
}

XmlContents::XmlContents(const resip::Data& txt, const resip::Mime& contentsType)
   : resip::Contents(contentsType),
     mText(txt)
{
}
 
XmlContents::XmlContents(resip::HeaderFieldValue* hfv, const resip::Mime& contentsType)
   : resip::Contents(hfv, contentsType),
     mText()
{
}
 
XmlContents::XmlContents(const XmlContents& rhs)
   : resip::Contents(rhs),
     mText(rhs.mText)
{
}

XmlContents::~XmlContents()
{
}

XmlContents&
XmlContents::operator=(const XmlContents& rhs)
{
   if (this != &rhs)
   {
      resip::Contents::operator=(rhs);
      mText = rhs.mText;
   }
   return *this;
}

resip::Contents* 
XmlContents::clone() const
{
   return new XmlContents(*this);
}

const Mime& 
XmlContents::getStaticType() 
{
   static resip::Mime type("application", "global_eye_v10+xml");
   return type;
}

EncodeStream& 
XmlContents::encodeParsed(EncodeStream& str) const
{
   DebugLog(<< "XmlContents::encodeParsed " << mText);
   str << mText;
   return str;
}


void 
XmlContents::parse(resip::ParseBuffer& pb)
{
   const char* anchor = pb.position();
   pb.skipToEnd();
   pb.data(mText, anchor);
}

resip::Data 
XmlContents::getBodyData() const
{
   checkParsed();
   return mText;
}


