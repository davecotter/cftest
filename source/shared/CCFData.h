//	CFDataRef.h

#ifndef _H_CFDataRef
#define _H_CFDataRef

#include "SuperString.h"

class CCFData : public ScCFReleaser<CFDataRef> {
	typedef ScCFReleaser<CFDataRef> _inherited;
	
	public:
	/*
		 usage:
		 CCFData		dataRef;							//	nothing allocated
		 CCFData		dataRef(NULL);						//	nothing allocated
		 CCFData		dataRef(NULL, false);				//	nothing allocated
		 CCFData		dataRef(NULL, true);				//	nothing allocated
		 
		 CCFData		dataRef(existingDataRef);			//	xfer ownership (WILL be released in d'tor)
		 CCFData		dataRef(existingDataRef, false);	//	xfer ownership (WILL be released in d'tor)
		 CCFData		dataRef(existingDataRef, true);		//	retain DataRef (will NOT be released, caller responsible for releasing)
	*/
	CCFData(CFDataRef dataRef = NULL, bool retainB = false) : 
		_inherited(dataRef, retainB) 
	{ }
	
	CCFData(const CCFData& other) {
		operator=(other);
	}
	
	CCFData(CFDictionaryRef dict) {
		i_typeRef = NULL;
		Set(dict);
	}
	
	CCFData(const UCharVec& uCharVec) {
		i_typeRef = NULL;
		Set(uCharVec);
	}
	
	CCFData(CFStringRef str) {
		i_typeRef = NULL;
		Set(str);
	}
	
	CCFData(const UInt8 *beginP, size_t sizeL) {
		i_typeRef = NULL;
		assign(beginP, sizeL);
	}

	CCFData(const SuperString& str) {
		i_typeRef = NULL;
		Set(str.ref());
	}
	
	CCFData(CFURLRef urlRef, bool scan_xml_for_tag_windowslatin1_defaultB = false);

	/****************************************/
	CCFData&	operator=(CFDataRef dataRef) {
		SetAndRetain(dataRef);
		return *this;
	}
	
	CCFData&	operator=(const CCFData& other) {
		return operator=(other.i_typeRef);
	}

	CCFData&	operator=(CFStringRef str){
		Set(str);
		return *this;
	}
	
	CCFData&	operator=(const SuperString& other){
		Set(other.ref());
		return *this;
	}
	
	operator SuperString() const {
		CCFString		cfStr(CopyCFStringRef());
		
		return cfStr.ref();
	}
	
	/*****************************************************/
	const UInt8&	operator[](size_t indexL) const {	ETX(empty()); return CFDataGetBytePtr(i_typeRef)[indexL];	}
	bool			empty() const	{	return size() == 0;	}
	size_t			size() const	{	return i_typeRef ? CFDataGetLength(i_typeRef) : 0;	}
	
	const UInt8*	begin() const {	return &operator[](0);		}
	const UInt8*	end()	const {	return &operator[](size());	}
	
	bool			IsValid() const	{	return i_typeRef != NULL;	}
	
	/*****************************************************/
	OSStatus			Download_Resource(
		CFURLRef			urlRef, 
		CFDictionaryRef		*headerDictP0 = NULL, 
		bool				*scan_xml_for_tag_windowslatin1_defaultBP0 = NULL);

	CFDictionaryRef		CopyAs_Dict(bool mutableB);
	
	void		Set(CFDictionaryRef dict, CFDictionaryRef *errorDictP0 = NULL);
	
	void		Set(CFStringRef ref) {
		adopt(CFStringCreateExternalRepresentation(
			kCFAllocatorDefault, ref, kCFStringEncodingUTF8, 0));
	}

	void		Set(const SuperString& str) {
		Set(str.ref());
	}
	
	void		Set(const UCharVec& vec) {
		assign(vec.begin(), vec.end());
	}
	
	void		assign(const UInt8 *beginP, size_t sizeL) {
		ScCFReleaser<CFDataRef>		dataRef(CFDataCreate(kCFAllocatorDefault, beginP, sizeL));
		
		if (dataRef.Get() == NULL) ETX(memFullErr);
		adopt(dataRef.transfer());
	}
	
	void		assign(const UInt8 *beginP, const UInt8 *endP) {
		assign(beginP, endP - beginP);
	}
	
	void		assign(UCharVec::const_iterator beginRef, UCharVec::const_iterator endRef) {
		assign(&*beginRef, std::distance(beginRef, endRef));
	}
	
	/**************************************************/
	void		enmutable() {
		CFMutableDataRef	dataRef(CFDataCreateMutableCopy(kCFAllocatorDefault, 0, i_typeRef));
		
		adopt(dataRef);
	}
	
	/************************/
	//	vvvv  for the below functions:
	//	only call these if you've called enmutable()
	CFMutableDataRef	GetMutable()	{ return ((CFMutableDataRef)i_typeRef); }
	UInt8 *				mutable_begin()	{ ETX(empty()); return CFDataGetMutableBytePtr(GetMutable()); }

	void		pad(size_t padL);	
	void		pad_to_multiple(size_t multipleL);
	
	void		Scramble();
	void		Unscramble();
	//	only call these if you've called enmutable()
	//	^^^^  for the above functions:
	/************************/
	
	CFDataRef		Copy() const {
		if (i_typeRef) {
			CFRetainDebug(i_typeRef);
		}
		
		return Get();
	}

	CFStringRef		CopyCFStringRef() const {
		CFStringRef		cfStr = NULL;
		
		if (!empty()) {
			cfStr = CFStringCreateFromExternalRepresentation(
				kCFAllocatorDefault, i_typeRef, kCFStringEncodingUTF8);
		} else {
			cfStr = CFCopyEmptyString();
		}
		
		return cfStr;
	}
	
	void			GetAs_Vec(UCharVec *vecP) {
		vecP->resize(size());
		std::copy(begin(), end(), vecP->begin());
	}
};

#endif
