/*	CoreMath.cpp
 *
 *		Core math routines
 */

#include "stdafx.h"
#include "MinMax.h"
#include "QDUtils.h"
#include "CFUtils.h"

void	QD_ScaleRect(Rect *frameRP, float scaleF)
{
	frameRP->left	*= scaleF;
	frameRP->right	*= scaleF;
	frameRP->top	*= scaleF;
	frameRP->bottom	*= scaleF;
}

void	QD_SetRect(Rect *r, short left, short top, short right, short bottom)
{
	r->left = left;
	r->top = top;
	r->right = right;
	r->bottom = bottom;
}

void	QD_OffsetRect(Rect *r, short x, short y)
{
	r->left += x;
	r->right += x;
	r->top += y;
	r->bottom += y;
}

void	QD_InsetRect(Rect *r, short x, short y)
{
	r->left += x;
	r->right -= x;
	r->top += y;
	r->bottom -= y;
}

Boolean		QD_EmptyRect(const Rect *r)
{
	return QD_RectWidth(*r) <= 0 || QD_RectHeight(*r) <= 0;
}

bool	QD_EqualRect(const Rect *a, const Rect *b)
{	
	if (a->left != b->left) return false;
	if (a->top != b->top) return false;
	if (a->right != b->right) return false;
	if (a->bottom != b->bottom) return false;
	return true;
}

bool	QD_EqualPt(const Point &a, const Point &b)
{
	if (a.h != b.h) return false;
	if (a.v != b.v) return false;
	return true;
}

Point		QD_GetNullPoint()
{
	Point emptyPt;

	emptyPt.h = 0;
	emptyPt.v = 0;
	return emptyPt;
}

bool		QD_IsNullPoint(const Point& pt)
{
	return pt == QD_GetNullPoint();
}

short		QD_RectWidth(const Rect& r)
{
	return r.right - r.left;
}

short		QD_RectHeight(const Rect& r)
{
	return r.bottom - r.top;
}

void		QD_SetRectWidth(Rect& ioR, short widthS)
{
	ioR.right = ioR.left + widthS;
}

void		QD_SetRectHeight(Rect& ioR, short heightS)
{
	ioR.bottom = ioR.top + heightS;
}

Point		QD_RectSize(const Rect& r)
{
	Point		sizePt;

	sizePt.h = QD_RectWidth(r);
	sizePt.v = QD_RectHeight(r);
	return sizePt;
}

void		QD_SetRectSize(Rect& ioR, const Point& sizePt)
{
	QD_SetRectWidth(ioR, sizePt.h);
	QD_SetRectHeight(ioR, sizePt.v);
}

void		QD_SetRectPos(Rect& ioR, Point posPt)
{
	QD_OffsetRect(&ioR, -ioR.left + posPt.h, -ioR.top + posPt.v);
}

Point		QD_RectPos(const Rect& ioR)
{
	return QD_TopLeft(ioR);
}

bool	QD_EqualRectSize(const Rect& a, const Rect& b)
{	
	if (QD_RectWidth(a) != QD_RectWidth(b)) return false;
	if (QD_RectHeight(a) != QD_RectHeight(b)) return false;
	return true;
}

Boolean		QD_SectRect(const Rect *a, const Rect *b, Rect *out)
{
	Boolean		has_some_areaB = true;
	
	out->left	= math_max(a->left,b->left);
	out->right	= math_min(a->right,b->right);
	out->top	= math_max(a->top,b->top);
	out->bottom	= math_min(a->bottom,b->bottom);
	
	has_some_areaB = !QD_EmptyRect(out);
	
	if (!has_some_areaB) {
		QD_SetRect(out,0,0,0,0);
	}
	
	return has_some_areaB;
}


// https://stackoverflow.com/questions/25068538/intersection-and-difference-of-two-rectangles
//https://stackoverflow.com/questions/5144615/difference-xor-between-two-rectangles-as-rectangles

static Rect		MakeRect(short left, short right, short top, short bottom)
{
	Rect		outR;
	
	QD_SetRect(&outR, left, top, right, bottom);
	return outR; 
}

