#include "stdafx.h"

#include "CFTest.h"
#include "CNetHTTP.h"
#include "CFBonjour.h"
#include "CFNetworkAddress.h"
#include "CFNetServices.h"
#include "CWebServerTest.h"

#if OPT_WINOS
#include "ws2ipdef.h"
#endif

//---------------------------------------------------------------------------------------------------------
//	test all of these, all must succeed
#if 0
	#define		kTest_Bonjour					1
	#define		kTest_SimpleDownload			1
	#define		kTest_SSL_Download				1
	#define		kTest_SSL_DateHeader			1
	#define		kTest_Redirect					1
	#define		kTest_ComplexDownload_SMALL		1
	#define		kTest_ComplexDownload_LARGE		0
	#define		kTest_Upload					1
#else
	#define		kTest_Bonjour					1
	#define		kTest_SimpleDownload			1
	#define		kTest_SSL_Download				1
	#define		kTest_SSL_DateHeader			1
	#define		kTest_Redirect					1
	#define		kTest_ComplexDownload_SMALL		1
	#define		kTest_ComplexDownload_LARGE		1
	#define		kTest_Upload					1
#endif

//	no need to test these
#define		kTest_Telnet			0	//	 to test, "telnet localhost 12345", then type something, it should merely echo
#define		kTest_Server			0	//	this doesn't work any more: sessionP is NULL unexpectedly, need to factor code from kJams again
//---------------------------------------------------------------------------------------------------------

bool	CFGetFileData(bool logB, const char* filePathZ, CCFData *dataRefP);

bool	LaunchURL(const char *utf8Z)
{
	bool			successB = false;
	SuperString		urlStr(uc(utf8Z));

	#if OPT_MACOS
		urlStr.Escape("#");

		ScCFReleaser<CFURLRef>		url(urlStr.CopyAs_URLRef(false));
		
		successB = LSOpenCFURLRef(url, NULL) == noErr;
	#else
		if (urlStr.StartsWith("telnet")) {
			CCFLog(true)(CFSTR("You need a telnet client to test this."));
			CCFLog(true)(CFSTR("I suggest using ÒPuTTYÓ: http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html"));
			CCFLog(true)(CFSTR("Then, log in to this address:"));
			CCFLog(true)(urlStr.ref());
		} else {
			OSStatus		err = noErr;
			size_t			result = 0;
			LPCWSTR			urlZ = urlStr.w_str();

			if (ExtraLogging()) {
				Logf("Launching <%s>\n", urlStr.utf8Z());
			}
			
			result = (size_t)ShellExecuteW(GetDesktopWindow(), L"open", urlZ, NULL, NULL, SW_SHOWNORMAL);
			successB = err == noErr && result > 32;
		}
	#endif
	
    return successB;
} 

void	CCFLogErr(const char *errZ, OSStatus err = noErr)
{
	CFReportUnitTest(errZ, err);
}

static void CB_S_SocketCallback(
	CFSocketRef				socketRef, 
	CFSocketCallBackType	cbType, 
	CFDataRef				address, 
	const void				*data, 
	void					*cbData)
{
	UNREFERENCED_PARAMETER(address);
	UNREFERENCED_PARAMETER(cbData);

	if (cbType == kCFSocketDataCallBack) {
		CCFData			dataRef((CFDataRef)data, true);
		size_t			sizeL = dataRef.size();
		bool			endB = sizeL == 0;
		
		if (!endB) {
			UInt8			endOfTransmission(0x04);	//	ctrl-D, or close the telnet window
			UInt8			curData(dataRef[sizeL - 1]);
			
			endB = curData == endOfTransmission;
		}
		
		if (endB) {
			CFRunLoopStop(CFRunLoopGetCurrent());
			
		} else if (ExtraLogging()) {
			OSStatus	err((OSStatus)CFSocketSendData(socketRef, NULL, dataRef, 10));

			CFReportUnitTest("CB_S_SocketCallback", err);
		}
	}
}
 
