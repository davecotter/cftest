#include "stdafx.h"
#include "CFTestUtils.h"
#include "SuperString.h"

void	PostAlert(const char *strZ)
{
	CCFLog(true)(SuperString(strZ).ref());
}

#ifndef _KJQT_
SuperString		SSLocalize(const char *strZ, const char *commentZ)
{
	SuperString		str(strZ);
	
	UNREFERENCED_PARAMETER(commentZ);
	return str;
}
#endif

/***************************************************/
long	CMutex_long::inc()
{
	long		valueL = 0;
	CCritical	sc(this, CRW_Write);
	
	if (!sc.GetErr()) {
		valueL = ++i_value;
	}
	
	return valueL;
}

long	CMutex_long::dec(bool quietB)
{
	long		valueL = 0;
	CCritical	sc(this, CRW_Write);
	
	if (!sc.GetErr()) {
		valueL = --i_value;
		
		#ifdef kDEBUG
		if (!quietB) {
			CF_ASSERT(valueL >= 0);
		}
		#else
		UNREFERENCED_PARAMETER(quietB);
		#endif
	}
	
	return valueL;
}

void	CMutex_long::wait(long for_valueL, Duration eventType)
{
	while (Get() != for_valueL) {
		IdleDuration(kQuarterSecond, eventType);
	}
}

/***************************************************/
size_t	CMutex_size_t::inc(size_t inc_sizeL)
{
	size_t		valueL = 0;
	CCritical	sc(this, CRW_Write);
	
	if (!sc.GetErr()) {
		valueL = i_value += inc_sizeL;
	}
	
	return valueL;
}

size_t	CMutex_size_t::dec()
{
	size_t		valueL = 0;
	CCritical	sc(this, CRW_Write);
	
	if (!sc.GetErr()) {
		valueL = --i_value;
		
		#ifdef kDEBUG
			CF_ASSERT(valueL >= 0);
		#endif
	}
	
	return valueL;
}

void	CMutex_size_t::wait(size_t for_valueL, Duration eventType)
{
	while (Get() != for_valueL) {
		IdleDuration(kQuarterSecond, eventType);
	}
}

/***************************************************/
void	CMutex_bool::wait(bool for_valueL, Duration eventType)
{
	while (Get() != for_valueL) {
		IdleDuration(kQuarterSecond, eventType);
	}
}

void	IdleDuration(float secondsF, Duration eventType)
{
	UNREFERENCED_PARAMETER(eventType);
	CFSleep(secondsF);
}

/********************************************************/
CSemaphore::CSemaphore(const char *typeZ) : 
//	i_semID(NULL), 
	i_waitingB(false),
	i_create_err(noErr)
{
	#ifdef kDEBUG
		i_typeStr = typeZ;
	#else
		UNREFERENCED_PARAMETER(typeZ);
	#endif

/*
	BEX_TRY {
		i_semID = new boost::condition_variable();
	} BEX_CATCH {
		i_create_err = kMPInsufficientResourcesErr;
		BEX_NO_PROPAGATE;
	} BEX_ENDTRY;
*/
	i_signaledB = true;
}

#define		kLogSemaphoreWaits		0	//	kDEBUG

//static 
void	CSemaphore::FirstWait(CSemaphore& sem)
{
	sem.i_signaledB = false;
}

CSemaphore::~CSemaphore()
{
	if (!i_create_err) {
		bool	waitingB = false;
		
		do {
			{
/*				boost::mutex::scoped_lock	lock(i_mutex);
				
				if (i_semID) {
					delete i_semID;
					i_semID = NULL;
				}
*/				
				waitingB = i_waitingB;
			}
			
			if (waitingB) {
				if (IsPreemptiveThread()) {
					IdleDuration(0.01f);
				} else {
					#ifdef _ROSE_
						CFDebugBreak();
					#else
						#ifdef _KJAMS_
						if (gApp && gApp->i_threads) {
							IdleDuration(0.01f);
						} else
						#endif
						{
							waitingB = false;
							#ifdef kDEBUG
								Logf("lost a thread: %s\n", i_typeStr.c_str());
							#endif
						}
					#endif
				}
			}
		} while (waitingB);
	}
}

OSStatus		CSemaphore::signal(bool ignoreB)
{
	OSStatus	err = GetErr();
	
	if (!i_is_ignoredB.Get()) {
		
		if (ignoreB) {
			i_is_ignoredB.Set(true);
		}
		
		if (!err) {
			//boost::mutex::scoped_lock	lock(i_mutex);
			
			i_signaledB = true;
			//	CF_ASSERT(i_semID);
			//	IX(i_semID->notify_one());
		}
	}
	
	return err;
}

//	this will NOT return if the timeout is exceeded!
//	instead, between timeouts it will yield / idle / idlegraphics
OSStatus		CSemaphore::wait(Duration unused)
{
	OSStatus	err = GetErr();
	
	UNREFERENCED_PARAMETER(unused);

	if (i_is_ignoredB.Get()) {
		#if kLogSemaphoreWaits
		Logf("--> Sem: Ignore wait: %s\n", i_typeStr.c_str());
		#endif
	} else {
		
		while (!i_signaledB) {
			i_waitingB = true;
			IdleDuration(0.01f);
			i_waitingB = false;
		}
	
		i_waitingB = false;
		i_signaledB = false;
	}
	
	return err;
}

//	this will return kMPTimeoutErr if the timeout is exceeded
//	only call on back thread
OSStatus		CSemaphore::timed_wait(Duration wait_duration)
{
	OSStatus	err = GetErr();
	
	CF_ASSERT(!IsMainThread());

	ERR(wait(wait_duration));

	return err;
}

void			CSemaphore::ignore(bool ignoreB)
{
	if (ignoreB) {
		signal(true);
	} else {
		i_is_ignoredB.Set(false);
		signal();
		wait();
	}
}


