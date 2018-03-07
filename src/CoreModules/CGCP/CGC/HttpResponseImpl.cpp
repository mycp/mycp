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
#ifdef WIN32
#include <winsock2.h>
#endif
#include "HttpResponseImpl.h"
#include <stdarg.h>
#include "SessionMgr.h"

CHttpResponseImpl::CHttpResponseImpl(const cgcRemote::pointer& pcgcRemote, const cgcParserHttp::pointer& pcgcParser)
: m_cgcRemote(pcgcRemote)
, m_cgcParser(pcgcParser)
, m_pParserHttpHandler(NULL)
, m_bResponseSended(false)
, m_bNotResponse(false)
, m_nHoldSecond(-1)
, m_pSendLock(NULL)
, m_bForward(false)

{
	BOOST_ASSERT (m_cgcParser.get() != 0);
}

CHttpResponseImpl::~CHttpResponseImpl(void)
{
	if (m_pParserHttpHandler!=NULL)
	{
		//printf("onReturnParserHttp\n");
		m_pParserHttpHandler->onReturnParserHttp(m_cgcParser);
	}
	if (m_pSendLock)
		delete m_pSendLock;
}

void CHttpResponseImpl::setCgcRemote(const cgcRemote::pointer& pcgcRemote)
{
	m_cgcRemote = pcgcRemote;
	BOOST_ASSERT (m_cgcRemote.get() != 0);
}

//void CHttpResponseImpl::setCgcParser(const cgcParserHttp::pointer& pcgcParser)
//{
//	m_cgcParser = pcgcParser;
//	BOOST_ASSERT (m_cgcParser.get() != 0);
//}

void CHttpResponseImpl::lockResponse(void)
{
	//if (m_bNotResponse)
	//	return;
	boost::mutex::scoped_lock * pLockTemp = new boost::mutex::scoped_lock(m_sendMutex);
	m_pSendLock = pLockTemp;
}

int CHttpResponseImpl::sendResponse(bool bSendForce)
{
	try
	{
		//printf("**** sendResponse(bSendForce=%d)\n",(int)(bSendForce?1:0));
		if (isInvalidate() || m_cgcParser.get() == 0) return -1;
		//printf("**** sendResponse(bSendForce=%d), m_bNotResponse=%d\n",(int)(bSendForce?1:0),(int)(m_bNotResponse?1:0));
		if (m_bNotResponse || (m_bResponseSended && !bSendForce))
		{
			// ** setNotResponse()已经保存，不需要重复处理；
			//if (m_session.get() != NULL)
			//{
			//	CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
			//	if (m_nHoldSecond > 0)
			//		pSessionImpl->HoldResponse(this->shared_from_this(), m_nHoldSecond);
			//	//else if (m_nHoldSecond == 0)
			//	//{
			//	//	m_nHoldSecond = -1;
			//	//	pSessionImpl->removeResponse(getRemoteId());
			//	//}
			//}
			return 0;
		}else if (m_nHoldSecond > 0)
		{
			if (m_session.get() != NULL)
			{
				CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
				m_nHoldSecond = -1;
				pSessionImpl->removeResponse(getRemoteId());
			}
		}

		//setHeader("P3P","CP=CAO PSA OUR");	// 解决IE跨域问题
		boost::mutex::scoped_lock * pSendLockTemp = m_pSendLock;
		m_pSendLock = NULL;
		m_bResponseSended = true;
		m_bForward = false;
		//std::string responseData = m_cgcParser->getResult();

		size_t outSize = 0;
		//printf("**** CHttpResponseImpl::getHttpResult...\n");
		const char * responseData = m_cgcParser->getHttpResult(outSize);
		//printf("**** CHttpResponseImpl::getHttpResult size=%d, pSendLockTemp=%x, sendData(0x%x)...\n",outSize,(int)pSendLockTemp,(int)m_cgcRemote.get());
		if (pSendLockTemp)
			delete pSendLockTemp;
		const int ret = m_cgcRemote->sendData((const unsigned char*)responseData, outSize);
		const Http_StatusCode nHttpStatusCode = getStatusCode();
		//printf("**** nHttpStatusCode=%d\n",(int)nHttpStatusCode);
		//if (nHttpStatusCode==STATUS_CODE_302)
		//	printf("**** CHttpResponseImpl::getHttpResult...\n%s\n",responseData);
		//printf("**** CHttpResponseImpl->m_cgcRemote->sendData ok, StatusCode=%d, reset(?)\n",(int)nHttpStatusCode);
		if (nHttpStatusCode != STATUS_CODE_206 && nHttpStatusCode != STATUS_CODE_100)
		{
			reset();
		}
		return ret;
	}catch (const boost::exception &)
	{
	}catch (const std::exception &)
	{
	}catch (...)
	{
	}

	return -105;
}

void CHttpResponseImpl::println(const char * format,...)
{
	try
	{
		char bufferTemp[MAX_FORMAT_SIZE+1];
		memset(bufferTemp, 0, MAX_FORMAT_SIZE+1);
		va_list   vl;
		va_start(vl, format);
		int len = vsnprintf(bufferTemp, MAX_FORMAT_SIZE, format, vl);
		va_end(vl);
		if (len > MAX_FORMAT_SIZE)
			len = MAX_FORMAT_SIZE;
		bufferTemp[len] = '\0';
		m_cgcParser->println(bufferTemp, len);
	}catch(std::exception const &)
	{
	}catch(...)
	{
	}
}

void CHttpResponseImpl::write(const char * format,...)
{
	try
	{
		char bufferTemp[MAX_FORMAT_SIZE+1];
		memset(bufferTemp, 0, MAX_FORMAT_SIZE+1);
		va_list   vl;
		va_start(vl, format);
		int len = vsnprintf(bufferTemp, MAX_FORMAT_SIZE, format, vl);
		va_end(vl);
		if (len > MAX_FORMAT_SIZE)
			len = MAX_FORMAT_SIZE;
		bufferTemp[len] = '\0';
		m_cgcParser->write(bufferTemp, len);
	}catch(std::exception const &)
	{
	}catch(...)
	{
	}
}

void CHttpResponseImpl::invalidate(void)
{
	if (m_cgcRemote.get() != NULL)
		m_cgcRemote->invalidate();
}

int CHttpResponseImpl::sendResponse(HTTP_STATUSCODE statusCode)
{
	setStatusCode(statusCode);
	m_bResponseSended = true;
	m_bNotResponse = false;
	return this->sendResponse(true);
}

unsigned long CHttpResponseImpl::setNotResponse(int nHoldSecond)
{
	m_bNotResponse = true;
	if (m_nHoldSecond > 0  || nHoldSecond > 0)
	{
		m_nHoldSecond = nHoldSecond;

		if (m_session.get() != NULL)
		{
			//printf("**** CHttpResponseImpl::setNotResponse %d sid=%s\n",getRemoteId(), m_session->getId().c_str());
			CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
			if (m_nHoldSecond <= 0)
			{
				m_nHoldSecond = -1;
				pSessionImpl->removeResponse(getRemoteId());
			}else
			{
				// ** 这里必须记下来，否则外面业务处理太快，会来不及；
				pSessionImpl->HoldResponse(this->shared_from_this(), m_nHoldSecond);
			}
		}
	}
	return this->getRemoteId();
}
