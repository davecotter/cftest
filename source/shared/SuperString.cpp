/*
 *  SuperString.cpp
 *  CFTest
 *
 *  Created by David M. Cotter on 6/25/08.
 *  Copyright 2008 David M. Cotter. All rights reserved.
 *
 */
#include "stdafx.h"
#include "CFUtils.h"

#if defined(_QTServer_)
	#include "CarbonEventsCore.h"
#endif

#if defined(_HELPERTOOL_)
	#include "CTenFive_Funcs.h"
#endif

#if defined(_CFTEST_) || defined(rDEBUG)
	bool	g_debugStringsB = false;	//	change this for debugging
#else
	bool	g_debugStringsB = false;	//	DO NOT CHANGE this
#endif

#ifdef _KJAMS_
	#include "CApp.h"
	#include "CPreferences.h"
	#include "PrefKeys_p.h"
#endif

#if OPT_KJMAC
	#include "CThreads.h"
	#include "MessageAlert.h"
	#include "CLocalize.h"

#else
	#if defined(_CFTEST_)
		#include <algorithm>
	#else
		#include "CThreads.h"
		
		#if _YAAF_
			#include <XErrorDialogs.h>
			#include "CLocalize.h"
		#else
			#include "MessageAlert.h"
		#endif
	#endif
#endif

#include "SuperString.h"

static CFStringRef		CFStringCreateWithUTF32(const UTF32Char *bufA, size_t utf32CharCountL)
{
	return CFStringCreateWithBytes(
		kCFAllocatorDefault, (UInt8 *)&bufA[0], 
		utf32CharCountL << 2, kCFStringEncodingUTF32LE, false);
}

SuperString			PtrToString(const void *ptr)
{
	//	use %p for pointer in 64 bit
	SuperString		str("%p");
	
	#if OPT_WINOS
		str.prepend("0x");
	#endif

	str.ssprintf(NULL, ptr);
	
	str.pop_front(2);
	
	//	leading zeroes
	while (str.size() < 8) {
		str.prepend("0");
	}
	
	str.ToUpper();
	str.prepend("0x");
	
	return str;
}

/*******************************************/

#ifdef _KJAMS_
CMutexT<SuperString>		s_lastErrStr;
#else
SuperString					s_lastErrStr;
#endif
void			SetLastErrorStr(const SuperString& in_str)
{
	SuperString		str(in_str);
	
	str.Replace("\n", ". ");
	s_lastErrStr.Set(str);
}

SuperString		GetLastErrorStr()
{
	return s_lastErrStr;
}
/*******************************************/
//	SBI: CFUtils.h
bool	CFGetLogging();
void		LogErr(const char *utf8Z, OSStatus err, bool crB, bool unixB)
{
	if (
		err 
		&& err != abortErr 
		&& err != eofErr 
		&& err != userCanceledErr
#ifdef _KJAMS_
		&& !ScSilenceStrangePathWarn::IsSilent()
		&& err != ERR_Already_Reported
		&& CFGetLogging()
#endif
	) {
		LogErr_always(utf8Z, err, crB, unixB);
	}
}

SuperString		LogErr_GetStr(const char *utf8Z, OSStatus err, bool unixB)
{
	SuperString		valStr;
	
	if (err) {
		
		#if defined(_MIN_CF_) || defined(_CFTEST_) || defined(_ROSE_)
			UNREFERENCED_PARAMETER(unixB);
			
			valStr.Set(uc(utf8Z));
			valStr.append(": ");
			valStr.append((long)err);
		#else
			if (unixB) {
				valStr.Set(UnixErrStr(uc(utf8Z), err));
			} else {
				valStr.Set(ErrStr(uc(utf8Z), err));
			}
		#endif

		valStr.append(" (");
		valStr.append((long)err);
		valStr.append(")");

		valStr.append(" (");
		valStr.append(ULong_To_HexStr(err));
		valStr.append(")");		
	}
	
	return valStr;
}

void		LogErr_always(const char *utf8Z, OSStatus err, bool crB, bool unixB)
{
	if (err) {
		SuperString		valStr(LogErr_GetStr(utf8Z, err, unixB));
		CCFLog			logger(crB);
		
		logger(valStr.ref());		
		SetLastErrorStr(uc(utf8Z));
	}
}

#if OPT_KJMAC || (_QT_ && _KJAMS_)
void			DebugReport(const SuperString& errTypeStr, OSStatus err)
{
	#if defined(kDEBUG) && !defined(_MIN_CF_)
		ReportErr(errTypeStr, err);
	#else
		LogErr(errTypeStr.utf8Z(), err);
	#endif
}

void	DebugReport(const char *utf8Z, OSStatus err)
{
	DebugReport(SuperString(utf8Z), err);
}

class CPostReportErr : public CMainThreadProc {
	SuperString		i_errTypeStr;
	OSStatus		i_err;
	bool			i_unixB;

	public:
	CPostReportErr(
		const SuperString&	errTypeStr, 
		OSStatus			err, 
		bool				unixB
	) :
		i_errTypeStr(errTypeStr), 
		i_err(err),
		i_unixB(unixB), CMainThreadProc("ReportErr")
	{
		call(CMT_Call_FORCE_POST);
	}
	
	void	operator()() {
		ReportErr(i_errTypeStr, i_err, i_unixB, false);
	}
};

void			ReportErr(const SuperString& errTypeStr, OSStatus err, bool unixB, bool postB)
{
	SuperString			errStr;
	SuperString			str;
	
	if (unixB) {
		errStr.Set(SSLocalize("Unix Error:", "bummer"));
	} else {
		errStr.Set(SSLocalize("Mac Error", "bummer"));
	}
	
	#if defined(_MIN_CF_)
		str.Set(errTypeStr);
		str.append(": ");
		str.append((long)err);
	#else
		if (unixB) {
			str.Set(UnixErrStr(errTypeStr, err));
		} else {
			str.Set(ErrStr(errTypeStr, err));
		}
	#endif
	
	if (IsPreemptiveThread() || (gApp && gApp->GetCurScope())) {
		postB = true;
	}
	
	if (postB) {
		PostAlert(errStr.utf8Z(), str.utf8Z());
	} else {
		MessageAlert(errStr.utf8Z(), str.utf8Z());
	}
}
//	end #if OPT_KJMAC
#else

void	DebugReport(const char *utf8Z, OSStatus err)
{
	#ifdef rDEBUG
		AlertID(utf8Z, err);
	#else
		LogErr(utf8Z, err);
	#endif
}

void			ReportErr(const SuperString& errTypeStr, OSStatus err, bool unixB, bool postB)
{
	UNREFERENCED_PARAMETER(postB);
	UNREFERENCED_PARAMETER(unixB);

	#if defined(_CFTEST_) && !defined(_CONSTRUCTOR_)
		LogErr(errTypeStr.utf8Z(), err);
	#else
		AlertID(errTypeStr.utf8Z(), err);
	#endif
}

#endif

CFStringRef			CFStrCreateWithCurAbsTime()
{
	CFAbsoluteTime		curTime(CFAbsoluteTimeGetCurrent());
	CFAbsoluteTime		milliSecondsF(curTime * 1000);
	UInt64				milliSeconds((UInt64)(milliSecondsF));
	char				bufAC[256];

	//	put LSB first, for windows, to avoid "short path collisions"	
	milliSeconds = CFSwapInt64(milliSeconds);
	sprintf(bufAC, "%llu", milliSeconds);

	return CFStringCreateWithC(bufAC, kCFStringEncodingASCII);
}

char*	strrstr(const char* stringZ, const char* findZ)
{
	bool		firstB = true, doneB = false;
	const char	*nextZ;
	
	do {
		if (firstB) {
			nextZ = strstr(stringZ, findZ);
		} else {
			nextZ = strstr(&stringZ[1], findZ);
		}
		
		doneB = nextZ == NULL;
		
		if (!doneB) {
			stringZ = nextZ;
		} else if (firstB) {
			stringZ = NULL;
		}
		
		firstB = false;		
	} while (!doneB);
	
	return const_cast<char *>(stringZ);
}

float			CStringToFloat(const char *numF)
{
	float	valF = 0;
	
	sscanf(numF, "%f", &valF);
	return valF;
}

double			CStringToDouble(const char *numF)
{
	double	valF = 0;
	
	sscanf(numF, "%lf", &valF);
	return valF;
}

UInt64			ParseBytes(SuperString str)
{
	UInt64			bytesLL = 0;
	SuperString		rhs;
	
	str.Split(" ", &rhs);
	
	bytesLL = str.GetAs_SInt32();
	
	if (rhs == "TB") {
		bytesLL *= kTeraByte;
		
	} else if (rhs == "GB") {
		bytesLL *= kGigaByte;
		
	} else if (rhs == "MB") {
		bytesLL *= kMegaByte;

	} else if (rhs == "K") {
		bytesLL *= kKiloByte;
	}

	return bytesLL;
}

SuperString		FormatBytes(UInt64 bytes)
{
	char		buf[1024];
	double		valF	= (double)bytes;
	const char	*labelZ	= "K";
	
	if (bytes < kKiloByte) {
		labelZ	= "b";
	} else {
		valF /= kKiloBytef;
		
		if (valF < kKiloByte) {
			labelZ	= "K";
		} else {
			valF /= kKiloBytef;

			if (valF < kKiloByte) {
				labelZ	= "MB";
			} else {
				valF /= kKiloBytef;

				if (valF < kKiloByte) {
					labelZ	= "GB";
				} else {
					valF /= kKiloBytef;

					labelZ	= "TB";
				}
			}
		}
	}
	
	sprintf(buf, "%.1f %s", valF, labelZ);
	SuperString		bufStr(buf);
	
	bufStr.Replace(".0", "");
	return bufStr;
}

/********************************************************/
bool		CFStringIsEmpty(CFStringRef nameRef)
{
	bool	emptyB = nameRef == NULL;

	if (!emptyB) {
		CFIndex		lengthL = CFStringGetLength(nameRef);
		
		emptyB = lengthL == 0;
	}
	
	return emptyB;
}

CFStringEncoding	s_file_encoding = kCFStringEncodingInvalidId;

bool				IsDefaultEncodingSet()
{
	return s_file_encoding != kCFStringEncodingInvalidId;
}

void				SetDefaultEncoding(CFStringEncoding encoding)
{
	if (s_file_encoding != encoding) {
		s_file_encoding = encoding;
	}
}

CFStringEncoding	GetSourceTextEncodingDefault()
{
	CFStringEncoding		encoding =
		#if _QT6_ && OPT_WINOS && _KJAMS_
			kCFStringEncodingWindowsLatin1;
		#else
			kCFStringEncodingMacRoman;
		#endif

	return encoding;
}

static CFStringEncoding	ValidateEncoding(CFStringEncoding encoding = kCFStringEncodingInvalidId)
{
	if (encoding == kCFStringEncodingInvalidId) {

		if (!IsDefaultEncodingSet()) {
			SetDefaultEncoding(GetSourceTextEncodingDefault());
			CCFLog(true)(CFSTR("$$$ Default encoding not set!"));	//	must be after to avoid recursion
		}

		encoding = s_file_encoding;
	}
	
	return encoding;
}

#ifndef _KJAMSX_
	class Asciify {
		public: void operator()(char &ch) {
			if (ch < 32 || ch > 126) ch = '?';
		}
	};
#endif

CFStringRef		CFCopyEmptyString()
{
	static CFStringRef		s_emptyStr(CFSTR(""));
		
	return (CFStringRef)CFRetainDebug(s_emptyStr);
}


CFStringRef		CFStringCreateWithC(
	const char *		bufZ, 
	CFStringEncoding	encoding)
{
	CFStringRef		cf = NULL;

	if (bufZ && bufZ[0]) {
		encoding = ValidateEncoding(encoding);
		
		cf = CFStringCreateWithCString(kCFAllocatorDefault, bufZ, encoding);
		if (!cf) cf = CFStringCreateWithCString(kCFAllocatorDefault, bufZ, kCFStringEncodingWindowsLatin1);
		if (!cf) cf = CFStringCreateWithCString(kCFAllocatorDefault, bufZ, kCFStringEncodingISOLatin1);
		if (!cf) cf = CFStringCreateWithCString(kCFAllocatorDefault, bufZ, kCFStringEncodingMacRoman);
		
		if (!cf) {
			#if OPT_KJMAC && !defined(_MIN_CF_)
				CharVec			newBuf;
				
				newBuf.assign(&bufZ[0], &bufZ[strlen(bufZ) + 1]);
				std::for_each(newBuf.begin(), newBuf.end(), Asciify());
				bufZ = &newBuf[0];

				static	CMutex_long		s_incdec;

				if (s_incdec.inc() == 1) {
					Logf("illegal string (asciified): <%s>, enc: %ld\n", bufZ, (long)encoding);
					SuperString		encRef(CFStringGetNameOfEncoding(encoding));
					SuperString		formatStr("Illegal String %s with encoding: [%s], the encoding is: %s\n");
					
					formatStr.ssprintf(
						NULL, 
						bufZ, 
						encRef.utf8Z(), 
						CFStringIsEncodingAvailable(encoding) ? "AVAILABLE" : "NOT available");
					
					MessageAlert(formatStr.utf8Z());
				}
				
				s_incdec.dec();
			#else
				CFDebugBreak();
			#endif
		}
	}

	if (cf) {
		CFRetainDebug(cf, false);
	} else {
		cf = CFCopyEmptyString();
	}
	
	return cf;
}

