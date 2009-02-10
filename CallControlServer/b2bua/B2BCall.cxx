
#include <sstream>

#include "resip/dum/ServerInviteSession.hxx"

#include "B2BCall.hxx"
#include "CallHandle.hxx"
#include "Logging.hxx"
#include "MyAppDialog.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

// DialogUsageManager *B2BCall::dum = NULL;
// CallController *B2BCall::callController = NULL;
// list<B2BCall *> B2BCall::calls;

//B2BCall Invoke Sequsence
//c offer t answer c:
//  onNewSession -> onOffer(recv c offer) -> doNewCall -> doAuthorizationPending ->doAuthorizationSuccess ->doMediaProxySuccess ->doReadyToDial(send offer to t) ->
//  onAnswer(recv t answer) -> doCallAccepted -> doCallAcceptedMediaProxySuccess(send answer to c) -> doCallActive

//c noOffer t offer c answer t:
//  onNewSession  -> doNewCall -> doAuthorizationPending ->doAuthorizationSuccess ->doMediaProxySuccess ->doReadyToDial(send noOffer to t) ->
//  onOffer(recv t offer) -> doCallOffered(send offer to c) -> onAnswer(recv c answer) -> doCallAnswered(send answer to t) -> doCallActive

// new add sequence
//c offer t:
// onOffer(recv c offer) -> sis->provideAnswer(sdp)[send answer to c]

//t offer c:
// onOfferRequired( recv c noOffer) -> sis->provideOffer(sdp)[send offer to c] -> onAnswer

char* B2BCall::B2BCallStateName[] =  
{
    "NewCall",				// just started
    "CallerCancel",				// CANCEL received from A leg
    "AuthorizationPending",
    "AuthorizationSuccess",
    "AuthorizationFail",
//    RoutesPending,				// routes requested
//    RoutesSuccess,
//    RoutesFail,
//   MediaProxyPending,				// Media Proxy requested
    "MediaProxySuccess",
    "MediaProxyFail",
    "ReadyToDial",				// Route ready
    "DialInProgress",				// INVITE sent
    "DialFailed",				// Network error, e.g. ICMP
    "DialRejected",				// SIP error received
						// error code in data member
						// failureStatusCode
    "SelectAlternateRoute",			// Need to select another
						// route
    "DialAborted",				// No other carriers 
						// available, or a failure
						// that doesn't require us
						// to try another carrier
						// e.g. Busy
//    DialReceived100,				// 100 response received
    "DialReceived180",			// 180 response received
//    DialReceived183,				// 183 response received
    "DialReceivedEarlyAnswer",		// early answer (SDP) received
//    DialEarlyMediaProxyRequested,		// Media proxy requested for
    // early media
    "DialEarlyMediaProxySuccess",		// Media Proxy ready
    "DialEarlyMediaProxyFail",		// Media Proxy failed
    "CallAccepted",				// 200 received
//    CallAcceptedMediaProxyRequested,		// Media proxy requested for
    // media
    "CallAcceptedMediaProxySuccess",		// Media Proxy ready
    "CallAcceptedMediaProxyFail",		// Media proxy failed
    "CallActive",				// Call in progress
    "CallerHangup",				// Caller has hungup
    "CalleeHangup",				// Callee has hungup
    "LocalHangup",				// B2BUA initiated hangup
    "CallStop",				// Call is stopped
//    CallStopMediaProxyNotified,		// Media proxy informed
    "CallStopMediaProxySuccess",		// Media proxy acknowledged
    "CallStopMediaProxyFail",			// Media proxy failed to ack
    "CallStopFinal"				// Call can be purged from
};


Data B2BCall::callStateNames[] = 
{
    Data("NewCall"),
    Data("CallerCancel"),
    Data("AuthorizationPending"),
    Data("AuthorizationSuccess"),
    Data("AuthorizationFail"),
    Data("MediaProxySuccess"),
    Data("MediaProxyFail"),
    Data("ReadyToDial"),
    Data("DialInProgress"),
    Data("DialFailed"),
    Data("DialRejected"),
    Data("SelectAlternateRoute"),
    Data("DialAborted"),
    Data("DialReceived180"),
    Data("DialReceivedEarlyAnswer"),
    Data("DialEarlyMediaProxySuccess"),
    Data("DialEarlyMediaProxyFail"),
    Data("CallAccepted"),
    Data("CallAcceptedMediaProxySuccess"),
    Data("CallAcceptedMediaProxyFail"),
    Data("CallActive"),
    Data("CallerHangup"),
    Data("CalleeHangup"),
    Data("LocalHangup"),
    Data("CallStop"),
    Data("CallStopMediaProxySuccess"),
    Data("CallStopMediaProxyFail"),
    Data("CallStopFinal")
};

const char *B2BCall::basicClearingReasonName[] = 
{
    "NO ANSWER",
    "BUSY",
    "CONGESTION",
    "ERROR",
    "ANSWERED"
};

/* void B2BCall::setDum(DialogUsageManager *dum) {
  B2BCall::dum = dum;
} */

/* void B2BCall::setCallController(CallController *callController) {
  B2BCall::callController = callController;
} */

void 
B2BCall::addALegAppDialog(MyAppDialog *aLegAppDialog)
{
    assert(aLegAppDialog!=NULL);
    this->aLegAppDialogs[aLegAppDialog->getDialogId().getCallId()] = aLegAppDialog;
    aLegAppDialog->setB2BCall(this);
}

