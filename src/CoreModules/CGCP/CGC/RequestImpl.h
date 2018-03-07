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

// RequestImpl.h file here
#ifndef __requestimpl_head_included__
#define __requestimpl_head_included__

#include "IncludeBase.h"
#include <CGCClass/AttributesImpl.h>
//#include "AttributesImpl.h"

// CRequestImpl
class CRequestImpl
	: public cgcRequest
{
protected:
	cgcRemote::pointer m_cgcRemote;
	cgcParserBase::pointer m_pcgcParser;
	cgcSession::pointer m_session;
	cgcAttributes::pointer m_attributes;
	const char * m_pContentData;
	size_t m_nContentLength;
	bool m_bInSessionApiLocked;
public:
	CRequestImpl(cgcRemote::pointer pcgcRemote, cgcParserBase::pointer pcgcParser)
		: m_cgcRemote(pcgcRemote), m_pcgcParser(pcgcParser)
		, m_pContentData(NULL), m_nContentLength(0)
		, m_bInSessionApiLocked(false)
	{
		BOOST_ASSERT (m_cgcRemote.get() != 0);
		//BOOST_ASSERT (m_pcgcParser.get() != 0);
	}
	virtual ~CRequestImpl(void)
	{
		m_attributes.reset();
	}

	cgcRemote::pointer getRemote(void) const {return m_cgcRemote;}
	void setSession(const cgcSession::pointer& session) {m_session = session;}
	void setAttributes(const cgcAttributes::pointer& attributes) {m_attributes=attributes;}
	void setContent(const char * pData, size_t nLength) {m_pContentData = pData; m_nContentLength = nLength;}
	void SetSessionApiLocked(bool bInLocked) {m_bInSessionApiLocked = bInLocked;}
protected:
	virtual void setProperty(const tstring & propertyName, const tstring & propertyValue, bool clear)
	{
		getAttributes(true)->setProperty(propertyName, CGC_VALUEINFO(propertyValue), clear);
	}
	virtual void setProperty(const tstring & propertyName, int propertyValue, bool clear)
	{
		getAttributes(true)->setProperty(propertyName, CGC_VALUEINFO(propertyValue), clear);
	}
	virtual void setProperty(const tstring & propertyName, double propertyValue, bool clear)
	{
		getAttributes(true)->setProperty(propertyName, CGC_VALUEINFO(propertyValue), clear);
	}
	virtual cgcValueInfo::pointer getProperty(const tstring & propertyName) const
	{
		if (m_attributes.get() != NULL)
		{
			std::vector<cgcValueInfo::pointer> vectors;
			if (m_attributes->getProperty(propertyName, vectors))
			{
				cgcValueInfo::pointer result;
				if (vectors.size() == 1)
				{
					result = vectors[0]->copy();;
				}else
				{
					result = CGC_VALUEINFO(vectors);
				}
				return result;
			}
		}
		return cgcNullValueInfo;
	}
	virtual cgcAttributes::pointer getAttributes(bool create)
	{
		if (create && m_attributes.get() == NULL)
			m_attributes = cgcAttributes::pointer(new AttributesImpl());
		return m_attributes;
	}
	virtual cgcAttributes::pointer getAttributes(void) const {return m_attributes;}
	virtual cgcSession::pointer getSession(void) const {return m_session;}
	virtual const char * getContentData(void) const {return m_pContentData;}
	virtual size_t getContentLength(void) const {return m_nContentLength;}
	virtual bool lockSessionApi(int nLockApi, int nTimeoutSeconds=0, bool bUnlockForce=false) {return false;}
	virtual void unlockSessionApi(void) {}

	//virtual size_t getContentLength(void) const {return 0;}
//	virtual const tstring & getContentType(void) const {return m_sContentType;}
	virtual tstring getScheme(void) const {return m_cgcRemote->getScheme();}

	virtual int getProtocol(void) const {return m_cgcRemote->getProtocol();}
	virtual int getServerPort(void) const {return m_cgcRemote->getServerPort();}
	virtual const tstring & getServerAddr(void) const {return m_cgcRemote->getServerAddr();}
	virtual unsigned long getRemoteId(void) const {return m_cgcRemote->getRemoteId();}
	virtual unsigned long getIpAddress(void) const {return m_cgcRemote->getIpAddress();}
	virtual const tstring & getRemoteAddr(void) const {return m_cgcRemote->getRemoteAddr();}
	virtual const tstring & getRemoteHost(void) const {return m_cgcRemote->getRemoteHost();}
	virtual tstring getRemoteAccount(void) const {return m_pcgcParser.get() == NULL ? "" : m_pcgcParser->getAccount();}
	virtual tstring getRemoteSecure(void) const {return m_pcgcParser.get() == NULL ? "" : m_pcgcParser->getSecure();}

};

#endif // __requestimpl_head_included__
