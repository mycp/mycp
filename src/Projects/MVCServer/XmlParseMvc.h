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

// XmlParseMvc.h file here
#ifndef __XmlParseMvc_h__
#define __XmlParseMvc_h__
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include "XmlParseStructs.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

class CMvcInfo
{
public:
	typedef std::shared_ptr<CMvcInfo> pointer;
	const tstring & getServletName(void) const {return m_sServletName;}
	const tstring & getFullName(void) const {return m_sFullName;}
	const tstring & getMvcFile(void) const {return m_sMvcFile;}

	XmlParseStructs::pointer m_parseStructs;

	CMvcInfo(const tstring & name, const tstring & fullName, const tstring & mvcFile)
		: m_sServletName(name), m_sFullName(fullName), m_sMvcFile(mvcFile)
	{
	}
private:
	tstring m_sServletName;	// mvctest
	tstring m_sFullName;	// /strvlet/mvctest
	tstring m_sMvcFile;		// mvctest-mvc.xml

};
const CMvcInfo::pointer cgcNullMvcInfo;


class XmlParseMvc
{
public:
	XmlParseMvc(void)
	{}
	~XmlParseMvc(void)
	{
		clear();
	}

public:
	void clear(void) {m_mvcs.clear();}
	CMvcInfo::pointer getMvcInfo(const tstring& name) const
	{
		CMvcInfo::pointer result;
		return m_mvcs.find(name, result) ? result : cgcNullMvcInfo;
	}

	void load(const tstring & filePath, const tstring & filename)
	{
		m_filePath = filePath;
		ptree pt;
		read_xml(filePath+filename, pt);

		try
		{
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("mvc"))
				InsertMvcInfo(v);
		}catch(...)
		{}
	}

private:
	void InsertMvcInfo(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("enable-mvc") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			std::string name = v.second.get("mvc-servlet", "");
			if (name.empty()) return;

			std::string fullName("/servlet/");
			fullName.append(name);

			std::string mvcFile(name);
			mvcFile.append("-mvc.xml");

			try
			{
				CMvcInfo::pointer mvcInfo = CMvcInfo::pointer(new CMvcInfo(name, fullName, mvcFile));
				mvcInfo->m_parseStructs = XmlParseStructs::pointer(new XmlParseStructs());
				mvcInfo->m_parseStructs->setServletName(name);
				mvcInfo->m_parseStructs->load(m_filePath+mvcFile);
				m_mvcs.insert(name, mvcInfo);
			}catch(...)
			{}
		}
	}

	tstring m_filePath;
	CLockMap<tstring, CMvcInfo::pointer> m_mvcs;	// fullname ->

};

#endif // __XmlParseMvc_h__
