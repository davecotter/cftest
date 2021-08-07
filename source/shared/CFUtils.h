#ifndef _H_CFUtils
#define _H_CFUtils

#if defined(_KJAMS_)
	#include <boost/foreach.hpp>
#endif

#include <vector>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <CoreFoundation/CoreFoundation.h>

#if !_PaddleServer_
	#if defined(_ROSE_)
		#include <CFNetwork/CFHTTPMessage.h>
	#else
		#include <CFHTTPMessage.h>
	#endif
#endif

#include "CCFError.h"
#include "QDUtils.h"

#if _QT_ && !_JUST_CFTEST_
	#include <QDateTime>
#else
	#define __CFUUID__
#endif

#ifndef CF_OPTIONS
	#define CF_OPTIONS(_type, _name) _type _name; enum
#endif

#if _QT_ && !_JUST_CFTEST_
	#include <QtGlobal>

	#if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
		#define _QT6_	1
	#else
		#define _QT6_	0
	#endif

#endif


/******************************************************************************/
#define		LERP(to_min, to_max, from, from_min, from_max)	\
	((from_max) == (from_min) ? from : 						\
	(double)(to_min) + ((double)((to_max) - (to_min))		\
		* ((double)((from) - (from_min))					\
		/ (double)((from_max) - (from_min)))))

#define		LERP_PERCENT(from, from_max) \
	LERP(0.0f, 1.0f, from, 0.0f, from_max)

#if OPT_WINOS || defined(_rose_spotlight_)

#define	isnumber(_c)	(isalnum(_c) && !isalpha(_c))

#if defined(_QTServer_)
	#include "QuickDraw.h"
#endif // defined(_QTServer_)

#if !defined(_KJAMS_) && !defined(_ROSE_) && !defined(_QTServer_)
	#define		kDefineMacOSErrors	1
#else
	#define		kDefineMacOSErrors	0
#endif

#if _QT_ || kDefineMacOSErrors

enum {
	abortErr                      = -27,  //IO call aborted by KillIO
	mFulErr                       = -41,  //memory full (open) or file won't fit (load)
	eofErr                        = -39,  //End of fil
	memFullErr                    = -108, //Not enough room in heap zone
	updPixMemErr                  = -125, /*insufficient memory to update a pixmap*/
	urlDataHHTTPURLErr				= -2131,
	urlDataHHTTPProtocolErr			= -2129,
	urlDataHHTTPRedirectErr			= -2132,
	urlDataHFTPPermissionsErr		= -2141,
	kURLFileEmptyError				= -30783,
	invalidIndexErr					= -20002,
	notExactSizeErr					= -2206,
	hrMiscellaneousExceptionErr		= -5361,
	userCanceledErr					= -128,
	networkErr						= -925,
	kOTNoDataErr					= -3162,
	kENOTCONNErr					= -3256,
	kNSpHostFailedErr				= -30365,
	kNSpTimeoutErr					= -30393,
	procNotFound                  = -600, //no eligible process with specified descriptor
	vLckdErr                      = -46,  /*volume is locked*/
	threadTooManyReqsErr          = -617,
	threadNotFoundErr             = -618,
	fsDataTooBigErr               = -1310, /*file or volume is too big for system*/
	kBadArgLengthErr              = -9063, /* ArgLength argument is invalid*/
	notEnoughDataErr              = -2149,
//	duped in BasicTypes.h ??? factor
/*
	ioErr                         = -36,  //I/O error (bummers)
  	tmfoErr                       = -42,  //too many files open
	fnfErr                        = -43,  //File not found
	wPrErr                        = -44,  //diskette is write protected.
	fLckdErr                      = -45   //file is locked
*/
};

#define	kUnicodeUTF16Format				0
#define	kUnicodeHFSPlusDecompVariant	8
#define	kUnicodeHFSPlusCompVariant		9

typedef UInt32		TextEncoding;
typedef UInt32		TextEncodingBase;
typedef UInt32		TextEncodingVariant;
typedef UInt32		TextEncodingFormat;

enum {
                                        /* Mac OS encodings*/
  kTextEncodingMacRoman         = 0L,
  kTextEncodingMacJapanese      = 1,
  kTextEncodingMacChineseTrad   = 2,
  kTextEncodingMacKorean        = 3,
  kTextEncodingMacArabic        = 4,
  kTextEncodingMacHebrew        = 5,
  kTextEncodingMacGreek         = 6,
  kTextEncodingMacCyrillic      = 7,
	kTextEncodingISOLatinCyrillic = 0x0205, /* ISO 8859-5*/
	kTextEncodingDOSCyrillic      = 0x0413, /* code page 855, IBM Cyrillic*/
	kTextEncodingWindowsCyrillic  = 0x0502, /* code page 1251, Slavic Cyrillic*/
};

#if !_QT_
	#define		__MACTYPES__
#endif

#endif	//	_QT_ || kDefineMacOSErrors
#endif	//	OPT_WINOS || defined(_rose_spotlight_)

/******************************************************************************/

#if !defined(_CFTEST_) && !_PaddleServer_
	#include "XPlatConditionals.h"
#endif

#ifdef _KJAMS_
	#include "QDUtils.h"
#endif

#ifndef memclr
	#define	offset_of(_s, _m)		(size_t)((char *)&(_s)._m - (char *)&(_s))
	#define	structclr_section(_struct, _begin, _end)	\
		std::fill((char *)&_struct + offset_of(_struct, _begin), (char *)&_struct + offset_of(_struct, _end), 0)

	#define	memclr(p, s)	std::memset(p, 0, s)
	#define	structclr(p)	memclr(&p, sizeof(p))
#endif

static const CFIndex kCFIndex_0		= 0;
//static const CFIndex kCFNotFound = -1;
static const CFIndex kCFIndexEnd	= -2;

#define		kMacOS_System6			0x060000
#define		kMacOS_10_3				0x100300		//	Panther
#define		kMacOS_10_4				0x100400		//	Tiger
#define		kMacOS_10_5				0x100500		//	Leopard
#define		kMacOS_10_6				0x100600		//	SnowLeopard
#define		kMacOS_10_7				0x100700		//	Lion
#define		kMacOS_10_8				0x100800		//	MountainLion
#define		kMacOS_10_9				0x100900		//	Mavericks
#define		kMacOS_10_10			0x101000		//	Yosemite
#define		kMacOS_10_11			0x101100		//	ElCapitan
#define		kMacOS_10_12			0x101200		//	Sierra
#define		kMacOS_10_13			0x101300		//	HighSierra	
#define		kMacOS_10_14			0x101400		//	Mojave
#define		kMacOS_10_15			0x101500		//	Catalina
#define		kMacOS_11_0				0x110000		//	Big Sur
#define		kMacOS_11_1				0x110100		//	Monterey
#define		kMacOS_11_2				0x110200		//	?

#define		kMacOS_Current			kMacOS_11_0		//	currently released
#define		kMacOS_Next				kMacOS_11_1		//	not released yet

class SuperString;

typedef SInt32	SystemVersType;
SystemVersType	GetSystemVers(void);
SuperString		GetSystemVersStr(SystemVersType type);

#define		FourCharQuestionMarks	0x3F3F3F3F	//	'????'

#if __cplusplus >= 201103L
	#include <memory>
	template <typename T>
	using CFAutoPtr = std::unique_ptr<T>;
#else
	#if _QT_
		template <typename T>
		typedef std::auto_ptr<T>	CFAutoPtr;
	#else
		#define	CFAutoPtr			std::auto_ptr	
	#endif
#endif

#if !_QT6_
namespace std {

	template<typename T>
	T	clamp(const T& in_v, const T& min, const T& max) {
		T	v(in_v);
		
		if (v < min) {
			v = min;
		} else if (v > max) {
			v = max;
		}

		return v;
	}
};
#endif

#if OPT_MACOS
	#define	UNREFERENCED_PARAMETER(_foo)

	#if _QT_
		#define	DEF_PASCAL
	#else
		#define	DEF_PASCAL	pascal
	#endif

	#if COMPILING_ON_10_4
		#define	kHIThemeTextHorizontalFlushDefault	kHIThemeTextHorizontalFlushLeft

		enum {
			kCFCompareDiacriticInsensitive = 128, /* If specified, ignores diacritics (o-umlaut == o) */
			kCFCompareWidthInsensitive = 256, /* If specified, ignores width differences ('a' == UFF41) */
			
			/* 
				If kCFCompareForcedOrdering is specified, comparisons are forced to return either kCFCompareLessThan 
				or kCFCompareGreaterThan if the strings are equivalent but not strictly equal, for stability when 
				sorting (e.g. "aaa" > "AAA" with kCFCompareCaseInsensitive specified)
			*/
			kCFCompareForcedOrdering = 512 
		};
		
		enum CFNetworkErrors {
		  kCFURLErrorNetworkConnectionLost = -1005
		};
	#endif
	
	#if _QT_
	#include <CFNetwork/CFNetworkErrors.h>
	#endif
	
