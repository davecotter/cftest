/*
 *  CFUtils.cpp
 *  CFTest
 *
 *  Created by David M. Cotter on 7/23/08.
 *  Copyright 2008 David M. Cotter. All rights reserved.
 *
 */
#include "stdafx.h"
#include "CFStackTrace.h"
#include "MinMax.h"

#ifdef _KJAMS_
	#include "CApp.h"
	#include "PrefKeys_p.h"
#endif

#if defined(_KJAMS_) && !defined(_KJAMSX_)
	#include "FSRefUtils.h"
	#include "MessageAlert.h"
#else
	#ifdef _CFTEST_
		#include <algorithm>

		#if !defined(_HELPERTOOL_) && !_QTServer_ && !_PaddleServer_
			#include "CFTestUtils.h"

			#if _QT_ && !defined(_MIN_CF_)
				#include "FSRefUtils.h"
			#endif
		#else
			#define SSLocalize(_foo, _bar)	_foo
		#endif

		#if _QTServer_
			#include "Script.h"
		#endif
	#else
		#include "XConfig.h"

		#if !defined(_HELPERTOOL_)
			#include "FSRefUtils.h"
			#include "CFileRef.h"
		#endif

		#ifdef _ROSE_
			#define SSLocalize(_foo, _bar)	_foo
		#endif
	#endif

	#include "SuperString.h"
	#include <CoreFoundation/CFDateFormatter.h>

	#if !defined(_HELPERTOOL_)// && !_QTServer_
		#include <CFNetwork/CFHTTPMessage.h>
	#endif

	#include <CoreFoundation/CFUserNotification.h>
	#include <CoreFoundation/CFNumberFormatter.h>
	#include <CoreFoundation/CFCalendar.h>
#endif

#ifndef _CFTEST_
	#if OPT_KJMAC || _QT_
		#include <CFNetwork/CFHTTPMessage.h>
		#include <CoreFoundation/CFUserNotification.h>
		#include "MessageAlert.h"
		#include "CLocalize.h"
	#else
		#include <XErrorDialogs.h>
	#endif
#endif

#if OPT_MACOS
	#if _QT_ || _YAAF_ || _CFTEST_
		#include "sys/sysctl.h"
	#endif
#else

	#if defined(_KJAMS_) || defined(_QTServer_)
		#include "FSRefUtils.h"
		#include "CFileRef.h"
	#endif

	#if !_QT_
		#ifdef _DEBUG
			#define		kCFLiteLib	"CFLite Debug"
		#else
			#define		kCFLiteLib	"CFLite"
		#endif
		
		//	this parkles the Qt compiler, and is unnecessary
		//	since it's spec'd in the .pro file
		#pragma comment(lib, kCFLiteLib ".lib")
	#endif
#endif
	
#include "CCFData.h"

bool	CFGetLogging();

bool	s_breakOnFNF = false;

#ifndef _KJAMS_
static	bool	s_insensB = true;
#endif

const char ENTER_STR[2]		=	{ ENTER_KEY, 0x00 };
const char RETURN_STR[2]	=	{ RETURN_KEY, 0x00 };
const char LINEFEED_STR[2]	=	{ LINEFEED_KEY, 0x00 };

void	CFLogArgs(int argc, const char *argv[])
{
	loop (argc) {
		SuperString		formatStr("arg%d: %s\n");
		
		formatStr.ssprintf(NULL, (int)_indexS, argv[_indexS]);
		CCFLog()(formatStr.ref());
	}
}

void	CFLogTime()
{
	SuperString			str;
	
	str.Set(CFAbsoluteTimeGetCurrent(), SS_Time_LOG);
	Logf("Date/Time:     %s\n", str.utf8Z());
}

void	LogRect(const char *msg, const Rect& rectR)
{
	Logf("** Rect: %s\n"
		"\tLeft:   %d\n"
		"\tTop:    %d\n"
		"\tWidth:  %d\n"
		"\tHeight: %d\n", 
		msg, rectR.left, rectR.top, rectR.right - rectR.left, rectR.bottom - rectR.top); 
}

void	CFSetDiacriticInsensitive(bool insensB)
{
	#ifdef _KJAMS_
		if (gApp && gApp->i_prefs) {
			gApp->i_prefs->i_dict.SetValue(DIACRITIC_INSENSITIVE, insensB);
			gApp->i_prefs->i_diacritic_insensB = insensB;
		}
	#else
		s_insensB = insensB;
	#endif
}

bool	CFGetDiacriticInsensitive()
{
	bool	insensB = true;
	
	#ifdef _KJAMS_
		insensB = !gApp || !gApp->i_prefs || gApp->i_prefs->i_diacritic_insensB;
	#else
		insensB = s_insensB;
	#endif
	
	return insensB;
}

#if defined(_QTServer_)
	#include "CarbonEventsCore.h"
#endif
const CFTimeInterval kCFAbsoluteTimeIntervalOneMonth	= kEventDurationMonth;

void	FilterErr(OSStatus err, bool from_exceptionB)
{
	#if !defined(kDEBUG) || defined(_MIN_CF_)
		UNREFERENCED_PARAMETER(err);
		UNREFERENCED_PARAMETER(from_exceptionB);
	#else
	if (err) {
		//if (err == errDataBrowserItemNotFound) {
		//	err = 1;
		//}
		
		if (from_exceptionB) {
			int i = 0;	++i;
		}

		if (err == dupFNErr || err == fnfErr) {
			static	bool s_breakB = false;
			
			if (s_breakB && s_breakOnFNF) {
				CFDebugBreak();
			}
		}
	}
	#endif
}

#if defined(__WIN32__)
	static	bool	s_setLogB = false;
#endif

// static
CFStringRef		CCFLog::i_logPathRef = NULL;
//static
void		CCFLog::SetLogPath(CFStringRef logPathRef)
{
	if (logPathRef) {
		SuperString		pathStr(logPathRef);
		
		#ifdef _QTServer_
			pathStr.Replace(":", ";");
			pathStr.Replace(";\\", ":\\");
		#else
			pathStr.MakeRecoverable(CFRecoverType_EXCLUDE_SLASHES_DASHES);
		#endif

		logPathRef = pathStr.Retain();
	}
	
	if (i_logPathRef) {
		CFReleaseDebug(i_logPathRef);
	}
	
	i_logPathRef = logPathRef; 
}

#if defined(_KJAMS_)
boost::thread_specific_ptr<bool> 	s_last_crB;
#endif

#if (OPT_WINOS || defined(_CFTEST_) || defined(_MIN_CF_))
	#define	kUseCFLogger		1
#else
	#define	kUseCFLogger		0
#endif


#define		kLogWithTime		0

#if !kUseCFLogger || defined(_KJAMSX_) || _QT_

#if defined(_KJAMS_)
SuperString			CurThreadIndexStr();
#endif

static void			PrependThreadNumber(SuperString& str, bool crB)
{
#if defined(_KJAMS_)
	if (s_last_crB.get() == NULL) {
		s_last_crB.reset(new bool(true));
	}
	
	bool&		was_crB(*(s_last_crB.get()));
	bool		is_crB = crB || str.GetIndCharR() == '\n';
							
	if (was_crB) {
		SuperString			prefixStr(CurThreadIndexStr());
		
		prefixStr.append(": ");
		
		#if kLogWithTime
		{
			SuperString			timeStr;
			
			timeStr.Set(CFAbsoluteTimeGetCurrent(), SS_Time_LOG);
			prefixStr.append(timeStr);
			prefixStr.append(": ");
		}
		#endif
		
		str.prepend(prefixStr);
	}
	
	was_crB = is_crB;
#endif
}
#endif

#ifdef _H_CFileRef
static CFileRef			s_cfileRef;
#endif

void	CCFLog::trim()
{
	#ifdef _KJAMS_
	IX_START {
		#if kUseCFLogger
		#if OPT_WINOS
		{
			CDlogScopeProgress		sc(SSLocalize("Trimming Log filesÉ", ""), true);
			CFileRef				folderRef(CFileRef::kFolder_LOGS);
			FSRefVec				refVec;
			SuperString				elipsisStr("É");
			
			sc.Message(SSLocalize("Counting", "enumerating, tallying") + elipsisStr);
			ETX(FSrGetCatInfoBulk(folderRef, refVec));
			
			if (refVec.size() > 5) {
				sc.Message(SSLocalize("Sorting", "collating, orgainzing") + elipsisStr);
				std::sort(refVec.begin(), refVec.end(), FSr_ForEach_SortFilesByDate(false));

				sc.SetMinMax(0, refVec.size());
				sc.Message(SSLocalize("Trimming", "pruning, cropping, paring down") + elipsisStr);
				sc.SetCur(5);

				FSRefVec::iterator		ref_it(refVec.begin() + 5);
				CFileRef				cFile;

				while (ref_it != refVec.end()) {
					cFile.SetRef(*ref_it);
					cFile.Delete();
					sc.Inc();
					++ref_it;
				}
			}
		}
		#endif // OPT_WINOS
		#else // !kUseCFLogger
		{
			if (s_cfileRef.Exists()) {
				
				//	trash it after it gets too big
				if (s_cfileRef.size() > 20 * kMegaByte) {
					(void)s_cfileRef.Delete(true);
					s_cfileRef.SetPath(GetConsoleFilePath());	//	causes log file to be created
				}
			}
		}
		#endif // !kUseCFLogger
	} IX_END;
	#endif // _KJAMS_
}

static	FILE	*s_logP = NULL;

FILE*	CCFLog_GetLogFile();
FILE*	CCFLog_GetLogFile()
{
	return  s_logP;
}

void CCFLog::operator()(CFTypeRef valRef) {
	
	#if defined(__WIN32__)
		#define		USE_CFSHOW		0

		if (!s_setLogB) {
			s_setLogB = true;
			//	CFSetLogFile(CFSTR("CF_Log.txt"), kCFStringEncodingUTF8);
		}
	#else
		#define		USE_CFSHOW		0
	#endif

	#if !USE_CFSHOW
	{
		if (!CFGetLogging()) return;
		
		SuperString			valStr;	valStr.Set_CFType(valRef);
		FILE				*log_fileP = stdout;
		
		if (i_crB) {
			valStr.append("\n");
		}
		
		#if defined(__WIN32__)
			
			//	GUI APP
			
			if (s_logP == NULL) {
				int				tryLastErr, tryErrNo;
				SuperString		tryPathStr;
				SuperString		pathStr;

				if (GetLogPath()) {
					pathStr.Set(GetLogPath());
					tryPathStr = pathStr;
				} else {
					SuperString						relPathStr("../");
					CFBundleRef						bundleRef(CFBundleGetMainBundle());
					CCFURL							urlRef(CFBundleCopyExecutableURL(bundleRef));
					SuperString						testRelPath(CFURLCopyLastPathComponent(urlRef), false);

					#if 0
						testRelPath.pop_ext();
						testRelPath.append(" ");
						testRelPath.append(kJams_LogFileName);
					#else
						testRelPath.Set(kJams_LogFileName);
					#endif

					testRelPath.prepend(relPathStr);

					CCFURL							bundleUrlRef(CFBundleCopyBundleURL(bundleRef));
					CCFURL							xmlUrlRef(CFURLCreateWithFileSystemPathRelativeToBase(
						kCFAllocatorDefault, testRelPath.ref(), kCFURLPOSIXPathStyle, false, bundleUrlRef));
					CCFURL							absUrlRef(CFURLCopyAbsoluteURL(xmlUrlRef));

					pathStr.Set(CFURLCopyFileSystemPath(absUrlRef, kCFURLPlatformPathStyle), false);
				}

				#if _YAAF_
					if (!pathStr.empty())
				#endif
				s_logP = _wfsopen(pathStr.w_str(), L"a", _SH_DENYWR);

				if (s_logP == NULL) {
					SetLogPath(NULL);
					tryLastErr = GetLastError();
					tryErrNo = errno;

					SuperString		errStr("Failed to open log file (last err: %d) (errno: %d): <%s>");

					errStr.ssprintf(NULL, (int)tryLastErr, (int)tryErrNo, pathStr.utf8Z());

					CFUserNotificationDisplayNotice(
						0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL,
						CFSTR("Error"), errStr.ref(), NULL);
				} else {
					Logf("opened log file at <%s>\n", pathStr.utf8Z());
					CFSetLogFile(s_logP);
				}
			}
			
			log_fileP = s_logP;
	
		#endif	//	defined(__WIN32__)
		
		if (log_fileP) {
			
			#if defined(_KJAMSX_) || _QT_
				#if OPT_WINOS
					valStr.Replace(RETURN_STR, "");
				#endif

				PrependThreadNumber(valStr, i_crB);

				valStr.ScrubSensitiveInfo();

				fprintf(log_fileP, "%s", valStr.utf8Z());
				fflush(log_fileP);

				#if OPT_WINOS
					OutputDebugString(valStr.w_str());
				#endif
			#else
				#if _CFTEST_
					#if __WIN32__
						valStr.Replace("\r", "");
						OutputDebugString(valStr.w_str());
					#endif

					fprintf(log_fileP, "%s", valStr.utf8Z());
					fflush(log_fileP);
				#else
					Log(valStr.utf8Z(), false);
				#endif
			#endif
		}
	}
	#else	//	!USE_CFSHOW
	{
		#ifndef __WIN32__
			fflush(stdout);
		#endif
	
		CF_ASSERT("not scrubbing sensitive info" == NULL);
		//valStr.ScrubSensitiveInfo();
	
		CFShow(valRef);
		if (i_crB) {
			CFShow(CFSTR("\n"));
		}

		#ifndef __WIN32__
			fflush(stdout);
		#endif
	}
	#endif	//	!USE_CFSHOW
}

void CCFLog::operator()(CFStringRef keyRef, CFTypeRef valRef) {
	SuperString		keyStr(keyRef);
	
	keyStr.append(": ");
	
	{
		bool	wasB = i_crB;
		i_crB = false;
		
		operator()(keyStr.ref());
		i_crB = wasB;
	}
	
	operator()(valRef);
}

//	static
void	CCFLog::close()
{
	::fclose(s_logP);
	s_logP = NULL;

	#if defined(__WIN32__)
		CFSetLogFile(NULL);
	#endif
}

/*****************************************************************************/

#if OPT_KJMAC
	#include "CocoaFunctions.h"
#else

CFXMLTreeRef	CCFXMLTreeCreateFromData(
	CFDataRef		xmlData, 
	CFURLRef		dataSource, 
	CFOptionFlags	parseOptions, 
	CFDictionaryRef	*errorDictP)
{
	return CFXMLTreeCreateFromDataWithError(
		kCFAllocatorDefault, 
		xmlData, 
		dataSource, 
		parseOptions, 
		kCFXMLNodeCurrentVersion,
		errorDictP);
}

CFDataRef		CCFPropertyListCreateXMLData(
	CFPropertyListRef	plist, 
	CFErrorRef			*errorRefP0)
{
	return CFPropertyListCreateXMLData(kCFAllocatorDefault, plist);
}

#endif

/********************************************************/
bool	Read_PList(const CFURLRef &url, CFDictionaryRef *plistP)
{
	bool			successB = false;
    OSStatus        err = noErr;
    CCFData	  		xmlData;
	
    XTE_START {
        CCFData	  		local_xmlData(url);

        xmlData.adopt(local_xmlData.transfer());
    } XTE_END;

    *plistP = NULL;
		
	if (xmlData.Get()) {
		//Log("created xml from file");
		
		*plistP = xmlData.CopyAs_Dict(true);
		
		successB = *plistP != NULL;
		
		if (successB) {
			//	Log("created plist from xml");
		} else {
			//	CCFLog(true)(CFSTR("FAILED converting xml to plist"));
		}
	} else {
		//CCFLog(true)(CFSTR("FAILED creating xml from file"));
	}
	
	return successB;
}

OSStatus		Write_PList(
	CFPropertyListRef	plist,
	CFURLRef			urlRef)
{
	OSStatus			err	= noErr;
	CCFData				xmlData;
	
	// Convert the property list into XML data.
	xmlData.Set((CFDictionaryRef)plist);
	ETRL(xmlData.Get() == NULL, "creating xml data");
	
	if (!err) {
		(void)CFURLWriteDataAndPropertiesToResource (
			urlRef,		// URL to use
			xmlData,	// data to write
			NULL,   
			&err);
	}
	
	ETRL(err, "writing xml");
	
	return err;
}

double		CFNumberToDouble(const CFNumberRef &numRef)
{
	double	valueDF = 0;
	
	CF_ASSERT(CFGetTypeID(numRef) == CFNumberGetTypeID());
	CFNumberGetValue(numRef, kCFNumberDoubleType, &valueDF);
	return valueDF;
}

OSErr	CFDictionaryCreate(CFMutableDictionaryRef *dictP)
{
	OSErr		err = noErr;
	
	*dictP = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
	
	if (*dictP == NULL) {
		err = 1;
	}
	
	return err;
}

OSErr	CFArrayCreate(CFMutableArrayRef *arrayP)
{
	OSErr		err = noErr;
	
	*arrayP = CFArrayCreateMutable(
		kCFAllocatorDefault, 0,
		&kCFTypeArrayCallBacks);
	
	if (*arrayP == NULL) {
		err = 1;
	}
	
	return err;
}

/******************************************************/

bool	CCFXmlTree::CreateFromData(CFDataRef xmlData, CFURLRef dataSource, CFOptionFlags parseOptions)
{
	bool			successB = false;
	
	if (xmlData) {
		bool			attempt_macRoman_to_UTF8B = (parseOptions & kCFXMLParserAttemptConvertMacRomanToUTF8) != 0;
		
		if (attempt_macRoman_to_UTF8B) {
			parseOptions &= ~((CFOptionFlags)kCFXMLParserAttemptConvertMacRomanToUTF8);
		}
		
		CCFDictionary	dict(NULL, true);
		CFXMLTreeRef	treeRef(CCFXMLTreeCreateFromData(
			xmlData, 
			dataSource, 
			parseOptions, 
			dict.ImmutableAddressOf()));

		successB = treeRef != NULL;

		if (!successB && attempt_macRoman_to_UTF8B) {
			SuperString			str;
			
			str.Set(xmlData, kCFStringEncodingMacRoman);
			
			CCFData			ccfData(str.CopyDataRef());
			CCFDictionary	newErrDict(NULL, true);
			
			treeRef = CCFXMLTreeCreateFromData(
				ccfData, 
				dataSource, 
				parseOptions, 
				newErrDict.ImmutableAddressOf());
			
			successB = treeRef != NULL;
			
			if (successB) {
				dict.clear();
				
			} else if (newErrDict.Get()) {
				dict.adopt(newErrDict.transfer());
			}
		}

		_inherited::adopt(treeRef);
		
		if (!successB) {
		
			if (dict.Get() && !dict.empty()) {
				//	error info is in the dict if it fails
				CCFLog(true)(dict);
			} else {
				SuperString		urlStr(dataSource);
				SuperString		str("Illegal XML file");
				
				#if defined(_KJAMS_)
					PostAlert(str.utf8Z(), urlStr.utf8Z());
				#else
					Logf("%s: %s\n", str.utf8Z(), urlStr.utf8Z());
				#endif
			}
		}
	}

	return successB;
}

