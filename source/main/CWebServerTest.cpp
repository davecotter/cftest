#include "stdafx.h"

#include <algorithm>
#include "CFTest.h"
#include "CFNetworkAddress.h"
#include "CNetHTTP.h"
#include "CWebServerTest.h"

CWebServer* CWebServer::s_serverP = NULL;

#define		kPostData_SINGER			"singer"
#define		kPostData_SINGER_NAME		"singername"
#define		kPostData_PASSWORD			"password"
#define		kPostData_PASSWORD_CONFIRM	"confirm"
#define		kPostData_SUBMIT			"submit"
#define		kPostData_PLAYLIST			"playlist"
#define		kPostData_ORDER_BY			"orderby"
#define		kPostData_SEARCH			"search"
#define		kPostData_PITCH				"pitch"
#define		kPostData_SONG				"song"
#define		kPostData_INDEX				"index"
#define		kPostData_INDEX_OLD			"oldIndex"
#define		kPostData_PLI				"piIx"
#define		kPostData_ERROR				"error"
#define		kCFNetTest					1

CWebServer::CWebServer(UInt16 portS) :
	i_portS(kWebServerPort_NONE),
	i_bonjour_startedB(false),
	i_socketRef(NULL),
	i_runningB(false)
{
	//Create a reference to this instance
	CFSocketContext		socketContext;	structclr(socketContext);
	socketContext.info = this;

	//Create socket
	ScCFReleaser<CFSocketRef>		socketRef;
	
	do {
		if (CreateSocketRef(&socketContext, portS, socketRef.AddressOf()) == userCanceledErr) {
			return;
		}
	} while (socketRef.Get() == NULL);

	if (socketRef.Get() == NULL) {
		Log("Web Server Not Started: Problem starting the kJams web server. Make sure the specified port isn't in use. Only port 80 or ports >= 1024 are valid.");
		return;
	}
	
	//Create run loop source
	ScCFReleaser<CFRunLoopSourceRef>		rlSourceRef(CFSocketCreateRunLoopSource(
		kCFAllocatorDefault, socketRef, 0));
	
	if (rlSourceRef.Get() == NULL) {
		Log(kWebServerErrorString, "Error creating run loop source. Server not started.");
		return;
	}
	
	//Add to run loop
	CFRunLoopAddSource(CFRunLoopGetCurrent(), rlSourceRef, kCFRunLoopCommonModes);

	i_socketRef	= socketRef.transfer();
	i_runningB	= true;
	i_portS		= portS;
	i_bonjour_startedB	= false;

	Bonjour_Restart();
	CCFLog(true)(CFSTR("Server started"));
}

OSStatus			CWebServer::CreateSocketRef(CFSocketContext *socketContextP, UInt16 portS, CFSocketRef *socketRefP)
{
	OSStatus						err = noErr;
	ScCFReleaser<CFSocketRef>		socketRef;
	
	{
		socketRef.adopt(CFSocketCreate(
			kCFAllocatorDefault, 
			PF_INET, SOCK_STREAM, IPPROTO_TCP, 
			kCFSocketAcceptCallBack, 
			CWebServer::CB_S_AcceptConnection, 
			socketContextP));
		
		ERR_NULL(socketRef.Get(), kENOTCONNErr);
		
		//Create address
		if (!err) {
			CFNetworkSockAddrUnion	sockAddr;
			
			CFNetworkInitSockAddress(&sockAddr, AF_INET, portS);
			
			CCFData					addrDataRef(
				(UInt8 *)&sockAddr, 
				CFNetworkGetSockAddressSize(&sockAddr));

			ERR((OSStatus)CFSocketSetAddress(socketRef, addrDataRef));
		}
	}

	if (!err) {
		*socketRefP = socketRef.transfer();
	}

	return err;
}


void		CWebServer::Bonjour_Restart()
{
/*
	if (i_bonjour_startedB) {
		CFBonjour_StopPublish();
	}

	SuperString		venueName(gApp->i_db->GetVenueName());
	
	venueName.Recover();
//	SuperString		machineName(CSCopyMachineName(), false);
	
	venueName.prepend("kJams: ");

	
	CFBonjour_Publish(CFSTR("_http._tcp"), venueName.ref(), i_portS);
	i_bonjour_startedB = true;
*/
}