#else
	#define	DEF_PASCAL
	
	#if !_PaddleServer_
	enum CFNetworkErrors {
		kCFHostErrorUnknown					= 2,		 // Query the kCFGetAddrInfoFailureKey to get the value returned from getaddrinfo; lookup in netdb.h
		kCFURLErrorNetworkConnectionLost	= -1005,
		kCFErrorHTTPBadURL					= 305,
	};
	#endif
#endif

#define	CFIsDigit(_x)	(_x >= '0' && _x <= '9')

#define		loop(_n)					for (CFIndex _indexS = 0, _maxS = (_n); _indexS < _maxS; ++_indexS)
#define		loop2(_n)					for (CFIndex _index2S = 0, _max2S = (_n); _index2S < _max2S; ++_index2S)
#define		loop3(_n)					for (CFIndex _index3S = 0, _max3S = (_n); _index3S < _max3S; _index3S++)
#define		loop_reverse(_n)			for (CFIndex _indexS = ((_n) - 1); _indexS >= 0; --_indexS)
#define		loop_range(_begin, _end)	for (CFIndex _indexS = _begin, _maxS = (_end); _indexS < _maxS; ++_indexS)
#define		loop2_range(_begin, _end)	for (CFIndex _index2S = _begin, _max2S = (_end); _index2S < _max2S; ++_index2S)

#ifdef kDEBUG
	#define CF_STATIC_ASSERT(_test) typedef char static_assert_fail[( !!(_test) )*2-1 ]
#else
	#define CF_STATIC_ASSERT(_test)
#endif

