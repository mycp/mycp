
#include "../stl/lockmap.h"
#include "Sip.h"
#include "Siptransmit.h"

//Siptransmit gSiptransmit;
CLockMap<DoSipHandler*, DoSipHandler::pointer> m_sips;

CSip::CSip(void)
{

}

CSip::~CSip(void)
{

}

DoSipHandler::pointer CSip::initSip(const CSipParameter & sipp, OnSipHandler * handler)
{
	BOOST_ASSERT (handler != NULL);

	DoSipHandler::pointer sipHandler = Siptransmit::create();
	if (!sipHandler->initSip(sipp, handler))
	{
		sipHandler.reset();
		return sipHandler;
	}
	m_sips.insert(sipHandler.get(), sipHandler);
	return sipHandler;
}

void CSip::quitSip(DoSipHandler::pointer sipHandler)
{
	DoSipHandler::pointer doSipHandler;
	if (m_sips.find(sipHandler.get(), doSipHandler, true))
	{
		doSipHandler->quitSip();
	}
}

void CSip::quitAll(void)
{
	AUTO_LOCK(m_sips);
	CLockMap<DoSipHandler*, DoSipHandler::pointer>::const_iterator iter;
	for (iter=m_sips.begin(); iter!=m_sips.end(); iter++)
	{
		iter->second->quitSip();
	}
	m_sips.clear(false);
}

//bool CSip::isInitSip(void) const
//{
//	return gSiptransmit.isInitSip();
//}

//const CSipParameter & CSip::GetSipParameter(void) const
//{
//	return gSiptransmit.GetSipParameter();
//}
//
//int CSip::sipRegister(void)
//{
//	return gSiptransmit.sipRegister();
//}
//
//void CSip::sipUnRegister(void)
//{
//	gSiptransmit.sipUnRegister();
//}

// call control
//int CSip::CallInvite(const tstring & callee_num)
//{
//	return gSiptransmit.CallInvite(callee_num);
//}
//
//int CSip::CallAnswer(SipCallInfo::pointer callInfo, int localaudioport, int localvideoport)
//{
//	return gSiptransmit.CallAnswer(callInfo, localaudioport, localvideoport);
//}
//
//int CSip::CallTerminate(SipCallInfo::pointer callInfo)
//{
//	return gSiptransmit.CallTerminate(callInfo);
//}
//
//int CSip::CallSendDtmf(SipCallInfo::pointer callInfo, char dtmf)
//{
//	return gSiptransmit.CallSendDtmf(callInfo, dtmf);
//}
