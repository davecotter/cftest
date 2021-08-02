#ifndef _H_SuperString
#define _H_SuperString

#include <string>
#include <set>
#include <cctype>
#include "string.h"
#include <stdarg.h>
#include "CFUtils.h"

#if !defined(_CFTEST_) && !_PaddleServer_ && !defined(_ROSE_)
	#define	HAS_CDHMSF		1
	#include "CDHMSF.h"
#else
	#define	HAS_CDHMSF		0
#endif

#if _QT_

	#if defined(_JUST_CFTEST_)
		//	we are JUST CFTEST
	#else
		#include <QString>
		#define _HAS_QSTR_

		#include "BasicTypes.h"
		#include <QFontMetricsF>
	#endif
#endif

#define	kCodePage_WindowsLatin1				1252
#define	kCFStringEncodingPercentEscapes		(0xfffffffeU)
	
#ifdef __WIN32__	
	typedef std::vector<wchar_t>	WCharVec;	//	careful: 16bits on win, 32bits on mac
	//#include "Files.h"
#else
	#if !_QT_
		#include <Carbon/Carbon.h>
	#endif
#endif

typedef enum {
	EnquoteType_PLAIN,
	EnquoteType_SMART,
	EnquoteType_SINGLE_PLAIN,
	EnquoteType_SINGLE_SMART,
	EnquoteType_GRAVE,
	EnquoteType_PARENS,
	EnquoteType_PERCENTS,
	EnquoteType_SPACES,
	EnquoteType_NO_BREAK_SPACES
} EnquoteType;

/****************************************************************/
char *		mt_vsnprintf(const char *formatZ, va_list &args);
void		LogErr(const char *strZ, OSStatus err, bool crB = true, bool unixB = false);
SuperString	LogErr_GetStr(const char *utf8Z, OSStatus err, bool unixB = false);
void		LogErr_always(const char *utf8Z, OSStatus err, bool crB = true, bool unixB = false);
void		ReportErr(const SuperString& errTypeStr, OSStatus err = 0, bool unixB = false, bool postB = false);

void			SetLastErrorStr(const SuperString& str);
SuperString		GetLastErrorStr();

/****************************************************************/

char *			CopyDoubleToC(double valF, char *bufZ, short precisionS = 2);
double			CStringToDouble(const char *numF);
char*			strrstr(const char* stringZ, const char* findZ);
const char *	CopyLongToC(long valL);
OSType			GetExtension(const char *strZ);
SuperString		Get_XML_KeyStr(const SuperString& keyStr, bool endB = false);

CFStringRef			CFCopyEmptyString();
CFStringRef			CFStrCreateWithCurAbsTime();
bool				CFStringContains(CFStringRef inRef, CFStringRef findRef, bool case_and_diacritic_sensitiveB = false);
CFComparisonResult	CFStringCompare(CFStringRef str1, CFStringRef str2, bool case_and_diacritic_sensitiveB = false);
bool				CFStringEqual(CFStringRef str1, CFStringRef str2, bool case_and_diacritic_sensitiveB = false);
bool				CFStringLess(CFStringRef lhs, CFStringRef rhs, bool case_and_diacritic_sensitiveB = false);
bool				CFStringIsEmpty(CFStringRef nameRef);

#if (_QT_ || _QT6_) && OPT_WINOS
	#define kAppDefaultTextEncoding		kCFStringEncodingUTF8
#else
	#define kAppDefaultTextEncoding		kCFStringEncodingMacRoman
#endif

void				SetDefaultEncoding(CFStringEncoding encoding);
bool				IsDefaultEncodingSet();

ustring			&CopyCFStringToUString(
									   CFStringRef			str, 
									   ustring				&result, 
									   CFStringEncoding	encoding	= kCFStringEncodingUTF8, 
									   bool				externalB	= false);

CFStringRef		CFStringCreateWithC(
									const char *		bufZ, 
									CFStringEncoding	encoding = kCFStringEncodingInvalidId);

CFStringRef		CFStringCreateWithCu(
									 const UTF8Char *	bufZ, 
									 CFStringEncoding	encoding = kCFStringEncodingUTF8);

std::string		&CopyCFStringToStd(
								   CFStringRef			str, 
								   std::string			&stdstr, 
								   CFStringEncoding	encoding = kCFStringEncodingInvalidId);

void			CFStrReplaceWith(CFMutableStringRef stringRef, CFStringRef replaceStr, CFStringRef withStr, bool allB = true);

/*******************************/
//	upper case, prefix with "0x"
SuperString		ULong_To_HexStr(UInt32 valueL);

#define			kMaxOSTypeNumber		0x0F