class CDeleteSessions {
	public: void operator()(CWebSession *sessionP) {
		delete sessionP;
	}
};

CWebServer::~CWebServer() {

//	CFBonjour_StopPublish();
	i_bonjour_startedB = false;

	if (i_socketRef) {
		CFSocketInvalidate(i_socketRef);
		CFRelease(i_socketRef);
		i_socketRef		= NULL;
	}
	
	//Clean up sessions
	Logf("Cleaning up %d web session(s).\n", i_sessions.size());
	
	std::for_each(i_sessions.begin(), i_sessions.end(), CDeleteSessions());
	
	i_sessions.clear();
	
	s_serverP		= NULL;
	Log("Server stopped.");
}

bool		i_start_serverB = true;

bool	CWebServer::Start(UInt16 portS) {

	if (i_start_serverB) {
		CF_ASSERT(s_serverP == NULL);
		
		if (portS == kWebServerPort_NONE) {
			portS = kWebServerPort_Default;
		}
		
		s_serverP = new CWebServer(portS);
		return s_serverP->i_runningB;
	}
	
	return false;
}

void	CWebServer::Stop() {
	delete s_serverP;
	s_serverP = NULL;
}

//static 
bool			CWebServer::Restart(UInt16 portS)
{
	UNREFERENCED_PARAMETER(portS);

	if (s_serverP) {
		CWebServer::Stop();
	}

	bool	successB = CWebServer::Start();

	if (!successB) {
		CWebServer::Stop();
	}

	return successB;
}

CWebServer*	CWebServer::Get() {
	return s_serverP;
}

/*************************************************************************
class CWebClient_RunLoop : public CT_Preemptive {
	CFDataRef				i_addressDataRef;
	CFSocketNativeHandle	i_socketH;
	
	public:
	CWebClient_RunLoop(
		CFSocketNativeHandle	socketH,
		CFDataRef				addressDataRef
	) :
		i_socketH(socketH),
		i_addressDataRef(addressDataRef)
	{
		call("serve socket");
	}
							
	OSStatus	operator()(OSStatus err) {
		new CWebClient(i_socketH, i_addressDataRef);
		CFRunLoopRun();
		return noErr;
	}
};
*/
 
//	static
void	CWebServer::CB_S_AcceptConnection(
	CFSocketRef				socketRef, 
	CFSocketCallBackType	callbackType, 
	CFDataRef				addressDataRef, 
	const void				*data, 
	void					*info
) {
	UNREFERENCED_PARAMETER(socketRef);
	UNREFERENCED_PARAMETER(callbackType);
	UNREFERENCED_PARAMETER(addressDataRef);

	if (data == NULL) return;
	
	CWebServer				*thiz = (CWebServer *)info;
	CFSocketNativeHandle	socketH(*(CFSocketNativeHandle *)data);
	
	CF_ASSERT(thiz->i_socketRef == socketRef);

	#if 0
		new CWebClient_RunLoop(socketH, addressDataRef);
	#else
		new CWebClient(socketH, addressDataRef);
	#endif
}

CWebClient::CWebClient(
	CFSocketNativeHandle		socketH,	
	CFDataRef					addressDataRef
) {
	CFSocketContext socketContext;	structclr(socketContext);

	socketContext.info = this;

	ScCFReleaser<CFSocketRef>		socketRef(CFSocketCreateWithNative(
		kCFAllocatorDefault, socketH, kCFSocketDataCallBack, CWebClient::CB_S_ClientCallBack, &socketContext));
	
	if (socketRef.Get() == NULL) {
		Logf("%s Could not create client socket.", kWebServerErrorString);
		return;
	}

	//Create run loop source
	ScCFReleaser<CFRunLoopSourceRef>		rlSourceRef(CFSocketCreateRunLoopSource(
		kCFAllocatorDefault, socketRef, 0));
	
	if (rlSourceRef.Get() == NULL) {
		Logf("%s Problem creating client socket run loop.", kWebServerErrorString);
		return;
	}

	CFRunLoopAddSource(
		CFRunLoopGetCurrent(), 
		rlSourceRef, 
		kCFRunLoopCommonModes);

	i_socketRef			= socketRef;
	i_addressDataRef	= addressDataRef;
	
	#if kCFNetTest
		i_requestRef.adopt(CFHTTPMessageCreateEmpty(kCFAllocatorDefault, true));
	#endif
}