#define		CF_ASSERT(_foo)	if (!(_foo)) {					\
	AssertAlert(#_foo, __FILE__, __LINE__, true);	}(0)	//	last part suppresses warning about extraneous semicolon

int		AssertAlert(const char *msgZ, const char *fileZ, long lineL, bool noThrowB = false);

#if _CFTEST_
	#ifdef _QTServer_
		#define		kJams_LogFileName		"QTServer Log File.txt"
	#else
		#define		kJams_LogFileName		"console.txt"
	#endif
#else
	#define		kJams_LogFileName		"kJams Log file.txt"
#endif

#define		kKiloByte				1024
#define		kKiloBytef				1024.0f
#define		kMegaByte				(kKiloByte * kKiloByte)
#define		kGigaByte				(kMegaByte * kKiloByte)
#define		kTeraByte				((UInt64)kGigaByte * (UInt64)kKiloByte)

#if	defined(kDEBUG) && !defined(_MIN_CF_)
	#define		TRACK_RETAINS	1
#else
	#define		TRACK_RETAINS	0
#endif

#if TRACK_RETAINS
	class	ScTrackRetains {
		void	*i_dataP;
		
		public:
		ScTrackRetains();
		~ScTrackRetains();
	};
	
	CFTypeRef	CFRetainDebug(CFTypeRef cf, bool do_itB = true);
	void		CFReleaseDebug(CFTypeRef cf);
#else
	class	ScTrackRetains {};

	inline CFTypeRef	CFRetainDebug(CFTypeRef cf, bool do_itB = true) {
		return do_itB ? ::CFRetain(cf) : cf;
	}

	#define	CFReleaseDebug(_x)		::CFRelease(_x)
#endif

#if OPT_KJMAC
	//#include "BasicTypes.h"
#else
	void	DebugReport(const char *utf8Z, OSStatus err);

	#if defined(_CFTEST_)
		#if defined(__WIN32__) || defined(_rose_spotlight_)

		#else
			#if defined(_CONSTRUCTOR_)
				#include <Carbon/Carbon.h>
//					#include <QuickDraw.h>
			#elif defined(_CFTEST_)
				#if !defined(_HELPERTOOL_)
					#include <Carbon/Carbon.h>
				#endif
			#elif !defined(_MIN_CF_)
				#include <QD/QuickDraw.h>
			#endif
		#endif
	#elif !_QT_
		#if OPT_WINOS
			#include "QuickDraw.h"
		#else
			#include <Carbon/Carbon.h>
		#endif
	#endif

	#if !defined(_CFTEST_) || (_QT_ && _KJAMS_ || _PaddleServer_)
		#if !defined(_CONSTRUCTOR_)
			#include "XErrors.h"
		#endif
	#endif

	#ifndef _H_XErrors
		#define ERR_(_ERR, FUNC)	if (!_ERR) { _ERR = (FUNC); }
		#define ERR(FUNC)			ERR_(err, FUNC);
		#define ERR_NULL(_ptr, _err)	ERR(_ptr == NULL ? (OSStatus)_err : (OSStatus)noErr)
		#define	ERRL(FUNC, _str)	if (!err) { ERR(FUNC); if (err) { 	LogErr("### Error " _str, err); }}
		#define	ETRL(_exp, _str)	{ ERRL(_exp, _str); if (err) { return err;} }

		#define BEX_THROW(ERR)		throw ((long) ERR)	
		#define ETX(EXPR)			{ OSStatus _err = (EXPR); if (_err) BEX_THROW(_err); }
		
		#define BEX_TRY				{{ OSStatus _err = noErr; try {
		#define XTE_TRY(ERR)				if (! ERR) BEX_TRY
		
		#define BEX_CATCH_PART1		}	catch (long _tmp_err) { _err = _tmp_err; } \
										catch(std::bad_alloc&) { _err = mFulErr; } \
										catch(std::exception&) { _err = hrMiscellaneousExceptionErr; }

		#define BEX_CATCH_PART2		if (_err) {

		#define BEX_CATCH			BEX_CATCH_PART1 BEX_CATCH_PART2

		#define XTE_CATCH			BEX_CATCH
		#define BEX_CLEANUP			BEX_CATCH_PART1 {
		#define XTE_CLEANUP			BEX_CLEANUP
		#define BEX_ENDTRY			} if (_err) BEX_THROW(_err); }}

		#define BEX_NO_PROPAGATE	_err = noErr
		#define XTE_NO_PROPAGATE			BEX_NO_PROPAGATE

		#define IX_ENDTRY(ERR)				ERR = _err; XTE_NO_PROPAGATE; BEX_ENDTRY

		#define IX_START	{ OSStatus xte_err_private = noErr; XTE_TRY(xte_err_private) { 
		#define IX_END		} XTE_CLEANUP { } IX_ENDTRY(xte_err_private); }

		#define XTE_START		XTE_TRY(err); { 
		#define XTE_END			} XTE_CATCH { ; } IX_ENDTRY(err)

		#define XTE(FUNC)		XTE_TRY(err) { (FUNC); } XTE_CATCH { ; } IX_ENDTRY(err)
		#define	IX(FUNC)		{	OSStatus err = noErr; XTE(FUNC);  }
	#endif

	#define	memclr(p, s)	std::memset(p, 0, s)
	#define	structclr(p)	memclr(&p, sizeof(p))

	#define noErr	0

	#if OPT_MACOS
		#ifndef COMPILING_ON_10_4
		#define	COMPILING_ON_10_4	(MAC_OS_X_VERSION_MAX_ALLOWED == MAC_OS_X_VERSION_10_4)	//	1	//	defined(__GNUC__)
		#endif
	#endif

#endif

#ifndef _USTRING_
	#define _USTRING_
	typedef std::basic_string<UTF8Char, std::char_traits<UTF8Char>, std::allocator<UTF8Char> > ustring;
#endif

#ifndef uc
	#define uc(_foo) (const UTF8Char *)(_foo)
#endif

typedef std::vector<int>			IntVec;
typedef std::vector<char>			CharVec;
typedef std::vector<UTF8Char>		UCharVec;
typedef std::deque<UTF8Char>		UCharQue;
typedef std::vector<SInt16>			SInt16Vec;
typedef std::vector<UTF16Char>		UTF16Vec;
typedef std::vector<char *>			CharPVec;
typedef std::vector<std::string>	StringVec;
typedef	std::vector<UInt32>			UInt32Vec;
typedef	std::vector<SInt32>			SInt32Vec;
typedef	std::vector<UInt64>			UInt64Vec;
typedef std::set<UInt32>			UInt32Set;

#define		kCFURLPathSeparator_Windows		"\\"
#define		kCFURLPathSeparatorC_Windows	'\\'

#define		kCFURLPathSeparator_POSIX		"/"
#define		kCFURLPathSeparatorC_POSIX		'/'

#if OPT_MACOS
	typedef OptionBits PMPrintDialogOptionFlags;
	#define		kCFURLPlatformPathStyle			kCFURLPOSIXPathStyle
	#define		kCFURLPlatformPathSeparator		kCFURLPathSeparator_POSIX
	#define		kCFURLPlatformPathSeparatorC	kCFURLPathSeparatorC_POSIX
#else
	#define		kCFURLPlatformPathStyle			kCFURLWindowsPathStyle
	#define		kCFURLPlatformPathSeparator		kCFURLPathSeparator_Windows
	#define		kCFURLPlatformPathSeparatorC	kCFURLPathSeparatorC_Windows
#endif

/*****************************/
//	protocols
#define kCFURLProtocolSeparator			":/"
#define kCFURLProtocolName_HTTP			"http"
#define kCFURLProtocol_HTTP				kCFURLProtocolName_HTTP		kCFURLProtocolSeparator
#define	kCFURLProtocol_HTTPS			kCFURLProtocolName_HTTP "s"	kCFURLProtocolSeparator
#define	kCFURLProtocol_RAM				"ram"	kCFURLProtocolSeparator
#define	kCFURLProtocol_FILE				"file"	kCFURLProtocolSeparator

#define	kURL_kJams						"/karaoke.kjams.com"
#define	kURL_kJams_SSL					kCFURLProtocol_HTTPS kURL_kJams
#define	kURL_kJamsWiki					kURL_kJams_SSL "/wiki/"
/*****************************/

#define	kXML_EncodingKey			"encoding"
#define	kXML_EncodingValue_UTF8		"UTF-8"

#define	kXML_EncodingKeyValUTF8		kXML_EncodingKey "=\"" kXML_EncodingValue_UTF8 "\""

/*****************************/
#define	kCFString_KeySeparator		"\\"

/*****************************/
extern const char*	kCFTimeZoneDictKey_Name;			//	ISO name
extern const char*	kCFTimeZoneDictKey_DisplayName;
extern const char*	kCFTimeZoneDictKey_LocalizedName;
extern const char*	kCFTimeZoneDictKey_LocalizedName_Dst;
extern const char*	kCFTimeZoneDictKey_Abbreviation;
extern const char*	kCFTimeZoneDictKey_Abbreviation_Dst;
extern const char*	kCFTimeZoneDictKey_ZoneOffset;
extern const char*	kCFTimeZoneDictKey_IsDst;
extern const char*	kCFTimeZoneDictKey_DstOffset;

/*****************************/
#define		ENTER_KEY			0x03
#define		LINEFEED_KEY		'\n'	//	0x0A
#define		RETURN_KEY			'\r'	//	0x0D
#define		IsEOLN(_x)			(_x == RETURN_KEY || _x == LINEFEED_KEY)

extern const char ENTER_STR[];
extern const char RETURN_STR[];
extern const char LINEFEED_STR[];

enum {
	OSType_NONE		= static_cast<OSType>(-1)
};

/*****************************/

typedef enum {
	CFRecoverType_NORMAL,
	CFRecoverType_EXCLUDE_SLASHES,
	CFRecoverType_EXCLUDE_SLASHES_DASHES,
	CFRecoverType_OLD
} CFRecoverType;

/***************************************************************************************/

void	CFLogArgs(int argc, const char *argv[]);
void	CFLogTime();

typedef enum {
	CFType_NONE,
	
	CFType_NULL,
	CFType_BOOL,
	CFType_NUMBER_INT,
	CFType_NUMBER_FLOAT,
	CFType_DATE,
	CFType_TIMEZONE,
	CFType_STRING,
	CFType_DICT,
	CFType_ARRAY,
	CFType_DATA,
	CFType_HTTP_MESSAGE,
	CFType_CFURL,
	
	CFType_UNKNOWN
} CFTypeEnum;

CFTypeEnum		CFGetCFType(CFTypeRef cfType);
CFTypeID		CFTypeGetID(CFTypeEnum typeEnum);
CFStringRef		CFTypeCopyDesc(CFTypeEnum typeEnum);
bool			CFIsDebug();
UInt32			CFTickCount();

CFIndex			S_LogCount(CFTypeRef typeRef, const char *utf8Z, bool forceB = false);

typedef	std::vector<bool>			BitVec;
typedef std::vector<CFTypeRef>		CFTypeRefVec;
typedef std::vector<CFStringRef>	StringRefVec;
typedef std::vector<CFAbsoluteTime>	CFAbsTimeVec;

void	CFReportUnitTest(const char *utf8Z, OSStatus err);

static	inline CFRange		CFStrGetRange(CFStringRef ref) {
	return CFRangeMake(0, CFStringGetLength(ref));
}

static	inline CFRange		CFArrayGetRange(CFArrayRef ref) {
	return CFRangeMake(0, CFArrayGetCount(ref));
}

OSErr	CFDictionaryCreate(CFMutableDictionaryRef *dictP);
OSErr	CFArrayCreate(CFMutableArrayRef *arrayP);

OSStatus		CFWriteDataToURL(const CFURLRef urlRef, CFDataRef dataRef);

bool			Read_PList(const CFURLRef &url, CFDictionaryRef *plistP);
OSStatus		Write_PList(
	CFPropertyListRef	plist,
	CFURLRef			urlRef);

double			CFNumberToDouble(const CFNumberRef &num);
void			CFWaitForKeyPress(CFStringRef msgRef);

bool			CFDateFormatterUses24HourTimeDiaplay();

CFStringRef		CFStringCreateWithDate(
	CFDateRef				dateRef, 
	CFDateFormatterStyle	dateFormat = kCFDateFormatterShortStyle, 
	CFDateFormatterStyle	timeFormat = kCFDateFormatterShortStyle);

void	CFSetDiacriticInsensitive(bool insensB);
bool	CFGetDiacriticInsensitive();

CFDateRef		CFDateCreateWithGregorian(const CFGregorianDate& gregDate);
CFGregorianDate	CFDateGetGregorian(CFDateRef dateRef);

CFStringRef		CFStringCreateWithNumber(CFNumberRef numRef);
CFNumberRef		CFNumberCreateWithSInt32(SInt32 valL);
CFNumberRef		CFNumberCreateWithUInt32(UInt32 valL);
CFNumberRef		CFNumberCreateWithUInt64(UInt64 valLL);

CFStringRef		CFCopyBundleResourcesFSPath();
void			CFLogSetLogPath();
long			CFNumberToLong(const CFNumberRef &num);

CFDateRef 		CFDateCreateWithLongDateTime(const LongDateTime &ldt);
LongDateTime	CFDateToLongDateTime(CFDateRef dateRef);

bool			CFDebuggerAttached();
void			CFDebugBreak();
CFStringRef		CFBundleCopyLocalizationPath();

CFStringRef			CFCopyLocaleLangKeyCode();
bool				CFLocaleIsEnglish();
CFLocaleRef			CFLocaleCopyCurrent_Mutex();

//	pass (0, false) if you need to test for DST
//	passing true gives you an absolute timezone with no DST (but it works on windows)
CFTimeZoneRef		CFTimeZoneCopyCurrent_Mutex(CFAbsoluteTime absT = 
	#if OPT_WINOS
		CFAbsoluteTimeGetCurrent(),
	#else
		0,
	#endif
	bool hackB = true);
	
void				CFLocaleResetSystem();

UInt32		CFSwapInt24HostToBig(UInt32 valL);
UInt32		CFSwapInt24BigToHost(UInt32 valL);

bool			CFAbsoluteTimeExpired(const CFAbsoluteTime &expireDate);
bool			CFGregorianDateExpired(const CFGregorianDate &expireDate);

//	gets current year in current time zone
UInt16				CFTimeZoneGetCurrentYear();
CFTimeZoneRef		CFTimeZoneCopyGMT();
CFDictionaryRef		CFTimeZoneCopyDict(CFTimeZoneRef tz);

CFGregorianDate		CFAbsoluteTimeConvertToGregorian(const CFAbsoluteTime& cfTime);
CFAbsoluteTime		CFAbsoluteTimeCreateFromGregorian(const CFGregorianDate &greg, bool gmtB = false);
CFAbsoluteTime		CFAbsoluteTimeCreateExpiryFromMonthYear(long monthL, long yearL);

void	CFAbsoluteTime_RoundDown_Day(CFAbsoluteTime *t);
void	CFAbsoluteTime_RoundUp_Day(CFAbsoluteTime *t);

CFGregorianUnits	CFTimeIntervalGetAsGregorianUnits(CFTimeInterval intervalT);
CFStringRef			CFCalendarCopyUnitString(CFCalendarUnit unit, bool pluralB);
CFStringRef			CFTimeIntervalCopyString(CFTimeInterval intervalT);

#define		CFSwapInt32BigToHost_InPlace(_foo)	_foo = CFSwapInt32BigToHost(_foo)
#define		CFSwapInt24BigToHost_InPlace(_foo)	_foo = CFSwapInt24BigToHost(_foo)
#define		CFSwapInt16BigToHost_InPlace(_foo)	_foo = CFSwapInt16BigToHost(_foo)

#define		CFSwapInt32HostToBig_InPlace(_foo)	_foo = CFSwapInt32HostToBig(_foo)
#define		CFSwapInt24HostToBig_InPlace(_foo)	_foo = CFSwapInt24HostToBig(_foo)
#define		CFSwapInt16HostToBig_InPlace(_foo)	_foo = CFSwapInt16HostToBig(_foo)

/***************************************************************************************/
//	stuff from StringUtils

CFStringEncoding	LanguageRegionToEncoding(const SuperString& langRgn);
CFStringEncoding	CFLangRgnStrToEncoding(const SuperString& localeIDStr);

char			*CopyCFStringToC(
	CFStringRef			str, 
	UInt32				maxL, 
	char				*strZ, 
	CFStringEncoding	encoding = kCFStringEncodingInvalidId);

#if defined(_KJAMS_) && !_QT_
	#define	_HAS_HANDLES_
#endif

#ifdef _HAS_HANDLES_
	CFDataRef		CFDataCreateWithHandle(Handle theH);
	void			Dict_Set_Handle(CFMutableDictionaryRef dict, const char *keyZ, Handle theH);
	Handle			Dict_Copy_Handle(CFDictionaryRef dict, const char *keyZ);
#endif

CFDataRef		Dict_Get_Data(CFDictionaryRef dict, const char *keyZ);
void			Dict_Set_Data(CFMutableDictionaryRef dict, const char *keyZ, CFDataRef dataRef);

CFStringRef		CFStrCreateRecoverableName(CFStringRef basisRef, CFRecoverType recoverType = CFRecoverType_NORMAL);
CFStringRef		CFStrRecoverName(CFStringRef basisRef);

long			CStringToNum(const char *num);
const char		*NumToCString(long numL, std::string &str);
long			CFStringToLong(const CFStringRef &num);
float			CFNumberToFloat(const CFNumberRef &num);

CFComparisonResult	CFDateCompare(CFDateRef date1, CFDateRef date2);
bool			CFDateEqual(CFDateRef str1, CFDateRef str2);
bool			CFDateLess(CFDateRef lhs, CFDateRef rhs);

bool			CFDictIsEmpty(CFDictionaryRef nameRef);

const char *	CopyLongToC(long valL);

CFStringRef		CFStringCreateWithNumber(long numberL);

CFStringRef		CFStringCreateWithStd(const std::string &stdStr);
CFNumberRef		CFNumberCreateWithNumber(long numberL);
CFNumberRef		CFNumberCreateWithFloat(float numberF);
CFNumberRef		CFNumberCreateWithDouble(double numberF);

std::string		&UpperString(std::string &str);
std::string		&LowerString(std::string &str);

ustring			&UpperString(ustring &str);
ustring			&LowerString(ustring &str);

void			FixTrailingPeriod(bool recoverB, CFMutableStringRef stringRef);

void			Dict_Set_Str(CFMutableDictionaryRef dict, const char *keyZ, CFStringRef str);
void			Dict_Set_Str(CFMutableDictionaryRef dict, const char *keyZ, const char *strZ);
CFStringRef		Dict_Get_Str(CFDictionaryRef dict, const char *keyZ);
bool			Dict_Get_Str(CFDictionaryRef dict, const char *keyZ, std::string &str);
bool			Dict_Get_Str(CFDictionaryRef dict, const char *keyZ, ustring &str);
bool			Dict_Get_Str(CFDictionaryRef dict, const char *keyZ, SuperString *strP);

void	Dict_Set_OSType(CFMutableDictionaryRef dict, const char *keyZ, OSType osType);
OSType	Dict_Get_OSType(CFDictionaryRef dict, const char *keyZ, OSType defaultType = FourCharQuestionMarks);	//	kFileType_WILDCARD);

void	Dict_Set_Bool(CFMutableDictionaryRef dict, const char *keyZ, bool valB);
bool	Dict_Get_Bool(CFDictionaryRef dict, const char *keyZ, bool defaultB = false);

void	Dict_Set_Float(CFMutableDictionaryRef dict, const char *keyZ, float valF);
float	Dict_Get_Float(CFDictionaryRef dict, const char *keyZ, float defaultF = 0);

void	Dict_Set_Long(CFMutableDictionaryRef dict, CFStringRef keyRef, long valL);
void	Dict_Set_Long(CFMutableDictionaryRef dict, const char *keyZ, long valL);
long	Dict_Get_Long(CFDictionaryRef dict, const char *keyZ, long defaultL = -1);

void					Dict_Set_Dict(CFMutableDictionaryRef dict, const char *keyZ, CFDictionaryRef subDict);
CFDictionaryRef			Dict_Get_Dict(CFDictionaryRef dict, const char *keyZ);
CFMutableDictionaryRef	Dict_Copy_Dict(CFDictionaryRef dict, const char *keyZ);

void			Dict_Set_Array(CFMutableDictionaryRef dict, const char *keyZ, CFArrayRef subArray);
CFArrayRef		Dict_Get_Array(CFDictionaryRef dict, const char *keyZ);

SuperString		Array_GetIndStr(CFArrayRef array, long indexL);
CFIndex			Array_GetLongIndex(CFArrayRef array, long valL);
long			Array_GetIndLong(CFArrayRef dict, CFIndex indexL);
void			Array_Add_Long_AtIndex(CFMutableArrayRef arrayRef, CFIndex atIndex, UInt32 valL);
void			Array_Add_Long(CFMutableArrayRef arrayRef, UInt32 valL);
void			Array_SetIndLong(CFMutableArrayRef dict, CFIndex indexL, long valueL);

typedef enum {
	kTimeCode_NORMAL, 
	kTimeCode_PRETTY, 
	kTimeCode_MEDIUM,
	kTimeCode_LONG,
	kTimeCode_LONG_FLOAT
} kTimeCodeType;

CFTimeInterval	CFDateDifference(CFDateRef newDateRef, CFDateRef oldDateRef);
CFDateRef		CFDateCreateCurrent();

#if _QT_ && !_JUST_CFTEST_
QDateTime	CFDateGetQDateTime(CFDateRef date);
CFDateRef	CFDateCreateFromQDateTime(const QDateTime& dt);
#endif

void			Dict_Set_Date(CFMutableDictionaryRef dict, const char *keyZ, CFDateRef dateRef);
CFDateRef		Dict_Copy_Date(CFDictionaryRef dict, const char *keyZ);

void	Dict_Set_Rect(CFMutableDictionaryRef dict, const char *keyZ, const Rect &valR);
Rect	Dict_Get_Rect(CFDictionaryRef dict, const char *keyZ);

RGBColor	Dict_Get_Color(CFDictionaryRef dict, const char *keyZ);
void		Dict_Set_Color(CFMutableDictionaryRef dict, const char *keyZ, const RGBColor &valR);

void		Array_Append_ColorSpec(CFMutableArrayRef array, const ColorSpec &cspec);
ColorSpec	Array_GetInd_ColorSpec(CFArrayRef array, CFIndex indexL);

void		SplitOffExtension(std::string &str, std::string *extenstionP0 = NULL);

ustring			Hexify(const UCharVec& vec, bool to_upperB = false);
ustring			Hexify(const UInt8* bufA, size_t sizeL, bool to_upperB = false);
UCharVec		UnHexify(const ustring& str);

std::string		ULong_To_Hex(UInt32 valueL);
UInt32			Hex_To_ULong(const char *hexZ);

std::string		ULLong_To_Hex(UInt64 valueL);
UInt64			Hex_To_ULLong(const char *hexZ);

/***************************************************************************************/

void	CFSetLogDuringStartup(bool logB);
bool	CFGetLogDuringStartup();

void	CFSetLogging(bool logB);

class CCFLog {
	bool	i_crB;
	static	CFStringRef		i_logPathRef;
	
	public:
	CCFLog(bool crB = false) : i_crB(crB) { }
	
	static	void		SetLogPath(CFStringRef logPathRef);
	static	CFStringRef	GetLogPath()	{ return i_logPathRef; }
	static	void		close();
	static	void		trim();

	void operator()(CFTypeRef valRef);
	//inline void operator()(void const* valRef)	{	operator()((CFTypeRef)valRef);	}
	void operator()(CFStringRef keyRef, CFTypeRef valRef);
};

template <typename T>
class	ScCFTypeRef {
	protected:
	T	i_typeRef;
	
	public:
	ScCFTypeRef(const ScCFTypeRef& other) : i_typeRef(NULL) {
		SetAndRetain(other.Get());
	}
	
	ScCFTypeRef(T typeRef = NULL, bool retainB = false) : i_typeRef(typeRef)	{
		if (retainB) {
			Retain();
		}
	}
	
	virtual ~ScCFTypeRef() {
		Release();
	}
	
	CFIndex RetainCount()	{
		CFIndex		countL = 0;
		
		if (i_typeRef) {
			 countL = CFGetRetainCount(i_typeRef);
		}
		
		return countL;
	}
	
	CFTypeID			GetTypeID()		{	return CFGetTypeID(i_typeRef);	}
	virtual CFTypeEnum	GetTypeEnum()	{	return CFGetCFType(i_typeRef);	}
	
	T	Retain()	{	if (i_typeRef) CFRetainDebug(i_typeRef);	return i_typeRef;	}
	T	Release()	{
		CFIndex		countL = RetainCount();
		
		if (i_typeRef) {
			CFReleaseDebug(i_typeRef);
		}

		if (countL == 1) {
			i_typeRef = NULL;
		}
		
		return i_typeRef;
	}
	
	T	Get() const		{	return i_typeRef;	}
	T	ref() const		{	return i_typeRef;	}
	T	SetAndRetain(T typeRef)	{
		
		if (typeRef) {
			CFRetainDebug(typeRef);
		}
		
		Release();
		
		i_typeRef = typeRef;
		return i_typeRef;
	}
	
	void	clear()	{	SetAndRetain(NULL);	}
	
	T*	AddressOf()	{	return &i_typeRef;	}
	
	operator T()			{	return i_typeRef;	}
	operator T*()			{	return AddressOf();	}
	//operator bool()			{	return i_typeRef != NULL;	}
	
	//T	operator =(T typeRef)	{	return Set(typeRef);	}
	
	bool	operator==(T other) {
		return CFEqual(Get(), other);
	}
	
	bool	operator!=(T other) {
		return !operator==(other);
	}
	
	bool	operator==(ScCFTypeRef& other) {
		return operator==(other.ref());
	}

	bool	operator!=(ScCFTypeRef& other) {
		return !operator==(other.ref());
	}

	T	transfer()	{
		T	ret = i_typeRef;
		
		i_typeRef = NULL;
		return ret;
	}
	
	T	adopt(T typeRef)	{
		clear();
		i_typeRef = typeRef;
		return i_typeRef;
	}
	
	void	LogCount(const char *nameZ) {	S_LogCount(i_typeRef, nameZ);	}
	
	void	Log()	{
		if (i_typeRef) {
			CCFLog(true)(i_typeRef);
		} else {
			CCFLog(true)(CFSTR("-- i_typeRef is NULL -- "));
		}
	}
};

template <typename T>
class	ScCFReleaser : public ScCFTypeRef<T> {
	typedef ScCFTypeRef<T> _inherited;
	public:
	
	ScCFReleaser(T typeRef = NULL, bool retainB = false) : _inherited(typeRef, retainB)	{ }
	operator CFTypeRef()	{	return _inherited::Get();	}
};

typedef ScCFReleaser<CFURLRef>				CCFURL;
typedef ScCFReleaser<CFTypeRef>				CCFType;
typedef ScCFReleaser<CFDateRef>				CCFDate;
typedef ScCFReleaser<CFErrorRef>			CCFError;
typedef ScCFReleaser<CFLocaleRef>			CCFLocale;
typedef ScCFReleaser<CFStringRef>			CCFString;
typedef ScCFReleaser<CFTimeZoneRef>			CCFTimeZone;
typedef ScCFReleaser<CFHTTPMessageRef>		CCFHTTPMessage;
typedef ScCFReleaser<CFDateFormatterRef>	CCFDateFormatter;

/***************************************************************************************/
class	CDictionaryIterator {
	CFDictionaryRef		i_dict;
	
	static	void 	CB_S_Operator(CFTypeRef key, CFTypeRef value, void *context) {
		CDictionaryIterator		*thiz = (CDictionaryIterator *)context;
		
		thiz->operator()((CFStringRef)key, value);
	}
	
	public:
	CDictionaryIterator(CFDictionaryRef dict) : i_dict(dict) { }
	virtual ~CDictionaryIterator() {}
	
	void	for_each() {
		CFDictionaryApplyFunction(i_dict, CB_S_Operator, this);
	}
	
	virtual void	operator()(CFStringRef key, CFTypeRef value) {
		UNREFERENCED_PARAMETER(key);
		UNREFERENCED_PARAMETER(value);		
	}
};

template <class Function>
class CDict_ForEach : public CDictionaryIterator {
	Function		i_f;
	
	public:
	CDict_ForEach(CFDictionaryRef dict, Function f) : CDictionaryIterator(dict), i_f(f) { }
	virtual ~CDict_ForEach() {}
	
	void	operator()(CFStringRef keyRef, CFTypeRef valRef) {
		i_f(keyRef, valRef);
	}
};

template <class Function>
inline	void	dict_for_each(CFDictionaryRef dict, Function f)
{
	if (dict) {
		CDict_ForEach<Function>		cdict(dict, f);
		
		cdict.for_each();
	}
}

#define		CFArrayApplyFunctionToAll(_array, _cb, _data) \
CFArrayApplyFunction((CFArrayRef)_array, CFArrayGetRange((CFArrayRef)_array), _cb, _data)

class	CArrayIterator {
	CFArrayRef		i_array;
	
	static	void 	CB_S_Operator(CFTypeRef value, void *context) {
		CArrayIterator		*thiz = (CArrayIterator *)context;
		
		thiz->operator()(value);
	}
	
public:
	CArrayIterator(CFArrayRef array) : i_array(array) { }
	virtual ~CArrayIterator() {}
	
	void	for_each() {
		CFArrayApplyFunctionToAll(i_array, CB_S_Operator, this);
	}
	
	virtual void	operator()(CFTypeRef value) {
		UNREFERENCED_PARAMETER(value);
	}
};

template <class Function>
class CArray_ForEach : public CArrayIterator {
	Function		i_f;
	
	public:
	CArray_ForEach(CFArrayRef dict, Function f) : CArrayIterator(dict), i_f(f) { }
	virtual ~CArray_ForEach() {}
	
	void	operator()(CFTypeRef valRef) {
		i_f(valRef);
	}
};

template <class Function>
inline	void	array_for_each(CFArrayRef array, Function f)
{
	if (array) {
		CArray_ForEach<Function>		carray(array, f);
		
		carray.for_each();
	}
}

#define		DICT_STR_RECT_LEFT		"Left"
#define		DICT_STR_RECT_RIGHT		"Right"
#define		DICT_STR_RECT_TOP		"Top"
#define		DICT_STR_RECT_BOTTOM	"Bottom"
#define		DICT_STR_RECT_WIDTH		"Width"
#define		DICT_STR_RECT_HEIGHT	"Height"

#define		DICT_STR_COLOR_VALUE	"Value"
#define		DICT_STR_COLOR_RED		"Red"
#define		DICT_STR_COLOR_GREEN	"Green"
#define		DICT_STR_COLOR_BLUE		"Blue"

typedef enum {
	kJSON_DateFormat_NONE,
	kJSON_DateFormat_JSON,				//	JS.toJSON
	kJSON_DateFormat_DOT_NET,			//	DataContractJsonSerializer
	kJSON_DateFormat_DOT_NET_STRIPPED	//	milliseconds since kCFAbsoluteTimeIntervalSince1970
} JSON_DateFormatType;

#define		kJSON_Compact	-1 
#define		kJSON_Indented	0 

class	CPlistFormat {
	public:
	bool					i_jsonB;
	int						i_indentI;
	JSON_DateFormatType		i_dateFormatType;

	CPlistFormat(
		bool					jsonB			= false,
		int						indentI			= kJSON_Compact,
		JSON_DateFormatType		dateFormatType	= kJSON_DateFormat_JSON)
	:
		i_jsonB(jsonB), 
		i_indentI(indentI),
		i_dateFormatType(dateFormatType)
	{ }
};


/*****************************************************/
class	CFParseProgress {
	public:
	CFParseProgress() {}
	virtual ~CFParseProgress() {}
	
	virtual void	operator()() {}
};

typedef enum {
	SS_Time_NONE	= -1,
	SS_Time_SHORT, 					//	Thu, 23 Jul 2009 15:15:21 GMT
	SS_Time_LONG, 					//	August 29, 2008 15:36:59 PM PDT
	SS_Time_LONG_12,				//	August 29, 2008 03:36:59 PM PDT
	SS_Time_LONG_PRETTY,			//	Thursday, March 29, 2012 - 3:31 PM PDT
	SS_Time_LOG,					//	2009-01-24 18:20:16 or 2009-11-24 20:00:47.586 -0800, or SS_Time_JSON
	SS_Time_SHORT_12,				//	15:23
	SS_Time_SHORT_24,				//	3:23 PM
	SS_Time_SHORT_J,				//	one of the two above depending on locale
	SS_Time_COMPACT_DATE_TZ,		//	5/13/2009 PT
	SS_Time_COMPACT_DATE_ONLY,		//	5/13/2009
	SS_Time_COMPACT_DATE_REVERSE,	//	2012/09/01
	SS_Time_NAKED,					//	float, double, or int, 
	SS_Time_DOT_NET,				//	/Date(1494646923590+0000)/, DataContractJsonSerializer
	SS_Time_TIMESTAMP_HELSINKI,		//	"20120401182543234"	helsinki time
	SS_Time_LOG_HELSINKI, 			//	"2009-01-24 18:20:16 -0300" helsinki time (hacked)
	SS_Time_SYSTEM_LONG, 			//	something like SS_Time_LONG_PRETTY

	//	http://stackoverflow.com/questions/10286204/the-right-json-date-format
	SS_Time_JSON,					//	"2009-11-24T18:20:16.4313Z"

	SS_Time_SIZE_begin,
	SS_Time_SIZE_4 = SS_Time_SIZE_begin,	//	"Wednesday, February 21, 2018 6:39 AM"
	SS_Time_SIZE_3,							//	"February 21, 2018 6:39 AM"
	SS_Time_SIZE_2,							//	"Feb 21, 2018 6:39 AM"
	SS_Time_SIZE_1,							//	"2/21/18 6:39 AM"
	SS_Time_SIZE_0,							//	"2/21/18"
	SS_Time_SIZE_end,

	SS_Time_NUMTYPES = SS_Time_SIZE_end
} SS_TimeType;

/*****************************************************/
bool			CFType_ContainsKey(
	CFTypeRef		thisRef,
	SuperString		curKey);

CFTypeRef		CFType_GetValFromPathKey(
	CFTypeRef		thisRef, 
	SuperString		curKey);

/*****************************************************
 usage: (same for CCFArray)
 CCFDictionary		dict;							//	allocate a dictionary
 CCFDictionary		dict(NULL);						//	allocate a dictionary
 CCFDictionary		dict(NULL, false);				//	allocate a dictionary
 
 CCFDictionary		dict(existingDict);				//	xfer ownership, for use with "Copy" calls (WILL be released in d'tor)
 CCFDictionary		dict(existingDict, false);		//	xfer ownership, for use with "Copy" calls (WILL be released in d'tor)
 CCFDictionary		dict(existingDict, true);		//	retain dictionary, for use with "Get" calls (will NOT be released, caller responsible for releasing)
 
 CCFDictionary		dict(NULL, true);				//	NULL dictionary, ONLY use if next instruction is .[Immutable]AddressOf(), use with legacy C APIs
 */

class CCFDictionary : public ScCFReleaser<CFMutableDictionaryRef> {
	typedef	ScCFReleaser<CFMutableDictionaryRef>	_inherited;

	void	validate() {
		if (Get() == NULL) {
			realloc();
		}
	}

	public:
	CCFDictionary(
		CFDictionaryRef		dictRef0	= NULL, 
		bool				retainB		= false
	) : 
		_inherited((CFMutableDictionaryRef)dictRef0, retainB) 
	{
		if (dictRef0 == NULL && retainB == false) {
			validate();
		}
	}
	
	CCFDictionary(const SuperString& str, bool jsonB = false);	//	false means XML
	
	CFDictionaryRef*	ImmutableAddressOf()	{	return (CFDictionaryRef *)AddressOf();	}
	operator CFDictionaryRef() const			{	return Get();	}

	void	realloc() {
		CFMutableDictionaryRef	dictRef = CFDictionaryCreateMutable(
			kCFAllocatorDefault, 0,
			&kCFTypeDictionaryKeyCallBacks,
			&kCFTypeDictionaryValueCallBacks);

		if (dictRef == NULL) {
			ETX(1);
		}

		_inherited::adopt(dictRef);
	};
	
	CFDictionaryRef				adopt(CFDictionaryRef typeRef) {
		return (CFDictionaryRef)_inherited::adopt((CFMutableDictionaryRef)typeRef);
	}

	CFMutableDictionaryRef		Copy();
	void						CopyTo(CFMutableDictionaryRef destDictRef);

	virtual bool				ContainsKey(CFStringRef keyRef);
	bool						ContainsKey(const char *utf8Z);
	
	bool						empty()	{	return size() == 0;		}
	CFIndex						size() {
		CFIndex		sizeL(0);
		
		if (Get()) {
			sizeL = CFDictionaryGetCount(Get());
		}
		
		return sizeL;
	}

	/*********************************************/
	virtual CFTypeID		GetType(CFStringRef key)
	{
		CFTypeID		typeID(CFNullGetTypeID());
		CFTypeRef		valRef(GetValue(key));

		if (valRef) {
			typeID = CFGetTypeID(valRef);
		}

		return typeID;
	}

	virtual CFTypeEnum		GetTypeEnum()	{	return _inherited::GetTypeEnum();	}
	virtual CFTypeEnum		GetTypeEnum(CFStringRef key)
	{
		CFTypeEnum		typeEnum(CFType_NULL);
		CFTypeRef		valRef(GetValue(key));

		if (valRef) {
			typeEnum = CFGetCFType(valRef);
		}

		return typeEnum;
	}

	virtual CFTypeRef	GetValue(CFStringRef key) {
		CFTypeRef		typeRef = NULL;
		
		if (ContainsKey(key)) {
			typeRef = CFDictionaryGetValue(Get(), key);
		}
		
		return typeRef;
	}

	CFTypeRef			GetValue(const char* keyZ);

	virtual CFStringRef	GetAs_String	(CFStringRef key);
	virtual CFStringRef	GetAs_String	(const char *utf8Z);
	virtual	CFStringRef	GetAs_String	(OSType key);

	SuperString			GetAs_SString	(const char *utf8Z);
	SuperString			GetAs_SString	(OSType key);

	SInt32				GetAs_SInt32	(const char *utf8Z);
	SInt32				GetAs_SInt32	(OSType key);
	UInt32				GetAs_UInt32	(const char *utf8Z)		{ return (UInt32)GetAs_SInt32(utf8Z);	}

	Rect				GetAs_Rect		(const char *utf8Z);
	bool				GetAs_Bool		(const char *utf8Z, bool defaultB = false);
	float				GetAs_Float		(const char *utf8Z);
	double				GetAs_Double	(const char *utf8Z);
	UInt64				GetAs_UInt64	(const char *utf8Z);
	OSType				GetAs_OSType	(const char *utf8Z);

	virtual SInt16		GetAs_SInt16	(const char *utf8Z, SInt16 defaultS = 0);
	SInt16				GetAs_SInt16	(OSType key);
	UInt16				GetAs_UInt16	(const char *utf8Z)		{	return (UInt16)GetAs_SInt16(utf8Z);	}

	CFDictionaryRef			GetAs_Dict			(CFStringRef key);
	CFDictionaryRef			GetAs_Dict			(const char *utf8Z);
	CFMutableDictionaryRef	GetAs_MutableDict	(const char *utf8Z);

	CFArrayRef			GetAs_Array			(CFStringRef keyStr);
	CFArrayRef			GetAs_Array			(const char *utf8Z);
	CFMutableArrayRef	GetAs_MutableArray	(const char *utf8Z) { return (CFMutableArrayRef)GetAs_Array(utf8Z); }

	CFDataRef			GetAs_Data		(const char *utf8Z);
	CFDateRef			GetAs_Date		(const char *utf8Z);
	CFAbsoluteTime		GetAs_AbsTime	(CFStringRef keyStr);
	CFAbsoluteTime		GetAs_AbsTime	(const char *utf8Z);
	CFGregorianDate		GetAs_GregDate	(const char *utf8Z);
	RGBColor			GetAs_Color		(const char *utf8Z);
	Ptr					GetAs_Ptr		(const char *utf8Z);
	
	template <typename T>
	T					GetAs_PtrT(const char *utf8Z)
	{
		return reinterpret_cast<T>(GetAs_Ptr(utf8Z));
	}
	
	CFAbsoluteTime		GetAs_TimeFromString(const char *utf8Z, SS_TimeType timeType = SS_Time_LOG, CFTimeInterval epochT = 0);

	/*********************************************/
	void				RemoveValue		(CFStringRef key);
	void				RemoveValue		(const char *utf8Z);
	void				RemoveValue		(OSType osType);
	
	virtual void		SetRealValue(CFTypeRef key, CFTypeRef val);

	inline void		SetValue(CFTypeRef key, CFTypeRef val) {
		if (key && val) {
			SetRealValue(key, val);
		}
	}

	void				SetValue_Ref(CFStringRef ref, SInt32 value);

	void				SetValue(OSType osType, const SuperString& value);
	void				SetValue(OSType osType, CFTypeRef val);
	void				SetValue(const char *utf8Z, CFTypeRef val);

	void				SetValue(const char *utf8Z, const CFGregorianDate& val);
	void				SetValue(const char *utf8Z, const Rect& frameR);
	void				SetValue(const char *utf8Z, const char *utf8ValZ);
	void				SetValue(const char *utf8Z, bool value);
	void				SetValue_OSType(const char *utf8Z, OSType value);
	void				SetValue(const char *utf8Z, SInt32 value);
	
	#if _QT_ && !_CFTEST_ && !OPT_WINOS
	void				SetValue(const char *utf8Z, long value);
	void				SetValue(const char *utf8Z, unsigned long value);
	void				SetValue(OSType key, long value);
	#endif
	
	void				SetValue(const char *utf8Z, UInt32 value);
	void				SetValue(const char *utf8Z, UInt64 value);
	void				SetValue(const char *utf8Z, SInt16 value);
	void				SetValue(OSType key, SInt16 value);
	void				SetValue(OSType key, SInt32 value);
	void				SetValue(OSType key, UInt32 value);
	void				SetValue(const char *utf8Z, float valueF);
	void				SetValue(const char *utf8Z, double valueF);
	void				SetValue_TimeToString(const char *utf8Z, CFAbsoluteTime valueT, SS_TimeType timeType = SS_Time_LOG, CFTimeInterval epochT = 0);
	void				SetValue_AbsTime(CFStringRef keyStr, CFAbsoluteTime valueT);
	void				SetValue_AbsTime(const char *utf8Z, CFAbsoluteTime valueT);
	void				SetValue(const char *utf8Z, const SuperString& value);
	void				SetValue(const char *utf8Z, const RGBColor& value);
	void				SetValue(const char *utf8Z, Ptr valueP);

/*	void				SetValue(OSType osType, const SuperString& value) {
		SetValue(osType, value.ref());
	}
*/
	void				TransferValue(const char *utf8Z, CCFDictionary& other) {
		SetValue(utf8Z, other.GetValue(utf8Z));
	}

	void				TransferValue(OSType osType, CCFDictionary& other);

	/*********************************************/
	//	for an example of a recursive for_each(), see CHasSingerValue
	template <class Function>
	inline	void	for_each(Function f) {
		dict_for_each(Get(), f);
	}
	
	SuperString		GetXML() const;
	void			SetXML(const SuperString& xml);

	SuperString		GetJSON(int indentI = kJSON_Compact, JSON_DateFormatType dateFormat = kJSON_DateFormat_DOT_NET) const;
	void			SetJSON(const SuperString& json, CFParseProgress *progP0 = NULL);

	void			SetJSON_p(SuperString& json, CFParseProgress *progP0 = NULL);
};

class CCFArray : public ScCFReleaser<CFMutableArrayRef> {
	typedef	ScCFReleaser<CFMutableArrayRef>	_inherited;

	void	validate() {
		if (Get() == NULL) {
			realloc();
		}
	}

	public:
	CCFArray(CFArrayRef arrayRef0 = NULL, bool retainB = false) : 
		_inherited((CFMutableArrayRef)arrayRef0, retainB)
	{ if (arrayRef0 == NULL && retainB == false) {validate();} }
	
	void	realloc() {
		CFMutableArrayRef	arrayRef = CFArrayCreateMutable(
			kCFAllocatorDefault, 0,
			&kCFTypeArrayCallBacks);

		if (arrayRef == NULL) {
			ETX(1);	//	errCppbad_alloc);
		}

		_inherited::adopt(arrayRef);
	};
	
	CFArrayRef*		ImmutableAddressOf()	{	return (CFArrayRef *)AddressOf();	}

	bool				empty()	{	return size() == 0;		}
	CFIndex				size() {
		CFIndex		sizeL = CFArrayGetCount(Get());
		return sizeL;
	}

	bool				IndexExists(CFIndex idx) {
		return idx < size();
	}

	CFIndex				ValidateIndex(CFIndex idx) {
		if (idx == kCFIndexEnd) {
			idx = size() - 1;

			if (idx == -1) {
				ETX(invalidIndexErr);
			}
		}

		return idx;
	}

	CFTypeRef			operator[](CFIndex idx) {
		CFTypeRef		cfType = NULL;

		idx = ValidateIndex(idx);
		if (idx >= 0 && idx < size()) {
			cfType = CFArrayGetValueAtIndex(Get(), idx);
		}

		return cfType;
	}
	
	UInt32				GetIndValAs_UInt32(CFIndex idx) 
	{
		CFNumberRef			numberRef((CFNumberRef)operator[](idx));
		SInt32				valL = 0;

		if (numberRef) {
			CFNumberGetValue(numberRef, kCFNumberSInt32Type, &valL);
		}

		return valL;
	}
	
	CFTypeRef			GetIndValAs_CFType(CFIndex idx);

	CFDictionaryRef		GetIndValAs_Dict(CFIndex idx);
	CFDictionaryRef		GetIndValAs_Dict(const char *path_utf8Z);	//	first elem of path MUST be numeric

	CFArrayRef			GetIndValAs_Array(CFIndex idx);
	CFArrayRef			GetIndValAs_Array(const char *path_utf8Z);	//	first elem of path MUST be numeric
	
	CFStringRef			GetIndValAs_Str(CFIndex idx) 
	{
		return (CFStringRef)operator[](idx);
	}
	
	CFTypeEnum			GetIndItemType(CFIndex idx) {
		CFTypeRef		typeRef(operator[](idx));

		return CFGetCFType(typeRef);
	}
	
	CFIndex				GetFirstIndexOfValue(CFTypeRef valRef) {
		return CFArrayGetFirstIndexOfValue(Get(), CFArrayGetRange(Get()), valRef);
	}
	
	//	OSType is UInt32
	CFIndex				GetFirstIndOf_ValAs_UInt32(UInt32 valL) {
		ScCFReleaser<CFNumberRef>	valTypeRef(
			CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &valL));
											   
		return GetFirstIndexOfValue(valTypeRef.Get());
	}
	
	void				push_back(CFTypeRef val) {
		CFArrayAppendValue(Get(), val);
	}
	
	void				push_back(const SuperString& valStr);
	
	void				append(CFArrayRef arrayRef) {
        CFArrayAppendArray(
			Get(), arrayRef,
			CFArrayGetRange(arrayRef));
	}

	void				push_back(UInt32 val) {
		ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithSInt32(val));

		push_back(numberRef);
	}

	void				insert(CFIndex idx, CFTypeRef val) {
		if (idx == kCFIndexEnd) {
			CFArrayAppendValue(Get(), val);
		} else {
			CFArrayInsertValueAtIndex(Get(), idx, val);
		}
	}

	void				erase(CFIndex idx) {
		CFArrayRemoveValueAtIndex(Get(), idx);
	}

	void				insert(CFIndex idx, UInt32 val) {
		ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithUInt32(val));
		
		insert(idx, numberRef.Get());
	}

	void				set_ind_value(CFIndex idx, CFTypeRef val) {
		CFArraySetValueAtIndex(Get(), ValidateIndex(idx), val);
	}

	void				set_ind_value(CFIndex idx, UInt32 val) {
		ScCFReleaser<CFNumberRef>	numberRef(CFNumberCreateWithUInt32(val));
		
		set_ind_value(idx, numberRef);
	}
	
	CFMutableArrayRef	Copy() {
		return CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, Get());
	}

	/*********************************************/
	//	for an example of a recursive for_each(), see CHasSingerValue
	template <class Function>
	inline	void	for_each(Function f) {
		array_for_each(Get(), f);
	}
	
	SuperString		GetJSON(int indentI = kJSON_Compact, JSON_DateFormatType dateFormat = kJSON_DateFormat_DOT_NET);
	void			SetJSON_p(SuperString& json, CFParseProgress *progP0 = NULL);
};
/*****************************************/