OSType			CharToOSType(const char *bufZ, bool convertB = true);
void			OSType_ToLower(OSType *type);

bool			OSType_IsNumber(OSType osType, bool assertB = true);
bool			OSType_IsStringNumber(OSType osType);

OSType			OSType_FromNumber(UInt32 valL);
UInt32			OSType_ToNumber(OSType osType);

char			*OSTypeToChar(OSType osType, char *bufZ);
SuperString		OSTypeToString(OSType osType);

SuperString		UTF8HexToString(const char *hexZ);
SuperString		UTF8BytesToString(UInt32 bytesL);
SuperString		PtrToString(const void *ptr);

/****************************************************************/
typedef enum {
	UTF_SpecialChar_HYPHEN,
	UTF_SpecialChar_ZH_COLON,
	UTF_SpecialChar_NO_BREAK_SPACE,
	UTF_SpecialChar_EM_DASH,
	UTF_SpecialChar_EN_DASH,
	UTF_SpecialChar_FIGURE_DASH,
	UTF_SpecialChar_MINUS_SIGN,
	UTF_SpecialChar_SIX_PER_EM_SPACE,
	UTF_SpecialChar_RED_ALERT,
	UTF_SpecialChar_FULLWIDTH_PERCENT,

	UTF_SpecialChar_KEY_CHANGE_FLAT,
	UTF_SpecialChar_KEY_CHANGE_SHARP,
	UTF_SpecialChar_KEY_CHANGE_BAR,
	UTF_SpecialChar_KEY_CHANGE_DIAMOND,
	UTF_SpecialChar_KEY_CHANGE_TRIANGLE,

	UTF_SpecialChar_LETTER_YOO,	//	stupid hack for Qt for "AboUt kJams" shenanegens
	UTF_SpecialChar_EM_SPACE,
	UTF_SpecialChar_e_ACUTE,
	UTF_SpecialChar_SMALL_ASTERISK,
	UTF_SpecialChar_BLACK_DOWN_TRIANGLE,
	UTF_SpecialChar_COPYRIGHT,

	UTF_SpecialChar_NUMTYPES
} UTF_SpecialChar;

SuperString		GetSpecialChar(UTF_SpecialChar charType);
SuperString		GetHyphen();

/****************************************************************/

typedef struct {
	const char	*replaceZ;
	const char	*withZ;
} SuperStringReplaceRec;

#if	defined(_KJAMS_) || defined(_ROSE_) || defined(_QTServer_) || _PaddleServer_ || (_QT_ && !defined(_JUST_CFTEST_))
	#define HFS_UNISTRING 1
	
#else
	#define HFS_UNISTRING 0
#endif

class	UniString {
	UTF16Char	i_nullChar;
	UTF16Vec	*i_charVecP;
	
	void		UpdateNamePointer() {
		if (i_charVecP) {
			i_charVecP->push_back(0);
			i_nameP = &(*i_charVecP)[0];
		} else {
			i_nameP = &i_nullChar;
		}
	}
	
	void		SetNULL() {
		i_nullChar = 0;
		i_charVecP = NULL;
		i_lengthL = 0;
		i_nameP = NULL;
	}
	
	public:
	void	Initialize(CFStringRef cf_name, bool forceBigEndianB = false) {
		if (cf_name && !CFStringIsEmpty(cf_name)) {
			ustring		utf16str;
			
			CopyCFStringToUString(cf_name, utf16str, forceBigEndianB ? kCFStringEncodingUTF16BE : kCFStringEncodingUTF16);
			
			//	divide by 2
			i_lengthL	= utf16str.size() >> 1;
			
			if (i_charVecP == NULL) {
				i_charVecP = new UTF16Vec();
			}
			
			UTF16Char	*utf16A = (UTF16Char *)utf16str.c_str();
			
			i_charVecP->assign(&utf16A[0], &utf16A[i_lengthL]);
			//			CFStringGetCharacters(cf_name, CFRangeMake(0, i_lengthL), &(*i_charVecP)[0]);
		} else {
			delete i_charVecP;
			i_charVecP = NULL;
			i_lengthL = 0;
		}
		
		UpdateNamePointer();
	}

	#if defined(__MWERKS__)
		typedef long	UniCharCount;
	#endif

	UniCharCount		i_lengthL;
	UniChar				*i_nameP;
	
	const UniChar*	begin() const {	return &i_nameP[0];	}
	const UniChar*	end()	const {	return &i_nameP[i_lengthL];	}
	size_t			size()	const { return i_lengthL; }