static void CB_S_SocketCallOut(CFSocketRef s, CFSocketCallBackType cbType, CFDataRef address, const void *data, void *cbData)
{
	UNREFERENCED_PARAMETER(s);
	UNREFERENCED_PARAMETER(cbData);
	UNREFERENCED_PARAMETER(address);

	if (cbType == kCFSocketAcceptCallBack) {
		CFSocketNativeHandle		nativeSocketRef(*(CFSocketNativeHandle *)data);
		ScCFReleaser<CFSocketRef>	socketRef(CFSocketCreateWithNative(
			kCFAllocatorDefault, 
			nativeSocketRef, 
			kCFSocketDataCallBack,
			CB_S_SocketCallback, 
			NULL));
		
		if (socketRef.Get()) {
			CFIndex								priorityIndex(0);
			ScCFReleaser<CFRunLoopSourceRef>	loopSrc(CFSocketCreateRunLoopSource(
				kCFAllocatorDefault, socketRef, priorityIndex));
			
			CFRunLoopAddSource(CFRunLoopGetCurrent(), loopSrc, kCFRunLoopDefaultMode);
		}
	}
}

bool		TestAddressRoundTrip(CFBonjour *bonjourP, short familyS, const SuperString& startAddr)
{
	bool						successB = true;
	CFNetworkSockAddrUnion		sockAddr;
	CFStreamError				streamErr(kCFStreamErrorNone);
	const char					*typeZ = familyS == AF_INET ? "IPv4" : "IPv6";
	SuperString					formatStr("Bonjour: %s: %s");
	SuperString					finalStr;
	
	CFNetworkInitSockAddress(&sockAddr, familyS, 0);
		
	streamErr = CFNetworkGetSockAddressFromName(startAddr.ref(), &sockAddr);
	
	finalStr = formatStr;
	finalStr.ssprintf(NULL, "Name -> Addr", typeZ);
	
	if (streamErr.error != noErr) {
		finalStr = bonjourP->GetStreamErrStr(finalStr.utf8Z(), streamErr);
		successB = false;
	}
	
	CFReportUnitTest(finalStr.utf8Z(), !successB);
	 
	if (successB) {
		SuperString		endAddr(CFNetworkCreateNameFromSockAddress(&sockAddr), false);
		
		finalStr = formatStr;
		finalStr.ssprintf(NULL, "Addr -> Name", typeZ);
		successB = startAddr == endAddr;
		CFReportUnitTest(finalStr.utf8Z(), !successB);
	}
	
	return successB;
}


