//	CNetHTTP
#include "stdafx.h"
#include "CNetHTTP.h"

#ifndef __ppc__
	#include <CFNetwork/CFHTTPStream.h>
	#include <CFNetwork/CFSocketStream.h>
#endif

#if defined(_KJAMS_)
	#include "PrefKeys_p.h"
	#include "CWindow.h"
	#include "CTaskMgr.h"
	#include "CNetHTTPClient.h"
#endif

#define		kVeryLongDebugClientTimeout		0

/***************************************************************/
#define		kSlowNetworkSimulator			0

#define		kSlowness_HIGH					0.07
#define		kSlowness_MED					0.02
#define		kSlowness_LOW					0.005

#define		kSlowness_SETTING				kSlowness_LOW

/***************************************************************/
float		Pref_GetTimeout_Client()
{
	float		secondsF = 
		#if !defined(_KJAMS_) || (defined(kDEBUG) && kVeryLongDebugClientTimeout)
			1000;
		#else
			gApp->i_prefs->i_dict.GetAs_SInt32(kPrefKey_TIMEOUT_CLIENT);
		#endif
	
	return secondsF;
}

CFRunLoopRef	GetMainRunLoop()
{
	CFRunLoopRef		runLoopRef(NULL);
	
	#ifdef _KJAMS_
		if (CNetHTTPClient::IsMyThread()) {
			runLoopRef = CFRunLoopGetCurrent();
			
		} else {
			if (!CNetHTTPClient::Get()) {
				FatalAlert("CNetHTTPClient not initted!");
			}
			
			runLoopRef = CNetHTTPClient::Get()->GetRunLoop();
		}
	#else
		runLoopRef = CFRunLoopGetCurrent();
	#endif
	
	return runLoopRef;
}

/***************************************************************/
bool			CCFHeaderDict::ContainsKey(CFStringRef keyRef)
{
	bool		containsB = _inherited::ContainsKey(keyRef);
	
	if (!containsB) {
		SuperString		keyStr(keyRef);
		
		keyStr.ToLower();
		containsB = _inherited::ContainsKey(keyRef);
	}
	
	return containsB;
}

CFTypeRef		CCFHeaderDict::GetValue(CFStringRef keyRef)
{
	SuperString		keyStr(keyRef);
	
	if (!_inherited::ContainsKey(keyRef)) {
		keyStr.ToLower();
	}
	
	return _inherited::GetValue(keyStr.ref());
}

/****************************************************************************************/
size_t			CMutex_UCharQue::size()
{
	CCritical		sc(this, CRW_Read);
	
	return i_que.size();
}

void			CMutex_UCharQue::push_back(UTF8Char *dataP, size_t sizeL)
{
	CCritical		sc(this, CRW_Write);

	i_que.insert(i_que.end(), &dataP[0], &dataP[sizeL]);
}

void			CMutex_UCharQue::pull_front(UTF8Char *dataP, size_t sizeL)
{
	CCritical		sc(this, CRW_Read);
	
	std::copy(i_que.begin(), i_que.begin() + sizeL, dataP);
	i_que.erase(i_que.begin(), i_que.begin() + sizeL);
}

void			CMutex_UCharQue::clear()
{
	CCritical		sc(this, CRW_Write);

	i_que.clear();
}

/**********************************************************/
void		CNetHTTP::realloc()
{
	i_incoming_headerDict.realloc();
	i_url.clear();
	
	i_got_headerB			= false;
	i_terminateEventType	= kCFStreamEventTimedOut;
	i_terminateErr.Set(noErr);
	i_use_leftoverB			= false;
	i_completed_err_reportB = true;
	i_leftOverL				= -1;
	i_statusCode			= kCFURLStatusCode_NONE;
	i_downloaded_sizeL.Set(0);
}

CNetHTTP::CNetHTTP(CFStringRef accessMethodRef) :
	i_got_headerB(false),
	i_timeOutRef(0), 
	i_taskP(NULL),
	i_ignore_sizeB(false),
	i_logB(false),
	i_dataSemaphore("net data"),
	i_download_manual_sizeL(0),
	i_next_accessMethodStr(accessMethodRef)
{
	realloc();
	
	#ifdef _KJAMS_
	if (gApp) {
		i_logB = gApp->Logging(LogType_NETWORK);
	}
	#endif

	if (IsLogging()) {
		Log("initting HTTP Network");
	}
}

CNetHTTP::~CNetHTTP()
{
	DeleteTask();

	i_terminateErr.Set(userCanceledErr);
	i_doneB.Set(true);
	
	if (IsLogging()) {
		Logf("Deleting HTTP for: %s\n", i_url.utf8Z());
	}
	
	Kill();
	i_dataQue.clear();
	i_downloaded_sizeL.Set(0);
}

void		CNetHTTP::SetLogging(bool logB)
{
	i_logB = logB;
	
	#ifdef _KJAMS_
	if (gApp) {
		gApp->SetLogging(LogType_NETWORK, i_logB);
	}
	#endif
}

bool		CNetHTTP::IsLogging()
{
	return i_logB;
}

void		CNetHTTP::DeleteTask()
{
	#ifdef _KJAMS_
	if (i_taskP) {
		i_taskP->Delete();
		i_taskP = NULL;
	}
	#endif
}

void	CNetHTTP::Kill()
{
	if (i_timeOutRef) {
		#ifdef _KJAMS_
		{
			APP_ForkRef			forkRef = i_timeOutRef;
			CT_Timer			*timerP = GetTimer();
			
			Log("There was a live thread");
			if (timerP) timerP->operator()();
			gApp->i_threads->Kill(forkRef);
		}
		#endif
	} else {
		if (IsLogging()) {
			Log("There was no NET thread to kill");
		}
	}
}

/************************************************************************************************/
void		CNetHTTP::abort()
{
	RequestTermination(GetTimer(), kCFStreamUserCanceled);
}

static	void	TerminateStream(
	CFReadStreamRef&	streamRef,
	CFRunLoopRef		runLoopRef)
{
	if (streamRef) {
		//***  ALWAYS set the stream client (notifier) to NULL if you are releaseing it
		//  otherwise your notifier may be called after you released the stream leaving you with a 
		//  bogus stream within your notifier.
		
		//	refCount is 4 when stream is complete or 5 if a rez is still open (mavericks)
		//S_LogCount(streamRef, "streamRef: CFReadStreamSetClient");
		CFReadStreamSetClient(streamRef, 0, NULL, NULL);
		
		//	now we can unschedule, close, and finally release the stream
		//S_LogCount(streamRef, "streamRef: CFReadStreamUnscheduleFromRunLoop");
		CFReadStreamUnscheduleFromRunLoop(streamRef, runLoopRef, kCFRunLoopCommonModes);

		//S_LogCount(streamRef, "streamRef: CFReadStreamClose");
		CFReadStreamClose(streamRef);

		CFIndex		indexL = CFGetRetainCount(streamRef);
		//S_LogCount(streamRef, "streamRef: close") - 1;

		//	must be down to 0 (or 1 or 3? on HighSierra? if a rez is still open, will be closed ASAP)
		if (!(
			   indexL >= 0 
			&& indexL <= 3
		)) {
			SuperString		msg("WARNING: streamRef Retain Count: %d");
			
			msg.ssprintf(NULL, (int)indexL);
			PostAlert(msg.utf8Z());
		}

		CFReleaseDebug(streamRef);
		
		#ifdef _KJAMS_
		if (CNetHTTPClient::OnePerThread()) {
			//	i do not know why this needs to be done, i was under the impression
			//	that unscheduling the stream reader would be sufficient for the runloop
			//	to run out of things to do, and exit itself?
			CFRunLoopStop(runLoopRef);
		}
		#endif

		//streamRef = NULL;
	}
}