CWebClient::~CWebClient()
{
	//if (IsPreemptiveThread()) {
	//	CFRunLoopStop(CFRunLoopGetCurrent());
	//}
}

typedef enum {
	CWS_OrderBy_NONE = -1, 

	CWS_OrderBy_INDEX, 
	CWS_OrderBy_NAME, 
	CWS_OrderBy_ARTIST, 
	CWS_OrderBy_ALBUM, 
	CWS_OrderBy_PITCH, 

	CWS_OrderBy_NUMTYPES
} CWS_OrderByType;

typedef enum {
	CWS_Path_NONE, 

	CWS_Path_PING,
	CWS_Path_LOGIN,
	CWS_Path_LOGOUT, 
	CWS_Path_MAIN, 
	CWS_Path_HELP, 
	CWS_Path_NEW_SINGER,
	CWS_Path_SINGERS, 
	CWS_Path_PLAYLISTS, 
	CWS_Path_SONGS, 
	CWS_Path_SEARCH, 
	CWS_Path_DROP, 
	CWS_Path_REORDER, 
	CWS_Path_REMOVE,
	CWS_Path_PITCH, 
	CWS_Path_SCRIPT_MAIN,

	CWS_Path_NUMTYPES
} CWS_PathType;

typedef struct {
	CWS_PathType		indexS;
	const char			*strZ;
} CWS_StringTable;

CWS_StringTable		g_CWS_StringTable[] = {
	{	CWS_Path_NONE,				"nothing"				}, 
	{	CWS_Path_PING,				"ping"					},
	{	CWS_Path_LOGIN,				""						}, 
	{	CWS_Path_LOGOUT,			"logout"				}, 
	{	CWS_Path_MAIN,				"main"					}, 
	{	CWS_Path_HELP,				"help"					}, 
	{	CWS_Path_NEW_SINGER,		"newsinger"				},
	{	CWS_Path_SINGERS,			"singers"				}, 
	{	CWS_Path_PLAYLISTS,			"playlists"				}, 
	{	CWS_Path_SONGS,				"songs"					}, 
	{	CWS_Path_SEARCH,			"search"				}, 
	{	CWS_Path_DROP,				"drop"					}, 
	{	CWS_Path_REORDER,			"rearrange"				},
	{	CWS_Path_REMOVE,			"remove"				},
	{	CWS_Path_PITCH,				kPostData_PITCH			},
	{	CWS_Path_SCRIPT_MAIN,		"main.js"				},
};

#define	CWS_Str(_x)	g_CWS_StringTable[_x].strZ

static	CWS_PathType		CWS_KeyToType(const SuperString& str)
{
	typedef std::map<std::string, CWS_PathType>		CWS_KeyToTypeMapType;
	static	CWS_KeyToTypeMapType					s_match_map;
	static	bool									s_match_map_inittedB = false;
	
	if (!s_match_map_inittedB) {
		loop (CWS_Path_NUMTYPES)	s_match_map[CWS_Str(_indexS)]	= (CWS_PathType)_indexS;
		s_match_map_inittedB = true;
	}

	CWS_PathType					keyType	= CWS_Path_NONE;
	CWS_KeyToTypeMapType::iterator	it		= s_match_map.find(str.std());
	
	if (it != s_match_map.end()) {
		keyType = (*it).second;
	}
	
	return keyType;
}

