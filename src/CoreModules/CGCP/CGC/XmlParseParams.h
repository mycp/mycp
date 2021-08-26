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

// XmlParseParsms.h file here
#ifndef __XmlParseParsms_h__
#define __XmlParseParsms_h__
#pragma warning(disable:4819)

#include "IncludeBase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
using boost::property_tree::ptree;

//class CSyncInfo
//{
//public:
//	typedef std::shared_ptr<CSyncInfo> pointer;
//	static CSyncInfo::pointer create(bool bEnableSync)
//	{
//		return CSyncInfo::pointer(new CSyncInfo(bEnableSync));
//	}
//	//void setSyncEnable(bool v) {m_bEnableSync = v;}
//	bool getSyncEnable(void) const {return m_bEnableSync;}
//	void setHosts(const std::string& v) {m_sHosts = v;}
//	const std::string& getHosts(void) const {return m_sHosts;}
//
//	CSyncInfo(bool bEnableSync)
//		: m_bEnableSync(bEnableSync)
//	{
//	}
//	CSyncInfo(void)
//		: m_bEnableSync(false)
//	{
//	}
//private:
//	bool m_bEnableSync;
//	std::string m_sHosts;
//};

typedef enum CGC_SYNC_RESULT_PROCESS_MODE
{
	CGC_SYNC_RESULT_PROCESS_ERROR_STOP
	, CGC_SYNC_RESULT_PROCESS_WARNING_STOP
	, CGC_SYNC_RESULT_PROCESS_BACKUP_SYNC_NEXT
	, CGC_SYNC_RESULT_PROCESS_DELETE_SYNC_NEXT
};

class XmlParseParams
{
public:
	XmlParseParams(void)
		: m_nMajorVersion(0)
		, m_nMinorVersion(0)
		, m_nMaxSize(1)
		, m_nBackFile(1)
		, m_nLogLevel(20)
		, m_bDtf(true)
		, m_bOlf(true)
		, m_bLts(false)
		, m_sLocale(_T(""))
		, m_bEnableSync(false), m_nSyncSocketType(1), m_bSerialize(false), m_nConnectRetry(60)
		, m_nErrorRetry(1), m_nSyncResultProcess(CGC_SYNC_RESULT_PROCESS_ERROR_STOP)
	{
		m_parameters = cgcParameterMap::pointer(new cgcParameterMap());
	}
	virtual ~XmlParseParams(void)
	{
		m_parameters.reset();
	}

public:
	cgcParameterMap::pointer getParameters(void) const {return m_parameters;}
	void clearParameters(void) {m_parameters->clear();}
	bool isDisableLogSign(int nSign) const {return m_pDisableLogSignList.exist(nSign,false);}
	bool isDisableLogApi(const tstring& sApi) const {return m_pDisableLogApiList.exist(sApi,false);}

	long getMajorVersion(void) const {return m_nMajorVersion;}
	long getMinorVersion(void) const {return m_nMinorVersion;}
	const tstring & getLogPath(void) const {return m_sLogPath;}
	void setLogPath(const tstring & newValue) {m_sLogPath = newValue;}
	const tstring & getLogFile(void) const {return m_sLogFile;}
	void setLogFile(const tstring & newValue) {m_sLogFile = newValue;}
	int getLogMaxSize(void) const {return m_nMaxSize;}	// µ¥Î» 'MB'
	int getLogBackFile(void) const {return m_nBackFile;}
	int getLogLevel(void) const {return m_nLogLevel;}
	bool isDtf(void) const {return m_bDtf;}		// datetime format
	bool isOlf(void) const {return m_bOlf;}		// one line format
	bool isLts(void) const {return m_bLts;}		// log to system
	void setLts(bool newValue) {m_bLts=newValue;}
	const tstring & getLogLocale(void) const {return m_sLocale;}	// locale
	//CSyncInfo::pointer getSyncInfo(void) const {return m_pSyncInfo;}

