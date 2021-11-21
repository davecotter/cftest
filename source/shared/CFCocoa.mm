//	CFCocoa
#if !defined(YAAF_MACHO)
	#include "stdafx.h"
#endif

#import <Cocoa/Cocoa.h>
//#include "SuperString.h"	//	causes compile errors (tangled headers)

#include "CFCocoa.h"

CFTypeID	CFRangeGetTypeID()
{
	static CFTypeID		s_rangeTypeID = 0;

	if (s_rangeTypeID == 0) {
		CFTypeRef		cfType(CFRangeCreate());

		s_rangeTypeID = CFGetTypeID(cfType);
		CFRelease(cfType);
	}

	return s_rangeTypeID;
}

CFRange		CFTypeGetRange(CFTypeRef typeRef)
{
	NSValue*		rangeValRef	((__bridge NSValue *)typeRef);
	NSRange			nsRange		([rangeValRef rangeValue]);
	CFRange			cfRange		(CFRangeMake(nsRange.location, nsRange.length));

	return cfRange;
}

CFTypeRef	CFRangeCreate(const CFRange& cfRange)
{
	NSRange			nsRange		(NSMakeRange(cfRange.location, cfRange.length));
	NSValue*		rangeValRef	([NSValue valueWithRange: nsRange]);
	CFTypeRef		cfTypeRef	((__bridge CFTypeRef)rangeValRef);

	return cfTypeRef;
}

CFTypeRef	CFRangeCreate(CFIndex loc, CFIndex len)
{
	return CFRangeCreate(CFRangeMake(loc, len));

}
