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

// XmlParseWeb.h file here
#ifndef __XmlParseWeb_h__
#define __XmlParseWeb_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

class CServletInfo
{
public:
	typedef std::shared_ptr<CServletInfo> pointer;
	const tstring & getServletRequest(void) const {return m_sServletRequest;}
	const tstring & getServletApp(void) const {return m_sServletApp;}
	//const tstring & getServletRunction(void) const {return m_sServletFunction;}
	//const tstring& getServletWebPath(void) const {return m_sServletWebPath;}

	CServletInfo(const tstring & request, const tstring & app)
		: m_sServletRequest(request), m_sServletApp(app)
		//, m_sServletWebPath(webPath)
	{
	}

private:
	tstring m_sServletRequest;
	tstring m_sServletApp;
	//tstring m_sServletWebPath;
	//tstring m_sServletFunction;
};
const CServletInfo::pointer cgcNullServletInfo;


class XmlParseWeb
{
public:
	XmlParseWeb(void)
	{}
	~XmlParseWeb(void)
	{
		FreeHandle();
	}

	CLockMap<tstring, CServletInfo::pointer> m_servlets;	// servletname ->

public:
	void FreeHandle(void) {m_servlets.clear();}
	CServletInfo::pointer getServletInfo(const tstring& request) const
	{
		CServletInfo::pointer result;
		return m_servlets.find(request, result) ? result : cgcNullServletInfo;
	}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("web-app"))
				Insert(v);

		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("servlet") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			const std::string request = v.second.get("servlet-request", "");
			const std::string app = v.second.get("servlet-app", "");
			if (request.empty()) return;

			//std::string function = v.second.get("servlet-funcntion", "doGET");
			//const std::string webPath = v.second.get("servlet-web-path", "");

			CServletInfo::pointer servletInfo = CServletInfo::pointer(new CServletInfo(request, app));
			m_servlets.insert(request, servletInfo);
		}
	}
};

#endif // __XmlParseWeb_h__
