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

// HttpResponseImpl.h file here
#ifndef _httpresponseimpl_head_included__
#define _httpresponseimpl_head_included__

#include "IncludeBase.h"

class CParserHttpHandler
{
public:
	virtual void onReturnParserHttp(cgcParserHttp::pointer cgcParser) = 0;
};

class CHttpResponseImpl
	: public cgcHttpResponse
	, public std::enable_shared_from_this<CHttpResponseImpl>
{
private:
	cgcRemote::pointer m_cgcRemote;
	cgcParserHttp::pointer m_cgcParser;
	cgcSession::pointer m_session;
	CParserHttpHandler * m_pParserHttpHandler;
	bool m_bResponseSended;
	bool m_bNotResponse;
	int m_nHoldSecond;		// default:-1; 0:remove; >0:hold
	boost::mutex m_sendMutex;
	boost::mutex::scoped_lock * m_pSendLock;

	bool m_bForward;
public:
	CHttpResponseImpl(const cgcRemote::pointer& pcgcRemote, const cgcParserHttp::pointer& pcgcParser);
	virtual ~CHttpResponseImpl(void);
	
	int sendResponse(bool bSendForce = false);
	bool getForward(void) const {return m_bForward;}
	void setForward(bool v) {m_bForward = v;}

	void SetParserHttpHandler(CParserHttpHandler * pParserHttpHandler) {m_pParserHttpHandler = pParserHttpHandler;}

	//void clearCgcRemote(void);
	void setCgcRemote(const cgcRemote::pointer& pcgcRemote);
	void setSessionClosed(void) {}
	//void setCgcParser(const cgcParserHttp::pointer& pcgcParser);
	void setSession(const cgcSession::pointer& session) {m_session = session;}
	//cgcParserHttp::pointer getCgcParser(void) const {return m_cgcParser;}
	cgcRemote::pointer getCgcRemote(void) const {return m_cgcRemote;}
	unsigned long getCommId(void) const {return m_cgcRemote.get() == NULL ? 0 : m_cgcRemote->getCommId();}

	bool isResponseSended(void) const {return m_bResponseSended;}
	bool IsInvalidate(void) const {return isInvalidate();}
	virtual int sendResponse(const char * pResponseData, size_t nResponseSize) {return -1;}
	//virtual unsigned long setNotResponse(void) {m_bNotResponse = true;}
	virtual unsigned long setNotResponse(int nHoldSecond);

protected:
	virtual void println(const char * format,...);
	virtual void println(const tstring& text) {m_cgcParser->println(text);}
	virtual void write(const char * format,...);
	virtual void write(const tstring& text) {m_cgcParser->write(text);}
	virtual void newline(void) {m_cgcParser->newline();}
	virtual void reset(void) {m_cgcParser->reset();}

	virtual void writeData(const char * buffer, size_t size) {m_cgcParser->write(buffer, size);}

	virtual void setCookie(const tstring& name, const tstring& value) {m_cgcParser->setCookie(name,value);}
	virtual void setCookie(const cgcCookieInfo::pointer& pCookieInfo) {m_cgcParser->setCookie(pCookieInfo);}
	virtual void setHeader(const tstring& name, const tstring& value) {m_cgcParser->setHeader(name,value);}
	virtual void addDateHeader(void) {m_cgcParser->addDateHeader();}
	virtual size_t getBodySize(void) const {return m_cgcParser->getBodySize();}

	virtual void setStatusCode(HTTP_STATUSCODE statusCode) {m_cgcParser->setStatusCode(statusCode);}
	virtual HTTP_STATUSCODE getStatusCode(void) const {return m_cgcParser->getStatusCode();}
	virtual void setKeepAlive(int nKeepAliveInterval) {m_cgcParser->setKeepAlive(nKeepAliveInterval);}

	virtual void setContentType(const tstring& contentType) {m_cgcParser->setContentType(contentType);}
	virtual const tstring& getContentType(void) const {return m_cgcParser->getContentType();}

	virtual void forward(const tstring& url) {m_bForward = true; m_cgcParser->forward(url);}
	virtual void location(const tstring& url) {m_cgcParser->location(url);}

	//virtual cgcSession::pointer getSession(void) const {return m_session;}
	virtual unsigned long getRemoteId(void) const {return m_cgcRemote.get() == NULL ? 0 : m_cgcRemote->getRemoteId();}
	virtual void invalidate(void);
	virtual bool isInvalidate(void) const {return m_cgcRemote.get() == NULL ? true : m_cgcRemote->isInvalidate();}

	virtual void lockResponse(void);
	virtual int sendResponse(HTTP_STATUSCODE statusCode);

	//tstring GetResponseClusters(const ClusterSvrList & clusters);

};

#endif // _httpresponseimpl_head_included__