	bool isSyncEnable(void) const {return m_bEnableSync;}
	int getSyncSocketType(void) const {return m_nSyncSocketType;}
	const std::string& getSyncNames(void) const {return m_sSyncNames;}
	bool isSyncName(const std::string& sName) const {return (m_sSyncNames=="*" || m_sSyncNames.find(sName)!=std::string::npos)?true:false;}
	//bool isSyncName(const std::string& sName) const {return (sName.empty() || m_sSyncNames=="*" || m_sSyncNames.find(sName)!=std::string::npos)?true:false;}
	const std::string& getSyncHosts(void) const {return m_sSyncHosts;}
	const std::string& getSyncPassword(void) const {return m_sSyncPassword;}
	const std::string& getSyncCdbcService(void) const {return m_sCdbcService;}
	bool isSyncSerialize(void) const {return m_bSerialize;}
	int getConnectRetry(void) const {return m_nConnectRetry;}
	int getErrorRetry(void) const {return m_nErrorRetry;}
	CGC_SYNC_RESULT_PROCESS_MODE getSyncResultProcessMode(void) const {return m_nSyncResultProcess;}

	void load(const tstring & filename)
	{
		clearParameters();
		m_pDisableLogSignList.clear();

		try
		{
			ptree pt;
			read_xml(filename, pt);
			BOOST_FOREACH(const ptree::value_type &v, pt.get_child("root"))
				Insert(v);
		}catch(std::exception const & e)
		{
			printf("**** load %s exception, %s\n", filename.c_str(), e.what());
		}catch(...)
		{
			printf("**** load %s exception\n", filename.c_str());
		}
	}

private:
	void Insert(const boost::property_tree::ptree::value_type & v)
	{
		if (v.first.compare("parameter") == 0)
		{
			std::string name = v.second.get("name", "");
			std::string type = v.second.get("type", "");
			std::string value = v.second.get("value", "");

			cgcParameter::pointer parameter = CGC_PARAMETER(name, value);
			parameter->totype(cgcValueInfo::cgcGetValueType(type));
			parameter->setAttribute(cgcParameter::ATTRIBUTE_READONLY);
			m_parameters->insert(parameter->getName(), parameter);
		}else if (v.first.compare("log") == 0)
		{
			m_sLogPath = v.second.get("path", "");
			m_sLogFile = v.second.get("file", "");
			m_nMaxSize = v.second.get("maxsize", 1);
			m_nBackFile = v.second.get("backfile", 1);
			//m_nBackFile = v.second.get("logtoserver", 1);
			m_nLogLevel = v.second.get("loglevel", 20);
			m_bDtf = v.second.get("datetimeformat", 1) == 1;
			m_bOlf = v.second.get("onelineformat", 1) == 1;
			m_bLts = v.second.get("logtosystem", 0) == 1;
			m_sLocale = v.second.get("locale", "");

			const std::string sDisableSigns = v.second.get("disable_log_signs", "");
			ParseString(sDisableSigns.c_str(),",",m_pDisableLogSignList);
			const std::string sDisableApis = v.second.get("disable_log_apis", "");
			ParseString(sDisableApis.c_str(),",",m_pDisableLogApiList);
		}else if (v.first.compare("version") == 0)
		{
			m_nMajorVersion = v.second.get("Major", 0);
			m_nMinorVersion = v.second.get("Minor", 0);
		}else if (v.first.compare("sync") == 0)
		{
			m_bEnableSync = v.second.get("enable", 0)==1?true:false;
			if (m_bEnableSync)
			{
				m_sSyncHosts = v.second.get("hosts", "");
				m_nSyncSocketType = v.second.get("socket_type", 1);
				m_sSyncNames = v.second.get("names", "");
				m_sSyncPassword = v.second.get("password", "");
				m_sCdbcService = v.second.get("cdbc_service", "");
				m_nConnectRetry = v.second.get("connect_retry_time", 60);
				if (m_nConnectRetry<10)
					m_nConnectRetry = 10;
				m_nErrorRetry = v.second.get("error_retry", 1);
				m_nSyncResultProcess = (CGC_SYNC_RESULT_PROCESS_MODE)v.second.get("result_process_mode", (int)CGC_SYNC_RESULT_PROCESS_ERROR_STOP);
				m_bSerialize = v.second.get("serialize", 0)==1?true:false;
				
				//if (m_pSyncInfo.get()==NULL)
				//	m_pSyncInfo = CSyncInfo::create(m_bEnableSync);
				//m_pSyncInfo->setHosts(sHost);
			}
		}
	}
	static void ParseString(const char * lpszString, const char * lpszInterval, CLockMap<int,bool> & pOut)
	{
		if (lpszString==NULL || strlen(lpszString)==0) return;
		const tstring sIn(lpszString);
		const size_t nInLen = sIn.size();
		const size_t nIntervalLen = strlen(lpszInterval);
		pOut.clear();
		std::string::size_type nFindBegin = 0;
		while (nFindBegin<nInLen)
		{
			std::string::size_type find = sIn.find(lpszInterval,nFindBegin);
			if (find == std::string::npos)
			{
				pOut.insert(atoi(sIn.substr(nFindBegin).c_str()),true,false);
				break;
			}
			if (find==nFindBegin)
			{
				//pOut.push_back("");	// ¿Õ
			}else
			{
				pOut.insert(atoi(sIn.substr(nFindBegin, (find-nFindBegin)).c_str()),true,false);
			}
			nFindBegin = find+nIntervalLen;
		}
	}
	static void ParseString(const char * lpszString, const char * lpszInterval, CLockMap<tstring,bool> & pOut)
	{
		if (lpszString==NULL || strlen(lpszString)==0) return;
		const tstring sIn(lpszString);
		const size_t nInLen = sIn.size();
		const size_t nIntervalLen = strlen(lpszInterval);
		pOut.clear();
		std::string::size_type nFindBegin = 0;
		while (nFindBegin<nInLen)
		{
			std::string::size_type find = sIn.find(lpszInterval,nFindBegin);
			if (find == std::string::npos)
			{
				pOut.insert(sIn.substr(nFindBegin),true,false);
				break;
			}
			if (find==nFindBegin)
			{
				//pOut.push_back("");	// ¿Õ
			}else
			{
				pOut.insert(sIn.substr(nFindBegin, (find-nFindBegin)),true,false);
			}
			nFindBegin = find+nIntervalLen;
		}
	}
private:
	long m_nMajorVersion;
	long m_nMinorVersion;
	tstring m_sLogPath;
	tstring m_sLogFile;
	int m_nMaxSize;		// default 1
	int m_nBackFile;	// default 1
	int m_nLogLevel;	// default 20
	bool m_bDtf;		// default true
	bool m_bOlf;		// default true
	bool m_bLts;		// default false
	tstring m_sLocale;	// default "chs"

	cgcParameterMap::pointer m_parameters;
	CLockMap<int,bool> m_pDisableLogSignList;	// for sign
	CLockMap<tstring,bool> m_pDisableLogApiList;	// for api name
	// for sync
	//CSyncInfo::pointer m_pSyncInfo;
	bool m_bEnableSync;
	int m_nSyncSocketType;	// 1=tcp; 2=udp
	std::string m_sSyncNames;
	std::string m_sSyncHosts;
	std::string m_sSyncPassword;
	std::string m_sCdbcService;
	bool m_bSerialize;
	int m_nConnectRetry;
	int m_nErrorRetry;		// <!-- [>=0] 0=disable retry; default 1 -->
	CGC_SYNC_RESULT_PROCESS_MODE m_nSyncResultProcess;
};

#endif // __XmlParseParsms_h__