#ifndef _KJAMS_
	CFReadStreamRef		s_streamRef = NULL;
	CFRunLoopRef		s_runLoopRef = NULL;
#endif

void 	CNetHTTP::RequestTermination(CT_Timer *timerP, CFStreamEventType eventType)
{
	i_terminateEventType = eventType;

	if (eventType == kCFStreamEventErrorOccurred) {
		ReportErrors();
	}

	#ifdef _KJAMS_
		if (timerP == NULL) {
			//	CF_ASSERT(timerP);
			//	user can press cancel before the request has even been sent (no timer yet)

			//	or there could have been an error establishing the connection, so the 
			//	timer never got allocated at all
		} else {
			timerP->SetFireInterval(kEventDurationNoWait);
		}
	#else	//	_KJAMS_
		UNREFERENCED_PARAMETER(timerP);
		TerminateStream(s_streamRef, s_runLoopRef);
		Completion(i_terminateEventType);
	#endif	//	_KJAMS_
	
}

#ifdef _KJAMS_
/*
	We set up a one-shot timer to fire in Pref_GetTimeout_Client() seconds which will terminate 
	the download.  Every time we get some download activity in our notifier, we tickle the timer
*/
class CNet_Timer_StreamTimedOut : public CT_Timer {
	CNetHTTP			*i_netP;
	CFReadStreamRef		i_streamRef;
	CFRunLoopRef		i_runLoopRef;

	public:
	CNet_Timer_StreamTimedOut(
		EventTimerInterval	durationF, 
		CNetHTTP			*netP, 
		CFReadStreamRef		streamRef,
		CFRunLoopRef		runLoopRef
	) : 
		CT_Timer(NO_KILL "Stream Timeout", durationF),
		i_netP(netP), 
		i_streamRef(streamRef), 
		i_runLoopRef(runLoopRef)
	{
		call();
	}
	
	~CNet_Timer_StreamTimedOut() {	}
	
	OSStatus		operator()()
	{
		TerminateStream(i_streamRef, i_runLoopRef);
		i_netP->i_timeOutRef = 0;
		i_descZ = &i_descZ[strlen(NO_KILL)];
		
		return threadTimerTerminate;
	}

	OSStatus		CB_Completion() {
		i_netP->Completion(i_netP->i_terminateEventType);
		return noErr;
	}
};
#endif

CT_Timer		*CNetHTTP::GetTimer()
{
	CT_Timer	*timerP = NULL;
	
	#ifdef _KJAMS_
	if (gApp && gApp->i_threads) {
		timerP = (CT_Timer *)gApp->i_threads->Fork_GetTimer(i_timeOutRef);
	}
	#endif
	
	return timerP;
}

static void		CB_S_CNetHTTP_ReadStream(CFReadStreamRef incomingStreamRef, CFStreamEventType eventType, void *dataP)
{
	CNetHTTP		*netP = (CNetHTTP *)dataP;
	
	netP->CB_CNetHTTP_ReadStream(incomingStreamRef, eventType);
}

