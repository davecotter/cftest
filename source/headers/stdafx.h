/*
	Add local class declarations to this file.
*/
#ifndef _STDAFX
#define _STDAFX

#ifdef _CFTEST_
	#ifdef __WIN32__
		#include <numeric>	//	including here avoids compile warning about conflict with "loop" macro (for statement)
		#define OPT_WINOS	1
		#define OPT_MACOS	0
		#include <CFNetwork.h>
	#else
		#define OPT_MACOS	1
		#define OPT_WINOS	0
	#endif
#endif

#endif
