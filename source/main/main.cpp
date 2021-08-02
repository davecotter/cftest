#include "stdafx.h"
#include "CFUtils.h"
#include "CFNetworkTest.h"
#include "CFTest.h"
#include <CoreFoundation/CFUserNotification.h>

#define		kStockChecker		0

#if kStockChecker

#include "CNetHTTP.h"

#define			kSleepMinutes			5
#define			kDebugUsingLocalFile	0

#if kDebugUsingLocalFile
	#define		kStockUrlStr	"file:///Users/davec/Desktop/quote %s.json"
#else
	#define		kStockUrlStr	"https://cloud.iexapis.com/stable/stock/%s/quote?token=pk_de2386cc76654554b8d2a8a0f3bc8b63"
#endif

#define		kNotifyStr		"/usr/local/bin/growlnotify -m \"holy cats\""

void		GrowlNotify(SuperString msgStr)
{
	CCFLog(true)(msgStr.ref());
	msgStr.prepend("/usr/local/bin/growlnotify -m \"");
	msgStr.append("\"");
	
	(void)(system(msgStr.utf8Z()));
}

void		StockCheck(const SuperString& symbolStr)
{
	bool			doneB(false);
	SuperString		quotedSymbolStr(symbolStr);
	
	SetDefaultEncoding(kCFStringEncodingUTF8);

	quotedSymbolStr.Enquote(true);

	GrowlNotify("Starting Stock Checker for " + quotedSymbolStr);

	do {
		OSStatus			err(noErr);
		CNetHTTP			net(kCFURLAccessMethod_GET);
		SuperString			resultStr;
		SuperString			urlStr(kStockUrlStr);
		
		urlStr.ssprintf(NULL, symbolStr.utf8Z());
		
		#if kDebugUsingLocalFile
		{
			CCFURL				urlRef(urlStr.CopyAs_URLRef());
			CCFData				urlData(urlRef);
			
			resultStr = urlData;
		}
		#else
			// net.SetLogging(true);
			ERR(net.Download_String(urlStr, &resultStr, true));
		#endif
		
		if (err) {
			SuperString		errStr("Error attempting http: ");
			
			errStr.append((long)err);
			GrowlNotify(errStr);
		} else {
			CCFDictionary		dict(resultStr, true);
			SuperString			openStr("open");
			
			if (!dict.ContainsKey(openStr.ref())) {
				GrowlNotify(openStr.Enquote() + " key not found");
				CCFLog(true)(dict);
				
			} else {
				CFTypeEnum			openType(dict.GetTypeEnum(openStr.ref()));
				
				if (openType != CFType_STRING) {
					doneB = true;
					
				} else {
					CFTypeRef		valRef(dict.GetValue(openStr.ref()));
					SuperString		valStr; valStr.Set_CFType(valRef);
					SuperString		str;
					
					str += openStr.Enquote();
					str += " is ";
					str += valStr;
					str += " (";
					str += CFTypeCopyDesc(openType);
					str += ")";
					
					CCFLog(true)(str.ref());
				}
			}			
		}
		
		if (!doneB) {
			CFSleep(kSleepMinutes * 60);
		}
		
	} while (!doneB);

	GrowlNotify(quotedSymbolStr + SuperString(" is open for trading!"));
}

#endif // kStockChecker

#if _QT_ || OPT_MACOS
int		main()
#else
int		WINAPI wWinMain(HINSTANCE h, HINSTANCE prev, LPWSTR cmd, int show)
#endif
{
	#if kStockChecker
		StockCheck("bynd");
	#elif 1
		CFTest();
		//CFNetworkTest();
	#else
		CFUserNotificationDisplayNotice(
			0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL,
			CFSTR("Holy Cats"), CFSTR("It Showed a Message!"), NULL);
	#endif
	return 0;
}