/************************************************/
void		CNetHTTP::CB_CNetHTTP_ReadStream(CFReadStreamRef incomingStreamRef, CFStreamEventType eventType)
{
	if (i_terminateEventType != kCFStreamEventTimedOut) {
		#ifdef kDEBUG
		//	int i = 0;
		#endif
		
		return;
	}

	CT_Timer				*timerP = GetTimer();
	bool					terminateB = 
	#ifdef _KJAMS_	
		gApp->IsShutdown();
	#else
		false;
	#endif
	
	bool					bytesAvailB = (eventType & kCFStreamEventHasBytesAvailable) != 0;
	bool					eventEndB = (eventType & kCFStreamEventEndEncountered) != 0;
	bool					eventErrorB = (eventType & kCFStreamEventErrorOccurred) != 0;

	#ifdef _KJAMS_
	if (timerP && !terminateB) {
//		timerP->prime();	??
		timerP->SetFireInterval(Pref_GetTimeout_Client());
	}
	#endif

	if (!(bytesAvailB || eventEndB || eventErrorB)) {
		if (IsLogging()) {
			Logf("no bytes, but not end. event type: %d\n", (int)eventType);
		}
	} else if (!terminateB) {
		
		//	try to get the header right up front:
		if (i_download_sizeL.Get() == 0) {
			ScCFReleaser<CFHTTPMessageRef>		msgRef((CFHTTPMessageRef)CFReadStreamCopyProperty(
				incomingStreamRef, kCFStreamPropertyHTTPResponseHeader));
			
			if (IsLogging()) {
				//Logf("msgRef: %s\n", msgRef.Get() ? "GOT it" : "NOTHING!");
			}

			if (eventErrorB) {
				CCFError			cfErr(CFReadStreamCopyError(incomingStreamRef));
				CCFDictionary		errDict(CFErrorCopyAsDict(cfErr));
				OSStatus			streamErr(errDict.GetAs_SInt32(SuperString(kCFErrorCodeKey).utf8Z()));
				
				if (streamErr && i_statusCode == kCFURLStatusCode_NONE) {

					if (IsLogging()) {
						CCFLog(true)(CFSTR("strange http error"));
						CCFLog(true)(errDict.ref());
						
						if (streamErr == kCFHostErrorUnknown) {
							OSStatus			addrErr(errDict.GetAs_SInt32(SuperString(kCFStreamErrorCodeKey).utf8Z()));
							
							//	from netdb.h
							#define	EAI_NONAME	 8	/* hostname nor servname provided, or not known */

							if (addrErr == EAI_NONAME) {
								//	could not reach host
								//	maybe provide a better statusCode here, like kCFURLStatusCode_NOT_FOUND
								//	but ensure windows gets here too on same error
							}
						}
					}
					
					i_statusCode = kCFURLStatusCode_ERR_ALREADY_SET;
					i_terminateErr.Set(streamErr);
				}
			}

			if (msgRef.Get()) {			
				if (i_statusCode == kCFURLStatusCode_NONE) {
					i_statusCode = (UInt32)CFHTTPMessageGetResponseStatusCode(msgRef);
					
					if (i_statusCode >= kCFURLStatusCode_BAD_REQUEST) {
						eventType = kCFStreamEventErrorOccurred;
						terminateB = true;
						eventErrorB = true;

						if (i_statusCode == kCFURLStatusCode_NOT_FOUND) {
							i_terminateErr.Set(kCFErrorHTTPBadURL);
						} else {
							i_terminateErr.Set(kNSpHostFailedErr);
						}
					}
				}

				if (i_incoming_headerDict.empty()) {
					i_incoming_headerDict.adopt(CFHTTPMessageCopyAllHeaderFields(msgRef));
				
					if (IsLogging()) {
						CCFLog(true)(CFSTR("no header yet: getting now"));
						CCFLog(true)(i_url.ref());
						CCFLog(true)(i_incoming_headerDict);
					}
				}

				if (!i_got_headerB && !eventErrorB) {
					
					if (IsLogging()) {
						CCFLog(true)(CFSTR("bytesAvailB && !i_got_headerB"));
					}
					
					i_got_headerB = true;
					
					SuperString			stringRef	= i_incoming_headerDict.GetAs_String(kCFURLAccessHeaderKey_ContentLength);
					size_t				sizeL		= stringRef.GetAs_SInt32();
					
					if (sizeL) {
						i_download_sizeL.Set(sizeL);

					} else if (i_download_manual_sizeL) {
						i_download_sizeL.Set(i_download_manual_sizeL);
					}

					if (IsLogging()) {
						Logf("i_download_sizeL = %d\n", (int)i_download_sizeL);
					}
				}

				if (
					   !i_ignore_sizeB 
					&& bytesAvailB 
					&& i_download_sizeL.Get() == 0
					&& eventType != kCFStreamEventErrorOccurred
				) {
				
					if (IsLogging()) {
						CCFLog(true)(msgRef);
					}
					
					eventType = kCFStreamEventErrorOccurred;
					terminateB = true;
					
					if (i_statusCode != kCFURLStatusCode_FORBIDDEN) {
						Logf("$$$ Did not get download size in headers.  Status: %d\n", (int)i_statusCode);

						i_statusCode = kCFURLStatusCode_ERR_ALREADY_SET;
						i_terminateErr.Set(kBadArgLengthErr);
					}
				}
			}
		}

		if (!terminateB && bytesAvailB) {
			CFIndex			bytesRead = CFReadStreamRead(incomingStreamRef, &i_buffer[0], kNetBufferSize);
			
			// If zero bytes were read, wait for the EOF to come.
			if (bytesRead > 0) {
				i_dataQue.push_back(&i_buffer[0], bytesRead);
				i_downloaded_sizeL.inc(bytesRead);
				
				if (IsLogging()) {
					if (i_url.get_ext() == '.xml' && bytesRead < 1024) {
						i_buffer[bytesRead] = 0;
						Log((const char *)&i_buffer[0]);
					}
				}
				
				i_dataSemaphore.signal();
			} else if (bytesRead < 0 ) {
				// Less than zero is an error
				//  0 assume we are done with the stream
				eventType = kCFStreamEventErrorOccurred;
			}
		}
		
		#ifdef kDEBUG
		#if kSlowNetworkSimulator
			MPDelay(kSlowness_SETTING);
			//IdleDuration(kSlowness_SETTING, kDurationForever_EventLoop_OnlyCallbacks);
		#endif
		#endif
	}

	if (eventEndB) {
		if (IsLogging()) {
			Log("finished reading stream");
		}
		
		terminateB = true;
	}

	if (eventType & kCFStreamEventErrorOccurred) {
		if (IsLogging()) {
			SuperString		str("%s: %lu");
			
			str.ssprintf(NULL, "kCFStreamEventErrorOccurred", 0);
			CCFLog(true)(str.ref());
		}
		
		terminateB = true;
	}

	if (terminateB) {
		RequestTermination(timerP, eventType);
	}
}

void	CNetHTTP::ReportErrors()
{
	switch (i_terminateEventType) {
		
		case kCFStreamEventTimedOut: {
			PostAlert(SSLocalize("Download session timed out", "").utf8Z());
			break;
		}
		
		case kCFStreamEventErrorOccurred: {
			SuperString			errStr;
			
			/*
				right here: CNetHTTP::Completion() actually does some error
				tranlsation, why don't factor and we use that here?
				big testing hit however
			*/
			
			if (i_dataQue.size() == 1) {
				#ifdef kDEBUG
					//	if you hit this assert, turn it into a real error
					//	by setting i_terminateErr at the source
					CF_ASSERT(0);
					i_terminateErr.Set(kNSpHostFailedErr);
				#endif
			}

			switch (i_terminateErr.Get()) {

				case kBadArgLengthErr: {
					errStr = SSLocalize("kJams was unable to get size of download.", "error string");
				} break;

				case kCFErrorHTTPBadURL:
				case kNSpHostFailedErr:
				case notEnoughDataErr: {
					#if !_PaddleServer_ && !_JUST_CFTEST_
						errStr = MacErrToStr(i_terminateErr.Get());
					#endif
				} break;
			}
			
			//	here we ONLY report on the above 3 types of errors
			//	other errors are handled / reported elsewhere

			if (!errStr.empty()) {
				SuperString		prefixStr(SSLocalize("Error", "prepended on a message to indicate failure state"));
			
				prefixStr.append(": ");
				
				errStr.prepend(prefixStr);
				errStr.append(". (");
				
				if (i_verb1.empty()) {
					errStr.append(GetUrl());
				} else {
					errStr.append(i_verb1);
					
					if (!i_verb2.empty()) {
						errStr.append(", ");
						errStr.append(i_verb2);
					}
				}

				errStr.append(")");

				PostAlert(errStr.utf8Z());
				i_terminateErr.Set(ERR_Already_Reported);
			}
			break;
		}
	}
}

void 	CNetHTTP::Completion(CFStreamEventType eventType)
{
	if (IsLogging()) {
		Log("net completion called");
	}

	switch (eventType) {

		case kCFStreamEventEndEncountered: {
		
			if (i_use_leftoverB) {
				size_t		sizeL = i_dataQue.size();
				short		leftOverL = sizeL % kSampleSize;
				
				if (leftOverL) {
					UInt8		buffer[kSampleSize];	structclr(buffer);
					
					leftOverL = kSampleSize - leftOverL;
					
					//	pad the buffer to a multiple of kSampleSize due to CAS_URL::cas_read
					i_dataQue.push_back(&buffer[0], leftOverL);
					
					//	don't include the leftovers in the downloded size
					//	cuz we did NOT download those bytes
					//	i_downloaded_sizeL.inc(leftOverL);
					
					if (!i_ignore_sizeB && i_leftOverL > 0) {
						if (i_leftOverL != leftOverL) {
							Logf("i thought i'd have %d left over, but i really have %d left over\n", (int)i_leftOverL, (int)leftOverL);
							CFDebugBreak();
							i_leftOverL = leftOverL;
						}
					} else {
						i_leftOverL = leftOverL;
					}
				}
			}
			
			i_download_manual_sizeL = 0;
			break;
		}

		case kCFStreamEventErrorOccurred: {
			
			switch (i_statusCode) {
			
				case kCFURLStatusCode_SUCCESS: {
					Log("WTF error is not an error");
					i_terminateErr.Set(noErr);
					break;
				}
					
				case kCFURLStatusCode_BAD_REQUEST: {
					i_terminateErr.Set(kNSpHostFailedErr);
					break;
				}
					
				case kCFURLStatusCode_NOT_FOUND: {
					i_terminateErr.Set(kNSpHostFailedErr);
					break;
				}
					
				case kCFURLStatusCode_FORBIDDEN: {
					i_terminateErr.Set(kNSpHostFailedErr);
					break;
				}
				
				case kCFURLStatusCode_REDIRECT_FOUND: {
					i_terminateErr.Set(urlDataHHTTPRedirectErr);
					break;
				}
				
				case kCFURLStatusCode_ERR_ALREADY_SET: {
					//	terminate error already set
					break;
				}
				
				default: {
					Logf("Strange web status code: %d\n", (int)i_statusCode);
					i_terminateErr.Set(networkErr);
					break;
				}
			}

			break;
		}

		case kCFStreamEventTimedOut: {
			i_terminateErr.Set(kNSpTimeoutErr);
			break;
		}

		case kCFStreamUserCanceled: {
			i_terminateErr.Set(userCanceledErr);
			break;
		}
	}

	i_doneB.Set(true);
	i_dataSemaphore.signal();
}

