//	CNetHTTP.h

#ifndef _H_CNetHTTP
#define	_H_CNetHTTP

#include "CCFData.h"

#ifndef __ppc__
#include <CFNetwork/CFHTTPMessage.h>
#endif

#if !defined(_KJAMS_) && defined(_CFTEST_)
	#define	_CFTEST_ONLY_
#endif

#if defined(_CFTEST_ONLY_) && (_JUST_CFTEST_ || _PaddleServer_)
	#include "CFTestUtils.h"
#endif

#ifdef _KJAMS_
#include "CApp.h"
#include "CAS_URL.h"
#endif

//	error types
#define	kCFStreamEventTimedOut					(CFStreamEventType)32
#define	kCFStreamUserCanceled					(CFStreamEventType)64

//	ports
#define kCFURLDefaultPortHTTP					(UInt32)80
#define kCFURLSecurePortHTTP					(UInt32)443

//	status codes
#define kCFURLStatusCode_NONE					0
#define kCFURLStatusCode_SUCCESS				200
#define kCFURLStatusCode_REDIRECT				300
#define kCFURLStatusCode_REDIRECT_FOUND			302
#define kCFURLStatusCode_BAD_REQUEST			400
#define kCFURLStatusCode_FORBIDDEN				403
#define kCFURLStatusCode_NOT_FOUND				404
#define kCFURLStatusCode_REQUEST_TOO_LARGE		413
#define kCFURLStatusCode_SSL_CERT_ERR			495	//	SSL Certificate Error
#define kCFURLStatusCode_ERR_ALREADY_SET		500	//	actually "internal server error"
#define kCFURLStatusCode_ERR_UNKNOWN			506
#define kCFURLStatusCode_ERR_UNKNOWN2			507

//	access methods
#define kCFURLAccessMethod_POST					CFSTR("POST")
#define kCFURLAccessMethod_GET					CFSTR("GET")
#define kCFURLAccessMethod_HEAD					CFSTR("HEAD")

//	general HTTP header defines
#define kCFURLAccessHeaderKey_Category			CFSTR("Category")
#define kCFURLAccessHeaderKey_Charset			CFSTR("Charset")
#define kCFURLAccessHeaderKey_UserEmail			CFSTR("User-Email")
#define kCFURLAccessHeaderKey_SetCookie			CFSTR("Set-Cookie")
#define kCFURLAccessHeaderKey_GetCookie			CFSTR("Cookie")
#define kCFURLAccessHeaderKey_UserAgent			CFSTR("User-Agent")
#define kCFURLAccessHeaderKey_ContentType		CFSTR("Content-Type")
#define kCFURLAccessHeaderKey_ContentDisposition CFSTR("Content-Disposition")
#define kCFURLAccessHeaderKey_Connection		CFSTR("Connection")
#define kCFURLAccessHeaderKey_ContentLength		CFSTR("Content-Length")
#define kCFURLAccessHeaderKey_LastModified		CFSTR("Last-Modified")
#define kCFURLAccessHeaderKey_Date				CFSTR("Date")
#define kCFURLAccessHeaderKey_RedirectLocation	CFSTR("Location")
#define kCFURLAccessHeaderKey_TansferEncoding	CFSTR("Transfer-Encoding")
#define kCFURLAccessHeaderKey_ContentEncoding	CFSTR("Content-Encoding")
#define kCFURLAccessHeaderKey_StatusCode		CFSTR("Status-Code")
#define kCFURLAccessHeaderKey_AllowOrign		CFSTR("Access-Control-Allow-Origin")

#define	kCFURLAccessHeaderValue_UTF8			CFSTR("UTF-8")
#define	kCFURLAccessHeaderValue_Close			CFSTR("close")
#define	kCFURLAccessHeaderValue_Chunked			CFSTR("chunked")
#define	kCFURLAccessHeaderValue_GZIP			CFSTR("gzip")

#define	kCFURLContentType_Text_HTML				CFSTR("text/html")
#define	kCFURLContentType_Text_XML				CFSTR("text/xml")
#define	kCFURLContentType_Text_JS				CFSTR("text/javascript")
#define	kCFURLContentType_Text_CSS				CFSTR("text/css")

#define	kCFURLContentType_Image_PNG				CFSTR("image/x-png")
#define	kCFURLContentType_Image_JPG				CFSTR("image/jpeg")
#define	kCFURLContentType_Image_GIF				CFSTR("image/gif")
#define	kCFURLContentType_Image_ICO				CFSTR("image/vnd.microsoft.icon")

#define kCFURLContentType_APP_URL				CFSTR("application/x-www-form-urlencoded")
#define kCFURLContentType_APP_JSON				CFSTR("application/json")

