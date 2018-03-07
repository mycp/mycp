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

// XmlParseApps.h file here
#ifndef __XmlParseApps_h__
#define __XmlParseApps_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

namespace mycp {

class CAppInfo
{
public:
	typedef boost::shared_ptr<CAppInfo> pointer;

	const tstring & getAppId(void) const {return m_appid;}
	const tstring & getAppName(void) const {return m_appname;}
	const tstring & getInitParam(void) const {return m_initparam;}

	void setIntParam(int v) {m_intparam = v;}
	int getIntParam(void) const {return m_intparam;}

	CAppInfo(const tstring& appid, const tstring& appname, const tstring& initparam)
		: m_appid(appid), m_appname(appname), m_initparam(initparam), m_intparam(0)
	{}
private:
	tstring m_appid;
	tstring m_appname;
	tstring m_initparam;
	int m_intparam;
};

#define APPINFO(id, name, init) CAppInfo::pointer(new CAppInfo(id, name, init))

class XmlParseApps
{
public:
	typedef boost::shared_ptr<XmlParseApps> pointer;

	XmlParseApps(void)
	{}
	~XmlParseApps(void)
	{
		clear();
	}

public:
	void clear(void) {m_apps.clear();}
	CAppInfo::pointer getAppInfo(const tstring& appid) const
	{
		CAppInfo::pointer result;
		m_apps.find(appid, result);
		return result;
	}
	const CLockMap<tstring, CAppInfo::pointer>& getApps(void) const {return m_apps;}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("apps"))
				Insert(v);

		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first == "app")
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string appid = v.second.get("app-id", "");
			std::string appname = v.second.get("app-name", "");
			if (appid.empty() || appname.empty()) return;
			std::string initparam = v.second.get("init-param", "");
			int intparam = v.second.get("int-param", 0);

			CAppInfo::pointer appInfo = APPINFO(appid, appname, initparam);
			appInfo->setIntParam(intparam);
			m_apps.insert(appid, appInfo);
		}
	}

	CLockMap<tstring, CAppInfo::pointer> m_apps;

};
#define XMLPARSEAPPS XmlParseApps::pointer(new XmlParseApps())

} // namespace mycp

#endif // __XmlParseApps_h__
