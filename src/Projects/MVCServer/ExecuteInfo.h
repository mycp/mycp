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

// ExecuteInfo.h file here
#ifndef __ExecuteInfo_h__
#define __ExecuteInfo_h__

class CExecuteInfo
{
public:
	typedef boost::shared_ptr<CExecuteInfo> pointer;

	CLockMap<tstring, tstring> m_controlerForwardUrls;	// FaileCode - ForwardURL
	CLockMap<tstring, tstring> m_controlerLocationUrls;	// FaileCode - LocationURL

	const tstring& getRequestUrl(void) const {return m_requestUrl;}
	const tstring& getExecuteApp(void) const {return m_executeApp;}

	void setExecuteFunction(const tstring& v) {m_executeFunction = v;}
	const tstring getExecuteFunction(void) const {return m_executeFunction;}

	void setNormalForwardUrl(const tstring& v) {m_normalForward = v;}
	const tstring& getNormalForwardUrl(void) const {return m_normalForward;}

	CExecuteInfo(const tstring& requestUrl, const tstring& executeApp)
		: m_requestUrl(requestUrl), m_executeApp(executeApp), m_executeFunction(_T("")), m_normalForward("")
	{}
	~CExecuteInfo(void)
	{
		m_controlerForwardUrls.clear();
		m_controlerLocationUrls.clear();
	}
private:
	tstring m_requestUrl;
	tstring m_executeApp;
	tstring m_executeFunction;
	tstring m_normalForward;

};
const CExecuteInfo::pointer NullExecuteInfo;

#define EXECUTEINFO_INFO(url, app) CExecuteInfo::pointer(new CExecuteInfo(url, app))

#endif // __ExecuteInfo_h__
