// SipCallInfo.h file here
#ifndef __SipCallInfo_h__
#define __SipCallInfo_h__

#include <boost/shared_ptr.hpp>
#include "../stl/stldef.h"

class SipCallInfo
{
public:
	typedef boost::shared_ptr<SipCallInfo> pointer;

	static SipCallInfo::pointer create(int tranId, int callId, int dialogId)
	{
		return SipCallInfo::pointer(new SipCallInfo(tranId, callId, dialogId));
	}

	enum MessageType
	{
		MT_Dtmf = 0x01

		, MT_Other = 0xFF
	};

	int tranId(void) const {return m_tranId;}
	int callId(void) const {return m_callId;}
	int dialogId(void) const {return m_dialogId;}
	void dialogId(int newv) {m_dialogId = newv;}

	void caller(const tstring & newv) {m_caller = newv;}
	const tstring & caller(void) const {return m_caller;}
	void callee(const tstring & newv) {m_callee = newv;}
	const tstring & callee(void) const {return m_callee;}

	void remoteIp(const tstring & newv) {m_remoteIp = newv;}
	const tstring & remoteIp(void) const {return m_remoteIp;}
	void audioPort(unsigned int newv) {m_audioPort = newv;}
	unsigned int audioPort(void) const {return m_audioPort;}
	void videoPort(unsigned int newv) {m_videoPort = newv;}
	unsigned int videoPort(void) const {return m_videoPort;}

	void callMessage(MessageType messageType, const tstring & newv) {m_messageType = messageType; m_callMessage = newv;}
	MessageType messageType(void) const {return m_messageType;}
	const tstring & callMessage(void) const {return m_callMessage;}

public:
	SipCallInfo(int tranId, int callId, int dialogId)
		: m_tranId(tranId), m_callId(callId), m_dialogId(dialogId)
		, m_caller(_T(""))
		, m_remoteIp(_T("")), m_audioPort(0), m_videoPort(0)
		, m_messageType(MT_Other), m_callMessage(_T(""))
	{}
	~SipCallInfo(void)
	{}

private:
	int m_tranId;
	int m_callId;
	int m_dialogId;

	tstring m_caller;
	tstring m_callee;
	tstring m_remoteIp;
	unsigned int m_audioPort;
	unsigned int m_videoPort;

	MessageType m_messageType;
	tstring m_callMessage;
};

const SipCallInfo::pointer NullSipCallInfo;

#endif // __SipCallInfo_h__