enum {
    kCFXMLParserAttemptConvertMacRomanToUTF8 = (1UL << 6)
};
//	CFXMLParserOptions

class CCFXMLNode {	//	does NOT retain OR release
	CFXMLNodeRef		i_nodeRef;	//	may be NULL if fake
	
	//	fake:
	CFXMLNodeTypeCode			i_typeCode;
	CCFString					i_str;
	CFXMLElementInfo			i_elemInfo;

	public:
	CCFXMLNode(CFXMLNodeRef nodeRef) : i_nodeRef(nodeRef) { }
	CCFXMLNode(CFXMLNodeTypeCode typeCode, CFStringRef stringRef) : 
		i_nodeRef(NULL), 
		i_typeCode(typeCode),
		i_str(stringRef, true)
	{
		structclr(i_elemInfo);
	}

	CFXMLNodeTypeCode		GetTypeCode()	{	return i_nodeRef ? CFXMLNodeGetTypeCode(i_nodeRef) : i_typeCode;	}
	CFXMLElementInfo*		GetInfoPtr()	{	return i_nodeRef ? (CFXMLElementInfo *)CFXMLNodeGetInfoPtr(i_nodeRef) : &i_elemInfo;	}
	CFStringRef				GetString()		{	return i_nodeRef ? CFXMLNodeGetString(i_nodeRef) : i_str.Get();	}
	operator				CFXMLNodeRef()	{	return i_nodeRef;	}
};