bool			IsLocalHostURL(const SuperString& in_urlStr, bool *gotBP0, SuperString *out_pathStrP0)
{
	bool	localB = in_urlStr.Contains("/localhost/");
	
	if (gotBP0) {
		*gotBP0 = false;
	}
	
	#ifdef _KJAMS_
	if (localB) {
		CFileRef					file(in_urlStr);	file.ExpandTilde(true);
		SuperString					urlStr(file.path());
		CCFURL						urlRef(CFURLCreateWithString(kCFAllocatorDefault, urlStr.ref(), NULL));

		if (urlRef.Get()) {
			SuperString		pathStr(CFURLCopyFileSystemPath(urlRef, kCFURLPlatformPathStyle));

			if (!pathStr.empty()) {
				CFileRef		cFile(pathStr);

				if (cFile.Exists()) {

					if (gotBP0) {
						*gotBP0 = true;
					}

					if (out_pathStrP0) {
						*out_pathStrP0 = pathStr;
					}
				}
			}
		}
	}
	#else
		UNREFERENCED_PARAMETER(out_pathStrP0);
	#endif //	_KJAMS_
	
	return localB;
}

#ifndef DOT_JSON
	#define DOT_JSON ".json"
#endif

SuperString		CNetHTTP::GetDate(const SuperString& urlStr, OSStatus *errP0)
{
	SuperString		dateStr;
	OSStatus		err(noErr);

	XTE_START {
		bool			gotB;
		SuperString		pathStr;

		if (IsLocalHostURL(urlStr, &gotB, &pathStr)) {
			if (gotB) {
				CFAbsoluteTime	absT(0);

				#ifdef _KJAMS
				{
					CFileRef		fileRef(pathStr);

					ETX(FSrGetFileDates(fileRef.ref(), &absT));
				}
				#endif

				dateStr.Set(absT, SS_Time_SHORT);
			}
		} else if (urlStr.Contains(DOT_JSON)) {
			CCFDictionary	dict(Download_String(urlStr), true);
			CFAbsoluteTime	absT(dict.GetAs_AbsTime(kCFURLAccessHeaderKey_LastModified));

			dateStr.Set(absT, SS_Time_SHORT);
		} else {
			CNetHTTP		net(kCFURLAccessMethod_HEAD);

			ETX(net.SendMessage(urlStr.utf8().c_str()));
			ETX(net.WaitForEnd());

			if (net.i_incoming_headerDict.Get()) {
				dateStr.Set(net.i_incoming_headerDict.GetAs_String(kCFURLAccessHeaderKey_LastModified));

				if (dateStr.empty()) {
					Log("$$$ Last-Modified is empty!  using date instead (which means current date stamp usually, ie: it's always out of date!)");
					dateStr.Set(net.i_incoming_headerDict.GetAs_String(kCFURLAccessHeaderKey_Date));
				}
			}

			//	round trip to convert to local time
			dateStr.Set(dateStr.GetAs_CFAbsoluteTime());
		}
	} XTE_END;

	if (err) {
		if (errP0) {
			*errP0 = err;
		} else {
			ETX(err);
		}
	}

	return dateStr;
}

UInt64			CNetHTTP::GetSize(const SuperString& urlStr)
{
	UInt64			sizeL = 0;
	bool			gotB = false;
	SuperString		pathStr;
	
	if (IsLocalHostURL(urlStr, &gotB, &pathStr)) {
		if (gotB) {
			#ifdef _KJAMS
			{
				CFileRef		fileRef(pathStr);

				ETX(FSrGetEOF(fileRef.ref(), &sizeL));
			}
			#endif
		}
	} else {
		CNetHTTP		net(kCFURLAccessMethod_HEAD);
		OSStatus		err = noErr;
		
		ERR(net.SendMessage(urlStr.utf8().c_str()));
		ERR(net.WaitForEnd());
		
		if (!err && net.i_incoming_headerDict.Get()) {
			SuperString		lenStr(net.i_incoming_headerDict.GetAs_String(kCFURLAccessHeaderKey_ContentLength));

			sizeL = (UInt64)lenStr.GetAs_Double();
		}
	}

	return sizeL;
}

/****************************************************************************************************/
class CNetHTTP_ForEach_FormUrlEncoded {
	SuperString&	i_str;
	bool			i_bodyB;
	
	public:
	CNetHTTP_ForEach_FormUrlEncoded(bool bodyB, SuperString *strP) : i_str(*strP), i_bodyB(bodyB) {}
	
	void	operator()(CFStringRef keyRef, CFTypeRef valRef) {
		
		if (i_str.empty()) {
			if (!i_bodyB) {
				i_str.append("?");
			}
		} else {
			i_str.append("&");
		}

		i_str.append(keyRef);
		i_str.append("=");
		
		CFTypeEnum		typeEnum(CFGetCFType(valRef));

		if (typeEnum == CFType_NUMBER_INT) {
			i_str.append(CFNumberToLong((CFNumberRef)valRef));
			
		} else {
			CF_ASSERT(typeEnum == CFType_STRING);
			i_str.append((CFStringRef)valRef);
		}
	}
};

//	static
SuperString 	CNetHTTP::UrlEncodedStr(CCFDictionary& dict, bool bodyB)
{
	SuperString							urlStr;
	CNetHTTP_ForEach_FormUrlEncoded		CFMessageRef_SetFormURLEncoded(bodyB, &urlStr);

	dict.for_each(CFMessageRef_SetFormURLEncoded);
	return urlStr;
}

/******************************************************/
//	static
CFDictionaryRef		CNetHTTP::CopyUrlEncodeStrAsDict(const SuperString& in_str)
{
	CCFDictionary		dict;
	
	if (!in_str.empty()) {
		SuperString		str(in_str);
		bool			doneB(false);
		
		if (str.GetIndChar() == '?') {
			str.pop_front();
		}
		
		do {
			SuperString		keyStr, valStr;

			if (str.Contains("&")) {
				str.rSplit("&", &keyStr);
				
			} else {
				doneB = true;
				keyStr = str;
				str.clear();
			}
			
			keyStr.Split("=", &valStr);
			
			if (valStr.Contains("%")) {
				valStr = SuperString(valStr.utf8Z(), kCFStringEncodingPercentEscapes);
			}
			
			valStr.Replace("+", " ");
			dict.SetValue(keyStr.ref(), valStr.ref());
		} while (!doneB);
	}
	
	return dict.transfer();
}

