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

// XmlParseAllowMethods.h file here
#ifndef __XmlParseAllowMethods_h__
#define __XmlParseAllowMethods_h__

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

class XmlParseAllowMethods
{
public:
	XmlParseAllowMethods(void)
	{}
	virtual ~XmlParseAllowMethods(void)
	{
		m_mapAllowMethods.clear();
	}

public:
	StringMap m_mapAllowMethods;

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
		if (v.first.compare("allow") == 0)
		{
			std::string name = v.second.get("name", "");
			std::string method = v.second.get("method", "");
			m_mapAllowMethods.insert(StringMapPair(name, method));
		}
	}
};

#endif // __XmlParseAllowMethods_h__
