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

// XmlParseModules.h file here
#ifndef __XmlParseModules_h__
#define __XmlParseModules_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

class XmlParseModules
{
public:
	XmlParseModules(void)
		: m_bSupportDebug(true)
	{}
	virtual ~XmlParseModules(void)
	{
		FreeHandle();
	}

	std::vector<ModuleItem::pointer> m_modules;
	//CLockMap<tstring, ModuleItem::pointer,DisableCompare<tstring> > m_modules;
	//ModulesMap m_modules;

public:
//	ModulesMap & getModulesMap(void){return this->m_modules;}
	bool isAllowModule(const tstring & moduleName) const
	{
		return getModuleItem(moduleName).get() != NULL;
	}
	bool getAllowMethod(const tstring & moduleName, const tstring & invokeName, tstring & methodName)
	{
		for (size_t i=0;i<m_modules.size();i++)
		{
			ModuleItem::pointer moduleItem = m_modules[i];
			if (moduleItem->getName()==moduleName)
			{
				return moduleItem->getAllowMethod(invokeName, methodName);
			}
		}
		return false;
		//ModuleItem::pointer result;
		//if (!m_modules.find(moduleName, result))
		//	return false;
		//return result->getAllowMethod(invokeName, methodName);
	}

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
	//ModuleItem::pointer swapModuleItemByName(const ModuleItem::pointer& pNewModuleItem)
	//{
	//	for (size_t i=0;i<m_modules.size();i++)
	//	{
	//		ModuleItem::pointer moduleItem = m_modules[i];
	//		if (moduleItem->getName()==pNewModuleItem->getName())
	//		{
	//			ModuleItem::pointer result = moduleItem;
	//			// ???
	//			return result;
	//		}
	//	}
	//	m_modules.push_back(pNewModuleItem);
	//	ModuleItem::pointer NullResult;
	//	return NullResult;
	//}
	//bool isModuleItem(const ModuleItem::pointer & pModuleItem) const;
	bool isSupportDebug(void) const {return m_bSupportDebug;}
	ModuleItem::pointer getSotpClientServiceModuleItem(void) const {return m_pSotpClientServiceModuleItem;}

	void FreeHandle(void)
	{
		m_modules.clear();
		m_pSotpClientServiceModuleItem.reset();
	}

	void load(const tstring & filename)
	{
		ptree pt;
		read_xml(filename, pt);

		m_bSupportDebug = pt.get("supportdebug", 1) == 1;

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root.log"))
				Insert(v, MODULE_LOG);


			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root.parser"))
				Insert(v, MODULE_PARSER);

			if (pt.get("root.app.<xmlattr>.disable",0) == 0)
			{
				BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root.app"))
					Insert(v, MODULE_APP);
			}else
			{
				printf("**** disable all app modules!!! ****\n");
			}

			if (pt.get("root.server.<xmlattr>.disable",0) == 0)
			{
				BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root.server"))
					Insert(v, MODULE_SERVER);
			}else
			{
				printf("**** disable all server modules!!! ****\n");
			}

			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root.communication"))
				Insert(v, MODULE_COMM);

		}catch(...)
		{}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v, MODULETYPE modultType)
	{
		if (v.first.compare("module") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string file = v.second.get("file", "");
			if (file.empty()) return;

			ModuleItem::pointer moduleItem = ModuleItem::create();
			std::string name = v.second.get("name", "");
			if (name.empty())
				name = file;
#ifdef WIN32
#ifdef _DEBUG
			if (m_bSupportDebug)
				file.append("d");
#endif // _DEBUG
			file.append(".dll");
#else
			file.append(".so");
			std::string filetemp("lib");
			filetemp.append(file);
			file = filetemp;
#endif
			std::string param = v.second.get("param", "");
			const int protocol = v.second.get("protocol", (int)MODULE_PROTOCOL_SOTP);

			if (modultType == MODULE_COMM)
			{
				int commport = v.second.get("commport", 0);
				moduleItem->setCommPort(commport);
			}

			// APP
			int allowall = v.second.get("allowall", 0);
			int authaccount = v.second.get("authaccount", 0);
			std::string lockstate = v.second.get("lockstate", "");

			moduleItem->setName(name);
			moduleItem->setModule(file);
			moduleItem->setParam(param);
			moduleItem->setType(modultType);
			moduleItem->setProtocol(protocol);
			moduleItem->setDisable(disable == 1);

			// APP
			moduleItem->setAllowAll(allowall == 1);
			moduleItem->setAuthAccount(authaccount == 1);
			moduleItem->setLockState(ModuleItem::getLockState(lockstate));

			if (protocol==MODULE_PROTOCOL_SOTP_CLIENT_SERVICE && !moduleItem->getDisable())
			{
				m_pSotpClientServiceModuleItem = moduleItem;
			}
			m_modules.push_back(moduleItem);
			//this->m_modules.insert(moduleItem->getName(), moduleItem);
		}
	}

	private:
		bool m_bSupportDebug;
		ModuleItem::pointer m_pSotpClientServiceModuleItem;
};

#endif // __XmlParseModules_h__
