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

// XmlParseAutoUpdate.h file here
#ifndef __XmlParseAutoUpdate_h__
#define __XmlParseAutoUpdate_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

class XmlParseAutoUpdate
{
public:
	XmlParseAutoUpdate(void)
		: m_nAutoUpdateType(0)
	{}
	virtual ~XmlParseAutoUpdate(void)
	{
		FreeHandle();
	}

	std::vector<ModuleItem::pointer> m_modules;
	//CLockMap<tstring, ModuleItem::pointer,DisableCompare<tstring> > m_modules;
	//ModulesMap m_modules;

public:
	ModuleItem::pointer getModuleItem(const tstring & appName) const
	{
		for (size_t i=0;i<m_modules.size();i++)
		{
			ModuleItem::pointer moduleItem = m_modules[i];
			if (moduleItem->getName()==appName)
			{
				return moduleItem;
			}
		}
		ModuleItem::pointer NullResult;
		return NullResult;
	}
	int getAutpUpdateType(void) const {return m_nAutoUpdateType;}
	void FreeHandle(void)
	{
		m_modules.clear();
	}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		m_nAutoUpdateType = pt.get("root.update_type", 0);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root.update_modules"))
				Insert(v);
		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("module") == 0)
		{
			const int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string file = v.second.get("file", "");
			if (file.empty()) return;

			ModuleItem::pointer moduleItem = ModuleItem::create();
#ifdef WIN32
#ifdef _DEBUG
			//if (m_bSupportDebug)
				file.append("d");
#endif // _DEBUG
			file.append(".dll");
#else
			file.append(".so");
			std::string filetemp("lib");
			filetemp.append(file);
			file = filetemp;
#endif
			moduleItem->setModule(file);
			m_modules.push_back(moduleItem);
		}
	}

	private:
		int m_nAutoUpdateType;
};

#endif // __XmlParseAutoUpdate_h__
