// RtpProxy.h file here
#ifndef __RtpProxy_h__
#define __RtpProxy_h__

// jrtp
#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "RecvRtpSession.h"
// boost
#include "boost/shared_ptr.hpp"
#include "boost/enable_shared_from_this.hpp"
// owner
#include "librtphandler.h"

//
// ÃÌº”lib¡¥Ω”

#ifdef WIN32
#ifdef _DEBUG
//#pragma comment(linker,"/NODEFAULTLIB:uafxcwd.lib") 
//#pragma comment(lib, "uafxcwd.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "jthreadd.lib")
#pragma comment(lib, "jrtplibd.lib")
#else
#pragma comment(linker,"/NODEFAULTLIB:libcmt.lib") 
//#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "jthread.lib")
#pragma comment(lib, "jrtplib.lib")
#endif // _DEBUG
#endif // WIN32


class CRtpProxy
	: public DoRtpHandler
	, public std::enable_shared_from_this<CRtpProxy>
{
public:
	typedef std::shared_ptr<CRtpProxy> pointer;

	static CRtpProxy::pointer create(void)
	{
		return CRtpProxy::pointer(new CRtpProxy());
	}
	static int InitComm(void);

	int InitSession(uint16_t rtpPort);
	void DestroySession();

protected:
	void SetRtpHandler(OnRtpHandler * handler, void * param) {m_rtpSession.SetRtpHandler(handler, shared_from_this(), param);}

	bool IsActive() const {return m_bActive;}

	void ClearDest(void);
	int AddDest(const char* IP,uint16_t destport);
	int AddDest(unsigned long destip, uint16_t destport);
	int DelDest(unsigned long destip, uint16_t destport);
	void CleaerDest(void) {m_rtpSession.ClearDestinations();}

	int AddAccept(char* IP,uint16_t port);
	int SendData(const unsigned char * pData, int nLen, unsigned int timestamp);

protected:
	// DoRtpHandler
	virtual void doSetRtpHandler(OnRtpHandler * handler, void * param) {m_rtpSession.SetRtpHandler(handler, shared_from_this(), param);}
	virtual const OnRtpHandler *  doGetRtpHandler(void) const {return m_rtpSession.GetRtpHandler();}
	virtual void doClearDest(void) {ClearDest();}
	virtual int doAddDest(const char * destip, unsigned int destport) {return AddDest(destip, destport);}
	virtual int doAddDest(unsigned long destip, unsigned int destport) {return AddDest(destip, destport);}
	virtual int doSendData(const unsigned char * pData, int nLen, unsigned int timestamp) {return SendData(pData, nLen, timestamp);}
	virtual void doSetMediaType(unsigned short mediatype) {m_pt = (uint8_t)mediatype;}
	virtual unsigned short doGetMediaType(void) const {return m_pt;}
	virtual int doGetRtpPort(void) const {return m_rtpPort;}

public:
	CRtpProxy(void);
	~CRtpProxy(void);

private:
	boost::mutex m_mutexRtpSerssion;
	CRecvRtpSession m_rtpSession;
	bool m_bActive;
	uint16_t m_rtpPort;
	uint8_t m_pt;
};

#endif // __RtpProxy_h__

