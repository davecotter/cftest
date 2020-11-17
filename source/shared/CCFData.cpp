//	CCFData
#include "stdafx.h"
#include "CFUtils.h"
#include "CCFData.h"

#if defined(_KJAMS_)
	#include "CAS_Adapters.h"
	#include "CTaskMgr.h"
#endif

OSStatus	CCFData::Download_Resource(
	CFURLRef			urlRef, 
	CFDictionaryRef		*headerDictP0, 
	bool				*scan_xml_for_tag_windowslatin1_defaultBP0)
{
	OSStatus		err = noErr;
	
	#if defined(_KJAMS_)
	std::auto_ptr<CTask>		sc;
	
	if (CTaskMgr::Get() && !gApp->IsStartup()) {
		sc.reset(new CTask(urlRef, 0));
	}
	#endif
	
	if (!CFURLCreateDataAndPropertiesFromResource(
		kCFAllocatorDefault, urlRef, AddressOf(), headerDictP0, NULL, &err)
	) {
		ERR(urlDataHHTTPURLErr);
	}
	
	if (
		!err 
		&& scan_xml_for_tag_windowslatin1_defaultBP0 
		&& *scan_xml_for_tag_windowslatin1_defaultBP0
	) {
		SuperString			str;
		
		str.Set(Get());
		
		if (!str.Contains(kXML_EncodingKeyValUTF8)) {
			CF_ASSERT(!str.Contains(kXML_EncodingKey "="));
			str.Set(Get(), kCFStringEncodingWindowsLatin1);
			adopt(str.CopyDataRef());
		}
	}
	
	return err;
}

void	CCFData::Set(CFDictionaryRef dict, CFDictionaryRef *errorDictP0)
{
	CCFError	cfErr;
	
	adopt(CCFPropertyListCreateXMLData(dict, errorDictP0 ? cfErr.AddressOf() : NULL));
	
	if (errorDictP0 && cfErr.Get()) {
		*errorDictP0 = CFErrorCopyAsDict(cfErr);
	}
}

CFDictionaryRef		CCFData::CopyAs_Dict(bool mutableB)
{
	CCFDictionary		dict;
	SuperString			dataStr;

	if (!empty()) {
		char		ch(operator[](0));
		
		dataStr.append(ch);
	}
	
	if (dataStr.IsJSON()) {
		dict.SetJSON(*this);
		
	} else {
		dict.adopt((CFDictionaryRef)CFPropertyListCreateFromXMLData(
			kCFAllocatorDefault, i_typeRef,
			mutableB
				? kCFPropertyListMutableContainersAndLeaves
				: kCFPropertyListImmutable,
			NULL));
	}
	
	return dict.transfer();
}


CCFData::CCFData(CFURLRef urlRef, bool scan_xml_for_tag_windowslatin1_defaultB)
{
	ETX(Download_Resource(urlRef, NULL, &scan_xml_for_tag_windowslatin1_defaultB));
}

void		CCFData::pad(size_t padL)
{
	if (padL) {
		CFDataIncreaseLength(GetMutable(), padL);
	}
}

void		CCFData::pad_to_multiple(size_t multipleL)
{
	size_t		leftoverL = size() % multipleL;
	
	if (leftoverL) {
		pad(multipleL - leftoverL);
	}

	leftoverL = size() % multipleL;
	CF_ASSERT(leftoverL == 0);
}

void		CCFData::Scramble()
{
#if defined(_KJAMS_)
	size_t			multipleL(sizeof(UInt32));

	enmutable();
	pad_to_multiple(multipleL);
	
	UInt32		*bitsA(reinterpret_cast<UInt32 *>(mutable_begin()));
	size_t		sizeL(size());
	size_t		wordsL(sizeL / multipleL);
	
	ScrambleBits(true, bitsA, wordsL);
#endif
}

void		CCFData::Unscramble()
{
#if defined(_KJAMS_)
	size_t			multipleL(sizeof(UInt32));

	enmutable();
		
	UInt32		*bitsA(reinterpret_cast<UInt32 *>(mutable_begin()));
	size_t		sizeL(size());
	size_t		wordsL(sizeL / multipleL);
	
	CF_ASSERT(sizeL % multipleL == 0);
	ScrambleBits(false, bitsA, wordsL);
#endif
}
