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

// SotpRequestImpl.h file here
#ifndef _sotprequestimpl_head_included__
#define _sotprequestimpl_head_included__

#include "IncludeBase.h"
#include <CGCClass/AttributesImpl.h>
//#include "AttributesImpl.h"
#include "XmlParseParams.h"
#include "RequestImpl.h"

class CSotpRequestImpl;
typedef std::map<unsigned long, CSotpRequestImpl*> CSotpRequestImplMap;
typedef CSotpRequestImplMap::iterator CSotpRequestImplMapIter;
typedef std::pair<unsigned long, CSotpRequestImpl*> CSotpRequestImplMapPair;

// CSotpRequestImpl
class CSotpRequestImpl
	: public cgcSotpRequest
	, public CRequestImpl
{
private:
	cgcParserSotp::pointer m_sotpParser;
	//std::string m_sApi;

public:
	CSotpRequestImpl(const cgcRemote::pointer& pcgcRemote, const cgcParserSotp::pointer& pcgcParser)
		: CRequestImpl(pcgcRemote, pcgcParser)
		, m_sotpParser(pcgcParser)
		, m_nInSessionApiLocked(-1), m_bUnlockForce(false), m_bLockType(0)
	{}
	virtual ~CSotpRequestImpl(void)
	{
		unlockSessionApi();
	}

	//void SetApi(const std::string& sApi) {m_sApi = sApi;}
	//const std::string& GetApi(void) const {return m_sApi;}
	//int GetIsInSessionApiLocked(void) const {return m_bInSessionApiLocked;}

private:
	// for attach
	virtual bool isHasAttachInfo(void) const {return m_sotpParser->isRecvHasAttachInfo();}
	virtual cgcAttachment::pointer getAttachment(void) const {return m_sotpParser->getRecvAttachment();}

	virtual cgcParameter::pointer getParameter(const tstring & sParamName) const {return m_sotpParser->getRecvParameter(sParamName);}
	virtual bool getParameter(const tstring & sParamName, std::vector<cgcParameter::pointer>& outParams) const {return m_sotpParser->getRecvParameter(sParamName, outParams);}
	virtual tstring getParameterValue(const tstring & sParamName, const char* sDefault) const {return m_sotpParser->getRecvParameterValue(sParamName, sDefault);}
	virtual tstring getParameterValue(const tstring & sParamName, const tstring& sDefault) const {return m_sotpParser->getRecvParameterValue(sParamName, sDefault);}
	virtual int getParameterValue(const tstring & sParamName, int nDefault) const {return m_sotpParser->getRecvParameterValue(sParamName, nDefault);}
	virtual bigint getParameterValue(const tstring & sParamName, bigint tDefault) const {return m_sotpParser->getRecvParameterValue(sParamName, tDefault);}
	virtual bool getParameterValue(const tstring & sParamName, bool bDefault) const {return m_sotpParser->getRecvParameterValue(sParamName, bDefault);}
	virtual double getParameterValue(const tstring & sParamName, double fDefault) const {return m_sotpParser->getRecvParameterValue(sParamName, fDefault);}
	virtual size_t getParameterCount(void) const {return m_sotpParser->getRecvParameterCount();}
	virtual const cgcParameterMap & getParameters(void) const {return m_sotpParser->getRecvParameters();}

	// CRequestImpl
	virtual void setProperty(const tstring & key, const tstring & value, bool clear) {CRequestImpl::setProperty(key, value, clear);}
	virtual void setProperty(const tstring & key, int value, bool clear) {CRequestImpl::setProperty(key, value, clear);}
	virtual void setProperty(const tstring & key, double value, bool clear) {CRequestImpl::setProperty(key, value, clear);}
	virtual cgcValueInfo::pointer getProperty(const tstring & key) const {return CRequestImpl::getProperty(key);}
	virtual cgcAttributes::pointer getAttributes(bool create) {return CRequestImpl::getAttributes(create);}
	virtual cgcAttributes::pointer getAttributes(void) const {return CRequestImpl::getAttributes();}
	virtual cgcSession::pointer getSession(void) const {return CRequestImpl::getSession();}
	virtual const char * getContentData(void) const {return CRequestImpl::getContentData();}
	virtual size_t getContentLength(void) const {return CRequestImpl::getContentLength();}
	virtual bool lockSessionApi(int nLockType, int nLockSeconds=0, bool bUnlockForce=false){
	//virtual bool isInSessionApiLocked(int nLockSeconds) {
	
		cgcSession::pointer pSession = getSession();
		const bool bInSessionApiLocked = (pSession.get()!=NULL)?((CSessionImpl*)pSession.get())->lockSessionApi(nLockType,nLockSeconds):false;
		m_nInSessionApiLocked = bInSessionApiLocked?1:0;
		m_bUnlockForce = bUnlockForce;
		m_bLockType = nLockType;
		return bInSessionApiLocked;
	}
	virtual void unlockSessionApi(void){
		if (m_nInSessionApiLocked==0 || m_bUnlockForce)
		{
			m_nInSessionApiLocked = -1;
			m_bUnlockForce = false;
			cgcSession::pointer pSession = getSession();
			if (pSession.get()!=NULL)
			{
				((CSessionImpl*)pSession.get())->unlockSessionApi(m_bLockType);
			}
		}
	}

	virtual tstring getScheme(void) const {return CRequestImpl::getScheme();}
	virtual int getProtocol(void) const {return CRequestImpl::getProtocol();}
	virtual int getServerPort(void) const {return CRequestImpl::getServerPort();}
	virtual const tstring & getServerAddr(void) const {return CRequestImpl::getServerAddr();}
	virtual unsigned long getRemoteId(void) const {return CRequestImpl::getRemoteId();}
	virtual unsigned long getIpAddress(void) const {return CRequestImpl::getIpAddress();}
	virtual const tstring & getRemoteAddr(void) const {return CRequestImpl::getRemoteAddr();}
	virtual const tstring & getRemoteHost(void) const {return CRequestImpl::getRemoteHost();}
	virtual tstring getRemoteAccount(void) const {return CRequestImpl::getRemoteAccount();}
	virtual tstring getRemoteSecure(void) const {return CRequestImpl::getRemoteSecure();}
private:
	int m_nInSessionApiLocked;
	bool m_bUnlockForce;
	int m_bLockType;
};

#endif // _sotprequestimpl_head_included__