class CCFXmlParser {
	public:
	
	class Data {
		public:
		bool				i_trackPhysB;
		bool				i_inlineB;
		bool				i_call_all_completionsB;
		CCFString			i_physicalEntities;
		
		Data(
			bool trackPhysB				= false, 
			bool inlineB				= true, 
			bool call_all_completionsB	= false
		) : 
			i_trackPhysB(trackPhysB), 
			i_inlineB(inlineB),
			i_call_all_completionsB(call_all_completionsB) 
		{}

		virtual ~Data() {}

		virtual void	completion(short levelS) {
			UNREFERENCED_PARAMETER(levelS);
		}
	};

	Data&		i_data;

	CCFXmlParser(Data& data) : i_data(data) {}
	virtual ~CCFXmlParser() {}

	virtual void	completion(short levelS) {
		i_data.completion(levelS);
	}
};

class CCFXmlTree : public ScCFReleaser<CFXMLTreeRef> {
	typedef	ScCFReleaser<CFXMLTreeRef>	_inherited;

	public:
	CCFXmlTree(CFXMLTreeRef treeRef0 = NULL, bool retainB = false) : _inherited(treeRef0, retainB) { }

	bool	CreateFromData(CFDataRef xmlData, CFURLRef dataSource, CFOptionFlags parseOptions = kCFXMLParserSkipWhitespace);
	