	UniString(const UniString &uni)	{
		SetNULL();
		i_lengthL	= uni.i_lengthL;
		
		if (i_lengthL) {
			i_charVecP = new UTF16Vec();
			i_charVecP->resize(i_lengthL);
			std::copy(&uni.i_nameP[0], &uni.i_nameP[i_lengthL], &(*i_charVecP)[0]);
		}
		
		UpdateNamePointer();
	}
	
	UniString(CFStringRef cf_name, bool forceBigEndianB = false) {
		SetNULL();
		Initialize(cf_name, forceBigEndianB);
	}
	
	UniString(const char *nameZ = NULL) {
		SetNULL();
		
		if (nameZ) {
			CCFString		cf_name(CFStringCreateWithC(nameZ));
			
			Initialize(cf_name);
		}
	}

	#if HFS_UNISTRING
	HFSUniStr255		Get() const
	{
		HFSUniStr255		uniStr;

		if (i_lengthL > 255) {
			ETX(pathTooLongErr);
		}

		uniStr.length = (UInt16)i_lengthL;

		if (i_lengthL) {
			std::copy(i_charVecP->begin(), i_charVecP->end(), &uniStr.unicode[0]);
		}

		return uniStr;
	}
	
	operator HFSUniStr255() const {
		return Get();
	}
	#endif

	~UniString() {
		delete i_charVecP;
	}
};

typedef std::vector<UTF32Char>		UTF32Vec;
typedef std::basic_string<UTF32Char, std::char_traits<UTF32Char>, std::allocator<UTF32Char> > ww_string;

#define	kCFStringNormalizationFormKD_HFSPlus	(CFStringNormalizationForm)(kCFStringNormalizationFormKC + 1)
#define	kCFStringNormalizationFormKC_HFSPlus	(CFStringNormalizationForm)(kCFStringNormalizationFormKD_HFSPlus + 1)

/**********************************************************************************************************/
class SuperString {
	CFStringRef				i_ref;
	mutable std::string		*i_std;
	mutable UniString		*i_uni;
	mutable ustring			*i_utf8;
	mutable ustring			*i_pstr;
	mutable ww_string		*i_utf32;
	
public:
	const	std::string	&std() const	{	Update_std();	return *i_std;	}
	const	CFStringRef	&ref() const	{	return i_ref;	}
	const	UniString	&uni(bool forceBigEndianB = false) const	{	Update_uni(forceBigEndianB);	return *i_uni;	}
	const	ustring		&utf8() const	{	Update_utf8();	return *i_utf8;	}
	const	char *		c_str() const  	{	return std().c_str();	}
	const	char *		utf8Z() const	{	return (const char *)utf8().c_str();	}
	ConstStr255Param	p_str() const	{	Update_pstr();	return i_pstr->c_str();	}
	const	ww_string&	utf32() const	{	Update_utf32();	return *i_utf32;		}

	#ifdef _HAS_QSTR_
	QString		q_str() const;
	SuperString(const QString& str);
	operator QString() const { return q_str(); }
	#endif

	#ifdef __WIN32__
		LPCWSTR		w_str() const			{	return (LPCWSTR)uni().i_nameP;		}
	#else
		UTF16Char*	w_str() const			{	return uni().i_nameP;		}
	#endif

	size_t		w_strlen() const		{	return w_size();	}
	
	operator const UniString&() const	{	return uni();	}
	operator const std::string&() const	{	return std();	}
	//operator const ustring&() const		{	return utf8();	}	causes all sorts of ambiguities
	operator CFStringRef() const		{	return ref();	}
	operator const UInt8*() const		{	return utf8().c_str();	}

	/************************************/
//	SuperString&	Truncate(bool activeB, const Rect& frameR);
	SuperString&	ssprintf(const char *formatZ0, ...);
	
private:
	void	SetNULL()
	{
		i_ref	= NULL;
		i_std	= NULL;
		i_uni	= NULL;
		i_utf8	= NULL;
		i_utf32 = NULL;
		i_pstr	= NULL;
	}
	
public:
	#ifdef __WIN32__
		SuperString(const wchar_t *wcharZ);
		
		SuperString(const WCharVec& wcharVec) {
			SetNULL();
			Set(*(UTF16Vec *)&wcharVec);
		}
	#endif

	#if HFS_UNISTRING
	SuperString(const HFSUniStr255 &str) {
		SetNULL();

		CCFString		myRef(CFStringCreateWithCharacters(
			kCFAllocatorDefault, 
			str.unicode,
			str.length));

		Set(myRef);
	}
	#endif
	
	SuperString(const UTF32Char *strZ, size_t sizeL = -1);
	SuperString(const UTF32Char ch);
	
	SuperString(CFURLRef urlRef);
	
