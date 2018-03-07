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

// XmlParseDSs.h file here
#ifndef __XmlParseDSs_h__
#define __XmlParseDSs_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

namespace mycp {

class CDSInfo
{
public:
	typedef boost::shared_ptr<CDSInfo> pointer;

	const tstring & getDSId(void) const {return m_dsid;}
	const tstring & getDSCdbc(void) const {return m_dscdbc;}

	CDSInfo(const tstring& dsid, const tstring& dscdbc)
		: m_dsid(dsid), m_dscdbc(dscdbc)
	{}
private:
	tstring m_dsid;
	tstring m_dscdbc;
};

#define DATASOURCEINFO(id, cdbc) CDSInfo::pointer(new CDSInfo(id, cdbc))

class XmlParseDSs
{
public:
	typedef boost::shared_ptr<XmlParseDSs> pointer;

	XmlParseDSs(void)
	{}
	~XmlParseDSs(void)
	{
		clear();
	}

public:
	void clear(void) {m_dss.clear();}
	CDSInfo::pointer getDSInfo(const tstring& dsid) const
	{
		CDSInfo::pointer result;
		m_dss.find(dsid, result);
		return result;
	}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("datasources"))
				Insert(v);

		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first == "datasource")
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string dsid = v.second.get("ds-id", "");
			std::string dscdbc = v.second.get("ds-cdbc", "");
			if (dsid.empty() || dscdbc.empty()) return;

			CDSInfo::pointer dsInfo = DATASOURCEINFO(dsid, dscdbc);
			m_dss.insert(dsid, dsInfo);
		}
	}

	CLockMap<tstring, CDSInfo::pointer> m_dss;

};
#define XMLPARSEDSS XmlParseDSs::pointer(new XmlParseDSs())

} // namespace mycp

#endif // __XmlParseDSs_h__