CFXMLTreeRef		CCFXmlTree::GetChild(const char *childKeyZ)
{
	return GetChild(SuperString(uc(childKeyZ)).ref());
}

void				CCFXmlTree::ParseEntityRef(CCFString& physRef, CCFXMLNode& node, short levelS)
{
	UNREFERENCED_PARAMETER(levelS);

	//	CFXMLEntityReferenceInfo		*refInfoP = (CFXMLEntityReferenceInfo *)node.GetInfoPtr();
	SuperString						nodeStr(node.GetString());
	
	if (!nodeStr.empty()) {
		nodeStr = SuperString("&") + nodeStr + SuperString(";");
		nodeStr.UnEscapeAmpersands();

		SuperString		physStr(physRef);
		
		physStr.append(nodeStr);
		physRef.SetAndRetain(physStr);
	}
}

#if OPT_WINOS
#ifdef _CFTEST_
#ifndef _QTServer_
#if !_QT_ || defined(_JUST_CFTEST_)
#define		kLOCAL_ConvertToShort
#endif
#endif
#endif
#endif

SuperString		Win_ConvertToShortName(const SuperString& filePath)
#ifdef kLOCAL_ConvertToShort
{
	return filePath;
}
#else
;
#endif


CFURLRef		CFURLCreateWithPathStr(const SuperString& in_pathStr)
{
	SuperString		pathStr(in_pathStr);
	
	#if OPT_WINOS
		pathStr = Win_ConvertToShortName(pathStr);
		pathStr.Update_utf8(CFStringGetSystemEncoding());
	#endif
	
	CFURLRef		urlRef = CFURLCreateFromFileSystemRepresentation(
		kCFAllocatorDefault, pathStr.utf8().c_str(), pathStr.size(), false);
	
	return urlRef;
}

bool	Read_XML(const CFURLRef url, CCFXmlTree& xml, bool attempt_macRoman_to_UTF8B)
{
	bool		successB = false;
	CCFData		xmlData;
	
	IX(xmlData = CCFData(url, attempt_macRoman_to_UTF8B));
	
	if (xmlData.Get()) {
	
		#if 0	//	def kDEBUG
		this is HUUUUUGE for karaoke locker
			SuperString		str(xmlData);
			
			Logf("%s\n", str.utf8Z());
			Logf("size: %d\n", xmlData.size());
		#endif
		
		CFOptionFlags		parseOptions = kCFXMLParserSkipWhitespace;
		
		if (attempt_macRoman_to_UTF8B) {
			parseOptions |= kCFXMLParserAttemptConvertMacRomanToUTF8;
		}
		
		successB = xml.CreateFromData(xmlData.Get(), url, parseOptions);
	}
	
	return successB;
}

bool	Read_XML(const SuperString &pathStr, CCFXmlTree& xml)
{
	bool				successB = false;
	CCFURL				url(CFURLCreateWithPathStr(pathStr));
	
	if (url.Get()) {
		successB = Read_XML(url.Get(), xml);
	}
	
	return successB;
}

/********************************************************/
CCFDictionary::CCFDictionary(const SuperString& str, bool jsonB)
{
	if (jsonB) {
		SetJSON(str);
	} else {
		SetXML(str);
	}
}

CFMutableDictionaryRef		CCFDictionary::Copy()
{
	return CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, Get());
}

class CSimpleCopy {
	CCFDictionary	i_dest;

	public:
	CSimpleCopy(CFMutableDictionaryRef destDictRef) : i_dest(destDictRef) { }
	void operator()(CFStringRef keyRef, CFTypeRef valRef) {
		i_dest.SetValue(keyRef, valRef);
	}
};

void		CCFDictionary::CopyTo(CFMutableDictionaryRef destDictRef)
{
	for_each(CSimpleCopy(destDictRef));
}

bool				CCFDictionary::ContainsKey(CFStringRef keyRef)
{
	bool	containsB = !!CFDictionaryContainsKey(Get(), keyRef);

	return containsB;
}

bool				CCFDictionary::ContainsKey(const char *utf8Z)
{
	SuperString		keyStr(uc(utf8Z));
	bool			containsB = false;
	
	if (!keyStr.Contains(kCFString_KeySeparator)) {
		containsB = ContainsKey(keyStr.ref());
		
	} else {
		containsB = CFType_ContainsKey(ref(), keyStr);
	}
	
	return containsB;
}

CFTypeRef	CCFDictionary::GetValue(const char* keyZ)
{
	SuperString		keyStr(uc(keyZ));

	return GetValue(keyStr.ref());
}

CFStringRef			CCFDictionary::GetAs_String	(CFStringRef keyRef)
{
	CFStringRef		stringRef((CFStringRef)GetValue(keyRef));
	
	if (stringRef) {
		CFTypeEnum		typeEnum(CFGetCFType(stringRef));
		
		CF_ASSERT(typeEnum == CFType_STRING);
	}

	return stringRef;
}

CFStringRef			CCFDictionary::GetAs_String	(OSType key)
{
	return GetAs_String(OSTypeToString(key).utf8Z());
}

CFStringRef			CCFDictionary::GetAs_String	(const char *keyZ)
{
	SuperString		keyStr(uc(keyZ));

	return GetAs_String(keyStr);
}

SuperString			CCFDictionary::GetAs_SString	(const char *utf8Z)
{
	return SuperString(GetAs_String(utf8Z));
}

CFAbsoluteTime		CCFDictionary::GetAs_TimeFromString(const char *utf8Z, SS_TimeType timeType, CFTimeInterval epochT)
{
	SuperString			str(GetAs_String(utf8Z));
	
	return str.GetAs_CFAbsoluteTime(timeType, epochT);
}

void				CCFDictionary::SetValue_TimeToString(const char *utf8Z, CFAbsoluteTime valueT, SS_TimeType timeType, CFTimeInterval epochT)
{
	SuperString			str;
	
	str.Set(valueT, timeType, epochT);
	SetValue(utf8Z, str);
}

CFTypeRef			CCFArray::GetIndValAs_CFType(CFIndex idx)
{
	CFTypeRef		typeRef(operator[](idx));
	
	if (typeRef == NULL) {
		ETX(invalidIndexErr);
	}
	
	return typeRef;
}

CFDictionaryRef			CCFArray::GetIndValAs_Dict(CFIndex idx) 
{
	CFTypeRef		typeRef(GetIndValAs_CFType(idx));
	
	CF_ASSERT(CFGetTypeID(typeRef) == CFDictionaryGetTypeID());
	return (CFDictionaryRef)typeRef;
}

CFArrayRef				CCFArray::GetIndValAs_Array(CFIndex idx) 
{
	CFTypeRef		typeRef(GetIndValAs_CFType(idx));
	
	CF_ASSERT(CFGetTypeID(typeRef) == CFArrayGetTypeID());
	return (CFArrayRef)typeRef;
}

CFTypeRef		CFType_GetValFromPathKey(
	CFTypeRef		thisRef, 
	SuperString		curKey)
{
	CFTypeRef			returnRef = NULL;
	
	if (!curKey.Contains(kCFString_KeySeparator)) {
	
		if (CFGetTypeID(thisRef) == CFDictionaryGetTypeID()) {
			CCFDictionary	cfDict((CFDictionaryRef)thisRef, true);
			
			returnRef = cfDict.GetValue(curKey.ref());
			
		} else if (curKey.IsNumeric()) {
			CCFArray		cfArray((CFArrayRef)thisRef, true);
			CFIndex			valIndex(curKey.GetAs_SInt32());
			
			returnRef = cfArray.GetIndValAs_CFType(valIndex);
		}
	} else {
		SuperString		pathTail;
		SuperString		nextKey;
		
		//	pop "curKey" from the front, leaving "nextKey" as the rest of the path
		curKey.Split(kCFString_KeySeparator, &pathTail);

		//	grab "nextKey" from tail
		nextKey = pathTail;
		nextKey.Split(kCFString_KeySeparator);
		
		CFTypeRef			nextRef = NULL;

		//	get the next node (ref) in the path
		if (curKey.IsNumeric()) {
		
			if (CFGetTypeID(thisRef) == CFArrayGetTypeID()) {
				CCFArray			thisA((CFArrayRef)thisRef, true);
				CFTypeID			nextTypeID = 0;
				
				IX(nextRef = thisA.GetIndValAs_CFType(curKey.GetAs_SInt32()))

				if (nextRef) {

					if (nextKey.IsNumeric()) {
						nextTypeID = CFArrayGetTypeID();

					} else {
						nextTypeID = CFDictionaryGetTypeID();
					}
					
					if (CFGetTypeID(nextRef) != nextTypeID) {
						nextRef = NULL;
					}
				}
			}

		} else {
			
			if (CFGetTypeID(thisRef) == CFDictionaryGetTypeID()) {
				CCFDictionary		thisDict((CFDictionaryRef)thisRef, true);

				if (nextKey.IsNumeric()) {
					nextRef = thisDict.GetAs_Array(curKey);

				} else {
					nextRef = thisDict.GetAs_Dict(curKey);
				}
			}
		}

		if (nextRef) {
			//	be recursive:
			returnRef = CFType_GetValFromPathKey(nextRef, pathTail);
		}
	}

	return returnRef;
}

bool		CFType_ContainsKey(
	CFTypeRef		thisRef,
	SuperString		curKey)
{
	CFTypeRef		foundRef(CFType_GetValFromPathKey(thisRef, curKey));
	bool	 		containsB(foundRef != NULL);
	
	return containsB;
}

CFDictionaryRef			CCFArray::GetIndValAs_Dict(const char *path_utf8Z)
{
	return (CFDictionaryRef)CFType_GetValFromPathKey(ref(), uc(path_utf8Z));
}

CFArrayRef				CCFArray::GetIndValAs_Array(const char *path_utf8Z)
{
	return (CFArrayRef)CFType_GetValFromPathKey(ref(), uc(path_utf8Z));
}

CFArrayRef			CCFDictionary::GetAs_Array	(const char *path_utf8Z)
{
	return (CFArrayRef)CFType_GetValFromPathKey(ref(), uc(path_utf8Z));
}

CFDictionaryRef			CCFDictionary::GetAs_Dict(const char *path_utf8Z)
{
	return (CFDictionaryRef)CFType_GetValFromPathKey(ref(), uc(path_utf8Z));
}

CFDictionaryRef			CCFDictionary::GetAs_Dict(CFStringRef key)
{
	CFDictionaryRef		dictRef(reinterpret_cast<CFDictionaryRef>(GetValue(key)));

	if (dictRef) {
		CF_ASSERT(CFGetTypeID(dictRef) == CFDictionaryGetTypeID());
	}
	
	return dictRef;
}

CFMutableDictionaryRef	CCFDictionary::GetAs_MutableDict(const char *utf8Z)
{
	return (CFMutableDictionaryRef)GetAs_Dict(utf8Z);
}

void				CCFDictionary::SetValue_OSType(const char *utf8Z, OSType value)
{
	SetValue(utf8Z, OSTypeToString(value));
}

OSType				CCFDictionary::GetAs_OSType(const char *utf8Z)
{
	return SuperString(GetAs_String(utf8Z)).GetAs_OSType();
}

SInt32				CCFDictionary::GetAs_SInt32	(const char *utf8Z)
{
	CFNumberRef		valRef((CFNumberRef)GetValue(utf8Z));
	SInt32			valL = 0;

	if (valRef) {
		CFTypeEnum		typeEnum(CFGetCFType(valRef));
		
		CF_ASSERT(typeEnum == CFType_NUMBER_INT);
		CFNumberGetValue(valRef, kCFNumberSInt32Type, &valL);
	}

	return valL;
}

SInt16				CCFDictionary::GetAs_SInt16	(const char *utf8Z, SInt16 defaultS)
{
	CFNumberRef		valRef((CFNumberRef)GetValue(utf8Z));
	SInt16			valL(defaultS);

	if (valRef) {
		CF_ASSERT(CFGetTypeID(valRef) == CFNumberGetTypeID());
		CFNumberGetValue(valRef, kCFNumberSInt16Type, &valL);
	}

	return valL;
}

SInt16				CCFDictionary::GetAs_SInt16	(OSType key)
{
	return GetAs_SInt16(OSTypeToString(key).utf8Z());
}

CFDateRef			CCFDictionary::GetAs_Date	(const char *utf8Z)
{
	CFDateRef		dateRef((CFDateRef)GetValue(utf8Z));
	
	if (dateRef) {
		CF_ASSERT(CFGetTypeID(dateRef) == CFDateGetTypeID());
	}

	return dateRef;
}

CFGregorianDate		CCFDictionary::GetAs_GregDate(const char *utf8Z)
{
	CFGregorianDate		gregDate;	structclr(gregDate);
	CFDateRef			dateRef(GetAs_Date(utf8Z));
	
	if (dateRef) {
		gregDate = CFDateGetGregorian(dateRef);
	}
	
	return gregDate;
}

void				CCFDictionary::SetValue(const char *utf8Z, const CFGregorianDate& in_gregDate)
{
	CFGregorianDate				gregDate(in_gregDate);
	
	if (gregDate.year == 0) {
		gregDate.year		= 2000;
		gregDate.month		= 1;
		gregDate.day		= 1;
	}
	
	ScCFReleaser<CFDateRef>		dateRef(CFDateCreateWithGregorian(gregDate));
	
	SetValue(utf8Z, dateRef.Get());
}

CFArrayRef			CCFDictionary::GetAs_Array	(CFStringRef keyStr)
{
	CFArrayRef			arrayRef((CFArrayRef)GetValue(keyStr));
	
	if (arrayRef) {
		CF_ASSERT(CFGetTypeID(arrayRef) == CFArrayGetTypeID());
	}
	
	return arrayRef;
}

CFDataRef			CCFDictionary::GetAs_Data	(const char *utf8Z)
{
	CFDataRef			dataRef((CFDataRef)GetValue(utf8Z));
	
	if (dataRef) {
		CF_ASSERT(CFGetTypeID(dataRef) == CFDataGetTypeID());
	}
	
	return dataRef;
}

OSStatus		CFWriteDataToURL(const CFURLRef urlRef, CFDataRef dataRef)
{
	OSStatus	err = noErr;

	(void)CFURLWriteDataAndPropertiesToResource (
		urlRef,			// URL to use
		dataRef,		// data to write
		NULL,			// dictRef meta properties to write
		&err);
	
	return err;
}

void			CCFDictionary::RemoveValue(CFStringRef key)
{
	CFDictionaryRemoveValue(Get(), key);
}

void			CCFDictionary::RemoveValue(const char *utf8Z)
{
	SuperString		keyStr(uc(utf8Z));
	
	RemoveValue(keyStr.ref());
}

void			CCFDictionary::RemoveValue(OSType osType)
{
	RemoveValue(OSTypeToString(osType).ref());
}


/*****************************************/
Rect			CCFDictionary::GetAs_Rect		(const char *utf8Z)
{
	Rect				frameR; structclr(frameR);
	CCFDictionary		dict(GetAs_Dict(utf8Z), true);
	
	if (dict.Get()) {
		frameR.left		= dict.GetAs_SInt16(DICT_STR_RECT_LEFT);
		frameR.right	= dict.GetAs_SInt16(DICT_STR_RECT_RIGHT);
		frameR.top		= dict.GetAs_SInt16(DICT_STR_RECT_TOP);
		frameR.bottom	= dict.GetAs_SInt16(DICT_STR_RECT_BOTTOM);
	}
	
	return frameR;
}

void			CCFDictionary::SetValue(const char *utf8Z, const Rect& frameR)
{
	CCFDictionary	dict;
	
	dict.SetValue(DICT_STR_RECT_LEFT,	frameR.left);
	dict.SetValue(DICT_STR_RECT_RIGHT,	frameR.right);
	dict.SetValue(DICT_STR_RECT_TOP,	frameR.top);
	dict.SetValue(DICT_STR_RECT_BOTTOM,	frameR.bottom);

	SetValue(utf8Z, dict.Get());
}

/*****************************************/
bool				CCFDictionary::GetAs_Bool		(const char *utf8Z, bool defaultB)
{
	bool				valB	= defaultB;
	CFBooleanRef		boolRef	= (CFBooleanRef)GetValue(utf8Z);
	
	if (boolRef) {
		CF_ASSERT(CFGetTypeID(boolRef) == CFBooleanGetTypeID());
		valB = boolRef == kCFBooleanTrue;
	}

	return valB;
}

void				CCFDictionary::SetValue			(const char *utf8Z, bool valB)
{
	SetValue(utf8Z, valB ? kCFBooleanTrue : kCFBooleanFalse);
}
/*****************************************/
float				CCFDictionary::GetAs_Float		(const char *utf8Z)
{
	float				valF = 0;
	CFNumberRef			valRef((CFNumberRef)GetValue(utf8Z));
	
	if (valRef) {
		CF_ASSERT(CFGetTypeID(valRef) == CFNumberGetTypeID());
		CFNumberGetValue(valRef, kCFNumberFloatType, &valF);
	}
		
	return valF;
}

void				CCFDictionary::SetValue			(const char *utf8Z, float valF)
{
	ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &valF));

	SetValue(utf8Z, numberRef.Get());
}

/*****************************************/
double				CCFDictionary::GetAs_Double		(const char *utf8Z)
{
	double				valDF = 0;
	CFNumberRef			valRef((CFNumberRef)GetValue(utf8Z));
	
	if (valRef) {
		valDF = CFNumberToDouble(valRef);
	}
		
	return valDF;
}