CFStringRef		CFStringCreateWithCu(
	const UTF8Char *	bufZ, 
	CFStringEncoding	encoding)
{
	return CFStringCreateWithC((const char *)bufZ, encoding);
}

ustring&		CopyCFStringToUString(CFStringRef str, ustring &result, CFStringEncoding encoding, bool externalB)
{
	result.clear();
	
	if (str) {
		CFRange						cfRange = CFStrGetRange(str);
		CFIndex						resultSize;
		CFIndex						numChars;
		
		encoding = ValidateEncoding(encoding);

#if 0
		#define						kBufSize		256
		UTF8Char					utf8Buf[kBufSize];

		result.reserve(cfRange.length * 8);

		while (cfRange.length > 0) {
			
			numChars = CFStringGetBytes(
				str, cfRange, encoding, '?', externalB, 
				&utf8Buf[0], kBufSize, &resultSize);
			
			if (numChars == 0) break;   // Failed to convert anything...
			
			result.append(&utf8Buf[0], &utf8Buf[resultSize]);
			
			cfRange.location	+= numChars;
			cfRange.length		-= numChars;
		}
#else
		numChars = CFStringGetBytes(
			str, cfRange, encoding, '?', externalB, 
			NULL, 0, &resultSize);

		if (numChars) {
			result.resize(resultSize, 0);
			
			CFStringGetBytes(
				str, cfRange, encoding, '?', externalB, 
				&result[0], result.size(), &resultSize);
		}
#endif
	}
	
	return result;
}

std::string		&CopyCFStringToStd(
	CFStringRef			str, 
	std::string			&stdstr, 
	CFStringEncoding	encoding)
{
	stdstr.clear();
	
	encoding = ValidateEncoding(encoding);
	
	if (str) {
		const char	*charZ = CFStringGetCStringPtr(str, encoding);
		
		if (charZ) {
			stdstr = charZ;
		} else {
			ustring		ustr;
			
			CopyCFStringToUString(str, ustr, encoding);
			stdstr.assign(ustr.begin(), ustr.end());
		}
	}
	
	return stdstr;
}

/*******************************/
SuperString		ULong_To_HexStr(UInt32 valueL)
{
	SuperString			str(ULong_To_Hex(valueL));
	
	str.ToUpper();
	str.prepend("0x");
	return str;
}

bool		OSType_IsNumber(OSType osType, bool assertB)
{
	bool	isNumberB = osType < '    ';
	
	if (isNumberB && assertB) {
		
		if (osType > kMaxOSTypeNumber) {
			SuperString		str(ULong_To_HexStr(osType));
			
			Logf("$$ osType is a number, but > 15: <%s>\n", str.utf8Z());
		}
		
		CF_ASSERT(osType <= kMaxOSTypeNumber);
	}
	
	return isNumberB;
}

bool		OSType_IsStringNumber(OSType osType)
{
	bool	isNumberB = osType >= '   0' && osType <= '   F';
	
	return isNumberB;
}

OSType		OSType_FromNumber(UInt32 valL)
{
	SuperString		str(ULong_To_Hex(valL));

	str.pop_front(7);
	str.prepend("   ");
	return CharToOSType(str.utf8Z(), false);
}

UInt32		OSType_ToNumber(OSType osType)
{
	OSTypeCharBuf	bufA;
	
	OSTypeToChar(osType, bufA);
	return Hex_To_ULong(bufA);
}

char		*OSTypeToChar(OSType osType, char *bufZ)
{
	if (OSType_IsNumber(osType)) {
		osType = OSType_FromNumber(osType);
	}
		
	osType = CFSwapInt32HostToBig(osType);
	*((OSType *)bufZ) = osType;
	
	bufZ[4] = 0;
	return bufZ;
}

typedef std::map<OSType, SuperString>		OSTypeStrMap;
OSTypeStrMap		s_ostypeStrMap;

SuperString		OSTypeToString(OSType osType)
{
	SuperString					str;
	OSTypeStrMap::iterator		it(s_ostypeStrMap.find(osType));
	
	if (it != s_ostypeStrMap.end()) {
		str = it->second;
		
	} else {
		if (osType == OSType_NONE) {
			str.Set("-");
		} else {
			OSTypeCharBuf	bufAC;
			
			str.Set(OSTypeToChar(osType, bufAC));
		}
		
		s_ostypeStrMap[osType] = str;
	}
	
	return str;
}

OSType		CharToOSType(const char *bufZ, bool convertB)
{
	OSType		osType = OSType_NONE;
	size_t		lenL = strlen(bufZ);
	
	if (lenL >= 4) {
		osType = *((OSType *)(&bufZ[lenL - 4]));
		osType = CFSwapInt32BigToHost(osType);

		if (convertB && OSType_IsStringNumber(osType)) {
			osType = OSType_ToNumber(osType);
		}
	}

	return osType;
}

SuperString&		SuperString::InsertSpaces(size_t everyNth)
{
	if (size() > everyNth) {
		UTF16Vec		vec, newVec;
		size_t			curLoopL = 0;
		
		Get(vec);
		
		for (
			UTF16Vec::iterator it = vec.begin();
			it != vec.end();
			++it
		) {
			UTF16Char& 	ch(*it);

			newVec.push_back(ch);
			
			if (++curLoopL == everyNth) {
				newVec.push_back(' ');
				curLoopL = 0;
			}
		}
		
		newVec.pop_back();
		Set(newVec);
	}
	
	return *this;
}


#if !defined(_QTServer_) && !defined(_HELPERTOOL_)
SuperString		UTF8HexToString(const char *hexZ)
{
	SuperString		hexStr(uc(hexZ));
	
	return hexStr.UnHexify();
}
#endif

#if defined(_KJAMS_) || _PaddleServer_

#if defined(_ROSE_)
	#include "md5.h"
#else
	#include "k_md5.h"
#endif

#define	MD5_DIGEST_LENGTH	16

static unsigned char *S_MD5(const unsigned char *d, unsigned long n, unsigned char *md)
{
	md5_state_t		context;
	int				valN((int)n);
	unsigned long	convertedN(valN);
	
	CF_ASSERT(n == convertedN);
	
	md5_init(&context);
	md5_append(&context, d, valN);
	
	#if defined(_ROSE_)
		md5_finish(&context, md);
	#else
		k_md5_finish(&context, md);
	#endif
	
	return md;
}
#endif	// _HELPERTOOL_

SuperString		SuperString::md5() const
{
#if defined(_KJAMS_) || _PaddleServer_ 
	UCharVec	digest(MD5_DIGEST_LENGTH);

	S_MD5(utf8().c_str(), utf8().size(), &digest[0]);
	return ::Hexify(digest);
#else
	CF_ASSERT(0);
	return SuperString();
#endif
}

/*************************************************************/

class	CAssign_CharToBits {
	BitVec&			i_bitVec;
	
	public:
	CAssign_CharToBits(BitVec& bitVec) : i_bitVec(bitVec) { }
	
	void	operator()(UInt8 curCh) {
		bool	curBit;
		
		loop (8) {
			curBit = (curCh & 0x80) != 0;
			
			i_bitVec.push_back(curBit);
			curCh <<= 1;
		}
	}
};

class	CAssign_BitsToChar {
	UCharVec&		i_vec;
	UInt8			i_curByte;
	short			i_curBitS;
	
	public:
	CAssign_BitsToChar(UCharVec& vec) : i_vec(vec), i_curByte(0), i_curBitS(0) { }
	
	void	operator()(bool bit) {
		i_curByte = (i_curByte << 1) | (int)bit;
		
		if (++i_curBitS == 8) {
			i_vec.push_back(i_curByte);
			i_curByte = 0;
			i_curBitS = 0;
		}
	}
};

static	void	RotateVec(UCharVec& vec, short rotS)
{
	BitVec		bitVec;
	
	bitVec.reserve(vec.size() * 8);
	std::for_each(vec.begin(), vec.end(), CAssign_CharToBits(bitVec));
	
	if (rotS > 0) {
		std::rotate(bitVec.begin(), bitVec.begin() + rotS, bitVec.end());
	} else {
		std::rotate(bitVec.begin(), bitVec.end() + rotS, bitVec.end());
	}
	
	vec.clear();
	vec.reserve(bitVec.size() / 8);
	std::for_each(bitVec.begin(), bitVec.end(), CAssign_BitsToChar(vec));
}

SuperString&	SuperString::Scramble(short rotateS)
{
	if (utf8().size()) {
		UCharVec			vec(utf8().begin(), utf8().end());
		
		if (rotateS) {
			RotateVec(vec, rotateS);
		}
		
		Set(::Hexify(vec));
	}
	return *this;
}

SuperString&	SuperString::UnScramble(short rotateS)
{
	if (utf8().size() && IsHexString()) {
		UCharVec		vec(::UnHexify(utf8()));
		
		if (rotateS) {
			RotateVec(vec, rotateS);
		}
		
		vec.push_back(0);
		
		Set(&vec[0]);
	}

	return *this;
}

SuperString		UTF8BytesToString(UInt32 bytesL)
{
	UInt32			bitEndianL[2];

	bitEndianL[0] = CFSwapInt32HostToBig(bytesL);
	bitEndianL[1] = 0;

	SuperString		str((UInt8*)&bitEndianL);
	
	return str;
}


class SS_ForEach_FindNonDigit {
	public: bool operator()(UTF8Char uch) {
		return !CFIsDigit((char)uch);
	}
};

bool	SuperString::IsNumeric() const
{
	bool		numericB = false;
	
	if (!empty()) {
		ustring&	ustr(const_cast<ustring&>(utf8()));
		
		numericB = std::find_if(ustr.begin(), ustr.end(), SS_ForEach_FindNonDigit()) == utf8().end();
	}
	
	return numericB;
}

#ifdef _HAS_QSTR_
QString		SuperString::q_str() const
{
	return QString::fromUtf8(utf8Z());
}

SuperString::SuperString(const QString& str)
{
	SetNULL();
	Set(uc(str.toStdString().c_str()));
}

SuperString::SuperString(const QByteArray& str)
{
	SetNULL();
	Set(uc(QString(str).toStdString().c_str()));
}
#endif

class SS_ForEach_Ascii {
	bool	i_include_highB;
	
	public: 
	SS_ForEach_Ascii(bool highB) : i_include_highB(highB) {}
	
	void operator()(char &ch) {
		if (ch < 32) ch = '?';
		
		if (i_include_highB && ch > 126) {
			ch = '?';
		}
	}
};

//	legacy implementation did NOT convert high values to ?
//	but obviously this should be done
SuperString		&SuperString::Ascii(bool including_high_valuesB)
{
	CharVec		charVec;
	
	charVec.assign(std().begin(), std().end());
	
	std::for_each(charVec.begin(), charVec.end(), SS_ForEach_Ascii(including_high_valuesB));
	charVec.push_back(0);
	Set(&charVec[0]);
	return *this;
}

SuperString		&SuperString::Alpha()
{
	CharVec			charVec;
	const ustring&	str(utf8());
	
	for (
		ustring::const_iterator it = str.begin();
		it != str.end();
		++it
	) {	
		unsigned char	ch(*it);

		if (isalpha(ch)) {
			charVec.push_back(ch);
		}
	}
	
	charVec.push_back(0);

	Set(&charVec[0]);
	return *this;
}

//	allows alpha, numeric, +/-, and "."
SuperString		&SuperString::AlphaNum()
{
	CharVec			charVec;
	const ustring&	str(utf8());
	
	for (
		ustring::const_iterator it = str.begin();
		it != str.end();
		++it
	) {	
		unsigned char	ch(*it);

		if (
			   isalnum(ch) 
			|| ch == '.'
			|| ch == '+'
			|| ch == '-'
		) {
			charVec.push_back(ch);
		}
	}
	
	charVec.push_back(0);

	Set(&charVec[0]);
	return *this;
}

void	SuperString::Set(const ww_string &wwStr)
{
	CCFString		myRef(CFStringCreateWithUTF32(wwStr.c_str(), wwStr.size()));
	
	Set(myRef);
}

SuperString::SuperString(const UTF32Char ch)
{
	UTF32Char	bufA[2];
	
	bufA[0] = ch;
	bufA[1] = 0;
	SetNULL();
	*this = SuperString(&bufA[0]);
}

