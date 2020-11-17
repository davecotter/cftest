// MacAlias.cpp

#ifdef _KJAMS_
#include "stdafx.h"
#include "SuperString.h"
#endif

#include <stdint.h>
#include <cassert>
#include <string>
#include <map>
#include <vector>
#include "MacAlias.h"

typedef uint16_t					MA_UTF16Char;
typedef std::vector<MA_UTF16Char>	MA_UTF16Vec;

// ------------------------------------------------------------------------
std::string		MA_ConvertUTF16_BE_to8(const MA_UTF16Vec& vec)
{
	std::string		str;

	#ifdef _KJAMS_
	if (!vec.empty()) {
		UTF8Char		*begin_bytesP((UTF8Char *)&*vec.begin());
		UTF8Char		*end_bytesP(begin_bytesP + (vec.size() * 2));
		SuperString		sstr;

		sstr.assign(begin_bytesP, end_bytesP, kCFStringEncodingUTF16BE);
		str = sstr.utf8Z();
	}
	#else
	//	provide your own implementation here
	#endif

	return str;
}

void		MA_LogString(const char *strZ)
{
	#ifdef _KJAMS_
		Log(strZ);
	#else
		//	provide your own implementation here
		fprintf(stdout, "%s\n", strZ);
	#endif
}

// ------------------------------------------------------------------------
enum {
	kAliasRecV2_NONE	= -2,
	kAliasRecV2_END,

	kAliasRecV2_DIR_NAME,		//	0
	kAliasRecV2_DIR_ID,
	kAliasRecV2_HFS_PATH,
	kAliasRecV2_ZONE,
	kAliasRecV2_SERVER,
	kAliasRecV2_USER,			//	5
	kAliasRecV2_DRIVER,
	kAliasRecV2_APPLESHARE,
	kAliasRecV2_DIALUP,
	kAliasRecV2_unk_09,
	kAliasRecV2_unk_10,			//	10
	kAliasRecV2_unk_11,
	kAliasRecV2_unk_12,
	kAliasRecV2_unk_13,
	kAliasRecV2_UTF16_NAME,
	kAliasRecV2_UTF16_VOLUME,	//	15
	kAliasRecV2_unk_16,
	kAliasRecV2_unk_17,
	kAliasRecV2_PosixPath,
	kAliasRecV2_PosixRoot,
	kAliasRecV2_unk_20,			//	20
	kAliasRecV2_unk_21,
};
typedef int16_t	AliasRecV2Type;

typedef std::map<AliasRecV2Type, std::string>	AliasTypeToStrMap;

static
std::string		num_to_string(int num)
{
	char	buffer[100] = {0};
	
	sprintf(buffer, "%d", num);
	return buffer;
}

#define	ADD_TO_STRMAP(_foo)		s_strMap[_foo] = #_foo;

static
std::string			AliasRecTypeToStr(AliasRecV2Type type)
{
	std::string					str;
	static AliasTypeToStrMap	s_strMap;

	if (s_strMap.empty()) {
		ADD_TO_STRMAP(kAliasRecV2_NONE);
		ADD_TO_STRMAP(kAliasRecV2_END);
		ADD_TO_STRMAP(kAliasRecV2_DIR_NAME);
		ADD_TO_STRMAP(kAliasRecV2_DIR_ID);
		ADD_TO_STRMAP(kAliasRecV2_HFS_PATH);
		ADD_TO_STRMAP(kAliasRecV2_ZONE);
		ADD_TO_STRMAP(kAliasRecV2_SERVER);
		ADD_TO_STRMAP(kAliasRecV2_USER);
		ADD_TO_STRMAP(kAliasRecV2_DRIVER);
		ADD_TO_STRMAP(kAliasRecV2_APPLESHARE);
		ADD_TO_STRMAP(kAliasRecV2_DIALUP);
		ADD_TO_STRMAP(kAliasRecV2_UTF16_NAME);
		ADD_TO_STRMAP(kAliasRecV2_UTF16_VOLUME);
		ADD_TO_STRMAP(kAliasRecV2_PosixPath);
		ADD_TO_STRMAP(kAliasRecV2_PosixRoot);
	}

	AliasTypeToStrMap::iterator		it(s_strMap.find(type));

	if (it == s_strMap.end()) {
		str = "kAliasRecV2_unk_";
		str.append(num_to_string(type));
	} else {
		str = it->second;
	}

	return str;
}

