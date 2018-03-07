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

// XmlParseCdbcs.h file here
#ifndef __XmlParseCdbcs_h__
#define __XmlParseCdbcs_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;


class XmlParseCdbcs
{
public:
	XmlParseCdbcs(void)
	{}
	~XmlParseCdbcs(void)
	{
		FreeHandle();
	}

	CLockMap<tstring, cgcCDBCInfo::pointer> m_cdbcInfos;
	CLockMap<tstring, cgcDataSourceInfo::pointer> m_dsInfos;

public:
	void FreeHandle(void) {m_cdbcInfos.clear(); m_dsInfos.clear();}
	cgcCDBCInfo::pointer getCDBCInfo(const tstring& name) const
	{
		cgcCDBCInfo::pointer result;
		return m_cdbcInfos.find(name, result) ? result : cgcNullCDBCInfo;
	}
	cgcDataSourceInfo::pointer getDataSourceInfo(const tstring& datasource) const
	{
		cgcDataSourceInfo::pointer result;
		return m_dsInfos.find(datasource, result) ? result : cgcNullDataSourceInfo;
	}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("cdbcs"))
				Insert(v);

		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("cdbc") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string name = v.second.get("name", "");
			if (name.empty()) return;

			std::string database = v.second.get("database", "");
			std::string host = v.second.get("host", "");
			std::string account = v.second.get("account", "");
			std::string secure = v.second.get("secure", "");
			std::string connection = v.second.get("connection", "");
			std::string charset = v.second.get("charset", "");
			//std::string descriptiop = v.second.get("descriptiop", "");
			int nMin = v.second.get("minsize", 5);
			int nMax = v.second.get("maxsize", 500);

			cgcCDBCInfo::pointer cdbcInfo = CGC_CDBCINFO(name, database);
			cdbcInfo->setHost(host);
			cdbcInfo->setAccount(account);
			cdbcInfo->setSecure(secure);
			cdbcInfo->setConnection(connection);
			cdbcInfo->setCharset(charset);
			cdbcInfo->setMinSize(nMin);
			cdbcInfo->setMaxSize(nMax);
			m_cdbcInfos.insert(name, cdbcInfo);
		}else if (v.first.compare("datasource") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string name = v.second.get("name", "");
			std::string cdbcservice = v.second.get("cdbcservice", "");
			if (name.empty() || cdbcservice.empty()) return;

			std::string cdbc = v.second.get("cdbc", "");
			cgcCDBCInfo::pointer cdbcInfo = getCDBCInfo(cdbc);
			if (cdbcInfo.get() == NULL) return;

			cgcDataSourceInfo::pointer dsInfo = CGC_DSINFO(name, cdbcservice, cdbcInfo);
			m_dsInfos.insert(name, dsInfo);
		}
	}
};

#endif // __XmlParseCdbcs_h__
