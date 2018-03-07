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

// XmlParseStructs.h file here
#ifndef __XmlParseStructs_h__
#define __XmlParseStructs_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

#include "ValidateInfo.h"
#include "ExecuteInfo.h"

class XmlParseStructs
{
public:
	typedef boost::shared_ptr<XmlParseStructs> pointer;
	XmlParseStructs(void)
	{}
	~XmlParseStructs(void)
	{
		clear();
	}

public:
	void clear(void) {m_validates.clear(); m_executes.clear();}
	//void addVirtualHst(const tstring& host, CVirtualHost::pointer virtualHost)
	//{
	//	if (virtualHost.get() != NULL)
	//		m_virtualHost.insert(host, virtualHost);
	//}
	CValidateInfo::pointer getValidateInfo(const tstring& reqeestUrl) const
	{
		CValidateInfo::pointer result;
		return m_validates.find(reqeestUrl, result) ? result : NullValidateInfo;
	}
	CExecuteInfo::pointer getExecuteInfo(const tstring& reqeestUrl) const
	{
		CExecuteInfo::pointer result;
		return m_executes.find(reqeestUrl, result) ? result : NullExecuteInfo;
	}
	//const CLockMap<tstring, CVirtualHost::pointer>& getHosts(void) const {return m_virtualHost;}

	void setServletName(const tstring & name) {m_sServletName = name;}
	const tstring & getServletName(void) const {return m_sServletName;}
	const tstring & getProfix(void) const {return m_sProfix;}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		try
		{
			m_sProfix = pt.get("propertys.prefix", _T(""));
			if (m_sProfix.empty() || m_sProfix.c_str()[m_sProfix.size()-1] != '/')
				m_sProfix.append("/");


			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("url-validates"))
				InsertValidate(v);

			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("validate-executes"))
				InsertExecute(v);

		}catch(...)
		{}
	}

private:
	void InsertValidate(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("validate") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string requestUrl = v.second.get("request-url", "");
			std::string validateApp = v.second.get("validate-app", "");
			if (requestUrl.empty() || validateApp.empty()) return;

			std::string validateFunction = v.second.get("validate-function", "doGET");
			std::string failForward = v.second.get("fail-forward-url", "");

			CValidateInfo::pointer validate = VALIDATEINFO_INFO(requestUrl, validateApp);
			validate->setValidateFunction(validateFunction);
			validate->setFailForwardUrl(failForward);
			m_validates.insert(requestUrl, validate);
		}
	}
	void InsertExecute(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("execute") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string requestUrl = v.second.get("request-url", "");
			//if (requestUrl.empty())
			//	requestUrl = v.second.get("servlet-function", "");		// ?
			std::string executeApp = v.second.get("execute-app", "");
			if (requestUrl.empty()) return;

			if (requestUrl.size() < 2 || requestUrl.substr(0, 2) != "do")
			{
				tstring temp("/servlet/");
				temp.append(m_sServletName);
				temp.append("/");
				temp.append(requestUrl);
				requestUrl = temp;
			}

			std::string executeFunction = v.second.get("execute-function", "doGET");
			std::string normalForward = v.second.get("normal-forward-url", "");
			if (!normalForward.empty() && normalForward.c_str()[0] != '/')
				normalForward = m_sProfix + normalForward;
			if (executeApp.empty() && normalForward.empty()) return;

			CExecuteInfo::pointer execute = EXECUTEINFO_INFO(requestUrl, executeApp);

			BOOST_FOREACH(const ptree::value_type &sv, v.second)
				InsertSubExecute(sv, execute);

			execute->setExecuteFunction(executeFunction);
			execute->setNormalForwardUrl(normalForward);
			m_executes.insert(requestUrl, execute);
		}
	}
	void InsertSubExecute(const boost::property_tree::ptree::value_type & v, CExecuteInfo::pointer execute)
	{
		if (v.first.compare("controler") == 0)
		{
			std::string failCode = v.second.get("result-code", "");
			if (failCode.empty()) return;
			std::string forwardUrl = v.second.get("forward-url", "");
			if (!forwardUrl.empty())
			{
				if (forwardUrl.c_str()[0] == '/')
					execute->m_controlerForwardUrls.insert(failCode, forwardUrl);
				else
					execute->m_controlerForwardUrls.insert(failCode, m_sProfix+forwardUrl);
			}

			std::string locationUrl = v.second.get("location-url", "");
			if (!locationUrl.empty())
			{
				if (locationUrl.c_str()[0] == '/')
					execute->m_controlerLocationUrls.insert(failCode, locationUrl);
				else
					execute->m_controlerLocationUrls.insert(failCode, m_sProfix+locationUrl);
			}

		}
	}

	CLockMap<tstring, CValidateInfo::pointer> m_validates;
	CLockMap<tstring, CExecuteInfo::pointer> m_executes;
	tstring m_sServletName;	// mvctest
	tstring m_sProfix;

};

#endif // __XmlParseStructs_h__