static
void		LogRecMap(const AliasTypeToStrMap& strMap)
{
	#ifdef kDEBUG
		BOOST_FOREACH(const AliasTypeToStrMap::value_type& pair, strMap) {
			char			bufAC[256];
			std::string		typeStr(AliasRecTypeToStr(pair.first));
			
			sprintf(bufAC, "%s: \"%s\"", typeStr.c_str(), pair.second.c_str());
			MA_LogString(bufAC);
		}
	#endif
}

// -------------------------------------------------------------

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=packed
#elif PRAGMA_STRUCT_PACKPUSH
    #pragma pack(push, 1)
#elif PRAGMA_STRUCT_PACK
    #pragma pack(1)
#endif

typedef struct {
	/*
		4 bytes user type name/app creator code = long ASCII text string (none = 0)
		2 bytes record size = short unsigned total length
		2 bytes record version = short integer version (current version = 2)
		2 bytes alias kind = short integer value (file = 0; directory = 1)
	*/

	int32_t	creator;
	int16_t	recordSize;
	int16_t	version;
	int16_t	recordType;	//	file=0, folder=1

	union {

		//	from https://en.wikipedia.org/wiki/Alias_(Mac_OS)#Alias_record_structure_outside_of_size_length

		/*
			1 byte volume name string length = byte unsigned length
			27 bytes volume name string (if volume name string < 27 chars then pad with zeros)

			4 bytes volume created mac date = long unsigned value in seconds since beginning 1904 to 2040
			2 bytes volume signature = short unsigned HFS value
			2 bytes volume type = short integer mac os value (types are Fixed HD = 0; Network Disk = 1; 400kB FD = 2;800kB FD = 3; 1.4MB FD = 4; Other Ejectable Media = 5 )
			4 bytes parent directory id = long unsigned HFS value

			1 bytes file name string length = byte unsigned length
			63 bytes file name string (if file name string < 63 chars then pad with zeros)

			4 bytes file number = long unsigned HFS value
			4 bytes file created mac date = long unsigned value in seconds since beginning 1904 to 2040
			4 bytes file type name = long ASCII text string
			4 bytes file creator name = long ASCII text string
			2 bytes nlvl From (directories from alias thru to root) = short integer range
			2 bytes nlvl To (directories from root thru to source) = short integer range (if alias on different volume then set above to -1)
			4 bytes volume attributes = long hex flags
			2 bytes volume file system id = short integer HFS value
			10 bytes reserved = 80-bit value set to zero

			4+ bytes optional extra data strings =
				short integer type +
				short unsigned string length

				Extended Info End = -1;
				Directory Name = 0;
				Directory IDs = 1;
				Absolute Path = 2;
				AppleShare Zone Name = 3;
				AppleShare Server Name = 4;
				AppleShare User Name = 5;
				Driver Name = 6;
				Revised AppleShare info = 9;
				AppleRemoteAccess dialup info = 10

			string data = hex dump

			odd lengths have a 1 byte odd string length pad = byte value set to zero
		*/

		struct {
			char				volNameLen;
			char				volName[27];
			int32_t		volCreDate;
			int16_t		volSignature;
			int16_t		volType;
			int32_t		parDirID;
			char				fileNameLen;
			char				fileName[63];
			int32_t		fileNum;
			int32_t		fileCreDate;
			int32_t		fileType;
			int32_t		fileCreator;
			int16_t		levelsFromAliasToRoot;
			int16_t		levelsFromRootToSource;
			int32_t		volAttrs;
			int16_t		fsID;
			char				reserved[10];

			char				start_of_str_stuff;

			/*
			from here starts the variable records.

			repeat:
				AliasRecV2Type		strType;
				int16_t				strLen;
				char				str[strLen + pad byte if strLen is odd]
			*/
		} v2;

		//	from https://web.archive.org/web/20170222235430/http://sysforensics.org/2016/08/mac-alias-data-objects/
		struct {
			int16_t		padding0;
			int32_t		volCheckedDate;
			int32_t		fsType;
			int32_t		unknown0;
			int32_t		parentCNID;
			int32_t		targetCNID;
			int16_t		unknown1;
			int32_t		creDate;
			char				staticSomething[22];
			int32_t		num_CNID_bytes;
			int32_t		CNIDs;

			char				start_of_str_stuff;
			/*
			from here starts the variable records
			int16_t		totalLen0
				int16_t		nameKeyLen;
				uchar		name[];
			int16_t		unk0;

			int16_t		totalLen1;
				int16_t		volNameLen;
				uchar		volName[]
			int16_t		unk1;

			int16_t		totalLen2;
				int16_t		pathLen;
				uchar		path[]
			int16_t		unk2;

			char		unknown3[9] // ?
			*/
		} v3;
	} u;
} MacAliasRec;

