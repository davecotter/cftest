//	CFStackTrace
#include "stdafx.h"
#include "sstream"

#if !_YAAF_
	//	yaaf win still uses bost 1.40
	#define _GNU_SOURCE
	#include <boost/stacktrace.hpp>
#endif // !_YAAF_

#include "SuperString.h"
#include "CFStackTrace.h"

CFStringRef		CFStackTraceCreateString()
{
#if !_YAAF_
	std::ostringstream		ostr;
	
	ostr << boost::stacktrace::stacktrace();

	std::string		mbStr(ostr.str());
	SuperString		stackStr(uc(mbStr.c_str()));
	
	stackStr.prepend("Stack Trace:\n");
	return stackStr.Retain();
#else
	return SuperString().Retain();
#endif // !_YAAF_
}

void			CFStackTraceLog()
{
#if !_YAAF_
	CCFString		stackStr(CFStackTraceCreateString());
	
	CCFLog(true)(stackStr.ref());
#endif // !_YAAF_
}
