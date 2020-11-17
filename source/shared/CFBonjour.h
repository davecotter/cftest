//
//  CFBonjour.h
//  murmur
//
//  Created by ecume des jours on 10/28/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#ifndef _H_CFBonjour
#define _H_CFBonjour

#if OPT_WINOS
	#include "CFNetServices.h"
#endif

#include "CCFData.h"

typedef std::map<SInt32, const char*>	ErrMap;

#define		kCFBonjourService_HTTP			CFSTR("_http._tcp")

#define		kCFBonjourClientKey_NAME		CFSTR("name")
#define		kCFBonjourClientKey_ADDRESS		CFSTR("address")
#define		kCFBonjourClientKey_PORT		CFSTR("port")
#define		kCFBonjourClientKey_HOST		CFSTR("host")
#define		kCFBonjourClientKey_TYPE		CFSTR("type")

#define		kCFBonjourClientValue_IPv4		CFSTR("IPv4")
#define		kCFBonjourClientValue_IPv6		CFSTR("IPv6")

extern CFStreamError		kCFStreamErrorNone;

class CFBonjourTask;
typedef std::set<CFBonjourTask *>	CFBonjourTaskSet;

class CFBonjour {
	public:
	
	typedef enum {
		kServiceType_NONE, 
		
		kServiceType_REGISTER,
		kServiceType_RESOLVE, 
		kServiceType_BROWSE,
		
		kServiceType_NUMTYPES
	} ServiceType;
	
	private:
	friend class CFBonjourTask;
	static	CFBonjour				*s_selfP;
	CCFArray 						i_clientsArray;
	ErrMap							i_errMap;
	CFBonjourTask					*i_browseTaskP;
	CFBonjourTask					*i_publishTaskP;
	CFBonjourTaskSet				i_taskSet;
	bool							i_extra_loggingB;

	/*******************************************/

	CFStreamError		NewTask(
		CFBonjourTask		**return_taskPP, 
		ServiceType			serviceType, 
		const SuperString&	domainStr, 
		const SuperString&	serviceTypeStr, 
		const SuperString&	nameStr = SuperString(), 
		UInt16				portL = 0);

	void				RemoveTask(CFBonjourTask *taskPP);
	void				CancelTask(CFBonjourTask **taskPP);
	
	/*******************************************/
	public:
	static	CFBonjour*	Get();
	CFBonjour();
	~CFBonjour();
	
	SuperString			GetStreamErrStr(const char * valZ, CFStreamError& err);
	void				LogStreamErr(const char * valZ, CFStreamError& err);

	void				SetExtraLogging(bool logB)	{ i_extra_loggingB = logB; }
	bool				IsExtraLogging()			{ return i_extra_loggingB; }
	/*******************************************/
	bool			IsPublished()	{	return i_publishTaskP != NULL;	}
	void			StopPublish() 	{	CancelTask(&i_publishTaskP);		}
	OSStatus		Publish(const SuperString& serviceTypeStr, const SuperString& serviceNameStr, UInt16 portL) {
		return NewTask(
			&i_publishTaskP, kServiceType_REGISTER, 
			CFSTR(""), serviceTypeStr, serviceNameStr, portL).error;
	}


	/*******************************************/
	bool			IsBrowsing()	{	return i_browseTaskP != NULL;	}
	void			StopBrowsing() 	{	CancelTask(&i_browseTaskP);		}
	OSStatus		Browse(const SuperString& serviceTypeStr, const SuperString& domainStr) {
		return NewTask(
			&i_browseTaskP, kServiceType_BROWSE, 
			domainStr, serviceTypeStr).error;
	}

	CFArrayRef		CopyBrowseClients() {
		return i_clientsArray.Retain();
	}

	CCFArray&		GetBrowseClients() {
		return i_clientsArray;
	}
};

#endif
