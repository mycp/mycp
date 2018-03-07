// RecvRtpSession.h file here
#ifndef __RecvRtpSession_h__
#define __RecvRtpSession_h__
//
//#if (defined(WIN32) || defined(_WIN32_WCE))
//#define ui64	
//#endif

#include <boost/thread.hpp>
#include "../stl/locklist.h"
#include "rtpsession.h"
#include "librtphandler.h"
#include "CrecvBuffer.h"
#define MAX_WAITPACK 60

class CRecvRtpSession
	: public RTPSession
{
public:
	CRecvRtpSession(void);
	~CRecvRtpSession(void);
	void SetRtpHandler(OnRtpHandler * handle, DoRtpHandler::pointer doHandler, void * param) {m_handlerReceiver = handle; m_pDoRtpHandler = doHandler, m_paramhandler = param;}
	const OnRtpHandler * GetRtpHandler(void) const {return m_handlerReceiver;}
	//void SetRtpType(uint8_t type) {m_rtpType = type;}
	//void ReleaseBuffer(void) {m_FrameFamily.releaseBuffer();}
	void Release(void);

	void proc_Poll(void);
	void proc_Data(void);

protected:
	static void do_proc_poll(CRecvRtpSession * owner);
	static void do_proc_data(CRecvRtpSession * owner);
	void OnPollThreadStep(void);
	void ProcessRTPPacket(const RTPPacket &rtppack, u_long destIp, u_int destPort);


private:
	CrecvBuffer m_FrameFamily;
	OnRtpHandler * m_handlerReceiver;
	unsigned int m_lasttimestamp;
	BYTE * m_tempBuffer;

	DoRtpHandler::pointer m_pDoRtpHandler;
	void * m_paramhandler;
	bool m_killed;
	boost::thread * m_proc_poll;
	boost::thread * m_proc_data;
	CLockList<CRTPData::pointer> m_rtpdatas;
};

#endif // __RecvRtpSession_h__