	SuperString(const char *strZ = NULL, CFStringEncoding encoding = kCFStringEncodingInvalidId) {
		SetNULL();		
		Set(uc(strZ), encoding);
	}
	
	SuperString(const SuperString &sstr) {
		SetNULL();
		Set(sstr.ref());
	}

#if HAS_CDHMSF
	void	Set(const CDHMSF msf, kTimeCodeType type = kTimeCode_NORMAL) {
		char	bufAC[256];
		
		Set(uc(CopyMSFToC(msf, bufAC, type)));
	}

	SuperString(const CDHMSF msf, kTimeCodeType type = kTimeCode_NORMAL) {
		SetNULL();
		Set(msf, type);
	}
#endif

	/*
	 FAIL!
	SuperString(const UInt16* strZ) {
		SetNULL();
		
		CCFString		myRef(strZ 
			? CFStringCreateWithC((const char *)strZ, kCFStringEncodingUnicode) : 
			CFCopyEmptyString());

		Set(myRef);
	}
	*/
	
	SuperString(const UInt8 *strZ) {
		SetNULL();
		Set(strZ);
	}
	
	SuperString(long valL) {
		SetNULL();
		append(valL);
	}
	
	SuperString(int valI) {
		SetNULL();
		append((long)valI);
	}
	
	SuperString(double valF, short precS = 1) {
		SetNULL();
		append(valF, precS);
	 }
	
	SuperString(const ustring &str) {
		SetNULL();
		Set(str.c_str());
	}
	
	SuperString(const std::string &str) {
		SetNULL();
		Set(str.c_str());
	}
	
	SuperString(CFStringRef myRef, bool retainB = true) {
		SetNULL();
		Set(myRef, retainB);
	}

	void	LogCount(const char *nameZ);
	void	Set_CFType(CFTypeRef cfType);
	
	/************************************/
	void	Set_p(ConstStr255Param strZ, CFStringEncoding encoding = kCFStringEncodingInvalidId);
	
	SuperString&	Set(
		CFAbsoluteTime	absT, 
		SS_TimeType		timeType	= SS_Time_SHORT, 
		CFTimeInterval	epochT		= 0,
		CFTimeZoneRef	timeZoneRef	= NULL);

	void	Set(const char *strZ) {
		CCFString		myRef(CFStringCreateWithC(strZ));
		
		Set(myRef);
	}
	
	void	Set(const UInt8 *strZ, CFStringEncoding encoding = kCFStringEncodingUTF8);
	
	SuperString&	Escape(SuperString allowedStr = SuperString(), SuperString disallowedStr = SuperString());
	void			UnEscape();
	
	bool			ConformsToDateFormat(SS_TimeType timeType);
	SS_TimeType		GetTimeType();

	#if _QT_ && !_JUST_CFTEST_
	static SuperString		GetBestPrettyDate(CFDateRef dateRef, const Rect& frameR, QFontMetricsF& metricsF);
	#endif

	bool	IsJSON() const;
	void	UnEscapeJSON();
	void	EscapeJSON();
	
	bool			GetUnicodeHexStr(SuperString *foundStrP);
	SuperString		UnicodeHexToStr();

	void	Set(const ustring& utf8, CFStringEncoding encoding = kCFStringEncodingUTF8) {
		Set(utf8.c_str(), encoding);
	}
	
	void	Set(const UTF16Vec &vec);
	
	void	Get(UTF16Vec &vec, bool forceBigEndianB = false) const;
	
	void	Set(const ww_string &wwStr);
	void	Set(const UCharVec& byteVec, CFStringEncoding encoding = kCFStringEncodingUTF8);
	
	void	assign(const UTF8Char *begin_bytesP, const UTF8Char *end_bytesP, CFStringEncoding encoding = kCFStringEncodingUTF8);
	
/*
	void	assign(const UTF8Char *beginZ, const UTF8Char *endZ, CFStringEncoding encoding = kCFStringEncodingUTF8) {
		ustring		str(beginZ, endZ);
		
		Set(str, encoding);
	}
*/	
	void	Set(const SuperString &sstr) {
		if (&sstr != this) {
			Set(sstr.i_ref);
		}
	}
	
  	CFStringRef	Retain() const {	CFRetainDebug(i_ref);	return i_ref;	}
	
	void	Set(CFStringRef myRef, bool retainB = true);
	
	/************************************/
	~SuperString() {
		clear();
		Release();	//	releases the null string
		i_ref = NULL;
	}
	
	void	Release() {
		CFReleaseDebug(i_ref);
	}
	