B2BCall::B2BCall(CDRHandler& cdrHandler, DialogUsageManager& dum, AuthorizationManager& authorizationManager, 
		 MyAppDialog *aLegAppDialog, const resip::NameAddr& sourceAddr, const resip::Uri& destinationAddr, 
		 const resip::Data& authRealm, const resip::Data& authUser, const resip::Data& authPassword, 
		 const resip::Data& srcIp, const resip::Data& contextId, const resip::Data& accountId, 
		 const resip::Data& baseIp, const resip::Data& controlId) 
    : cdrHandler(cdrHandler), 
      dum(dum), 
      authorizationManager(authorizationManager), 
      sourceAddr(sourceAddr), 
      destinationAddr(destinationAddr), 
      authRealm(authRealm), 
      authUser(authUser), 
      authPassword(authPassword), 
      srcIp(srcIp), 
      contextId(contextId), 
      accountId(accountId), 
      baseIp(baseIp), 
      controlId(controlId) 
{
    callHandle = NULL;
    fullClearingReason = Unset;
    rejectOtherCode = 0;
    failureStatusCode = -1;

    //this->aLegAppDialog = aLegAppDialog;
    this->aFirstLegAppDialog = aLegAppDialog;

    B2BUA_LOG_DEBUG( <<"B2BCall add alegappdialog callid "<<aLegAppDialog->getDialogId().getCallId() );
    aFirstLegAppDialogCallId = aLegAppDialog->getDialogId().getCallId();
    this->aLegAppDialogs[aLegAppDialog->getDialogId().getCallId()] = aLegAppDialog;
    aLegAppDialog->setB2BCall(this);
    
    bLegAppDialogSet = NULL;
    bLegAppDialog = NULL;
    //callState = CALL_NEW;
    callState = NewCall;
    time(&startTime);
    connectTime = 0;
    finishTime = 0;
    //aLegSdp = NULL;
    //bLegSdp = NULL;
    try 
    {
	//mediaManager = new MediaManager(*this, aLegAppDialog->getDialogId().getCallId(), aLegAppDialog->getDialogId().getLocalTag(), Data(""));
	mediaManager = new MediaManager( *this, aLegAppDialog->getDialogId().getLocalTag(), Data("") );
    } 
    catch (...) 
    {
	B2BUA_LOG_ERR( << "failed to instantiate MediaManager");
	throw new exception;
    }
    earlyAnswerSent = false;
    failureReason = NULL;
//  calls.push_back(this);
}

B2BCall::~B2BCall() 
{
    if(callHandle != NULL)
	delete callHandle;
    if(mediaManager != NULL)
	delete mediaManager;
    if(failureReason != NULL)
	delete failureReason;
    
    //if(aLegAppDialog != NULL)
    //	aLegAppDialog->setB2BCall(NULL);
    //do foreach aLegAppDialogs, everyone exec setB2BCall(NULL); !!!NTD

    if(bLegAppDialogSet != NULL)
	bLegAppDialogSet->setB2BCall(NULL);
    if(bLegAppDialog != NULL)
	bLegAppDialog->setB2BCall(NULL);
}


bool B2BCall::setCallState(B2BCall::B2BCallState newCallState) 
{
    B2BUA_LOG_DEBUG( << "CallState change: " << callState << ":" << getCallStateName(callState) << " -> " << newCallState << ":" << getCallStateName(newCallState) << ": ");
    if(!isCallStatePermitted(newCallState)) 
    {
	B2BUA_LOG_ERR( <<"Denied call state change: " << callState << ":" << getCallStateName(callState).c_str() << "->"  
		       << newCallState << ":" << getCallStateName(newCallState).c_str() );
	return false;
    }

    B2BUA_LOG_DEBUG( <<"permitted.");
    callState = newCallState;
    return true;
}

