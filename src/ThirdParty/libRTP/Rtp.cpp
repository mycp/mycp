
#include "Rtp.h"
#include "RtpProxy.h"
#include "../stl/lockmap.h"

CLockMap<DoRtpHandler*, CRtpProxy::pointer> m_rtpProxy;

CRtp::CRtp(void)
{
}

CRtp::~CRtp(void)
{
	//stopAll();
}

DoRtpHandler::pointer CRtp::startRtp(unsigned short rtpPort)
{
	CRtpProxy::pointer rtpProxy = CRtpProxy::create();
	CRtpProxy::InitComm();
	if (rtpProxy->InitSession(rtpPort) != 0)
		return NullDoRtpHandlerPointer;

	DoRtpHandler * handler = (DoRtpHandler*)rtpProxy.get();
	m_rtpProxy.insert(handler, rtpProxy);
	return rtpProxy;
	//return handler;
}

void CRtp::stopRtp(DoRtpHandler::pointer handler)
{
	CRtpProxy::pointer rtpProxy;
	if (m_rtpProxy.find(handler.get(), rtpProxy, true))
	{
		//handler->doSetRtpHandler(0);
		rtpProxy->DestroySession();
		handler->doSetRtpHandler(0);
	}
}

void CRtp::stopAll(void)
{
	AUTO_LOCK(m_rtpProxy);
	CLockMap<DoRtpHandler*, CRtpProxy::pointer>::const_iterator iter;
	for (iter=m_rtpProxy.begin(); iter!=m_rtpProxy.end(); iter++)
	{
		iter->second->DestroySession();
	}
	m_rtpProxy.clear(false);
}

