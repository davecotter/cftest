#include "stdafx.h"

#if OPT_MACOS
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	
	#if _QT_ && defined(_CFTEST_)
//	Xcode/CFSocketStream has the error domains defined
//	but if i include them here, it attempts to load CFLite version
//	#include <CFNetwork/CFSocketStream.h>

//	CFLite/CFStream also has them defined
//	but if i include them here, it attempts to load Xcode version
//	#include <CoreFoundation/CFStream.h>

//	hack around the problem by forcing us to get the CFStream that is
//	in our CFLite directory (under 'dist')
	#include <include/CoreFoundation/CFStream.h>
	#endif
#else
	#include "CFSocketStream.h"
	#include "CFFTPStream.h"
	#include "CFHTTPStream.h"
	#include "ws2tcpip.h"
#endif

#include "CFNetworkAddress.h"
#include "CFBonjour.h"

CFStreamError		kCFStreamErrorNone = { kCFStreamErrorDomainMacOSStatus, noErr };

/************************************************************************************************/

class CFBonjourTask {
	private:
	CFBonjour				*i_bonjourP;
	CFBonjour::ServiceType	i_serviceType;
	CFStreamError			i_streamErr;
	
	union {
		CFTypeRef								typeRef;
		CFNetServiceRef							serviceRef;
		CFNetServiceBrowserRef 					browserRef;
	} i_net;

	bool					i_scheduledB;
	bool					i_clientSetB;
	Boolean					i_startedB;
	
	/*************************/
	static void	CB_S_RegisterResolve(
		CFNetServiceRef		serviceRef, 
		CFStreamError		*errorP, 
		void				*infoP
	) {
		CFBonjourTask		*thiz((CFBonjourTask *)infoP);
		
		CF_ASSERT(serviceRef == thiz->i_net.serviceRef);
		thiz->CB_BonjourTask();
		*errorP = thiz->i_streamErr;
	}

	static void	CB_S_Browse(
		CFNetServiceBrowserRef	serviceRef, 
		CFOptionFlags			flags, 
		CFTypeRef				domainOrService, 
		CFStreamError			*errorP, 
		void					*infoP)
	{
		CFBonjourTask		*thiz((CFBonjourTask *)infoP);
		
		CF_ASSERT(serviceRef == thiz->i_net.browserRef);
		thiz->CB_BonjourTask(flags, domainOrService);
		*errorP = thiz->i_streamErr;
	}
	/*************************/
	
	CCFArray		ConvertAddressArray(CCFArray& addressArray)
	{
		CCFArray		dictArray;
		
		loop (addressArray.size()) {
			CCFData						addressData((CFDataRef)addressArray[(CFIndex)_indexS], true);
			CFNetworkSockAddrUnion		*sockAddrP = (CFNetworkSockAddrUnion *)addressData.begin();
			CCFDictionary				curDict;
			SuperString					tmpStr;
			
			tmpStr.Set(CFNetworkCreateNameFromSockAddress(sockAddrP), false);
			
			if (!tmpStr.empty()) {
				curDict.SetRealValue(kCFBonjourClientKey_ADDRESS,	tmpStr.ref());

				tmpStr.clear();
				tmpStr.append((long)CFSwapInt16BigToHost(sockAddrP->v4.sin_port));
				curDict.SetRealValue(kCFBonjourClientKey_PORT,	tmpStr.ref());
				
				tmpStr.clear();
				if (sockAddrP->header.sa_family == AF_INET) {
					tmpStr.Set(kCFBonjourClientValue_IPv4);
				} else {
					CF_ASSERT(sockAddrP->header.sa_family == AF_INET6);
					tmpStr.Set(kCFBonjourClientValue_IPv6);
				}
				curDict.SetRealValue(kCFBonjourClientKey_TYPE,	tmpStr.ref());
				
				dictArray.push_back(curDict);
			}
		}
		
		return dictArray.transfer();
	}

	
	CFDictionaryRef		GetRegistrationDict()
	{
		CCFDictionary		dict;
		
		dict.SetRealValue(CFSTR("domain"),				CFNetServiceGetDomain(i_net.serviceRef));
		dict.SetRealValue(kCFBonjourClientKey_TYPE,		CFNetServiceGetType(i_net.serviceRef));
		dict.SetRealValue(kCFBonjourClientKey_NAME,		CFNetServiceGetName(i_net.serviceRef));
		dict.SetRealValue(CFSTR("target host"),			CFNetServiceGetTargetHost(i_net.serviceRef));
		
		CCFData			txtData(CFNetServiceGetTXTData(i_net.serviceRef), true);
		
		if (txtData.Get()) {
			CCFDictionary		txtDict(CFNetServiceCreateDictionaryWithTXTData(kCFAllocatorDefault, txtData));

			dict.SetRealValue(CFSTR("txt"), txtDict.Get());
		}
		
		#if 0
		{
			ScCFReleaser<CFHostRef>		cfHost(CFHostCreateWithName(kCFAllocatorDefault, CFSTR("localhost")));
			CFStreamError				streamErr = {0};
			Boolean						resolvedB(CFHostStartInfoResolution(cfHost, kCFHostAddresses, &streamErr));
			
			if (resolvedB) {
				CCFArray					addressArray(CFHostGetAddressing(cfHost, &resolvedB), true);
				
				if (resolvedB) {
					CCFArray		addrDictArray(ConvertAddressArray(addressArray));
					
					dict.SetRealValue(CFSTR("host addresses"),	addrDictArray.ref());
				}
			}
		}
		#endif
		
		return dict.transfer();
	}
	
