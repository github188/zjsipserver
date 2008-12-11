#ifndef RESIP_XMLCONTENTS_HXX
#define RESIP_XMLCONTENTS_HXX

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"

namespace repro
{

class XmlContents : public resip::Contents
{
   public:
      static const XmlContents Empty;

      XmlContents();
      XmlContents(const resip::Data& text);
      XmlContents( resip::HeaderFieldValue* hfv, const resip::Mime& contentType);
      XmlContents(const resip::Data& data, const resip::Mime& contentType);
      XmlContents(const XmlContents& rhs);
      virtual ~XmlContents();
      XmlContents& operator=(const XmlContents& rhs);

      virtual resip::Contents* clone() const;

      virtual resip::Data getBodyData() const;

      static const resip::Mime& getStaticType() ;

      virtual EncodeStream& encodeParsed(EncodeStream& str) const;

      virtual void parse(resip::ParseBuffer& pb);

      resip::Data& text() {checkParsed(); return mText;}

      static bool init();
      
   private:
      resip::Data mText;
};

static bool invokeXmlContentsInit = XmlContents::init();

}

#endif