void				CCFDictionary::SetValue			(const char *utf8Z, double valF)
{
	ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &valF));

	SetValue(utf8Z, numberRef.Get());
}

/*****************************************/
RGBColor			CCFDictionary::GetAs_Color		(const char *utf8Z)
{
	RGBColor			valR		= { 0, 0, 0 };
	CCFDictionary		dict(GetAs_Dict(utf8Z), true);
	
	if (dict.Get()) {
		valR.red		= (UInt16)dict.GetAs_UInt32(DICT_STR_COLOR_RED);
		valR.green		= (UInt16)dict.GetAs_UInt32(DICT_STR_COLOR_GREEN);
		valR.blue		= (UInt16)dict.GetAs_UInt32(DICT_STR_COLOR_BLUE);
	}
	
	return valR;
}

void				CCFDictionary::SetValue(const char *utf8Z, const RGBColor& value)
{
	CCFDictionary		dict;
	
	dict.SetValue(DICT_STR_COLOR_RED,	(UInt32)value.red);
	dict.SetValue(DICT_STR_COLOR_GREEN, (UInt32)value.green);
	dict.SetValue(DICT_STR_COLOR_BLUE,	(UInt32)value.blue);
	
	SetValue(utf8Z, dict.Get());
}
/*****************************************/

void				CCFDictionary::TransferValue(OSType osType, CCFDictionary& other)
{
	TransferValue(OSTypeToString(osType).utf8Z(), other);
}
/*****************************************/


CFNumberRef			CFNumberCreateWithUInt64(UInt64 valLL)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &valLL);
}

CFNumberRef			CFNumberCreateWithUInt32(UInt32 valL)
{
	return CFNumberCreateWithUInt64(valL);
}

CFNumberRef			CFNumberCreateWithSInt32(SInt32 valL)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &valL);
}

void		CCFDictionary::SetRealValue(CFTypeRef key, CFTypeRef val)
{
	if (val != NULL) {
		CFDictionarySetValue(Get(), key, val);
	}
}

void				CCFDictionary::SetValue_Ref(CFStringRef keyRef, SInt32 value)
{
	ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithSInt32(value));
	
	SetValue(keyRef, numberRef.Get());
}

void				CCFDictionary::SetValue(const char *utf8Z, SInt32 value)
{
	ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithSInt32(value));
	
	SetValue(utf8Z, numberRef.Get());
}

#if _QT_ && !_CFTEST_ && !OPT_WINOS
void				CCFDictionary::SetValue(const char *utf8Z, long value)
{
	SetValue(utf8Z, (SInt32)value);
}

void				CCFDictionary::SetValue(const char *utf8Z, unsigned long value)
{
	SetValue(utf8Z, (UInt32)value);
}
#endif
 
void				CCFDictionary::SetValue(const char *utf8Z, UInt32 value)
{
	ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithUInt32(value));
	
	SetValue(utf8Z, numberRef.Get());
}

/**********************************************/

UInt64				CCFDictionary::GetAs_UInt64	(const char *utf8Z)
{
	CFNumberRef		valRef((CFNumberRef)GetValue(utf8Z));
	UInt64			valLL = 0;

	if (valRef) {
		CF_ASSERT(CFGetTypeID(valRef) == CFNumberGetTypeID());
		CFNumberGetValue(valRef, kCFNumberSInt64Type, &valLL);
	}

	return valLL;
}

void				CCFDictionary::SetValue(const char *utf8Z, UInt64 value)
{
	ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithUInt64(value));

	SetValue(utf8Z, numberRef.Get());
}

void			CCFDictionary::SetValue(const char *utf8Z, Ptr valueP)
{
	#if __LP64__
		SetValue(utf8Z, (UInt64)valueP);
	#else
		SetValue(utf8Z, (UInt32)valueP);
	#endif
}

Ptr				CCFDictionary::GetAs_Ptr(const char *utf8Z)
{
	Ptr		ptrP = 
	
	#if __LP64__
		(Ptr)GetAs_UInt64(utf8Z);
	#else
		(Ptr)GetAs_UInt32(utf8Z);
	#endif
	
	return ptrP;
}
/**********************************************/

void				CCFDictionary::SetValue(const char *utf8Z, SInt16 value)
{
	ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &value));
	
	SetValue(utf8Z, numberRef.Get());
}

void				CCFDictionary::SetValue(OSType key, SInt16 value)
{
	SetValue(OSTypeToString(key).utf8Z(), value);
}

void				CCFDictionary::SetValue(OSType key, SInt32 value)
{
	SetValue(OSTypeToString(key).utf8Z(), value);
}

#if _QT_ && !_CFTEST_ && !OPT_WINOS
void				CCFDictionary::SetValue(OSType key, long value)
{
	SetValue(key, (SInt32)value);
}
#endif

void				CCFDictionary::SetValue(OSType key, UInt32 value)
{
	SetValue(OSTypeToString(key).utf8Z(), value);
}

void				CCFDictionary::SetValue(const char *utf8Z, CFTypeRef val)
{
	SuperString		keyStr(uc(utf8Z));
	
	SetValue(keyStr.ref(), val);
}
 
void				CCFDictionary::SetValue(OSType key, CFTypeRef val)
{
	SuperString		keyStr(OSTypeToString(key));
	
	SetValue(keyStr.ref(), val);
}

void				CCFDictionary::SetValue(OSType key, const SuperString& value)
{
	SetValue(key, value.ref());
}
 
void				CCFDictionary::SetValue(const char *utf8Z, const SuperString& value)
{
	SetValue(utf8Z, value.ref());
}

void				CCFDictionary::SetValue(const char *utf8Z, const char *utf8ValZ)
{
	SetValue(utf8Z, SuperString(uc(utf8ValZ)).ref());
}

SuperString		CCFDictionary::GetXML() const {
	CCFData			cfData(Get());
	SuperString		xmlStr; xmlStr.Set(cfData.Get());
	
	return xmlStr;
}

//	string can be JSON as or PLIST
void	CCFDictionary::SetXML(const SuperString& xml) {
	CFStringBuiltInEncodings		encoding;
	
	if (xml.Contains(kXML_EncodingKeyValUTF8)) {
		encoding = kCFStringEncodingUTF8;
	} else {
		encoding = kCFStringEncodingUnicode;
	}

	CCFData		dataRef(CFStringCreateExternalRepresentation(kCFAllocatorDefault, xml.ref(), encoding, 0));
	
	if (dataRef.Get()) {
		ScCFReleaser<CFPropertyListRef>		plistRef(dataRef.CopyAs_Dict(true));
		
		if (plistRef.Get()) {
			CFTypeID					cfTypeID(CFGetTypeID(plistRef));
			
			if (cfTypeID == CFDictionaryGetTypeID()) {
				adopt((CFMutableDictionaryRef)plistRef.transfer());
			}
		}
	}
	
	if (Get() == NULL) {
		realloc();
	}
}

class ForEach_CopyToJSON {
	public: 
	
	class Data {
		SuperString&			i_jsonStr;
		bool					i_arrayB;
		bool					i_firstB;
		int						i_indentI;
		JSON_DateFormatType		i_dateFormat;
		
		void	add_space() {
			if (i_indentI != kJSON_Compact) {
				i_jsonStr.append(" ");
			}
		}
		
		void	add_tabs() {
		
			if (i_indentI != kJSON_Compact) {
				i_jsonStr += GetIndentString(i_indentI);
			}
		}
		
		void	add_return() {
			if (i_indentI != kJSON_Compact) {
				#if OPT_WINOS
					i_jsonStr.append("\r");
				#endif

				i_jsonStr.append("\n");
			}
		}

		void	StartDict() {
			i_jsonStr.append(i_arrayB ? "[" : "{");
			add_return();

			i_indentI = i_indentI == kJSON_Compact ? kJSON_Compact : i_indentI + 1;
			i_firstB = true;
		}
		
		void	EndDict() {
			i_indentI = i_indentI == kJSON_Compact ? kJSON_Compact : i_indentI - 1;

			add_return();
			add_tabs();
			i_jsonStr.append(i_arrayB ? "]" : "}");
		}
		
		public:
		Data(SuperString& json, bool arrayB, int indentI, JSON_DateFormatType dateFormat) : 
			i_jsonStr(json),
			i_arrayB(arrayB),
			i_firstB(false),
			i_indentI(indentI),
			i_dateFormat(dateFormat)
		{
			StartDict();
		}
		
		~Data() {
			EndDict();
		}

		void	operator()(CFTypeRef valRef) {
			operator()(NULL, valRef);
		}
		
		void	operator()(CFStringRef keyRef0, CFTypeRef valRef) {
			CFTypeEnum		typeEnum(CFGetCFType(valRef));
			SuperString		valueStr;
			
			switch (typeEnum) {
				
				case CFType_NUMBER_FLOAT: {
					double			valF = CFNumberToDouble((CFNumberRef)valRef);

					valueStr.append(valF, 2);
					break;
				}
				
				case CFType_NUMBER_INT: {
					long			valL = CFNumberToLong((CFNumberRef)valRef);
					
					valueStr.append(valL);
					break;
				}
				
				case CFType_STRING: {
					valueStr.Set((CFStringRef)valRef, true);
					
					if (valueStr != "null") {
						valueStr.EscapeJSON();
						valueStr.Enquote();
					}
					break;
				}
				
				case CFType_ARRAY: {
					CCFArray			array((CFArrayRef)valRef, true);
					
					valueStr = array.GetJSON(i_indentI, i_dateFormat);
					break;
				}
				
				case CFType_DICT: {
					CCFDictionary		dict((CFDictionaryRef)valRef, true);
					
					valueStr = dict.GetJSON(i_indentI, i_dateFormat);
					break;
				}
				
				case CFType_TIMEZONE: {
					CF_ASSERT(0);
				} break;

				case CFType_DATA: {
					CF_ASSERT(0);
					valueStr = "<data blob>";
				} break;

				case CFType_DATE: {
					//	seconds, epoch jan 1, 2001
					CFAbsoluteTime		timeT = CFDateGetAbsoluteTime((CFDateRef)valRef);
					
					switch (i_dateFormat) {
						
						default: {
							CF_ASSERT(0);
							break;
						}
					
						case kJSON_DateFormat_ISO_8601: {
							//	https://en.wikipedia.org/wiki/ISO_8601
							//	here's why we use this:
							//	http://stackoverflow.com/questions/10286204/the-right-json-date-format
							CF_ASSERT(0);
							break;
						}
							
						case kJSON_DateFormat_DOT_NET: {
							valueStr.Set(timeT, SS_Time_JSON);
							valueStr.EscapeJSON();
							valueStr.Enquote();
							break;
						}
							
						case kJSON_DateFormat_DOT_NET_STRIPPED: {
							timeT += kCFAbsoluteTimeIntervalSince1970;
							timeT /= kEventDurationMillisecond;
							valueStr.append(timeT, 0);
							break;
						}
					}
					break;
				}
				
				case CFType_BOOL: {
					bool	setB = (CFBooleanRef)valRef == kCFBooleanTrue;
					
					valueStr.Set(setB ? "true" : "false");
					break;
				}
				
				default: {
					CF_ASSERT(0);
					break;
				}
			}
			
			if (i_firstB) {
				i_firstB = false;
				
			} else {
				i_jsonStr.append(",");
				add_return();
			}
			
			add_tabs();
				
			if (keyRef0) {
				SuperString		keyStr(keyRef0);

				keyStr.Enquote();
				i_jsonStr.append(keyStr);
				i_jsonStr.append(":");
				add_space();
			}
			
			i_jsonStr.append(valueStr);
		}
	};
	
	private:
	Data&	i_data;
	
	/******************************/
	public:
	ForEach_CopyToJSON(Data& data) : i_data(data) { }
	
	void	operator()(CFStringRef keyRef, CFTypeRef valRef) {
		i_data.operator()(keyRef, valRef);
	}

	void	operator()(CFTypeRef valRef) {
		i_data.operator()(valRef);
	}
};

// #define		kJSON_Compact	-1 
// #define		kJSON_Indented	0 
SuperString		CCFDictionary::GetJSON(int indentI, JSON_DateFormatType dateFormat) const
{
	SuperString					json;
	
	{
		ForEach_CopyToJSON::Data	data(json, false, indentI, dateFormat);
		CCFDictionary&				non_const_this(const_cast<CCFDictionary&>(*this));
		
		non_const_this.for_each(ForEach_CopyToJSON(data));
		//	data must go out of scope for string to be updated
	}
	
	return json;
}

static	void	eat_white_space(ustring& str, bool include_containersB = true)
{
	bool	doneB = false;
	
	do {
		doneB = str.empty();
		
		if (!doneB) {
			char	ch = str[0];
			
			if (
				   ch == ' '
				|| ch == '\n'
				|| ch == '\r'
				|| ch == '\t'
				|| (
					include_containersB
					&& (
						   ch == '{'
						|| ch == '['
					)
				)
			) {
				str.erase(str.begin());
			} else {
				doneB = true;
			}
		}
	} while (!doneB);
}

static	ustring	rSplit(ustring& io_rhs, const char *pivotZ)
{
	ustring				lhs;
	ustring::iterator	it;
	
	if (pivotZ[0] == 0) {
		lhs = io_rhs;
		io_rhs.clear();
	} else {
		it = std::find(io_rhs.begin(), io_rhs.end(), *pivotZ);
	}
		
	if (it != io_rhs.end()) {
		lhs.assign(io_rhs.begin(), it);
		++it;
		io_rhs.erase(io_rhs.begin(), it);
	}
	
	return lhs;
}

void	NoQuotes(ustring& str)
{
	SuperString		sStr(str);
	
	sStr.NoQuotes(false);
	str = sStr.utf8();
}

void			CCFDictionary::SetJSON(const SuperString& in_sJson, CFParseProgress *progP0)
{
	realloc();
	
	if (!in_sJson.empty()) {
		SuperString		copyStr;
		SuperString& 	sJson(progP0 ? const_cast<SuperString &>(in_sJson) : copyStr);
		
		if (progP0 == NULL) {
			copyStr.Set(in_sJson);
		}

		SetJSON_p(sJson, progP0);
	}
}

SuperString		CCFArray::GetJSON(int indentI, JSON_DateFormatType dateFormat)
{
	SuperString					json;
	
	{
		ForEach_CopyToJSON::Data	data(json, true, indentI, dateFormat);
		
		for_each(ForEach_CopyToJSON(data));
		//	data must go out of scope for string to be updated
	}
	
	return json;
}

void			CCFArray::push_back(const SuperString& valStr)
{
	push_back(valStr.ref());
}

static	UTF8Char			get_end_brace(bool arrayB)
{
	return arrayB ? ']' : '}';
}

static	ustring::iterator		find_separator(ustring& jsonStr, bool arrayB)
{
	ustring				commaBrace(uc(",")); commaBrace += get_end_brace(arrayB);
	bool				doneB = false;
	ustring::iterator	it = jsonStr.begin();
	ustring::iterator	prev_it = it;
	
	if (jsonStr[0] == '"') {
		//	skip over quotes
		do {
			prev_it = std::find(prev_it + 1, jsonStr.end(), '"');
		} while (*(prev_it - 1) == '\\');
	}

	do {
		it = std::find_first_of(prev_it, jsonStr.end(), commaBrace.begin(), commaBrace.end());
	
		if (it == jsonStr.end()) {
			CF_ASSERT(0);
			
		} else {
			doneB = true;
		}
	} while (!doneB);
	
	return it;
}

class CFTupleAdder {
	bool		i_is_arrayB;

	protected:
	UTF8Char	get_brace() {
		return get_end_brace(i_is_arrayB);
	}

	public:
	CFTupleAdder(bool is_arrayB) : i_is_arrayB(is_arrayB) { }
	
	virtual	void	add(CFTypeRef cfType) = 0;
	
	void			add_value(ustring& jsonStr, bool& doneB)
	{
		eat_white_space(jsonStr, false);

		ustring::iterator	it(find_separator(jsonStr, i_is_arrayB));
		ustring				valStr(jsonStr.begin(), it);
		
		if (*it == get_end_brace(i_is_arrayB)) {
			doneB = true;
		}
		
		++it;
		if (it != jsonStr.end()) {
			if (*it == ',') {
				++it;
			}
		}
		
		jsonStr.erase(jsonStr.begin(), it);

		eat_white_space(jsonStr, false);
		
		SuperString		sValStr(valStr);

		if (valStr[0] == '"') {
			sValStr.NoQuotes(false);
			sValStr.UnEscapeJSON();
			
			if (sValStr.StartsWith("/")) {
				//	date only supports kJSON_DateFormat_DOT_NET when reading
				CFAbsoluteTime		dateT = sValStr.GetAs_CFAbsoluteTime(SS_Time_JSON, kCFAbsoluteTimeIntervalSince1970);
				CCFDate				dateRef(CFDateCreate(kCFAllocatorDefault, dateT));
				
				add(dateRef.Get());
			} else {
				//	string
				add(sValStr.ref());
			}
		} else {
			char		ch = valStr[0];
			
			//	float, int, or bool
			if (ch == 't' || ch == 'f') {
				bool	valB = ch == 't';
				
				add(valB ? kCFBooleanTrue : kCFBooleanFalse);
			} else if (ch == 'n') {
				//	'null'
				add(sValStr.ref());
			} else {
		
				if (sValStr.Contains(".")) {
					double						valF = sValStr.GetAs_Double();
					ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &valF));
					
					add(numberRef);
				} else {
					SInt32						valL = sValStr.GetAs_SInt32();
					ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithSInt32(valL));
					
					add(numberRef);
				}
			}
		}
	}
};

class CFDictAdder : public CFTupleAdder {
	typedef CFTupleAdder	_inherited;
	CCFDictionary&			i_dict;
	const SuperString&		i_keyStr;
	
	public:
	CFDictAdder(CCFDictionary& dict, const SuperString& keyStr) : 
		_inherited(false),
		i_dict(dict),
		i_keyStr(keyStr)
	{ }
	
	void	add(CFTypeRef cfType) {
		i_dict.SetValue(i_keyStr.ref(), cfType);
	}
};

class CFArrayAdder : public CFTupleAdder {
	typedef CFTupleAdder	_inherited;
	CCFArray&				i_array;
	
	public:
	CFArrayAdder(CCFArray& array) : 
		_inherited(true),
		i_array(array)
	{ }

	void	add(CFTypeRef cfType) {
		i_array.push_back(cfType);
	}
};