	bool		GetServiceStrings(SuperString *addressStrP, SuperString *portStrP, SuperString *typeStrP)
	{
		CCFArray		addressArray(CFNetServiceGetAddressing(i_net.serviceRef), true);

		CF_ASSERT(addressStrP			!= NULL);
		CF_ASSERT(portStrP				!= NULL);    
		CF_ASSERT(addressArray.Get()	!= NULL);
		CF_ASSERT(!addressArray.empty());
		
		CCFArray		addrDictArray(ConvertAddressArray(addressArray));
		bool			foundB = !addrDictArray.empty();
		
		if (foundB) {
			CCFDictionary			addrDict(addrDictArray.GetIndValAs_Dict(kCFIndex_0), true);
			
			addressStrP->Set(addrDict.GetAs_String(kCFBonjourClientKey_ADDRESS));
			portStrP->Set(addrDict.GetAs_String(kCFBonjourClientKey_PORT));
			typeStrP->Set(addrDict.GetAs_String(kCFBonjourClientKey_TYPE));
		}

		return foundB;
	}
	
	class CBonjour_Array_Remove {
		CFBonjour			*i_bonjourP;
		const SuperString&	i_stringToCheck;
		CCFArray&			i_array;
		
		public:
		CBonjour_Array_Remove(
			CFBonjour				*bonjourP,
			const SuperString&		stringToCheck
		) : 
			i_bonjourP(bonjourP), 
			i_stringToCheck(stringToCheck),
			i_array(bonjourP->GetBrowseClients())
		{
			CF_ASSERT(i_bonjourP);
		}
		
		void operator()(CFTypeRef valRef) {
			CCFDictionary		theOneToRemove((CFDictionaryRef)valRef, true);
			SuperString			serviceNameStr(theOneToRemove.GetAs_String(kCFBonjourClientKey_NAME));
			
			if (i_bonjourP->IsExtraLogging()) {
				Logf("Bonjour checking %s against %s",
					serviceNameStr.utf8Z(), 
					i_stringToCheck.utf8Z());
			}
		
			if (serviceNameStr == i_stringToCheck) {
				CFIndex		indexI = i_array.GetFirstIndexOfValue(theOneToRemove.Get());
				
				i_array.erase(indexI);

				if (i_bonjourP->IsExtraLogging()) {
					Logf("Bonjour removing: %s\n", i_stringToCheck.utf8Z());
				}
			}
		}
	};
	
