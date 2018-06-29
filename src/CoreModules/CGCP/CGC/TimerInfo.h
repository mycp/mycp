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

// TimerInfo.h file here
#ifndef _TIMERINFO_HEAD_INCLUDED__
#define _TIMERINFO_HEAD_INCLUDED__

#define USES_THREAD_STACK_SIZE

//
// include
#include "IncludeBase.h"
#include <sys/timeb.h>

//#define USES_STATIC_DO_TIMER
// TimerInfo class
class TimerInfo
{
public:
	enum TimerState
	{
		TS_Init = 0
		, TS_Running = 1
		, TS_Pause = 2
		, TS_Exit = 3
	};
private:
	unsigned int m_nIDEvent;
	unsigned int m_nElapse;
	//boost::mutex m_mutex;
#if (USES_TIMER_HANDLER_POINTER==1)
	cgcOnTimerHandler * m_timerHandler;
#else
	cgcOnTimerHandler::pointer m_timerHandler;
#endif
	bool m_bOneShot;
	int m_nThreadStackSize;
	const void * m_pvParam;

	TimerState m_timerState;
	boost::shared_ptr<boost::thread> m_timerThread;
	struct timeb m_tLastRunTime;

public:
#if (USES_TIMER_HANDLER_POINTER==1)
	TimerInfo(unsigned int nIDEvent, unsigned int nElapse, cgcOnTimerHandler * handler, bool bOneShot, int nThreadStatckSize, const void * pvParam);
#else
	TimerInfo(unsigned int nIDEvent, unsigned int nElapse, const cgcOnTimerHandler::pointer& handler, bool bOneShot, int nThreadStatckSize, const void * pvParam);
#endif
	virtual ~TimerInfo(void);

public:
	TimerState getTimerState(void) const {return m_timerState;}
	void PauseTimer(void);
	void RunTimer(void);
	void KillTimer(void);

	unsigned int getIDEvent(void) const {return m_nIDEvent;}
	void setElapse(unsigned int newv) {m_nElapse = newv;}
	unsigned int getElapse(void) const {return m_nElapse;}
#if (USES_TIMER_HANDLER_POINTER==1)
	void setTimerHandler(cgcOnTimerHandler * newv) {m_timerHandler = newv;}
#else
	void setTimerHandler(const cgcOnTimerHandler::pointer& newv) {m_timerHandler = newv;}
#endif
	void setOneShot(bool newv) {m_bOneShot = newv;}
	bool getOneShot(void) const {return m_bOneShot;}
	void setParam(const void * newv) {m_pvParam = newv;}
	const void * getParam(void) const {return m_pvParam;}

protected:
#ifdef USES_STATIC_DO_TIMER
	static void do_timer(TimerInfo * pTimerInfo);
#else
	void do_timer(void);
#endif
	void doRunTimer(void);
	void doTimerExit(void);
};

//
// TimerTable class
class TimerTable
{
private:
	CLockMapPtr<unsigned int, TimerInfo*> m_mapTimerInfo;

public:
	TimerTable(void);
	virtual ~TimerTable(void);

public:
#if (USES_TIMER_HANDLER_POINTER==1)
	unsigned int SetTimer(unsigned int nIDEvent, unsigned int nElapse, cgcOnTimerHandler * handler, bool bOneShot, int nThreadStackSize, const void * pvParam);
#else
	unsigned int SetTimer(unsigned int nIDEvent, unsigned int nElapse, const cgcOnTimerHandler::pointer& handler, bool bOneShot, int nThreadStackSize, const void * pvParam);
#endif
	void KillTimer(unsigned int nIDEvent);

	void KillAll(void);

};

#endif // _TIMERINFO_HEAD_INCLUDED__