SStringMap		CNetHTTP::CopyUrlEncodeStrAsMap(const SuperString& in_str)
{
	CCFDictionary		dict(CopyUrlEncodeStrAsDict(in_str));
	SStringMap			map(DictToStringMap(dict));
	
	return map;
}

/******************************************************/
OSErr 			CNetHTTP::SendMessage(CCFDictionary& dict)
{
	OSErr			err				= noErr;
	SuperString		urlStr(dict.GetAs_String(kURL_url));
	long			portL			= kCFURLDefaultPortHTTP;
	bool			escapeB			= true;
	SuperString		bodyStr;
	const UTF8Char	*bodyZ			= NULL;
	CCFDictionary	headerDict, *headerDictP = NULL;
	
	if (dict.ContainsKey(kURL_escape)) {
		escapeB = dict.GetAs_Bool(kURL_escape);
	}
	
	if (dict.ContainsKey(kURL_port)) {
		portL = dict.GetAs_UInt32(kURL_port);
	}

	if (dict.ContainsKey(kURL_body)) {
		bodyStr = dict.GetAs_String(kURL_body);
		CF_ASSERT(!dict.ContainsKey(kURL_form_urlencode));
		
	} else if (dict.ContainsKey(kURL_form_urlencode)) {
		
		{
			CCFDictionary			form_urlEncoded((CFMutableDictionaryRef)dict.GetAs_Dict(kURL_form_urlencode), true);

			bodyStr.append(UrlEncodedStr(form_urlEncoded, true));
		}
		
		bodyStr.Escape();

		{
			CCFDictionary	tempHeaderDict;
			
			if (dict.ContainsKey(kURL_headers)) {
				tempHeaderDict.SetAndRetain((CFMutableDictionaryRef)dict.GetAs_Dict(kURL_headers));
			}
			
			tempHeaderDict.SetValue(
				kCFURLAccessHeaderKey_ContentType, 
				kCFURLContentType_APP_URL);
				
			dict.SetValue(kURL_headers, tempHeaderDict.Get());
		}
	}
	
	if (!bodyStr.empty()) {
		SetAccessMethod(kCFURLAccessMethod_POST);
		bodyZ = bodyStr.utf8().c_str();
	}
	
	if (dict.ContainsKey(kURL_headers)) {
		headerDict.SetAndRetain((CFMutableDictionaryRef)dict.GetAs_Dict(kURL_headers));
		headerDictP = &headerDict;
	}

	if (i_verb1.empty()) {
		i_verb1 = dict.GetAs_String(kURL_verb1);
	}
	
	if (i_verb2.empty()) {
		i_verb2 = dict.GetAs_String(kURL_verb2);
	}
	
	if (dict.ContainsKey(kURL_urlencode)) {
		CCFDictionary		urlEncodeDict(dict.GetAs_Dict(kURL_urlencode), true);
		
		urlStr.append(UrlEncodedStr(urlEncodeDict));
	}

	ERR(SendMessage(urlStr.utf8().c_str(), portL, headerDictP, bodyZ, escapeB));

	if (dict.ContainsKey(kURL_use_leftovers)) {
		i_use_leftoverB	= true;
	}

	return err;
}

OSErr 	CNetHTTP::SendMessage(
	const UTF8Char	*urlZ, 
	long			portL, 
	CCFDictionary	*headerDictP, 
	const UTF8Char	*bodyZ,
	bool			escapeB)
{
	OSErr		err = noErr;
	
	UNREFERENCED_PARAMETER(portL);

	realloc();
	
	i_cur_accessMethodStr = GetAccessMethod();

	#if defined(_KJAMS_)
	if (!CNetHTTPClient::IsMyThread() && CNetHTTPClient::OnePerThread()) {
		CCFDictionary		dict;
		
		dict.SetValue(kURL_this_CNetHTTP, (Ptr)this);
		dict.SetValue(kURL_url, (const char *)urlZ);
		dict.SetValue(kURL_port, portL);
		
		if (headerDictP) {
			dict.SetValue(kURL_headers, headerDictP->ref());
		}
		
		dict.SetValue(kURL_body, (const char *)bodyZ);
		dict.SetValue(kURL_escape, escapeB);
		
		CNetHTTPClient(&err, &dict);
		return err;
	} else 
	#endif
	{
		i_url.Set(urlZ);
		
		DeleteTask();

		#ifdef _KJAMS_
			if (i_verb1.empty()) {
				i_verb1 = i_url;
			}
			
			i_taskP = gApp->NewTask(i_verb1.ref(), i_verb2.ref());
		#endif
					
		if (escapeB) {
			i_url.Escape();
		}
		
		CCFURL							urlRef(i_url.CopyAs_URLRef(false));
		
		if (urlRef.Get() == NULL) {
			Logf("Bad URL: <<%s>>\n", i_url.utf8Z());
			ETRL(urlDataHHTTPURLErr, "CopyAs_URLRef");
		}
		
		ScCFReleaser<CFHTTPMessageRef>	messageRef(CFHTTPMessageCreateRequest(
			kCFAllocatorDefault, i_cur_accessMethodStr.ref(), urlRef, kCFHTTPVersion1_1));
		
		ETRL(messageRef.Get() == NULL, "create request");
	
		{
			CNetHTTP_ForEach_HeaderKey		CFMessageRef_SetHeader(messageRef);

			if (headerDictP) {
				headerDictP->for_each(CFMessageRef_SetHeader);
			}

			CFMessageRef_SetHeader(kCFURLAccessHeaderKey_Connection, kCFURLAccessHeaderValue_Close);
		}
		
		if (bodyZ) {
			ScCFReleaser<CFDataRef>		dataRef(SuperString(bodyZ).CopyDataRef());

			ETRL(dataRef.Get() == NULL, "dataref");
			CFHTTPMessageSetBody(messageRef, dataRef);
		}

		if (IsLogging()) {
			CCFLog(true)(messageRef);
			
			CCFString					schemeRef(CFURLCopyScheme(urlRef)); 
			CCFString					netLocRef(CFURLCopyHostName(urlRef)); 
			CCFData						msgRef(CFHTTPMessageCopySerializedMessage(messageRef));
			SuperString					stringRef; stringRef.Set(msgRef.Get());
			SuperString					str(schemeRef);
			
			str.append("://");
			str.append(netLocRef);
			str.append(" ");
			str.append(stringRef);
			
			CCFLog(true)(str.ref());
		}

		// Create the stream for the request.
		ScCFReleaser<CFReadStreamRef>	readStreamRef(CFReadStreamCreateForHTTPRequest(
			kCFAllocatorDefault, messageRef));		ETRL(readStreamRef.Get() == NULL, "readstreamref");

		(void)CFReadStreamSetProperty(
			readStreamRef, kCFStreamPropertyHTTPShouldAutoredirect, kCFBooleanTrue); 

		{
	//		ScCFReleaser<CFNumberRef>		numRef(CFNumberCreateWithNumber(portL));
			
	//		(void)CFReadStreamSetProperty(
	//			readStreamRef, kCFStreamPropertyHTTPProxyPort, numRef); 
		}

		CFRunLoopRef	runLoopRef(GetMainRunLoop());

		#ifdef _KJAMS_
		{
			CNet_Timer_StreamTimedOut	*timerP = new CNet_Timer_StreamTimedOut(
				Pref_GetTimeout_Client(), this, readStreamRef, runLoopRef);
			
			i_timeOutRef = timerP->GetForkRef();
		}
		#else
			s_streamRef		= readStreamRef;
			s_runLoopRef	= runLoopRef;
		#endif

		// Set the client notifier
		CFStreamClientContext		contextRec = { 0, this, NULL, NULL, NULL };

		if (!CFReadStreamSetClient(
			readStreamRef, kNetworkEvents, CB_S_CNetHTTP_ReadStream, &contextRec)
		) {
			ERR(urlDataHHTTPURLErr);
		}

		if (!err) {
			// Schedule the stream
			CFReadStreamScheduleWithRunLoop(
				readStreamRef, 
				runLoopRef, 
				kCFRunLoopCommonModes);

			i_doneB.Set(false);
			i_download_sizeL.Set(0);
			i_dataQue.clear();
			i_downloaded_sizeL.Set(0);
			
			//	reset the semaphore
			i_dataSemaphore.signal();
			i_dataSemaphore.wait();

			// Start the HTTP connection
			
			if (!CFReadStreamOpen(readStreamRef)) {
				ERR(urlDataHHTTPURLErr);
			}
		}
		
		if (err) {
			if (readStreamRef.Get()) {
				CFReadStreamSetClient(readStreamRef, 0, NULL, NULL);
				i_terminateErr.Set(err);
				i_doneB.Set(true);
			}
		} else {
			//	successful launching of the stream means we MUST keep the readStreamRef retained, 
			//	it will be released in the CNet_Timer_StreamTimedOut callback
			readStreamRef.transfer();	//	abandons call to Release()
		}
	}

	return err;
}