	//	CB_S_RegisterResolve
	//	CB_S_Browse
	void		CB_BonjourTask(CFOptionFlags flags = 0, CFTypeRef in_domainOrService = NULL)
	{
		bool		extra_loggingB(i_bonjourP->IsExtraLogging());
	
		switch (i_serviceType) {
            
            default: {
                //  nothing
                break;
            }
				
			case CFBonjour::kServiceType_REGISTER:
			case CFBonjour::kServiceType_RESOLVE: {
				SuperString			addressStr;
				SuperString			portStr;
				SuperString			nameStr;
				SuperString			hostStr;
				const char			*actionZ;
				
				nameStr.Set(CFNetServiceGetName(i_net.serviceRef));
				hostStr.Set(CFNetServiceGetTargetHost(i_net.serviceRef));
				
				if (i_serviceType == CFBonjour::kServiceType_REGISTER) {
					actionZ = "Registering";

					if (extra_loggingB) {
						CCFDictionary		bonjourDict(GetRegistrationDict());
						
						CCFLog(true)(bonjourDict);
					}
				} else {
					SuperString			typeStr;
					
					if (GetServiceStrings(&addressStr, &portStr, &typeStr)) {
						CCFDictionary		bonjourDict;
						
						bonjourDict.SetRealValue(kCFBonjourClientKey_NAME,		nameStr.ref());
						bonjourDict.SetRealValue(kCFBonjourClientKey_ADDRESS,	addressStr.ref());
						bonjourDict.SetRealValue(kCFBonjourClientKey_PORT,		portStr.ref());
						bonjourDict.SetRealValue(kCFBonjourClientKey_HOST,		hostStr.ref());
						bonjourDict.SetRealValue(kCFBonjourClientKey_TYPE,		typeStr.ref());

						if (i_bonjourP->i_clientsArray.GetFirstIndexOfValue(bonjourDict.Get()) == kCFNotFound) {
							i_bonjourP->i_clientsArray.push_back(bonjourDict);
							actionZ = "Resolving";
						}
					} else {
						actionZ = "FAIL: Unknown";
					}
					
					Cancel();
					
					//	after here, i_bonjourP is NULL!!
				}
				
				if (extra_loggingB) {
                    SuperString     formatStr("Bonjour Success: %s: \"%s\" to \"%s\" (%s) on port <%s>\n");
                    
                    formatStr.Smarten();
					Logf(formatStr.utf8Z(),
						 actionZ, nameStr.utf8Z(), hostStr.utf8Z(), addressStr.utf8Z(), portStr.utf8Z());
				}
				break;
			}
							
			case CFBonjour::kServiceType_BROWSE: {
				bool				is_serviceB = (flags & kCFNetServiceFlagIsDomain) == 0;	//	if false, then it's a domain, duh

				if (is_serviceB) {
					CFNetServiceRef		serviceRef((CFNetServiceRef)in_domainOrService);
					SuperString			serviceNameStr(CFNetServiceGetName(serviceRef));
					bool				add_serviceB = (flags & kCFNetServiceFlagRemove) == 0;	//	if false, then it's a removal, duh
					bool				doneB = (flags & kCFNetServiceFlagMoreComing) == 0;		//	if false, then there's more on the way

					if (add_serviceB) {
						SuperString			serviceTypeStr(CFNetServiceGetType(serviceRef));
						SuperString			domainStr(CFNetServiceGetDomain(serviceRef));
						
						i_streamErr = i_bonjourP->NewTask(NULL, CFBonjour::kServiceType_RESOLVE, domainStr, serviceTypeStr, serviceNameStr);
					} else {
						//service needs to be removed
						if (extra_loggingB) {
							Log("Bonjour removing a service");
						}

						//cycle through our found services until we find one whose name matches the one we want to remove
						CCFArray		loop_array(i_bonjourP->i_clientsArray.Copy());
						
						loop_array.for_each(CBonjour_Array_Remove(i_bonjourP, serviceNameStr));
					}
					
					#ifdef _CFTEST_
					if (doneB) {
			//			CFRunLoopStop(CFRunLoopGetCurrent());
					}
					#endif
				} else {
					//	this is a domain
				}
				break;
			}
		}
	}

	/*********************************************************************************************************************************/
	private:
	void				clear()
	{
		i_bonjourP		= NULL;
		i_serviceType	= CFBonjour::kServiceType_NONE;
		i_streamErr		= kCFStreamErrorNone;
		i_scheduledB	= false;
		i_clientSetB	= false;
		i_startedB		= false;
	}
	
	public:
	CFStreamError		GetError()		 { return i_streamErr;	}
	
	virtual				~CFBonjourTask()
	{ 
		if (i_bonjourP) {
			i_bonjourP->RemoveTask(this);
		}
		
		clear();
	}