	/************************************/
	void	Update_pstr() const {
		
		if (!i_pstr) {
			i_pstr = new ustring;
			i_pstr->push_back(0);
			Update_std();
			i_pstr->insert(i_pstr->end(), i_std->begin(), i_std->end());
			CF_ASSERT(i_pstr->size() < 256);
			(*i_pstr)[0] = (UInt8)(i_pstr->size() - 1);
			
			delete i_std;
			i_std	= NULL;
		}
	}
	
	void	Update_uni(bool forceBigEndianB) const {
		if (!i_uni) {
			i_uni = new UniString(i_ref, forceBigEndianB);
		}
	}
	
	void	Update_std() const {
		if (!i_std) {
			i_std = new std::string;
			CopyCFStringToStd(i_ref, *i_std);
		}
	}
	
	void	Update_utf32() const;
	
	//	you can pass say kCFStringEncodingWindowsLatin1 if you want
	void	Update_utf8(CFStringEncoding encoding = kCFStringEncodingUTF8) const {
	
		if (encoding != kCFStringEncodingUTF8 && i_utf8) {
			delete i_utf8;
			i_utf8 = NULL;
		}
		
		if (!i_utf8) {
			i_utf8 = new ustring;
			CopyCFStringToUString(i_ref, *i_utf8, encoding);
		}
	}

	//	convert to "from", then assume it is "to"
	SuperString&	Reinterpret(CFStringEncoding fromEncoding, CFStringEncoding toEncoding);

	/************************************/
	void	clear()	{	Set((CFStringRef)NULL);	}
	
	//	insensitive
	bool				ContainsI(SuperString other) const {
		SuperString		self(*this);

		self.Normalize(kCFStringNormalizationFormD, true);
		other.Normalize(kCFStringNormalizationFormD, true);
		return self.Contains(other);
	}

	SuperString&		ReplaceI(const SuperString& in_replaceStr, const SuperString& withStr, bool allB = true) {
		SuperString		self(*this);
		SuperString		replaceStr(in_replaceStr);

		self.Normalize(kCFStringNormalizationFormD, true);
		replaceStr.Normalize(kCFStringNormalizationFormD, true);
		return self.Replace(replaceStr, withStr, allB);
	}

	bool				Contains(const SuperString& other) const {
		return strstr(utf8Z(), other.utf8Z()) != NULL;
	}
	
	//	returns number of utf8 bytes (not characters) that match the start of the other string
	size_t				MatchStart(const SuperString& other, char delimiterCh = 0) const;

	bool				StartsWith(const SuperString& other) const {
		return MatchStart(other) == other.size();
	}
	
	bool				EndsWith(const SuperString& other) const;
	
	SuperString&		ScrubSensitiveInfo();
	SuperString&		ReplaceTable(SuperStringReplaceRec *recA, long sizeL, CFStringEncoding encoding = kCFStringEncodingInvalidId);
	SuperString&		ReplaceTable_Reverse(SuperStringReplaceRec *recA, long sizeL);
	SuperString&		Replace(const SuperString& replaceStr, const SuperString& withStr, bool allB = true) {
		ScCFReleaser<CFMutableStringRef>	newRef(CFStringCreateMutableCopy(
			kCFAllocatorDefault, 0, i_ref));
		
		CFStrReplaceWith(newRef, replaceStr.ref(), withStr.ref(), allB);
		Set(newRef);
		return *this;
	} 
	
	SuperString&		Smarten();
	SuperString			&UnderScoresToSpaces();
		
	//	returns a new string, does not modify original
	SuperString			md5() const;
	
	#define			kBitsToRotate	2

	bool				IsHexString();
	SuperString&		Scramble(short rotateS = kBitsToRotate);
	SuperString&		UnScramble(short rotateS = -kBitsToRotate);
	
	SuperString&		Enquote(EnquoteType quoteType, bool only_if_notB = false);
	SuperString&		Enquote(bool smartB = false) { return Enquote((EnquoteType)smartB); }
	SuperString			Quoted(EnquoteType quoteType = EnquoteType_SMART) const;

	SuperString&		NoQuotes(bool recoverB = true);
	SuperString&		trim();

	//	wrap with full-width space chars
	SuperString&		Enspace();
	
	SuperString&		InsertSpaces(size_t everyNth);

#ifndef _QTServer_
	SuperString&		Hexify(bool to_upperB = false)	 { Scramble(0);	if (to_upperB) { ToUpper(); } return *this;	}
	SuperString&		UnHexify()	 { return UnScramble(0);	}
#endif

	void			MakeRecoverable(CFRecoverType recoverType = CFRecoverType_NORMAL) {
		#if !defined(_CFTEST_) && !defined(_MIN_CF_)
			CCFString		sc(CFStrCreateRecoverableName(i_ref, recoverType));
		
			Set(sc);
		#else
			UNREFERENCED_PARAMETER(recoverType);
		#endif
	}
	