RectVec		QD_DiffRect(const Rect& lhsR, const Rect& rhsR)
{
	RectVec		rectVec;
	short		a = math_min( lhsR.left, rhsR.left );
	short		b = math_max( lhsR.left, rhsR.left );
	short		c = math_min( lhsR.right, rhsR.right );
	short		d = math_max( lhsR.right, rhsR.right );

	short		e = math_min( lhsR.top, rhsR.top );
	short		f = math_max( lhsR.top, rhsR.top );
	short		g = math_min( lhsR.bottom, rhsR.bottom );
	short		h = math_max( lhsR.bottom, rhsR.bottom );

	// X = intersection, 0-7 = possible difference areas
	// h +-+-+-+
	// . |5|6|7|
	// g +-+-+-+
	// . |3|X|4|
	// f +-+-+-+
	// . |0|1|2|
	// e +-+-+-+
	// . a b c d

	// we'll always have rectangles 1, 3, 4 and 6
	rectVec.push_back(MakeRect( b, c, e, f ));
	rectVec.push_back(MakeRect( a, b, f, g ));
	rectVec.push_back(MakeRect( c, d, f, g ));
	rectVec.push_back(MakeRect( b, c, g, h ));

	// decide which corners
	if( lhsR.left == a && lhsR.top == e || rhsR.left == a && rhsR.top == e ) {
		// corners 0 and 7
		rectVec.push_back(MakeRect( a, b, e, f ));
		rectVec.push_back(MakeRect( c, d, g, h ));
	} else { 
		// corners 2 and 5
		rectVec.push_back(MakeRect( c, d, e, f ));
		rectVec.push_back(MakeRect( a, b, g, h ));
	}

	return rectVec;
}
	
void		QD_UnionRect(const Rect *a, const Rect *b, Rect *out)
{
	if (QD_EmptyRect(a)) {
		*out = *b;
	} else if (QD_EmptyRect(b)) {
		*out = *a;
	} else {
		out->left	= math_min(a->left,b->left);
		out->right	= math_max(a->right,b->right);
		out->top	= math_min(a->top,b->top);
		out->bottom	= math_max(a->bottom,b->bottom);
	}
}

Boolean		QD_PtInRect(Point pt, const Rect *r)
{
	return	   (pt.h >= r->left) 
			&& (pt.h <  r->right) 
			&& (pt.v >= r->top) 
			&& (pt.v <  r->bottom);
}

void		QD_Pt2Rect(Point a, Point b, Rect *r)
{
	r->left		= math_min(a.h,b.h);
	r->right	= math_max(a.h,b.h);
	r->top		= math_min(a.v,b.v);
	r->bottom	= math_max(a.v,b.v);
}

static CGPoint	CGPointIntegral_low(CGPoint pt, bool roundB = false)
{
	if (roundB) {
		if (pt.x >= 0) {
			pt.x	= (SInt32)math_round(pt.x);
		} else {
			pt.x	= -(SInt32)math_round(-pt.x);
		}

		if (pt.y >= 0) {
			pt.y	= (SInt32)math_round(pt.y);
		} else {
			pt.y	= -(SInt32)math_round(-pt.y);
		}
	} else {
		if (pt.x >= 0) {
			pt.x	= (SInt32)math_ceil(pt.x);
		} else {
			pt.x	= -(SInt32)math_ceil(-pt.x);
		}

		if (pt.y >= 0) {
			pt.y	= (SInt32)math_ceil(pt.y);
		} else {
			pt.y	= -(SInt32)math_ceil(-pt.y);
		}
	}	
	return pt;
}

#if OPT_WINOS || kDEBUG

static HIRect	CGRectIntegral_low(HIRect rect)
{
	CGPoint		botRightPt(CGPointMake(
		rect.origin.x + rect.size.width, 
		rect.origin.y + rect.size.height)); 
	
	rect.origin = CGPointIntegral_low(rect.origin);

	botRightPt = CGPointIntegral_low(botRightPt);
	rect.size.width		= botRightPt.x - rect.origin.x;
	rect.size.height	= botRightPt.y - rect.origin.y;
	
	return rect;
}
#endif

#if OPT_WINOS
// Expand `rect' to the smallest rect containing it with integral origin and size
static HIRect	CGRectIntegral(HIRect rect)
{
	return CGRectIntegral_low(rect);
}

int			CGRectIsEmpty(CGRect rect)
{
	return rect.size.width == 0 || rect.size.height == 0;
}
#endif

