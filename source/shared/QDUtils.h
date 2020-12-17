/*	QDUtils.h
 *
 *		Core math routines
 */
 
#ifndef _H_QDUtils
#define _H_QDUtils

#include "CFMacTypes.h"

#if _QT_ || (OPT_WINOS && !_KJAMS_ && !_QTServer_)

	#if !defined(__QUICKDRAW__)

		//	QuickDraw somehow gets included in kJams, but NOT in CFTest? (both on mac)

		struct RGBColor {
			unsigned short      red;                    /*magnitude of red component*/
			unsigned short      green;                  /*magnitude of green component*/
			unsigned short      blue;                   /*magnitude of blue component*/
		};
		typedef struct RGBColor		RGBColor;

		struct ColorSpec {
		  short               value;                  /*index or other value*/
		  RGBColor            rgb;                    /*true color*/
		};
		typedef struct ColorSpec                ColorSpec;
		typedef ColorSpec *                     ColorSpecPtr;
		typedef ColorSpec                       CSpecArray[1];

		struct ColorTable {
		  long                ctSeed;                 /*unique identifier for table*/
		  short               ctFlags;                /*high bit: 0 = PixMap; 1 = device*/
		  short               ctSize;                 /*number of entries in CTTable*/
		  CSpecArray          ctTable;                /*array [0..0] of ColorSpec*/
		};
		typedef struct ColorTable               ColorTable;
		typedef ColorTable *                    CTabPtr;
		typedef CTabPtr *                       CTabHandle;
	#endif	//	!defined(__QUICKDRAW__)

	#ifndef __CONTROLS__
	struct ControlFontStyleRec {
	  SInt16              flags;
	  SInt16              font;
	  SInt16              size;
	  SInt16              style;
	  SInt16              mode;
	  SInt16              just;
	  RGBColor            foreColor;
	  RGBColor            backColor;
	};

	typedef SInt16                          ControlContentType;

	struct ControlButtonContentInfo {
	  ControlContentType  contentType;
	  union {
		SInt16              resID;
		void	*extraP;
	  }                       u;
	};
	typedef struct ControlButtonContentInfo ControlButtonContentInfo;
	#endif // __CONTROLS__

#endif	//	_QT_

#if !defined(_JUST_CFTEST_)

	void		QD_SetRect(Rect *r, short left, short top, short right, short bottom);
	void		QD_OffsetRect(Rect *r, short x, short y);
	void		QD_InsetRect(Rect *r, short x, short y);
	Boolean		QD_EmptyRect(const Rect *r);
	bool		QD_EqualRect(const Rect *a, const Rect *b);
	bool		QD_EqualRectSize(const Rect& a, const Rect& b);

	void		QD_SetRectPos(Rect& ioR, Point posPt);
	Point		QD_RectPos(const Rect& ioR);

	void		QD_SetRectWidth(Rect& ioR, short widthS);
	short		QD_RectWidth(const Rect& r);

	void		QD_SetRectHeight(Rect& ioR, short heightS);
	short		QD_RectHeight(const Rect& r);

	void		QD_SetRectSize(Rect& ioR, const Point& sizePt);
	Point		QD_RectSize(const Rect& r);

	Boolean		QD_SectRect(const Rect *a, const Rect *b, Rect *out);
	void		QD_UnionRect(const Rect *a, const Rect *b, Rect *out);
	RectVec		QD_DiffRect(const Rect &lhsR, const Rect &rhsR);
	Boolean		QD_PtInRect(Point pt, const Rect *r);
	void		QD_Pt2Rect(Point a, Point b, Rect *r);
	void		QD_ScaleRect(Rect *r, float scaleF);
	bool		QD_EqualPt(const Point &a, const Point &b);

	void		QD_MapRect(
		Rect		*io_frameR,
		const Rect	*srcR,
		const Rect	*dstR);

	void		QD_LocalToGlobal(Point *pt);
	void		QD_GlobalToLocal(Point *pt);

	#define		QD_TopLeft(r)	(((Point *) &(r))[0])
	#define		QD_BotRight(r)	(((Point *) &(r))[1])

	bool	operator == (const Rect &a, const Rect &b);
	bool	operator != (const Rect &a, const Rect &b);

	bool	operator == (const Point &a, const Point &b);
	bool	operator != (const Point &a, const Point &b);

	Point	operator + (const Point &a, const Point &b);
	Point	operator - (const Point &a, const Point &b);
	Point&	operator += (Point &a, const Point &b);
	Point&	operator -= (Point &a, const Point &b);
	Point&	operator - (Point &a);

	Point		QD_GetNullPoint();
	bool		QD_IsNullPoint(const Point& pt);

#if OPT_MACOS && !_QT_ && !_CFTEST_ && !_PaddleServer_
	#define	OPT_HAS_GDHANDLE	1
#else
	#define	OPT_HAS_GDHANDLE	0
#endif

#if _QT_
	class QScreen;
	typedef		QScreen*	DisplayRef;
#elif OPT_HAS_GDHANDLE
	typedef		GDHandle	DisplayRef;
#else
	typedef		void*		DisplayRef;
#endif

	DisplayRef		GetEnclosingDevice(const Rect *theRect);

	Rect			ReturnEnclosingDeviceRect(const Rect *theRectPtr);

	// ********************************************************************
	//	CG suff
	#if !_PaddleServer_ && !__OBJC__
	CGRect		CG_MapRect(CGRect dstR, const CGRect& fromR, CGRect toR);
	CGRect		CGRectRound(CGRect rect, bool snap_originB = false);

	Rect		RectFromHIRect(const HIRect& in_cgR);
	HIRect		HIRectFromRect(const Rect &r);
	#endif

	#if OPT_WINOS
		int		CGRectIsEmpty(CGRect rect);

		CGSize	CGSizeMake(
		  float   width,
		  float   height);

		CGPoint	CGPointMake(
		  float   width,
		  float   height);
	#endif
	
#endif //	!defined(_JUST_CFTEST_)

#endif //	_H_QDUtils