static	bool	JSON_Start(ustring& jsonStr, bool arrayB)
{
	bool		doneB = false;
	
	eat_white_space(jsonStr, false);
	
	if (!jsonStr.empty() && jsonStr[0] == get_end_brace(arrayB)) {
		doneB = true;
		jsonStr.erase(jsonStr.begin());

		eat_white_space(jsonStr, false);

		if (!jsonStr.empty() && jsonStr[0] == ',') {
			jsonStr.erase(jsonStr.begin());
		}
	}
	
	return doneB;
}

static	bool	JSON_Finish(
	CFTupleAdder	*adderP, 
	SuperString&	sJson, 
	ustring&		jsonStr,
	CFParseProgress *progP0	= NULL)
{
	eat_white_space(jsonStr, false);

	char	curCh(jsonStr[0]);
	bool	doneB(curCh == 0);

	if (!doneB) {
	
		if (curCh == '[') {
			CCFArray			subArray;
			
			sJson = jsonStr;
			subArray.SetJSON_p(sJson, progP0);
			jsonStr = sJson;
			adderP->add(subArray.Get());
			
		} else if (curCh == '{') {
			CCFDictionary		subDict;
			
			sJson = jsonStr;
			subDict.SetJSON_p(sJson, progP0);
			jsonStr = sJson;
			adderP->add(subDict.Get());

		} else if (curCh == ']') {
			jsonStr.erase(jsonStr.begin());
			doneB = true;
			
		} else {
			CF_ASSERT(curCh != '}');
			
			//	a VALUE, not a dict or array
			adderP->add_value(jsonStr, doneB);
		}
	}
	
	if (progP0) {
		progP0->operator()();
	}
	
	return doneB;
}

void			CCFArray::SetJSON_p(SuperString& sJson, CFParseProgress *progP0)
{
	ustring			jsonStr(sJson.utf8());
	bool			doneB = false;
	
	jsonStr.erase(jsonStr.begin());
	
	do {
		doneB = JSON_Start(jsonStr, true);
		
		if (!doneB) {
			CFArrayAdder		adder(*this);

			doneB = JSON_Finish(&adder, sJson, jsonStr, progP0);
		}
	} while (!doneB);
	
	sJson = jsonStr;
}

void			CCFDictionary::SetJSON_p(SuperString& sJson, CFParseProgress *progP0)
{
	ustring			jsonStr(sJson.utf8());
	bool			doneB = false;
	bool			arrayB(jsonStr[0] == '[');
	SuperString		keyStr;
	ustring			valStr;
		
	if (!arrayB) {
		jsonStr.erase(jsonStr.begin());
	}
	
	do {
		if (!arrayB) {
			doneB = JSON_Start(jsonStr, false);

			if (!doneB) {
				keyStr = rSplit(jsonStr, ":");
				doneB = keyStr.empty();
			}
			
			if (!doneB) {
				keyStr.NoQuotes(false);
				doneB = keyStr.empty();
			}
		}
		
		if (!doneB) {
			CFDictAdder			adder(*this, keyStr);

			doneB = JSON_Finish(&adder, sJson, jsonStr, progP0);
		}
		
		arrayB = false;
	} while (!doneB);
	
	sJson = jsonStr;
}

// Returns true if the current process is being debugged (either
// running under the debugger or has a debugger attached post facto).
bool	CFDebuggerAttached()
{
	bool				debuggedB(false);
	
	#if defined(kDEBUG) || DEBUG
	#if !defined(_HELPERTOOL_)// && !_YAAF_

	#if OPT_WINOS
	{
		debuggedB = IsDebuggerPresent();
	}
	#else // OPT_MACOS
	{
		int                 junk;
		int                 mib[4];
		struct kinfo_proc   info;
		size_t              size;

		// Initialize the flags so that, if sysctl fails for some bizarre
		// reason, we get a predictable result.

		info.kp_proc.p_flag = 0;

		// Initialize mib, which tells sysctl the info we want, in this case
		// we're looking for information about a specific process ID.

		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PID;
		mib[3] = getpid();

		// Call sysctl.

		size = sizeof(info);
		junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
		assert(junk == 0);

		// We're being debugged if the P_TRACED flag is set.

		debuggedB = ( (info.kp_proc.p_flag & P_TRACED) != 0 );
	}
	#endif	//	OPT_MACOS
	#endif	//	!defined(_HELPERTOOL_) && !_YAAF_
	#endif	//	DEBUG
	
    return debuggedB;
}


bool			g_debugBreakB = true;

void			CFDebugBreak()
{
#if defined(kDEBUG) || DEBUG
	if (g_debugBreakB && CFDebuggerAttached()) {
		#if OPT_MACOS
			__asm__("int $3\n" : : );
		#elif _QT_
			__debugbreak();
		#else
			DebugBreak();
		#endif
	}
#endif
}

/**********************************************************************/
/*
		that the epoch for
 *    LongDateTime is January 1, 1904 while the epoch for
 *    CFAbsoluteTime is January 1, 2001.
*/
#ifdef __WIN32__
OSStatus	UCConvertLongDateTimeToCFAbsoluteTime(
	LongDateTime		iLongTime,
	CFAbsoluteTime		*oCFTimeP)
{
	*oCFTimeP = iLongTime + kCFAbsoluteTimeIntervalSince1904;
	return noErr;
}

OSStatus	UCConvertCFAbsoluteTimeToLongDateTime(
	CFAbsoluteTime		iCFTime,
	LongDateTime		*oLongDateP)
{
	*oLongDateP = (LongDateTime)(iCFTime - kCFAbsoluteTimeIntervalSince1904);
	return noErr;
}
#endif

CFDateRef 		CFDateCreateWithLongDateTime(const LongDateTime &ldt)
{
	CFDateRef		dateRef = NULL;

	if (ldt != 0) {
		OSStatus			err		= noErr;
		CFAbsoluteTime		absT;
		
		err = UCConvertLongDateTimeToCFAbsoluteTime(ldt, &absT);
		if (!err) {
			dateRef = CFDateCreate(kCFAllocatorDefault, absT);
		}
	}
	
	return dateRef;
}

LongDateTime	CFDateToLongDateTime(CFDateRef dateRef)
{
	LongDateTime		ldt;	structclr(ldt);
	
	if (dateRef) {
		CFAbsoluteTime		absT = CFDateGetAbsoluteTime(dateRef);

		UCConvertCFAbsoluteTimeToLongDateTime(absT, &ldt);	//	kCFAbsoluteTimeIntervalSince1904
	}

	return ldt;
}

CFLocaleRef			CFLocaleCopyCurrent_Mutex()
{
	static CFLocaleRef		s_ref = NULL;
	
	if (s_ref == NULL) {
		s_ref = CFLocaleCopyCurrent();
	}
	
	CFRetainDebug(s_ref);
	return s_ref;
}

static CFCalendarRef		CFCalendarCopyCurrent_Mutex()
{
	static CFCalendarRef		s_ref = NULL;
	
	if (s_ref == NULL) {
		s_ref = CFCalendarCopyCurrent();
	}

	CFRetainDebug(s_ref);	
	return s_ref;
}

static CFTimeZoneRef		CFCalendarCopyTimeZone_Mutex()
{
	static CFTimeZoneRef			s_ref = NULL;
	
	if (s_ref == NULL) {
		ScCFReleaser<CFCalendarRef>		calendarRef(CFCalendarCopyCurrent_Mutex());
		
		s_ref = CFCalendarCopyTimeZone(calendarRef);
	}
	
	CFRetainDebug(s_ref);
	return s_ref;
}

CFDateRef		CFDateCreateWithGregorian(const CFGregorianDate& gregDate)
{
	CCFTimeZone		timeZoneRef(CFCalendarCopyTimeZone_Mutex());
	
	return CFDateCreate(kCFAllocatorDefault, CFGregorianDateGetAbsoluteTime(gregDate, timeZoneRef));
}

CFGregorianDate	CFDateGetGregorian(CFDateRef dateRef)
{
	CCFTimeZone		timeZoneRef(CFCalendarCopyTimeZone_Mutex());
	
	return CFAbsoluteTimeGetGregorianDate(dateRef ? CFDateGetAbsoluteTime(dateRef) : 0, timeZoneRef);
}

CFStringRef		CFStringCreateWithDate(
	CFDateRef				dateRef, 
	CFDateFormatterStyle	dateFormat,		//	= kCFDateFormatterShortStyle
	CFDateFormatterStyle	timeFormat)		//	= kCFDateFormatterShortStyle
{
	ScCFReleaser<CFLocaleRef>			localeRef(CFLocaleCopyCurrent_Mutex());
	ScCFReleaser<CFDateFormatterRef>	formatterRef(CFDateFormatterCreate(
		kCFAllocatorDefault, localeRef, dateFormat, timeFormat));
	CFStringRef							ref(CFDateFormatterCreateStringWithDate(
		kCFAllocatorDefault, formatterRef, dateRef));
	
	return (CFStringRef)CFRetainDebug(ref, false); // don't retain, just track that it's existing refcount
}

CFStringRef		CFStringCreateWithNumber(CFNumberRef numRef)
{
	ScCFReleaser<CFLocaleRef>			localeRef(CFLocaleCopyCurrent_Mutex());
	ScCFReleaser<CFNumberFormatterRef>	formatterRef(CFNumberFormatterCreate(
		kCFAllocatorDefault, localeRef, kCFNumberFormatterNoStyle));
	CFStringRef							ref(CFNumberFormatterCreateStringWithNumber(
		kCFAllocatorDefault, formatterRef, numRef));

	return (CFStringRef)CFRetainDebug(ref, false);
}

/*
CFStringRef		CFCopyBundleResourcesFSPath()
{
	ScCFReleaser<CFBundleRef>		exeRef(CFBundleGetMainBundle(), true);
	CCFURL							exeUrlRef(CFBundleCopyBundleURL(exeRef));
	SuperString						dirStr(CFURLCopyFileSystemPath(exeUrlRef, kCFURLPlatformPathStyle));
	
	dirStr.append(kCFURLPlatformPathSeparator "Contents" kCFURLPlatformPathSeparator "Resources");
	return dirStr.Retain();
}
*/

/**********************************************************************/

#if defined(_KJAMS_) && !defined(_MIN_CF_)

int		AssertAlert(const char *msgZ, const char *fileZ, long lineL, bool noThrowB)
{
	bool	incB = IsPreemptiveThread();
	
	if (incB) {
		if (gApp) ++gApp->i_callbackL;
		noThrowB = true;
	}

	bool	mainB = gApp == NULL || gApp->i_callbackL == 0;

	if (incB) {
		if (gApp) --gApp->i_callbackL;
	}

	SuperString		formatStr(SSLocalize("Assert Fail: %s, in file: '%s' at line %ld", "eeeks!"));

	const char * utf8Z = formatStr.utf8Z();
	formatStr.ssprintf(NULL, msgZ, fileZ, lineL);

	#ifdef kDEBUG
		static bool	s_showB = true;
		if (s_showB) {
			CFStackTraceLog();
			CFDebugBreak();
		}
	#endif

	if (gApp == NULL || gApp->IsShutdown()) {
		Log(formatStr.utf8Z());

	} else {
		if (mainB) {
			MessageAlert(formatStr.utf8Z());
		} else {
			PostAlert(formatStr.utf8Z());
		}
	}
	
	if (!mainB && !noThrowB) {
		ETX(ERR_Assert_Failed);
	}
	
	return 1;
}
#else
int		AssertAlert(const char *msgZ, const char *fileZ, long lineL, bool noThrowB)
{
	UNREFERENCED_PARAMETER(noThrowB);

	SuperString		formatStr("$$$ Assert Fail: %s, in file: '%s' at line %ld\n");
	
	formatStr.ssprintf(NULL, msgZ, fileZ, lineL);
	CCFLog()(formatStr.ref());
	ReportErr(formatStr.utf8Z(), -1);
	return 1;
}
#endif

#if !kUseCFLogger
static bool	s_alert_logsB = false;
#endif

static bool	s_static_loggingB = true;
static bool	s_static_log_during_startB = false;

void	CFSetLogDuringStartup(bool logB)
{
	s_static_log_during_startB = logB;
}

bool	CFGetLogDuringStartup()
{
	return s_static_log_during_startB;
}

void	CFSetLogging(bool logB)
{
	s_static_loggingB = logB;
}

#if !kUseCFLogger
class CSuppressLogging {
	ScSetReset<bool>	i_sc;

	public:
	CSuppressLogging() : i_sc(&s_static_loggingB, false) {}
};
#endif

bool	CFGetLogging()
{
	bool		can_logB = s_static_loggingB;
	
	if (can_logB) {
		#ifdef _KJAMS_
			if (gApp) {
				can_logB = gApp->Logging();
			} else {
				can_logB = CFGetLogDuringStartup();
			}
		#endif

		#ifdef _QTServer_
		SuperString		pathStr(CCFLog::GetLogPath());

		if (pathStr.empty()) {
			can_logB = CFGetLogDuringStartup();
		}
		#endif
	}	
	
	return can_logB;
}

#ifndef _CFTEST_
SuperString		GetUserName()
{
	SuperString		userNameStr;
	
	#if defined(_KJAMS_)
		OSStatus		err = noErr;
		FSRef			userRef;
		
		ERR(FSrFindFolder(kUserDomain, kCurrentUserFolderType, &userRef));
		ERR(FSrGetName(userRef, &userNameStr));
	#endif
	
	return userNameStr;
}

#define		kLogConsoleGetting		0

SuperString		GetConsoleFilePath()
{
	SuperString		pathStr;

#if OPT_WINOS
	pathStr = CCFLog::GetLogPath();
#else
	OSStatus		err = noErr;
	
	XTE_START {
		FSRef			fileRef;
		CFileRef		cFileRef;
		bool			existsB = false;
		
		if (kLogConsoleGetting) Log("getting console data");
		
		ETX(FSrFindFolder(kUserDomain, kLogsFolderType, &fileRef));
		if (!err) {
			if (kLogConsoleGetting) Log("10.5 or better version");

			SuperString		nameStr(CFileRefMgr::Get()->GetBundleName());
			
			if (kLogConsoleGetting)  Log("found user logs folder");

			#ifdef kDEBUG
				nameStr.Replace(" Debug", "");
			#endif

			nameStr.append(".log");
			XTE(cFileRef.SetRef(fileRef));
			ERR(cFileRef.GetChild(nameStr));
			existsB = err == noErr;
			
			if (!existsB) {
				if (kLogConsoleGetting) Logf("~/Library/Logs/%s file not created\n", nameStr.utf8Z());
			} else {
				if (kLogConsoleGetting) Logf("~/Library/Logs/%s file found or created!  YES!\n", nameStr.utf8Z());
			}
		} else {
			if (kLogConsoleGetting) Logf("%s/Library/Logs/ folder not found", "~/");
		}
			
		if (!existsB || err) {
			cFileRef.Clear();
		}

		pathStr = cFileRef.path();
	} XTE_END;
#endif

	return pathStr;
}
#endif

void			LogDialogInfo(
	const char			*typeZ, 
	const char			*titleZ, 
	const char			*msgZ0, 
	const char			*resultZ0)
{
	if (CFGetLogging()) {
		SuperString		formatStr("\n------ %s BEGIN:\n%s\n%s\n%s\n------ %s END");

		formatStr.ssprintf(
			NULL, 
			typeZ, 
			titleZ, 
			msgZ0 ? msgZ0 : "", 
			resultZ0 ? resultZ0 : "", 
			typeZ);
		
		Log(formatStr.utf8Z());
	}
}

void		Log(const SuperString& str, bool crB)
{
	Log(str.utf8Z(), crB);
}

void		Log(const char *utf8Z, bool crB)
{
	if (CFGetLogging()) {
		SuperString				str(uc(utf8Z));
		static	SuperString		percentStr;
		
		if (percentStr.empty()) {
			percentStr.Set(UTF8BytesToString(0xEFBC8500));
		}
		
		str.Replace("%", percentStr);
		
		#if defined(_KJAMS_) && !defined(_MIN_CF_)
			CCritical	sc(LogMutex());
		#endif
		
		str.ScrubSensitiveInfo();

		#if kUseCFLogger
			CCFLog			logger(crB);

			logger(str.ref());
		#else
			PrependThreadNumber(str, crB);
		
			{
				static bool				s_triedB = false;
				
				if ((FILE *)s_cfileRef == NULL && !s_triedB) {
					CSuppressLogging	sc2;							//	protected by LogMutex
					ScSetReset<bool>	sc3(&s_alert_logsB, true);		//	protected by LogMutex
					OSStatus			err = noErr;
					
					s_cfileRef.SetPath(GetConsoleFilePath());	//	causes log file to be created
					
					ERR(s_cfileRef.IsValid() ? (OSStatus)noErr : (OSStatus)fnfErr);
					
					if (!err) {
	//					Log("console.log file is valid, attempting to open for append");
					}

					CCFLog::trim();
					
					ERR(s_cfileRef.fopen("a"));

					if (err) {
	//					ReportErr("Creating log file, this is very bad, please tell dave", err);
						s_triedB = true;
					}
				}

				if ((FILE *)s_cfileRef) {
					s_cfileRef.fprintf(str.utf8Z());
					
					if (crB) {
						s_cfileRef.fprintf("\n");
					}
				}

				#ifdef kDEBUG
				{
					fputs(str.utf8Z(), stdout);
					
					if (crB) {
						fputs("\n", stdout);
						fflush(stdout);
					}
				}
				#endif
			} 
		#endif
	}
}

#ifdef _KJAMS_
CMutex*		GetSprintfMutex();
#endif

const char *	YesOrNo(bool yesB)
{
	return yesB ? "Yes" : "No";
}

SuperString		YesOrNoStr(const char *msgZ, bool yesB)
{
	SuperString		str("%s: %s");
	
	str.ssprintf(NULL, msgZ, YesOrNo(yesB));
	return str;
}

void			LogYesOrNo(const char *msgZ, bool yesB)
{
	Logf("%s\n", YesOrNoStr(msgZ, yesB).utf8Z());
}

SuperString		GetIndentString(size_t tabsL)
{
	SuperString		tabStr;

	loop (tabsL) {
		tabStr.append("\t");
	}

	return tabStr;
}

void	IndentLevel(short levelS)
{
	Logf(GetIndentString(levelS).utf8Z());
}

