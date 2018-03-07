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

// HttpRequestImpl.h file here
#ifndef _httprequestimpl_head_included__
#define _httprequestimpl_head_included__

#include "IncludeBase.h"
#include <CGCClass/AttributesImpl.h>
//#include "AttributesImpl.h"
#include "XmlParseParams.h"
#include "RequestImpl.h"

class CHttpRequestImpl;
typedef std::map<unsigned long, CHttpRequestImpl*> CHttpRequestImplMap;
typedef CHttpRequestImplMap::iterator CHttpRequestImplMapIter;
typedef std::pair<unsigned long, CHttpRequestImpl*> CHttpRequestImplMapPair;

// CHttpRequestImpl
class CHttpRequestImpl
	: public cgcHttpRequest
	, public CRequestImpl
{
private:
	cgcParserHttp::pointer m_httpParser;
public:
	CHttpRequestImpl(cgcRemote::pointer pcgcRemote, cgcParserHttp::pointer pcgcParser)
		: CRequestImpl(pcgcRemote, pcgcParser)
		, m_httpParser(pcgcParser)
	{}
	virtual ~CHttpRequestImpl(void)
	{}

private:
	virtual const tstring& getHttpVersion(void) const {return m_httpParser->getHttpVersion();}
	virtual const tstring& getRestVersion(void) const {return m_httpParser->getRestVersion();}
	virtual const tstring& getModuleName(void) const {return m_httpParser->getModuleName();}
	virtual const tstring& getFunctionName(void) const {return m_httpParser->getFunctionName();}

	virtual const tstring& getHost(void) const {return m_httpParser->getHost();}
	virtual HTTP_METHOD getHttpMethod(void) const {return m_httpParser->getHttpMethod();}
	//virtual size_t getContentLength(void) const {return m_httpParser->getContentLength();}
	virtual const tstring& getContentType(void) const {return m_httpParser->getReqContentType();}
	virtual const tstring& getRequestURL(void) const {return m_httpParser->getRequestURL();}
	virtual const tstring& getRequestURI(void) const {return m_httpParser->getRequestURI();}
	virtual const tstring& getQueryString(void) const {return m_httpParser->getQueryString();}
	virtual const tstring& getFileName(void) const {return m_httpParser->getFileName();}
	virtual unsigned int getRangeFrom(void) const {return m_httpParser->getRangeFrom();}
	virtual unsigned int getRangeTo(void) const {return m_httpParser->getRangeTo();}

	virtual bool isKeepAlive(void) const {return m_httpParser->isKeepAlive();}
	//virtual size_t getContentLength(void) const {return m_httpParser->getContentLength();}
	//virtual const char* getContentData(void) const {return m_httpParser->getContentData();}
	virtual bool getUploadFile(std::vector<cgcUploadFile::pointer>& outFiles) const {return m_httpParser->getUploadFile(outFiles);}

	virtual bool isFrowardFrom(void) const {return !m_httpParser->getForwardFromURL().empty();}
	virtual const tstring& getForwardFromURL(void) const {return m_httpParser->getForwardFromURL();}

	virtual cgcValueInfo::pointer getParameter(const tstring & paramName) const {return m_httpParser->getParameter(paramName);}
	//virtual bool getParameter(const tstring & paramName, std::vector<cgcValueInfo::pointer>& outParameters) const {return m_httpParser->getParameter(paramName, outParameters);}
	virtual bool getParameters(std::vector<cgcKeyValue::pointer>& outParameters) const {return m_httpParser->getParameters(outParameters);}
	virtual tstring getParameterValue(const tstring & sParamName, const char * sDefault = "") const
	{
		cgcValueInfo::pointer var = getParameter(sParamName);
		return var.get()==NULL?(tstring)(sDefault):var->getStrValue();
	}
	virtual tstring getParameterValue(const tstring & sParamName, const tstring& sDefault) const
	{
		cgcValueInfo::pointer var = getParameter(sParamName);
		return var.get()==NULL?sDefault:var->getStrValue();
	}
	virtual int getParameterValue(const tstring & sParamName, int nDefault) const
	{
		cgcValueInfo::pointer var = getParameter(sParamName);
		return var.get()==NULL?nDefault:var->getIntValue();
	}
	virtual bigint getParameterValue(const tstring & sParamName, bigint bDefault) const
	{
		cgcValueInfo::pointer var = getParameter(sParamName);
		return var.get()==NULL?bDefault:var->getBigIntValue();
	}
	virtual bool getParameterValue(const tstring & sParamName, bool bDefault) const
	{
		cgcValueInfo::pointer var = getParameter(sParamName);
		return var.get()==NULL?bDefault:var->getBooleanValue();
	}
	virtual double getParameterValue(const tstring & sParamName, double fDefault) const
	{
		cgcValueInfo::pointer var = getParameter(sParamName);
		return var.get()==NULL?fDefault:var->getFloatValue();
	}

	virtual tstring getHeader(const tstring & headerName, const tstring & sDefault) const {return m_httpParser->getHeader(headerName, sDefault);}
	virtual bool getHeaders(std::vector<cgcKeyValue::pointer>& outHeaders) const {return m_httpParser->getHeaders(outHeaders);}

	virtual tstring getCookie(const tstring & name, const tstring & sDefault = _T("")) const {return m_httpParser->getCookie(name,sDefault);}
	virtual bool getCookies(std::vector<cgcKeyValue::pointer>& outCookies) const {return m_httpParser->getCookies(outCookies);}
	cgcAttributes::pointer m_pPageAttributes;
	virtual cgcAttributes::pointer getPageAttributes(void) const {return m_pPageAttributes;}
	virtual void setPageAttributes(const cgcAttributes::pointer& pPageAttributes) {m_pPageAttributes = pPageAttributes;}

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
	virtual bool lockSessionApi(int nLockApi, int nTimeoutSeconds=0, bool bUnlockForce=false) {return false;}
	virtual void unlockSessionApi(void) {}
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

};

#endif // _httprequestimpl_head_included__