	SuperString&	Recover();

	SuperString&	RecoverCaps(bool titleCaseB = true) {
		Recover();
		InterCaps(false, titleCaseB);
		MakeRecoverable();
		return *this;
	}
		
	CFDataRef		CopyDataRef() {
		return CFStringCreateExternalRepresentation(
			kCFAllocatorDefault, i_ref, kCFStringEncodingUTF8, 0);
	}
	
	void			Set(
		CFDataRef			dataRef,
		CFStringEncoding	encoding = kCFStringEncodingUTF8
	) {
		CCFString		myRef(CFStringCreateFromExternalRepresentation(
			kCFAllocatorDefault, dataRef, encoding));

		Set(myRef);
	}
	
	size_t			size() const	{	return utf8().size();		}	//	utf8 chars
	size_t			w_size() const	{	return uni().i_lengthL;		}	//	utf16 chars
	
	/*
	 ambiguous
	 
	 char			operator[](size_t indexS) {
	 char	ch = 0;
	 
	 if (utf8().size() > indexS) {
	 ch = utf8().c_str()[indexS];
	 }
	 
	 return ch;
	 }
	 */
	
	/*******************************/
	size_t			ww_size() const	{	return utf32().size();		}	//	utf32 chars
	
	SuperString		GetSection_ww(size_t beginL, size_t endL);
	void			DeleteSection_ww(size_t beginL, size_t endL);
	void			InsertSection_ww(size_t beginL, const SuperString& str);
	
	UTF32Char		GetIndChar_ww(size_t indexL = 0) const {
		UTF32Char		ch = 0;
		
		if ((long)indexL >= 0L && indexL < utf32().size()) {
			ch = utf32()[indexL];
		}
		
		return ch;
	}

	UTF32Char		GetIndCharR_ww(size_t indexL = 0) const {
		return GetIndChar_ww(utf32().size() - (indexL + 1));
	}
	/*******************************/
		
	UTF8Char		GetIndChar(size_t indexL = 0) const {
		UTF8Char		ch = 0;
		
		if ((long)indexL >= 0L && indexL < utf8().size()) {
			ch = utf8()[indexL];
		}
		
		return ch;
	}
	
	UTF8Char		GetIndCharR(long indexL = 0) const {
		return GetIndChar(utf8().size() - (indexL + 1));
	}
	
	SuperString&	IncNumberAtEnd();
	SuperString&	RemoveNumberAtEnd();

	SuperString&	InsertAt(size_t posL, const SuperString& str, bool from_endB = false);
	SuperString&	ToUpper();
	SuperString&	ToLower();
	bool			IsAllCaps();
	SuperString&	FoldStrip(bool foldCaseB, bool stripB);
	SuperString&	Normalize(CFStringNormalizationForm form = kCFStringNormalizationFormD, bool foldCaseB = false, bool stripB = false);
	SuperString&	InterCaps(bool allow_line_breaksB = false, bool titleCaseB = true);
	SuperString&	TheToEnd();
	
	SuperString&	operator =(const SuperString &other)	{	Set(other);		return *this;	}
	SuperString&	operator =(const char *otherZ)			{	Set(otherZ);	return *this;	}

	SuperString&	operator +=(const SuperString &other)	{	append(other);	return *this;	}
	SuperString&	operator +=(const char *otherZ)		 	{	append(otherZ);	return *this;	}
	SuperString&	operator +=(char asciiCh)		 		{	append(asciiCh); return *this;	}
	
	bool			operator<(CFStringRef other) const {
		return CFStringLess(i_ref, other);
	}
	
	bool			operator>(CFStringRef other) const {
		return CFStringLess(other, i_ref);
	}
	
	bool			operator ==(CFStringRef other) {
		return CFStringEqual(i_ref, other);
	}
	
	bool			operator ==(CFStringRef other) const {
		return CFStringEqual(i_ref, other);
	}
	
	bool			operator !=(CFStringRef other)	{
		return ! operator==(other);
	}
	
	bool			operator ==(const SuperString &other) {
		return operator==(other.i_ref);
	}
	
	bool			operator ==(const SuperString &other) const {
		return operator==(other.i_ref);
	}
	
	bool			operator !=(const SuperString &other)	{
		return ! operator==(other.i_ref);
	}

	bool			operator !=(SuperString &other)	{
		return ! operator==(other.i_ref);
	}
	
	bool			operator !=(const SuperString &other) const {
		return ! operator==(other.i_ref);
	}
	