void	Logf(const char *utf8Z,...)
{
	if (CFGetLogging()) {
		va_list 	args;

		va_start(args, utf8Z);
		char		*sprintfBuf = mt_vsnprintf(utf8Z, args);
		va_end(args);

		Log(sprintfBuf, false);
	}
}

void	IfLog(bool logB, const char *labelZ, const char *utf8Z, bool crB)
{
	if (logB && CFGetLogging()) {
		SuperString		str(labelZ);
		
		str += ": ";
		str += SuperString(uc(utf8Z));
		
		Log(str.utf8Z(), crB);
	}
}

void	IfLogf(bool logB, const char *labelZ, const char *utf8Z,...)
{
	if (logB && CFGetLogging()) {
		va_list 	args;

		va_start(args, utf8Z);
		char		*sprintfBuf = mt_vsnprintf(utf8Z, args);
		va_end(args);

		IfLog(logB, labelZ, sprintfBuf, false);
	}
}

SuperString		LogPtr_GetStr(const char *strZ, const void *ptr)
{
	SuperString		ptrStr("%s: %s");
	
	ptrStr.ssprintf(NULL, strZ, PtrToString(ptr).utf8Z());
	return ptrStr;
}

void			LogPtr(const char *strZ, const void *ptr)
{
	SuperString		ptrStr(LogPtr_GetStr(strZ, ptr));
	
	Logf("%s\n", ptrStr.utf8Z());
}

void			IfLogPtr(bool ifB, const char *strZ, const void *ptr)
{
	if (ifB) {
		LogPtr(strZ, ptr);
	}
}

void		CFLogSetLogPath()
{
#if !defined(_CFTEST_) || defined(_QTServer_)
	SuperString		pathStr(CCFLog::GetLogPath());

	if (pathStr.empty()) {
		CFileRef		fileRef(CFileRef::kFolder_LOGS);
		
		pathStr.Set(fileRef.path());
		pathStr.append(kCFURLPlatformPathSeparator);
		pathStr.append(uc(kJams_LogFileName));

		#if defined(_QTServer_) || (!defined(kDEBUG) && !defined(_DEBUG))
		{
			CFAbsoluteTime	curT(CFAbsoluteTimeGetCurrent());
			SuperString		dateStr; dateStr.Set(curT, SS_Time_LOG);
			SuperString		ext; pathStr.pop_ext(&ext);

			pathStr.append(" ");
			pathStr.append(dateStr);
			pathStr.append(ext);
		}
		#endif

		CCFLog::SetLogPath(pathStr.ref());
	}
#endif
}

CFStringRef		CFBundleCopyLocalizationPath()
{
	SuperString		pathStr;

#ifndef _CFTEST_
	CFileRef		fileRef(CFileRef::kFolder_RES);
	
	pathStr.Set(fileRef.path());

	CCFArray		arrayRef(CFBundleCopyBundleLocalizations(CFBundleGetMainBundle()));
	CCFArray		prefRef(CFBundleCopyPreferredLocalizationsFromArray(arrayRef));
	SuperString		resLangStr(kCFURLPlatformPathSeparator);
	
	resLangStr.append((CFStringRef)prefRef[(CFIndex)0]);
	resLangStr.append(".lproj");
	pathStr.append(resLangStr);
#endif
	
	return pathStr.Retain();
}

UInt32		CFSwapInt24HostToBig(UInt32 valL)
{
	valL = CFSwapInt32HostToBig(valL);

	#if TARGET_RT_LITTLE_ENDIAN
		valL >>= 8;
	#endif
	
	return valL;
}

UInt32		CFSwapInt24BigToHost(UInt32 valL)
{
	valL = CFSwapInt32BigToHost(valL);

	#if TARGET_RT_LITTLE_ENDIAN
		valL >>= 8;
	#endif
	
	return valL;
}

CFStringRef		CFCopyLocaleLangKeyCode()
{
	CCFArray		arrayRef;
	
	#if defined(__WIN32__)
		arrayRef.adopt((CFMutableArrayRef)CFLocaleCopyPreferredLanguages());
	#else
		arrayRef.adopt((CFMutableArrayRef)CFPreferencesCopyAppValue(CFSTR("AppleLanguages"), kCFPreferencesCurrentApplication));
	#endif

	SuperString		localeKeyCode(arrayRef.GetIndValAs_Str(0));

	return localeKeyCode.Retain();
}

bool			CFLocaleIsEnglish()
{
	SuperString		localeCode(CFCopyLocaleLangKeyCode(), false);
	
	return localeCode == "en";
}

bool			CFAbsoluteTimeExpired(const CFAbsoluteTime &absTimeT)
{
	CFAbsoluteTime			curT(CFAbsoluteTimeGetCurrent());
	bool					expiredB = curT > absTimeT;

	return	expiredB;
}

bool			CFGregorianDateExpired(const CFGregorianDate &gregTimeT)
{
	CCFTimeZone			timeZone(CFTimeZoneCopyDefault());
	CFAbsoluteTime		absTimeT(CFGregorianDateGetAbsoluteTime(gregTimeT, timeZone));
	
	return CFAbsoluteTimeExpired(absTimeT) ;
}

CFGregorianDate		CFAbsoluteTimeConvertToGregorian(const CFAbsoluteTime& cfTime)
{
	CCFTimeZone		tz(CFTimeZoneCopyDefault());

	return CFAbsoluteTimeGetGregorianDate(cfTime, tz);
}

CFAbsoluteTime		CFAbsoluteTimeCreateFromGregorian(const CFGregorianDate &greg, bool gmtB)
{
	CCFTimeZone		tz(gmtB ? CFTimeZoneCopyGMT() : CFTimeZoneCopyDefault());

	return CFGregorianDateGetAbsoluteTime(greg, tz);
}

/***************************************************/
CFStringRef			CFCalendarCopyUnitString(CFCalendarUnit unit, bool pluralB)
{
	SuperString			labelStr;
	
	#if defined(_KJAMS_)
	CF_ASSERT(bitcount(unit) == 1);
	#endif
	
	switch (unit) {
	
		case kCFCalendarUnitYear: {
			if (pluralB) {
				labelStr.Set(SSLocalize("Years", "plural"));
			} else {
				labelStr.Set(SSLocalize("Year", "one"));
			}
			break;
		}

		case kCFCalendarUnitMonth: {
			if (pluralB) {
				labelStr.Set(SSLocalize("Months", "plural"));
			} else {
				labelStr.Set(SSLocalize("Month", "one"));
			}
			break;
		}

		case kCFCalendarUnitDay: {
			if (pluralB) {
				labelStr.Set(SSLocalize("Days", "plural"));
			} else {
				labelStr.Set(SSLocalize("Day", "one"));
			}
			break;
		}

		case kCFCalendarUnitHour: {
			if (pluralB) {
				labelStr.Set(SSLocalize("Hours", "plural"));
			} else {
				labelStr.Set(SSLocalize("Hour", "one"));
			}
			break;
		}

		case kCFCalendarUnitMinute: {
			if (pluralB) {
				labelStr.Set(SSLocalize("Minutes", "plural"));
			} else {
				labelStr.Set(SSLocalize("Minute", "one"));
			}
			break;
		}

		case kCFCalendarUnitSecond: {
			if (pluralB) {
				labelStr.Set(SSLocalize("Seconds", "plural"));
			} else {
				labelStr.Set(SSLocalize("Second", "one"));
			}
			break;
		}
	}
	
	return labelStr.Retain();
}

CFStringRef			CFGregorianUnitsCopyString(const CFGregorianUnits& in_intervalT)
{
	CFGregorianUnits	intervalT(in_intervalT);
	double				majorValF, minorValF;
	CFCalendarUnit		unitType;
	
	if (
		   intervalT.years		== 0 
		&& intervalT.months		== 0 
		&& intervalT.days		== 0 
		&& intervalT.hours		== 1 
		&& intervalT.minutes	< 30
	) {
		intervalT.hours		= 0;
		intervalT.minutes	+= 60;
	}
	
	if (intervalT.years) {
		unitType			= kCFCalendarUnitYear;
		majorValF			= intervalT.years;
		minorValF			= intervalT.months 
			+ intervalT.days  * (1 /  kCFTimeDaysPerMonth) 
			+ intervalT.hours * (1 / (kCFTimeHoursPerDay * kCFTimeDaysPerMonth));

	} else if (intervalT.months) {
		unitType			= kCFCalendarUnitMonth;
		majorValF			= intervalT.months;
		minorValF			= intervalT.days 
			+ intervalT.hours   * (1 /  kCFTimeHoursPerDay) 
			+ intervalT.minutes * (1 / (kCFTimeMinutePerHour * kCFTimeHoursPerDay));

	} else if (intervalT.days) {
		unitType			= kCFCalendarUnitDay;
		majorValF		 	= intervalT.days;
		minorValF			= intervalT.hours 
			+ intervalT.minutes * (1 /  kCFTimeMinutePerHour) 
			+ intervalT.seconds * (1 / (kCFTimeSecondsPerMinute * kCFTimeMinutePerHour));

	} else if (intervalT.hours) {
		unitType			= kCFCalendarUnitHour;
		majorValF			= intervalT.hours;
		minorValF			= intervalT.minutes 
			+ intervalT.seconds * (1 / kCFTimeSecondsPerMinute);

	} else if (intervalT.minutes) {
		unitType			= kCFCalendarUnitMinute;
		majorValF			= intervalT.minutes;
		minorValF			= intervalT.seconds;

	} else {
		unitType			= kCFCalendarUnitSecond;
		majorValF			= intervalT.seconds;
		minorValF			= 0;
	}
	
	SuperString		finalStr(majorValF, 1);
	bool			pluralB(majorValF > 1);
	
	if (finalStr.EndsWith(".0")) {
		finalStr.pop_back(2);
		
		if (majorValF == 0.0) {
			pluralB = true;
		}
	}
	
	finalStr += " ";
	finalStr += SuperString(CFCalendarCopyUnitString(unitType, pluralB), false);
	
	if (unitType != kCFCalendarUnitSecond && minorValF != 0.0) {
		finalStr += " ";
		finalStr += SuperString(minorValF, 1);
		
		pluralB = true;
		if (finalStr.EndsWith(".0")) {
			finalStr.pop_back(2);
			pluralB = minorValF > 1;
		}
		
		finalStr += " ";
		finalStr += SuperString(CFCalendarCopyUnitString((CFCalendarUnit)(unitType << 1), pluralB), false);
	}
	
	return finalStr.Retain();
}

CFGregorianUnits	CFTimeIntervalGetAsGregorianUnits(CFTimeInterval intervalT)
{
	CCFTimeZone			tz(CFTimeZoneCopyDefault());
	CFAbsoluteTime		curT(CFAbsoluteTimeGetCurrent());
	CFAbsoluteTime		futureT(curT + intervalT);
	CFGregorianUnits	diffUnits(CFAbsoluteTimeGetDifferenceAsGregorianUnits(futureT, curT, tz, kCFGregorianAllUnits));
	
	return diffUnits;
}

CFStringRef			CFTimeIntervalCopyString(CFTimeInterval intervalT)
{
	CFGregorianUnits	diffUnits(CFTimeIntervalGetAsGregorianUnits(intervalT));

	return CFGregorianUnitsCopyString(diffUnits);
}

CFTimeZoneRef		CFTimeZoneCopyGMT()
{
	return CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0);
}

UInt16				CFTimeZoneGetCurrentYear()
{
	CFAbsoluteTime		absT(CFAbsoluteTimeGetCurrent());
	CFGregorianDate		gregDate(CFAbsoluteTimeConvertToGregorian(absT));
	
	return (UInt16)gregDate.year;
}

//	expires after the LAST day of the month, not the start
CFAbsoluteTime	CFAbsoluteTimeCreateExpiryFromMonthYear(long monthL, long yearL)
{
	CFAbsoluteTime			timeT;
	CCFTimeZone				tz(CFTimeZoneCopyDefault());
	CFGregorianDate			gregDate; structclr(gregDate);
	CFGregorianUnits		oneMonth; structclr(oneMonth);
	
	gregDate.year = (SInt32)yearL;
	gregDate.month = (SInt8)monthL;
	gregDate.day = 1;
	oneMonth.months = 1;

	timeT = CFGregorianDateGetAbsoluteTime(gregDate, tz);
	timeT = CFAbsoluteTimeAddGregorianUnits(timeT, tz, oneMonth);
	return timeT;
}

void	CFAbsoluteTime_RoundDown_Day(CFAbsoluteTime *t)
{
	CFGregorianDate		gregDate;

	gregDate = CFAbsoluteTimeConvertToGregorian(*t);
	gregDate.hour	= 0;
	gregDate.minute	= 0;
	gregDate.second	= 0;
	*t = CFAbsoluteTimeCreateFromGregorian(gregDate);
}

void	CFAbsoluteTime_RoundUp_Day(CFAbsoluteTime *t)
{
	CFGregorianDate		gregDate;

	gregDate = CFAbsoluteTimeConvertToGregorian(*t);
	gregDate.hour	= 23;
	gregDate.minute	= 59;
	gregDate.second	= 59;
	*t = CFAbsoluteTimeCreateFromGregorian(gregDate);
}

CFArrayRef			CFLocaleCreateMonthArray()
{
	CCFArray							monthArray;
	SuperString							formatStr("LLLL"), monthStr;
	CFGregorianDate						gregDate;	structclr(gregDate);
	SuperString							localeLangCode(CFCopyLocaleLangKeyCode(), false);
	ScCFReleaser<CFLocaleRef>			localeRef(CFLocaleCreate(kCFAllocatorDefault, localeLangCode.ref()));
	ScCFReleaser<CFDateFormatterRef>	formatterRef(CFDateFormatterCreate(
		kCFAllocatorDefault, localeRef, kCFDateFormatterFullStyle, kCFDateFormatterFullStyle));
		
	CFDateFormatterSetFormat(formatterRef, formatStr.ref());
	CFDateFormatterSetProperty(formatterRef, kCFDateFormatterIsLenient, kCFBooleanTrue);
	
	gregDate.day = 3;
	gregDate.year = CFTimeZoneGetCurrentYear();
	
	loop (12) {
		gregDate.month = (SInt8)(_indexS + 1);
		monthStr.Set(CFDateFormatterCreateStringWithAbsoluteTime(
			kCFAllocatorDefault, 
			formatterRef, 
			CFAbsoluteTimeCreateFromGregorian(gregDate)), 
			false);
		
		monthArray.push_back(monthStr.ref());
	}
	
	return monthArray.transfer();
}

CFTypeID		CFTypeGetID(CFTypeEnum typeEnum)
{
	CFTypeID		typeID = 0;
	
	switch (typeEnum) {
            
        default: {
            //  nothing to do
            break;
        }
	
		case CFType_NULL: {
			typeID = CFNullGetTypeID();
			break;
		}

		case CFType_BOOL: {
			typeID = CFBooleanGetTypeID();
			break;
		}

		case CFType_NUMBER_INT:
		case CFType_NUMBER_FLOAT: {
			typeID = CFNumberGetTypeID();
			break;
		}

		case CFType_DATE: {
			typeID = CFDateGetTypeID();
			break;
		}

		case CFType_TIMEZONE: {
			typeID = CFTimeZoneGetTypeID();
			break;
		}

		case CFType_STRING: {
			typeID = CFStringGetTypeID();
			break;
		}

		case CFType_DICT: {
			typeID = CFDictionaryGetTypeID();
			break;
		}

		case CFType_ARRAY: {
			typeID = CFArrayGetTypeID();
			break;
		}

		case CFType_DATA: {
			typeID = CFDataGetTypeID();
			break;
		}
		
		case CFType_HTTP_MESSAGE: {
			typeID = CFHTTPMessageGetTypeID();
			break;
		}

		case CFType_CFURL: {
			typeID = CFURLGetTypeID();
			break;
		}
	}
	
	return typeID;
}

CFStringRef		CFTypeCopyDesc(CFTypeEnum typeEnum)
{
	return CFCopyTypeIDDescription(CFTypeGetID(typeEnum));
}

CFTypeEnum		CFGetCFType(CFTypeRef cfTypeRef)
{
	CF_ASSERT(cfTypeRef);
	
	CFTypeEnum		enumType = CFType_NONE;
	CFTypeID		cfTypeID(CFGetTypeID(cfTypeRef));
	
	loop_range(CFType_NULL, CFType_UNKNOWN) {
		
		if (cfTypeID == CFTypeGetID((CFTypeEnum)_indexS)) {
			
			if (cfTypeID == CFNumberGetTypeID()) {
				CFNumberType		cfNumType = CFNumberGetType((CFNumberRef)cfTypeRef);
				
				switch (cfNumType) {
				
					default: {
						CF_ASSERT("illegal CFNumber type" == NULL);
						break;
					}
				
					case kCFNumberFloatType:
					case kCFNumberDoubleType:
					case kCFNumberFloat32Type:
					case kCFNumberFloat64Type: {
						enumType = CFType_NUMBER_FLOAT;
						break;
					}
									
					case kCFNumberSInt8Type:
					case kCFNumberSInt16Type:
					case kCFNumberSInt32Type:
					case kCFNumberSInt64Type:
					case kCFNumberCharType:
					case kCFNumberShortType:
					case kCFNumberIntType:
					case kCFNumberLongType:
					case kCFNumberLongLongType:
					case kCFNumberCFIndexType: {
						enumType = CFType_NUMBER_INT;
						break;
					}
				}
			} else {
				enumType = (CFTypeEnum)_indexS;
			}
			
			break;
		}
	}
	
	#ifdef kDEBUG
	if (enumType == CFType_NONE) {
		CFDebugBreak();
	}
	#endif
	
	return enumType;
}

bool	CFIsDebug()
{
	bool		debugB = false;
	
	#ifdef kDEBUG
		debugB = true;
	#endif

	return debugB;
}


CFIndex		S_LogCount(CFTypeRef typeRef, const char *utf8Z, bool forceB)
{
	CFIndex			countL = typeRef ? CFGetRetainCount(typeRef) : 0;
	
	if (CFIsDebug() || forceB) {
		SuperString		nameStr(uc(utf8Z));
		SuperString		str("%s: retains: %d");
		
		nameStr.Enquote(true);
		str.ssprintf(NULL, nameStr.utf8Z(), countL);
		
		if (typeRef) {
			str.append(": <%@>");
			str.ssprintf(NULL, typeRef);
		}
		
		Log(str.utf8Z());
	}
		
	return countL;
}