#define	kURL_this_CNetHTTP		"this_CNetHTTP"
#define	kURL_url				"url"
#define	kURL_url_alt			"url_alt"
#define	kURL_destFold			"dest_folder"
#define	kURL_fileName			"file_name"
#define	kURL_tetherFilePath		"tether_file_path"
#define	kURL_completion			"completion"
#define	kURL_thread_queP		"thread_queP"
#define	kURL_body				"body"
#define	kURL_encrypt			"crypt"
#define	kURL_encryption_salt	"crypt_salt"
#define	kURL_port				"port"
#define	kURL_escape				"escape"
#define	kURL_headers			"headers"
#define	kURL_use_leftovers		"use_leftovers"
#define	kURL_verb1				"verb1"
#define	kURL_verb2				"verb2"
#define	kURL_min_increment		"min_increment"
#define	kURL_preview_vid		"video preview only"	//	zeroes after 30 seconds
#define	kURL_song_id			"sample song id"
#define	kURL_resultStr			"resultStr"
#define	kURL_ignore_size		"ignore_size"
#define	kURL_form_urlencode		"form_urlencoded"
#define	kURL_urlencode			"urlencode"

#define	kHTML_DocType			"<!DOCTYPE HTML"

#define	kNetworkEvents						\
	kCFStreamEventOpenCompleted				\
    | kCFStreamEventHasBytesAvailable		\
    | kCFStreamEventEndEncountered			\
    | kCFStreamEventErrorOccurred

#if OPT_MACOS
	#define		kPlatformStr	"mac"
	#define		kDeviceStr		"Macintosh"
#else
	#define		kPlatformStr	"win"
	#define		kDeviceStr		"PC"
#endif

/************************************************/
float			Pref_GetTimeout_Client();
bool			IsLocalHostURL(const SuperString& in_urlStr, bool *gotBP0 = NULL, SuperString *out_pathStrP0 = NULL);

/************************************************/
class	CMutex_UCharQue : public CMutex {
	protected:
	UCharQue		i_que;
	
	public:	
	size_t			size();
	bool			empty()	{	return size() == 0; }
	void			clear();
	void			push_back(UTF8Char *dataP, size_t sizeL);
	void			pull_front(UTF8Char *dataP, size_t sizeL);
};

/************************************************/
class	CNetHTTP;

#if !defined(_ROSE_)
class	CNet_Completion : public CS_MainThread_AbortProc {
	protected:
	CMutex		i_mutex;
	CNetHTTP	*i_netP;
	bool		i_haveFileB;

	#ifdef _KJAMS_
		FSRef		i_fileRef;
	#endif

	bool		i_ignore_leftoverB;
	
	/****************************************/
	public:	
	CNet_Completion(CNetHTTP *netP);
	
	CCFDictionary&		GetHeaderDict();

	void	SetParam(CNetHTTP *netP) {
		i_netP = netP;
	}
	
	void	SetParam(bool ignore_leftoverB) {
		i_ignore_leftoverB = ignore_leftoverB;;
	}
	
	#ifdef _KJAMS_
	void	SetParam(const FSRef &fileRef) {
		i_fileRef = fileRef;
		i_haveFileB = true;
	}
	#endif
	
	virtual void	net_operator() { }
	virtual void	operator()();
};
#endif

/************************************************/

class CCFHeaderDict : public CCFDictionary {
	typedef CCFDictionary	_inherited;
	public:
	bool			ContainsKey(CFStringRef keyRef);
	CFTypeRef		GetValue(CFStringRef key);

};

class CNetHTTP_ForEach_HeaderKey {
	CFHTTPMessageRef	i_messageRef;
	
	public:
	CNetHTTP_ForEach_HeaderKey(CFHTTPMessageRef messageRef) : i_messageRef(messageRef) {}
	
	void	operator()(CFStringRef keyRef, CFTypeRef valRef) {
		CFHTTPMessageSetHeaderFieldValue(i_messageRef, keyRef, (CFStringRef)valRef);
	}
};

/************************************************/
#if _PaddleServer_ || _JUST_CFTEST_ || defined(_ROSE_)
typedef short ScNetThread;
#endif

class CNetHTTP {
	SuperString			i_url;
	ScNetThread			i_sc;
	
	friend class CNet_Completion;
	friend class CNet_Timer_StreamTimedOut;
	#define				kNetBufferSize		8191   //  Create an 8K buffer

	bool				i_got_headerB;
	APP_ForkRef			i_timeOutRef;
	UInt8				i_buffer[kNetBufferSize + 1]; 

	static	OSStatus	CB_S_DownloadToFolder(void *dataP, OSStatus err);
	OSStatus			DownloadToFolder(CCFDictionary& dict);
	void				realloc();