SuperString::SuperString(const UTF32Char *strZ, size_t sizeL)
{
	const UTF32Char*		endZ = strZ;
	
	if (sizeL == static_cast<size_t>(-1)) {
		++sizeL;

		while (*endZ++ != 0) {
			++sizeL;
		}
	}

	CCFString		myRef(CFStringCreateWithUTF32(strZ, sizeL));

	SetNULL();
	Set(myRef);
}

void	SuperString::Set(const UTF16Vec &vec)
{
	UTF16Vec::const_iterator	it(std::find(vec.begin(), vec.end(), (UTF16Char)0));
	size_t						sizeL(std::distance(vec.begin(), it));
	CCFString					myRef(CFStringCreateWithCharacters(kCFAllocatorDefault, &vec[0], sizeL));
	
	Set(myRef);
}

void	SuperString::assign(const UTF8Char *begin_bytesP, const UTF8Char *end_bytesP, CFStringEncoding encoding)
{
	CCFString			myRef(CFStringCreateWithBytes(
		kCFAllocatorDefault, begin_bytesP, std::distance(begin_bytesP, end_bytesP), encoding, false));
		
	Set(myRef);
}

void	SuperString::Set(const UCharVec& byteVec, CFStringEncoding encoding)
{
	if (byteVec.empty()) {
		clear();
	} else {
		const UTF8Char *begin_bytesP	= &byteVec[0];
		const UTF8Char *end_bytesP		= &begin_bytesP[byteVec.size()];
		
		assign(begin_bytesP, end_bytesP, encoding);
	}
}

void	SuperString::Get(UTF16Vec &vec, bool forceBigEndianB) const
{
	if (forceBigEndianB) {
		delete i_uni;
		i_uni = NULL;
	}

	const UniString&	uni_str(uni(forceBigEndianB));
	
	vec.assign(uni_str.begin(), uni_str.end());
}

void	SuperString::Set(CFStringRef in_myRef, bool retainB)
{
	CFStringRef 	myRef(in_myRef);
	CFStringRef		oldRef = i_ref;
	
	{
		CCFString		emptyStr(CFCopyEmptyString());
		
		if (myRef == NULL) {
			myRef = emptyStr.ref();
			if (!retainB) {
				retainB = true;
			}
		}
		
		i_ref = myRef;
		
		if (retainB) {
			CFRetainDebug(i_ref);
		}
	}
	
	if (oldRef) {
		CFReleaseDebug(oldRef);
	}

	delete i_uni;
	i_uni	= NULL;
	
	delete i_std;
	i_std	= NULL;
	
	delete i_utf8;
	i_utf8	= NULL;

	delete i_utf32;
	i_utf32	= NULL;
	
	delete i_pstr;
	i_pstr = NULL;

	#if kDEBUG || _CFTEST_ || rDEBUG
		if (in_myRef && g_debugStringsB) {
			#if _QT_
				#if OPT_MACOS
					utf8();
				#else
					w_str();
				#endif
			#elif OPT_MACOS
				c_str();
			#else
				w_str();
			#endif
		}
	#endif
}

struct StringPair {
	const char	*beginZ;
	const char	*endZ;
};

static StringPair		s_stringPairA[] = {
	{	"CCNUM=",				"&"			},
	{	"ccnum=",				"&"			},
	{	"cc_number=", 			"&"			},
	{	"ccNum/",				"/cvv2"		},
	{	"CardNumber\":\"",		"\""		},	
	{	"CardNumber\"}",		"}"			},
	{	"password=",			"&submit"	},
	{	"CVV=",					"&"			},	
	{	"DeveloperKey\":\"",	"\""		},
	{	"DeveloperKey\"}",		"}"			},
	{	"Password\"}",			"}"			},
	{	"pass\":\"",			"\""		},
	{	"Password\":\"",		"\""		},
	{	"CVV2\"}",				"}"			},
	{	"CVV2\":\"",			"\""		},
	{	"?giftcode=",			"\n"		},
	{	"GIFTPACK, ", 			"\n"		},
	{	"AccountID=", 			"\n"		},
	{	"vendor_id=", 			"&"			},
	{	"vendor_auth_code=", 	"&"			},
	{	"session_vendor=",		";"			},
	{	"product_ids=", 		"&"			},
	{	"Set-Cookie\":", 		";"			},
	
};
#define		kStringPairVecSize		(sizeof(s_stringPairA) / sizeof(StringPair))

//	static
SuperString&			SuperString::ScrubSensitiveInfo()
{
	#ifdef kDEBUG
	//	return *this;
	#endif
	
	SuperString			postStr, cutStr;
	
	loop (kStringPairVecSize) {
		const char	*beginZ = s_stringPairA[_indexS].beginZ;
		
		if (Contains(beginZ)) {
			const char	*endZ = s_stringPairA[_indexS].endZ;

			Split(beginZ, &postStr);
			
			if (postStr.Contains(endZ)) {
				postStr.rSplit(endZ, &cutStr);

				append(beginZ);
				
				{
					UCharVec		charVec(16, 'x');
					
					charVec.push_back(0);
					
					SuperString		appendStr(&charVec[0]);
					
					if (_indexS <= 4) {
						if (cutStr.size() > 12) {
							
							if (cutStr.size() > 16) {
								SuperString		extractStr(cutStr.extract("nts = \"", "\""));
								
								if (extractStr.size() == 16) {
									cutStr = extractStr;
								}
							}
							
							appendStr.resize(12);
							cutStr.pop_front(cutStr.size() - 4);
							appendStr.append(cutStr);
						}
					}
					
					append(appendStr);
				}
				
				append(endZ);
				append(postStr);
			}
		}
	}
	
	return *this;
}

SuperString&	SuperString::Escape(SuperString leaveUnescaped, SuperString alwaysEscape)
{
	Set(CFURLCreateStringByAddingPercentEscapes(
		kCFAllocatorDefault, ref(), 
		leaveUnescaped.empty() ? NULL : leaveUnescaped.ref(),
		alwaysEscape.empty() ? NULL : alwaysEscape.ref(),
		kCFStringEncodingUTF8), false);

	return *this;
}

void	SuperString::UnEscape()
{
	CCFString		emptyStr(CFCopyEmptyString());

	Set(CFURLCreateStringByReplacingPercentEscapes(
		kCFAllocatorDefault, ref(), emptyStr), false);
}

SuperString::SuperString(CFURLRef urlRef)
{
	SetNULL();

	//	Percent-escape sequences in the orig urlRef are retained
	Set(CFURLGetString(urlRef));
}

CFURLRef		SuperString::CopyAs_URLRef(bool escapeB /* true */) const
{
	SuperString		str(*this);

	if (escapeB) {
		//	probably always want to escape? since creating a SuperString
		//	from a real CFURLRef leaves escape sequences intact

		//	but double escaping is bad, so if it already IS escaped
		//	definitely don't do it AGAIN!
		str.Escape();
	}

	return CFURLCreateWithString(kCFAllocatorDefault, str.ref(), NULL);
}

bool			SuperString::IsHexString()
{
	bool			is_hexB = true;
	const ustring	&str(utf8());
	
	for (
		ustring::const_iterator it = str.begin();
		it != str.end();
		++it
	) {
		unsigned char	ch(*it);

		if (!isxdigit(ch)) {
			is_hexB = false;
			break;
		}
	}
	
	return is_hexB;
}

SuperString		UTF32ByteToString(UInt32 byteL)
{
	UTF32Vec		vec;

	vec.push_back(byteL);
	vec.push_back(0);
	
	SuperString		str(&vec[0]);
	
	return str;
}


SuperString		SuperString::UnicodeHexToStr()
{
	UInt32			byteL(Hex_To_ULong(utf8Z() + 2));
	SuperString		outStr(UTF32ByteToString(byteL));

	return outStr;
}

bool	SuperString::GetUnicodeHexStr(SuperString *foundStrP)
{
	bool			foundB(false);
	const char		hexPrefixAC[] = "\\u";
	const char		*strZ(utf8Z());
	const char		*startZ(strstr(strZ, hexPrefixAC));
	
	if (startZ != NULL) {
		startZ += 2;
		foundStrP->clear();
		foundStrP->append(*startZ++);
		foundStrP->append(*startZ++);
		foundStrP->append(*startZ++);
		foundStrP->append(*startZ++);
		
		foundB = foundStrP->IsHexString();
		
		if (foundB) {
			foundStrP->prepend(hexPrefixAC);
		}
	}
	
	return foundB;
}

SuperStringReplaceRec		s_json_replacementsA[] = {
	{	"\\/",		"/"		},
	{	"\\\"",		"\""	}
};
#define		kReplacementSize		(sizeof(s_json_replacementsA) / sizeof(SuperStringReplaceRec))

void	SuperString::UnEscapeJSON()
{
	SuperString		uniHex, uni;
	
	while (GetUnicodeHexStr(&uniHex)) {
		Replace(uniHex, uniHex.UnicodeHexToStr());
	}
	
	if (strstr(utf8Z(), "\\") != NULL) {
		ReplaceTable(s_json_replacementsA, kReplacementSize);
	}
}

void	SuperString::EscapeJSON()
{
	ReplaceTable_Reverse(s_json_replacementsA, kReplacementSize);
}

bool	SuperString::IsJSON() const
{
	//	maybe check for closing brace / bracket?
	return StartsWith("{") || StartsWith("[");
}

bool	SuperString::IsURL(URLProtocolType protocolType) const
{
	bool			is_urlB(false);
	const char		*protocolZ = NULL;


	switch (protocolType) {

		default: {
			CF_ASSERT(protocolType > kURLProtocol_NONE && protocolType < kURLProtocol_NUMTYPES);
		} break;

		case kURLProtocol_HTTP:	protocolZ = kCFURLProtocol_HTTP; break;
		case kURLProtocol_FILE:	protocolZ = kCFURLProtocol_FILE; break;
		case kURLProtocol_RAM:	protocolZ = kCFURLProtocol_RAM; break;
	}

	if (protocolZ) {
		is_urlB = StartsWith(protocolZ);
	}

	return is_urlB;
}


void	SuperString::Set(const UInt8 *strZ, CFStringEncoding encoding)
{
	if (strZ) {
		if (encoding == kCFStringEncodingPercentEscapes) {
			Set(strZ, kCFStringEncodingUTF8);
			
			UnEscape();
			return;
		} else {
			CCFString	myRef(CFStringCreateWithCu(strZ, encoding));
			
			Set(myRef);
		}
		
	} else {
		clear();
	}
}

SuperString&		SuperString::FoldStrip(bool foldCaseB, bool stripB)
{
	bool								setB = false;
	ScCFReleaser<CFMutableStringRef>	stringRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, ref()));
	
	#if OPT_MACOS
	{
		if (stripB) {
			setB = CFStringTransform(stringRef, NULL, kCFStringTransformStripCombiningMarks, false);
		}
	}
	#else
		UNREFERENCED_PARAMETER(stripB);
	#endif

	if (foldCaseB && GetSystemVers() >= kMacOS_10_5) {
		CFOptionFlags		flagsL = kCFCompareCaseInsensitive | kCFCompareDiacriticInsensitive | kCFCompareForcedOrdering;
		
		CFStringFold(stringRef, flagsL, NULL);
		setB = true;
	}

	if (setB) {
		Set(stringRef);
	}
	
	return *this;
}

TextEncoding	CreateTextEncoding_Win(
	TextEncodingBase      encodingBase,
	TextEncodingVariant   encodingVariant,
	TextEncodingFormat    encodingFormat)
{
	return encodingBase | (encodingVariant << 16) | (encodingFormat << 26);
}

TextEncoding		EncodingFromVariant(TextEncodingVariant encodingVariant)
{
	TextEncoding		encoding = 0;
	
	#if OPT_MACOS
		encoding  = CreateTextEncoding(kCFStringEncodingUnicode, encodingVariant, kUnicodeUTF16Format);
	#else
		encoding  = CreateTextEncoding_Win(kCFStringEncodingUnicode, encodingVariant, kUnicodeUTF16Format);
	#endif
	
	return encoding;
}

