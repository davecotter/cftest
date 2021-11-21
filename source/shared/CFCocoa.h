//	CFCocoa.h
#ifndef _H_CFCocoa
#define _H_CFCocoa

#include	<CoreFoundation/CoreFoundation.h>

CFTypeID	CFRangeGetTypeID();
CFRange		CFTypeGetRange(CFTypeRef typeRef);
CFTypeRef	CFRangeCreate(const CFRange& cfRange);
CFTypeRef	CFRangeCreate(CFIndex loc = 0, CFIndex len = 0);

#endif
