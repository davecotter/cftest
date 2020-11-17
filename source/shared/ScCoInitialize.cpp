//	ScCoInitialize.cpp

#include "stdafx.h"

#if _KJAMS_
#include "XPlatConditionals.h"
#endif

#if OPT_WINOS
	#include "objbase.h"
#endif

#include "ScCoInitialize.h"

ScCoInitialize::ScCoInitialize(bool apartment_threadB)
{
#if OPT_WINOS
	{
		if (apartment_threadB) {
			::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		} else {
			::CoInitialize(NULL);
		}
	}
#endif
}

ScCoInitialize::~ScCoInitialize()
{
#if OPT_WINOS
	{
		::CoUninitialize();
	}
#endif
}