static void		NormalizeString_HFSPlus(SuperString& io_string, TextEncodingVariant encodingVariant)
{
	SuperString			resultStr;
	CFStringRef			stringRef(io_string.ref());
	CFIndex				stringLength = stringRef ? CFStringGetLength(stringRef) : 0;
	
	if (stringLength) {
		TextEncoding		encoding(EncodingFromVariant(encodingVariant));
		UCharVec			resultBuf8;
		UCharVec			tempBuf8(256);
		CFRange				cfRange = { 0, stringLength };
		CFIndex				chars_consumedL = 0;	//	chars = 2 bytes each
		CFIndex				bytes_producedL = 0;	//	bytes, obviously 1 byte each
		const bool			representation_INTERNAL(false);
		const UInt8			lossy_NOT_ALLOWED(0);
		
		resultBuf8.reserve(cfRange.length * 2);
		
		while (cfRange.length > 0) {
			
			chars_consumedL = CFStringGetBytes(
				stringRef, cfRange, encoding,
				lossy_NOT_ALLOWED, representation_INTERNAL,
				&tempBuf8[0], tempBuf8.size(), &bytes_producedL);
			
			if (chars_consumedL == 0) break;   // Failed to convert anything...
			
			resultBuf8.insert(resultBuf8.end(), &tempBuf8[0], &tempBuf8[bytes_producedL]);
			
			cfRange.location	+= chars_consumedL;
			cfRange.length		-= chars_consumedL;
		}
		
		if (!resultBuf8.empty()) {
			io_string.assign(&*resultBuf8.begin(), &*resultBuf8.end(), kCFStringEncodingUTF16);
		}
	}
}


SuperString&		SuperString::Normalize(CFStringNormalizationForm form, bool foldCaseB, bool stripB)
{
	if (
		   form == kCFStringNormalizationFormKD_HFSPlus
		|| form == kCFStringNormalizationFormKC_HFSPlus
	) {
		NormalizeString_HFSPlus(*this, 
			form == kCFStringNormalizationFormKD_HFSPlus 
				? kUnicodeHFSPlusDecompVariant
				: kUnicodeHFSPlusCompVariant);
	} else {
		ScCFReleaser<CFMutableStringRef>	stringRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, ref()));
		
		CFStringNormalize(stringRef, form);
		Set(stringRef);
	}
	
	if (foldCaseB || stripB) {
		FoldStrip(foldCaseB, stripB);
	}

	return *this;
}
	
SuperString&		SuperString::Recover()
{
#if !defined(_CFTEST_) && !defined(_MIN_CF_)
	CCFString			sc(CFStrRecoverName(i_ref));
	
	Set(sc);
#endif

	return *this;
}

void		CFStrReplaceWith(CFMutableStringRef stringRef, CFStringRef replaceStr, CFStringRef withStr, bool allB)
{
	ScCFReleaser<CFArrayRef>	arrayRef;
	
	arrayRef.adopt(CFStringCreateArrayWithFindResults(
		NULL, stringRef, replaceStr, CFStrGetRange(stringRef), kCFCompareCaseInsensitive));
	
	if (arrayRef.Get()) {
		CFRange			*rangeRef;
		
		if (allB) {
			loop_reverse (CFArrayGetCount(arrayRef)) {
				rangeRef = (CFRange *)CFArrayGetValueAtIndex(arrayRef, _indexS);
				CFStringReplace(stringRef, *rangeRef, withStr);
			}
		} else {
			rangeRef = (CFRange *)CFArrayGetValueAtIndex(arrayRef, 0);
			CFStringReplace(stringRef, *rangeRef, withStr);
		}
	}
}

static CFOptionFlags		GetFlags_NormalizeStrings(
	bool			case_and_diacritic_sensitiveB, 
	SuperString&	str1, 
	SuperString&	str2, 
	CFOptionFlags	optionFlags = 0
) {
	optionFlags |= (CFOptionFlags)(0
		| kCFCompareNonliteral
		| kCFCompareLocalized);

	if (!case_and_diacritic_sensitiveB) {
		optionFlags |= kCFCompareCaseInsensitive;

		if (CFGetDiacriticInsensitive()) {
			static	bool	s_diacritic_compare_inittedB = false;
			static	bool	s_has_diacritic_insensitive_compareB;
			
			if (!s_diacritic_compare_inittedB) {
			
				#ifdef __WIN32__
					SuperString		str_e("e");
					SuperString		str_e_grave(GetSpecialChar(UTF_SpecialChar_e_ACUTE));
					
					s_has_diacritic_insensitive_compareB = ::CFStringCompare(
						str_e.ref(), str_e_grave.ref(), (CFOptionFlags)kCFCompareDiacriticInsensitive)
						== kCFCompareEqualTo;
				#else
					s_has_diacritic_insensitive_compareB = GetSystemVers() >= kMacOS_10_5;
				#endif
				
				s_diacritic_compare_inittedB = true;
			}
			
			if (s_has_diacritic_insensitive_compareB) {
				optionFlags |= (CFOptionFlags)(0
					| kCFCompareDiacriticInsensitive
					| kCFCompareWidthInsensitive);
			} else {
				str1.Normalize(kCFStringNormalizationFormD, false, true);
				str2.Normalize(kCFStringNormalizationFormD, false, true);
			}
		}
	}
	
	return optionFlags;
}

bool					CFStringContains(CFStringRef inRef, CFStringRef findRef, bool case_and_diacritic_sensitiveB)
{
	if (inRef == NULL || findRef == NULL) {
		return false;
	}
	
	SuperString		str1(inRef), str2(findRef);
	CFOptionFlags	optionFlags = 0;
	
	optionFlags = GetFlags_NormalizeStrings(case_and_diacritic_sensitiveB, str1, str2, optionFlags);
	
	return !!CFStringFindWithOptions(str1.ref(), str2.ref(), CFStrGetRange(str1.ref()), optionFlags, NULL);
}

CFComparisonResult		CFStringCompare(CFStringRef ref1, CFStringRef ref2, bool case_and_diacritic_sensitiveB)
{
	CFComparisonResult		compareResult = kCFCompareEqualTo;
	
	if ((ref1 == NULL) || (ref2 == NULL)) {
		
		if ((ref1 == NULL) ^ (ref2 == NULL)) {
			if (ref1) {
				compareResult = kCFCompareLessThan;
			} else {
				compareResult = kCFCompareGreaterThan;
			}
		}
	} else {
		SuperString		str1(ref1), str2(ref2);
		CFOptionFlags	optionFlags = kCFCompareNumerically;

		optionFlags = GetFlags_NormalizeStrings(case_and_diacritic_sensitiveB, str1, str2, optionFlags);
		compareResult = ::CFStringCompare(str1.ref(), str2.ref(), optionFlags);
	}
	
	return compareResult;
}

bool		CFStringEqual(CFStringRef str1, CFStringRef str2, bool case_and_diacritic_sensitiveB)
{
	return CFStringCompare(str1, str2, case_and_diacritic_sensitiveB) == kCFCompareEqualTo;
}

bool		CFStringLess(CFStringRef lhs, CFStringRef rhs, bool case_and_diacritic_sensitiveB)
{
	bool	lessB = CFStringCompare(lhs, rhs, case_and_diacritic_sensitiveB) == kCFCompareLessThan;
	
	return lessB;
}

bool	Sort_Str_LessThan::operator()(const SuperString& str1, const SuperString& str2) const
{
	bool	lessB = CFStringLess(str1.ref(), str2.ref(), i_case_and_diacritic_sensitiveB);
	
	#if _HAS_PROG_
	if (i_progB && gApp->GetCurScope()) {
		gApp->GetCurScope()->Inc(false, false, false);
	}
	#endif

	return lessB;
}

SuperString		operator+(const SuperString &lhs, SuperString rhs)
{
	SuperString		str(lhs);
	
	str.append(rhs);
	return str;
}

SuperString		operator+(const SuperString &lhs, const char *rhs)
{
	return operator+(lhs, SuperString(rhs));
}

class	CLowerfier {
	public: void operator()(char& ch) {
		ch = tolower(ch);
	}
};

void	OSType_ToLower(OSType *type)
{
	char	*testZ = (char *)type;
	
	std::for_each(&testZ[0], &testZ[4], CLowerfier());
}

SuperString&		SuperString::ToUpper()
{
	ScCFReleaser<CFMutableStringRef>	capsRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, i_ref));
	
	CFStringUppercase(capsRef, CFLocaleGetSystem());
	Set(capsRef);
	return *this;
}

SuperString&		SuperString::ToLower()
{
	ScCFReleaser<CFMutableStringRef>	capsRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, i_ref));
	
	CFStringLowercase(capsRef, CFLocaleGetSystem());
	Set(capsRef);
	return *this;
}

bool			SuperString::IsAllCaps()
{
	bool								capsB = false;
	
	if (!empty()) {
		ScCFReleaser<CFMutableStringRef>	capsRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, i_ref));
		
		CFStringUppercase(capsRef, CFLocaleGetSystem());
		capsB = CFStringEqual(i_ref, capsRef, true);
	}
	
	return capsB;
}

OSType		GetExtension(const char *strZ)
{
	OSType	ext;
	size_t	lenS = strlen(strZ);

	if (lenS >= 4) {
		CharVec		str(&strZ[lenS - 4], &strZ[lenS]); str.push_back(0);
		char 		*extZ = &str[0];
		char 		*dotZ = strchr(extZ, '.');
		
		if (dotZ == NULL) {
			dotZ = extZ;
		}
		
		for (char *curP = extZ; curP != dotZ; ++curP) {
			*curP = ' ';
		}
		
		ext = CharToOSType(extZ);
	} else {
		CharVec		str;	str.reserve(4 + lenS + 2);
		
		loop (4) {
			str.push_back(' ');
		}
		
		loop (lenS) {
			str.push_back(strZ[_indexS]);
		}

		str.push_back(0);

		ext = CharToOSType(&str[str.size() - 4]);
	}

	OSType_ToLower(&ext);
	return ext;
}

#ifdef _KJAMS_
static CMutex*		S_vsprintMutexP = NULL;

CMutex*		GetSprintfMutex()
{
	if (S_vsprintMutexP == NULL) {
		S_vsprintMutexP = new CMutex;

		if (S_vsprintMutexP->GetErr()) {
			fprintf(stdout, "error creating vsprintf mutex");
		}
	}
	
	return S_vsprintMutexP;
}
#endif

// --------------------------------------------------------------------------
char *			mt_vsnprintf(const char *formatZ, va_list &args)
{
	#ifdef _H_CThreads
		char			*g_sprintfBuf(CThreads::GetSprintfBuf());
	#else
		#define			kSprintfBufSize		(64 * kKiloByte)
		static char		g_sprintfBuf[kSprintfBufSize];
	#endif

	my_vsnprintf(g_sprintfBuf, kSprintfBufSize - 1, formatZ, args);

	g_sprintfBuf[kSprintfBufSize - 1] = 0;
	
	return g_sprintfBuf;
}

SuperString&		SuperString::append(double valueF, short precS)
{
	SuperString		formatStr, numStr;
	
	formatStr.ssprintf("%%.%dlf", (int)precS);
	numStr.ssprintf(formatStr.utf8Z(), valueF);
	Set(utf8() + numStr.utf8());
	return *this; 
}

// --------------------------------------------------------------------------

void	SuperString::Update_utf32() const
{
	if (!i_utf32) {
		ustring			str;

		i_utf32 = new ww_string;
		CF_ASSERT(i_utf32);
		
		CopyCFStringToUString(i_ref, str, kCFStringEncodingUTF32LE);

		size_t			bytesL(str.size());
		size_t			sizeL(bytesL >> 2);
		UTF32Char		*utf32A((UTF32Char *)str.c_str());
		UTF32Char		*beginZ(&utf32A[0]);
		UTF32Char		*endZ(beginZ + sizeL);
		
		i_utf32->assign(beginZ, endZ);
	}
}

SuperString		SuperString::ww_pop_front(size_t num_utf32_charsL)
{
	SuperString		popped;

	if (num_utf32_charsL) {
		size_t			sizeL(utf32().size());

		if (sizeL <= num_utf32_charsL) {
			popped = *this;
			clear();
		} else {
			{
				CCFString		myRef(CFStringCreateWithUTF32(
					i_utf32->c_str(), num_utf32_charsL));
	
				popped.Set(myRef);
			}

			{
				CCFString		myRef(CFStringCreateWithUTF32(
					i_utf32->c_str() + num_utf32_charsL, sizeL - num_utf32_charsL));

				Set(myRef);
			}
		}
	}

	return popped;
}

SuperString&	SuperString::ww_resize(size_t num_utf32_charsL)
{
	if (num_utf32_charsL <= 0) {
		clear();

	} else if (num_utf32_charsL > utf32().size()) {
		//	do nothing, can't resize a string to BIGGER than it was
	} else {
		CCFString		myRef(CFStringCreateWithUTF32(
			i_utf32->c_str(), num_utf32_charsL));

		Set(myRef);
	}

	return *this;
}

SuperString&	SuperString::ReplaceTable(SuperStringReplaceRec *recA, long sizeL, CFStringEncoding encoding)
{
	ScCFReleaser<CFMutableStringRef>	capsRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, i_ref));
	
	loop (sizeL) {
		SuperString		replaceStr(recA[_indexS].replaceZ);
		SuperString		withStr(recA[_indexS].withZ, encoding);
		
		CFStrReplaceWith(capsRef, replaceStr.ref(), withStr.ref());
	}
	
	Set(capsRef);
	return *this;
}