/******************************************************************/

OSStatus	CNetHTTP::WaitForStart()
{
	OSStatus	err = noErr;
	bool		doneB = false;
	
	do {
		i_dataSemaphore.wait();
		doneB = i_terminateErr.Get() || i_doneB.Get() || i_download_sizeL.Get() > 0;
	} while (!doneB);
	
	ERR((OSStatus)i_terminateErr.Get());
	return err;
}

OSStatus		CNetHTTP::DownloadCompleted(OSStatus in_err)
{
	OSStatus		err = noErr;
	bool			user_canceledB(in_err == userCanceledErr);
	bool			net_errorB(i_terminateEventType == kCFStreamEventErrorOccurred);
	bool			only_requesting_headB(i_cur_accessMethodStr == kCFURLAccessMethod_HEAD);
	
	if (1
		&& !user_canceledB
		&& !net_errorB
		&& !only_requesting_headB
		&& (!i_ignore_sizeB || i_download_sizeL.Get() != 0)
	) {
		size_t			dl_size_expectedL(i_download_sizeL.Get());
		size_t			dl_size_actualL(i_downloaded_sizeL.Get());
		
		if (dl_size_expectedL != dl_size_actualL) {
			i_terminateEventType = kCFStreamEventErrorOccurred;
			i_terminateErr.Set(err = notEnoughDataErr);

			if (i_completed_err_reportB) {
				ReportErrors();
			}
		}
	}
	
	return err;
}

OSStatus	CNetHTTP::WaitForEnd(
	const SuperString&	verb1,
	const SuperString&	verb2)
{
	OSStatus	err = noErr;
	
	UNREFERENCED_PARAMETER(verb2);

	ERR(WaitForStart());
	
	if (!err) {
	
		if (!verb1.empty()) {
			#ifdef _KJAMS_
			{
				DeleteTask();
				ScTask_SelfContained		scTask(verb1, verb2, i_download_sizeL.Get());

				scTask.Inc();

				while (!err && !i_doneB.Get()) {
					i_dataSemaphore.wait();
					ERR(scTask.Inc(i_dataQue.size()));
					ERR((OSStatus)i_terminateErr.Get());
					
					if (err) {
						//	not necessary?  causes crash due to no mutex
						//abort();
					}
				}
			}
			#endif
		} else {
			
			while (!i_doneB.Get()) {
				i_dataSemaphore.wait();
			}
			
			ERR((OSStatus)i_terminateErr.Get());
		}
		
		ERR(DownloadCompleted(noErr));
	}
	
	return err;
}

/******************************************************************/

OSStatus	CNetHTTP::Download_UCharVec(
	CCFDictionary& dict, 
	UCharVec *charVecP)
{
	OSStatus		err = noErr;

	XTE_START {
		SuperString		verb1(dict.GetAs_String(kURL_verb1));
		SuperString		verb2(dict.GetAs_String(kURL_verb2));
		bool			ignore_lenB(dict.GetAs_Bool(kURL_ignore_size));
		
		if (ignore_lenB) {
			i_ignore_sizeB = true;
		}
		
		#if 0 // kDEBUG
		//	cause a task to be shown for this
		if (verb1.empty()) {
			verb1 = "something";
		}
		#endif
		
		ERR(SendMessage(dict));
		ERR(WaitForEnd(verb1, verb2));
		
		if (i_dataQue.empty()) {
			ERR(kOTNoDataErr);
		}
		
		if (!err) {
			UCharVec&		charVec(*charVecP);
			
			charVec.resize(i_dataQue.size());
			i_dataQue.pull_front(&charVec[0], charVec.size());
		}
	} XTE_END;
	
	return err;
}

OSStatus	CNetHTTP::Download_String(CCFDictionary& dict, SuperString *resultStrP)
{
	OSStatus	err = noErr;
	UCharVec	charVec;

	resultStrP->clear();
	
	ERR(Download_UCharVec(dict, &charVec));
	if (!err) {
		charVec.push_back(0);
		resultStrP->Set(&charVec[0]);
	}
	
	return err;
}

OSStatus	CNetHTTP::Download_String(const SuperString& urlStr, SuperString *resultStrP, bool ignore_sizeB)
{
	CCFDictionary		dict;

	dict.SetValue(kURL_url, urlStr);
	
	if (ignore_sizeB) {
		dict.SetValue(kURL_ignore_size, ignore_sizeB);
	}
	
	return Download_String(dict, resultStrP);
}

//	static
SuperString		CNetHTTP::Download_String(const SuperString& urlStr, bool ignore_sizeB)
{
	SuperString		resultStr;
	CNetHTTP		net(kCFURLAccessMethod_GET);

	ETX(net.Download_String(urlStr, &resultStr, ignore_sizeB));
	return resultStr;
}


