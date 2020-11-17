#include "stdafx.h"
#include "SuperString.h"
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CFNumberFormatter.h>
#include <CoreFoundation/CFDateFormatter.h>
#include <CoreFoundation/CFCalendar.h>
#include <CoreFoundation/CFBundle.h>
#include "CCFData.h"
#include "CFTest.h"

#define			kTest_ExtraLogging					0

//---------------------------------------------------------------------------------------------------------
//	test all of these, all must succeed
#if 0
	#define		kTest_CFTicks						0
	#define		kTest_StringCompare					0
	#define		kTest_EncodingConversion			0
	#define		kTest_Locale						0
	#define		kTest_LocToEncoding					0
	#define		kTest_DateTime						1
	#define		kTest_Bundle						0
#else
	#define		kTest_CFTicks						0
	#define		kTest_StringCompare					1
	#define		kTest_EncodingConversion			1
	#define		kTest_Locale						1
	#define		kTest_LocToEncoding					1
	#define		kTest_DateTime						1
	#define		kTest_Bundle						1
#endif

bool		ExtraLogging()
{
	return kTest_ExtraLogging;
}

const char *    GetTestDataPath()
{
	return

	#if _QT_
		"../"
	#endif

	"../../../../test_data/";
}


static SuperString		GetAccentedString()
{
	return 	SuperString("%C3%B1%C3%A9%C3%BC%C3%AE", kCFStringEncodingPercentEscapes);
}

static void	ShowDiacriticSensitiveCompare(bool insensitiveB)
{
	SuperString		testStr;

	CFSetDiacriticInsensitive(insensitiveB);
	
	testStr.ssprintf("Diacritic %sensitive", insensitiveB ? "Ins" : "S");
	
	SuperString			str1("NeUi");
	SuperString			str2(GetAccentedString());
	bool				diacritic_insensitive_compareB = str1 == str2;
	SuperString			resultStr;
	
	resultStr.ssprintf("%s %s %s", 
	   str1.utf8Z(), 
	   diacritic_insensitive_compareB ? "==" : "!=", 
	   str2.utf8Z());

	if (ExtraLogging()) {
		CCFLog(true)(resultStr.ref());
	}
	
	CFReportUnitTest(testStr.utf8Z(), insensitiveB != diacritic_insensitive_compareB);

	CFSetDiacriticInsensitive(true);
}

static void	ShowCompare(const SuperString& str1, const SuperString& str2, CFComparisonResult expectedResult)
{
	if (ExtraLogging()) {
		SuperString			format("%s %s %s");
		const char*			ineqZ(str1 < str2 ? "<" : (str2 < str1 ? ">" : "=="));

		format.ssprintf(NULL, str1.utf8Z(), ineqZ, str2.utf8Z());
		CCFLog(true)(format.ref());
	}

	switch (expectedResult) {
	
		case kCFCompareLessThan: {
			CFReportUnitTest("Compare Less", !(str1 < str2));
		} break;

		case kCFCompareEqualTo: {
			CFReportUnitTest("Compare Equal", !(str1 == str2));
		} break;

		case kCFCompareGreaterThan: {
			CFReportUnitTest("Compare Greater", !(str1 > str2));
		} break;
	}
}

/******************************************************/
class CParseXML {
	#define						kLogDuringParse		1
	CFStringRef					i_keyStr;
	CFXMLNodeTypeCode			i_lastType;
	
	#if kLogDuringParse
	void	IndentLevel(short levelS) {
		loop (levelS) {
			CCFLog()(CFSTR("\t"));
		}
	}
	#endif

	public: 
	CParseXML() : i_lastType(kCFXMLNodeTypeDocument) { }
	
	void operator()(CCFXMLNode& node, short levelS)
	{
		CFXMLNodeTypeCode	type = node.GetTypeCode();
		
		#if kLogDuringParse
			if (type == i_lastType) {
				CCFLog()(CFSTR("\n"));
			}
		#endif
		
		i_lastType = type;
		
		switch (type) {

			default:
				//	nobody cares
				break;
				
			case kCFXMLNodeTypeElement: {
				i_keyStr = node.GetString();
				
				#if kLogDuringParse
					IndentLevel(levelS);
					CCFLog()(i_keyStr);
					
					if (!node.GetInfoPtr()->attributes) {
						CCFLog()(CFSTR(": "));
					}
				#endif
				break;
			}
				
			case kCFXMLNodeTypeText: {
				SuperString			valueStr(node.GetString());
				
				#if kLogDuringParse
					CCFLog()(valueStr.ref());
				#endif
				break;
			}
		}
	}
};