#if OPT_MACOS
static SystemVersType decToBcd(SystemVersType val)
{
	return (val / 10 * 16) + (val % 10);
}

static		bool	MergeSystemVersByte(OSType selector, int shiftI, SystemVersType& io_val)
{
	bool			failB		= false;
	SystemVersType	compareByte = (io_val >> shiftI) & 0xFF;
	
	if (compareByte == 0x99) {
		SystemVersType	resultVal;
		
		failB = Gestalt(selector, &resultVal) != noErr;
		
		if (!failB) {
			failB = resultVal > 0xFF;

			if (!failB) {
				io_val &= ~(0xFF << shiftI);
				resultVal = decToBcd(resultVal);
				io_val |= (resultVal << shiftI);
			}
		}
	}

	return failB;
}

static SystemVersType	GenerateSystemVersionStatic()
{
	SystemVersType		sysVers		= kMacOS_10_9;
	bool				failB		= false;
	
	failB = Gestalt(gestaltSystemVersionMajor, &sysVers) != noErr;
	
	if (!failB) {
		sysVers = decToBcd(sysVers);
		sysVers = (sysVers << 16) | 0x9999;
	}	

	if (!failB) failB = MergeSystemVersByte(gestaltSystemVersionMinor, 8, sysVers);
	if (!failB) failB = MergeSystemVersByte(gestaltSystemVersionBugFix, 0, sysVers);

	if (failB) {
		Log("Error getting system version!");
		CFDebugBreak();
	}
	
	return sysVers;
}
#endif	//	OPT_MACOS

SystemVersType		GetSystemVers(void)
{
	static	SystemVersType	s_sysVers	= kMacOS_System6;
	static	bool			s_inittedB	= false;
	
	if (!s_inittedB) {
		s_sysVers =
		
		#if OPT_MACOS
			GenerateSystemVersionStatic();
		#else
			kMacOS_10_10;
		#endif
		
		s_inittedB = true;
	}
	
	return s_sysVers;
}

SuperString		GetSystemVersStr(SystemVersType sysVers)
{
	SuperString		versStr = "macOS 10.";
	
	sysVers &= 0xFFFFFF00;
	
	switch (sysVers) {
		default:			versStr += "Unknown System";	break;
		case kMacOS_System6:versStr = "System 6";			break;
		case kMacOS_10_3:	versStr += "3 (Panther)";		break;
		case kMacOS_10_4:	versStr += "4 (Tiger)";			break;
		case kMacOS_10_5:	versStr += "5 (Leopard)";		break;
		case kMacOS_10_6:	versStr += "6 (Snow Leopard)";	break;
		case kMacOS_10_7:	versStr += "7 (Lion)";			break;
		case kMacOS_10_8:	versStr += "8 (Mountain Lion)";	break;
		case kMacOS_10_9:	versStr += "9 (Mavericks)";		break;
		case kMacOS_10_10:	versStr += "10 (Yosemite)";		break;
		case kMacOS_10_11:	versStr += "11 (El Capitan)";	break;
		case kMacOS_10_12:	versStr += "12 (Sierra)";		break;
		case kMacOS_10_13:	versStr += "13 (High Sierra)";	break;
		case kMacOS_10_14:	versStr += "14 (Mojave)";		break;
		case kMacOS_10_15:	versStr += "15 (Catalina)";		break;
	}
	
	return versStr;
}


void	CFReportUnitTest(const char *utf8Z, OSStatus err)
{
	//CCFLog(true)(CFSTR("---------------"));
	
	SuperString		str;
	
	str.Set(
		err
			? uc("$$ FAIL: ")
			: uc("PASS: "));
	
	if (err) {
		str.append("(");
		str.append((long)err);
		str.append(") ");
	}

	str.append(uc(utf8Z));
	
	CCFLog(true)(str.ref());

	//CCFLog(true)(CFSTR("---------------"));
}

void	CFSleep(CFTimeInterval durationT)
{
	//	CCFLog()(CFSTR("----- starting sleep\n"));

	SInt32		resultL = CFRunLoopRunInMode(kCFRunLoopDefaultMode, durationT, false);

	if (resultL == kCFRunLoopRunTimedOut) {
		//		CCFLog()(CFSTR("----- Success sleeping!\n"));
	} else {
		//		CFReportUnitTest("----- FAILED sleeping!", resultL);
	}
}

/*******************************/
bool		IsRamURL(const SuperString& str)
{
	return str.StartsWith(kCFURLProtocol_RAM);
}

CFDataRef	RamURL_GetEssence(const SuperString& str)
{
	SuperString			dictMemAddr(str);
	CFDictionaryRef		dictRef = NULL;
	
	dictMemAddr.rSplit("//");
	
	sscanf(dictMemAddr.utf8Z(), "%p", (void **)&dictRef);
	
	CCFDictionary		dict(dictRef);
	CCFData				cfData(dict.GetAs_Data("essence"), true);
	
	return cfData.transfer();
}

/*******************************/

bool		IsLocalURL(const SuperString& str)
{
	return str.StartsWith(kCFURLProtocol_FILE);
}

CFDataRef	LocalURL_GetEssence(const SuperString& in_str)
{
	CCFData		cfData;

	#ifdef _H_CFileRef
	CFileRef		cFile;

	cFile.SetUrl(in_str);
	
	if (cFile.Exists()) {
		IX(cFile.Load(&cfData));
	}
	#else
		UNREFERENCED_PARAMETER(in_str);
	#endif
	
	return cfData.transfer();
}

/***************************************************************************************/
//	stuff from StringUtils
char			*CopyCFStringToC(
	CFStringRef			str, 
	UInt32				maxL, 
	char				*strZ, 
	CFStringEncoding	encoding)
{
	CF_ASSERT(str);
	
	if (str == NULL) {
		strZ[0] = 0;
	} else {
		std::string		stdstr;
		
		CopyCFStringToStd(str, stdstr, encoding);
		strncpy(strZ, stdstr.c_str(), maxL);
	}
	
	return strZ;
}

#ifdef _HAS_HANDLES_
CFDataRef		CFDataCreateWithHandle(Handle theH)
{
	return CFDataCreate(kCFAllocatorDefault, (UInt8*)*theH, GetHandleSize(theH));
}

void			Dict_Set_Handle(CFMutableDictionaryRef dict, const char *keyZ, Handle theH)
{
	ScCFReleaser<CFDataRef>		data(CFDataCreateWithHandle(theH));
	CCFString					keyRef(CFStringCreateWithC(keyZ));
	
	CFDictionarySetValue(dict, keyRef.Get(), data.Get());
}

Handle			Dict_Copy_Handle(CFDictionaryRef dict, const char *keyZ)
{
	Handle			valH = NULL;
	CCFString		keyRef(CFStringCreateWithC(keyZ));
	CFDataRef		valRef = (CFDataRef)CFDictionaryGetValue(dict, keyRef.Get());

	if (valRef) {
		CFIndex		lengthL = CFDataGetLength(valRef);
		
		valH = NewHandle(lengthL);
		
		if (valH) {
			CFDataGetBytes(valRef, CFRangeMake(0, lengthL), (UInt8*)*valH);
		}
	}

	return valH;
}
#endif

CFDataRef		Dict_Get_Data(CFDictionaryRef dict, const char *keyZ)
{
	CCFString		keyRef(CFStringCreateWithC(keyZ));
	
	return (CFDataRef)CFDictionaryGetValue(dict, keyRef.Get());
}

void		Dict_Set_Data(CFMutableDictionaryRef dict, const char *keyZ, CFDataRef dataRef)
{
	if (dataRef) {
		CCFString		keyRef(CFStringCreateWithC(keyZ));
		
		CFDictionarySetValue(dict, keyRef.Get(), dataRef);
	}
}

/****************************************************/
typedef struct {
	char		ch[2];
	UInt32		utf8;
} LegalCharRec;

/************************************************/
// DO NOT CHANGE ANY OF THESE BELOW
const LegalCharRec	g_old_charTable[] =  {
	{	":",	CFSwapInt32HostToBig(0xEFB99500)	},	//	(3A) (colon)				-> (small colon)				"EF B9 95"
	{	"-",	CFSwapInt32HostToBig(0xE2889200)	},	//	(2D) (minus-hyphen)			-> (minus sign)					"E2 88 92"
	{	"\"",	CFSwapInt32HostToBig(0xEFBC8200)	},	//	(22) (QUOTATION MARK)		-> (FULLWIDTH QUOTATION MARK)	"EF BC 82"
	{	"*",	CFSwapInt32HostToBig(0xEFB9A100)	},	//	(2A) (asterisk)				-> (SMALL ASTERISK)				"EF B9 A1"
	{	"?",	CFSwapInt32HostToBig(0xEFBC9F00)	},	//	(2A) (QUESTION MARK)		-> (FULLWIDTH QUESTION MARK)	"EF C9 F0"
	{	"<",	CFSwapInt32HostToBig(0xEFB9A400)	},	//	(3F) (LESS-THAN SIGN)		-> (SMALL LESS-THAN SIGN)		"EF B9 A4"
	{	">",	CFSwapInt32HostToBig(0xEFB9A500)	},	//	(3E) (GREATER-THAN SIGN)	-> (SMALL GREATER-THAN SIGN)	"EF B9 A5"
	{	"|",	CFSwapInt32HostToBig(0xEFBFA800)	},	//	(7C) (VERTICAL LINE)		-> (HALFWIDTH FORMS LIGHT VERTICAL)	"EF BF A8"
	{	"\\",	CFSwapInt32HostToBig(0xEFB9A800)	},	//	(5C) (REVERSE SOLIDUS)		-> (SMALL REVERSE SOLIDUS)		"EF B9 A8"
	{	"/",	CFSwapInt32HostToBig(0xE2818400)	},	//	(2F) (solidus)				-> (fraction slash)				"E2 81 84"
};
#define		kOldCharTableSize		(sizeof(g_old_charTable) / sizeof(LegalCharRec))
// DO NOT CHANGE ANY OF THESE ABOVE
/************************************************/

#define	kBackSlash		{	"\\",	CFSwapInt32HostToBig(0xC2AC0000)	},	//	(5C) (REVERSE SOLIDUS)	-> (NOT SIGN)
#define	kForwardSlash	{	"/",	CFSwapInt32HostToBig(0xC2A70000)	},	//	(2F) (solidus)			-> (SECTION SIGN)
#define	kDashChar		{	"-",	CFSwapInt32HostToBig(0xE2809300)	},	//	(2D) (minus-hyphen)		-> (EN DASH)


#if OPT_MACOS
	#define		kFileSystemSeparator		kForwardSlash
	#define		kOtherSlash					kBackSlash
#else
	#define		kFileSystemSeparator		kBackSlash
	#define		kOtherSlash					kForwardSlash
#endif

const LegalCharRec	g_charTable[] =  {
	{	":",	CFSwapInt32HostToBig(0x3B000000)	},	//	(3A) (colon)				-> (semicolon)
	{	"\"",	CFSwapInt32HostToBig(0xC2A80000)	},	//	(22) (QUOTATION MARK)		-> (DIARESIS)
	{	"*",	CFSwapInt32HostToBig(0xC2B70000)	},	//	(2A) (asterisk)				-> (MIDDLE DOT)
	{	"?",	CFSwapInt32HostToBig(0xC2B60000)	},	//	(2A) (QUESTION MARK)		-> (PILCROW SIGN)
	{	"<",	CFSwapInt32HostToBig(0xC2AB0000)	},	//	(3F) (LESS-THAN SIGN)		-> (LEFT-POINTING DOUBLE ANGLE QUOTATION MARK)
	{	">",	CFSwapInt32HostToBig(0xC2BB0000)	},	//	(3E) (GREATER-THAN SIGN)	-> (RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK)
	{	"|",	CFSwapInt32HostToBig(0xC2A60000)	},	//	(7C) (VERTICAL LINE)		-> (BROKEN BAR)
	kOtherSlash
	kDashChar
	kFileSystemSeparator
};
#define		kCharTableSize		(sizeof(g_charTable) / sizeof(LegalCharRec))

void		FixTrailingPeriod(bool recoverB, CFMutableStringRef stringRef)
{
	CFIndex			lenL = CFStringGetLength(stringRef);

	if (lenL > 0) {
		CFStringRef		periodStr(CFSTR("."));
		CFStringRef		underbarStr(CFSTR("_"));
		
		CFStringFindAndReplace(
			stringRef, 
			recoverB ? underbarStr	: periodStr, 
			recoverB ? periodStr	: underbarStr, 
			CFRangeMake(lenL - 1, 1), 0);
	}
}

static void		FixReservedWords(bool recoverB, CFMutableStringRef stringRef)
{
	static SStringVec	s_strVec;
	
	if (s_strVec.empty()) {
		s_strVec.push_back("con");
		s_strVec.push_back("nul");
		s_strVec.push_back("prn");
		s_strVec.push_back("aux");
		s_strVec.push_back("com0");
		s_strVec.push_back("com1");
		s_strVec.push_back("lpt0");
		s_strVec.push_back("lpt1");
	}
	
	if (!recoverB || SuperString(stringRef).StartsWith("x_")) {
	
		for (
			SStringVec::iterator it = s_strVec.begin();
			it != s_strVec.end();
			++it)
		{
			SuperString		conStr(*it);
			SuperString		xconStr(SuperString("x_") + conStr + SuperString("_x"));
			
			if (
				   (!recoverB && CFStringEqual(stringRef, conStr))
				|| (recoverB && CFStringEqual(stringRef, xconStr))
			) {
				xconStr.Set(stringRef);
				conStr.Set(stringRef);

				if (recoverB) {
					conStr.pop_front(2);
					conStr.pop_back(2);
				} else {
					xconStr.prepend("x_");
					xconStr.append("_x");
				}
				
				CFStringFindAndReplace(
					stringRef, 
					recoverB ? xconStr.ref()	: conStr.ref(), 
					recoverB ? conStr.ref()		: xconStr.ref(), 
					CFStrGetRange(stringRef), 0);
				break;
			}
		}
	}
}

CFStringRef		CFStrCreateRecoverableName(CFStringRef basisRef, CFRecoverType recoverType)
{
	#if OPT_WINOS
		bool			prefixB = false;
		SuperString		str(basisRef);
		SuperString		prefix;

		#ifdef _KJAMS_
			bool	IsWindowsLongPath(const SuperString& str, bool deviceB = false);
			
			prefixB = IsWindowsLongPath(str);
		#else
			prefixB = str.StartsWith("\\\\?\\");
		
		#endif
		
		if (prefixB) {
			prefix = str.pop_front(6);
		}

		basisRef = str.ref();
	#endif

	CCFString					emptyStr(CFCopyEmptyString());
	CFMutableStringRef			returnRef(CFStringCreateMutableCopy(NULL, 0, basisRef ? basisRef : emptyStr.ref()));
	SuperString					replaceStr;
	SuperString					withStr;
	size_t						sizeL = kCharTableSize;
	const LegalCharRec			*recA(g_charTable);
	
	if (recoverType == CFRecoverType_OLD) {
		bool		oldB = false;
		
		if (oldB) {
			sizeL = kOldCharTableSize;
			recA = g_old_charTable;
		}
	}
	
	const UInt8					*utf8Z;
	
	if (recoverType == CFRecoverType_EXCLUDE_SLASHES) {
		--sizeL;
	} else if (recoverType == CFRecoverType_EXCLUDE_SLASHES_DASHES) {
		sizeL -= 2;
	}

	loop (sizeL) {
		const LegalCharRec&		curRec(recA[_indexS]);
		
		replaceStr.Set(curRec.ch);
		utf8Z = (UInt8 *)&curRec.utf8;
		withStr.Set(utf8Z);
		CFStrReplaceWith(returnRef, replaceStr, withStr);
	}
	
	#ifdef _KJAMS_
	if (!gApp || !gApp->IsStartup()) 
	#endif
	{
		FixTrailingPeriod(false, returnRef);
		FixReservedWords(false, returnRef);
	}

	#if OPT_WINOS
		if (prefixB) {
			CFStringInsert(returnRef, 0, prefix.ref());
		}
	#endif

	/*
	if (CFStringCompare(basisRef, returnRef) != 0) {
		int i = 0;
	}
	 */

	return returnRef;
}

CFStringRef		CFStrRecoverName(CFStringRef basisRef)
{
	CCFString					emptyStr(CFCopyEmptyString());
	CFMutableStringRef			returnRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, basisRef ? basisRef : emptyStr.ref()));

	if (basisRef) {
		SuperString			replaceStr;
		SuperString			withStr;
		const UInt8			*utf8Z;
		
		loop (kCharTableSize) {
			const LegalCharRec&		curRec(g_charTable[_indexS]);

			utf8Z = (UInt8 *)&curRec.utf8;
			replaceStr.Set(utf8Z);
			withStr.Set(curRec.ch);
			CFStrReplaceWith(returnRef, replaceStr, withStr);
		}

		loop (kOldCharTableSize) {
			const LegalCharRec&		curRec(g_old_charTable[_indexS]);

			utf8Z = (UInt8 *)&curRec.utf8;
			replaceStr.Set(utf8Z);
			withStr.Set(curRec.ch);
			CFStrReplaceWith(returnRef, replaceStr, withStr);
		}
		
		FixReservedWords(true, returnRef);
		FixTrailingPeriod(true, returnRef);
	}

	/*
	if (CFStringCompare(basisRef, returnRef) != 0) {
		int i = 0;
	}
	 */

	return returnRef;
}

long			CStringToNum(const char *num)
{
	return SuperString(num).GetAs_SInt32();
}

const char		*NumToCString(long numL, std::string &str)
{
	SuperString		numStr(numL);
	
	str = numStr.c_str();
	return str.c_str();
}

long		CFStringToLong(const CFStringRef &num)
{
	std::string		str;
	long			valueL = 0;

	CopyCFStringToStd(num, str);
	sscanf(str.c_str(), "%ld", &valueL);
	return valueL;
}

float		CFNumberToFloat(const CFNumberRef &num)
{
	float	valueF = 0;
	
	CFNumberGetValue(num, kCFNumberFloatType, &valueF);
	return valueF;
}