static CGPoint		CGPointSubtract(const CGPoint& lhs, const CGPoint& rhs)
{
	CGPoint		diffPt(CGPointMake(
		lhs.x - rhs.x, lhs.y - rhs.y));

	return diffPt;
}

CGRect	CGRectRound(CGRect rect, bool snap_originB)
{
	CGPoint		topLeftPt(rect.origin);
	CGPoint		botRightPt(CGPointMake(
		rect.origin.x + rect.size.width, 
		rect.origin.y + rect.size.height)); 
	
	rect.origin	= CGPointIntegral_low(topLeftPt, true);

	if (snap_originB) {
		CGPoint		diffPt(CGPointSubtract(topLeftPt, rect.origin));

		botRightPt = CGPointSubtract(botRightPt, diffPt);
	}

	botRightPt	= CGPointIntegral_low(botRightPt, true);

	rect.size.width		= botRightPt.x - rect.origin.x;
	rect.size.height	= botRightPt.y - rect.origin.y;
	
	return rect;
}


Rect		RectFromHIRect(const HIRect& in_cgR)
{
	Rect	frameR;
	HIRect	cgR(CGRectIntegral(in_cgR));
	
	#if OPT_MACOS && kDEBUG
	{
		HIRect	cg2R(CGRectIntegral_low(in_cgR));
		bool	same_sizeB(memcmp(&cgR, &cg2R, sizeof(HIRect)) == 0);
		
	//	CF_ASSERT(same_sizeB);
		
		if (!same_sizeB) {
			cg2R = CGRectIntegral_low(in_cgR);
		}
	}
	#endif
	
	QD_SetRect(
		&frameR, 
		(cgR.origin.x), 
		(cgR.origin.y), 
		(cgR.origin.x + cgR.size.width), 
		(cgR.origin.y + cgR.size.height));
		
	return frameR;
}

HIRect		HIRectFromRect(const Rect &r)
{
#if OPT_MACOS
	return CGRectMake(r.left, r.top, r.right - r.left, r.bottom - r.top);
#else
	HIRect	foo = {0};

	foo.origin.x = r.left;
	foo.origin.y = r.top;
	foo.size.width = r.right - r.left;
	foo.size.height = r.bottom - r.top;
	return foo;
#endif
}

CGRect		CG_MapRect(CGRect dstR, const CGRect& fromR, CGRect toR)
{
	float		from_right(fromR.origin.x + fromR.size.width);
	float		from_bottom(fromR.origin.y + fromR.size.height);
	float		to_right(toR.origin.x + toR.size.width);
	float		to_bottom(toR.origin.y + toR.size.height);
	float		dst_right(dstR.origin.x + dstR.size.width);
	float		dst_bottom(dstR.origin.y + dstR.size.height);

	dstR.origin.x = LERP(toR.origin.x, toR.origin.x + toR.size.width, dstR.origin.x, fromR.origin.x, from_right);
	dstR.origin.y = LERP(toR.origin.y, toR.origin.y + toR.size.height, dstR.origin.y, fromR.origin.y, from_bottom);
	
	dst_right  = LERP(toR.origin.x, to_right,  dst_right,  fromR.origin.x, from_right);
	dst_bottom = LERP(toR.origin.y, to_bottom, dst_bottom, fromR.origin.y, from_bottom);
	
	dstR.size.width = dst_right - dstR.origin.x;
	dstR.size.height = dst_bottom - dstR.origin.y;
	
	return dstR;
}

Rect		QD_MapRect_low(
	const Rect&	frameR,
	const Rect&	srcR,
	const Rect&	destR)
{
	Rect		out_frameR(frameR);
	CGRect		cio_frameR(HIRectFromRect(out_frameR));
	CGRect		csrcR(HIRectFromRect(srcR));
	CGRect		cdestR(HIRectFromRect(destR));

	cio_frameR = CG_MapRect(cio_frameR, csrcR, cdestR);
	cio_frameR = CGRectRound(cio_frameR);
	out_frameR = RectFromHIRect(cio_frameR);
	
	return out_frameR;
}