SuperString&	SuperString::ReplaceTable_Reverse(SuperStringReplaceRec *recA, long sizeL)
{
	ScCFReleaser<CFMutableStringRef>	capsRef(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, i_ref));
	
	loop (sizeL) {
		SuperString		withStr(recA[_indexS].replaceZ);
		SuperString		replaceStr(recA[_indexS].withZ);
		
		CFStrReplaceWith(capsRef, replaceStr.ref(), withStr.ref());
	}
	
	Set(capsRef);
	return *this;
}

SuperStringReplaceRec		s_ampReplacementsA[] = {
	{	"&lt;",		"<"		},
	{	"&gt;",		">"		},
	{	"&quot;",	"\""	},
	{	"&amp;",	"&"		},
	{	"&apos;",	"'"		},
};
#define		kAmpReplacementSize		(sizeof(s_ampReplacementsA) / sizeof(SuperStringReplaceRec))

SuperString		&SuperString::UnEscapeAmpersands()
{
	if (strstr(utf8Z(), "&") != NULL) {
		ReplaceTable(s_ampReplacementsA, kAmpReplacementSize);
	}
	return *this;
}

SuperString		Get_XML_KeyStr(const SuperString& keyStr, bool endB)
{
	SuperString		outStr;
	
	outStr.append("<");
	
	if (endB) {
		outStr.append("/");
	}
	
	outStr.append(keyStr);
	outStr.append(">");
	return outStr;
}

SuperString&	SuperString::append_xml(const SuperString& keyStr, const SuperString& valStr)
{
	append(Get_XML_KeyStr(keyStr, false));
	append(valStr);
	append(Get_XML_KeyStr(keyStr, true));
	return *this;
}

SuperString		SuperString::extract_xml(const char *keyZ, bool destructiveB)
{
	return extract(
		Get_XML_KeyStr(keyZ, false).utf8Z(), 
		Get_XML_KeyStr(keyZ, true).utf8Z(),
		destructiveB);
}

SuperString		SuperString::extract(const char *lhsZ, const char *rhsZ, bool destructiveB, bool keep_pre_lhsB)
{
	SuperString		result, temp, cur(*this), pre_lhs;
	
	if (cur.rSplit(lhsZ, &pre_lhs)) {
		SuperString		leftOver;
		
		if (cur.Split(rhsZ, &leftOver)) {
			result = cur;
			
			if (destructiveB) {
				
				if (keep_pre_lhsB) {
					leftOver = pre_lhs + leftOver;
				}
				
				Set(leftOver);
			}
		}
	}
	
	return result;
}

#if !defined(_HELPERTOOL_) && !defined(_QTServer_) && !defined(_CFTEST_)
boost::thread_specific_ptr<bool> 		g_split_insensitiveB;
#endif

SuperString		SuperString::extracti(const char *lhsZ, const char *rhsZ, bool destructiveB, bool keep_pre_lhsB)
{
#if !defined(_HELPERTOOL_) && !defined(_QTServer_) && !defined(_CFTEST_)
	bool				*insenseBP0 = g_split_insensitiveB.get();
	
	if (insenseBP0 == NULL) {
		insenseBP0 = new bool(false);
		g_split_insensitiveB.reset(insenseBP0);
	}
	
	ScSetReset<bool>	sc(insenseBP0, true);
#endif
	
	return extract(lhsZ, rhsZ, destructiveB, keep_pre_lhsB);
}

static inline const UTF8Char *cast_ucharp(const char *charZ)
{
    return (const UTF8Char *)charZ;
}

//	keep_countL is the number to keep, ie if it is 1, then keep one of splitC
SuperString&	SuperString::SplitAfterCount(char splitC, size_t keep_countL)
{
	UCharVec	bufAC(GetAs_UCharVec());

	bufAC.push_back(0);

	for (
		UCharVec::value_type *chP = &bufAC[0];
		*chP != 0;
		++chP
	) {
		if (*chP == splitC) {
			if (keep_countL-- == 0) {
				chP = 0;
				break;
			}
		}
	}

	Set(&bufAC[0]);
	return *this;
}

SuperString&	SuperString::Split(size_t splitL, SuperString& rhs)
{
	if (!empty() && splitL < size()) {
		const UTF8Char	*strA(utf8().c_str());
		const UTF8Char	*beginP(strA + splitL);
		const UTF8Char	*endP(strA + size());

		rhs.assign(beginP, endP);
		resize(splitL);
	}

	return *this;
}

bool	SuperString::Split(const char *splitZ, SuperString *rhsP0, bool from_endB)
{
	if (rhsP0) {
		rhsP0->clear();
	}
	
	const UTF8Char	*chZ;
	bool			insensitiveB = false;
	
#if !defined(_HELPERTOOL_) && !defined(_QTServer_) && !defined(_CFTEST_)
	bool			*insenseBP0 = g_split_insensitiveB.get();

	if (insenseBP0) {
		insensitiveB = *insenseBP0;
	}
#endif

	if (insensitiveB) {
		SuperString		foldedStr(*this);
		SuperString		foldedSplit(uc(splitZ));
		const char	 	*foldStrZ = NULL;
		
		foldedStr.FoldStrip(true, false);
		foldedSplit.FoldStrip(true, false);
		
		splitZ = foldedSplit.utf8Z();
		foldStrZ = foldedStr.utf8Z();
	
		if (from_endB) {
			if (splitZ[0] == 0) {
				chZ = cast_ucharp(&foldStrZ[strlen(foldStrZ)]);
			} else {
				chZ = cast_ucharp(strrstr(foldStrZ, splitZ));
			}
		} else {
			if (splitZ[0] == 0) {
				chZ = cast_ucharp(foldStrZ);
			} else {
				chZ = cast_ucharp(strstr(foldStrZ, splitZ));
			}
		}
		
		if (chZ) {
			size_t		distL(std::distance(cast_ucharp(foldStrZ), chZ));
			
			chZ = &utf8().c_str()[distL];
		
			if (rhsP0) {
				rhsP0->Set(chZ + strlen(splitZ));
			}

			assign(utf8().c_str(), chZ);
		}
	} else {
		const char	*strZ(utf8Z());
		
		if (from_endB) {
			if (splitZ[0] == 0) {
				chZ = cast_ucharp(&strZ[strlen(strZ)]);
			} else {
				chZ = cast_ucharp(strrstr(strZ, splitZ));
			}
		} else {
			if (splitZ[0] == 0) {
				chZ = cast_ucharp(strZ);
			} else {
				chZ = cast_ucharp(strstr(strZ, splitZ));
			}
		}
		
		if (chZ) {
			if (rhsP0) {
				rhsP0->Set(uc(chZ + strlen(splitZ)));
			}

			assign(utf8().c_str(), chZ);
		}
	}
	
	return chZ != NULL;
}

bool	SuperString::rSplit(const char *splitZ, SuperString *lhsP0, bool from_endB)
{
	SuperString		rhs;
	bool			splitB = Split(splitZ, &rhs, from_endB);
	
	if (lhsP0) {
		lhsP0->clear();
	}

	if (splitB) {
		if (lhsP0) {
			lhsP0->Set(*this);
		}
		
		Set(rhs);
	}
	
	return splitB;
}

SuperString&	SuperString::Reinterpret(CFStringEncoding fromEncoding, CFStringEncoding toEncoding)
{
	SuperString			convertedStr(*this);	
	
	//	CONVERT to fromEncoding
	convertedStr.Update_utf8(fromEncoding);

	//	REINTERPRET as toEncoding
	SuperString			interpretedStr(convertedStr.utf8Z(), toEncoding);
	
	Set(interpretedStr);
	return *this;
}

void			SuperString::Set_Ptr(Ptr ptrP)
{
	#if __LP64__
		Set((UInt64)ptrP);
	#else
		clear();
		append((long)ptrP);
	#endif
}

Ptr				SuperString::GetAs_Ptr() const
{
	Ptr		ptrP = 
	
	#if __LP64__
		reinterpret_cast<Ptr>(GetAs_UInt64());
	#else
		reinterpret_cast<Ptr>(GetAs_SInt32());
	#endif
	
	return ptrP;
}

void				SuperString::Set(SInt64 valueL)
{
	SuperString		numStr;
	
	numStr.ssprintf("%lld", valueL);
	Set(numStr);
}

SInt64				SuperString::GetAs_SInt64() const
{
	SInt64			valLL(0);
	
	sscanf(utf8Z(), "%lld", &valLL);
	return valLL;
}

void			SuperString::Set(UInt64 valueL)
{
	SuperString		numStr;
	
	numStr.ssprintf("%llu", valueL);
	Set(numStr);
}

UInt64			SuperString::GetAs_UInt64() const
{
	UInt64			valLL(0);
	
	sscanf(utf8Z(), "%llu", &valLL);
	return valLL;
}


UInt32				SuperString::GetAs_Hash() const
{
	SuperString		str(*this);
	UInt32			hashL = 0;
	SuperString		curStr;

	while (!str.empty()) {
		curStr = str.pop_front(4);
		while (curStr.size() < 4) {
			curStr.append(" ");
		}

		hashL ^= curStr.GetAs_OSType();
	}

	return hashL;
}

UInt64				SuperString::GetAs_Hash64() const
{
	UInt64			hashLL = 0;
	SuperString		str2, str1(md5());

	str1.Split(str1.size() >> 1, str2);

	hashLL = ((UInt64)str1.GetAs_Hash() << 32) + str2.GetAs_Hash();

	return hashLL;
}

OSType				SuperString::GetAs_OSType(bool justifyB) const
{
	OSType			osType;
	const char		*strZ;
	SuperString		str;
	
	if (justifyB) {
		str.Set(*this);

		while (str.size() < 4) {
			str.append(" ");
		}
		
		str.resize(4);
		strZ = str.utf8Z();
	} else {
		strZ = utf8Z();
	}
	
	osType = CharToOSType(strZ);
	return osType;
}

static CFLocaleRef			CFLocaleCreateDefaultEnglishUS()
{
	return CFLocaleCreate(kCFAllocatorDefault, CFSTR("en_US"));
}

//	http://userguide.icu-project.org/formatparse/datetime
static const char*	GetFormatStr(SS_TimeType timeType)
{
	#define		kTimeFormat_SHORT			"EEE, d MMM y H:mm:ss z"		//	Thu, 23 Jul 2009 15:15:21 GMT
	#define		kTimeFormat_LONG			"MMMM d, y H:mm:ss a z"			//	August 29, 2008 15:36:59 PM PDT
	#define		kTimeFormat_LONG_12			"MMMM d, y h:mm:ss a z"			//	August 29, 2008 03:36:59 PM PDT
	#define		kTimeFormat_LONG_PRETTY		"eeee, MMMM d, y - h:mm a z"	//	Thursday, March 29, 2012 - 3:31 PM PDT
	#define		kTimeFormat_LOG				"y-MM-dd H:mm:ss.SSS Z"			//	2009-01-24 18:20:16 or 2009-11-24 20:00:47.586 -0800, or kTimeFormat_JSON
	#define		kTimeFormat_SHORT_TIME_12	"h:mm a"						//	3:23 PM
	#define		kTimeFormat_SHORT_TIME_24	"H:mm"							//	15:23
	#define		kTimeFormat_SHORT_CDZ		"M/dd/y z"						//	5/13/2009 PT
	#define		kTimeFormat_SHORT_CDO		"M/dd/y"						//	5/13/2009
	#define		kTimeFormat_SHORT_YMD		"y/M/dd"						//	2012/09/01
	#define		kTimeFormat_TS_HELSINKI		"yyyyMMddhhmmssSSS"				//	"20120401182543234"	helsinki time
	#define		kTimeFormat_LOG_HELSINKI	"yyyy-MM-dd hh:mm:ss Z"			//	"2009-01-24 18:20:16 -0300" helsinki time (hacked)
	#define		kTimeFormat_JSON			"y-MM-dd'T'HH:mm:ss.SSS'Z'"		//	"2009-11-24T18:20:16Z"
//	#define		kTimeFormat_SHORT_12		"EEE, d MMM y h:mm:ss z"		//	Thu, 23 Jul 2009 3:15:21 PM GMT

	
	const		char*					strZ = "";
	
	switch (timeType) {
		case SS_Time_SHORT:						strZ = kTimeFormat_SHORT;			break;
		case SS_Time_LONG:						strZ = kTimeFormat_LONG;			break;
		case SS_Time_LONG_12:					strZ = kTimeFormat_LONG_12;			break;
		case SS_Time_LONG_PRETTY:				strZ = kTimeFormat_LONG_PRETTY;		break;
		case SS_Time_LOG:						strZ = kTimeFormat_LOG;				break;
		case SS_Time_SHORT_12:					strZ = kTimeFormat_SHORT_TIME_12;	break;
		case SS_Time_SHORT_24:					strZ = kTimeFormat_SHORT_TIME_24;	break;
		case SS_Time_SHORT_J:					CF_ASSERT(0); break;
		case SS_Time_COMPACT_DATE_TZ:			strZ = kTimeFormat_SHORT_TIME_12;	break;
		case SS_Time_COMPACT_DATE_ONLY: 		strZ = kTimeFormat_SHORT_CDO;		break;
		case SS_Time_COMPACT_DATE_REVERSE:		strZ = kTimeFormat_SHORT_YMD;		break;
		case SS_Time_TIMESTAMP_HELSINKI:		strZ = kTimeFormat_TS_HELSINKI;		break;
		case SS_Time_LOG_HELSINKI:				strZ = kTimeFormat_LOG_HELSINKI;	break;
		case SS_Time_JSON:						strZ = kTimeFormat_JSON;			break;

		default: {
			CF_ASSERT(0);
			break;
		}
	}
	
	return strZ;
}

