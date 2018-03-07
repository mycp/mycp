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

// ResponseImpl.h file here
#ifndef _responseimpl_head_included__
#define _responseimpl_head_included__

#include "IncludeBase.h"

class CResponseImpl
	: public cgcResponse
{
private:
	cgcRemote::pointer m_cgcRemote;
	cgcParserBase::pointer m_cgcParser;
	cgcSession::pointer m_session;
	bool m_bNotResponse;
	int m_nHoldSecond;
public:
	CResponseImpl(cgcRemote::pointer pcgcRemote, cgcParserBase::pointer pcgcParser)
		: m_cgcRemote(pcgcRemote)
		, m_cgcParser(pcgcParser)
		, m_bNotResponse(false)
		, m_nHoldSecond(0)
	{}
	virtual ~CResponseImpl(void) {}

	void setSession(const cgcSession::pointer& session) {m_session = session;}
	int getHoldSecond(void) const {return m_nHoldSecond;}

protected:
	//virtual cgcSession::pointer getSession(void) const {return m_session;}
	virtual unsigned long getRemoteId(void) const {return m_cgcRemote.get() == NULL ? 0 : m_cgcRemote->getRemoteId();}
	virtual void invalidate(void)
	{
		if (m_cgcRemote.get() != NULL)
			m_cgcRemote->invalidate();
	}
	virtual bool isInvalidate(void) const {return m_cgcRemote.get() == NULL ? true : m_cgcRemote->isInvalidate();}
	virtual int sendResponse(const char * pResponseData, size_t nResponseSize)
	{
		return m_cgcRemote->sendData((const unsigned char*)pResponseData, nResponseSize);
	}
	virtual unsigned long setNotResponse(int nHoldSecond) {m_bNotResponse = true;m_nHoldSecond = nHoldSecond;return getRemoteId();}
	//virtual unsigned long holdResponse(int nHoldSecond) {m_nHoldSecond = nHoldSecond; return getRemoteId();}
};

#endif // _responseimpl_head_included__