void	CWebClient::ServeRequest(CFHTTPMessageRef requestRef) 
{
	#if kCFNetTest
		ScCFReleaser<CFURLRef>		urlRef(CFHTTPMessageCopyRequestURL(requestRef));
	#else
		ScCFReleaser<CFURLRef>		urlRef;
	#endif
	
	SuperString					path(CFURLCopyPath(urlRef), false);
	CWebSessionSet&				sessionSet(CWebServer::Get()->i_sessions);
	ReplaceMap					keywords;
	
	//Retrieve session
	if (GetCookie("sessionid") != "") {
		SuperString		session_idStr(GetCookie("sessionid"));
		CWebSession		*sessionP(reinterpret_cast<CWebSession *>(session_idStr.GetAs_Ptr()));

		//Make sure session exists
		if (sessionSet.find(sessionP) != sessionSet.end()) {
			i_session = sessionP;
		} else {
			SetCookie("sessionid", "");
		}
	}
	
	//Create new session
	if (GetCookie("sessionid").empty()) {
		SuperString			session_idStr(i_session->GetSessionID());
		CWebSession			*sessionP(reinterpret_cast<CWebSession *>(session_idStr.GetAs_Ptr()));
		
		i_session = new CWebSession();
		SetCookie("sessionid", session_idStr);
		sessionSet.insert(sessionP);
	}
	
	//Session checkin
	i_session->Checkin();
	
	path.pop_front();

	CF_ASSERT(i_session);	
	SuperString		singerStr = i_session->GetSinger();
	
	switch (CWS_KeyToType(path)) {
	
		case CWS_Path_PING: {
			i_session->Checkin();
			break;
		}

		case CWS_Path_LOGIN: {
			//Log out session
			i_session->RevokeAuthentication();
		
			SendHTTPFile("server/loginscreen.html");
			break;
		}
				
		case CWS_Path_LOGOUT: {
			SetCookie("sessionid", "");
			//Add code to clear session later
			SendHTTPFile("server/loginscreen.html");
			break;
		}
				
		case CWS_Path_MAIN: {
			
			if (!singerStr.empty()) {
				keywords[kPostData_SINGER] = singerStr;
				SendHTTPFile("server/mainscreen.html", &keywords);
				
			} else if (i_session->Authenticate(GetPostData(kPostData_SINGER), GetPostData(kPostData_PASSWORD))) {
				singerStr = i_session->GetSinger();
				keywords[kPostData_SINGER] = singerStr;
				SendHTTPFile("server/mainscreen.html", &keywords);
			}
			
			else {
				SendHTTPFile("server/loginscreen.html");
			}
			
			i_session->Checkin();
			break;
		}
				
		case CWS_Path_HELP: {
			SendHTTPFile("server/helpscreen.html");
			
			i_session->Checkin();
			break;
		}
		
				
		default: {
			keywords["auto_logout"]	= "false";
			keywords["timeout"]		= SuperString((long)kWebServerTimeout_Default);
		
			path.prepend("server/");
			SendHTTPFile(path.utf8Z(), &keywords);
			break;
		}
	}

	Disconnect();
}

//Various methods for sending data.
void	CWebClient::SendData(CFDataRef dataRef) {
	CFDataRef		addressDataRef = NULL;
	CFTimeInterval	timeOutT(kWebServerTimeout_Default);
	CFSocketError	result(CFSocketSendData(i_socketRef, addressDataRef, dataRef, timeOutT));
	
	if (result != kCFSocketSuccess) {
		LogErr("Sending HTTP response", (OSStatus)result);
	}
}

void	CWebClient::SendHTTPResponse(CFHTTPMessageRef responseRef)
{
	SuperString		cookie;
	
	Logf("SendHTTPResponse: adding [%d] cookies", (int)i_serverCookies.size());
	
	for (CookiesMap::iterator it(i_serverCookies.begin()); it != i_serverCookies.end(); ++it) {
		CookiesMap::value_type& pair(*it);
		
		cookie.Set(pair.first);
		cookie.append("=");
		cookie.append(pair.second);
		
		#if kCFNetTest
			CFHTTPMessageSetHeaderFieldValue(responseRef, kCFURLAccessHeaderKey_SetCookie, cookie.ref());
		#endif
	}
	
	Log("SendHTTPResponse: creating data ref");
	#if kCFNetTest
		CCFData		dataRef(CFHTTPMessageCopySerializedMessage(responseRef));
	#else
		CCFData		dataRef;
	#endif

	Log("SendHTTPResponse: sending data");
	SendData(dataRef);
	
	Log("SendHTTPResponse: data sent");
}