bool	SuperString::ConformsToDateFormat(SS_TimeType timeType)
{
	bool	conformsB(true);

	switch (timeType) {

		default: {
			CF_ASSERT(0);
		} break;

		case SS_Time_DOT_NET: {
			conformsB = StartsWith("/Date(") && EndsWith(")/");
		} break;

		case SS_Time_JSON: {
			int		numA[] = {0,1,2,3,5,6,8,9,11,12,14,15,17,18};
			#define	kNumArraySize	(sizeof(numA) / sizeof(int))

			loop (kNumArraySize) {
				int		curI(numA[_indexS]);

				if (!CFIsDigit(GetIndChar(curI))) {
					conformsB = false;
				}
			}

			conformsB &= GetIndChar(4) == '-';
			conformsB &= GetIndChar(7) == '-';
			conformsB &= GetIndChar(10) == 'T';
			conformsB &= GetIndChar(13) == ':';
			conformsB &= GetIndChar(16) == ':';
			conformsB &= GetIndCharR() == 'Z';
		} break;
	}

	return conformsB;
}

typedef std::vector<SS_TimeType>	SS_TimeTypeVec;

SS_TimeType		SuperString::GetTimeType()
{
	SS_TimeType		timeType(SS_Time_NONE);
	SS_TimeTypeVec	supportedTypes;

	supportedTypes.push_back(SS_Time_DOT_NET);
	supportedTypes.push_back(SS_Time_JSON);

	for (SS_TimeTypeVec::iterator it(supportedTypes.begin()); it != supportedTypes.end(); ++it) {
		SS_TimeType curType(*it);

		if (ConformsToDateFormat(curType)) {
			timeType = curType;
			break;
		}
	}

	return timeType;
}

/************************/
static SuperString		CFTimeZoneGetHelsinkiOffsetStr()
{			
	SuperString		helsinkiTimeStr;
	
	helsinkiTimeStr.Set(CFAbsoluteTimeGetCurrent(), SS_Time_LOG_HELSINKI);
	helsinkiTimeStr.rSplit(" ", NULL, true);
	helsinkiTimeStr.prepend(" ");
	return helsinkiTimeStr;
}

static CFTimeZoneRef	CFTimeZoneCreateHelsinki()
{
	CFTimeZoneRef		helsinkiTz(CFTimeZoneCreateWithName(kCFAllocatorDefault, CFSTR("Europe/Helsinki"), false));

	if (helsinkiTz == NULL) {
		CFAbsoluteTime		absT(CFAbsoluteTimeGetCurrent());
		CCFTimeZone			curTz(CFTimeZoneCopyCurrent_Mutex(0, false));
		bool				is_dstB(!!CFTimeZoneIsDaylightSavingTime(curTz, absT));
		CFTimeInterval		gmt_plus_2_intervalF((2 + is_dstB) * kEventDurationHour);

		helsinkiTz = CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, gmt_plus_2_intervalF);
	}

	return helsinkiTz;
}

/************************/
static CFDateFormatterRef		CFDateFormatterCreate_JSON()
{
	CCFTimeZone				tz(CFTimeZoneCopyGMT());
	SuperString				formatStr(GetFormatStr(SS_Time_JSON));
	CCFDateFormatter		formatterRef(CFDateFormatterCreate(
		kCFAllocatorDefault, CFLocaleGetSystem(), 
		kCFDateFormatterLongStyle, kCFDateFormatterLongStyle));
	
	CFDateFormatterSetFormat(formatterRef, formatStr.ref());
	CFDateFormatterSetProperty(formatterRef, kCFDateFormatterTimeZone, tz);
	return formatterRef.transfer();			 
}

SuperString&				SuperString::Set(
	CFAbsoluteTime	absT, 
	SS_TimeType		timeType, 
	CFTimeInterval	epochT,
	CFTimeZoneRef	timeZoneRef)
{
	CCFDateFormatter	formatterRef;
	CCFTimeZone			localTz;
	bool				needs_time_zoneB(true);
	
	if (timeZoneRef) {
		localTz.SetAndRetain(timeZoneRef);
	}

	//	currently not supported
	CF_ASSERT(epochT == 0);
	
	switch (timeType) {

		case SS_Time_DOT_NET: {
			//	always in UTC
			CF_ASSERT(timeZoneRef == NULL);
			
			absT += kCFAbsoluteTimeIntervalSince1970;
			absT /= kEventDurationMillisecond;
			clear();
			append(absT, 0);
			append("+0000");

			prepend("/Date(");
			append(")/");
			
			// $$$ early return! ick!
			return *this;
		} break;

		case SS_Time_SIZE_0:	//	"2/21/18"
		case SS_Time_SIZE_1:	//	"2/21/18 6:39 AM"
		case SS_Time_SIZE_2:	//	"Feb 21, 2018 6:39 AM"
		case SS_Time_SIZE_3:	//	"February 21, 2018 6:39 AM"
		case SS_Time_SIZE_4: {	//	"Wednesday, February 21, 2018 6:39 AM"
			CFDateFormatterStyle		dateFormat(kCFDateFormatterNoStyle);
			CFDateFormatterStyle		timeFormat(kCFDateFormatterNoStyle);

			switch (timeType) {

				default: break;

				case SS_Time_SIZE_0: {	//	"2/21/18"
					dateFormat = kCFDateFormatterShortStyle;
				} break;

				case SS_Time_SIZE_1: {	//	"2/21/18 6:39 AM"
					dateFormat = kCFDateFormatterShortStyle;
					timeFormat = kCFDateFormatterShortStyle;
				} break;

				case SS_Time_SIZE_2: {	//	"Feb 21, 2018 6:39 AM"
					dateFormat = kCFDateFormatterMediumStyle;
					timeFormat = kCFDateFormatterShortStyle;
				} break;

				case SS_Time_SIZE_3: {	//	"February 21, 2018 6:39 AM"
					dateFormat = kCFDateFormatterLongStyle;
					timeFormat = kCFDateFormatterShortStyle;
				} break;

				case SS_Time_SIZE_4: {	//	"Wednesday, February 21, 2018 6:39 AM"
					dateFormat = kCFDateFormatterFullStyle;
					timeFormat = kCFDateFormatterShortStyle;
				} break;
			}

			CCFLocale			localeRef(CFLocaleCopyCurrent_Mutex());
			
			formatterRef.adopt(CFDateFormatterCreate(
				kCFAllocatorDefault, localeRef, dateFormat, timeFormat));
		} break;

		case SS_Time_JSON: {
			formatterRef.adopt(CFDateFormatterCreate_JSON());
			needs_time_zoneB = false;
		} break;

		default: {
			CCFLocale			localeRef;

			if (
				   timeType == SS_Time_LONG_PRETTY
				|| timeType == SS_Time_SYSTEM_LONG
				|| timeType == SS_Time_SHORT_12
				|| timeType == SS_Time_SHORT_24
				|| timeType == SS_Time_SHORT_J
			) {
				localeRef.adopt(CFLocaleCopyCurrent_Mutex());
			} else {
				localeRef.adopt(CFLocaleCreateDefaultEnglishUS());
			}

		//	SuperString							localeStr(CFLocaleGetIdentifier(localeRef));
		//	CCFLog(true)(localeStr.ref());

			CFDateFormatterStyle		formatStyle(timeType == SS_Time_SYSTEM_LONG
				? kCFDateFormatterLongStyle
				: kCFDateFormatterFullStyle);

			formatterRef.adopt(CFDateFormatterCreate(
				kCFAllocatorDefault, localeRef, formatStyle, formatStyle));

			if (timeType != SS_Time_SYSTEM_LONG) {
			
				if (timeType == SS_Time_SHORT_J) {
				
					if (CFDateFormatterUses24HourTimeDiaplay()) {
						timeType = SS_Time_SHORT_24;
					} else {
						timeType = SS_Time_SHORT_12;
					}
				}
			
				SuperString			formatStr(GetFormatStr(timeType));

				CFDateFormatterSetFormat(formatterRef, formatStr.ref());

				if (timeType == SS_Time_LOG_HELSINKI || timeType == SS_Time_TIMESTAMP_HELSINKI) {
					localTz.SetAndRetain(CFTimeZoneCreateHelsinki());
				}
			}

			CFDateFormatterSetProperty(formatterRef, kCFDateFormatterIsLenient, kCFBooleanTrue);
		} break;
	}

	if (needs_time_zoneB) {
	
		if (localTz.ref() == NULL) {
			localTz.adopt(CFTimeZoneCopyCurrent_Mutex(absT));

		}
		
		CF_ASSERT(formatterRef.ref());
		
		CFDateFormatterSetProperty(
			formatterRef, kCFDateFormatterTimeZone, localTz.ref());
	}

	Set(CFDateFormatterCreateStringWithAbsoluteTime(
		kCFAllocatorDefault, formatterRef, absT), false);

	return *this;
}


/*
	abs time is supposed to be in GMT
	however if you pass in a time labeled GMT
	it will return an ABS time in local time zone
	
	but then if you SET the string using an ABS time, it assumes it is 
	in the local time zone, not in the GMT
	
	both are wrong but cancel each other out if you round trip
	so it seems like no bug.  but there is a bug, in both
*/

CFAbsoluteTime		SuperString::GetAs_CFAbsoluteTime(
	SS_TimeType		timeType, 
	CFTimeInterval	epochT,
	CFTimeZoneRef	timeZoneRef) const
{
	CFAbsoluteTime		absT = 0;
	
	if (timeType == SS_Time_NAKED) {
		absT = GetAs_Double() - epochT;
		CF_ASSERT(timeZoneRef == NULL);	//	always GMT
		
	} else if (timeType == SS_Time_JSON) {
		CCFDateFormatter	formatterRef(CFDateFormatterCreate_JSON());

		CFDateFormatterGetAbsoluteTimeFromString(formatterRef, ref(), NULL, &absT);
		CF_ASSERT(timeZoneRef == NULL);	//	always GMT
		
	} else if (timeType == SS_Time_DOT_NET) {
		SuperString		dateStr(*this);
		const char		*s_dateStartZ = "/Date(";
		
		if (!dateStr.StartsWith(s_dateStartZ)) {
			Logf("Should: %s\n", s_dateStartZ);
			Logf("Actual: %s\n", dateStr.utf8Z());
			CF_ASSERT(0);
		}
		
		dateStr.rSplit("(");
		dateStr.Split("+");
		absT = dateStr.GetAs_Double();
		absT *= kEventDurationMillisecond;
		absT -= epochT;
		CF_ASSERT(timeZoneRef == NULL);	//	always GMT
		
	} else {
		CCFLocale				localeRef(CFLocaleCreateDefaultEnglishUS());
		CCFDateFormatter		formatterRef(CFDateFormatterCreate(
			kCFAllocatorDefault, localeRef, 
			kCFDateFormatterFullStyle, kCFDateFormatterFullStyle));
		SuperString				formatStr(GetFormatStr(timeType));
		SuperString				timeStr(*this);

		if (timeType == SS_Time_LOG && !Contains(".")) {

			if (timeStr.Contains("T") && timeStr.GetIndCharR() == 'Z') {
				timeStr.Replace("T", " ");
				timeStr.pop_back();
				timeStr.append(".000 -0000");
			} else {
				CFDateFormatterSetFormat(formatterRef, CFSTR("Z"));
				
				SuperString			tzStr(CFDateFormatterCreateStringWithAbsoluteTime(
					kCFAllocatorDefault, formatterRef, CFAbsoluteTimeGetCurrent()));

				tzStr.prepend(".000 ");
				timeStr.append(tzStr);
			}
			
			CF_ASSERT(timeZoneRef == NULL);	//	always GMT
			
		} else if (timeType == SS_Time_LOG_HELSINKI) {
			SuperString		tempStr(timeStr);
			
			tempStr.rSplit(" ", NULL, true);

			if (tempStr.GetIndChar() != '+') {
				timeStr.append(CFTimeZoneGetHelsinkiOffsetStr());
			}
			CF_ASSERT(timeZoneRef == NULL);	//	always GMT
			
		} else if (timeType == SS_Time_TIMESTAMP_HELSINKI) {
			formatStr.append(" Z");
			timeStr.append(CFTimeZoneGetHelsinkiOffsetStr());
			CF_ASSERT(timeZoneRef == NULL);	//	always GMT
		}

		CFDateFormatterSetProperty(formatterRef, kCFDateFormatterIsLenient, kCFBooleanTrue);
		CFDateFormatterSetFormat(formatterRef, formatStr.ref());
		
		{
			CCFTimeZone				localTz;
			
			if (timeZoneRef) {
				localTz.SetAndRetain(timeZoneRef);
			} else {
				localTz.adopt(CFTimeZoneCopyCurrent_Mutex());
			}

			CFDateFormatterSetProperty(formatterRef, kCFDateFormatterTimeZone, localTz);
			CFDateFormatterGetAbsoluteTimeFromString(formatterRef, timeStr.ref(), NULL, &absT);
		}
	}
	
	return absT;
}

