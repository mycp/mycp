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

// ValidateInfo.h file here
#ifndef __ValidateInfo_h__
#define __ValidateInfo_h__

class CValidateInfo
{
public:
	typedef boost::shared_ptr<CValidateInfo> pointer;

	const tstring& getRequestUrl(void) const {return m_requestUrl;}
	const tstring& getValidateApp(void) const {return m_validateApp;}

	void setValidateFunction(const tstring& v) {m_validateFunction = v;}
	const tstring& getValidateFunction(void) const {return m_validateFunction;}
	void setFailForwardUrl(const tstring& v) {m_failForward = v;}
	const tstring& getFailForwardUrl(void) const {return m_failForward;}

	CValidateInfo(const tstring& requestUrl, const tstring& validateApp)
		: m_requestUrl(requestUrl), m_validateApp(validateApp), m_validateFunction(""), m_failForward("")
	{}
private:
	tstring m_requestUrl;
	tstring m_validateApp;
	tstring m_validateFunction;
	tstring m_failForward;

};
const CValidateInfo::pointer NullValidateInfo;

#define VALIDATEINFO_INFO(url, app) CValidateInfo::pointer(new CValidateInfo(url, app))

#endif // __ValidateInfo_h__