	bool	IsNumeric() const;
	bool	empty() const		{	return CFStringIsEmpty(i_ref);	}

/*
	CFNumberRef		CopyAs_NumberRef()'
	CFDateRef		CopyAs_DateRef()'

	UInt32			GetAs_UInt32();
	UInt16			GetAs_UInt16();
	SInt16			GetAs_SInt16();
*/

	void			CopyAs_wchar(wchar_t *bufA, size_t buf_sizeL) const;
	CFURLRef		CopyAs_URLRef(bool escapeB = true) const;
	
	CFAbsoluteTime	GetAs_CFAbsoluteTime(
		SS_TimeType		timeType = SS_Time_SHORT, 
		CFTimeInterval	epochT = 0,
		CFTimeZoneRef	timeZoneRef = NULL) const;
		
	OSType			GetAs_OSType(bool justifyB = false) const;
	SInt32			GetAs_SInt32() const	{	return ::atoi(c_str());	}

	void			Set(SInt64 valueL);
	SInt64			GetAs_SInt64() const;

	void			Set(UInt64 valueL);
	UInt64			GetAs_UInt64() const;

	double			GetAs_Double() const	{	return CStringToDouble(c_str());	}
	UInt32			GetAs_Hash() const;
	UInt64			GetAs_Hash64() const;
	UCharVec		GetAs_UCharVec(size_t first_n = 0)		{
		UCharVec		vec(first_n == 0 ? size() : first_n);
		
		utf8();
		std::copy(i_utf8->begin(), i_utf8->begin() + vec.size(), &vec[0]);
		return vec;
	}

	void			Set_Ptr(Ptr ptrP);
	Ptr				GetAs_Ptr() const;
	
	template <typename T>
	T				GetAs_PtrT() const
	{
		return reinterpret_cast<T>(GetAs_Ptr());
	}

	long			value_long() const	{	return GetAs_SInt32();	}
	UInt32			hex_to_ulong()		{	return Hex_To_ULong(c_str());	}
	
	//	returns the chars popped off the front
	SuperString		ww_pop_front(size_t num_utf32_charsL = 1);

	//	returns the chars popped off the front
	SuperString		pop_front(size_t numL = 1)	{
		SuperString			str;
		
		if (numL) {
			if (utf8().size() <= numL) {
				str.Set(*this);
				clear();
			} else {
				UCharVec	bufAC(GetAs_UCharVec(numL));

				bufAC.push_back(0);
				str.Set(&bufAC[0]);
				Set(&(utf8().c_str())[numL]);
			}
		}
		
		return str;
	}
	
	short	count_match(const char *matchZ) {
		short		countS = 0;
		const char	*chZ = (const char *)utf8().c_str();
		
		do {
			chZ = strstr(chZ, matchZ);
			if (chZ) {
				++countS;
				++chZ;
			}
		} while (chZ);
		
		return countS;
	}
	
	short	count_match_ww(const SuperString& matchStr);

	SuperString&	Split(size_t splitAt, SuperString& rhs);

	SuperString&	SplitAfterCount(char splitC, size_t keep_countL);

	bool			rSplit(const char *splitZ, SuperString *lhsP0 = NULL, bool from_endB = false);	
	bool			Split(const char *splitZ, SuperString *rhsP0 = NULL, bool from_endB = false);
	SuperString		extract(const char *lhsZ, const char *rhsZ, bool destructiveB = false, bool keep_pre_lhsB = false);
	SuperString		extracti(const char *lhsZ, const char *rhsZ, bool destructiveB = false, bool keep_pre_lhsB = false);

	SuperString&	append_xml(const SuperString& keyStr, const SuperString& valStr);
	SuperString		extract_xml(const char *keyZ, bool destructiveB = false);
	
	SuperString	&pop_back(size_t numCharsL = 1);
	SuperString	&pop_back_str(const SuperString& str);
	SuperString	&pop_back_ww(size_t numCharsL = 1);
	
	OSType	&pop_ext(OSType *extP) const;
	
	OSType	get_ext() const {
		OSType		ext;
		
		pop_ext(&ext);
		return ext;
	}
	
	SuperString	&pop_ext(SuperString *extP0 = NULL) {
		ustring				ustr(utf8());
		const unsigned char	*dotP;
		
		dotP = uc(strrchr(utf8Z(), '.'));
		
		if (dotP) {
			if (extP0) {
				extP0->Set(dotP);
			}
			
			ustr.resize(ustr.size() - strlen((char *)dotP));
			
			Set(ustr);
		}
		
		return *this;
	}
	
	SuperString		GetExt()
	{
		SuperString		otherStr(*this);
		SuperString		extStr;

		otherStr.pop_ext(&extStr);
		return extStr;
	}