OSStatus	CWebClient::CreateAndSendHTTPResponse(
	CFStringRef		contentType,
	CFIndex			responseCode,
	CFDataRef		responseBody
) {
	OSStatus						err = noErr;
	
	#if kCFNetTest
		ScCFReleaser<CFHTTPMessageRef>	messageRef(CFHTTPMessageCreateResponse(
			kCFAllocatorDefault, responseCode, CFSTR("OK"), kCFHTTPVersion1_1));
	
		CFHTTPMessageSetHeaderFieldValue(messageRef, kCFURLAccessHeaderKey_ContentType, contentType);
	//	CFHTTPMessageSetHeaderFieldValue(messageRef, kCFURLAccessHeaderKey_Connection, kCFURLAccessHeaderValue_Close);
		CFHTTPMessageSetBody(messageRef, responseBody);
	#else
		ScCFReleaser<CFHTTPMessageRef>	messageRef;
	#endif
	
	//	this should return an error
	SendHTTPResponse(messageRef);
	return err;
}

OSStatus	CWebClient::CreateAndSendHTTPResponse(
	CFStringRef		contentType,
	CFIndex			responseCode,
	const char*		strZ
) {
	CCFData	dataRef(strZ);
	return CreateAndSendHTTPResponse(contentType, responseCode, dataRef);
}

bool	CFGetFileData(bool logB, const char* filePathZ, CCFData *dataRefP);
bool	CFGetFileData(bool logB, const char* filePathZ, CCFData *dataRefP)
{
	bool	goodB = false;
	SuperString						pathStr(filePathZ);
	ScCFReleaser<CFURLRef>			bundleUrlRef;
	ScCFReleaser<CFBundleRef>		bundleRef(CFBundleGetMainBundle(), true);
	
	if (bundleRef.Get() == NULL) {
		CCFLog()(CFSTR("$$ Failed getting bundle!\n"));	
	} else {
		bundleUrlRef.adopt(CFBundleCopyBundleURL(bundleRef));
	}
	
	CF_ASSERT(bundleUrlRef.Get() != NULL);
	
	if (logB) {
		CCFLog(true)(bundleUrlRef.Get());
	}
	
	SuperString				relPathStr(GetTestDataPath());
	
	pathStr.prepend(relPathStr);
	
	ScCFReleaser<CFURLRef>	resourceUrlRef(CFURLCreateWithFileSystemPathRelativeToBase(
		kCFAllocatorDefault, pathStr.ref(), kCFURLPOSIXPathStyle, false, bundleUrlRef));
		
	goodB = resourceUrlRef.Get() != NULL;
	
	if (goodB) {
		CCFDictionary						dictRef;
		SuperString							resultStr;
		ScCFReleaser<CFURLRef>				absUrlRef(CFURLCopyAbsoluteURL(resourceUrlRef));
		
		if (logB) {
			CCFLog()(resultStr.ssprintf("URL: %s\n", SuperString(CFURLGetString(absUrlRef)).utf8Z()).ref());
		}
		
		goodB = !!CFURLCreateDataAndPropertiesFromResource(
			kCFAllocatorDefault, absUrlRef, dataRefP->AddressOf(), NULL, NULL, NULL);

		//	to log what you get back
		if (goodB && logB) {
			SuperString		fileStr(*dataRefP);
			
			CCFLog(true)(fileStr.ref());
		}
	}
	
	return goodB;
}