	CFXMLNodeRef		GetNode()	{	return CFXMLTreeGetNode(Get());	}
	CFIndex				size() {
		return CFTreeGetChildCount(Get());
	}

	CFTypeRef			GetValueAtIndex(CFIndex idx) {
		return CFTreeGetChildAtIndex(Get(), idx);
	}

	CFXMLTreeRef		GetChild(CFStringRef childNameRef) {
		CCFXmlTree	childTree;

		loop (size()) {
			childTree.SetAndRetain((CFTreeRef)GetValueAtIndex(_indexS));

			CCFXMLNode	childNode(childTree.GetNode());

			if (::CFStringCompare(childNode.GetString(), childNameRef, kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
				return childTree.Get();
			}
		}
		return NULL;
	}
	
	CFXMLTreeRef		GetChild(const char *childKeyZ);

	void	ParseEntityRef(CCFString& physRef, CCFXMLNode& node, short levelS);

	typedef std::vector<CFTreeRef>	TreeVec;
	
	template <class Function>
	inline	void	for_each(Function f, short levelS = 0) {
		size_t		sizeL = size();
		
		if (sizeL) {
			TreeVec		treeVec(sizeL);
			
			CFTreeGetChildren(Get(), &treeVec[0]);
			
//			BOOST_FOREACH(CFTreeRef treeRef, treeVec)

			for (TreeVec::iterator it = treeVec.begin(); it != treeVec.end(); ++it) {
				CFTreeRef	treeRef(*it);
				CCFXmlTree	xmlTreeNode((CFXMLTreeRef)treeRef, true);
				CCFXMLNode	node(xmlTreeNode.GetNode());
				bool		parsePhysB = f.i_data.i_trackPhysB;
				
				/*
					see CMS_KDN for an example of iterating over attributes
				*/

				if (parsePhysB && node.GetTypeCode() == kCFXMLNodeTypeEntityReference) {
					ParseEntityRef(f.i_data.i_physicalEntities, node, levelS);
					
					if (f.i_data.i_inlineB) {
						CCFXMLNode		physNode(kCFXMLNodeTypeText, f.i_data.i_physicalEntities);
						
						f.i_data.i_physicalEntities.clear();
						f(physNode, levelS);
					}
					
					parsePhysB = false;
				} else {
					f(node, levelS);
				}
				
				xmlTreeNode.for_each(f, levelS + 1);
				
				if (parsePhysB && f.i_data.i_physicalEntities.Get()) {
					CCFXMLNode		physNode(kCFXMLNodeTypeText, f.i_data.i_physicalEntities);
					
					f.i_data.i_physicalEntities.clear();
					f(physNode, levelS);
				}
			}
			
			if (levelS != 0 && f.i_data.i_call_all_completionsB) {
				f.completion(levelS);
			}
		}
		
		if (levelS == 0) {
			f.completion(levelS);
		}
		
	//	int i = 0;
	}
};

CFArrayRef			CFLocaleCreateMonthArray();
CFURLRef			CFURLCreateWithPathStr(const SuperString& pathStr);
void				CFSleep(CFTimeInterval durationT = 0);

bool		Read_XML(const CFURLRef url, CCFXmlTree& xml, bool convertMacRomanToUTF8B = false);
bool		Read_XML(const SuperString &pathStr, CCFXmlTree& xml);

/*****************************************************/
//	logging
void	Log(const char *strZ, bool crB = true);
void	Log(const SuperString& str, bool crB = true);
void	LogAspect(const char *msgZ, int x, int y);

extern "C" void	Logf(const char *str,...);

void	LogRect(const char *msg, const Rect& rectR);


SuperString		GetIndentString(size_t tabsL);
void			IndentLevel(short levelS);

const char *	YesOrNo(bool yesB);
SuperString		YesOrNoStr(const char *msgZ, bool yesB);
void			LogYesOrNo(const char *msgZ, bool yesB);

void	IfLog(bool logB, const char *labelZ, const char *strZ, bool crB = true);
void	IfLog(bool logB, const char *labelZ, const SuperString& str, bool crB = true);

void	IfLogf(bool logB, const char *labelZ, const char *str,...);

SuperString		LogPtr_GetStr(const char *strZ, const void *ptr);
void			LogPtr(const char *strZ, const void *ptr);
void			IfLogPtr(bool ifB, const char *strZ, const void *ptr);

void	LogDialogInfo(
	const char *typeZ, 
	const char *titleZ, 
	const char *msgZ = NULL, 
	const char *resultStrZ = NULL);

#ifdef kDEBUG
extern bool s_breakOnFNF;
#endif

class ScNoBreakFNF {
	#ifdef kDEBUG
	bool		wasB;
	
