//	CCFError
#ifndef _H_CCFError
#define _H_CCFError

#if OPT_MACOS
	typedef struct __CFError *			CFErrorRef;

	#if !COMPILING_ON_10_7_OR_BETTER
		extern const CFStringRef kCFErrorDomainPOSIX;
		extern const CFStringRef kCFErrorDomainCocoa;
	#endif
#else
	// #include "errno.h"	//	on windows, these errors DO NOT MATCH mac errors
	#ifdef ENETDOWN
		#undef ENETDOWN
	#endif

	#define ENETDOWN       50
#endif	//	OPT_WINOS

extern const CFStringRef kCFErrorDomainKey;
extern const CFStringRef kCFErrorCodeKey;
extern const CFStringRef kCFStreamErrorCodeKey;
extern const CFStringRef kCFStreamErrorDomainKey;

CFDictionaryRef		CFErrorCopyAsDict(CFErrorRef err);
void				CFErrorTranslateWinToMac(CFDictionaryRef io_dict);

#endif // _H_CCFError
