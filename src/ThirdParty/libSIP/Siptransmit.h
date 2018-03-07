// Siptransmit.h file here
#ifndef __Siptransmit_h__
#define __Siptransmit_h__

#include <osip2/osip_mt.h>
#include <eXosip2/eXosip.h>
#include <boost/thread.hpp>
#include "../stl/lockmap.h"
#include "../stl/locklist.h"
#include "../stl/stldef.h"
#include "SipParameter.h"
#include "libsiphandler.h"

#pragma   warning(disable:4996)

#ifndef HANDLE
typedef void * HANDLE;
#endif // HANDLE

class Siptransmit
	: public DoSipHandler
{
public:
	Siptransmit(void);
	~Siptransmit(void);

	static DoSipHandler::pointer create(void)
	{
		return DoSipHandler::pointer(new Siptransmit());
	}

	static tstring GetRemoteIp(int did);
	static tstring GetCaller(int did);
	static int GetRemoteAudioPort(int did);
	static int GetRemoteVideoPort(int did);
	static void GetRemoteMedia(int did);

public:
	// DoSipHandler
	virtual bool initSip(const CSipParameter & sipp, OnSipHandler * handler);
	virtual bool isInitSip(void) const {return m_bInitedSip;}
	virtual void quitSip(void);

	virtual const CSipParameter & getParameter(void) const {return m_sipp;}
	virtual SipCallInfo::pointer getCallInfo(void) const {return m_currentCallInfo;}

	virtual int sipRegister(void);					///蛁聊
	virtual void sipUnRegister(void);				///蛁种
	virtual bool isSipRegistered(void) const {return m_bIsRegistered;}

	// call control
	virtual int sipCallInvite(const tstring & callee_num); ///網請
	virtual int sipCallAnswer(SipCallInfo::pointer callInfo, int localaudioport, int localvideoport);       ///茼湘網請
	virtual int sipCallAnswer(SipCallInfo::pointer callInfo);       ///茼湘網請
	virtual int sipCallTerminate(SipCallInfo::pointer callInfo);                    ///境儂
	virtual int sipCallSendDtmf(SipCallInfo::pointer callInfo, char dtmf);
	
	void procOneSipEvent(void);
	void addSipEvent(SipEventInfo::pointer eventInfo);

	// SipCallInfo
	SipCallInfo::pointer getCallInfo(int tranId, int callId, int dialogId);
	SipCallInfo::pointer getCallInfo(int callId);

protected:
	int sdp_complete_200ok (int did, osip_message_t * answer, int audio_port, int video_port);
	static void do_proc_SipEvent(Siptransmit * pSiptransmit);
	static void do_proc_EventInfo(Siptransmit * pSiptransmit);

private:
	bool m_bInitedSip;
	bool m_bIsRegistered;
	int m_regid;
	//osip_message_t * m_osipmessage;
	CSipParameter m_sipp;
	SipCallInfo::pointer m_currentCallInfo;

	//int  m_LocalAudioPort;
	//int  m_LocalVideoPort;
	bool m_ThreadRunFlag;
	OnSipHandler * m_EvenHandler;
	CLockMap<int, SipCallInfo::pointer> m_mapCallInfo;	// callId->*
	CLockList<SipEventInfo::pointer> m_mapEventInfo;
	boost::thread * m_threadSipEvent;
	boost::thread * m_threadEventInfo;
};

#endif // __Siptransmit_h__