#if PRAGMA_STRUCT_ALIGN
    #pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
    #pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
    #pragma pack()
#endif

// -------------------------------------------------------------
static
int32_t			EndianSwap_BigToHost_32Copy(const int32_t& val)
{
	return ntohl(val);
}

static
void			EndianSwap_BigToHost_32(int32_t& ioVal)
{
	ioVal = EndianSwap_BigToHost_32Copy(ioVal);
}

static
int16_t			EndianSwap_BigToHost_16Copy(const int16_t& val)
{
	return ntohs(val);
}

static
void			EndianSwap_BigToHost_16(int16_t& ioVal)
{
	ioVal = EndianSwap_BigToHost_16Copy(ioVal);
}
// -------------------------------------------------------------

void	SwapAliasRec(MacAliasRec& aliasRec)
{
	EndianSwap_BigToHost_32(aliasRec.creator);
	EndianSwap_BigToHost_16(aliasRec.recordSize);
	EndianSwap_BigToHost_16(aliasRec.version);
	EndianSwap_BigToHost_16(aliasRec.recordType);

	switch (aliasRec.version) {

		case 2: {
			EndianSwap_BigToHost_32(aliasRec.u.v2.volCreDate);
			EndianSwap_BigToHost_16(aliasRec.u.v2.volSignature);
			EndianSwap_BigToHost_16(aliasRec.u.v2.volType);
			EndianSwap_BigToHost_32(aliasRec.u.v2.parDirID);
			EndianSwap_BigToHost_32(aliasRec.u.v2.fileNum);
			EndianSwap_BigToHost_32(aliasRec.u.v2.fileCreDate);
			EndianSwap_BigToHost_32(aliasRec.u.v2.fileType);
			EndianSwap_BigToHost_32(aliasRec.u.v2.fileCreator);
			EndianSwap_BigToHost_16(aliasRec.u.v2.levelsFromAliasToRoot);
			EndianSwap_BigToHost_16(aliasRec.u.v2.levelsFromRootToSource);
			EndianSwap_BigToHost_32(aliasRec.u.v2.volAttrs);
			EndianSwap_BigToHost_16(aliasRec.u.v2.fsID);
		} break;

		case 3: {
			EndianSwap_BigToHost_32(aliasRec.u.v3.volCheckedDate);
			EndianSwap_BigToHost_32(aliasRec.u.v3.fsType);
			EndianSwap_BigToHost_32(aliasRec.u.v3.parentCNID);
			EndianSwap_BigToHost_32(aliasRec.u.v3.targetCNID);
			EndianSwap_BigToHost_32(aliasRec.u.v3.creDate);
			EndianSwap_BigToHost_32(aliasRec.u.v3.num_CNID_bytes);
			EndianSwap_BigToHost_32(aliasRec.u.v3.CNIDs);
		} break;
	}
}

#define	MA_ASSERT(_expr)			assert(_expr)
#define MA_IntegerIsEven(_int)		((_int & 0x01) == 0)