	/******************************/
	SuperString		&resize(size_t num_utf8_charsL) {
		ustring		ustr(utf8());
		
		ustr.resize(num_utf8_charsL);
		Set(ustr);
		return *this;
	}

	SuperString		&ww_resize(size_t num_utf32_charsL);

	SuperString		&append(const ustring &other) {
		Set(utf8() + other);
		return *this;
	}
	
	SuperString		&append(SuperString &other) {
		return append(other.utf8());
	}
	
	SuperString		&append(CFStringRef myRef) {
		return append(SuperString(myRef).utf8());
	}
	
	SuperString		&append(const char *other) {
		return append(SuperString(other).utf8());
	}
	
	SuperString		&append(long valueL)
	{
		char	bufAC[32];
		
		::sprintf(bufAC, "%ld", valueL);
		return append(uc(bufAC));
	}
	
	SuperString		&UnEscapeAmpersands();

	SuperString		&append(char ch)
	{
		char	bufAC[2];
		
		bufAC[0] = ch;
		bufAC[1] = 0;
		
		SuperString		appendStr(bufAC, kCFStringEncodingASCII);
		
		append(appendStr);
		return *this;
	}
	
	SuperString		&append(double valueF, short precS = 1)
	{
		char	bufAC[32];
		
		CopyDoubleToC(valueF, bufAC, precS);
		return append(bufAC);
	}
	
	/****************************/
	SuperString		&prepend(const ustring &other) {
		Set(other + utf8());
		return *this;
	}
	
	SuperString		&prepend(const SuperString &other) {
		return prepend(other.utf8());
	}
	
	SuperString		&prepend(const char *other) {
		return prepend(SuperString(other));
	}
	
	SuperString		&prepend(long valueL) {
		char	bufAC[32];
		
		::sprintf(bufAC, "%.2ld", valueL);
		return prepend(bufAC);
	}
	
	//	allows alpha, numeric, and "period"
	SuperString		&AlphaNum();
	SuperString		&Alpha();
	SuperString		&Ascii(bool including_high_valuesB = false);
	SuperString		&Localize(bool recurse_okayB = true);
};

#define			kElipsisStr		SuperString((UTF32Char)0x2026)

SuperString		operator+(const SuperString &lhs, SuperString rhs);
SuperString		operator+(const SuperString &lhs, const char *rhs);

UInt64			ParseBytes(SuperString str);
SuperString		FormatBytes(UInt64 bytes);

#if OPT_WINOS
inline SuperString		SuperString_CreateFrom_UniChar(
	UniCharCount     sizeL,
	const UniChar *  nameA)
{
	CF_ASSERT(nameA[sizeL] == 0);
	return SuperString(reinterpret_cast<const wchar_t *>(nameA));
}
#endif

void			IncrementNumberAtEndOfString(SuperString *strP);
bool			GetEditNumber(SuperString *nameStrP, int *numSP);
void			AddEditNumber(SuperString *strP, SuperString *numStrP0 = NULL);
SuperString		TruncEscapedToNum(size_t numS, SuperString str);

typedef std::deque<SuperString>								SStringDeq;
typedef std::vector<SuperString>							SStringVec;
typedef std::map<SuperString, SuperString>					SStringMap;
typedef std::map<SuperString, SInt32>						SStringToSInt32Map;
typedef	std::vector<std::pair<SuperString, SuperString> >	SStringPairVec;
typedef std::set<SuperString>								SStringSet;
typedef std::map<UInt32, SuperString>						UIntToStringMap;
typedef std::map<SInt32, SInt32>							SInt32Map;	//	well, not actually related to superstrings

SStringMap			DictToStringMap(CFDictionaryRef dictRef);
CFDictionaryRef		CopyStringMapToDictionary(CFDictionaryRef dictRef);

SuperString		TimeToString(SInt32 curT);				//	in records (300ths), not sectors (75ths)
SInt32			StringToTime(const SuperString& str);	//	in records (300ths), not sectors (75ths)

#define		_HAS_PROG_		(_KJAMS_)

class	Sort_Str_LessThan {
	#if _HAS_PROG_
	bool		i_progB;
	#endif

	bool		i_case_and_diacritic_sensitiveB;
	
	public:
	Sort_Str_LessThan(bool case_and_diacritic_sensitiveB = false, bool progB = false) : 
		#if _HAS_PROG_
		i_progB(progB),
		#endif
		i_case_and_diacritic_sensitiveB(case_and_diacritic_sensitiveB)
	{
		UNREFERENCED_PARAMETER(progB);
	}
		
	bool	operator()(const SuperString& str1, const SuperString& str2) const;
};



#endif
