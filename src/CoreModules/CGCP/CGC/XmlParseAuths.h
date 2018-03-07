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

// XmlParseAuths.h file here
#ifndef __XmlParseAuths_h__
#define __XmlParseAuths_h__

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

class XmlParseAuths
{
public:
	XmlParseAuths(void)
		: m_etAuth(ModuleItem::ET_NONE)
	{}
	virtual ~XmlParseAuths(void)
	{
		m_mapAuths.clear();
	}

public:
	bool emptyAuth(void) const {return m_mapAuths.empty();}
	bool authAccount(const tstring & account, const tstring & passwd) const{
		StringMapIter pIter = m_mapAuths.find(account);
		if (pIter != m_mapAuths.end())
		{
			return passwd.compare(pIter->second) == 0;
		}
		return false;
	}

public:
	StringMap m_mapAuths;	// account->passwd

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root"))
				Insert(v);

		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("auth") == 0)
		{
			std::string account = v.second.get("account", "");
			std::string password = v.second.get("password", "");
			m_mapAuths.insert(StringMapPair(account, password));
		}
	}

private:
	ModuleItem::EncryptionType m_etAuth;
};

#endif // __XmlParseAuths_h__