	public:
	CCFHeaderDict		i_incoming_headerDict;
	CPW_TaskRec			*i_taskP;
	CFStreamEventType	i_terminateEventType;
	CMutex_long			i_terminateErr;
	UInt32				i_statusCode;
	bool				i_ignore_sizeB;
	bool				i_logB;
	CMutex_bool			i_doneB;
	CSemaphore			i_dataSemaphore;
	size_t				i_download_manual_sizeL;
	CMutex_size_t		i_download_sizeL;
	CMutex_size_t		i_downloaded_sizeL;
	CMutex_UCharQue		i_dataQue;
	SuperString			i_next_accessMethodStr;
	SuperString			i_cur_accessMethodStr;
	bool				i_use_leftoverB;	//	padding only used for CAS_URL
	bool				i_completed_err_reportB;
	short				i_leftOverL;
	SuperString			i_verb1, i_verb2;
	
	CNetHTTP(CFStringRef accessMethodRef = kCFURLAccessMethod_GET);
	~CNetHTTP();
	
	void		SetLogging(bool logB);
	bool		IsLogging();

	void		DeleteTask();
	
	//	don't call this
	void		Kill();
	
	//	call this instead
	void		abort();
	
	SuperString	GetUrl() { return i_url; }
	void		SetAccessMethod(CFStringRef accessMethodRef) { i_next_accessMethodStr.Set(accessMethodRef); }
	SuperString	GetAccessMethod() { return i_next_accessMethodStr; }
		
	CT_Timer	*GetTimer();

	void 		RequestTermination(CT_Timer *timerP, CFStreamEventType eventType);
	void 		Completion(CFStreamEventType eventType);

	void		ReportErrors();
	void		CB_CNetHTTP_ReadStream(CFReadStreamRef stream, CFStreamEventType type);

	OSStatus	Download_UCharVec(CCFDictionary& dict, UCharVec *charVecP);
	OSStatus	Download_String(CCFDictionary& dict, SuperString *resultStrP);
	OSStatus	Download_String(const SuperString& urlStr, SuperString *resultStrP, bool ignore_sizeB = false);

	static SuperString		Download_String(const SuperString& urlStr, bool ignore_sizeB = false);

	OSStatus	Download_CFData(const SuperString& urlStr, CCFData *refP, bool ignore_sizeB = false);

	static SuperString		GetDate(const SuperString& urlStr, OSStatus *errP0 = NULL);
	static UInt64			GetSize(const SuperString& urlStr);
	
	static SuperString	 	UrlEncodedStr(CCFDictionary& dict, bool bodyB = false);
	static CFDictionaryRef	CopyUrlEncodeStrAsDict(const SuperString& in_str);
	static SStringMap		CopyUrlEncodeStrAsMap(const SuperString& in_str);

	OSErr 				SendMessage(CCFDictionary& dict);
	
	OSErr			SendMessage(
		const UTF8Char	*urlZ, 
		long			portL			= kCFURLDefaultPortHTTP, 
		CCFDictionary	*headerDictP	= NULL, 
		const UTF8Char	*bodyZ			= NULL, 
		bool			escapeB			= true);
	
	OSStatus	WaitForStart();
	OSStatus	WaitForEnd(
		const SuperString&	verb1 = SuperString(),
		const SuperString&	verb2 = SuperString());

	OSStatus	DownloadCompleted(OSStatus err);

#if defined(_KJAMS_)
	CAS_URL*		NewDownloadSpool(CCFDictionary& dict);
#endif

	APP_ForkRef		MT_DownloadToFolder(CCFDictionary& dict);
};

#ifdef _KJAMS_	//	cuz it requires CFileRef
class CDownloadToFolder {
	CNetHTTP			*i_netP;
	CCFDictionary		i_dict;
	void				Init(CFDictionaryRef dict);

	public:
	CDownloadToFolder(CFDictionaryRef dict)
	{ Init(dict); }

	CDownloadToFolder(
		const SuperString&	urlStr, 
		const CCFURL&		destFolderUrlStr,
		bool				ignoreSizeB = false);
	
	CFURLRef		GetDownloadedFile();

	virtual void	completion(OSStatus err, CNetHTTP *netP);
};

void			RemoteURL_GetEssence(
	const SuperString&	urlStr, 
	bool				escapeB, 
	CSong				*songP,
	UCharVec			*io_charVec);

class CNetHTTP_Disposer : public CT_Preemptive {
	CNetHTTP		*i_streamP;

public:
	CNetHTTP_Disposer(CNetHTTP *streamP) :
		i_streamP(streamP)
	{
		call("CNetHTTP disposer");
	}

	virtual	OSStatus	operator()(OSStatus err) {
		delete i_streamP;
		return err;
	}
};
#endif	//	_KJAMS_

#endif	//	_H_CNetHTTP