OSStatus	CNetHTTP::Download_CFData(const SuperString& urlStr, CCFData *refP, bool ignore_sizeB)
{
	OSStatus			err = noErr;
	UCharVec			charVec;
	CCFDictionary		dict;
	
	refP->clear();
	dict.SetValue(kURL_url, urlStr);
	
	if (ignore_sizeB) {
		dict.SetValue(kURL_ignore_size, ignore_sizeB);
	}

	#if 0
	if (urlStr.StartsWith(kCFURLProtocol_HTTPS)) {
		dict.SetValue(kURL_port, kCFURLSecurePortHTTP);
	}
	#endif
	
	ERR(Download_UCharVec(dict, &charVec));

	refP->clear();
	if (!err) {
		refP->Set(charVec);
	}
	
	return err;
}

/******************************************************************/
OSStatus		CNetHTTP::DownloadToFolder(CCFDictionary& dict)
{
	OSStatus		err					= noErr;

	#ifndef _KJAMS_
		UNREFERENCED_PARAMETER(dict);
	#else
	bool			complete_on_errB	= true;
	
	XTE_START {
		
		if (IsMainThread()) {
			CF_ASSERT(0);
			ETX(1);
		}
		
		SuperString			urlStr(dict.GetAs_String(kURL_url));
		const UTF8Char		*splitZ((const UTF8Char *)strrchr((const char *)urlStr.c_str(), '/'));
		
		if (splitZ == NULL) {
			err = urlDataHHTTPURLErr;
			
		} else {
			++splitZ;
			
			CFileRef			destFolder(dict.GetAs_String(kURL_destFold));
			SuperString			fileNameStr(dict.GetAs_String(kURL_fileName));
			
			if (fileNameStr.empty()) {
				fileNameStr.Set(splitZ);
				
				dict.SetValue(kURL_fileName, fileNameStr);
			}
			
			ETX(destFolder.GetChild(fileNameStr));

			{
				{
					i_verb1 = dict.GetAs_String(kURL_verb1);
					i_verb2 = dict.GetAs_String(kURL_verb2);
					
					if (i_verb1.empty()) {
						i_verb1 = SSLocalize("Downloading file:", "");
					}
					
					if (i_verb2.empty()) {
						i_verb2 = fileNameStr;
					}

					i_verb2.UnEscape();
				}
			
				CAS_URL				*urlP(NewDownloadSpool(dict));
				ScSpoolReleaser		sc1(urlP);
				CAS_File			*fileP(new CAS_File(i_verb1, i_verb2));
				ScSpoolReleaser		sc2(fileP);
				CNet_Completion		*completionP(dict.GetAs_PtrT<CNet_Completion *>(kURL_completion));
				
				fileP->SetParams(destFolder.ref());
				completionP->SetParam(destFolder.ref());
				fileP->SetCompletionProc(CAS_Write, completionP);

				Log("Download: Forking off process");
				
				complete_on_errB = false;
				ERR(urlP->Fork_Spool(fileP));
			}
		}
	} XTE_END;
	
	if (err && complete_on_errB) {
		CNet_Completion		*completionP(dict.GetAs_PtrT<CNet_Completion *>(kURL_completion));
		
		if (completionP) {
			completionP->SetErr(err);
			completionP->operator()();
		}
	}
	#endif	//	_KJAMS_
	
	return err;
}

//	static
OSStatus	CNetHTTP::CB_S_DownloadToFolder(
	void *dataP, OSStatus err)
{
	CCFDictionary		dict((CFDictionaryRef)dataP);
	CNetHTTP			*thiz(dict.GetAs_PtrT<CNetHTTP *>(kURL_this_CNetHTTP));

	UNREFERENCED_PARAMETER(err);

	//CCFLog(true)(dict);
	
	return thiz->DownloadToFolder(dict);
}

#ifdef _KJAMS_
class MT_ShowTasks : CMainThreadProc {
	public:
	
	#if !_QT_
	MT_ShowTasks() : CMainThreadProc("show tasks") {
		if (IsMainThread()) {
			operator()();
			delete this;
		} else {
			call();
		}
	}

	void	operator()() {
		XWindowRef		tasksP = gApp->GetWindow(PREF_Wind_TASKS);
		XWindowRef		videoP = gApp->GetWindow(PREF_Wind_GRAPHICS);
		
		if (tasksP && videoP) {
			if (!videoP->IsFullScreen()) {
				tasksP->SelectWindow();
			}
		}
	}
	#endif
};
#endif

APP_ForkRef		CNetHTTP::MT_DownloadToFolder(CCFDictionary& dict)
{
#ifdef _KJAMS_
	//new MT_ShowTasks();
	
	dict.SetValue(kURL_this_CNetHTTP, (Ptr)this);
	
	//CCFLog(true)(dict);
	
	return gApp->i_threads->Fork(
		APP_Thread_PREEMPTIVE, 
		CNetHTTP::CB_S_DownloadToFolder, NULL, 
		dict.Retain(), 
		"Download Launcher");
#else
	UNREFERENCED_PARAMETER(dict);
	return NULL;
#endif
}


CNet_Completion::CNet_Completion(CNetHTTP *netP) : 
	i_netP(netP),
	i_haveFileB(false),
	i_ignore_leftoverB(false)
{
}

CCFDictionary&		CNet_Completion::GetHeaderDict()
{
	return i_netP->i_incoming_headerDict;
}

void	CNet_Completion::operator()()
{
	#ifdef _KJAMS_
	OSStatus		err = noErr;
	
	if (!i_err && i_netP && i_netP->i_leftOverL > 0 && i_haveFileB && !i_ignore_leftoverB) {
		UInt64		eof;
		
		ERR(FSrGetEOF(i_fileRef, &eof));
		ERR(FSrSetEOF(i_fileRef, eof - i_netP->i_leftOverL));
	}

	//	can be called when there is no i_netP
	//	if it is short circuited before the net is set up
	if (i_netP) {
		ERR(i_netP->DownloadCompleted(i_err));
	}

	if (!i_err) i_err = err;

	IX(net_operator());

	if (i_err) {
		if (i_haveFileB) {
			ERR(FSrDeleteFile(&i_fileRef));
		}
	}
	
	if (i_netP) {
		CNetHTTP	*deleteMeP = i_netP;
		
		i_netP = NULL;
		delete deleteMeP;
	}
	#endif
}

// ***************************************************************************
#ifdef _KJAMS_	//	cuz it requires CFileRef
class		CDownloadToFolder_Completion : public CNet_Completion {
	typedef	CNet_Completion				_inherited;
	CFAutoPtr<CDownloadToFolder>	i_thiz;
	public:	

	CDownloadToFolder_Completion(CNetHTTP *netP, CDownloadToFolder *thiz) :
		_inherited(netP),
		i_thiz(thiz)
	{ }
	
	void	net_operator() {
	
		if (!i_err && !i_haveFileB) {
			i_err = fnfErr;
		}

		i_thiz->completion(i_err, i_netP);
	}
};