	CFBonjourTask(
		CFBonjour				*bonjourP, 
		CFBonjour::ServiceType	serviceType,
		CFStringRef				domainStr, 
		CFStringRef				serviceTypeStr, 
		CFStringRef				serviceNameStr, 
		SInt32					portL) 
	{
		CFNetServiceClientContext	context = { 0, this, NULL, NULL, NULL };

		clear();
		
		CF_ASSERT(bonjourP);
		i_bonjourP		= bonjourP;
		i_serviceType	= serviceType;

		i_net.serviceRef = NULL;
		
		//	create and schedule
		switch (i_serviceType) {
                
            default: {
                break;
            }
				
			case CFBonjour::kServiceType_REGISTER:
			case CFBonjour::kServiceType_RESOLVE: {
				i_net.serviceRef = CFNetServiceCreate(kCFAllocatorDefault, domainStr, serviceTypeStr, serviceNameStr, portL);
				
				if (i_net.serviceRef == NULL) {
					i_streamErr.domain = (CFStreamErrorDomain)kCFStreamErrorDomainNetServices;
					i_streamErr.error = kCFNetServicesErrorUnknown;
				} else {
					if (!CFNetServiceSetClient(i_net.serviceRef, CB_S_RegisterResolve, &context)) {
						i_streamErr.domain = (CFStreamErrorDomain)kCFStreamErrorDomainNetServices;
						i_streamErr.error = kCFNetServicesErrorNotFound;	//	-72002
					} else {
						i_clientSetB = true;
					}
				}

				if (i_clientSetB) {
					CFNetServiceScheduleWithRunLoop(i_net.serviceRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
					i_scheduledB = true;
				}
				
				break;
			}
				
			case CFBonjour::kServiceType_BROWSE: {
			    i_net.browserRef = CFNetServiceBrowserCreate(kCFAllocatorDefault, CB_S_Browse, &context);
				
				if (i_net.browserRef == NULL) {
					i_streamErr.domain = (CFStreamErrorDomain)kCFStreamErrorDomainNetServices;
					i_streamErr.error = kCFNetServicesErrorUnknown;
				} else {
			        CFNetServiceBrowserScheduleWithRunLoop(i_net.browserRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
					i_scheduledB = true;
				}
				break;
			}
		}
		
		const char		*serviceStrZ;
		SuperString		nameStr(serviceNameStr);
		SuperString		typeStr(serviceTypeStr);
		const char		*nameZ = nameStr.utf8Z();
		const char		*typeZ = typeStr.utf8Z();
		
		//	start the service
		if (!i_streamErr.error) {
			
			switch (i_serviceType) {
                    
                default: {
                    break;
                }
					
				case CFBonjour::kServiceType_REGISTER: {
					i_startedB = CFNetServiceRegisterWithOptions(i_net.serviceRef, kCFNetServiceFlagNoAutoRename, &i_streamErr);
					serviceStrZ = "Registering";
					break;
				}

				case CFBonjour::kServiceType_RESOLVE: {
					CFTimeInterval		timeoutT = 60;
					
					i_startedB = CFNetServiceResolveWithTimeout(i_net.serviceRef, timeoutT, &i_streamErr);
					serviceStrZ = "Resolving";
					break;
				}
					
				case CFBonjour::kServiceType_BROWSE: {
					i_startedB = CFNetServiceBrowserSearchForServices(i_net.browserRef, domainStr, serviceTypeStr, &i_streamErr);
					serviceStrZ = "Browsing";
					break;
				}
			}
		}

		{
			const char		*succesStrZ;
			
			if (i_startedB) {
				succesStrZ = "";
			} else {
				succesStrZ = "FAILED: ";
			}
			
			if (i_bonjourP->IsExtraLogging()) {
				SuperString		logStr("Bonjour Attempt %s%s: <%s> on \"%s\"");
                
                logStr.Smarten();
                logStr.ssprintf(NULL, succesStrZ, serviceStrZ, nameZ, typeZ);
				
				CCFLog(true)(logStr.ref());
			}

			if (!i_startedB) {
				i_bonjourP->LogStreamErr("Bonjour: start server", i_streamErr);
			}
		}
	}

	/**********************************************/
	void				Cancel(bool skipCancelServiceB = false)
	{
		UNREFERENCED_PARAMETER(skipCancelServiceB);

		if (i_net.typeRef) {
			
			if (i_startedB) {
				i_startedB = false;
				
				switch (i_serviceType) {
						
                    default: {
                        break;
                    }

					case CFBonjour::kServiceType_REGISTER:
					case CFBonjour::kServiceType_RESOLVE: {
						CFNetServiceCancel(i_net.serviceRef);
						break;
					}

					case CFBonjour::kServiceType_BROWSE: {
						CFNetServiceBrowserStopSearch(i_net.browserRef, NULL);
						break;
					}
				}
			}
						
			if (i_scheduledB) {
				i_scheduledB = false;
				
				switch (i_serviceType) {
						
                    default: {
                        break;
                    }

					case CFBonjour::kServiceType_REGISTER:
					case CFBonjour::kServiceType_RESOLVE: {
						CFNetServiceUnscheduleFromRunLoop(i_net.serviceRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);

						if (i_clientSetB) {
							i_clientSetB = false;
							CF_ASSERT(i_serviceType != CFBonjour::kServiceType_BROWSE);
							(void)CFNetServiceSetClient(i_net.serviceRef, NULL, NULL);
						}
						break;
					}

					case CFBonjour::kServiceType_BROWSE: {
						CF_ASSERT(i_clientSetB == false);
						CFNetServiceBrowserUnscheduleFromRunLoop(i_net.browserRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
						break;
					}
				}
			}
			
			CFReleaseDebug(i_net.typeRef);
			i_net.typeRef = NULL;
		}
		
		delete this;
	}
};

/************************************************************************************************/
SuperString		CFBonjour::GetStreamErrStr(const char * valZ, CFStreamError& err)
{
	SuperString		str("<%s> Error: Domain = %s, Code = %ld");
	
	str.ssprintf(NULL, valZ, i_errMap[(int)err.domain], (int)err.error);
	return str;
}

void			CFBonjour::LogStreamErr(const char * valZ, CFStreamError& err)
{
	SuperString		str(GetStreamErrStr(valZ, err));
	
	CFReportUnitTest(str.utf8Z(), -1);
}

void			CFBonjour::RemoveTask(CFBonjourTask *taskP)
{
	CFBonjourTaskSet::iterator		it(i_taskSet.find(taskP));
	
	if (it != i_taskSet.end()) {
		i_taskSet.erase(it);
	}
}

void			CFBonjour::CancelTask(CFBonjourTask **taskPP)
{
	if (*taskPP) {
		(**taskPP).Cancel();
		*taskPP = NULL;
	}
}

CFStreamError		CFBonjour::NewTask(
	CFBonjourTask		**return_taskPP, 
	ServiceType			serviceType, 
	const SuperString&	domainStr, 
	const SuperString&	serviceTypeStr, 
	const SuperString&	serviceNameStr, 
	UInt16				portL)
{
	CFStreamError	err(kCFStreamErrorNone);

	if (return_taskPP == NULL || (*return_taskPP) == NULL) {
		CFBonjourTask	*taskP = new CFBonjourTask(this, serviceType, domainStr, serviceTypeStr, serviceNameStr, portL);
		
		err = taskP->GetError();
		
		if (err.error) {
			taskP->Cancel();
			taskP = NULL;
		}
		
		if (taskP) {
			i_taskSet.insert(taskP);
		}

		if (return_taskPP != NULL) {
			*return_taskPP = taskP;
		}
	}

    return err;
}

/********************************************************/
//	static
CFBonjour*		CFBonjour::s_selfP = NULL;

//	static 
CFBonjour*		CFBonjour::Get()
{
	if (s_selfP == NULL) {
		s_selfP = new CFBonjour();
		
		#ifdef kDEBUG
			s_selfP->SetExtraLogging(true);
		#endif
	}
	
	return s_selfP;
}

CFBonjour::CFBonjour() :
	i_browseTaskP(NULL), 
	i_publishTaskP(NULL)
{
    i_errMap[kCFStreamErrorDomainCustom]				= "kCFStreamErrorDomainCustom";
    i_errMap[kCFStreamErrorDomainPOSIX]					= "kCFStreamErrorDomainPOSIX";
	i_errMap[kCFStreamErrorDomainMacOSStatus]			= "kCFStreamErrorDomainMacOSStatus";
	i_errMap[kCFStreamErrorDomainSSL]					= "kCFStreamErrorDomainSSL";
	i_errMap[kCFStreamErrorDomainHTTP]					= "kCFStreamErrorDomainHTTP";
	i_errMap[kCFStreamErrorDomainSOCKS]					= "kCFStreamErrorDomainSOCKS";
	i_errMap[kCFStreamErrorDomainFTP]					= "kCFStreamErrorDomainFTP";
	i_errMap[kCFStreamErrorDomainNetServices]			= "kCFStreamErrorDomainNetServices";
	i_errMap[kCFStreamErrorDomainMach]					= "kCFStreamErrorDomainMach";
	i_errMap[kCFStreamErrorDomainNetDB]					= "kCFStreamErrorDomainNetDB";
	i_errMap[kCFStreamErrorDomainSystemConfiguration]	= "kCFStreamErrorDomainSystemConfiguration";
	
	#if !COMPILING_ON_10_4
	i_errMap[(SInt32)kCFStreamErrorDomainWinSock]		= "kCFStreamErrorDomainWinSock";
	#endif
	
//	i_errMap[kCFStreamErrorDomainCFHTTPServer]			= "kCFStreamErrorDomainCFHTTPServer";
}

CFBonjour::~CFBonjour()
{
	s_selfP = NULL;	
	StopBrowsing();
	StopPublish();
	
	while (i_taskSet.begin() != i_taskSet.end()) {
		CFBonjourTask	*taskP(*i_taskSet.begin());
		
		CancelTask(&taskP);
	}

	i_taskSet.clear();
	i_errMap.clear();
}
