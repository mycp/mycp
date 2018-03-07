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

// XmlParseDefault.h file here
#ifndef __XmlParseDefault_h__
#define __XmlParseDefault_h__
#pragma warning(disable:4819)

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

typedef unsigned short  u_short;

class XmlParseDefault
{
public:
	XmlParseDefault(void)
		: m_sCgcpName(_T(""))
		, m_sCgcpCode(_T(""))
		, m_sCgcpAddress(_T(""))
		, m_nCgcpRank(0)
		, m_nWaitSleep(3)
		, m_nRetryCount(0)
		, m_sDefaultEncoding(_T("GBK"))
		, m_bLogReceiveData(false)
		, m_nThreadStackSize(0)
	{}
	virtual ~XmlParseDefault(void)
	{}

public:
	void load(const tstring & filename)
	{
		// 建立空的property tree对象
		using boost::property_tree::ptree;
		ptree pt;

		// 把XML文件载入到property tree里. 如果出错    // (无法打开文件,解析错误), 抛出一个异常.
		read_xml(filename, pt);

		// 如果键里有小数点,我们可以在get方法中指定其它分隔符
		// 如果debug.filename没找到,抛出一个异常
		//m_sCgcpName = pt.get<std::string>("debug.filename");
		// "root.cgcp.*"
		m_sCgcpName = pt.get("root.cgcp.name", _T("CGCP"));
		m_sCgcpAddress = pt.get("root.cgcp.address", _T("127.0.0.1"));
		m_sCgcpCode = pt.get("root.cgcp.code", _T("cgcp0"));
		m_nCgcpRank = pt.get("root.cgcp.rank", (int)0);
		// root.time.*
		m_nWaitSleep = pt.get("root.time.waitsleep", 3);
		m_nRetryCount = pt.get("root.app.retry", (int)0);
		// root.encoding
		m_sDefaultEncoding = pt.get("root.encoding", _T("GBK"));
		m_bLogReceiveData = pt.get("root.log.receive", (int)0)==1?true:false;
		m_nThreadStackSize = pt.get("root.thread.stack-size", (int)0);

		// 遍历debug.modules段并把所有找到的模块保存到m_modules里 
		// get_child()返回指定路径的下级引用.如果没有下级就抛出异常.
		// Property tree迭代器属于BidirectionalIterator(双向迭代器)
		//BOOST_FOREACH(ptree::value_type &v,
		//pt.get_child("debug.modules"))
		//	m_modules.insert(v.second.data());
	}

	const tstring & getCgcpName(void) const {return m_sCgcpName;}
	const tstring & getCgcpAddr(void) const {return m_sCgcpAddress;}
	const tstring & getCgcpCode(void) const {return m_sCgcpCode;}
	int getCgcpRank(void) const {return m_nCgcpRank;}
	int getWaitSleep(void) const {return m_nWaitSleep;}
	int getRetryCount(void) const {return m_nRetryCount;}
	const tstring & getDefaultEncoding(void) const {return m_sDefaultEncoding;}
	bool getLogReceiveData(void) const {return m_bLogReceiveData;}
	int getThreadStackSize(void) const {return m_nThreadStackSize;}
private:
	tstring m_sCgcpName;	// 本地名称
	tstring m_sCgcpAddress;	// 本地地址
	tstring m_sCgcpCode;	// 本地代码
	int m_nCgcpRank;			// 本地级别
	int m_nWaitSleep;			// 等待多少秒才启动
	int m_nRetryCount;			// 重试多少次（APP类型有效）
	tstring m_sDefaultEncoding;
	bool m_bLogReceiveData;
	int m_nThreadStackSize;		// 线程大小，默认 0 不处理
};

class CPortApp
{
public:
	typedef boost::shared_ptr<CPortApp> pointer;

	void setPort(int v) {m_port = v;}
	int getPort(void) const {return m_port;}
	void setApp(const tstring & v) {m_app = v;}
	const tstring & getApp(void) const {return m_app;}
	void setFunc(const tstring & v) {m_func = v;}
	const tstring getFunc(void) const {return m_func;}	

	bool resetByOldModuleHandle(void* pOldHandle)
	{
		if (m_hModule==pOldHandle)
		{
			m_hModule = NULL;
			m_hFunc1 = NULL;
			m_hFunc2 = NULL;
			m_hFunc3 = NULL;
			return true;
		}
		return false;
	}
	void setModuleHandle(void * v) {m_hModule = v;}
	void * getModuleHandle(void) const {return m_hModule;}
	void setFuncHandle1(void * v) {m_hFunc1 = v;}
	void * getFuncHandle1(void) const {return m_hFunc1;}
	void setFuncHandle2(void * v) {m_hFunc2 = v;}
	void * getFuncHandle2(void) const {return m_hFunc2;}
	void setFuncHandle3(void * v) {m_hFunc3 = v;}
	void * getFuncHandle3(void) const {return m_hFunc3;}

	CPortApp(int port, const tstring& app)
		: m_port(port), m_app(app), m_func("")
		, m_hModule(NULL), m_hFunc1(NULL), m_hFunc2(NULL), m_hFunc3(NULL)
	{}
	//virtual ~CPortApp(void)
	//{
	//	if (m_hModule!=NULL)
	//	{
	//	}
	//}
private:
	int m_port;
	tstring m_app;
	tstring m_func;

	void * m_hModule;
	void * m_hFunc1;
	void * m_hFunc2;
	void * m_hFunc3;
};

#define PORTAPP_POINTER(p, f) CPortApp::pointer(new CPortApp(p, f))

class XmlParsePortApps
{
public:
	XmlParsePortApps(void)

	{}
	~XmlParsePortApps(void)
	{
		clear();
	}

	CPortApp::pointer getPortApp(int port)
	{
		CPortApp::pointer result;
		m_portApps.find(port, result);
		return result;
	}
	void resetByOldModuleHandle(void* pOldHandle)
	{
		BoostReadLock rdlock(m_portApps.mutex());
		CLockMap<int, CPortApp::pointer>::iterator pIter = m_portApps.begin();
		for (; pIter!=m_portApps.end(); pIter++)
		{
			pIter->second->resetByOldModuleHandle(pOldHandle);
		}
	}
	void clear(void) {m_portApps.clear();}
public:
	void load(const tstring & filename)
	{
		// 建立空的property tree对象
		using boost::property_tree::ptree;
		ptree pt;

		// 把XML文件载入到property tree里. 如果出错    // (无法打开文件,解析错误), 抛出一个异常.
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
		if (v.first.compare("port-app") == 0)
		{
			int disable = v.second.get("disable", 0);
			if (disable == 1) return;

			int port = v.second.get("port", 0);
			std::string app = v.second.get("app", "");
			if (port == 0 || app.empty()) return;

			std::string func = v.second.get("func", "");

			CPortApp::pointer portApp = PORTAPP_POINTER(port, app);
			portApp->setFunc(func);
			m_portApps.insert(port, portApp);
		}
	}

private:
	CLockMap<int, CPortApp::pointer> m_portApps;

};

#endif // __XmlParseDefault_h__