void	CWebClient::SendHTTPFile(const char* filePathZ, ReplaceMap* keywords) {
	CCFData			dataRef;
	SuperString		pathStr(uc(filePathZ));
	OSType			extType(pathStr.get_ext());
	
	CF_ASSERT(pathStr.Contains("server/"));
	
	switch (extType) {
		case '.ico':
		case '.png':
		case '.jpg':
		case '.gif': {
			pathStr.rSplit("/", NULL, true);
			pathStr.prepend("server/media/");
			break;
		}
	}

	if (CFGetFileData(false, pathStr.utf8Z(), &dataRef)) {
		SuperString		contentType;
		
		switch (extType) {		
			case ' .js': {
				contentType.Set(kCFURLContentType_Text_JS);
				break;
			}

			case '.ico': {
				contentType.Set(kCFURLContentType_Image_ICO);
				break;
			}

			default:
			case 'html': {
				contentType.Set(kCFURLContentType_Text_HTML);
				break;
			}
			
			case '.xml': {
				contentType.Set(kCFURLContentType_Text_XML);
				break;
			}

			case '.css': {
				contentType.Set(kCFURLContentType_Text_CSS);
				break;
			}

			case '.png': {
				contentType.Set(kCFURLContentType_Image_PNG);
				break;
			}

			case '.jpg': {
				contentType.Set(kCFURLContentType_Image_JPG);
				break;
			}

			case '.gif': {
				contentType.Set(kCFURLContentType_Image_GIF);
				break;
			}
		}
		
		switch (extType) {
			case ' .js':
			case 'html':
			case '.xml':
			case '.css': {
				if (keywords != NULL) {
					SuperString		fileStr(dataRef);
					
					//contentType.Set(kCFURLContentType_Text_HTML);
					contentType.Set("text/html; charset=utf-8");
					
					//Iterate replacement keywords
					for (ReplaceMap::iterator it(keywords->begin()); it != keywords->end(); ++it) {
						ReplaceMap::value_type&		pair(*it);

						fileStr.Replace(SuperString("{") + pair.first + SuperString("}"), pair.second);
					}
					
					dataRef.Set(fileStr);
				}
			}
		}
		
		/*
		if (keywords != NULL) {
			fileVec.push_back(0);
			SuperString		fileStr(&fileVec[0]);
			
			//Iterate replacement keywords
			ReplaceMap::iterator	p;
			for (p = keywords->begin(); p != keywords->end(); ++p) {
				fileStr.Replace(SuperString("{") + p->first + SuperString("}"), p->second);
			}
			
			dataRef.Set(fileStr);
		}
		*/
		
		CreateAndSendHTTPResponse(contentType, 200, dataRef);
	} else {
		CreateAndSendHTTPResponse(
			kCFURLContentType_Text_HTML, 404,
			CCFData(CFSTR("<html><title>404 Document Not Found</title><body>Document not found.</body></html>")));
		
		Logf("%s Requested file was not found: %s", kWebServerErrorString, pathStr.utf8Z());
	}
}

void	CWebClient::SendXMLError(const char* errId, const SuperString& description) {
	ReplaceMap	keywords;
	
	keywords["error_id"]	= SuperString(errId);
	keywords["error_desc"]	= description;
	
	SendHTTPFile("server/error.xml", &keywords);
}

//Cookies
void	CWebClient::SetCookie(const SuperString& name, const SuperString& value) {
	i_serverCookies[name] = value;
	i_clientCookies[name] = value;
}

void	CWebClient::SetCookie(const char*	name, const char*	value) {
	SetCookie(SuperString(name), SuperString(value));
}

SuperString	CWebClient::GetCookie(const SuperString& name) {
	return i_clientCookies[name];
}

SuperString CWebClient::GetCookie(const char*	name) {
	return GetCookie(SuperString(name));
}

//Post Data
SuperString	CWebClient::GetPostData(const SuperString& key) {		
	return i_clientPostData[key];
}

SuperString	CWebClient::GetPostData(const char*	key) {
	return GetPostData(SuperString(key));
}

//Disconnect
void	CWebClient::Disconnect( ) {
	CFSocketInvalidate(i_socketRef);
	delete this;
}