static
void	ExtractV2String(const char **offsetPP, AliasRecV2Type *typeP, std::string *strP)
{
	int16_t			byteLen;

	strP->clear();

	*typeP = EndianSwap_BigToHost_16Copy(*(int16_t *)*offsetPP);
	*offsetPP += 2;

	byteLen = EndianSwap_BigToHost_16Copy(*(int16_t *)*offsetPP);
	*offsetPP += 2;

	if (
		   *typeP <= kAliasRecV2_DIALUP
		|| *typeP == kAliasRecV2_PosixPath
		|| *typeP == kAliasRecV2_PosixRoot
	) {
		strP->assign(*offsetPP, byteLen);

		if (!MA_IntegerIsEven(byteLen)) {
			++byteLen;
		}
	} else {
		int16_t			strLen	= EndianSwap_BigToHost_16Copy(*(int16_t *)*offsetPP);
		int16_t			strLen2	= (byteLen >> 1) - 1;

		MA_ASSERT(MA_IntegerIsEven(byteLen));

		if (byteLen <= 2) {
			strLen = 0;
		}

		if (strLen) {
			MA_ASSERT(strLen == strLen2);

			MA_UTF16Char	*uni16P((MA_UTF16Char *)(*offsetPP + 2));
			MA_UTF16Vec		uniStr(uni16P, uni16P + strLen);

			*strP = MA_ConvertUTF16_BE_to8(uniStr);
		}
	}

	*offsetPP += byteLen;
}

static
std::string		ExtractV3String(const char **offsetPP)
{
	int16_t		byteLen;
	std::string			str;

	*offsetPP += 2;

	byteLen = EndianSwap_BigToHost_16Copy(*(int16_t *)*offsetPP);
	*offsetPP += 2;

	str.assign(*offsetPP, byteLen);

	if (!MA_IntegerIsEven(byteLen)) {
		// ++byteLen; ????
	}

	*offsetPP += byteLen;
	return str;
}

static std::string		GetMapStringIf(const AliasTypeToStrMap& strMap, AliasRecV2Type type)
{
	std::string								str;
	AliasTypeToStrMap::const_iterator		it(strMap.find(type));

	if (it != strMap.end()) {
		str += it->second;
	}

	return str;
}

std::string			MacAlias_Resolve(const char* charP, size_t sizeL)
{
	std::string		fullPath;

	if (sizeL > sizeof(MacAliasRec)) {
		const MacAliasRec&		aliasRec(*(const MacAliasRec *)charP);
		MacAliasRec				swapped_aliasRec(*(MacAliasRec *)charP);

		SwapAliasRec(swapped_aliasRec);

		switch (swapped_aliasRec.version) {

			case 2: {
				#ifdef kDEBUG
				std::string			volNameStr(&swapped_aliasRec.u.v2.volName[0], swapped_aliasRec.u.v2.volNameLen);
				std::string			fileNameStr(&swapped_aliasRec.u.v2.fileName[0], swapped_aliasRec.u.v2.fileNameLen);
				#endif

				AliasTypeToStrMap	strMap;

				{
					std::string			str;
					AliasRecV2Type		strType(kAliasRecV2_NONE);
					const char			*offsetP(&aliasRec.u.v2.start_of_str_stuff);

					do {
						ExtractV2String(&offsetP, &strType, &str);

						if (strType != kAliasRecV2_END && !str.empty()) {
							strMap[strType] = str;
						}
					} while (strType != kAliasRecV2_END);
				}

				//LogRecMap(strMap);

				fullPath += GetMapStringIf(strMap, kAliasRecV2_PosixRoot);
				fullPath += GetMapStringIf(strMap, kAliasRecV2_PosixPath);
			} break;

			case 3: {
				#ifdef kDEBUG
				const char		*offsetP(	&aliasRec.u.v3.start_of_str_stuff);
				std::string		nameStr(	ExtractV3String(&offsetP));
				std::string		volName2Str(ExtractV3String(&offsetP));
				std::string		pathStr(	ExtractV3String(&offsetP));

				int i = 0;
				#endif

				//	never got a file to debug with
				CF_ASSERT(0);
			} break;
		}
	}


	return fullPath;
}