CFComparisonResult	CFDateCompare(CFDateRef date1, CFDateRef date2)
{
	CFComparisonResult		compareResult = kCFCompareEqualTo;
	
	if (date1 == NULL || date2 == NULL) {

		if ((date1 == NULL) ^ (date2 == NULL)) {
			if (date1) {
				compareResult = kCFCompareLessThan;
			} else {
				compareResult = kCFCompareGreaterThan;
			}
		}
	} else {
		compareResult = CFDateCompare(date1, date2, NULL);
	}
	
	return compareResult;
}

bool		CFDateEqual(CFDateRef str1, CFDateRef str2)
{
	return CFDateCompare(str1, str2) == kCFCompareEqualTo;
}

bool		CFDateLess(CFDateRef lhs, CFDateRef rhs)
{
	return CFDateCompare(lhs, rhs) == kCFCompareLessThan;
}

bool		CFDictIsEmpty(CFDictionaryRef dictRef)
{
	return dictRef == NULL || CFDictionaryGetCount(dictRef) == 0;
}

const char *		CopyLongToC(long valL)
{
	static	char	s_bufAC[256];
	
	sprintf(s_bufAC, "%ld", valL);
	return s_bufAC;
}

CFStringRef		CFStringCreateWithNumber(long numberL)
{
	char	bufAC[32];
	
	sprintf(bufAC, "%ld", numberL);
	return CFStringCreateWithC(bufAC);
}

CFStringRef		CFStringCreateWithStd(const std::string &stdStr)
{
	return CFStringCreateWithC(stdStr.c_str());
}

CFNumberRef		CFNumberCreateWithNumber(long numberL)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &numberL);
}

long		CFNumberToLong(const CFNumberRef &num)
{
	long	valueL = 0;
	
	CFNumberGetValue(num, kCFNumberLongType, &valueL);
	return valueL;
}

CFNumberRef		CFNumberCreateWithFloat(float numberF)
{
	return CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &numberF);
}

std::string		&UpperString(std::string &str)
{
	const char	*strZ = str.c_str();
	CharVec		buf(str.size() + 1);
	
	loop (str.size()) {
		buf[_indexS] = toupper(strZ[_indexS]);
	}
	
	buf[str.size()] = 0;

	str = &buf[0];
	return str;
}

std::string		&LowerString(std::string &str)
{
	const char	*strZ = str.c_str();
	CharVec		buf(str.size() + 1);
	
	loop (str.size()) {
		buf[_indexS] = tolower(strZ[_indexS]);
	}
	
	buf[str.size()] = 0;

	str = &buf[0];
	return str;
}

ustring		&UpperString(ustring &str)
{
	const UTF8Char	*strZ = str.c_str();
	UCharVec		buf(str.size() + 1);
	
	loop (str.size()) {
		buf[_indexS] = toupper((char)strZ[_indexS]);
	}
	
	buf[str.size()] = 0;

	str = &buf[0];
	return str;
}

ustring		&LowerString(ustring &str)
{
	const UTF8Char	*strZ = str.c_str();
	UCharVec		buf(str.size() + 1);
	
	loop (str.size()) {
		buf[_indexS] = tolower((char)strZ[_indexS]);
	}
	
	buf[str.size()] = 0;

	str = &buf[0];
	return str;
}

void		Dict_Set_Str(CFMutableDictionaryRef dict, const char *keyZ, CFStringRef str)
{
	CCFString		keyRef(CFStringCreateWithC(keyZ));
	
	CFDictionarySetValue(dict, keyRef.Get(), str);
}

void		Dict_Set_Str(CFMutableDictionaryRef dict, const char *keyZ, const char *strZ)
{
	CCFString		stringRef(CFStringCreateWithC(strZ));
	
	Dict_Set_Str(dict, keyZ, stringRef);
}

CFStringRef		Dict_Get_Str(CFDictionaryRef dict, const char *keyZ)
{
	CCFString		keyRef(CFStringCreateWithC(keyZ));

	return (CFStringRef)CFDictionaryGetValue(dict, keyRef.Get());
}

bool			Dict_Get_Str(CFDictionaryRef dict, const char *keyZ, std::string &str)
{
	bool			foundB		= false;
	CFStringRef		stringRef	= Dict_Get_Str(dict, keyZ);

	foundB = stringRef != NULL;
	
	if (foundB) {
		CopyCFStringToStd(stringRef, str);
	}

	return foundB;
}

bool			Dict_Get_Str(CFDictionaryRef dict, const char *keyZ, ustring &str)
{
	bool			foundB		= false;
	CFStringRef		stringRef	= Dict_Get_Str(dict, keyZ);

	foundB = stringRef != NULL;
	
	if (foundB) {
		CopyCFStringToUString(stringRef, str);
	}

	return foundB;
}

bool			Dict_Get_Str(CFDictionaryRef dict, const char *keyZ, SuperString *strP)
{
	bool			foundB		= false;
	CFStringRef		stringRef	= Dict_Get_Str(dict, keyZ);

	foundB = stringRef != NULL;
	
	if (foundB) {
		strP->Set(stringRef);
	}

	return foundB;
}

void	Dict_Set_OSType(CFMutableDictionaryRef dict, const char *keyZ, OSType osType)
{
	char	nameAC[5];
	
	OSTypeToChar(osType, nameAC);
	Dict_Set_Str(dict, keyZ, nameAC);
}

OSType	Dict_Get_OSType(CFDictionaryRef dict, const char *keyZ, OSType defaultType)
{
	std::string		str;
	
	if (Dict_Get_Str(dict, keyZ, str)) {
		defaultType = CharToOSType(str.c_str());
	}
	
	return defaultType;
}

void	Dict_Set_Bool(CFMutableDictionaryRef dict, const char *keyZ, bool valB)
{
	CCFString		keyRef(CFStringCreateWithC(keyZ));
	
	CFDictionarySetValue(dict, keyRef.Get(), valB ? kCFBooleanTrue : kCFBooleanFalse);
}

bool	Dict_Get_Bool(CFDictionaryRef dict, const char *keyZ, bool defaultB)
{
	CCFString			keyRef(CFStringCreateWithC(keyZ));
	CFBooleanRef		boolRef	= (CFBooleanRef)CFDictionaryGetValue(dict, keyRef.Get());
	bool				valB	= defaultB;

	if (boolRef) {
		valB = boolRef == kCFBooleanTrue;
	}

	return valB;
}

void	Dict_Set_Float(CFMutableDictionaryRef dict, const char *keyZ, float valF)
{
	CCFString					keyRef(CFStringCreateWithC(keyZ));
	ScCFReleaser<CFNumberRef>	floatRef(CFNumberCreateWithFloat(valF));
	
	CFDictionarySetValue(dict, keyRef.Get(), floatRef.Get());
}

float	Dict_Get_Float(CFDictionaryRef dict, const char *keyZ, float defaultF)
{
	CCFString			keyRef(CFStringCreateWithC(keyZ));
	CFNumberRef			floatRef	= (CFNumberRef)CFDictionaryGetValue(dict, keyRef.Get());
	float				valF		= defaultF;

	if (floatRef) {
		valF = CFNumberToFloat(floatRef);
	}

	return valF;
}

void	Dict_Set_Long(CFMutableDictionaryRef dict, CFStringRef keyRef, long valL)
{
	ScCFReleaser<CFNumberRef>		valRef(CFNumberCreateWithNumber(valL));
	
	CFDictionarySetValue(dict, keyRef, valRef.Get());
}

void	Dict_Set_Long(CFMutableDictionaryRef dict, const char *keyZ, long valL)
{
	CCFString			keyRef(CFStringCreateWithC(keyZ));
	
	Dict_Set_Long(dict, keyRef, valL);
}

long	Dict_Get_Long(CFDictionaryRef dict, const char *keyZ, long defaultL)
{
	long			valL = defaultL;
	CCFString		keyRef(CFStringCreateWithC(keyZ));
	CFNumberRef		valRef	= (CFNumberRef)CFDictionaryGetValue(dict, keyRef.Get());

	if (valRef) {
		valL = CFNumberToLong(valRef);
	}

	return valL;
}

void	Dict_Set_Dict(CFMutableDictionaryRef dict, const char *keyZ, CFDictionaryRef subDict)
{
	if (subDict) {
		CCFString		keyRef(CFStringCreateWithC(keyZ));
		
		CFDictionarySetValue(dict, keyRef.Get(), subDict);
	}
}

CFDictionaryRef	Dict_Get_Dict(CFDictionaryRef dict, const char *keyZ)
{
	CCFString		keyRef(CFStringCreateWithC(keyZ));
	
	return (CFDictionaryRef)CFDictionaryGetValue(dict, keyRef.Get());
}

CFMutableDictionaryRef	Dict_Copy_Dict(CFDictionaryRef dict, const char *keyZ)
{
	CFMutableDictionaryRef		copyRef = NULL;
	CFDictionaryRef				curDict = Dict_Get_Dict(dict, keyZ);
	
	if (curDict) {
		copyRef = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, curDict);
	}

	return copyRef;
}

void			Dict_Set_Array(CFMutableDictionaryRef dict, const char *keyZ, CFArrayRef subArray)
{
	if (subArray) {
		CCFString		keyRef(CFStringCreateWithC(keyZ));
		
		CFDictionarySetValue(dict, keyRef.Get(), subArray);
	}
}

CFArrayRef		Dict_Get_Array(CFDictionaryRef dict, const char *keyZ)
{
	CCFString		keyRef(CFStringCreateWithC(keyZ));
	
	return (CFArrayRef)CFDictionaryGetValue(dict, keyRef.Get());
}

//	returns kCFNotFound if not available
CFIndex			Array_GetLongIndex(CFArrayRef array, long valL)
{
	ScCFReleaser<CFNumberRef>		valRef(CFNumberCreateWithNumber(valL));
	CFRange							rangeRef(CFRangeMake(0, CFArrayGetCount(array)));

	return CFArrayGetFirstIndexOfValue(array, rangeRef, valRef.Get());
}

long			Array_GetIndLong(CFArrayRef dict, CFIndex indexL)
{
	long				valL = 0;
	CFNumberRef			valRef	= (CFNumberRef)CFArrayGetValueAtIndex(dict, indexL);
	
	if (valRef) {
		valL = CFNumberToLong(valRef);
	}

	return valL;
}

SuperString		Array_GetIndStr(CFArrayRef array, long indexL)
{
	SuperString			str;
	CFStringRef			strRef	= (CFStringRef)CFArrayGetValueAtIndex(array, indexL);
	
	if (strRef) {
		str.Set(strRef);
	}
	
	return str;
}

void			Array_Add_Long_AtIndex(CFMutableArrayRef arrayRef, CFIndex atIndex, UInt32 valL)
{
	if (atIndex == kCFIndexEnd) {
		Array_Add_Long(arrayRef, valL);
	} else {
		ScCFReleaser<CFNumberRef>		valRef(CFNumberCreateWithNumber(valL));
		
		CFArrayInsertValueAtIndex(arrayRef, atIndex, valRef.Get());
	}
}

void		Array_Add_Long(CFMutableArrayRef arrayRef, UInt32 valL)
{
	ScCFReleaser<CFNumberRef>		valRef(CFNumberCreateWithNumber(valL));

	CFArrayAppendValue(arrayRef, valRef.Get());
}

void			Array_SetIndLong(CFMutableArrayRef dict, CFIndex indexL, long valueL)
{
	ScCFReleaser<CFNumberRef>	valRef(CFNumberCreateWithNumber(valueL));

	CFArraySetValueAtIndex(dict, indexL, valRef.Get());
}

//	returns first - second, in seconds
//	if either is NULL, treat it as 1 hour away from other
CFTimeInterval		CFDateDifference(CFDateRef date1P0, CFDateRef date2P0)
{
	CFTimeInterval		secondsF = 0;
	
	if (date1P0 == NULL || date2P0 == NULL) {

		if ((date1P0 == NULL) ^ (date2P0 == NULL)) {
		
			if (date1P0) {
				secondsF = kEventDurationHour;
			} else {
				secondsF = -kEventDurationHour;
			}
		}
	} else {
		secondsF = CFDateGetTimeIntervalSinceDate(date1P0, date2P0);
	}
	
	return secondsF;
}

CFDateRef	CFDateCreateCurrent()
{
	return CFDateCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent());
}

void		Dict_Set_Date(CFMutableDictionaryRef dict, const char *keyZ, CFDateRef dateRef)
{
	if (dateRef) {
		CCFString				keyRef(CFStringCreateWithC(keyZ));

		CFDictionarySetValue(dict, keyRef.Get(), dateRef);
	}
}

CFDateRef		Dict_Copy_Date(CFDictionaryRef dict, const char *keyZ)
{
	CFStringRef			keyRef	= CFStringCreateWithC(keyZ);
	CFDateRef			dateRef	= (CFDateRef)CFDictionaryGetValue(dict, keyRef);

	CFReleaseDebug(keyRef);
	CFRetainDebug(dateRef);
	return dateRef;
}

void	Dict_Set_Rect(CFMutableDictionaryRef dict, const char *keyZ, const Rect &valR)
{
	CFStringRef					keyRef		= CFStringCreateWithC(keyZ);
	CFMutableDictionaryRef		rectDictRef = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
		
	if (rectDictRef) {
		Dict_Set_Long(rectDictRef, DICT_STR_RECT_LEFT,		valR.left);
		Dict_Set_Long(rectDictRef, DICT_STR_RECT_TOP,		valR.top);

		Dict_Set_Long(rectDictRef, DICT_STR_RECT_RIGHT,		valR.right);
		Dict_Set_Long(rectDictRef, DICT_STR_RECT_BOTTOM,	valR.bottom);
		
		Dict_Set_Long(rectDictRef, DICT_STR_RECT_WIDTH,		valR.right - valR.left);
		Dict_Set_Long(rectDictRef, DICT_STR_RECT_HEIGHT,	valR.bottom - valR.top);

		CFDictionarySetValue(dict, keyRef, rectDictRef);
		CFReleaseDebug(rectDictRef);
	}

	CFReleaseDebug(keyRef);
}

#define	kRectElementDefault		-1
Rect	Dict_Get_Rect(CFDictionaryRef in_dict, const char *keyZ)
{
	Rect				valR = { 0, 0, 0, 0 };
	CCFDictionary		dict(in_dict, true);
	CCFDictionary		rectDict(dict.GetAs_Dict(keyZ), true);
	
	if (!rectDict.empty()) {
		valR.left		= rectDict.GetAs_SInt16(DICT_STR_RECT_LEFT);
		valR.top		= rectDict.GetAs_SInt16(DICT_STR_RECT_TOP);

		SInt16		widthS	= rectDict.GetAs_SInt16(DICT_STR_RECT_WIDTH, kRectElementDefault);
		SInt16		heightS	= rectDict.GetAs_SInt16(DICT_STR_RECT_HEIGHT, kRectElementDefault);

		if (heightS == kRectElementDefault && widthS == kRectElementDefault) {
			valR.right		= rectDict.GetAs_SInt16(DICT_STR_RECT_RIGHT);
			valR.bottom		= rectDict.GetAs_SInt16(DICT_STR_RECT_BOTTOM);
		} else {
			valR.right		= valR.left + widthS;
			valR.bottom		= valR.top + heightS;
		}
	}
		
	return valR;
}

static void	Dict_Set_RGB(CFMutableDictionaryRef dictRef, const RGBColor &valR)
{
	Dict_Set_Long(dictRef, DICT_STR_COLOR_RED,		valR.red);
	Dict_Set_Long(dictRef, DICT_STR_COLOR_GREEN,	valR.green);
	Dict_Set_Long(dictRef, DICT_STR_COLOR_BLUE,		valR.blue);
}

static	RGBColor	Dict_Get_RGB(CFDictionaryRef in_dict)
{
	RGBColor			valR = { 0, 0, 0 };
	CCFDictionary		dict(in_dict, true);

	valR.red		= dict.GetAs_SInt16(DICT_STR_COLOR_RED);
	valR.green		= dict.GetAs_SInt16(DICT_STR_COLOR_GREEN);
	valR.blue		= dict.GetAs_SInt16(DICT_STR_COLOR_BLUE);
	return valR;
}

RGBColor	Dict_Get_Color(CFDictionaryRef dict, const char *keyZ)
{
	RGBColor			valR		= { 0, 0, 0 };
	CFStringRef			keyRef		= CFStringCreateWithC(keyZ);
	CFDictionaryRef		dictRef		= (CFDictionaryRef)CFDictionaryGetValue(dict, keyRef);

	CFReleaseDebug(keyRef);

	if (dictRef) {
		valR = Dict_Get_RGB(dictRef);
	}

	return valR;
}

void	Dict_Set_Color(CFMutableDictionaryRef dict, const char *keyZ, const RGBColor &valR)
{
	CFStringRef					keyRef	= CFStringCreateWithC(keyZ);
	CFMutableDictionaryRef		dictRef = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
		
	if (dictRef) {
		Dict_Set_RGB(dictRef, valR);
		CFDictionarySetValue(dict, keyRef, dictRef);
		CFReleaseDebug(dictRef);
	}

	CFReleaseDebug(keyRef);
}

void	Array_Append_ColorSpec(CFMutableArrayRef array, const ColorSpec &cspec)
{
	CFMutableDictionaryRef		dictRef = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 0,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
		
	if (dictRef) {
		Dict_Set_Long(dictRef, DICT_STR_COLOR_VALUE,	cspec.value);
		Dict_Set_RGB(dictRef, cspec.rgb);

		CFArrayAppendValue(array, dictRef);
		CFReleaseDebug(dictRef);
	}
}

ColorSpec	Array_GetInd_ColorSpec(CFArrayRef in_array, CFIndex indexL)
{
	ColorSpec				cspec;	structclr(cspec);
	CCFArray				array(in_array, true);
	CCFDictionary			dict(array.GetIndValAs_Dict(indexL), true);
	
	cspec.value		= dict.GetAs_SInt16(DICT_STR_COLOR_VALUE);
	cspec.rgb		= Dict_Get_RGB(dict.ref());
	
	return cspec;
}

void		SplitOffExtension(std::string &str, std::string *extensionP0)
{
	const char	*dotP;
	
	dotP = strrchr(str.c_str(), '.');
	
	if (dotP) {
		if (extensionP0) {
			*extensionP0 = dotP;
		}
		
		str.resize(str.size() - strlen(dotP));
	}
}

/*************************************************************/
class CHexify {
	ustring &i_str;
	
