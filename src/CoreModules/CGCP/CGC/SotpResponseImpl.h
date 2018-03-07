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

// SotpResponseImpl.h file here
#ifndef _SotpResponseImpl_HEAD_INCLUDED__
#define _SotpResponseImpl_HEAD_INCLUDED__

#include "IncludeBase.h"

class CResponseHandler
{
public:
	virtual unsigned short onGetNextSeq(void) = 0;
	virtual int onAddSeqInfo(const unsigned char * callData, unsigned int dataSize, unsigned short seq, unsigned long cid, unsigned long sign) = 0;
	virtual int onAddSeqInfo(unsigned char * callData, unsigned int dataSize, unsigned short seq, unsigned long cid, unsigned long sign) = 0;
};
class CParserSotpHandler
{
public:
	virtual void onReturnParserSotp(cgcParserSotp::pointer cgcParser) = 0;
};

class CSotpResponseImpl
	: public cgcSotpResponse
	, public boost::enable_shared_from_this<CSotpResponseImpl>
{
private:
	cgcRemote::pointer m_cgcRemote;
	cgcParserSotp::pointer m_cgcParser;
	cgcSession::pointer m_session;
	CResponseHandler * m_pResponseHandler;
	CParserSotpHandler* m_pParserSotpHandler;
	bool m_bResponseSended;
	bool m_bNotResponse;
	int m_nHoldSecond;		// default:-1; 0:remove; >0:hold
	boost::mutex m_sendMutex;
	boost::mutex::scoped_lock * m_pSendLock;
	unsigned long m_originalCallId;
	unsigned long m_originalCallSign;
	tstring m_sSslPublicKey;
	//tstring m_sSslPassword;
public:
	CSotpResponseImpl(const cgcRemote::pointer& pcgcRemote, const cgcParserSotp::pointer& pcgcParser, CResponseHandler * pHandler, CParserSotpHandler* pParserSotpHandler);
	virtual ~CSotpResponseImpl(void);
	
	void SetResponseHandler(CResponseHandler * pHandler) {m_pResponseHandler = pHandler;}
// ?	void SetAttach(const AttachInfo & newValue) {m_attach = newValue;}

	// type: 1:open, 2:close, 3:active
	int sendSessionResult(long retCode, const tstring & sSessionId);

	int sendAppCallResult(long retCode=0, unsigned long sign = 0, bool bNeedAck = true);

	int sendP2PTry(void);

	// ?
//	int sendCluResult(int clusterType, const ClusterSvrList & clusters);
//	int sendCluResult(int protoType, const tstring & value);

	//void clearCgcRemote(void);
	void setCgcRemote(const cgcRemote::pointer& pcgcRemote);
	void setSessionClosed(void) {}
	//void setCgcParser(const cgcParserSotp::pointer& pcgcParser);
	void setSession(const cgcSession::pointer& session) {m_session = session;}
	//cgcParserSotp::pointer getCgcParser(void) {return m_cgcParser;}
	cgcRemote::pointer getCgcRemote(void) {return m_cgcRemote;}
	unsigned long getCommId(void) const {return m_cgcRemote.get() == NULL ? 0 : m_cgcRemote->getCommId();}

	bool isResponseSended(void) const {return m_bResponseSended;}
	bool isSetNotResponse(void) const {return m_bNotResponse;}
	void SetEncoding(const tstring & newValue = _T("GBK")) {setEncoding(newValue);}
	bool IsInvalidate(void) const {return isInvalidate();}
	virtual int sendResponse(const char * pResponseData, size_t nResponseSize) {return -1;}
	void SetSslPublicKey(const tstring & newValue) {m_sSslPublicKey = newValue;}
	//void SetSslPassword(const tstring & newValue) {m_sSslPassword = newValue;}

protected:
	bool m_bDisableZip;
	virtual bool setConfigValue(int nConfigItem, unsigned int nConfigValue);
	// for attachment
	virtual void setAttachName(const tstring & name) {if (m_cgcParser.get() != NULL) m_cgcParser->setResAttachName(name);}
	virtual void setAttachInfo(bigint total, bigint index) {if (m_cgcParser.get() != NULL) m_cgcParser->setResAttachInfo(total, index);}
	virtual void setAttachData(const unsigned char * attachData, unsigned int attachSize) {if (m_cgcParser.get() != NULL) m_cgcParser->setResAttachData(attachData, attachSize);}
	virtual void setAttach(const cgcAttachment::pointer& pAttach) {if (m_cgcParser.get() != NULL) m_cgcParser->setResAttach(pAttach);}
	virtual void setParameter(const cgcParameter::pointer& parameter, bool bSetForce) {if (m_cgcParser.get() != NULL) m_cgcParser->setResParameter(parameter,bSetForce);}
	virtual void addParameter(const cgcParameter::pointer& parameter, bool bAddForce) {if (m_cgcParser.get() != NULL) m_cgcParser->addResParameter(parameter,bAddForce);}
	virtual void addParameters(const std::vector<cgcParameter::pointer>& parameters, bool bAddForce) {if (m_cgcParser.get() != NULL) m_cgcParser->addResParameters(parameters,bAddForce);}
	virtual void delParameter(const tstring& paramName) {if (m_cgcParser.get() != NULL) m_cgcParser->delResParameter(paramName);}
	virtual void clearParameter(void) {if (m_cgcParser.get() != NULL) m_cgcParser->clearResParameter();}
	virtual size_t getParameterCount(void) const {return m_cgcParser.get() == NULL ? 0 : m_cgcParser->getResParameterCount();}
	virtual void lockResponse(void);
	virtual int sendResponse(long retCode, unsigned long sign=0, bool bNeedAck = true);
	//virtual void setNotResponse(void) {m_bNotResponse = true;}
	virtual unsigned long setNotResponse(int nHoldSecond);
	//virtual cgcSession::pointer getSession(void) const {return m_session;}
	virtual unsigned long getRemoteId(void) const {return m_cgcRemote.get() == NULL ? 0 : m_cgcRemote->getRemoteId();}
//	virtual int sendString(const tstring & sString);
	virtual void setEncoding(const tstring & newValue = _T("GBK")) {if (m_cgcParser.get() != NULL) m_cgcParser->setResEncoding(newValue);}
	virtual void invalidate(void);
	virtual bool isInvalidate(void) const {return m_cgcRemote.get() == NULL ? true : m_cgcRemote->isInvalidate();}

	//tstring GetResponseClusters(const ClusterSvrList & clusters);

};

#endif // _SotpResponseImpl_HEAD_INCLUDED__
