//	CFTestUtils

#ifndef _H_CFTestUtils
#define	_H_CFTestUtils

#include <string>
#include "CFUtils.h"

inline bool	IsPreemptiveThread()	{ return false; }
inline bool	IsMainThread()			{ return !IsPreemptiveThread();	}

class	CPW_TaskRec;
class	CAS_URL;
class	CT_Timer;

typedef	void*	APP_ForkRef;

void			PostAlert(const char *strZ);
SuperString		SSLocalize(const char *strZ, const char *commentZ);

#define		ERR_Already_Reported	-1
#define		kSampleSize				4

/************************************************/

enum {
	CRW_Write, 
	CRW_Read
};

class CMutex;
class CCritical {
	public:
	CCritical(CMutex *queP, bool readB = false) {
		UNREFERENCED_PARAMETER(queP);
		UNREFERENCED_PARAMETER(readB);
	}

	OSStatus	GetErr() { return noErr; }
};

/************************************************/
//#define	kDurationMillisecond			1L
//#define	kDurationForever				0x7FFFFFFFL

enum{
	kDurationForever_Yield		= 0x7FFFFFFB, 
	kDurationForever_Idle,
	kDurationForever_EventLoop, 
	kDurationForever_EventLoop_OnlyCallbacks, 	
	kDurationForever_MAX,

	//kDurationSecond	= kDurationMillisecond * 1000	//	durationSecond
};  

#define	kQuarterSecond		(1/4.0f)

void	IdleDuration(float secondsF, Duration eventType = kDurationForever_EventLoop);

/************************************************/
//	this is NOT a real mutex
class	CMutex {
	friend class			CCritical;
//	short					i_countS;
//	boost::recursive_mutex	*i_muP;
	OSStatus				i_create_err;
	OSStatus				i_enter_err;

	CMutex(const CMutex& other) {
		UNREFERENCED_PARAMETER(other);
	}	
	public:

	CMutex() : /*i_countS(0), */i_create_err(noErr), i_enter_err(noErr) {}
	virtual ~CMutex() {}
	
	OSStatus	GetErr() {
		return i_create_err ? i_create_err : i_enter_err;
	}

	void	enter(bool readB = CRW_Write) {
		UNREFERENCED_PARAMETER(readB);
	}
	bool	try_lock() { return true; }	//	always for write
	void	exit() {}
};

template <class T>
class	CMutexT : public CMutex {
	protected:
	//	NEVER access this value
	//	unless you have explicitly set up a "CCritical	sc(this)" on the stack
	T		i_value;

	public:	
	virtual	T	default_value() const {	return 0;	}

	CMutexT() : i_value(default_value()) {}
	virtual ~CMutexT() { }
	
	T	Get() {
		CCritical	sc(this, CRW_Read);
		
		if (!sc.GetErr()) {
			return i_value;
		}
		
		return default_value();
	}

	T	Set(T new_value)	{
		T			prev_value(default_value());
		CCritical	sc(this, CRW_Write);
		
		if (!sc.GetErr()) {
			prev_value	= i_value;
			i_value		= new_value;
		}
		
		return prev_value;
	}
	
	operator T()		{	return Get();	}
};

class	CMutex_long : public CMutexT<long> {
	public:
	long	inc();
	long	dec(bool quietB = false);
	void	wait(long for_valueL, Duration eventType = kDurationForever_EventLoop);
};

class	CMutex_size_t : public CMutexT<size_t> {
	public:
	size_t	inc(size_t inc_sizeL = 1);
	size_t	dec();
	void	wait(size_t for_valueL, Duration eventType = kDurationForever_EventLoop);
};

class	CMutex_bool : public CMutexT<bool> {
	public:
	void	wait(bool for_valueL, Duration eventType = kDurationForever_EventLoop);
};

class	CS_MainThread_AbortProc {	//	: public CMainThreadProc {
//	typedef	CMainThreadProc		_inherited;
	
	protected:
	friend class CMutex_Proc;
	friend class CSpooler;
	OSStatus			i_err;
	bool				i_writeB;
//	CSpooler			*i_spoolP;
	
	public:

	void	SetErr(OSStatus err) { i_err = err; }

	CS_MainThread_AbortProc() :
		//	_inherited("abort proc"), 
		i_err(noErr),
		i_writeB(false)
		//		i_spoolP(NULL),
	{
	}
};

/**************************************************************/
//	not a real semaphore
class CSemaphore {
	bool						i_waitingB;
//	boost::mutex				i_mutex;
	bool						i_signaledB;
//	boost::condition_variable	*i_semID;
	
	#ifdef kDEBUG
	std::string		i_typeStr;
	#endif	
	
	OSStatus		i_create_err;
	CMutex_bool		i_is_ignoredB;
	
	public:
	bool			IsIgnored()						{	return i_is_ignoredB.Get();	}
	void			SetIgnored(bool setB = true)	{	i_is_ignoredB.Set(setB);	}

	CSemaphore(const char *typeZ);
	~CSemaphore();
	
	static void	FirstWait(CSemaphore& sem);

	OSStatus	GetErr() {
		return i_create_err;
	}
	
	OSStatus		signal(bool ignoreB = false);
	void			ignore(bool ignoreB = true);
	
	//	this will NOT return if the timeout is exceeded!
	//	instead, between timeouts it will yield / idle / idlegraphics
	OSStatus		wait(Duration timeout = kDurationForever);

	//	this will return kMPTimeoutErr if the timeout is exceeded
	//	only call on back thread
	OSStatus		timed_wait(Duration timeout);
};

#endif	//	_H_CFTestUtils