void			CDownloadToFolder::Init(CFDictionaryRef in_dict)
{
	i_dict.SetAndRetain((CFMutableDictionaryRef)in_dict);
	
	i_netP = new CNetHTTP();
	i_dict.SetValue(kURL_completion, (Ptr)new CDownloadToFolder_Completion(i_netP, this));
	i_netP->MT_DownloadToFolder(i_dict);
}

CDownloadToFolder::CDownloadToFolder(
	const SuperString&	urlStr, 
	const CCFURL&		destFolderUrlStr,
	bool				ignoreSizeB)
{
	CFileRef		tempFolder(destFolderUrlStr.ref());
	CCFDictionary	dict;

	dict.SetValue(kURL_url, urlStr);
	dict.SetValue(kURL_destFold, tempFolder.path());
	
	if (ignoreSizeB) {
		dict.SetValue(kURL_ignore_size, ignoreSizeB);
	}
	
	Init(dict);
}

CFURLRef		CDownloadToFolder::GetDownloadedFile()
{
	SuperString		destFold(i_dict.GetAs_String(kURL_destFold));
	SuperString		destFile(i_dict.GetAs_String(kURL_fileName));
	CFileRef		cFile(destFold);
	
	cFile.Descend(destFile);
	return cFile.CopyURL();
}

void	CDownloadToFolder::completion(OSStatus err, CNetHTTP *netP)
{
	if (!err) {

		if (
			   i_netP->i_statusCode != 0
			&& i_netP->i_statusCode != kCFURLStatusCode_SUCCESS
		) {
			SuperString			str(SSLocalize("Network error", ""));
			
			str.append(": ");
			str.append((long)i_netP->i_statusCode);
			PostAlert(str.utf8Z());
			err = i_netP->i_statusCode;
		}
	}

	#if defined(kDEBUG) && 0
	if (!err) {
		CFileRef		cFile(GetDownloadedFile());

		cFile.Reveal();
	}
	#endif
}

void			RemoteURL_GetEssence(
	const SuperString&	urlStr, 
	bool				escapeB, 
	CSong				*songP,
	UCharVec			*io_charVec)
{
	CCFDictionary		dict;
	
	CF_ASSERT(songP);

	dict.SetValue(kURL_url, urlStr);
	dict.SetValue(kURL_verb1, SSLocalize("Cacheing Audio stream:", "copying the data to local storage"));
	dict.SetValue(kURL_verb2, songP->GetName());
	dict.SetValue(kURL_escape, escapeB);
	
	CNetHTTP			*streamP = new CNetHTTP();
	
	BEX_TRY {
		{
			CCritical		sc(&songP->i_netP);
			
			ETX(songP->i_abortErr.Get());
			songP->i_netP.Set(streamP);
		}
		
		ETX(streamP->Download_UCharVec(dict, io_charVec));
	} BEX_CLEANUP {
	
		{
			CCritical		sc(&songP->i_netP);
			
			songP->i_netP.Set(NULL);
		}

		new CNetHTTP_Disposer(streamP);
	} BEX_ENDTRY;
}
#endif

// ***************************************************************************

OSStatus		DownloadTest()
{
	OSStatus		err = noErr;

#ifdef _KJAMS_
	CFileRef		tempFolder(CFileRef::kFolder_DESKTOP);
	
	#if 1
		new CDownloadToFolder(
			"https://karaoke.kjams.com/downloads/Python_Installer.msi", 
			tempFolder.CopyURL(),
			true);
	#else
		CNetHTTP		*netP = new CNetHTTP();
		SuperString		urlStr("http://api.karaokecloud.com/Library.svc/json/GetCatalogURL");
		SuperString		bodyStr("{ \"DeveloperKey\":\"509236fc-5e16-4740-b28a-76be0283f139\" }");
		CCFDictionary	dict;
		CCFDictionary	headerDict;
		
		//	kCFURLAccessHeaderKey_ContentLength will be set automatically

		headerDict.SetValue(kCFURLAccessHeaderKey_ContentType, kCFURLContentType_APP_JSON);

		dict.SetValue(kURL_url, urlStr);
		dict.SetValue(kURL_destFold, tempFolder.path());
		dict.SetValue(kURL_body, bodyStr);
		dict.SetValue(kURL_headers, headerDict.Get());
		dict.SetValue(kURL_completion, (Ptr)new CNet_Completion(netP));
		netP->MT_DownloadToFolder(dict);
	#endif
#endif //	_KJAMS_
	return err;
}



#if 0
static SuperString		CST_GetUserName()
{
	return "dave@kjams.com";
}

static SuperString		CST_GetPassword()
{
	return "asdf";
}
#endif

#ifdef _KJAMS_
//	ChillSecTran download completion
class CST_Completion : public CNet_Completion {
	typedef CNet_Completion	_inherited;

	public:
	CST_Completion(CNetHTTP *netP) : _inherited(netP) { }
	
	void	net_operator()
	{
		if (!i_err && i_haveFileB) {
			SuperString		fileNameStr = (GetHeaderDict().GetAs_String(
				kCFURLAccessHeaderKey_ContentDisposition));
					
			fileNameStr.pop_front(strlen("attachment; filename="));

			CFileRef(i_fileRef).Rename(fileNameStr);
			i_haveFileB = false;
		}
		
		_inherited::net_operator();
	}
};
#endif

void				TestCSTDownload()
{
#ifdef _KJAMS_
	SuperString		urlStr(kCFURLProtocol_HTTPS "/tracks.chillsectran.com/test2.php");
	CNetHTTP		*netP = new CNetHTTP();
	CCFDictionary	dict;
	
	dict.SetValue(kURL_url, urlStr);
	dict.SetValue(kURL_destFold, CFileRef(CFileRef::kFolder_DESKTOP).path());
	dict.SetValue(kURL_completion, (Ptr)new CST_Completion(netP));
	
	// WARNING: no "content-length" given in result header!!
	dict.SetValue(kURL_ignore_size, true);
	
	//	set body
	CCFDictionary	jsonBodyDict;
	{		
	//	jsonBodyDict.SetValue(CFSTR("agent"), CFSTR("kjams"));
	//	jsonBodyDict.SetValue(CFSTR("user"), CST_GetUserName());
	//	jsonBodyDict.SetValue(CFSTR("pass"), CST_GetPassword().md5());
		jsonBodyDict.SetValue(CFSTR("DISCID"), CFSTR("SC7515-01"));
		
		//	length will be set automatically when you set a body
		//	kCFURLAccessHeaderKey_ContentLength 
		
		SuperString		bodyStr(jsonBodyDict.GetJSON());

		dict.SetValue(kURL_body, bodyStr);
	}

	//	set headers
	{
		CCFDictionary	headerDict;
		
		headerDict.SetValue(kCFURLAccessHeaderKey_ContentType, kCFURLContentType_APP_JSON);
		dict.SetValue(kURL_headers, headerDict.Get());
	}

	{
		//	for the progress bar (no hooked up?!?)
	//	dict.SetValue(kURL_verb1, "ChillSecTran");
	//	dict.SetValue(kURL_verb2, "Some Crazy Song");
	}

	netP->MT_DownloadToFolder(dict);
#endif //	_KJAMS_
}