//	formatZ0 is in MacRoman!
//	%s args are in utf8!
//  %@ string args are, well CFStringRef's, obviously
//	so you can have %s's or %@'s but not both!
SuperString&	SuperString::ssprintf(const char *formatZ0, ...)
{
	va_list			args;
	SuperString		formatStr;
	
	if (formatZ0) {
		formatStr.Set(formatZ0);
	} else {
		formatStr.Set(ref());
	}
	
	va_start(args, formatZ0);

	if (formatStr.Contains("%@") || formatStr.Contains("$@")) {
		CF_ASSERT(!formatStr.Contains("%s"));
		CF_ASSERT(!formatStr.Contains("$s"));

		#if OPT_WINOS && _YAAF_
			CCFDictionary		optionsDict;

			optionsDict.SetValue_Ref(kCFStringPrintfEncoding, (SInt32)kCFStringEncodingUTF8);
		#else
			CFDictionaryRef		optionsDict = NULL;
		#endif

		Set(CFStringCreateWithFormatAndArguments(
			kCFAllocatorDefault, 
			optionsDict,
			formatStr.ref(), 
			args), false);
	} else {
		char		*sprintfBuf = mt_vsnprintf(formatStr.utf8Z(), args);
		
		Set(uc(sprintfBuf));
	}

	va_end(args);
	
	return *this;
}

SuperString	&		SuperString::pop_back(size_t numCharsL)
{
	ustring		ustr(utf8());
	
	if (!ustr.empty()) {
		ustr.resize(ustr.size() - numCharsL);
		Set(ustr.c_str());
	}
	
	return *this;
}

void				SuperString::CopyAs_wchar(wchar_t *bufA, size_t buf_sizeL) const
{
	if (buf_sizeL) {
		const UniString&	uniStr(uni());
		const UniChar		*endP;
		
		if (uniStr.size() >= buf_sizeL) {
			endP = uniStr.begin() + buf_sizeL - 1;
		} else {
			endP = uniStr.end();
		}

		std::copy(uniStr.begin(), endP, &bufA[0]);
		bufA[std::distance(uniStr.begin(), endP)] = 0;
	}
}

short	SuperString::count_match_ww(const SuperString& matchStr)
{
	short					countS = 0;
	ww_string				ww_str(utf32());
	ww_string::size_type	pos = 0;
	
	do {
		pos = ww_str.find(matchStr.utf32(), pos);
		if (pos != ww_string::npos) {
			++countS;
			++pos;
		}
	} while (pos != ww_string::npos);
	
	return countS;
}
	
SuperString		SuperString::GetSection_ww(size_t beginL, size_t endL)
{
	ww_string		wwStr;	wwStr.assign(&utf32()[beginL], &utf32()[endL]);
	SuperString		str;	str.Set(wwStr);
	
	return str;
}

void			SuperString::DeleteSection_ww(size_t beginL, size_t endL)
{
	ww_string		wwStr(utf32());
	
	wwStr.erase(beginL, endL - beginL);
	Set(wwStr);
}

void			SuperString::InsertSection_ww(size_t beginL, const SuperString& str)
{
	ww_string		wwStr(utf32());
	
	if (beginL <= wwStr.size()) {
		wwStr.insert(wwStr.begin() + beginL, str.utf32().begin(), str.utf32().end());
		Set(wwStr);
	} else {
		CF_ASSERT(beginL <= wwStr.size());
	}
}

SuperString	&		SuperString::pop_back_ww(size_t numCharsL)
{
	size_t		curSizeL = ww_size();

	curSizeL -= numCharsL;
	if (curSizeL <= 0) {
		clear();
	} else {
		ww_resize(curSizeL - numCharsL);
	}

	return *this;
}

#ifdef __WIN32__

	#if defined(__MWERKS__)
		class CIsZero {
			public: bool operator()(const wchar_t& ch) {
				return ch == 0;
			}
		};

		static size_t	wcslen(const wchar_t *wcharZ) {
			const wchar_t*		it(std::find_if(&wcharZ[0], &wcharZ[512], CIsZero()));
			
			return std::distance(&wcharZ[0], it);
		}
	#endif

	SuperString::SuperString(const wchar_t *wcharZ)
	{
		SetNULL();

		if (wcharZ && wcharZ[0]) {
			UTF16Vec	vec((UTF16Char *)&wcharZ[0], (UTF16Char *)&wcharZ[wcslen(wcharZ)]);

			Set(vec);
		}
	}
#endif

void	SuperString::Set_p(ConstStr255Param strZ, CFStringEncoding encoding)
{
	UCharVec			charVec;
	ConstStr255Param	beginZ(&strZ[1]);
	ConstStr255Param	endZ(&strZ[strZ[0] + 1]);
	
	charVec.assign(beginZ, endZ);
	charVec.push_back(0);
	Set(&charVec[0], encoding);
}

OSType&		SuperString::pop_ext(OSType *extP) const {
	*extP = GetExtension(c_str());
	return *extP;
}

void	SuperString::LogCount(const char *nameZ)
{
	#ifdef kDEBUG
		S_LogCount(ref(), nameZ);
	#else
		UNREFERENCED_PARAMETER(nameZ);
	#endif
}

void	SuperString::Set_CFType(CFTypeRef cfType)
{
	CFTypeEnum			cfTypeEnum(CFGetCFType(cfType));
	
	switch (cfTypeEnum) {

		case CFType_STRING: {
			Set((CFStringRef)cfType);
			break;
		}

		case CFType_DICT: {
			CCFDictionary		dict((CFDictionaryRef)cfType, true);		

			Set("CFDictionary:\n");
			append(dict.GetJSON(kJSON_Indented));
			UnEscapeJSON();
			break;
		}
		
		case CFType_ARRAY: {
			CCFArray			array((CFArrayRef)cfType, true);
		
			Set("CFArray:\n");
			append(array.GetJSON(kJSON_Indented));
			UnEscapeJSON();
			break;
		}
		
		case CFType_DATE: {
			CFDateRef			dateRef((CFDateRef)cfType);
			CFAbsoluteTime		absT(CFDateGetAbsoluteTime(dateRef));

			Set(absT, SS_Time_LONG_PRETTY);
			prepend("CFDate:\n");
			break;
		}
		
		default: {
			SuperString			descIDStr(CFCopyTypeIDDescription(CFGetTypeID(cfType)), false);
			SuperString			descStr(CFCopyDescription(cfType), false);
			SuperString			logStr("%s:\n%s");
			
			logStr.ssprintf(NULL, descIDStr.utf8Z(), descStr.utf8Z());
			Set(logStr);
			break;
		}
	}
}

#if _QT_ && !_JUST_CFTEST_
// static
SuperString		SuperString::GetBestPrettyDate(
	CFDateRef		dateRef,
	const Rect&		frameR,
	QFontMetricsF&	metricsF)
{
	SuperString			dateStr;
	CFAbsoluteTime		absT(CFDateGetAbsoluteTime(dateRef));
	int					widthI(QD_RectWidth(frameR));
	
	loop_range (SS_Time_SIZE_begin, SS_Time_SIZE_end) {

		dateStr.Set(absT, static_cast<SS_TimeType>(_indexS));

		if (metricsF.horizontalAdvance(dateStr) <= widthI) {
			break;
		}
	}

	return dateStr;
}
#endif

SuperString&	SuperString::InsertAt(size_t posL, const SuperString& str, bool from_endB)
{
	if (posL == 0) {
		if (from_endB) {
			append(str);
		} else {
			prepend(str);
		}
	} else {
		if (from_endB) {
			ustring		self(utf8());
			
			self.insert(self.end() - posL, str.utf8().begin(), str.utf8().end());
			Set(self);
		} else {
			CF_ASSERT("Unimplemented: SuperString:insertAt" == NULL);
		}
	}
	
	return *this;
}

SuperString		&SuperString::Localize(bool recurse_okayB)
{
	#if !defined(_KJAMS_) && (defined(_JUST_CFTEST_) || !_QT_)
		UNREFERENCED_PARAMETER(recurse_okayB);
	#else
	bool						menuB = StartsWith("Menu") || StartsWith("SubMenu");
	bool						dlgB = StartsWith("dlg");
	SuperString					asciiStr(ref());	if (menuB || dlgB) asciiStr.Ascii(true);
	CFStringRef					asciiKeyRef = asciiStr.ref();
	SuperString					key;
	CFStringRef					dictRef;
	CCFString					str;
	bool						specialB = false;

	if (dlgB) {
		specialB = IsSpecialString(*this);
		
		if (specialB) {
			Split("/", &key, true);
			key.prepend("dlg/");
			asciiStr = key;
			if (menuB || dlgB) asciiStr.Ascii(true);
			asciiKeyRef = asciiStr.ref();
		}

		dictRef = kLocalized_Dialogs;
	} else if (menuB) {
		dictRef = kLocalized_Menus;
	} else {
		dictRef = kLocalized_Strings;
	}
		
	str.adopt(CFStrLocalize(dictRef, asciiKeyRef));

	#if 1 // ndef kDEBUG
		if (1) {
			if (specialB) {
				if (asciiStr == str) {
					Set(key);
					Split("/", &key, true);
					str.SetAndRetain(key.ref());
				}
			} else if (asciiStr == str) {
				if ((dlgB || menuB) && Split("/", &key, true)) {
				
					if (recurse_okayB) {
						key.Localize(false);
					}
					
					str.SetAndRetain(key.ref());
				} else {
					str.SetAndRetain(*this);
				}
			}
		}
	#endif // !kDEBUG

	Set(str);
	#endif // !_KJAMS_

	return *this;
}

void		IncrementNumberAtEndOfString(SuperString *strP)
{
	ustring		ustr(strP->utf8());
	size_t		startOfNumS = ustr.size();
	
	while (startOfNumS && CFIsDigit(ustr[startOfNumS - 1])) {
		startOfNumS--;
	}

	if (startOfNumS == 0 || startOfNumS == ustr.size()) {
		ustr.append(uc(" 1"));
	} else {
		int		numberI;
		char	numAC[32];
		char	*numStartZ = (char *)&ustr[startOfNumS];
		
		sscanf(numStartZ, "%d", &numberI);
		ustr.resize(startOfNumS);

		if (ustr[ustr.size() - 1] != ' ') {
			ustr.append(uc(" "));
		}

		sprintf(numAC, "%d", ++numberI);
		ustr.append(uc(numAC));
	}
	
	strP->Set(ustr);
}

SuperString&	SuperString::IncNumberAtEnd()
{
	IncrementNumberAtEndOfString(this);
	return *this;
}

SuperString&	SuperString::RemoveNumberAtEnd()
{
	ustring		ustr(utf8());
	size_t		startOfNumS = ustr.size();

	while (startOfNumS && CFIsDigit(ustr[startOfNumS - 1])) {
		startOfNumS--;
	}

	if (startOfNumS != ustr.size()) {

		if (startOfNumS && ustr[startOfNumS - 1] == ' ') {
			--startOfNumS;
		}

		ustr.resize(startOfNumS);
		Set(ustr);
	}

	return *this;
}


#define	kStarChar	'*'

bool	GetEditNumber(SuperString *nameStrP, int *numSP)
{
	SuperString			spaceStar(" ");
	
	*numSP = 0;
	spaceStar.append(kStarChar);
	spaceStar.MakeRecoverable();
	
	bool				is_nakedB = !nameStrP->Contains(spaceStar);
	
	if (!is_nakedB) {
		size_t				spaceStarSizeS = spaceStar.utf8().size();
		const ustring		&str(nameStrP->utf8());
		const char			*ch = (const char*)&str[str.size() - (3 + spaceStarSizeS)];
		
		is_nakedB = memcmp(spaceStar.utf8Z(), ch, spaceStarSizeS) == 0;
		
		if (is_nakedB) ch += spaceStarSizeS;
		if (is_nakedB) {is_nakedB = !!CFIsDigit(*ch); ++ch;}
		if (is_nakedB) {is_nakedB = !!CFIsDigit(*ch); ++ch;}
		if (is_nakedB) {is_nakedB = !!CFIsDigit(*ch); ++ch;}
		
		if (is_nakedB) {
			ch = (const char*)&str[str.size() - 3];
			
			*numSP = atoi(ch);
			nameStrP->pop_back((UInt32)(3 + spaceStarSizeS));
		}
	}
	
	return is_nakedB;
}

