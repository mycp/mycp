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
#ifdef WIN32
#pragma warning(disable:4819)
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
//#include <boost/foreach.hpp>
using boost::property_tree::ptree;

class XmlParseLogSetting
{
public:
	XmlParseLogSetting(void)
		: m_nLogLevel(20)
		, m_bLogToStderr(true), m_bLogToFile(true), m_bLogToSystem(false), m_bLogToCallback(false), m_bLogToLogger(false)
		, m_sLogPath(_T("")), m_sLogFile(_T("")), m_nMaxSize(1), m_nBackupNumber(1), m_sLocale(_T(""))
		, m_bDateFormat(true), m_bTimeFormat(true), m_bOneLineFormat(true), m_bLevelString(true)
	{}
	virtual ~XmlParseLogSetting(void)
	{
	}

public:
	int getLogLevel(void) const {return m_nLogLevel;}
	
	bool isLogToStderr(void) const {return m_bLogToStderr;}
	bool isLogToFile(void) const {return m_bLogToFile;}
	bool isLogToSystem(void) const {return m_bLogToSystem;}
	bool isLogToCallback(void) const {return m_bLogToCallback;}
	bool isLogToLogger(void) const {return m_bLogToLogger;}

	const tstring & getLogPath(void) const {return m_sLogPath;}
	void setLogPath(const tstring & newValue) {m_sLogPath = newValue;}
	const tstring & getLogFile(void) const {return m_sLogFile;}
	void setLogFile(const tstring & newValue) {m_sLogFile = newValue;}
	int getMaxSize(void) const {return m_nMaxSize;}	// MB
	int getBackupNumber(void) const {return m_nBackupNumber;}
	const tstring & getLocale(void) const {return m_sLocale;}	// locale

	bool isDateFormat(void) const {return m_bDateFormat;}		// date format
	bool isTimeFormat(void) const {return m_bTimeFormat;}		// time format
	bool isOneLineFormat(void) const {return m_bOneLineFormat;}		// one line format
	bool isAddLevelString(void) const {return m_bLevelString;}
	//void setLts(bool newValue) {m_bLogToSystem = newValue;}

	void load(const tstring & filename)
	{
		try
		{
			ptree pt;
			read_xml(filename.c_str(), pt);

			m_nLogLevel = pt.get("root.log.loglevel", 20);

			m_bLogToStderr = pt.get("root.log.tostderr", 1) == 1;
			m_bLogToFile = pt.get("root.log.tofile", 1) == 1;
			m_bLogToSystem = pt.get("root.log.tosystem", 0) == 1;
			m_bLogToCallback = pt.get("root.log.tocallback", 0) == 1;
			m_bLogToLogger = pt.get("root.log.tologger", 0) == 1;

			m_sLogPath = pt.get("root.log.path", "");
			m_sLogFile = pt.get("root.log.file", "");
			m_nMaxSize = pt.get("root.log.maxsize", 1);
			m_nBackupNumber = pt.get("root.log.backupnumber", 1);
			m_sLocale = pt.get("root.log.locale", "");

			m_bDateFormat = pt.get("root.log.dateformat", 1) == 1;
			m_bTimeFormat = pt.get("root.log.timeformat", 1) == 1;
			m_bOneLineFormat = pt.get("root.log.onelineformat", 1) == 1;
			m_bLevelString = pt.get("root.log.levelstring", 1) == 1;
		}catch(std::exception const & e)
		{
			printf("**** load %s exception, %s\n", filename.c_str(), e.what());
		}catch(...)
		{
			printf("**** load %s exception\n", filename.c_str());
		}
	}

private:
	int m_nLogLevel;		// default 20

	bool m_bLogToStderr;	// default true
	bool m_bLogToFile;		// default true
	bool m_bLogToSystem;	// default false
	bool m_bLogToCallback;	// default false
	bool m_bLogToLogger;	// default false

	tstring m_sLogPath;
	tstring m_sLogFile;
	int m_nMaxSize;			// default 1
	int m_nBackupNumber;	// default 1
	tstring m_sLocale;		// default ""

	bool m_bDateFormat;		// default true
	bool m_bTimeFormat;		// default true
	bool m_bOneLineFormat;	// default true
	bool m_bLevelString;	// default true
};

#endif // __XmlParseParsms_h__