void		QD_MapRect(
	Rect		*io_frameR,
	const Rect	*srcR,
	const Rect	*destR)
{
	Rect		originR(*io_frameR);

	*io_frameR = QD_MapRect_low(originR, *srcR, *destR);
	
	#if kDEBUG && OPT_KJMAC
	{
		Rect		finalR(originR);
		
		MapRect(&finalR, srcR, destR);

		if (!QD_EqualRect(&finalR, io_frameR)) {
		//	CF_ASSERT(false);
			*io_frameR = QD_MapRect_low(originR, *srcR, *destR);
		}
	}
	#endif
}


void		QD_LocalToGlobal(Point *pt)
{
	#if _QT_ || _PaddleServer_
	CFDebugBreak();
	#else
	LocalToGlobal(pt);
	#endif
}

void		QD_GlobalToLocal(Point *pt)
{
	#if _QT_ || _PaddleServer_
	CFDebugBreak();
	#else
	GlobalToLocal(pt);
	#endif
}

bool	operator == (const Rect &a, const Rect &b)
{
	return !!QD_EqualRect(&a, &b);
}

bool	operator != (const Rect &a, const Rect &b)
{
	return !(a == b);
}

bool	operator == (const Point &a, const Point &b)
{
	return QD_EqualPt(a, b);
}

bool	operator != (const Point &a, const Point &b)
{
	return !(a == b);
}

Point	operator + (const Point &a, const Point &b)
{
	Point	p;
	
	p.h = a.h + b.h;
	p.v = a.v + b.v;
	return p;
}

Point	operator - (const Point &a, const Point &b)
{
	Point	p;
	
	p.h = a.h - b.h;
	p.v = a.v - b.v;
	return p;
}

Point&	operator += (Point &a, const Point &b)
{
	a = a + b;
	return a;
}

Point&	operator -= (Point &a, const Point &b)
{
	a = a - b;
	return a;
}

Point&	operator - (Point &a)
{
	a.h = -a.h;
	a.v = -a.v;
	return a;
}

#if OPT_HAS_GDHANDLE
	#define	FOR_EACH_GDEVICE(_GDHANDLE)				\
		for (										\
			(_GDHANDLE) = GetDeviceList();			\
			(_GDHANDLE) != NULL;					\
			(_GDHANDLE) = GetNextDevice(_GDHANDLE)	\
		)

	#define	FOR_EACH_OKAY_GDEVICE(_GDHANDLE)		\
		FOR_EACH_GDEVICE(_GDHANDLE)					\
			if (DeviceOkay(_GDHANDLE))

	static Boolean		DeviceOkay(GDHandle theDevice)
	{
		return(		TestDeviceAttribute(theDevice, screenDevice)
				&&	TestDeviceAttribute(theDevice, screenActive));
	}

	DisplayRef		GetEnclosingDevice(const Rect *theRect)
	{
		Rect		sectRect, deviceRect;
		DisplayRef	curDevice, bestDevice = NULL;
		long		curArea, bestArea = 0;

		FOR_EACH_OKAY_GDEVICE(curDevice) {

			deviceRect = (*curDevice)->gdRect;

			if (QD_SectRect(&deviceRect, theRect, &sectRect)) {

				curArea = ((long)(sectRect.bottom - sectRect.top)
				  * (long)(sectRect.right - sectRect.left)
				);

				if (curArea > bestArea) {
					bestArea	= curArea;
					bestDevice	= curDevice;
				}
			}
		}

		return(bestDevice);
	}
#elif !_QT_
DisplayRef		GetEnclosingDevice(const Rect *theRect)
{
	CF_ASSERT(0);
	return NULL;
}

#endif

#if OPT_KJMAC || (OPT_MACOS && _YAAF_)
Rect			ReturnEnclosingDeviceRect(const Rect *theRectPtr)
{
	Rect		deviceR	= { 0, 0, 0, 0 };

	DisplayRef		theDevice = GetEnclosingDevice(theRectPtr);

	if (theDevice) {
		deviceR = (*theDevice)->gdRect;
	}

	return deviceR;
}
#endif

#if _QT_
CGSize	CGSizeMake(
  float   width,
  float   height)
{
	CGSize		size;
	
	size.width = width;
	size.height = height;
	return size;
}
#endif

#if OPT_WINOS
CGPoint	CGPointMake(
  float   x,
  float   y)
{
	CGPoint		pt;
	
	pt.x = x;
	pt.y = y;
	return pt;
}
#endif