void	CFNetworkTest()
{
	SInt32				err = noErr;

	if (!IsDefaultEncodingSet()) {
		SetDefaultEncoding(kCFStringEncodingMacRoman);
	}
	
	CCFLog()(CFSTR("------------------New CFNetwork Log---------------\n"));

	if (kTest_Bonjour) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------Bonjour Server---------------\n"));
		CFBonjour		*bonjourP(CFBonjour::Get());
		
		ERR(bonjourP == NULL);
		CFReportUnitTest("Allocate Bonjour", err);

		if (err) {
			err = noErr;
		} else {
			bonjourP->SetExtraLogging(ExtraLogging());

			ERR(bonjourP->Publish(kCFBonjourService_HTTP, "CFTest", kWebServerPort_Default));
			
			CFReportUnitTest("Bonjour Server", err);

			if (err) {

				if (err == kCFNetServicesErrorNotFound) {
					CCFLog()(CFSTR("mDNSResponder not running.  Did you forget to install Bonjour for Windows?\n"));
				}

				err = noErr;
			} else {
				CFSleep(1);	//	must give it a second to register

				if (ExtraLogging()) CCFLog()(CFSTR("------------------Bonjour Browser---------------\n"));
				
				ERR(bonjourP->Browse(kCFBonjourService_HTTP, CFSTR("")));
				
				CFReportUnitTest("Bonjour Browser START", err);

				if (err) {
					err = noErr;
				} else {
					int		delay_secondsI = 3;
					
					if (ExtraLogging()) {
						SuperString		str;
						
						str.append((long)delay_secondsI);
						str.append(" second delay...");
						CCFLog(true)(str.ref());
					}
					
					//	must give it a few seconds to gather the data
					CFSleep(delay_secondsI);
					
					CCFArray	array(bonjourP->CopyBrowseClients());
					
					ERR(array.empty());
					
					CFReportUnitTest("Bonjour Browser FINISH", err);
					
					if (ExtraLogging()) {
						if (!err) {
							CCFLog(true)(array);
						}
					}
				}
			}

			if (ExtraLogging()) CCFLog()(CFSTR("------------------Round Trip Address Conversion---------------\n"));
			{
				TestAddressRoundTrip(bonjourP, AF_INET,		"192.150.23.3");
				TestAddressRoundTrip(bonjourP, AF_INET6,	"fdf9:529:c466:59c9:5ab0:35ff:fe74:f1ad");
			}

			delete bonjourP;
		}
	}
	
	
	if (kTest_SimpleDownload) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------Simple download---------------\n"));
		
		SuperString				urlStr("http://davecotter.com/");
		ScCFReleaser<CFURLRef>	urlRef(urlStr.CopyAs_URLRef());
		CCFDictionary			headerDict(NULL, true);
		CCFData					cfData;
			
		ERR(cfData.Download_Resource(urlRef, headerDict.ImmutableAddressOf()));
			
		CFReportUnitTest("Simple download", err);
		
		if (err) {
			err = noErr;
			
		} else if (ExtraLogging()) {
		
			{
				CCFLog()(CFSTR("---- Headers:\n"));
				CCFLog(true)(headerDict);
			}
			
			{
				SuperString			str(cfData);
				
				CCFLog()(CFSTR("---- Data:\n"));
				CCFLog()(str.ref());
			}
			
			CCFLog()(CFSTR("---- End:\n"));
		}
	}

	#if 1
		//	on windows, this test will fail, cuz actually SSL
		#define	kSSL_URL		kURL_kJams_SSL		"/downloads/version.xml"
	#else
		//	this test will pass, cuz not SSL actually
		#define	kSSL_URL		kURL_kJams_SSL_HACK	"/downloads/version.xml"
	#endif


	if (kTest_SSL_Download) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------SSL download---------------\n"));
		
		SuperString				urlStr(kSSL_URL);
		ScCFReleaser<CFURLRef>	urlRef(urlStr.CopyAs_URLRef());
		CCFData					cfData;
			
		CFURLCreateDataAndPropertiesFromResource(
			kCFAllocatorDefault, urlRef, cfData.AddressOf(), NULL, NULL, &err);
			
		CFReportUnitTest("SSL download", err);

		if (err) {
			err = noErr;
			
		} else if (ExtraLogging()) {
			SuperString			str(cfData);
			
			CCFLog()(CFSTR("---- Data:\n"));
			CCFLog()(str.ref());
			CCFLog()(CFSTR("---- End:\n"));
		}
	}

	if (kTest_SSL_Download) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------TLS 1.0 download---------------\n"));
		
		SuperString				urlStr("https://tls-v1-0.badssl.com:1010/icons/style.css");
		ScCFReleaser<CFURLRef>	urlRef(urlStr.CopyAs_URLRef());
		CCFData					cfData;
			
		CFURLCreateDataAndPropertiesFromResource(
			kCFAllocatorDefault, urlRef, cfData.AddressOf(), NULL, NULL, &err);
			
		CFReportUnitTest("TLS1.0 download", err);

		if (err) {
			err = noErr;
		} else if (ExtraLogging()) {
			SuperString			str(cfData);
			
			CCFLog()(CFSTR("---- Data:\n"));
			CCFLog()(str.ref());
			CCFLog()(CFSTR("---- End:\n"));
		}
	}

	if (kTest_SSL_Download) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------TLS 1.1 download---------------\n"));
		
		SuperString				urlStr("https://tls-v1-1.badssl.com:1011/style.css");
		ScCFReleaser<CFURLRef>	urlRef(urlStr.CopyAs_URLRef());
		CCFData					cfData;
			
		CFURLCreateDataAndPropertiesFromResource(
			kCFAllocatorDefault, urlRef, cfData.AddressOf(), NULL, NULL, &err);
			
		CFReportUnitTest("TLS1.1 download", err);

		if (err) {
			err = noErr;
		} else if (ExtraLogging()) {
			SuperString			str(cfData);
			
			CCFLog()(CFSTR("---- Data:\n"));
			CCFLog()(str.ref());
			CCFLog()(CFSTR("---- End:\n"));
		}
	}

	{
		SuperString			redir_ssl_urlStr("http://dl.dropbox.com/u/195957/kJams/version.xml");

		if (kTest_Redirect) {
			if (ExtraLogging()) CCFLog()(CFSTR("------------------redirect---------------\n"));
			
			ScCFReleaser<CFURLRef>	urlRef(redir_ssl_urlStr.CopyAs_URLRef());
			CCFData					cfData;
				
			CFURLCreateDataAndPropertiesFromResource(
				kCFAllocatorDefault, urlRef, cfData.AddressOf(), NULL, NULL, &err);
				
			CFReportUnitTest("redirect", err);

			if (err) {
				err = noErr;
				
			} else if (ExtraLogging()) {
				
				{
					SuperString			str(cfData);
					
					CCFLog()(CFSTR("---- Data:\n"));
					CCFLog()(str.ref());
				}
				
				CCFLog()(CFSTR("---- End:\n"));
			}
		}
		
		if (kTest_ComplexDownload_SMALL) {
			if (ExtraLogging()) CCFLog()(CFSTR("------------------complex download SMALL with redirect---------------\n"));
			
			{
				CNetHTTP				net(kCFURLAccessMethod_GET);
				SuperString				resultStr;
				
				net.SetLogging(ExtraLogging());
				ERR(net.Download_String("https://www.dropbox.com/s/rhca1ldydyhcrxf/Party%20Tyme%20Pro%20Streaming.json?dl=1", &resultStr));
				CFReportUnitTest("download string", err);
				
				CCFLog(true)(resultStr.ref());
			}
			
			CNetHTTP				net(kCFURLAccessMethod_GET);
			CCFData					dataRef;
			
			net.SetLogging(ExtraLogging());
			
			ERR(net.Download_CFData(redir_ssl_urlStr, &dataRef, true));

			CFReportUnitTest("complex download SMALL", err);
			
			{
				size_t		dataSizeL = dataRef.size();
				
				err = dataSizeL >= 450 ? noErr : notExactSizeErr;
				CFReportUnitTest("complex download SMALL: got all data", err);
			}

			if (err) {
				err = noErr;
			}
		}
	}

	if (kTest_ComplexDownload_LARGE) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------complex download LARGE with redirect---------------\n"));
		
		CNetHTTP				net(kCFURLAccessMethod_GET);
		CCFData					dataRef;
		
		net.SetLogging(ExtraLogging());
		
		ERR(net.Download_CFData("http://dl.dropbox.com/u/195957/kJams/kJams2.zip", &dataRef, true));

		CFReportUnitTest("complex download LARGE", err);
		
		{
			size_t		dataSizeL = dataRef.size();
			
			err = dataSizeL >= 12000000 ? noErr : notExactSizeErr;
			CFReportUnitTest("complex download LARGE: got all data", err);
		}

		if (err) {
			err = noErr;
		}
	}

	if (kTest_SSL_DateHeader) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------get date header SSL---------------\n"));
		
		SuperString				dateStr = CNetHTTP::GetDate(kSSL_URL, &err);
		
		CFReportUnitTest("get date header SSL", err);

		if (err) {
			err = noErr;

		} else if (ExtraLogging()) {
			CFAbsoluteTime		absT = dateStr.GetAs_CFAbsoluteTime(SS_Time_SHORT);
			
			dateStr.Set(absT, SS_Time_LONG_PRETTY);
			CCFLog(true)(dateStr.ref());
		}
	}

	if (kTest_Upload) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------upload to freedb---------------\n"));
		
		CCFData		fileData;
		
		ERR(CFGetFileData(ExtraLogging(), "freedb.plist", &fileData) == false);
		
		CFReportUnitTest("load freedb.plist", err);

		if (err) {
			err = noErr;
			
		} else {
			CCFDictionary		headerDict(fileData.CopyAs_Dict(true));
			SuperString			urlStr(headerDict.GetAs_String("urlStr"));
			SuperString			bodyStr(headerDict.GetAs_String("bodyStr"));
			
			headerDict.RemoveValue("urlStr");
			headerDict.RemoveValue("bodyStr");
			
			CNetHTTP			net(kCFURLAccessMethod_POST);
			
			net.SetLogging(ExtraLogging());
			net.i_ignore_sizeB = true;
			
			ERR(net.SendMessage(
				urlStr.utf8().c_str(), 80,  
				&headerDict, 
				bodyStr.utf8().c_str()));

			CFReportUnitTest("upload send", err);

			if (err) {
				err = noErr;
				
			} else {
				ERR(net.WaitForEnd());

				CFReportUnitTest("upload completion", err);

				if (err) {
					err = noErr;
					
				} else {
					UCharVec		charVec;

					charVec.resize(net.i_dataQue.size());
					net.i_dataQue.pull_front(&charVec[0], charVec.size());

					charVec.push_back(0);

					SuperString		resultStr(&charVec[0]);
					
					ERR(resultStr.empty());

					CFReportUnitTest("upload reply", err);

					if (resultStr.empty()) {
						err = noErr;
						
					} else if (ExtraLogging()) {
						CCFLog(true)(resultStr.ref());
					}
				}
			}
		}
	}
	
	if (kTest_Telnet) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------telnet server---------------\n"));

		//	https://sourceforge.net/projects/opencflite/forums/forum/890979/topic/3548983/index/page/1
		CFSocketError					serr = kCFSocketSuccess;
		ScCFReleaser<CFSocketRef>		serverSocketRef(CFSocketCreate(
			kCFAllocatorDefault, 
			PF_INET, SOCK_STREAM, IPPROTO_TCP,
			kCFSocketAcceptCallBack,
			CB_S_SocketCallOut,
			NULL));
		
		if (serverSocketRef.Get() == NULL) {
			serr = kCFSocketError;
			CCFLogErr("FAILED to CFSocketCreate", (OSStatus)serr);
		} else {
			int				yesB(1);
			int				fd(CFSocketGetNative(serverSocketRef));
			int				resultI(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yesB, sizeof(yesB)));
			
			if (resultI == -1) {
				serr = (CFSocketError)errno;
				CCFLogErr("FAILED to setsockopt", (OSStatus)serr);
			}
		}

		if (!serr) {
			CFNetworkSockAddr4		sockAddr; structclr(sockAddr);
			
			#if OPT_MACOS
				sockAddr.sin_len = sizeof(sockAddr);
			#endif

			sockAddr.sin_family			= AF_INET;
			sockAddr.sin_port			= CFSwapInt16HostToBig(kWebServerPort_Default);
			sockAddr.sin_addr.s_addr	= CFSwapInt32HostToBig(INADDR_LOOPBACK);

			CCFData			hostData((const UInt8 *)&sockAddr, sizeof(sockAddr));
			
       		serr = CFSocketSetAddress(serverSocketRef, hostData);
			
			if (serr) {
				CCFLogErr("FAILED to CFSocketSetAddress", (OSStatus)serr);
			}
		}
		
        if (!serr) {
			
			{
				CFIndex								priorityIndex(0);
				ScCFReleaser<CFRunLoopSourceRef>	runLoopSrcRef(CFSocketCreateRunLoopSource(
					kCFAllocatorDefault, serverSocketRef, priorityIndex));

				serverSocketRef.clear();

				if (runLoopSrcRef.Get() == NULL) {
					serr = kCFSocketError;
					CCFLogErr("FAILED to CFSocketCreateRunLoopSource", (OSStatus)serr);
				} else {
					CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSrcRef, kCFRunLoopDefaultMode);                                   
				}
			}
			
			if (!serr) {
				CCFLog(true)(CFSTR("Success: creating telnet server. typing should simply echo. close the telnet window to shut down the server."));

				{
					SuperString			urlStr("telnet://localhost:");
					
					urlStr.append((long)kWebServerPort_Default);
					LaunchURL(urlStr.utf8Z());
				}

				CFRunLoopRun();
			}
		}
	}

	if (kTest_Server) {
		if (ExtraLogging()) CCFLog()(CFSTR("------------------Test web server---------------\n"));
		
		if (!CWebServer::Start()) {
			CCFLog()(CFSTR("Failed to start on default port (perhaps it crashed last time, leaving that port open?)\n"));
		} else {
			SuperString			urlStr("http://localhost:");
			
			urlStr.append((long)kWebServerPort_Default);
			LaunchURL(urlStr.utf8Z());
		}
		
		CFRunLoopRun();
	}

	CCFLog()(CFSTR("----------------CFNetwork Tests Ended-------------\n"));
}