	public:
	CHexify(ustring &str) : i_str(str) { }
	void operator()(UInt8 ch) {
		char			bufAC[16];
		
		sprintf(bufAC, "%.2x", (int)ch);
		CF_ASSERT(strlen(bufAC) == 2);
		i_str.append(uc(bufAC));
	}
};

ustring			Hexify(const UInt8* bufA, size_t sizeL, bool to_upperB)
{
	ustring		resultStr;
	
	resultStr.reserve(sizeL * 2);
	std::for_each(&bufA[0], &bufA[sizeL], CHexify(resultStr));
	
	if (to_upperB) {
		resultStr = SuperString(resultStr).ToUpper().utf8();
	}
	
	return resultStr;
}

ustring			Hexify(const UCharVec& vec, bool to_upperB)
{
	return Hexify(&vec[0], vec.size(), to_upperB);
}

/*************************************************************/
class CUnHexify {
	UCharVec	&i_vec;
	UInt8		i_prevChar;
	bool		got_prevB;
	
	public:
	CUnHexify(UCharVec &vec) : i_vec(vec), got_prevB(false) { }
	void operator()(UInt8 ch) {
		
		if (got_prevB) {
			CharVec		buf;
			
			buf.push_back(i_prevChar);
			buf.push_back(ch);
			buf.push_back(0);

			unsigned int	val;
			
			sscanf(&buf[0], "%x", &val);
			
			i_prevChar = val;
			CF_ASSERT(i_prevChar == val);
			i_vec.push_back(i_prevChar);
			got_prevB = false;
		} else {
			got_prevB = true;
			i_prevChar = ch;
		}
	}
};

UCharVec		UnHexify(const ustring& str)
{
	UCharVec		resultVec;
	
	resultVec.reserve(str.size() / 2);
	std::for_each(str.begin(), str.end(), CUnHexify(resultVec));
	return resultVec;
}

std::string		ULong_To_Hex(UInt32 valueL)
{
	char			bufAC[16];
	
	sprintf(bufAC, "%.8lx", (long unsigned int)valueL);
	return bufAC;
}

UInt32			Hex_To_ULong(const char *hexZ)
{
	unsigned long	valL = 0;
	
	if (hexZ[0]) {
		sscanf(hexZ, "%lx", &valL);
	}

	return (UInt32)valL;
}

std::string		ULLong_To_Hex(UInt64 valueL)
{
	char			bufAC[32];
	
	sprintf(bufAC, "%.16llx", (long long unsigned int)valueL);
	return bufAC;
}

UInt64			Hex_To_ULLong(const char *hexZ)
{
	unsigned long long		valL = 0;
	
	if (hexZ[0]) {
		sscanf(hexZ, "%llx", &valL);
	}

	return (UInt64)valL;
}

/*************************************************************/
#if TRACK_RETAINS
bool	s_trackB = true;
	
ScTrackRetains::ScTrackRetains()
{
#if !defined(_ROSE_) && !_PaddleServer_
	i_dataP = new CCritical(LogMutex());
#endif
}

ScTrackRetains::~ScTrackRetains()
{
#if !defined(_ROSE_) && !_PaddleServer_
	CCritical	*cP = (CCritical *)i_dataP;
	
	delete cP;
#endif
}

#define		kCFRetainCountStatic64		0x7FFFFFFFFFFFFFFF
#define		kCFRetainCountStatic64_2	0x0FFFFFFFFFFFFFFF
#define		kCFRetainCountStatic64_3	0xFFFFFFFFFFFFFFFF
#define		kCFRetainCountStatic32		0x7FFFFFFF
#define		kCFRetainCountStatic32_2	0x0FFFFFFF
#define		kCFRetainCountStatic32_3	0xFFFFFFFF

#if __LP64__
	#define		kCFRetainCountStatic		kCFRetainCountStatic64
	#define		kCFRetainCountStatic2		kCFRetainCountStatic64_2
	#define		kCFRetainCountStatic3		kCFRetainCountStatic64_3
#else
	#define		kCFRetainCountStatic		kCFRetainCountStatic32
	#define		kCFRetainCountStatic2		kCFRetainCountStatic32_2
	#define		kCFRetainCountStatic3		kCFRetainCountStatic32_3
#endif

bool		IsStaticStr(CFIndex retainCountL)
{
	CFIndex			static1(kCFRetainCountStatic);
	CFIndex			static2(kCFRetainCountStatic2);
	CFIndex			static3(kCFRetainCountStatic3);
	
	bool		staticB = 
		   retainCountL == static1 
		|| retainCountL == static2
		|| retainCountL == static3;
	
	#if OPT_WINOS && __LP64__
		static1 = kCFRetainCountStatic32;
		static2 = kCFRetainCountStatic32_2;
		static3 = kCFRetainCountStatic32_3;
		
		staticB |= 
			   retainCountL == static1 
			|| retainCountL == static2
			|| retainCountL == static3;
	#endif
	
	return staticB;
}

void		LogRetainRelease(bool retainB, CFTypeRef cf, bool do_itB = true)
{
	if (do_itB) {
	
		if (s_trackB) {
			CFIndex		countL = CFGetRetainCount(cf);
			
			if (!IsStaticStr(countL)) {
				CFIndex		end_countL	= countL + (retainB ? 1 : -1);
				bool		bugB = false;
				
				if (countL <= 0 || countL > 1000000) {
					bugB = true;
					CFDebugBreak();
				}

				if (end_countL < 0 || end_countL > 1000000) {
					bugB = true;
					CFDebugBreak();
				}
				
				if (bugB && IsDefaultEncodingSet()) {
					ScTrackRetains		sc1;	//	mutex
					ScSetReset<bool>	sc(&s_trackB, false);	//	protected by mutex
					SuperString			str;
					
					if (retainB) {
						str.Set("CFRetain  ");
					} else {
						str.Set("CFRelease ");
					}

					str.append("%d -> %d: 0x%p, <<%@>>");
					str.ssprintf(NULL, countL, end_countL, cf, cf);

					Log(str.utf8Z());

#if defined(_ROSE_) || _PaddleServer_
					CF_ASSERT(0);
#else
					new MT_CrashPlease();
#endif
				}
			}
		}

		//	these calls actually have no effect on a static string
		if (retainB) {
			::CFRetain(cf);
		} else {
			::CFRelease(cf);
		}
	}
}

CFTypeRef	CFRetainDebug(CFTypeRef cf, bool do_itB)
{
	LogRetainRelease(true, cf, do_itB);
	return cf;
}

void		CFReleaseDebug(CFTypeRef cf)
{
	LogRetainRelease(false, cf);
}
#endif

CFStringEncoding	ValidateEncoding(CFStringEncoding encoding)
{
	if (encoding == kCFStringEncodingInvalidId) {
		encoding = kCFStringEncodingMacRoman;	//	GetApplicationTextEncoding();
	}
	
	return encoding;
}

/*************************************************************/
//	compare string IS case-insensitive
SuperStringReplaceRec	s_interCapA[] = {
	{	"Cavs",		"CAVS"		}, 
	{	"Abba",		"ABBA"		}, 
	{	"Sabbath",	"Sabbath"	}, 
	{	"Ac/Dc",	"AC/DC"		}, 
	{	"Zz Top",	"ZZ Top"	}, 
	{	"Xtc",		"XTC"		}, 
	{	"Ub40",		"UB40"		}, 
	{	" mc",		" MC"		}, 
	{	"mc-5",		"MC-5"		}, 
	{	" dj ",		" DJ "		}, 
	{	" bj ",		" BJ "		}, 
	{	" tv ",		" tv "		}, 
	{	"leann",	"LeAnn"		}, 
	{	"bbmak",	"BBMak"		}, 
	{	"dna",		"DNA"		}, 
	{	"dmc",		"DMC"		}, 
	{	"lmc",		"LMC"		}, 
	{	"elo",		"ELO"		}, 
	{	"Belong",	"Belong"	}, 
	{	"nyc",		"NYC"		}, 
	{	"omc",		"OMC"		}, 
	{	"omd",		"OMD"		}, 
	{	"tlc",		"TLC"		}, 
	{	"ymca",		"YMCA"		}, 
	{	"10Cc",					"10CC"		}, 
	{	"Tv Theme",				"TV Theme"	}, 
	{	" Ii ",					" II "		}, 
	{	"-Nsync",				"'N Sync"	}, 
	{	"Adam Ant",				"Adam and the Ants"	}, 
	{	"Adam & the Ants",		"Adam and the Ants"	}, 
	{	"Ac-Dc",				"AC/DC"		}, 
	{	"Ac Dc",				"AC/DC"		}, 
	{	"AcDc",					"AC/DC"		}, 
	{	"inxs",					"INXS"		}, 
	{	"Alanis Morrisette",	"Alanis Morrisete"	}, 
	{	" Folder",	""	}, 
	{	".zip",		""	}, 
};
#define	s_interCapSize	(sizeof(s_interCapA) / sizeof(SuperStringReplaceRec))

SuperStringReplaceRec	s_titleCaseA[] = {
	{	" A ",		" a "		}, 
	{	" An ",		" an "		}, 
	{	" And ",	" and "		}, 
	{	" At ",		" at "		}, 
	{	" But ",	" but "		}, 
	{	" By ",		" by "		}, 
	{	" Del ",	" del "		}, 
	{	" For ",	" for "		}, 
	{	" From ",	" from "	}, 
	{	" In ",		" in "		}, 
	{	" Into ",	" into "	}, 
	{	" Is ",		" is "		}, 
	{	" Nor ",	" nor "		}, 
	{	" Not ",	" not "		}, 
	{	" Of ",		" of "		}, 
	{	" On ",		" on "		}, 
	{	" Onto ",	" onto "	}, 
	{	" Or ",		" or "		}, 
	{	" So ",		" so "		}, 
	{	" That ",	" that "	}, 
	{	" The ",	" the "		}, 
	{	" This ",	" this "	}, 
	{	" With ",	" with "	}, 
	{	" To ",		" to "		}, 

	{	" Y ",		" y "		}, 
	{	" El ",		" el "		}, 
	{	" Los ",	" los "		}, 
	{	" La ",		" la "		}, 
	{	" las ",	" las "		}, 
};

#define	s_titleCaseSize	(sizeof(s_titleCaseA) / sizeof(SuperStringReplaceRec))

SuperString&	SuperString::InterCaps(bool allow_line_breaksB, bool titleCaseB)
{
	ScCFReleaser<CFMutableStringRef>	capsRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, i_ref));
	CFLocaleRef							locale(CFLocaleGetSystem());
	
	CFStringLowercase(capsRef, locale);
	CFStringCapitalize(capsRef, locale);
	
	if (!allow_line_breaksB) {
		CCFString		emptyStr(CFCopyEmptyString());
		
		CFStrReplaceWith(capsRef, SuperString(RETURN_STR).ref(), emptyStr);
		CFStrReplaceWith(capsRef, SuperString(LINEFEED_STR).ref(), emptyStr);
	}

	CFStringTrimWhitespace(capsRef);
	
	Set(capsRef);

	if (titleCaseB) {
		ReplaceTable(s_titleCaseA, s_titleCaseSize);
	}

	ReplaceTable(s_interCapA, s_interCapSize);

	utf8();

	//	first, upper to lower all the replacements
	Replace("O'", "o'");
	Replace(" Mc", " mc");
	Replace(",Mc", ",mc");

	bool	beginB = MatchStart("Mc") != 0;

	if (beginB) {
		Replace("mc", "mc");
	}
	
	if (Contains("o'") || Contains("mc")) {
		UCharVec	bufAC(utf8().size());
		
		std::copy(i_utf8->begin(), i_utf8->end(), &bufAC[0]);
		bufAC.push_back(0);

		char	*chP = strstr((char *)&bufAC[0], "o'");
		if (chP && chP[2] && isalpha(chP[2])) {
			chP[0] = toupper(chP[0]);
			chP[2] = toupper(chP[2]);
		}

		chP = NULL;
		if (beginB)	chP = (char *)&bufAC[0];
		if (!chP) chP = strstr((char *)&bufAC[0], " mc");
		if (!chP) chP = strstr((char *)&bufAC[0], ",mc");
		if (chP) {
			if (chP[0] != 'm') ++chP;
			
			if (chP[2] && isalpha(chP[2])) {
				chP[0] = toupper(chP[0]);
				chP[2] = toupper(chP[2]);
			}
		}
		
		Set(&bufAC[0]);
	}
	
	if (*this == "Beatles") {
		Set("The Beatles");
	}
	
	return *this;
}

SuperString&	SuperString::TheToEnd()
{
	static const char *	s_theToEndA[] = {
		"The", 
		"La", 
		"Las", 
		"El", 
		"Los"
	};
	#define	s_theToEndSize	(sizeof(s_theToEndA) / sizeof(char *))

	SuperString		str;
	
	loop (s_theToEndSize) {
		str.Set(s_theToEndA[_indexS]);
		str.append(" ");
		
		if (StartsWith(str)) {
			pop_front(str.size());
			append(", ");
			append(s_theToEndA[_indexS]);
		}
	}
	
	return *this;
}

#define		kCFStringEncodingMacRomanPlus		kCFStringEncodingMacRoman	| 0x00020000

typedef std::map<SuperString, CFStringEncoding>		SStringEncodingMap;


CFStringEncoding	CFLangRgnStrToEncoding(const SuperString& localeIDStr)
{
	CFStringEncoding			encoding = kCFStringEncodingInvalidId;
	static SStringEncodingMap	s_langMap;
	
	if (s_langMap.empty()) {
		s_langMap["en"]		= kCFStringEncodingMacRomanPlus;
		s_langMap["en_US"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["en_GB"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["en_AU"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["en_CA"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["en_SG"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["en_IE"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["fr"]		= kCFStringEncodingMacRomanPlus;
		s_langMap["fr_FR"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["fr_CA"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["fr_CH"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["fr_BE"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["de"]		= kCFStringEncodingMacRomanPlus;
		s_langMap["de_DE"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["de_CH"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["de_AT"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["ja"]		= kCFStringEncodingMacJapanese;
		s_langMap["it"]		= kCFStringEncodingMacRomanPlus;
		s_langMap["it_IT"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["it_CH"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["es"]		= kCFStringEncodingMacRomanPlus;
		s_langMap["es_ES"]	= kCFStringEncodingMacRomanPlus;
		s_langMap["es_XL"]	= kCFStringEncodingInvalidId;
	}
	
	SStringEncodingMap::iterator		it(s_langMap.find(localeIDStr));
	
	if (it != s_langMap.end()) {
		encoding = it->second;
	}
			
	return encoding;
}

CFStringEncoding	LanguageRegionToEncoding(const SuperString& langRgn)
{
	CFStringEncoding			encoding = kCFStringEncodingInvalidId;
	
	#if _JUST_CFTEST_
		encoding = CFLangRgnStrToEncoding(langRgn);
	#else
	
	ScCFReleaser<CFLocaleRef>	localeRef(CFLocaleCreate(kCFAllocatorDefault, langRgn.ref()));
	SuperString					languageCodeStr((CFStringRef)CFLocaleGetValue(localeRef, kCFLocaleLanguageCode));
	SuperString					regionCodeStr((CFStringRef)CFLocaleGetValue(localeRef, kCFLocaleCountryCode));
	LangCode					langCode = langEnglish;
	RegionCode					regionCode = verUS;
	ScriptCode					scriptCode = smRoman;

	if (languageCodeStr == "en") {
		langCode = langEnglish;
		
		if (regionCodeStr.empty() || regionCodeStr == "US") {
			regionCode = verUS;
		} else if (regionCodeStr == "GB") {
			regionCode = verBritain;
		} else if (regionCodeStr == "AU") {
			regionCode = verAustralia;
		} else if (regionCodeStr == "CA") {
			regionCode = verEngCanada;
		} else if (regionCodeStr == "SG") {
			regionCode = verSingapore;
		} else if (regionCodeStr == "IE") {
			regionCode = verIrelandEnglish;
		}
	} else if (languageCodeStr == "fr") {
		langCode = langFrench;

		if (regionCodeStr.empty() || regionCodeStr == "FR") {
			regionCode = verFrance;
		} else if (regionCodeStr == "CA") {
			regionCode = verFrCanada;
		} else if (regionCodeStr == "CH") {
			regionCode = verFrSwiss;
		} else if (regionCodeStr == "BE") {
			regionCode = verFrBelgium;
		}
	} else if (languageCodeStr == "de") {
		langCode = langGerman;

		if (regionCodeStr.empty() || regionCodeStr == "DE") {
			regionCode = verGermany;
		} else if (regionCodeStr == "CH") {
			regionCode = verGrSwiss;
		} else if (regionCodeStr == "AT") {
			regionCode = verAustria;
		}
	} else if (languageCodeStr == "ja") {
		langCode = langJapanese;
		regionCode = verJapan;
		scriptCode = smJapanese;
	} else if (languageCodeStr == "it") {
		langCode = langItalian;

		if (regionCodeStr.empty() || regionCodeStr == "IT") {
			regionCode = verItaly;
		} else if (regionCodeStr == "CH") {
			regionCode = verItalianSwiss;
		}
	} else if (languageCodeStr == "es") {
		langCode = langSpanish;

		if (regionCodeStr.empty() || regionCodeStr == "ES") {
			regionCode = verSpain;
		} else if (regionCodeStr == "XL") {
			regionCode = verSpLatinAmerica;
		}
	}
	
	#if OPT_MACOS
	{
		OSStatus	err = noErr;
		
		ERR(GetTextEncodingFromScriptInfo(
			scriptCode, langCode, regionCode, &encoding));
		
		if (err) {
			encoding		= kCFStringEncodingInvalidId;
		}
	}
	#endif
	
	#endif // _KJAMS_

	return encoding;
}

UInt32		CFTickCount()
{
	static CFAbsoluteTime	s_startT(0);
	//	on mac more correct to use GetCurrentEventTime() (seconds since boot)
	CFAbsoluteTime			curT(CFAbsoluteTimeGetCurrent());
	
	if (s_startT == 0) {
		s_startT = curT - kEventDurationHour;	//	pretend we booted an hour ago
	}
	
	curT = curT - s_startT;

	return EventTimeToTicks(curT);
}

SInt64			CFSecondsToMilliseconds(CFTimeInterval secF)
{
	double		milliF(secF * (double)kDurationSecond);

	return math_round(milliF);
}

CFTimeInterval		CFMillisecondsToSeconds(SInt64 milliI)
{
	return milliI / (double)kDurationSecond;
}