//	add a " *001" to the end
void	AddEditNumber(SuperString *strP, SuperString *numStrP0)
{
	SuperString		ext, numStr;
	int				num;

	strP->pop_ext(&ext);

	//	bool			is_nakedB = 
	GetEditNumber(strP, &num);

	if (numStrP0 == NULL || numStrP0->empty()) {
		char			bufAC[16];
		
		sprintf(bufAC, " %c%.3d", kStarChar, ++num);
		numStr.Set(bufAC);
		numStr.MakeRecoverable();
		
		if (numStrP0) {
			numStrP0->Set(numStr);
		}
	} else {
		numStr = *numStrP0;
	}
	
	strP->append(numStr);
	strP->append(ext);
}

SuperString		TimeToString(SInt32 curT)
{
#if HAS_CDHMSF
	SuperString		str(SuperString(CDHMSF((UInt32)(curT >> 2)), kTimeCode_LONG));
	
	str.append(".");
	str.append((long)(curT % 4));
	return str;
#else
	UNREFERENCED_PARAMETER(curT);
	return SuperString();
#endif
}

SInt32			StringToTime(const SuperString& str)
{
#if HAS_CDHMSF
	return CDHMSF(str.utf8Z()).blocks();
#else
	UNREFERENCED_PARAMETER(str);
	return 0;
#endif
}

bool			SuperString::EndsWith(const SuperString& other) const
{
	bool		ends_withB = false;
	
	if (size() >= other.size()) {
		ustring		thisStr(utf8().end() - other.size(), utf8().end());
		
		ends_withB = strcmp((const char *)thisStr.c_str(), (const char *)other.utf8().c_str()) == 0;
	}
	
	return ends_withB;
}

SuperString&		SuperString::pop_back_str(const SuperString& str)
{
	if (EndsWith(str)) {
		pop_back((UInt32)str.size());
	}
	
	return *this;
}

size_t			SuperString::MatchStart(const SuperString& other, char delimiterCh) const
{
	size_t			countL = 0;
	size_t			sizeL(utf8().size());
	bool			sameSizeB = false;
	
	if (sizeL > other.size()) {
		sizeL = other.size();
	} else if (sizeL == other.size()) {
		sameSizeB = true;
	}
	
	std::pair<ustring::const_iterator, ustring::const_iterator>		result(
		std::mismatch(utf8().begin(), utf8().begin() + sizeL, other.utf8().begin()));
	
	//	may not need that IF at all.  why bother?
	if (sameSizeB || result.first != utf8().end()) {
		countL = std::distance(utf8().begin(), result.first);		
	}
	
	if (delimiterCh) {
		const char	*thisA(utf8Z());

		while (thisA[countL - 1] != delimiterCh) {
			--countL;
		}
	}
	
	return countL;
}

static bool		IsWhiteSpace(char ch)
{	
	return 
	   ch == ' '
	|| ch == '\r'
	|| ch == '\t'
	|| ch == '\n';
}

SuperString&		SuperString::trim()
{
	ustring		str = utf8();

	//	trailing
	while (!str.empty() && IsWhiteSpace(str[str.size() - 1])) {
		str.erase(--str.end());
//		str.resize(str.size() - 1);
	}
	
	//	leading
	while (!str.empty() && IsWhiteSpace(str[0])) {
		str.erase(str.begin());
	}
	
	Set(str);
	return *this;
}

SuperString&		SuperString::Smarten()
{
	const ww_string&	wwStr(utf32());
	UTF32Vec			charVec;	charVec.assign(wwStr.begin(), wwStr.end());
	bool				openB = true;
	UTF32Char			openCh(0x201C);
	UTF32Char			closeCh(0x201D);
	UTF32Char			open1Ch(0x2018);
	UTF32Char			close1Ch(0x2019);

	for (UTF32Vec::iterator it(charVec.begin()); it != charVec.end(); ++it) {
		UTF32Char&	ch(*it);
		
		if (ch == '"') {

			if (openB) {
				ch = openCh;
			} else {
				ch = closeCh;
			}

			openB = !openB;

		} else if (ch == '\'') {

			if (openB) {
				ch = open1Ch;
			} else {
				ch = close1Ch;
			}

			openB = !openB;
		}
	}
	
	charVec.push_back(0);
	*this = SuperString(&charVec[0]);
	return *this;
}

SuperString&		SuperString::UnderScoresToSpaces()
{
	ScCFReleaser<CFMutableStringRef>	strRef;

	strRef.adopt(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, ref()));

	#if !defined(_CFTEST_) && !defined(_MIN_CF_)
		FixTrailingPeriod(true, strRef.Get());
	#endif
	
	Set(strRef.Get());
	return Replace("_", " ");
}

SuperString&		SuperString::pad(size_t charsL)
{
	while (size() < charsL) {
		append(" ");
	}

	return *this;
}
	
SuperString&		SuperString::Enquote(EnquoteType quoteType, bool only_if_notB)
{
	#define			QUOTE_CHAR			'\"'
	#define			SINGLE_QUOTE_CHAR	'\''
	SuperString		quotesStr;
	
	if (quoteType < EnquoteType_SINGLE_PLAIN) {
		quotesStr.append(QUOTE_CHAR);
		quotesStr.append(QUOTE_CHAR);
	} else if (quoteType < EnquoteType_GRAVE) {
		quotesStr.append(SINGLE_QUOTE_CHAR);
		quotesStr.append(SINGLE_QUOTE_CHAR);
	}

	switch (quoteType) {

		case EnquoteType_PLAIN:
		case EnquoteType_SINGLE_PLAIN: {
			// done!
		} break;

		case EnquoteType_SMART:
		case EnquoteType_SINGLE_SMART: {
			quotesStr.Smarten();
		} break;

		case EnquoteType_GRAVE: {
			quotesStr.append("``");
		} break;

		case EnquoteType_PARENS: {
			quotesStr.append("()");
		} break;

		case EnquoteType_PERCENTS: {
			quotesStr.append("%%");
		} break;

		case EnquoteType_SPACES: {
			quotesStr.append("  ");
		} break;

		case EnquoteType_NO_BREAK_SPACES: {
			SuperString			nbs(GetSpecialChar(UTF_SpecialChar_NO_BREAK_SPACE));
			
			quotesStr += nbs + nbs;
		} break;
	}
	
	SuperString		openingStr(quotesStr.ww_pop_front());
	
	if (!only_if_notB || !StartsWith(openingStr)) {
		prepend(openingStr);
	}

	if (!only_if_notB || !EndsWith(openingStr)) {
		append(quotesStr);
	}
	
	return *this;
}

SuperString			SuperString::Quoted(EnquoteType quoteType) const
{
	return SuperString(*this).Enquote(quoteType);
}

SuperString&		SuperString::NoQuotes(bool recoverB)
{
	if (recoverB) {
		Recover();
	} else {
		trim();
	}
	
	if (*this == "\"\"") {
		clear();
	} else {
		bool		startQuoteB = false;
		bool		endQuoteB = false;
		ustring		str = utf8();
		bool		doneB;

		//	trailing
		do {
			doneB = true;
			
			if (!str.empty()) {
				UTF8Char&		ch(str[str.size() - 1]);
				
				if (
					IsWhiteSpace(ch)
					|| ch == '~'
					|| ch == QUOTE_CHAR
				) {
					if (ch == QUOTE_CHAR) {
						endQuoteB = true;
					}

					str.erase(--str.end());
					doneB = false;
				}
			}
		} while (!doneB);
		
		//	leading
		while (
			   !startQuoteB 
			&& !str.empty() 
			&& (IsWhiteSpace(str[0]) || str[0] == QUOTE_CHAR)
		) {
			
			if (str[0] == QUOTE_CHAR) {
				startQuoteB = true;
			}
			
			str.erase(str.begin());
		}
		
		//	only remove quotes if they're on both start and end
		if (startQuoteB ^ endQuoteB) {
			if (startQuoteB) {
				str.insert(str.begin(), QUOTE_CHAR);
			} else {
				str.push_back(QUOTE_CHAR);
			}
		}
		
		Set(str);
	}

	return *this;
}

SuperString&		SuperString::Enspace()
{
	return Enquote(EnquoteType_NO_BREAK_SPACES, true);
}

SuperString		TruncEscapedToNum(size_t numS, SuperString str)
{
	bool			doneB = false;
	
	if (str.size() > numS) {
		str.resize(numS);
	}
	
	do {
		SuperString		escapedStr(str);
		
		escapedStr.Escape();
		doneB = escapedStr.size() <= numS;
		
		if (!doneB) {
			str.pop_back();
		}
	} while (!doneB);
	
	return str;
}

class CDictToStringMap {
	SStringMap&			i_map;
	
	public:
	CDictToStringMap(SStringMap& map) : i_map(map) {}

	void	operator()(CFStringRef keyRef, CFTypeRef valRef) {
		CFStringRef		valStrRef((CFStringRef)valRef);
		
		i_map[SuperString(keyRef)] = SuperString(valStrRef);
	}
};

SStringMap			DictToStringMap(CFDictionaryRef dictRef)
{
	CCFDictionary		dict(dictRef, true);
	SStringMap			map;
	
	dict.for_each(CDictToStringMap(map));
	return map;
}

CFDictionaryRef		CopyStringMapToDictionary(CFDictionaryRef dictRef)
{
	UNREFERENCED_PARAMETER(dictRef);
	CF_ASSERT(0);
	return NULL;
}

typedef std::map<UTF_SpecialChar, UInt32>	SpecialCharToUTF8Map;

static SpecialCharToUTF8Map		s_charToUTF8Map;

SuperString		GetSpecialChar(UTF_SpecialChar charType)
{
	if (s_charToUTF8Map.empty()) {
		s_charToUTF8Map[UTF_SpecialChar_ZH_COLON]				= 0xEFBC9A00;
		s_charToUTF8Map[UTF_SpecialChar_HYPHEN]					= 0xE2809000;
		s_charToUTF8Map[UTF_SpecialChar_RED_ALERT]				= 0xF09F9AA8;
		s_charToUTF8Map[UTF_SpecialChar_FULLWIDTH_PERCENT]		= 0xEFBC8500;
		s_charToUTF8Map[UTF_SpecialChar_NO_BREAK_SPACE]			= 0xC2A00000;
		s_charToUTF8Map[UTF_SpecialChar_SIX_PER_EM_SPACE]		= 0xE2808600;
		s_charToUTF8Map[UTF_SpecialChar_EM_DASH]				= 0xE2809400;
		s_charToUTF8Map[UTF_SpecialChar_EN_DASH]				= 0xE2809300;
		s_charToUTF8Map[UTF_SpecialChar_FIGURE_DASH]			= 0xE2809200;
		s_charToUTF8Map[UTF_SpecialChar_MINUS_SIGN]				= 0xE2889200;
		s_charToUTF8Map[UTF_SpecialChar_KEY_CHANGE_FLAT]		= 0xE299AD00;
		s_charToUTF8Map[UTF_SpecialChar_KEY_CHANGE_SHARP]		= 0xE299AF00;
		s_charToUTF8Map[UTF_SpecialChar_KEY_CHANGE_BAR]			= 0xE296AD00;
		s_charToUTF8Map[UTF_SpecialChar_KEY_CHANGE_DIAMOND]		= 0xE299A200;
		s_charToUTF8Map[UTF_SpecialChar_KEY_CHANGE_TRIANGLE]	= 0xE296B300;
		s_charToUTF8Map[UTF_SpecialChar_LETTER_YOO]				= 0xF09D978E;
		s_charToUTF8Map[UTF_SpecialChar_EM_SPACE]				= 0xE2808300;
		s_charToUTF8Map[UTF_SpecialChar_e_ACUTE]				= 0xC3A90000;
		s_charToUTF8Map[UTF_SpecialChar_SMALL_ASTERISK]			= 0xEFB9A100;
		s_charToUTF8Map[UTF_SpecialChar_BLACK_DOWN_TRIANGLE]	= 0xe296be00;
		s_charToUTF8Map[UTF_SpecialChar_COPYRIGHT]				= 0xC2A90000;
	}

	#ifdef kDEBUG
		CF_ASSERT(s_charToUTF8Map.find(charType) != s_charToUTF8Map.end());
	#endif

	return UTF8BytesToString(s_charToUTF8Map[charType]);
}

SuperString		GetHyphen()
{
	return GetSpecialChar(UTF_SpecialChar_HYPHEN);
}
