#ifndef _H_CWebServer
#define	_H_CWebServer

#include <list>
#include <set>
#include <CoreFoundation/CFSocket.h>
#include <CoreFoundation/CFStream.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CFHTTPMessage.h>
#include "CCFData.h"

#if OPT_MACOS
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
//	typedef CFURLRef	CFHTTPMessageRef;
#endif

#define		kWebServerPort_NONE			((UInt16)-1)
#define		kWebServerPort_Default		12345
#define		kWebServerTimeout_Default	30
#define		kWebServerErrorString		"CWebServer Error: "

class CWebSession;

//typedefs
typedef std::set<CWebSession *> CWebSessionSet;
typedef std::map<SuperString, SuperString>	CookiesMap;
typedef std::map<SuperString, SuperString>	ReplaceMap;

#define		kWeb
class CWebServer {
	UInt16			i_portS;
	bool			i_bonjour_startedB;

	protected:
		CFSocketRef		i_socketRef;
		
		static void		CB_S_AcceptConnection(
			CFSocketRef				socketRef, 
			CFSocketCallBackType	callbackType, 
			CFDataRef				addressDataRef, 
			const void				*data, 
			void					*info);
	
	public:
		static	CWebServer			*s_serverP;
		CWebSessionSet				i_sessions;
		bool						i_runningB;
		
		CWebServer(UInt16 portS);
		~CWebServer(void);
		
		OSStatus			CreateSocketRef(CFSocketContext *socketContextP, UInt16 portS, CFSocketRef *socketRefP);
		
		void				Bonjour_Restart();
		static bool			Start(UInt16 portS = -1);
		static void			Stop(void);
		static bool			Restart(UInt16 portS = -1);
		static CWebServer*	Get(void);
};

class CWebClient {
	protected:
	CFSocketRef						i_socketRef;
	ScCFReleaser<CFHTTPMessageRef>	i_requestRef;
	CFDataRef						i_addressDataRef;
	CWebSession*					i_session;
	
	CookiesMap	i_serverCookies; //cookies to be sent by the server
	CookiesMap	i_clientCookies; //cookies sent by the browser
	
	CookiesMap	i_clientPostData; //postdata sent by the browser
	
	public:
	CWebClient(CFSocketNativeHandle	socketH, CFDataRef	addressRef);
	virtual ~CWebClient();
	
	void			CB_ClientCallBack(
		CFSocketRef				socketRef,
		CFSocketCallBackType	callbackType,
		CFDataRef				addressDataRef,
		CFDataRef				data
	);
	
	static void		CB_S_ClientCallBack(
		CFSocketRef				socketRef, 
		CFSocketCallBackType	callbackType, 
		CFDataRef				addressDataRef, 
		const void				*data, 
		void					*info
	);
	
	void	ServeRequest(CFHTTPMessageRef	request);
	
	void	SendData(CFDataRef dataRef);
	void	SendHTTPResponse(CFHTTPMessageRef responseRef);
	void	SendHTTPFile(const char* filePathZ, ReplaceMap* keywords = NULL);
	void	SendXMLError(const char* errId, const SuperString& description);
	
	void			SetCookie(const SuperString& name, const SuperString& value);
	void			SetCookie(const char*	name, const char*	value);
	SuperString		GetCookie(const SuperString&	name);
	SuperString		GetCookie(const char*	name);
	
	SuperString		GetPostData(const SuperString&	key);
	SuperString		GetPostData(const char*	key);

	void	Disconnect();

	OSStatus	CreateAndSendHTTPResponse(
		CFStringRef		contentType,
		CFIndex			responseCode,
		CFDataRef		responseBody
	);
	
	OSStatus	CreateAndSendHTTPResponse(
		CFStringRef		contentType,
		CFIndex			responseCode,
		const char*		strZ
	);
};

class CWebSession {
	protected:
	SuperString		i_singer;
	SuperString		i_sessionID;
	time_t			i_lastCheckin;
	
	public:
	SuperString		i_recentSearch;
	OSStatus		i_recentErr;
	CFAbsoluteTime	i_recentT;

	CWebSession();
	
	bool		Authenticate(const SuperString& singer, const SuperString& password);
	void		RevokeAuthentication();
	void		SetSinger(SuperString& singer);
	SuperString	GetSinger();
	SuperString	GetSessionID();
	void		Checkin();
	void		CB_Timer();
	static void	CB_S_Timer(CFRunLoopTimerRef timer, void* info);
};

#endif