#ifdef __MWERKS__
static CFGregorianDate			CFGetGregorianDate(CFAbsoluteTime at, CFTimeZoneRef tz)
{
	CFGregorianDate		gregDate;

	CFAbsoluteTimeGetGregorianDate_MW(at, tz, &gregDate);
	return gregDate;
}
#else
	#define		CFGetGregorianDate		CFAbsoluteTimeGetGregorianDate
#endif

void	ReportBitDepth(const char *projZ, size_t detectedL, size_t properL)
{
	SuperString		str("%sbit %s");
	
	str.ssprintf(NULL, properL == 8 ? "64" : "32", projZ);
	CFReportUnitTest(str.utf8Z(), detectedL != properL);
}

void	CFTest()
{
	SuperString		resultStr;
	
	if (!IsDefaultEncodingSet()) {
		SetDefaultEncoding(kCFStringEncodingASCII);
	}
	
	CCFLog()(CFSTR("\n--------------------New CFTest Log----------------\n"));

	if (ExtraLogging()) {
		SuperString			str("Size of CFIndex: %d");
		
		str.ssprintf(NULL, (int)sizeof(CFIndex));
		CCFLog(true)(str.ref());
	}

	{
		#if __LP64__
			size_t		proper_sizeL = 8;
		#else
			size_t		proper_sizeL = 4;
		#endif

		#if OPT_WINOS
			ReportBitDepth("CFLite", CFIndexGetSize(), proper_sizeL);
		#endif
		
		ReportBitDepth("CFTest", sizeof(CFIndex), proper_sizeL);
	}

	{
		CCFDictionary		dict;
		Ptr					dictP((Ptr)&dict);
		
		dict.SetValue("ptr", dictP);
		
		Ptr					out_dictP(dict.GetAs_Ptr("ptr"));
		
		CFReportUnitTest("Dictionary Ptr", dictP != out_dictP);
	}

	{
		SuperString			str;
		Ptr					strP((Ptr)&str);
		
		str.Set_Ptr(strP);
		
		Ptr					out_strP(str.GetAs_Ptr());
		
		CFReportUnitTest("String Ptr", strP != out_strP);
	}

	if (kTest_CFTicks) {
		CFAbsoluteTime			curT(CFAbsoluteTimeGetCurrent());
		CFAbsoluteTime			endT(curT + (kEventDurationSecond * 16));
		UInt32					prevTick(0);
		
		do {
			UInt32			curTick = CFTickCount();
			
			if (prevTick == 0) {
				prevTick = curTick;
			} else {
				if (curTick - prevTick >= 60) {
					CCFLog()(CFSTR("Tick\n"));
					prevTick += 60;
				}
			}

		} while (CFAbsoluteTimeGetCurrent() < endT);
	}
	
	if (kTest_StringCompare) {
		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Strings---------------\n"));
		}
		
		{
			#define						kDataRefStr		"yeah baby"
			ScCFReleaser<CFDataRef>		dataRef(SuperString(kDataRefStr).CopyDataRef());
			SuperString					str;
			
			str.Set(dataRef);

			if (ExtraLogging()) {
				CCFLog()(CFSTR("The next line should read " kDataRefStr "\n"));
				CCFLog(true)(str.ref());
			}

			CFReportUnitTest("Convert to/from DataRef", str != SuperString(kDataRefStr));
		}
		
		{
			SuperString		str1("foscoobyar");
			SuperString		str2("scooby");
			bool			containsB(str1.Contains(str2));
			
			if (ExtraLogging()) {
				resultStr.ssprintf(
					"\n%s %s %s\n", 
					str1.utf8Z(), 
					containsB ? "contains" : "$$$ Error: does not contain(!?)", 
					str2.utf8Z());
				
				CCFLog()(resultStr.ref());
			}
			
			CFReportUnitTest("Containment", !containsB);

			str1.Replace(str2, "o b");

			#define						kReplacementStr		"foo bar"

			if (ExtraLogging()) {
				CCFLog()(CFSTR("The next line should read <" kReplacementStr ">\n"));
				CCFLog(true)(str1.ref());
			}

			CFReportUnitTest("Replacement", str1 != SuperString(kReplacementStr));
		}
		
		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Case Insensitive Compare---------------\n"));
		}
		
		ShowDiacriticSensitiveCompare(false);

		if (ExtraLogging()) {
			CCFLog()(CFSTR("\n"));
		}
		
		ShowDiacriticSensitiveCompare(true);
		
		{
			if (ExtraLogging()) {
				CCFLog()(CFSTR("------------------Inequality---------------\n"));
			}
			
			SuperString			catStr("Cat");
			SuperString			dogStr("Dog");

			ShowCompare(catStr, dogStr, kCFCompareLessThan);
			ShowCompare(dogStr, catStr, kCFCompareGreaterThan);
			ShowCompare(catStr, catStr, kCFCompareEqualTo);
		}
		
	}
	
	if (kTest_EncodingConversion) {
		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Encoding---------------\n"));
		}

		{
			SuperString				jStr(GetAccentedString());
			
			#if defined(__WIN32__)
				#define		kConvertEncode		kCFStringEncodingWindowsLatin1
			#else
				#define		kConvertEncode		kCFStringEncodingMacRoman
			#endif
			
			ustring				j;
			CopyCFStringToUString(jStr.ref(), j, kConvertEncode);
			
			SuperString			convertedJ;  convertedJ.Set(j, kConvertEncode);
			
			if (ExtraLogging()) {
				CCFLog()(resultStr.ssprintf(
					"The next line should read <%s>\n%s\n", 
					jStr.utf8Z(), convertedJ.utf8Z()).ref());
				
				CCFLog()(resultStr.ssprintf("conversion: %s\n", convertedJ == jStr ? "Success!" : "$$ FAILED!").ref());
			}

			CFReportUnitTest("Encoding Conversion", convertedJ != jStr);
		}

		CFStringEncoding		encoding;
		
		if (ExtraLogging()) {
			encoding = CFStringGetSystemEncoding();
			CCFLog()(resultStr.ssprintf("Sys Encoding: %s\n", SuperString(CFStringGetNameOfEncoding(encoding)).utf8Z()).ref());
			CCFLog()(resultStr.ssprintf("Sys IANA charset: %s\n", SuperString(CFStringConvertEncodingToIANACharSetName(encoding)).utf8Z()).ref());
		}
				
		{
			encoding = kCFStringEncodingWindowsLatin1;

			UInt32			codePage = CFStringConvertEncodingToWindowsCodepage(encoding);
			
			CFReportUnitTest("Encoding to Codepage", codePage != kCodePage_WindowsLatin1);
			
			if (ExtraLogging()) {
				CCFLog()(resultStr.ssprintf("codepage: %ld\n", codePage).ref());
			}
			
			CFReportUnitTest("Codepage to Encoding", CFStringConvertWindowsCodepageToEncoding(codePage) != encoding);
		}
		
		//	test for russian
		{
			SuperString			IANA_NameStr("iso-8859-1");
			
			encoding = CFStringConvertIANACharSetNameToEncoding(IANA_NameStr.ref());

			if (ExtraLogging()) {
				CCFLog()(resultStr.ssprintf("Iso Latin Encoding: %s (%d)\n", SuperString(CFStringGetNameOfEncoding(encoding)).utf8Z(), (int)encoding).ref());
				CCFLog()(resultStr.ssprintf("Iso Latin IANA charset: %s\n", SuperString(CFStringConvertEncodingToIANACharSetName(encoding)).utf8Z()).ref());
			}
			
			CFReportUnitTest("Convert IANA CharSet To Encoding", encoding != kCFStringEncodingISOLatin1);
		}
	}
	
	ScCFReleaser<CFLocaleRef>			localeRef(CFLocaleCopyCurrent());

	#if OPT_MACOS && !_QT_ && !_JUST_CFTEST_
	if (kTest_LocToEncoding) {
		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Locale to Encoding---------------\n"));
		}

		SStringVec				langVec;
		
		langVec.push_back("en");
		langVec.push_back("en_US");
		langVec.push_back("en_GB");
		langVec.push_back("en_AU");
		langVec.push_back("en_CA");
		langVec.push_back("en_SG");
		langVec.push_back("en_IE");

		langVec.push_back("fr");
		langVec.push_back("fr_FR");
		langVec.push_back("fr_CA");
		langVec.push_back("fr_CH");
		langVec.push_back("fr_BE");

		langVec.push_back("de");
		langVec.push_back("de_DE");
		langVec.push_back("de_CH");
		langVec.push_back("de_AT");
		
		langVec.push_back("ja");
		
		langVec.push_back("it");
		langVec.push_back("it_IT");
		langVec.push_back("it_CH");

		langVec.push_back("es");
		langVec.push_back("es_ES");
		langVec.push_back("es_XL");
		
		#define		kCreateArray		0
		
		for (SStringVec::iterator it = langVec.begin(); it != langVec.end(); ++it) {
			const SuperString		&curLang(*it);
			CFStringEncoding		oldEnc(LanguageRegionToEncoding(curLang));
			CFStringEncoding		newEnc(CFLangRgnStrToEncoding(curLang));
			SuperString				formatStr("Convert Locale Encoding: “%s” (%s, %s)", kCFStringEncodingUTF8);
			SuperString				charSetName(CFStringConvertEncodingToIANACharSetName(oldEnc));
			SuperString				encodingName(CFStringGetNameOfEncoding(oldEnc));

			formatStr.ssprintf(NULL, curLang.utf8Z(), charSetName.utf8Z(), encodingName.utf8Z());
			
			if (ExtraLogging()) {
			
				#if kCreateArray
				CCFLog()(
					resultStr.ssprintf(
						"s_langMap[\"%s\"]\t= \"%s\"\n",
						curLang.utf8Z(), 
						ULong_To_Hex(oldEnc).c_str() 
					).ref());
				
				#else
				CCFLog()(
					resultStr.ssprintf(
						"oldEnc: %s\n"
						"newEnc: %s\n", 
						ULong_To_Hex(oldEnc).c_str(), 
						ULong_To_Hex(newEnc).c_str() 
					).ref());
				#endif
			}

			#if !kCreateArray
				CFReportUnitTest(formatStr.utf8Z(), oldEnc != newEnc);
			#endif
		}
	}
	#endif
	
	if (kTest_Locale) {
		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Locale---------------\n"));
		}
		
		SStringMap				langVec;
		
		langVec["English"]		= "en";
		langVec["French"]		= "fr";
		langVec["German"]		= "de";
		langVec["Hungarian"]	= "hu";
		
		for (SStringMap::iterator it = langVec.begin(); it != langVec.end(); ++it) {
			const SuperString&	langStr(it->first);
			const SuperString&	correct_localIdStr(it->second);
			
			SuperString			localIdStr(CFLocaleCreateCanonicalLanguageIdentifierFromString(kCFAllocatorDefault, langStr.ref()),	false);
			SuperString			formatStr("Convert Locale to Lang ID: “%s”", kCFStringEncodingUTF8);
			
			formatStr.ssprintf(NULL, langStr.utf8Z());
			
			if (ExtraLogging()) {
				CCFLog()(
					resultStr.ssprintf(
						"---------\n"
						"LangStr: %s\n"
						"Locale ID: %s\n", 
						langStr.utf8Z(), 
						localIdStr.utf8Z()
					).ref());
				
				{
					ScCFReleaser<CFLocaleRef>		curLocaleRef(CFLocaleCreate(kCFAllocatorDefault, localIdStr.ref()));
					SuperString						lang_codeStr((CFStringRef)CFLocaleGetValue(curLocaleRef, kCFLocaleLanguageCode), false);
					SuperString						country_codeStr((CFStringRef)CFLocaleGetValue(curLocaleRef, kCFLocaleCountryCode), false);
					
					CCFLog()(resultStr.ssprintf("lang: %s\n", lang_codeStr.utf8Z()).ref());
					CCFLog()(resultStr.ssprintf("country: %s\n", country_codeStr.utf8Z()).ref());
				}
				
				{
					CCFDictionary			dictRef(CFLocaleCreateComponentsFromLocaleIdentifier(kCFAllocatorDefault, localIdStr.ref()));
					
					CCFLog(true)(dictRef);
				}
				
				SuperString		convertStr;
				
				// *******************
				convertStr.Set(CFLocaleCopyDisplayNameForPropertyValue(localeRef, kCFLocaleIdentifier, localIdStr.ref()));
				CCFLog()(resultStr.ssprintf("\tkCFLocaleIdentifier: %s\n", convertStr.utf8Z()).ref());
				
				// *******************
				convertStr.Set(CFLocaleCopyDisplayNameForPropertyValue(localeRef, kCFLocaleLanguageCode, localIdStr.ref()));
				CCFLog()(resultStr.ssprintf("\tkCFLocaleLanguageCode: %s\n", convertStr.utf8Z()).ref());

				// *******************
				convertStr.Set(CFLocaleCopyDisplayNameForPropertyValue(localeRef, kCFLocaleCountryCode, localIdStr.ref()));
				CCFLog()(resultStr.ssprintf("\tkCFLocaleCountryCode: %s\n", convertStr.utf8Z()).ref());
			}

			CFReportUnitTest(formatStr.utf8Z(), localIdStr != correct_localIdStr);
		}
	}

	if (kTest_Locale) {
		CCFArray		arrayRef(CFLocaleCopyPreferredLanguages());

		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Preferred Languages---------------\n"));
			CCFLog(true)(arrayRef);

			CCFLog()(CFSTR("------------------Preferred Language Match?---------------\n"));
		}
		
		SuperString		prefLang(arrayRef.GetIndValAs_Str(0));
		SuperString		prefLang2(CFCopyLocaleLangKeyCode(), false);
		
		if (ExtraLogging()) {
			CCFLog()(resultStr.ssprintf("lang: %s\n", prefLang.utf8Z()).ref());
			CCFLog()(resultStr.ssprintf("lang2: %s\n", prefLang2.utf8Z()).ref());
			CCFLog()(resultStr.ssprintf("lang match: %s\n", prefLang == prefLang2 ? "Success!" : "$$ FAILED!").ref());
		}

		CFReportUnitTest("Preferred Language Match", prefLang != prefLang2);
	}
	
	//	no unit tests
	if (kTest_DateTime) {
		
		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Calendar---------------\n"));
		}
		
		ScCFReleaser<CFCalendarRef>		calendarRef(CFCalendarCopyCurrent());
		SuperString						calIDStr(CFCalendarGetIdentifier(calendarRef));
		
		resultStr.ssprintf("Calendar ID: %s\n", calIDStr.utf8Z());
		
		CFReportUnitTest("Calendar ID", calIDStr != "gregorian");

		if (ExtraLogging()) {
			CCFLog()(resultStr.ref());
		}
		
		CFAbsoluteTime					absTime(CFAbsoluteTimeGetCurrent());

		SuperString			dateStr0;

		dateStr0.Set(absTime, SS_Time_LONG_PRETTY);

		CCFTimeZone						timeZoneRef(CFCalendarCopyTimeZone(calendarRef));
		CCFTimeZone						timeZoneRef2(CFTimeZoneCopyDefault());
		CFTimeInterval					ti_1(CFTimeZoneGetSecondsFromGMT(timeZoneRef, absTime));
		CFTimeInterval					ti_2(CFTimeZoneGetSecondsFromGMT(timeZoneRef2, absTime));

		CFReportUnitTest("Time Zone & Calendar match", ti_1 != ti_2);

		ScCFReleaser<CFDateRef>			dateRef(CFDateCreate(kCFAllocatorDefault, absTime));
		CCFString						tzNameRef(CFTimeZoneGetName(timeZoneRef));
		
		if (ExtraLogging()) {
			CCFLog(true)(dateRef.Get());
			CCFLog(true)(timeZoneRef.Get());
			CCFLog(true)(tzNameRef.Get());
		}

		CFAbsoluteTime					absTime2(CFDateGetAbsoluteTime(dateRef));

		CFReportUnitTest("Time round trip through time zone", absTime != absTime2);
		
		if (ExtraLogging()) {
			CFGregorianDate					gregDate(CFGetGregorianDate(absTime, timeZoneRef));
	
			CCFLog()(resultStr.ssprintf("CFGregorianDate:\n%d/%d/%d - %d:%d:%f\n",
				   (int)gregDate.month,
				   (int)gregDate.day, 
				   (int)gregDate.year,
				   (int)gregDate.hour,
				   (int)gregDate.minute, 
				   (float)gregDate.second).ref());
		}

		ScCFReleaser<CFDateFormatterRef>	dateFormatterRef(CFDateFormatterCreate(
			kCFAllocatorDefault, localeRef, kCFDateFormatterFullStyle, kCFDateFormatterFullStyle));
		
		SuperString							dateStr(CFDateFormatterCreateStringWithDate(
			kCFAllocatorDefault, dateFormatterRef, dateRef));
		
		SuperString							dateStr2(CFDateFormatterCreateStringWithAbsoluteTime(
			kCFAllocatorDefault, dateFormatterRef, absTime));
		
		CFReportUnitTest("CFDateTime and CFAbsTime match", dateStr != dateStr2);

		SuperString			dateStr3;
		
		{
			ScCFReleaser<CFLocaleRef>			localeRef(CFLocaleCopyCurrent());

			SuperString							refId_1(CFLocaleGetIdentifier(localeRef));
			SuperString							refId_2;

			{
				SuperString						localeLangCode(CFCopyLocaleLangKeyCode(), false);
				ScCFReleaser<CFLocaleRef>		localeRef2(CFLocaleCreate(
					kCFAllocatorDefault, localeLangCode.ref()));
				
				refId_2.Set(CFLocaleGetIdentifier(localeRef2));
				
				refId_2.Replace("-", "_");
			}

			CFReportUnitTest("Locale ID match", refId_1 != refId_2);

			CFDateFormatterStyle				formatStyle(kCFDateFormatterFullStyle);
			ScCFReleaser<CFDateFormatterRef>	formatterRef(CFDateFormatterCreate(
				kCFAllocatorDefault, localeRef, formatStyle, formatStyle));
			CCFTimeZone							tz(CFTimeZoneCopyDefault());

			//CFDateFormatterSetProperty(formatterRef, kCFDateFormatterTimeZone, tz);
			dateStr3.Set(CFDateFormatterCreateStringWithAbsoluteTime(kCFAllocatorDefault, formatterRef, absTime), false);
		}

		if (ExtraLogging()) {
			CCFLog(true)(dateStr3.ref());
		}

		SuperString			dateStr4;

		dateStr4.Set(absTime, SS_Time_LONG_PRETTY);

		CFReportUnitTest("Date Strings Match", dateStr0 != dateStr4);

		if (ExtraLogging()) {
			CCFLog(true)(dateStr4.ref());
			CCFLog(true)(dateStr0.ref());
		}

		//int i = 0;
		/*
		//	not on windows?
		CCFLog()(CFSTR("------------------Relative Date Test---------------\n"));
		CFDateFormatterSetProperty(dateFormatterRef, kCFDateFormatterDoesRelativeDateFormattingKey, kCFBooleanTrue);
		
		dateRef.adopt(CFDateCreate(kCFAllocatorDefault, absTime + kEventDurationWeek));
		dateStr.adopt(CFDateFormatterCreateStringWithDate(kCFAllocatorDefault, dateFormatterRef, dateRef));
		CCFLog()(CFSTR("is this a relative date?\n"));
		CCFLog(true)(dateStr.Get());
		*/
		
		#if 0
		CCFLog()(CFSTR("------------------timezone names---------------\n"));
		CCFArray		namesArray(CFTimeZoneCopyKnownNames());
		CCFLog(true)(namesArray);
		#endif
		
		#if 0
		CCFLog()(CFSTR("------------------Duration Printing---------------\n"));
		CFAbsTimeVec		intervalVec;
		
		intervalVec.push_back(0);
		intervalVec.push_back(1);
		intervalVec.push_back(2);
		
		intervalVec.push_back(kEventDurationMinute);
		intervalVec.push_back(kEventDurationMinute + kEventDurationSecond);
		intervalVec.push_back(kEventDurationMinute + (kEventDurationSecond * 2) + 0.5);

		intervalVec.push_back(kEventDurationMinute * 2);
		intervalVec.push_back((kEventDurationMinute * 2) + kEventDurationSecond);
		intervalVec.push_back((kEventDurationMinute * 2) + kEventDurationSecond + 0.5);
		intervalVec.push_back((kEventDurationMinute * 2) + (kEventDurationSecond * 2));

		intervalVec.push_back(kEventDurationHour);
		intervalVec.push_back(kEventDurationHour + kEventDurationMinute);
		intervalVec.push_back(kEventDurationHour + kEventDurationMinute + 0.5);
		intervalVec.push_back(kEventDurationHour + (kEventDurationMinute * 2));
		intervalVec.push_back(kEventDurationHour + (kEventDurationMinute * 29));
		intervalVec.push_back(kEventDurationHour + (kEventDurationMinute * 29.5));
		intervalVec.push_back(kEventDurationHour + (kEventDurationMinute * 30));
		intervalVec.push_back(kEventDurationHour + (kEventDurationMinute * 30.5));

		intervalVec.push_back((kEventDurationHour * 2));
		intervalVec.push_back((kEventDurationHour * 2) + kEventDurationMinute);
		intervalVec.push_back((kEventDurationHour * 2) + (kEventDurationMinute * 2));

		intervalVec.push_back(kEventDurationDay);
		intervalVec.push_back(kEventDurationDay + kEventDurationHour);
		intervalVec.push_back(kEventDurationDay + kEventDurationHour + kEventDurationHour * 0.5);
		intervalVec.push_back(kEventDurationDay + (kEventDurationHour * 2));

		intervalVec.push_back((kEventDurationDay * 2));
		intervalVec.push_back((kEventDurationDay * 2) + kEventDurationHour);
		intervalVec.push_back((kEventDurationDay * 2) + (kEventDurationHour * 2));

		intervalVec.push_back(kEventDurationMonth);
		intervalVec.push_back(kEventDurationMonth + kEventDurationDay);
		intervalVec.push_back(kEventDurationMonth + (kEventDurationDay * 2));

		intervalVec.push_back((kEventDurationMonth * 2));
		intervalVec.push_back((kEventDurationMonth * 2) + kEventDurationDay);
		intervalVec.push_back((kEventDurationMonth * 2) + (kEventDurationDay * 2));

		intervalVec.push_back(kEventDurationYear);
		intervalVec.push_back(kEventDurationYear + kEventDurationMonth);
		intervalVec.push_back(kEventDurationYear + (kEventDurationMonth * 2));

		intervalVec.push_back((kEventDurationYear * 2));
		intervalVec.push_back((kEventDurationYear * 2) + kEventDurationMonth);
		intervalVec.push_back((kEventDurationYear * 2) + (kEventDurationMonth * 2));
		
		for (CFAbsTimeVec::iterator it = intervalVec.begin(); it != intervalVec.end(); ++it) {
			CFTimeInterval		intervalT(*it);
			SuperString			str(CFTimeIntervalCopyString(intervalT), false);
			
			CCFLog(true)(str.ref());
		}
		#endif
	}
	
	//	no unit tests
	if (ExtraLogging()) {
		CCFLog()(CFSTR("------------------Numbers---------------\n"));
		ScCFReleaser<CFNumberFormatterRef>		numFormatRef;
		ScCFReleaser<CFStringRef>				numStr;
		float									numF = 123456.789f;
		ScCFReleaser<CFNumberRef>				numberRef(CFNumberCreate(
			kCFAllocatorDefault, kCFNumberFloat32Type, &numF));
		
		numFormatRef.adopt(CFNumberFormatterCreate(
			kCFAllocatorDefault, localeRef, kCFNumberFormatterDecimalStyle));
		numStr.adopt(CFNumberFormatterCreateStringWithNumber(
			kCFAllocatorDefault, numFormatRef, numberRef));
		CCFLog(true)(numStr.Get());
		
		numFormatRef.adopt(CFNumberFormatterCreate(
			kCFAllocatorDefault, localeRef, kCFNumberFormatterCurrencyStyle));
		numStr.adopt(CFNumberFormatterCreateStringWithNumber(
			kCFAllocatorDefault, numFormatRef, numberRef));
		CCFLog(true)(numStr.Get());
	}

	if (kTest_Bundle) {
		if (ExtraLogging()) {
			CCFLog()(CFSTR("------------------Bundle---------------\n"));
		}
		
		ScCFReleaser<CFURLRef>			bundleUrlRef;
		ScCFReleaser<CFBundleRef>		bundleRef(CFBundleGetMainBundle(), true);
		
		CFReportUnitTest("Getting Bundle", bundleRef.Get() == NULL);
		
		if (bundleRef.Get() != NULL) {
			bundleUrlRef.adopt(CFBundleCopyBundleURL(bundleRef));
		}
		
		CFReportUnitTest("Getting Bundle URL", bundleUrlRef.Get() == NULL);

		if (bundleUrlRef.Get() != NULL) {
			
			if (ExtraLogging()) {
				CCFLog(true)(bundleUrlRef.Get());

				{
					CCFDictionary	dictRef(CFBundleGetInfoDictionary(bundleRef), true);
					
					CCFLog(true)(dictRef);
				}
			}
			
			SuperString				relPathStr(GetTestDataPath());
				
			{
				if (ExtraLogging()) {
					CCFLog()(CFSTR("------------------plist---------------\n"));
				}

/*
				SuperString		jsonStr("{"
					"\"file status\": 2,"
					"\"path\": \"\\\\?\\C:\\Users\\davec\\Music\\kJams\\kJams Music\\Unknown Artist\\Unknown Album\\PBS Classics-01-Fred Rogers-Won't you be My Neighbor.mp4\","
					"\"type\": 4"
				"}");

				CCFDictionary			dict(jsonStr, true);

				CCFLog(true)(dict);

				CCFData					data(dict);
				CCFDictionary			dict2(data.CopyAs_Dict(false));

				CCFLog(true)(dict2);
*/
				SuperString				testRelPath("test.xml");	testRelPath.prepend(relPathStr);
				ScCFReleaser<CFURLRef>	xmlUrlRef(CFURLCreateWithFileSystemPathRelativeToBase(
					kCFAllocatorDefault, testRelPath.ref(), kCFURLPOSIXPathStyle, false, bundleUrlRef));
				bool					goodB = xmlUrlRef.Get() != NULL;
				
				CFReportUnitTest("Getting PList URL", !goodB);

				if (goodB) {
					CCFDictionary						dictRef;
					ScCFReleaser<CFURLRef>				absUrlRef(CFURLCopyAbsoluteURL(xmlUrlRef));
					
					if (ExtraLogging()) {
						CCFLog()(resultStr.ssprintf("URL: %s\n", SuperString(CFURLGetString(absUrlRef)).utf8Z()).ref());
					}
					
					goodB = Read_PList(xmlUrlRef, dictRef.ImmutableAddressOf());

					CFReportUnitTest("Reading PList file", !goodB);

					if (goodB) {
						if (ExtraLogging()) {
							CCFLog(true)(dictRef);
						}
						
						{
							CCFArray		cfArray(dictRef.GetAs_Array("Playlists\\0\\Columns\\SORTDIR"), true);
							
							goodB = !cfArray.empty();
							CFReportUnitTest("CFDict key as path", !goodB);
						}

						{
#if OPT_WINOS
							Logf("Exception report here is OK and expected. Please ignore this:");
#endif
							goodB = !dictRef.ContainsKey("Playlists\\25\\Columns\\SORTDIR");
							CFReportUnitTest("CFDict key as path: invalid array index", !goodB);
						}
						
						{
							goodB = !dictRef.ContainsKey("Playlists\\0\\foo\\bar");
							CFReportUnitTest("CFDict key as path: no such dict", !goodB);
						}
						
						{
							goodB = !dictRef.ContainsKey("Playlists\\0\\3");
							CFReportUnitTest("CFDict key as path: no such array", !goodB);
						}
						
						{
							SuperString				outRelpath("out.xml");	outRelpath.prepend(relPathStr);
							ScCFReleaser<CFURLRef>	outXmlUrlRef(CFURLCreateWithFileSystemPathRelativeToBase(
								kCFAllocatorDefault, outRelpath.ref(), kCFURLPOSIXPathStyle, false, bundleUrlRef));
							
							goodB = Write_PList(dictRef.Get(), outXmlUrlRef) == noErr;

							CFReportUnitTest("Writing PList file", !goodB);
						}
					}
				}
			}

			{
				if (ExtraLogging()) {
					CCFLog()(CFSTR("------------------xml---------------\n"));
				}

				SuperString				testRelPath("Chiquitita.xml");	testRelPath.prepend(relPathStr);
				ScCFReleaser<CFURLRef>	xmlUrlRef(CFURLCreateWithFileSystemPathRelativeToBase(
					kCFAllocatorDefault, testRelPath.ref(), kCFURLPOSIXPathStyle, false, bundleUrlRef));
				bool					goodB = xmlUrlRef.Get() != NULL;

				CFReportUnitTest("Getting XML URL", !goodB);

				if (xmlUrlRef.Get()) {
					CCFXmlTree					xml;
					ScCFReleaser<CFURLRef>		absUrlRef(CFURLCopyAbsoluteURL(xmlUrlRef));
					
					if (ExtraLogging()) {
						CCFLog()(resultStr.ssprintf("URL: %s\n", SuperString(CFURLGetString(absUrlRef)).utf8Z()).ref());
					}
					
					goodB = Read_XML(xmlUrlRef, xml);
					CFReportUnitTest("Reading XML file", !goodB);
					
					if (goodB) {
						//xml.for_each(CParseXML());
					}
				}
			}
		}
	}
		
	CCFLog()(CFSTR("------------------CFTest Tests Ended--------------\n"));
}
