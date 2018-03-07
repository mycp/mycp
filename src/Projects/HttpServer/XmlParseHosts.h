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

// XmlParseHosts.h file here
#ifndef __XmlParseHosts_h__
#define __XmlParseHosts_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

#include "VirtualHost.h"

namespace mycp {

class XmlParseHosts
{
public:
	XmlParseHosts(void)
	{}
	~XmlParseHosts(void)
	{
		clear();
	}

public:
	void clear(void) {m_virtualHost.clear();m_pFastcgiList.clear();}
	void addVirtualHst(const tstring& host, CVirtualHost::pointer virtualHost)
	{
		if (virtualHost.get() != NULL)
			m_virtualHost.insert(host, virtualHost);
	}
	CVirtualHost::pointer getVirtualHost(const tstring& host) const
	{
		CVirtualHost::pointer result;
		return m_virtualHost.find(host, result) ? result : NullVirtualHost;
	}
	const CLockMap<tstring, CVirtualHost::pointer>& getHosts(void) const {return m_virtualHost;}
	CFastcgiInfo::pointer getFastcgiInfo(const tstring& name) const
	{
		CFastcgiInfo::pointer result;
		return m_pFastcgiList.find(name, result) ? result : NullFastcgiInfo;
	}
	void delAllProperty(const tstring& key)
	{
		CLockMap<tstring, CVirtualHost::pointer>::iterator pIter = m_virtualHost.begin();
		for (; pIter!=m_virtualHost.end(); pIter++)
		{
			pIter->second->delProperty(key);
		}
	}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("hosts"))
				Insert(v);

		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("virtualhost") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string host = v.second.get("host", "");
			if (host.empty()) return;
			std::string servername = v.second.get("servername", "");
			std::string documentroot = v.second.get("documentroot", "");
			//int roottype = v.second.get("roottype", 0);
			std::string index = v.second.get("index", "");

			CVirtualHost::pointer virtualHost = VIRTUALHOST_INFO(host);
			virtualHost->setServerName(servername);
			virtualHost->setDocumentRoot(documentroot);
			//virtualHost->setRootType(roottype);
			virtualHost->setIndex(index);
			m_virtualHost.insert(host, virtualHost);
		}else if (v.first.compare("fastcgi") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string name = v.second.get("script_name", "");
			if (name.empty()) return;
			std::string url = v.second.get("fastcgi_pass", "");
			std::string index = v.second.get("fastcgi_index", "");
			std::transform(name.begin(), name.end(), name.begin(), tolower);
			CFastcgiInfo::pointer pFastcgiInfo = CFastcgiInfo::create(name,url);
			pFastcgiInfo->m_sFastcgiIndex = v.second.get("fastcgi_index", "");
			pFastcgiInfo->m_sFastcgiPath = v.second.get("fastcgi_path", "");
			pFastcgiInfo->m_nResponseTimeout = v.second.get("fastcgi_response_timeout", 30);
			pFastcgiInfo->m_nMinProcess = v.second.get("fastcgi_min_process", 5);
			pFastcgiInfo->m_nMaxProcess = v.second.get("fastcgi_max_process", 20);
			pFastcgiInfo->m_nMaxRequestRestart = v.second.get("fastcgi_max_requests", 500);
			m_pFastcgiList.insert(name,pFastcgiInfo);
		}
	}

	CLockMap<tstring, CVirtualHost::pointer> m_virtualHost;
	CLockMap<tstring, CFastcgiInfo::pointer> m_pFastcgiList;
};

} // namespace mycp

#endif // __XmlParseHosts_h__