	public: 
	ScNoBreakFNF() {
		wasB = s_breakOnFNF;
		s_breakOnFNF = false;
	}
	
	~ScNoBreakFNF() {
		s_breakOnFNF = wasB;
	}
	#endif
};

SuperString		GetUserName();
SuperString		GetConsoleFilePath();

bool		IsRamURL(const SuperString& str);
CFDataRef	RamURL_GetEssence(const SuperString& str);

bool		IsLocalURL(const SuperString& str);
CFDataRef	LocalURL_GetEssence(const SuperString& str);

CFXMLTreeRef	CCFXMLTreeCreateFromData(
	CFDataRef			xmlData, 
	CFURLRef			dataSource, 
	CFOptionFlags		parseOptions, 
	CFDictionaryRef		*errorDictP);

CFDataRef		CCFPropertyListCreateXMLData(
	CFPropertyListRef	propertyList,
	CFErrorRef			*errorRefP0 = NULL);

#include <typeinfo>

template <typename T>
void	CFAssertSize(size_t requiredSizeL)
{
	size_t		actualSizeL(sizeof(T));
	
	if (actualSizeL != requiredSizeL) {
		Logf(
			"%s size mismatch. Is: %d, should be %d",
			typeid(T).name(), (int)actualSizeL, (int)requiredSizeL);
		
		CF_ASSERT("Struct alignment mismatch" == NULL);
	}
}

template <typename T> class ScSetReset {
	T	*i_containerP;
	T	i_oldVal;

	public:	
	
	ScSetReset(T *containerP, const T &newVal)
		: i_containerP(containerP), i_oldVal(*containerP) 
	{
		*containerP = newVal;
	}
	
	void		keep() {
		i_oldVal = *i_containerP;
	}
	
	~ScSetReset() {
		*i_containerP = i_oldVal;
	}
	
	T			Get()	{	return i_oldVal;	}
};

typedef		SInt64	CFTimeIntervalMilliseconds;

CFTimeIntervalMilliseconds		CFSecondsToMilliseconds(CFTimeInterval secF);
CFTimeInterval					CFMillisecondsToSeconds(CFTimeIntervalMilliseconds milliI);

#endif