//Callbacks
void	CWebClient::CB_ClientCallBack(
	CFSocketRef				socketRef,
	CFSocketCallBackType	callbackType,
	CFDataRef				addressDataRef,
	CFDataRef				in_dataRef
) {
	UNREFERENCED_PARAMETER(addressDataRef);
	UNREFERENCED_PARAMETER(socketRef);

	if (CWebServer::Get() == NULL) {
		return;
	}

	CCFData			dataRef(in_dataRef, true);
	
	{
		SuperString		raw(dataRef);

		Log(raw.c_str());
	}
	
	//Data Read
	if (callbackType == kCFSocketDataCallBack) {
		
		if (dataRef.size() == 0) {
			Disconnect();
			return;
		}

		#if kCFNetTest
			(void)CFHTTPMessageAppendBytes(i_requestRef.Get(), dataRef.begin(), dataRef.size());
			if (!CFHTTPMessageIsHeaderComplete(i_requestRef.Get())) {
				return;
			}
			
			SuperString		contentLength(CFHTTPMessageCopyHeaderFieldValue(i_requestRef.Get(), kCFURLAccessHeaderKey_ContentLength), false);
			CCFData		bodyDataRef(CFHTTPMessageCopyBody(i_requestRef.Get()), false);

			if (
				!contentLength.empty() 
				&& contentLength.GetAs_SInt32() > 0 
				&& bodyDataRef.size() < (size_t)contentLength.GetAs_SInt32()
			) {
				return;
			}
						
			//Get cookies
			SuperString		cookieString(CFHTTPMessageCopyHeaderFieldValue(i_requestRef.Get(), kCFURLAccessHeaderKey_GetCookie), false);
			
			if (!cookieString.empty()) {
				SuperString		cookie;
				SuperString		value;
				
				while (cookieString.Split("; ", &cookie, true)) {
					value.clear();

					if (cookie.Split("=", &value)) {
						i_clientCookies[cookie] = value;
					}
				}
				
				if (cookieString.Split("=", &value)) {
					i_clientCookies[cookieString] = value;
				}
			}
			
			//Get POST data
			if (SuperString(CFHTTPMessageCopyRequestMethod(i_requestRef.Get()), false) == kCFURLAccessMethod_POST) {
				SuperString		postString(bodyDataRef);
				SuperString		pair;
				SuperString		value;
				
				while (postString.Split("&", &pair, true)) {
					value.clear();

					if (pair.Split("=", &value)) {
						i_clientPostData[pair] = value;
					}
				}
				
				if (postString.Split("=", &value)) {
					value = SuperString(value.utf8Z(), kCFStringEncodingPercentEscapes);
					i_clientPostData[postString] = value;
				}
			}
			
			//Serve the request
			ServeRequest(i_requestRef.Get());
		#endif
	}
}

//static
void	CWebClient::CB_S_ClientCallBack(
	CFSocketRef				socketRef, 
	CFSocketCallBackType	callbackType, 
	CFDataRef				addressDataRef, 
	const void				*data, 
	void					*info
) {
	if (data == NULL) {
		return;
	}
	
	CWebClient		*thiz = (CWebClient *)info;

	thiz->CB_ClientCallBack(socketRef, callbackType, addressDataRef, (CFDataRef)data);
}

CWebSession::CWebSession()
{
	i_singer.clear();
	i_sessionID.Set_Ptr((Ptr)this);
	i_recentErr = noErr;
	i_recentT = 0;
}

bool	CWebSession::Authenticate(const SuperString& singer_id, const SuperString& password)
{
	UNREFERENCED_PARAMETER(singer_id);
	UNREFERENCED_PARAMETER(password);
	return true;
}

void	CWebSession::RevokeAuthentication() {
	i_singer.clear();
}

void	CWebSession::SetSinger(SuperString&	singer) {
	i_singer = singer;
}

SuperString	CWebSession::GetSinger() {
	return i_singer;
}

SuperString		CWebSession::GetSessionID()
{
	return i_sessionID;
}

void	CWebSession::CB_Timer()
{
	CFRunLoopTimerContext	timerContext;	structclr(timerContext);
	
	timerContext.info = this;

	if (!i_singer.empty() && time(NULL) - i_lastCheckin > kWebServerTimeout_Default) {
		RevokeAuthentication();
		
	} else if (!i_singer.empty()) {
		CFRunLoopTimerRef	timer(CFRunLoopTimerCreate(
			kCFAllocatorDefault, 
			CFAbsoluteTimeGetCurrent() + kWebServerTimeout_Default, 
			-1, 0, 0, CWebSession::CB_S_Timer, 
			&timerContext));
		
		CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
		CFRelease(timer);
	}
}

void	CWebSession::CB_S_Timer(CFRunLoopTimerRef timer, void* info)
{
	UNREFERENCED_PARAMETER(timer);

	((CWebSession*)info)->CB_Timer();
}

void	CWebSession::Checkin() {
	i_lastCheckin = time(NULL);
}
