//	CCFError
#include "stdafx.h"
#include "SuperString.h"
#include "CCFError.h"

const CFStringRef kCFErrorCodeKey			= CFSTR("NSCode");
const CFStringRef kCFErrorDomainKey			= CFSTR("NSDomain");
const CFStringRef kCFStreamErrorCodeKey		= CFSTR("_kCFStreamErrorCodeKey");
const CFStringRef kCFStreamErrorDomainKey	= CFSTR("_kCFStreamErrorDomainKey");

#if !defined(_HELPERTOOL_)
#if OPT_KJMAC
	#include "CTenFive_Funcs.h"
	const CFStringRef kCFErrorDomainPOSIX	= CFSTR("NSPOSIXErrorDomain");
	const CFStringRef kCFErrorDomainCocoa	= CFSTR("NSCocoaErrorDomain");
#endif
#endif	// !defined(_HELPERTOOL_)

CFDictionaryRef		CFErrorCopyAsDict(CFErrorRef err)
{
	CCFDictionary		userInfoDict(CFErrorCopyUserInfo(err));
	CCFDictionary		dict(userInfoDict.Copy());
	CCFString			domainRef(CFErrorGetDomain(err), true);
	CFIndex				errCode(CFErrorGetCode(err));
		
	dict.SetValue(kCFErrorDomainKey, domainRef.ref());
	dict.SetValue_Ref(kCFErrorCodeKey, static_cast<SInt32>(errCode));

//	CCFLog(true)(userInfoDict);

	CFErrorTranslateWinToMac(dict);

	return dict.transfer();
}

#if 0
	on mac, if you're offline, you get this:
	
	{
		"NSCode":					50,	//	ENETDOWN
		"NSDomain":					"NSPOSIXErrorDomain",
		"_kCFStreamErrorCodeKey":	50,	//	ENETDOWN
		"_kCFStreamErrorDomainKey":	1	//	kCFStreamErrorDomainPOSIX
	}
	
	on widows, if you're offline you get this:
	
	{
		"NSCode":					11004,	//	WSANO_DATA
		"NSDomain":					"NSWinSockErrorDomain",
		"_kCFStreamErrorDomainKey":	12		//	kCFStreamErrorDomainWinSock
	}

	//	below will fix that
#endif

void				CFErrorTranslateWinToMac(CFDictionaryRef dict)
{
	#if OPT_WINOS
	CCFDictionary		errDict(dict, true);
	SInt32				errCode(errDict.GetAs_SInt32(SuperString(kCFErrorCodeKey).utf8Z()));
	CFIndex				errDomainKey((CFIndex)errDict.GetAs_SInt32(SuperString(kCFStreamErrorDomainKey).utf8Z()));
	
	if (
		   errCode			== WSANO_DATA 	// EAI_NODATA
		&& errDomainKey		== kCFStreamErrorDomainWinSock
	) {
		//CCFLog(true)(errDict.ref());

		errDict.SetValue_Ref(kCFErrorCodeKey,			ENETDOWN);
		errDict.SetValue_Ref(kCFStreamErrorCodeKey,		ENETDOWN);
		errDict.SetValue_Ref(kCFStreamErrorDomainKey,	kCFStreamErrorDomainPOSIX);
		errDict.SetRealValue(kCFErrorDomainKey,			kCFErrorDomainPOSIX);

		//CCFLog(true)(errDict.ref());
	}
	#endif
}