bool B2BCall::isCallStatePermitted(B2BCall::B2BCallState newCallState) 
{
    switch(callState) 
    {
    case NewCall:
	switch(newCallState) 
	{
	case CallerCancel:
	case AuthorizationPending:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case CallerCancel:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default: 
	    return false;
	};

    case AuthorizationPending:
	switch(newCallState) 
	{
	case AuthorizationSuccess:
	case AuthorizationFail:
	case CallerCancel:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case AuthorizationSuccess:
	switch(newCallState) 
	{
	case CallerCancel:
	case MediaProxySuccess:
	case MediaProxyFail:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case AuthorizationFail:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case MediaProxySuccess:
	switch(newCallState) {
	case CallerCancel:
	case ReadyToDial:
	    //case DialInProgress:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case MediaProxyFail:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case ReadyToDial:
	switch(newCallState) 
	{
	case CallerCancel:
	case DialInProgress:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	}

    case DialInProgress:
	switch(newCallState) 
	{
	case CallerCancel:
	case DialFailed:
	case DialRejected:
	case DialInProgress:	// FIXME - should we change to same state?
	    //case DialReceived100:
	case DialReceived180:
	case DialReceivedEarlyAnswer:
	case CallAccepted:
	case CallStop: //zhangjun add
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case DialFailed:
	switch(newCallState) 
	{
	case CallerCancel:
	    //case MediaProxySuccess:
	    //case ReadyToDial:
	case SelectAlternateRoute:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case DialRejected:
	switch(newCallState) 
	{
	case CallerCancel:
	    //case MediaProxySuccess:
	    //case ReadyToDial:
	case SelectAlternateRoute:
	case DialAborted:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case SelectAlternateRoute:
	switch(newCallState) 
	{
	case CallerCancel:
	case ReadyToDial:
	case DialAborted:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};
  
    case DialAborted:
	switch(newCallState) 
	{
	case CallStop:
	    return true;
	default:
	    return false;
	};

	/* case DialReceived100:
	   switch(newCallState) {
	   case CallerCancel:
	   case DialInProgress:
	   case DialReceived180:
	   case DialReceivedEarlyAnswer:
	   case DialFailed:
	   case DialRejected:
	   case CallAccepted:
	   case CallStop:
	   callState = newCallState;
	   return true;
	   default:
	   return false;
	   }; */

    case DialReceived180:
	switch(newCallState) 
	{
	case CallerCancel:
	case DialInProgress:
	case DialFailed:
	case DialRejected:
	case DialReceivedEarlyAnswer:
	case CallAccepted:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case DialReceivedEarlyAnswer:
	switch(newCallState) 
	{
	case CallerCancel:
	case DialInProgress:
	case DialFailed:
	case DialRejected:
	case DialEarlyMediaProxySuccess:
	case DialEarlyMediaProxyFail:
	case CallAccepted:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

/*  case DialEarlyMediaProxyRequested:
    switch(newCallState) {
    case :
    callState = newCallState;
    return true;
    default:
    return false;
    }; */

    case DialEarlyMediaProxySuccess:
	switch(newCallState) 
	{
	case CallerCancel:
	case DialInProgress:
	case DialFailed:
	case DialRejected:
	case CallAccepted:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case DialEarlyMediaProxyFail:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	}; 

    case CallAccepted:
	switch(newCallState) 
	{
	case CallerCancel:
	case CallAcceptedMediaProxySuccess:
	case CallAcceptedMediaProxyFail:
	case CallActive:
	case CallerHangup:
	case CalleeHangup:
	case LocalHangup:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

/*  case CallAcceptedMediaProxyRequested:
    switch(newCallState) {
    case :
    callState = newCallState;
    return true;
    default:
    return false;
    }; */

    case CallAcceptedMediaProxySuccess:
	switch(newCallState) 
	{
	case CallerCancel:
	case CallActive:
	case CallerHangup:
	case CalleeHangup:
	case LocalHangup:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case CallAcceptedMediaProxyFail:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case CallActive:
	switch(newCallState) 
	{
	case CallerCancel:
	case CallerHangup:
	case CalleeHangup:
	case LocalHangup:
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case CallerHangup:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case CalleeHangup:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case LocalHangup:
	switch(newCallState) 
	{
	case CallStop:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case CallStop:
	switch(newCallState) 
	{
	case CallStopMediaProxySuccess:
	case CallStopMediaProxyFail:
	case CallStopFinal:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

/*  case CallStopMediaProxyNotified:
    switch(newCallState) {
    case :
    callState = newCallState;
    return true;
    default:
    return false;
    }; */

    case CallStopMediaProxySuccess:
	switch(newCallState) 
	{
	case CallStopFinal:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	};

    case CallStopMediaProxyFail:
	switch(newCallState) 
	{
	case CallStopFinal:
	    callState = newCallState;
	    return true;
	default:
	    return false;
	}; 

    case CallStopFinal:
	return false;  // no state changes permitted

    default:
	B2BUA_LOG_ERR( <<"B2BCall in unknown call state " << callState );
	return false;
    };
};

const Data& B2BCall::getCallStateName(B2BCallState s) 
{
  return callStateNames[s];
};

void B2BCall::setALegSdp(const resip::Data& callid, const SdpContents& sdp, const in_addr_t& msgSourceAddress) 
{
  // try {
    mediaManager->setALegSdp(callid, sdp, msgSourceAddress);
  //} catch (...) {
    // FIXME - call state
   // B2BUA_LOG_WARNING, "failed to accept SDP from A leg");
    //callState = CALL_STOP;
  //}
}

void B2BCall::setBLegSdp(const resip::Data& callid, const SdpContents& sdp, const in_addr_t& msgSourceAddress) 
{
    //try {
    mediaManager->setBLegSdp(callid, sdp, msgSourceAddress);
    //} catch (...) {
    // B2BUA_LOG_WARNING, "failed to accept SDP from B leg");
    //callState = CALL_STOP;
    //}
}

void B2BCall::setBLegAppDialog(MyAppDialog *myAppDialog) 
{
    bLegAppDialog = myAppDialog; 
    //mediaManager->setToTag(bLegAppDialog->getDialogId().getLocalTag());
}

void B2BCall::releaseAppDialog(MyAppDialog *myAppDialog) //MyAppDialog's deconstruct invoke it
{
    //if(myAppDialog == aLegAppDialog) 
    //{
    //	aLegAppDialog = NULL;
    //  }
    B2BUA_LOG_DEBUG( <<"releaseAppDialog for callid " <<myAppDialog->getDialogId().getCallId() );

    if ( aLegAppDialogs.find(myAppDialog->getDialogId().getCallId()) != aLegAppDialogs.end() )
    {
	aLegAppDialogs.erase( myAppDialog->getDialogId().getCallId() );
    }
    else if(myAppDialog == bLegAppDialog) 
    {
	bLegAppDialog = NULL;
    }
    else
    {
	B2BUA_LOG_ERR( <<"releaseAppDialog for unknown AppDialog");
    }
}

void B2BCall::releaseAppDialogSet(MyAppDialogSet *myAppDialogSet) 
{
    B2BUA_LOG_DEBUG( <<"releaseAppDialogSet" );

    if(myAppDialogSet == bLegAppDialogSet) 
    {
	bLegAppDialogSet = NULL;
    }
    else 
    {
	B2BUA_LOG_ERR( <<"releaseAppDialogSet for unknown AppDialogSet");
    }
}

/* This method is called regularly.
  Instead, the callbacks or setCallState should send a message to 
  B2BUA to say that the call needs attention. */
void B2BCall::checkProgress(time_t now, bool stopping) 
{
    switch(callState) {
    case NewCall:
	doNewCall();
	break;
    case CallerCancel:
	doCallerCancel();
	break;
    case AuthorizationPending:
	doAuthorizationPending();
	break;
    case AuthorizationSuccess:
	doAuthorizationSuccess();
	break;
    case AuthorizationFail:
	doAuthorizationFail();
	break;
    case MediaProxySuccess:
	doMediaProxySuccess();
	break;
    case MediaProxyFail:
	doMediaProxyFail();
	break;
    case ReadyToDial:
	doReadyToDial();
	break;
    case DialInProgress:
	// FIXME - route timeout?
	doDialInProgress();
	break;
    case DialFailed:
	doDialFailed();
	break;
    case DialRejected:
	doDialRejected();
	break;
    case SelectAlternateRoute:
	doSelectAlternateRoute();
	break;
    case DialAborted:
	doDialAborted();
	break;
	/* case DialReceived100:
	   doDialReceived100();
	   break; */
    case DialReceived180:
	doDialReceived180();
	break;
	/* case CALL_DIAL_183:
	   doCallDial183();
	   break; */
    case DialReceivedEarlyAnswer:
	doDialReceivedEarlyAnswer();
	break;
    case DialEarlyMediaProxySuccess:
	doDialEarlyMediaProxySuccess();
	break;
    case DialEarlyMediaProxyFail:
	doDialEarlyMediaProxyFail();
	break;
	//case CALL_DIAL_PROGRESS:
	//doCallDialProgress();
	//break;
    case CallAccepted:
	doCallAccepted();
	break;
    case CallAcceptedMediaProxySuccess:
	doCallAcceptedMediaProxySuccess();
	break;
    case CallAcceptedMediaProxyFail:
	doCallAcceptedMediaProxyFail();
	break;
    case CallActive:
	doCallActive();
	break;
    case CallerHangup:
    case CalleeHangup:
    case LocalHangup:
	doHangup();
    case CallStop:
	doCallStop();
	break;
    case CallStopMediaProxySuccess:
	doCallStopMediaProxySuccess();
	break;
    case CallStopMediaProxyFail:
	doCallStopMediaProxyFail();
	break;
    case CallStopFinal:
	doCallStopFinal();
	break;
    default:
	B2BUA_LOG_ERR( <<"unknown call state" << callState);
	assert(0);
	break;
    }
}

bool B2BCall::isComplete() 
{
    return (callState == CallStopFinal);
}

B2BCall::CallStatus B2BCall::getStatus() 
{
    switch(callState) {
    case NewCall:
	return PreDial;
    case CallerCancel:
	return Finishing;
    case AuthorizationPending:
    case AuthorizationSuccess:
    case AuthorizationFail:
    case MediaProxySuccess:
    case MediaProxyFail:
	return PreDial;
    case ReadyToDial:
    case DialInProgress:
    case DialFailed:
    case DialRejected:
    case SelectAlternateRoute:
    case DialAborted:
	//case DialReceived100:
    case DialReceived180:
    case DialReceivedEarlyAnswer:
    case DialEarlyMediaProxySuccess:
    case DialEarlyMediaProxyFail:
	return Dialing;
    case CallAccepted:
    case CallAcceptedMediaProxySuccess:
    case CallAcceptedMediaProxyFail:
    case CallActive:
	return Connected;
    case CallerHangup:
    case CalleeHangup:
    case LocalHangup:
    case CallStop:
    case CallStopMediaProxySuccess:
    case CallStopMediaProxyFail:
    case CallStopFinal:
	return Finishing;
    default:
	return Unknown;
    }
}

void B2BCall::doDialInProgress()
{
    time_t now;

    time(&now);
    if ( now - startTime > 15 )
	setCallState(CallStop);
}

void B2BCall::doNewCall() //only first caller will invoke this function!!!
{
    // Indicate trying
    //ServerInviteSession *sis = (ServerInviteSession *)(aLegAppDialog->getInviteSession().get());
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    B2BUA_LOG_DEBUG( <<"doNewCall send 100Trying!");
    //sis->provisional(100);
    callHandle = authorizationManager.authorizeCall(sourceAddr, destinationAddr, authRealm, 
						    authUser, authPassword, srcIp, contextId, 
						    accountId, baseIp, controlId, startTime);
    if(callHandle == NULL) 
    {
	B2BUA_LOG_WARNING( <<"failed to get callHandle");
	setCallState(CallStop);
    } 
    else
    {
	//must is here!!!
	setCallState(AuthorizationPending);
	doAuthorizationPending();
    }
}

void B2BCall::doCallerCancel() 
{
    setClearingReason(NoAnswerCancel, -1);
    // If B leg dialing, send a CANCEL
    if(bLegAppDialogSet != NULL) {
	bLegAppDialogSet->end();
    }
    setCallState(CallStop);
}

void B2BCall::doAuthorizationPending() 
{
    switch(callHandle->getAuthResult()) 
    {
    case CC_PENDING:
	return;
    case CC_PERMITTED:
	setCallState(AuthorizationSuccess);
	doAuthorizationSuccess();
	break;
    default:
	setCallState(AuthorizationFail);
    }
    return;
}

void B2BCall::doAuthorizationSuccess() 
{
    // point to the first route
    callRoute = callHandle->getRoutes().begin();
    if ( callRoute == callHandle->getRoutes().end() ) 
    {
	appRef1 = Data("");
	appRef2 = Data("");
	setClearingReason(InvalidDestination, -1);
	B2BUA_LOG_NOTICE( <<"no routes returned");
	ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get()); 
	sis->reject(404);  // Server internal error // find not terminal
	setCallState(CallStop);
	return;
    }

    appRef1 = (*callRoute)->getAppRef1();
    appRef2 = (*callRoute)->getAppRef2();
    setCallState(MediaProxySuccess);
    doMediaProxySuccess();
}

void B2BCall::doAuthorizationFail() 
{
    setClearingReason(AuthError, -1);
    if(aFirstLegAppDialog != NULL) 
    {
	ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
	switch(callHandle->getAuthResult()) 
	{
	case CC_AUTH_REQUIRED:
	    sis->reject(401);  // Unauthorized/credentials required
	    break;
	case CC_PAYMENT_REQUIRED:
	    sis->reject(402);  // Payment required
	    break;
	case CC_REQUEST_DENIED:
	case CC_IP_DENIED:
	    sis->reject(403);  // Permission denied/forbidden
	    break;  
	case CC_ERROR:
	    sis->reject(500);  // Server internal error
	    break;
	case CC_INVALID:
	    sis->reject(403);  // Forbidden
	    break;
	case CC_DESTINATION_INCOMPLETE:
	    sis->reject(484);  // Incomplete
	    break;
	case CC_DESTINATION_INVALID:
	    sis->reject(404);  // User doesn't exist
	    break;
	case CC_TIMEOUT:
	    sis->reject(500);  // Server internal error
	    break;
	default:
	    sis->reject(500);  // Server internal error
	}
    }
    setCallState(CallStop);
}

void B2BCall::doMediaProxySuccess() 
{
  setCallState(ReadyToDial);
  doReadyToDial();
}

void B2BCall::doMediaProxyFail() 
{
  setClearingReason(AuthError, -1);
  B2BUA_LOG_ERR( <<"failed to prepare media proxy");
  setCallState(CallStop);
}

void B2BCall::doReadyToDial() //c offer t 
{
    // Media Proxy is ready, now we can create the B-leg and send an INVITE
    //SharedPtr<UserProfile> outboundUserProfile(new UserProfile);
    SharedPtr<UserProfile> outboundUserProfile(dum.getMasterUserProfile());
    outboundUserProfile->setDefaultFrom((*callRoute)->getSourceAddr());
    outboundUserProfile->setDigestCredential((*callRoute)->getAuthRealm(), (*callRoute)->getAuthUser(), (*callRoute)->getAuthPass());

    if( (*callRoute)->getOutboundProxy() != Uri() )
    {
	outboundUserProfile->setOutboundProxy((*callRoute)->getOutboundProxy());
    }
    bLegAppDialogSet = new MyAppDialogSet(dum, this, outboundUserProfile);

    SharedPtr<SipMessage> msgB;

    //use real terminal host to gene addrUri
    Uri camUri = (*callRoute)->getDestinationAddr().uri();
    

    try 
    {   //client provide offer
	B2BUA_LOG_INFO( <<"Client Provide Offer, Send to Terminal" );
	SdpContents *initialOffer = (SdpContents *)mediaManager->getALegSdp( aFirstLegAppDialog->getDialogId().getCallId() ).clone();
	msgB = dum.makeInviteSession( (*callRoute)->getDestinationAddr(), outboundUserProfile, initialOffer, bLegAppDialogSet );
	delete initialOffer;
	dum.send(msgB);

	setCallState(DialInProgress);
    } 
    catch (...) 
    {   //client dont provide offer
	B2BUA_LOG_INFO( <<"Client Don't Provide Offer, Send to Terminal" );
	msgB = dum.makeInviteSession((*callRoute)->getDestinationAddr(), outboundUserProfile, NULL, bLegAppDialogSet);
        dum.send(msgB);

        setCallState(DialInProgress);
    } 
}

// A route failed (timeout, ICMP, invalid domain, etc), try the next route
void B2BCall::doDialFailed() 
{
    if ( bLegAppDialogSet != NULL ) 
    {
	bLegAppDialogSet->end();
	bLegAppDialogSet->setB2BCall(NULL);
	bLegAppDialogSet = NULL;
    }
    
    if ( bLegAppDialog != NULL )
    {
	bLegAppDialog->setB2BCall(NULL);
	bLegAppDialog = NULL;
    }

    failureStatusCode = -1;
    //setClearingReason(AuthError, -1);
    setCallState(SelectAlternateRoute); 
    doSelectAlternateRoute();
}

// A route rejected the call, try the next route
void B2BCall::doDialRejected() 
{
    switch(failureStatusCode) 
    {
    case -1:
	//setClearingReason(AuthError, -1);
	setCallState(SelectAlternateRoute);
	doSelectAlternateRoute();
	break;
	//case 480: // Temporarily not available
    case 486: // Busy
	//case 600: // Busy everywhere
	//case 603: // Decline
	// We will consider all of the above to be `BUSY'
	setClearingReason(RejectBusy, failureStatusCode);
	setCallState(DialAborted);
	doDialAborted();
	break;
    default:
	//setClearingReason(RejectOther, failureStatusCode);
	if ( bLegAppDialogSet != NULL ) 
	{
	    bLegAppDialogSet->end();
	    bLegAppDialogSet->setB2BCall(NULL);
	    bLegAppDialogSet = NULL;
	}

	if ( bLegAppDialog != NULL )
	{
	    bLegAppDialog->setB2BCall(NULL);
	    bLegAppDialog = NULL;
	}

	setCallState(SelectAlternateRoute);
	doSelectAlternateRoute();
	break;
    }
}

void B2BCall::doSelectAlternateRoute() 
{
    callRoute++;
    if(callRoute == callHandle->getRoutes().end()) 
    {
	B2BUA_LOG_DEBUG( <<"no routes remaining, aborting attempt");
	setCallState(DialAborted);
	doDialAborted();
	return;
    }
    appRef1 = (*callRoute)->getAppRef1();
    appRef2 = (*callRoute)->getAppRef2();
    setCallState(ReadyToDial);
    doReadyToDial();
}

void B2BCall::doDialAborted() 
{
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    if(failureStatusCode == -1) 
    {
	setClearingReason(AuthError, failureStatusCode); // FIXME
	sis->reject(503);
    } 
    else 
    {
	setClearingReason(RejectOther, failureStatusCode);
	sis->reject(failureStatusCode);
	B2BUA_LOG_DEBUG( << "B2BCall::doDialAborted reject "<< failureStatusCode );
    }
    setCallState(CallStop);
    doCallStop();
}


/* void B2BCall::doDialReceived100() {
  setCallState(DialInProgress);
} */

void B2BCall::doDialReceived180() 
{
    if(setCallState(DialInProgress)) 
    {
	ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
	sis->provisional(180);
    }
}

/* void B2BCall::doCallDial183() {
  // FIXME - do nothing for now, unless there is an offer
  setCallState(DialInProgress);
} */

void B2BCall::doDialReceivedEarlyAnswer() 
{
    if(earlyAnswerSent == true) 
    {
	setCallState(DialInProgress);
	return;
    }
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    //sis->provideAnswer(*bLegSdp);
    try
    {
	sis->provideAnswer(mediaManager->getBLegSdp());
    } 
    catch (...) 
    {
	B2BUA_LOG_WARNING( <<"failed to get B Leg SDP");
	setClearingReason(AuthError, -1);
	setCallState(DialEarlyMediaProxyFail);
	return;
    } 
    //sis->provisional(183); 
    //earlyAnswerSent = true;
    setCallState(DialEarlyMediaProxySuccess);
    doDialEarlyMediaProxySuccess();
}

void B2BCall::doDialEarlyMediaProxySuccess() 
{
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    sis->provisional(183);
    earlyAnswerSent = true;
    setCallState(DialInProgress);
}

void B2BCall::doDialEarlyMediaProxyFail() 
{
    setClearingReason(AuthError, -1);
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    sis->reject(500);  // FIXME - error code
    setCallState(CallStop);
}

void B2BCall::onOfferRequired(MyAppDialog *myAppDialog) // t offer c: new add client, send offer to aleg
{
    if( (myAppDialog != aFirstLegAppDialog) && (myAppDialog != bLegAppDialog) && ( CallActive == callState ) )
    {
	ServerInviteSession *sis = (ServerInviteSession *)(myAppDialog->getInviteSession().get());
	SdpContents& sdp = mediaManager->getBLegSdp();
        sis->provideOffer(sdp);
        sis->accept();
    }
}

void B2BCall::doCallOffered() //t offer c: send offer to aleg
{
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    try
    {
	SdpContents& sdp = mediaManager->getBLegSdp();
	sis->provideOffer(sdp); 
	sis->accept();
    }
    catch(...)
    {
	//setClearingReason(Error, -1);
	setClearingReason(AnsweredError, -1);
	setCallState(CallAcceptedMediaProxyFail);
	return;
    }
}

void B2BCall::doCallAnswered() //t offer c: c answer, send answer to bleg
{
    InviteSession *is = (InviteSession *)(bLegAppDialog->getInviteSession().get());
    SdpContents& sdp = mediaManager->getALegSdp( aFirstLegAppDialog->getDialogId().getCallId() );
    is->provideAnswer(sdp);

    time(&connectTime);
    callHandle->connect(&connectTime);
    setCallState(CallActive);
}

void B2BCall::doCallAccepted() 
{
    B2BUA_LOG_DEBUG( <<"doCallAccepted()");
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get()); 
    if(!earlyAnswerSent) 
    {
	try 
	{
	    SdpContents& sdp = mediaManager->getBLegSdp();
	    sis->provideAnswer(sdp); // FIXME - for re-INVITE
	} 
	catch(...) 
	{
	    //setClearingReason(Error, -1);
	    B2BUA_LOG_DEBUG( <<"doCallAccepted getBLegSdp exception!" );
	    setClearingReason(AnsweredError, -1);
	    setCallState(CallAcceptedMediaProxyFail);
	    return;
	}
    }
    setCallState(CallAcceptedMediaProxySuccess);
    doCallAcceptedMediaProxySuccess();
}

void B2BCall::doCallAcceptedMediaProxySuccess() 
{
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    sis->accept();
    time(&connectTime);
    callHandle->connect(&connectTime);
    setCallState(CallActive);
}

void B2BCall::doCallAcceptedMediaProxyFail() 
{
    setClearingReason(AnsweredError, -1);
    ServerInviteSession *sis = (ServerInviteSession *)(aFirstLegAppDialog->getInviteSession().get());
    sis->reject(500); // FIXME - error codes
    setCallState(CallStop);
}

void B2BCall::doCallActive() 
{
    if( callHandle->mustHangup() ) 
    {
	B2BUA_LOG_DEBUG( <<"ending a call due to mustHangup()");
	setClearingReason(AnsweredLimit, -1);
	setCallState(LocalHangup);
    }
}

void B2BCall::doHangup(MyAppDialog *myAppDialog) //handle 1 aleg say goodbye
{
    //ServerInviteSession *sis = (ServerInviteSession *)(myAppDialog->getInviteSession().get());
    //sis->end();

    mediaManager->erase( myAppDialog->getDialogId().getCallId() ); //delete alegproxy
}

void B2BCall::doHangup() 
{
    setCallState(CallStop);
}

void B2BCall::doCallStop() //it will invoked is only bleg or last aleg cancel or say goodbye or rejected by bleg 
{
//  setClearingReason(AnsweredUnknown, -1);
    time(&finishTime);
    if(callHandle != NULL) 
    {
	if(connectTime != 0)
	    callHandle->finish(&finishTime);
	else
	    callHandle->fail(&finishTime);
    }
    
//    if(aLegAppDialog != NULL) 
    std::map<resip::Data, MyAppDialog*>::iterator i = aLegAppDialogs.begin();
    for ( ; i!=aLegAppDialogs.end(); ++i ) //say goodbye to every aleg
    {
	(i->second)->setB2BCall(NULL);

	ServerInviteSession *sis = (ServerInviteSession *)((i->second)->getInviteSession().get());
	B2BUA_LOG_DEBUG( <<"B2BCall::doCallStop() end ServerInviteSession callid " <<i->first );	
	sis->end(); 
    } 

    if ( bLegAppDialogSet != NULL ) 
    {
	B2BUA_LOG_DEBUG( <<"B2BCall::doCallStop() end bLegAppDialogSet "  );
	bLegAppDialogSet->end();
	bLegAppDialogSet->setB2BCall(NULL);
    }

    if ( bLegAppDialog != NULL )
    {
	bLegAppDialog->setB2BCall(NULL);
    }

//    writeCDR();

    setCallState(CallStopFinal);
}

void B2BCall::doCallStopMediaProxySuccess() 
{
    setCallState(CallStopFinal);
}

void B2BCall::doCallStopMediaProxyFail() 
{
    setCallState(CallStopFinal);
}

void B2BCall::doCallStopFinal() 
{
    // do nothing, the call should be deleted shortly
    if(callHandle != NULL) 
    {
	delete callHandle;
	callHandle = NULL;
    }
}

void B2BCall::setClearingReason(FullClearingReason reason, int code) 
{
    // has a reason already been specified?  If so, leave it alone
    if(fullClearingReason != Unset) 
	return; 

    fullClearingReason = reason;
    if(fullClearingReason == RejectOther)
	rejectOtherCode = code;
    switch(fullClearingReason) {
    case InvalidDestination:
    case AuthError:
	basicClearingReason = Error;
	break;
    case NoAnswerCancel:
    case NoAnswerTimeout:
	basicClearingReason = NoAnswer;
	break;
    case NoAnswerError:
	basicClearingReason = Error;
	break;
    case RejectBusy:
	basicClearingReason = Busy;
	break;
    case RejectOther:
	switch(rejectOtherCode) 
	{
	    /* case 503:  // ambiguous, could also indicate out of funds with a carrier
	       basicClearingReason = Congestion;
	       break; */
	default:
	    basicClearingReason = Error;
	    break;
	};
	break;
    case AnsweredALegHangup:
    case AnsweredBLegHangup:
    case AnsweredLimit:
    case AnsweredShutdown:
    case AnsweredError:
    case AnsweredUnknown:
	basicClearingReason = Answered;
	break;
    default:
	basicClearingReason = Error;
	break;
    };
}

void B2BCall::setClearingReasonMediaFail() 
{
    if(connectTime == 0)
	setClearingReason(NoAnswerError, -1);
    else
	setClearingReason(AnsweredError, -1);
}

void B2BCall::writeCDR() 
{
    std::ostringstream cdrStream;
    cdrStream << sourceAddr << ",";
    cdrStream << destinationAddr << ",";
    cdrStream << contextId << ",";
    cdrStream << '"' << basicClearingReasonName[basicClearingReason] << '"' << ",";
    cdrStream << fullClearingReason << ",";
    cdrStream << rejectOtherCode << ",";
    cdrStream << startTime << ",";
    if(connectTime == 0)
	cdrStream << ",";
    else
	cdrStream << connectTime << ",";
    cdrStream << finishTime << ",";
    cdrStream << finishTime - startTime << ",";
    if(connectTime != 0)
	cdrStream << finishTime - connectTime;
    cdrStream << ",";
    cdrStream << appRef1 << "," << appRef2 << ",";
    cdrHandler.handleRecord(cdrStream.str());
}

/* void B2BCall::onTrying() {
  setCallState(DialReceived100);
} */

void B2BCall::onRinging() 
{
    //callState = CALL_DIAL_180;
    setCallState(DialReceived180);
}

//void B2BCall::onSessionProgress() {
  //callState = CALL_DIAL_183;
  //setCallState(DialReceived183);
//}

void B2BCall::onEarlyMedia(MyAppDialog *myAppDialog, const SdpContents& sdp, const in_addr_t& msgSourceAddress) 
{
    setCallState(DialReceivedEarlyAnswer);
    try 
    {
	setBLegSdp(myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress); 
    }
    catch (...) 
    {
	// FIXME
	setCallState(DialEarlyMediaProxyFail);
	B2BUA_LOG_WARNING( <<"onEarlyMedia: exception while calling setBLegSdp");
	setClearingReason(NoAnswerError, -1);
	//setCallState(CallStop);
	return;
    }
    //ServerInviteSession *sis = (ServerInviteSession *)(aLegAppDialog->getInviteSession().get());
    //sis->provideOffer(sdp);
    //sis->provisional(183);
}

void B2BCall::onCancel( MyAppDialog *myAppDialog ) 
{
    //callState = CALL_CANCEL;
    assert( myAppDialog!=bLegAppDialog );
    if ( 1 == aLegAppDialogs.size() )
    {
	setCallState(CallerCancel);
    }
    else
    {
	ServerInviteSession *sis = (ServerInviteSession *)(myAppDialog->getInviteSession().get());
	sis->end();
    }
}

// Dial Failure due to ICMP, Timeout, protocol error or other reason
void B2BCall::onFailure(MyAppDialog *myAppDialog) 
{
    
  // If we are about to try another route, bLegAppDialogSet will be NULL
    if(bLegAppDialogSet == NULL) 
	return;
    
    // No need to do anything if it's already been rejected
    if(callState == DialRejected)
	return;
    
    if(setCallState(DialFailed)) 
    {
    }
}

// Dial failure due to rejection by remote party
void B2BCall::onRejected(int statusCode, const Data& reason) 
{
    if(setCallState(DialRejected)) 
    {
	failureStatusCode = statusCode;
	failureReason = new Data(reason);
    }
}


//now, below function only support c offer t answer c , don't support c nooffer t offer c answer t
void B2BCall::onOffer(MyAppDialog *myAppDialog, const SdpContents& sdp, const in_addr_t& msgSourceAddress) 
{
    InviteSession *otherInviteSession = NULL;	// used to relay SDP to other party
    SdpContents *otherSdp = NULL;

    if( myAppDialog == aFirstLegAppDialog ) //c offer t
    {
	B2BUA_LOG_DEBUG( << "received SDP offer from A leg");
	try 
	{
	    setALegSdp( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress );//to vtdu create
	}
	catch (...) 
	{
	    // FIXME - 
	    setClearingReasonMediaFail();
	    B2BUA_LOG_WARNING( <<"onOffer: exception while calling setALegSdp");
	    setCallState(CallStop);
	    return;
	}

	if ( (bLegAppDialog != NULL) && (1==aLegAppDialogs.size()) ) //this indicate that transtype is C2T
	{
	    otherInviteSession = (InviteSession *)(bLegAppDialog->getInviteSession().get());
	    otherSdp = (SdpContents *)mediaManager->getALegSdp( aFirstLegAppDialog->getDialogId().getCallId() ).clone();
	}
    } 
    else if(myAppDialog == bLegAppDialog) //t offer c
    {
	B2BUA_LOG_DEBUG( << "received SDP offer from B leg");
	try 
	{
	    setBLegSdp( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress); //to vtdu create
	    doCallOffered();//send 200 with offer to a
            //send ack to b. NTD, now state is DialInProgress
	} 
	catch (...) 
	{
	    // FIXME
	    setClearingReasonMediaFail();
	    B2BUA_LOG_WARNING( <<"onOffer: exception while calling setBLegSdp");
	    setCallState(CallStop);
	    return;
	}
	
	if ( !aLegAppDialogs.empty() ) 
	{
	    //otherInviteSession = (InviteSession *)(aLegAppDialog->getInviteSession().get());
	    //should get all aleg invitesessions, send reOffer
	    otherSdp = (SdpContents *)mediaManager->getBLegSdp().clone();
	}
    } 
    else //other aleg appdialog. new add client
    {
	//B2BUA_LOG_ERR( <<"onOffer: unrecognised myAppDialog");
	//throw new exception;
	if(callState == CallActive)
	{
	    B2BUA_LOG_DEBUG( << "received SDP offer from A leg");
	    try 
	    {
		setALegSdp( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress );//to vtdu append

		ServerInviteSession *sis = (ServerInviteSession *)(myAppDialog->getInviteSession().get()); 
		SdpContents& sdp = mediaManager->getBLegSdp();
		sis->provideAnswer(sdp); 
		sis->accept(); //other aleg should provide answer right now.
	    }
	    catch (...) 
	    {
		return;
	    }
	}
    }

    if(callState == CallActive) 
    {
	// Must be a re-INVITE, relay to other party
	// FIXME - change state
	B2BUA_LOG_DEBUG( <<"processing a re-INVITE");
	if( ( otherInviteSession != NULL) && (otherSdp != NULL) )
	{
	    otherInviteSession->provideOffer(*otherSdp);//resend a offer provided by peer 
	}
    }

    if(otherSdp != NULL)
	delete otherSdp;
}

void B2BCall::onAnswer(MyAppDialog *myAppDialog, const SdpContents& sdp, const in_addr_t& msgSourceAddress) 
{
    mediaManager->setToTag(bLegAppDialog->getDialogId().getLocalTag());

    if(callState != CallActive) //answer by bleg
    {
	if ( myAppDialog == bLegAppDialog )
	{
	    B2BUA_LOG_DEBUG( <<"received answer");
	    // Answer to original INVITE
	    //callState = CALL_ANSWERED;
	    setCallState(CallAccepted);
	    time(&connectTime);
	    try 
	    {
		setBLegSdp( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress);//note: to vtdu confirm rather than create
	    } 
	    catch (...) 
	    {
		// FIXME - stop the call
		setClearingReasonMediaFail();
		B2BUA_LOG_WARNING( <<"onAnswer: exception while processing SDP");
		setCallState(CallStop);
		return;
	    }
	}
	else //if ( myAppDialog == aFirstLegAppDialog ) note: must be aFirstLegAppDialog
	{
	    setALegSdp( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress); //note: to vtdu confirm rather than create
	    doCallAnswered();
	}
    } 
    else 
    {
	// Answer to re-INVITE
	InviteSession *inviteSession = NULL;
	SdpContents *otherSdp = NULL;

//	if(myAppDialog == aLegAppDialog) 
	if(myAppDialog == bLegAppDialog)
	{
	    B2BUA_LOG_DEBUG( <<"answer received from B leg");
	    setBLegSdp( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress);

//	    inviteSession = (InviteSession *)(aLegAppDialog->getInviteSession().get());
	    //should get all alegs
	    otherSdp = (SdpContents *)mediaManager->getBLegSdp().clone();
	}
	else if (myAppDialog == aFirstLegAppDialog)
	{
	    B2BUA_LOG_DEBUG( <<"answer received from A leg");
	    setALegSdp ( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress );//!!!存疑:a的应答不应该再访问分发，而是修改sdp后直接发给前端
	    inviteSession = (InviteSession *)( bLegAppDialog->getInviteSession().get() );
	    otherSdp = (SdpContents *)mediaManager->getALegSdp( myAppDialog->getDialogId().getCallId() ).clone();
	}
	else
	{
	    //new add client, use vtdu
	    setALegSdp( myAppDialog->getDialogId().getCallId(), sdp, msgSourceAddress); //note: to vtdu append
	}

	if ( inviteSession != NULL )
	    inviteSession->provideAnswer(*otherSdp);

	if(otherSdp != NULL)
	    delete otherSdp;
    }
}

void B2BCall::onMediaTimeout() //!!!be careful
{
    B2BUA_LOG_NOTICE( <<"call hangup due to media timeout");
    if(connectTime == 0)
	setClearingReason(AnsweredNoMedia, -1);
    else
	setClearingReason(NoAnswerError, -1);
    time(&finishTime);
    setCallState(CallStop); 
}

void B2BCall::onHangup(MyAppDialog *myAppDialog) 
{
//    if(myAppDialog == aLegAppDialog) 
    if ( NULL == myAppDialog )
    {
	B2BUA_LOG_INFO( <<"Call is stopped by control");
	setCallState(CallStop);
    }
    else if ( myAppDialog == bLegAppDialog ) 
    {
	B2BUA_LOG_DEBUG( <<"call hung up by bleg");
	setClearingReason(AnsweredBLegHangup, -1);
	setCallState(CalleeHangup);
	time(&finishTime);
    }
    else if ( 1 == aLegAppDialogs.size() )//only 1 aleg
    {
	B2BUA_LOG_DEBUG( <<"call hung up by last aleg");
	setClearingReason(AnsweredALegHangup, -1);
	setCallState(CallerHangup);
	time(&finishTime);
    } 
    else if ( aLegAppDialogs.find( myAppDialog->getDialogId().getCallId() ) != aLegAppDialogs.end() )
    {
	B2BUA_LOG_DEBUG( <<"call hung up by 1 aleg");
	//doHangup(myAppDialog); Don't need it!!! because InviteSession.dispatchBye will send 200OK resp
    }
    else 
    {
	// possible termination of B leg after route fail
	B2BUA_LOG_WARNING( <<"B2BCall::onHangup(): unrecognised MyAppDialog");
    }
    //time(&finishTime);
    //setCallState(CallStop);
}

void B2BCall::onStopping() 
{
    // FIXME - should any states be handled differently?
    switch(callState) 
    {
    case CallerCancel:
    case AuthorizationFail:
    case MediaProxyFail:
    case DialEarlyMediaProxyFail:
    case CallAcceptedMediaProxyFail:
    case CallerHangup:
    case CalleeHangup:
    case LocalHangup:
    case CallStop:
    case CallStopMediaProxySuccess:
    case CallStopMediaProxyFail:
    case CallStopFinal:
	return;
    default:
	onHangup(NULL);
    }
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

