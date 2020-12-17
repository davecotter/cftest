//	CFMacTypes.h
 
#ifndef _H_CFMacTypes
#define _H_CFMacTypes

#include <vector>
#include "CoreFoundation/CFBase.h"

#if OPT_WINOS

	#if !_QT_ && (_KJAMS_ || _QTServer_)
		#include <CGGeometry.h>
		#include <CarbonEventsCore.h>
	#endif

	#ifndef CGFLOAT_DEFINED

		#if defined(__LP64__) && __LP64__
			# define CGFLOAT_TYPE double
			# define CGFLOAT_IS_DOUBLE 1
			# define CGFLOAT_MIN DBL_MIN
			# define CGFLOAT_MAX DBL_MAX
		#else
			# define CGFLOAT_TYPE float
			# define CGFLOAT_IS_DOUBLE 0
			# define CGFLOAT_MIN FLT_MIN
			# define CGFLOAT_MAX FLT_MAX
		#endif

		// Definition of the `CGFloat' type and `CGFLOAT_DEFINED'
		typedef CGFLOAT_TYPE CGFloat;
		#define CGFLOAT_DEFINED 1
	#endif

	#ifndef CGGEOMETRY_H_
		struct
		CGPoint {
			CGFloat x;
			CGFloat y;
		};
		typedef struct CGPoint CGPoint;
		
		struct CGSize {
			CGFloat width;
			CGFloat height;
		};
		typedef struct CGSize CGSize;
		
		struct CGRect {
			CGPoint origin;
			CGSize size;
		};
		typedef struct CGRect CGRect;
	#endif CGGEOMETRY_H_

	//#if !_JUST_CFTEST_
			typedef CGRect		HIRect;
	//#endif

#endif	//	OPT_WINOS

#if _QT_ || (OPT_WINOS && !_KJAMS_ && !_QTServer_)

	#if OPT_MACOS
		#include "MacTypes.h"
		#include <CoreServices/CoreServices.h>
		#include <CoreGraphics/CoreGraphics.h>
	
		#include <Carbon/Carbon.h>	//	struct defs are used even in 64bit
	#else
		struct DateTimeRec {
		  short               year;
		  short               month;
		  short               day;
		  short               hour;
		  short               minute;
		  short               second;
		  short               dayOfWeek;
		};
		typedef struct DateTimeRec              DateTimeRec;
		typedef SInt64                          LongDateTime;
	
		typedef UInt32		OSType;
		typedef char *		Ptr;
		typedef size_t		UniCharCount;
		typedef int32_t		Duration;
		
		typedef struct {
			short               top;
			short               left;
			short               bottom;
			short               right;
		} Rect;
		
		struct Point {
		  short	v;
		  short	h;
		};
		typedef struct Point                    Point;
	
		struct HFSUniStr255 {
		  UInt16              length;                 /* number of unicode characters */
		  UniChar             unicode[255];           /* unicode characters */
		};
		typedef struct HFSUniStr255             HFSUniStr255;
		typedef const HFSUniStr255 *            ConstHFSUniStr255Param;

		typedef double                          EventTime;
		typedef EventTime                       EventTimeout;
		typedef EventTime                       EventTimerInterval;
	
		/* Helpful doodads to convert to and from ticks and event times*/
		#define TicksToEventTime( t )   ((EventTime)( (t) / 60.0 ))
		#define EventTimeToTicks( t )   ((UInt32)( ((t) * 60) + 0.5 ))
		
		enum {
			kDurationMicrosecond           = -1L,  /* Microseconds are negative*/
			kDurationMillisecond           = 1L,   /* Milliseconds are positive*/
			kDurationSecond                = 1000L, /* 1000 * durationMillisecond*/
			kDurationMinute                = 60000L, /* 60 * durationSecond,*/
			kDurationHour                  = 3600000L, /* 60 * durationMinute,*/
			kDurationDay                   = 86400000L, /* 24 * durationHour,*/
			kDurationNoWait                = 0L,   /* don't block*/
			kDurationImmediate             = 0L,   /* don't block*/
			kDurationForever               = 0x7FFFFFFF /* no time limit*/
		};
	#endif	//	!OPT_MACOS

#endif //	_QT_

typedef std::vector<Rect>				RectVec;

#if OPT_MACOS || _YAAF_

	//	the rest are in "Multiprocessing.h"
	enum {
		kDurationSecond                = 1000L, /* 1000 * durationMillisecond*/
		kDurationMinute                = 60000L, /* 60 * durationSecond,*/
		kDurationHour                  = 3600000L, /* 60 * durationMinute,*/
		kDurationDay                   = 86400000L, /* 24 * durationHour,*/
		kDurationNoWait                = 0L,   /* don't block*/
	};
#endif

#define	kCFTimeSecondsPerMinute			60.0
#define	kCFTimeMinutePerHour			60.0
#define	kCFTimeHoursPerDay				24.0
#define	kCFTimeDaysPerWeek				7.0
#define	kCFTimeMonthsPerYear			12.0
#define	kCFTimeDaysPerYear				365.0
#define	kCFTimeDaysPerMonth				(kCFTimeDaysPerYear / kCFTimeMonthsPerYear)
	
#if !defined(__CARBONEVENTSCORE__) && !_PaddleServer_
	#define kEventDurationSecond            ((EventTime)1.0)
	#define kEventDurationMillisecond       ((EventTime)(kEventDurationSecond/1000))
	#define kEventDurationMicrosecond       ((EventTime)(kEventDurationSecond/1000000))
	#define kEventDurationNanosecond        ((EventTime)(kEventDurationSecond/1000000000))
	#define kEventDurationMinute            ((EventTime)(kEventDurationSecond * kCFTimeSecondsPerMinute))
	#define kEventDurationHour              ((EventTime)(kEventDurationMinute * kCFTimeMinutePerHour))
	#define kEventDurationDay               ((EventTime)(kEventDurationHour * kCFTimeHoursPerDay))

	#define kEventDurationNoWait            ((EventTime)0.0)
	#define kEventDurationForever           ((EventTime)(-1.0))
#endif	//	!__CARBONEVENTSCORE__
	
#define kEventDurationWeek              ((EventTime)(kEventDurationDay * kCFTimeDaysPerWeek))
#define kEventDurationMonth             ((EventTime)(kEventDurationDay * kCFTimeDaysPerMonth))
#define kEventDurationYear              ((EventTime)(kEventDurationDay * kCFTimeDaysPerYear))

#endif	//  _H_CFMacTypes
