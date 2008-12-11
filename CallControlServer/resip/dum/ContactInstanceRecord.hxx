#if !defined(resip_ContactInstanceRecord_hxx)
#define resip_ContactInstanceRecord_hxx

#include <vector>
#include <list>

#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Tuple.hxx"

namespace resip
{
class ContactInstanceRecord 
{
   public:
      ContactInstanceRecord();
      static ContactInstanceRecord makeRemoveDelta(const NameAddr& contact);
      static ContactInstanceRecord makeUpdateDelta(const NameAddr& contact, 
                                                   UInt64 expires,  // absolute time in secs
                                                   const SipMessage& msg);
      
      NameAddr mContact;    // can contain callee caps and q-values
      UInt64 mRegExpires;   // in seconds
      UInt64 mLastUpdated;  // in seconds
      Tuple mReceivedFrom;  // source transport, IP address, and port 
      NameAddrs mSipPath;   // Value of SIP Path header from the request
      Data mInstance;       // From the instance parameter; usually a UUID URI
      UInt32 mRegId;        // From regid parameter of Contact header
      Data mServerSessionId;// if there is no SIP Path header, the connection/session identifier 
      // Uri gruu;  (GRUU is currently derived)
      
      bool operator==(const ContactInstanceRecord& rhs) const;
};

typedef std::list<ContactInstanceRecord> ContactList;

class RegistrationBinding 
{
   public:
      Data mAor;                      // canonical URI for this AOR and its aliases
      ContactList mContacts;
      std::vector<Uri> mAliases;     
};

struct RegistrationBindingDelta
{
      Data mAor;
      std::vector<ContactInstanceRecord> mInserts;
      std::vector<ContactInstanceRecord> mUpdates;
      std::vector<ContactInstanceRecord> mRemoves;
};

}
#endif
