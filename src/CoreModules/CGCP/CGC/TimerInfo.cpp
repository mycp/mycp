/*
    MYCP is a HTTP and C++ Web Application Server.
    Copyright (C) 2009-2010  Akee Yang <akee.yang@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#else
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#endif
#include "TimerInfo.h"

#define USES_ONE_SHOT_NEWVERSION

//
// TimerInfo class 
#if (USES_TIMER_HANDLER_POINTER==1)
TimerInfo::TimerInfo(unsigned int nIDEvent, unsigned int nElapse, cgcOnTimerHandler * handler, bool bOneShot, int nThreadStackSize, const void * pvParam)
#else
TimerInfo::TimerInfo(unsigned int nIDEvent, unsigned int nElapse, const cgcOnTimerHandler::pointer& handler, bool bOneShot, int nThreadStackSize, const void * pvParam)
#endif
: m_nIDEvent(nIDEvent), m_nElapse(nElapse)
, m_timerHandler(handler), m_bOneShot(bOneShot)
, m_nThreadStackSize(nThreadStackSize)
, m_pvParam(pvParam)
, m_timerState(TS_Init)

{
	ftime(&m_tLastRunTime);
}

TimerInfo::~TimerInfo(void)
{
	KillTimer();
#if (USES_TIMER_HANDLER_POINTER==1)
	m_timerHandler = 0;
#else
	m_timerHandler.reset();
#endif

	//if (m_timerThread.get()!=NULL) {
	//	if (m_bOneShot) {
	//		m_timerThread->detach();
	//	}
	//	m_timerThread.reset();
	//}
}

void TimerInfo::PauseTimer(void)
{
	m_timerState = TS_Pause;
}

#ifdef USES_STATIC_DO_TIMER
void TimerInfo::do_timer(TimerInfo * pTimerInfo)
#else
void TimerInfo::do_timer(void)
#endif
{
	try
	{
#ifdef USES_STATIC_DO_TIMER
		while (pTimerInfo->getTimerState() != TimerInfo::TS_Exit)
		{
			if (pTimerInfo->getTimerState() == TimerInfo::TS_Running)
			{
				pTimerInfo->doRunTimer();
			}
			else
			{
#ifdef WIN32
				Sleep(2);
#else
				usleep(2000);
#endif
			}
		}
		pTimerInfo->doTimerExit();
#else
		while (getTimerState() != TimerInfo::TS_Exit)
		{
			if (getTimerState() == TimerInfo::TS_Running)
			{
				doRunTimer();
			}else
			{
#ifdef WIN32
				Sleep(2);
#else
				usleep(2000);
#endif
			}
		}
		doTimerExit();
#endif
	}catch (const boost::exception &)
	{
	}catch (const boost::thread_exception &)
	{
	}catch (const std::exception& e)
	{
		//
		printf("!!!! TimerInfo::do_timer exception: %s\n",e.what());
	}catch (...)
	{
		//
	}
#ifdef USES_ONE_SHOT_NEWVERSION
#ifdef USES_STATIC_DO_TIMER
	if (pTimerInfo->getOneShot())
	{
		delete pTimerInfo;
		pTimerInfo = NULL;
	}
#else	// USES_STATIC_DO_TIMER
	if (m_bOneShot)
	{
		// *** 这种方式，也发生异常情况
		//if (m_timerThread.get()!=NULL)
		//	m_timerThread->detach();
		delete this;
	}
#endif	// USES_STATIC_DO_TIMER
#endif	// USES_ONE_SHOT_NEWVERSION
}


void TimerInfo::RunTimer(void)
{
	if (getOneShot())
		m_timerState = TS_Pause;
	else
		m_timerState = TS_Running;
	ftime(&m_tLastRunTime);
	if (m_timerThread.get() == NULL)
	{
		try
		{
#ifdef USES_THREAD_STACK_SIZE
			if (m_nThreadStackSize>0) {
				boost::thread_attributes attrs;
				attrs.set_stack_size(max(m_nThreadStackSize*1024,CGC_THREAD_STACK_MIN));
				//attrs.set_stack_size(CGC_THREAD_STACK_MAX);
				//attrs.set_stack_size(CGC_THREAD_STACK_MIN);
				m_timerThread = std::shared_ptr<boost::thread>(new boost::thread(attrs,boost::bind(&TimerInfo::do_timer, this)));
			}
			else {
				m_timerThread = std::shared_ptr<boost::thread>(new boost::thread(boost::bind(&TimerInfo::do_timer, this)));
			}
#else
			m_timerThread = std::shared_ptr<boost::thread>(new boost::thread(boost::bind(&TimerInfo::do_timer, this)));
#endif
			if (getOneShot())
			{
				// 在退出时再调用 detach
				//if (m_timerThread.get()!=NULL)
				//	m_timerThread->detach();
				m_timerState = TS_Running;
			}
		}catch (const boost::exception &)
		{
			m_timerState = TS_Exit;
		}catch (const boost::thread_exception &)
		{
			m_timerState = TS_Exit;
		}catch (const std::exception &)
		{
			m_timerState = TS_Exit;
		}catch (...)
		{
			m_timerState = TS_Exit;
		}
	}
}

void TimerInfo::KillTimer(void)
{
	m_timerState = TS_Exit;
	if (m_bOneShot) return;	// add by hd 2016-08-27
	//boost::mutex::scoped_lock lockTimer(m_mutex);
	//m_timerHandler.reset();
	std::shared_ptr<boost::thread> timerThreadTemp = m_timerThread;
	m_timerThread.reset();
	if (timerThreadTemp.get()!=NULL)
	{
		if (timerThreadTemp->joinable())
			timerThreadTemp->join();
		timerThreadTemp.reset();
	}
}

void TimerInfo::doRunTimer(void)
{
	struct timeb tNow;
	ftime(&tNow);

	//
	// the system time error.
	if (tNow.time < m_tLastRunTime.time)
	{
		m_tLastRunTime = tNow;
		return;
	}

	//
	// compare the second
	if (tNow.time < m_tLastRunTime.time + m_nElapse/1000)
	{
#ifdef WIN32
		Sleep(200);
#else
		usleep(200000);
#endif
		return;
	}

	//
	// compare the millisecond
	if (tNow.time == m_tLastRunTime.time + m_nElapse/1000)
	{
		const long nOff = m_tLastRunTime.millitm+(m_nElapse%1000) - tNow.millitm;
		if (nOff > 1)	// 0
		{
#ifdef WIN32
			Sleep(nOff);
#else
			usleep(nOff*1000);
#endif
		}
	}

	// OnTimer
#if (USES_TIMER_HANDLER_POINTER==1)
	if (m_timerHandler != NULL)
#else
	if (m_timerHandler.get() != NULL)
#endif
	{
		boost::mutex::scoped_lock * lock = NULL;
		try
		{
			if (m_timerHandler->IsThreadSafe())
				lock = new boost::mutex::scoped_lock(m_timerHandler->GetMutex());
			if (!m_bOneShot)
				ftime(&m_tLastRunTime);
			m_timerHandler->OnTimeout(m_nIDEvent, m_pvParam);
		}catch (const boost::exception &)
		{
		}catch (const boost::thread_exception &)
		{
		}catch (const std::exception & e)
		{
			printf("******* timeout exception: %s\n",e.what());
		}catch(...)
		{
			printf("******* timeout exception.\n");
		}
		if (lock != NULL)
			delete lock;
	}
	if (m_bOneShot)
	{
		KillTimer();
		return;
	}
//	//ftime(&m_tLastRunTime);
//	if (m_nElapse < 100)
//	{
////#ifdef WIN32
////		Sleep(2);
////#else
////		usleep(2000);
////#endif
//		return;
//	}
//
//#ifdef WIN32
//	Sleep(m_nElapse > 1000 ? 1000 : (m_nElapse-5));
//#else
//	usleep(m_nElapse > 1000 ? 1000000 : (m_nElapse-5)*1000);
//#endif
}

void TimerInfo::doTimerExit(void)
{
#if (USES_TIMER_HANDLER_POINTER==1)
	if (m_timerHandler != NULL) {
#else
	if (m_timerHandler.get() != NULL) {
#endif
		m_timerHandler->OnTimerExit(m_nIDEvent, m_pvParam);
	}
}


//
// TimerTable class
TimerTable::TimerTable(void)
{

}

TimerTable::~TimerTable(void)
{
	KillAll();
}


#if (USES_TIMER_HANDLER_POINTER==1)
unsigned int TimerTable::SetTimer(unsigned int nIDEvent, unsigned int nElapse, cgcOnTimerHandler * handler, bool bOneShot, int nThreadStackSize, const void * pvParam)
#else
unsigned int TimerTable::SetTimer(unsigned int nIDEvent, unsigned int nElapse, const cgcOnTimerHandler::pointer& handler, bool bOneShot, int nThreadStackSize, const void * pvParam)
#endif
{
#if (USES_TIMER_HANDLER_POINTER==1)
	if (nIDEvent <= 0 || nElapse <= 0 || handler == NULL) return 0;
#else
	if (nIDEvent <= 0 || nElapse <= 0 || handler.get() == NULL) return 0;
#endif

#ifdef USES_ONE_SHOT_NEWVERSION
	if (bOneShot)
	{
		TimerInfo * pTimerInfo = new TimerInfo(nIDEvent, nElapse, handler, bOneShot, nThreadStackSize, pvParam);
		if (pTimerInfo!=NULL)
			pTimerInfo->RunTimer();
		return (unsigned int)pTimerInfo;
	}
#endif

	TimerInfo * pTimerInfo = m_mapTimerInfo.find(nIDEvent, false);
	if (pTimerInfo == 0)
	{
		pTimerInfo = new TimerInfo(nIDEvent, nElapse, handler, bOneShot, nThreadStackSize, pvParam);
		if (pTimerInfo==NULL) return 0;
		m_mapTimerInfo.insert(nIDEvent, pTimerInfo);
	}else
	{
		pTimerInfo->PauseTimer();
		pTimerInfo->setElapse(nElapse);
		pTimerInfo->setOneShot(bOneShot);
		pTimerInfo->setTimerHandler(handler);
		pTimerInfo->setParam(pvParam);
	}

	pTimerInfo->RunTimer();
	return (unsigned int)pTimerInfo;
	//return true;
}

void TimerTable::KillTimer(unsigned int nIDEvent)
{
	TimerInfo * pTimerInfo = m_mapTimerInfo.find(nIDEvent, true);
	if (pTimerInfo != 0)
	{
		pTimerInfo->KillTimer();
		delete pTimerInfo;
	}
}

void TimerTable::KillAll(void)
{
	AUTO_LOCK(m_mapTimerInfo);
	for_each(m_mapTimerInfo.begin(), m_mapTimerInfo.end(),
		boost::bind(&TimerInfo::KillTimer, boost::bind(&std::map<unsigned int, TimerInfo*>::value_type::second,_1)));
	m_mapTimerInfo.clear(false, true);
	/*CLockMapPtr<unsigned int, TimerInfo*>::iterator pIter;
	for (pIter=m_mapTimerInfo.begin(); pIter!=m_mapTimerInfo.end(); pIter++)
	{
		TimerInfo * pTimerInfo = pIter->second;
		pTimerInfo->KillTimer();
		delete pTimerInfo;
	}
	m_mapTimerInfo.clear(false, false);*/
}
