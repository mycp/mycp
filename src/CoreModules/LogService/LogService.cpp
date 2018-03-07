/*
    MYCP is a HTTP and C++ Web Application Server.
	CommRtpServer is a RTP socket server.
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
// LogService.cpp : Defines the exported functions for the DLL application.
#ifdef WIN32
#pragma warning(disable:4819 4267 4996)
#include <windows.h>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif // WIN32

#include <iostream>
#include <stdarg.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>

#ifdef _UNICODE
typedef boost::wformat				tformat;
typedef boost::filesystem::wpath	tpath;
#else
typedef boost::format				tformat;
typedef boost::filesystem::path		tpath;
#endif // _UNICODE

// cgc head
#include <ThirdParty/stl/locklist.h>
#include <CGCBase/app.h>
#include <CGCBase/cgcLogService.h>

using namespace mycp;
#include "XmlParseLogSetting.h"
#include "LogInfo.h"
cgcAttributes::pointer theAppAttributes;

class CLogService
	: public cgcLogService
	, public cgcOnTimerHandler
{
public:
	typedef boost::shared_ptr<CLogService> pointer;
	static CLogService::pointer create(const tstring& name, const tstring& xmlfile)
	{
		return CLogService::pointer(new CLogService(name, xmlfile));
	}

	void initlog(void)
	{
		namespace fs = boost::filesystem;
		fs::path pathXmlFile(m_xmlfile.c_str());
		boost::system::error_code ec;
		if (boost::filesystem::exists(pathXmlFile,ec))
			m_setting.load(m_xmlfile);

		if (m_setting.getLogFile().empty())
		{
			tstring logFile(m_name);
			logFile.append(".log");
			m_setting.setLogFile(logFile);
		}

		if (m_setting.getLogPath().empty())
		{
			// Create log directory.
			tstring sLogRoot(theSystem->getServerPath());
			sLogRoot.append("/log");
			fs::path pathLogRoot(sLogRoot.c_str());
			if (!boost::filesystem::exists(pathLogRoot,ec))
				boost::filesystem::create_directory(pathLogRoot,ec);

			char logPath[256];
			memset(logPath, 0, 256);
			sprintf(logPath, "%s/%s", sLogRoot.c_str(), m_name.c_str());
			m_setting.setLogPath(logPath);
		}
	}

	bool DoLog(void)
	{
		CLogInfo::pointer logInfo;
		if (!m_logs.front(logInfo))
			return false;

		if (this->m_setting.isOneLineFormat())
		{
			string_replace(logInfo->m_msg, _T("\n"), _T(" "));
			logInfo->m_msg.append(_T("\n"));
		}
		if (this->m_setting.isAddLevelString())
		{
			logInfo->m_msg.insert(0, getLevelString(logInfo->m_level));
		}

		if (this->m_setting.isLogToStderr())
		{
			std::cerr << m_name.c_str() << _T(": ") << logInfo->m_msg.c_str();// << std::endl;
		}

		if (!m_stream.is_open()) return false;

		// Add date information.
		if (this->m_setting.isDateFormat())
		{
			try{
				tformat formatDatetime(_T("%d-%02d-%02d "));
				struct tm * ptm = localtime(&logInfo->m_logtime.time);
				const tstring sTemp((formatDatetime%(1900+ptm->tm_year)%(ptm->tm_mon+1)%ptm->tm_mday).str());
				m_stream.write(sTemp.c_str(), sTemp.size());
			}catch (const std::exception &)
			{
			}catch (...)
			{
			}
		}
		// Add time information.
		if (this->m_setting.isTimeFormat())
		{
			try{
				tformat formatDatetime(_T("%02d:%02d:%02d.%03d "));
				struct tm * ptm = localtime(&logInfo->m_logtime.time);
				const std::string sTemp((formatDatetime%ptm->tm_hour%ptm->tm_min%ptm->tm_sec%logInfo->m_logtime.millitm).str());
				m_stream.write(sTemp.c_str(), sTemp.size());
			}catch (const std::exception &)
			{
			}catch (...)
			{
			}
		}
		//// Add time information.
		//if (this->m_setting.isTimeFormat())
		//{
		//	try{
		//		tformat formatDatetime(_T("%02d:%02d:%02d.%03d "));
		//		struct tm * ptm = localtime(&logInfo->m_logtime.time);
		//		const std::string sTemp((formatDatetime%ptm->tm_hour%ptm->tm_min%ptm->tm_sec%logInfo->m_logtime.millitm).str());
		//		logInfo->m_msg.insert(0, sTemp);
		//	}catch (const std::exception &)
		//	{
		//	}catch (...)
		//	{
		//	}
		//}
		//// Add date information.
		//if (this->m_setting.isDateFormat())
		//{
		//	try{
		//		tformat formatDatetime(_T("%d-%02d-%02d "));
		//		struct tm * ptm = localtime(&logInfo->m_logtime.time);
		//		const tstring sTemp((formatDatetime%(1900+ptm->tm_year)%(ptm->tm_mon+1)%ptm->tm_mday).str());
		//		logInfo->m_msg.insert(0, sTemp);
		//	}catch (const std::exception &)
		//	{
		//	}catch (...)
		//	{
		//	}
		//}
		m_stream.write(logInfo->m_msg.c_str(), logInfo->m_msg.size());
		m_stream.flush();

		if (((m_nIndex++)%3)!=2) return true;
		const tfstream::pos_type fsize = m_stream.tellg();
		// Backup log file.
		if (fsize >= this->m_setting.getMaxSize()*1024*1024)
		{
			// Close file first.
			m_stream.close();
			std::string src_file = m_setting.getLogPath();
			src_file.append("/");
			src_file.append(m_setting.getLogFile().c_str());

			if (m_setting.getBackupNumber() <= 0)
			{
				// Delete File.
				tpath src_path(src_file);
				boost::filesystem::remove(src_path);
			}else
			{
				if (++m_currentBackup > m_setting.getBackupNumber())
				{
					m_currentBackup = 1;
				}

				char bufferBackupFile[256];
				memset(bufferBackupFile, 0, 256);
				sprintf(bufferBackupFile, "%s%d", src_file.c_str(), m_currentBackup-1);

				// Backup file.
				tpath src_path(src_file);
				tpath des_path(bufferBackupFile);

				boost::filesystem::remove(des_path);
				boost::filesystem::rename(src_path, des_path);
			}

			// Reopen file.
			m_stream.open(src_file.c_str(), std::ios::out);
			if (!m_stream.is_open()) return false;	// Open failed.
		}

		return true;
	}

	virtual tstring serviceName(void) const {return _T("LOGSERVICE");}
	//virtual bool initService(cgcValueInfo::pointer parameter = cgcNullValueInfo)
	//{
	//	if (this->isServiceInited()) return true;
	//	return cgcServiceInterface::initService();
	//}
	virtual void finalService(void)
	{
		if (!isServiceInited()) return;
		cgcServiceInterface::finalService();

		boost::shared_ptr<boost::thread> threadLogTemp = m_threadLog;
		m_threadLog.reset();
		if (threadLogTemp.get()!=NULL)
		{
			if (threadLogTemp->joinable())
				threadLogTemp->join();
			threadLogTemp.reset();
		}
		m_stream.close();
	}

protected:
	void do_logservice(void)
	//static void do_logservice(CLogService * pLogService)
	{
		//if (pLogService == NULL) return;
		while (true)
		{
			try
			{
				if (!isServiceInited())
				{
					while (DoLog())
						;
					break;
				}

				if (!DoLog())
				{
#ifdef WIN32
					Sleep(10);
#else
					usleep(10000);
#endif
				}
			}catch(std::exception const &)
			{
			}catch(...){
			}
		}
	}

	void string_replace(tstring & strBig, const tstring & strsrc, const tstring &strdst) const
	{
		std::string::size_type pos=0;
		std::string::size_type srclen=strsrc.size();
		std::string::size_type dstlen=strdst.size();

		while ((pos=strBig.find(strsrc, pos)) != std::string::npos)
		{
			strBig.replace(pos, srclen, strdst);
			pos += dstlen;
		}
	}

	bool beginLog(LogLevel level)
	{
		// Not the log level.
		if ((int)(m_setting.getLogLevel() & level) != (int)level) return false;

		const bool bFirstLog = m_threadLog.get() == NULL?true:false;
		if (m_threadLog.get() == NULL)
		{
			boost::thread_attributes attrs;
			attrs.set_stack_size(CGC_THREAD_STACK_MIN);
			m_threadLog = boost::shared_ptr<boost::thread>(new boost::thread(attrs,boost::bind(&CLogService::do_logservice, this)));
		}

		if (bFirstLog && !m_stream.is_open())
		{
			//
			tstring sLogPath(m_setting.getLogPath());
			tstring sLogFile(m_setting.getLogFile());

			// Create log directory.
			namespace fs = boost::filesystem;
			fs::path logPath(sLogPath.c_str());
			boost::system::error_code ec;
			if (!boost::filesystem::exists(logPath,ec))
				boost::filesystem::create_directory(logPath,ec);

			sLogPath.append("/");
			sLogPath.append(sLogFile);
			m_stream.open(sLogPath.c_str(), std::ios::out|std::ios::app|std::ios::ate);
			if (!m_stream.is_open()) return false;	// Open failed.

			// Set locale to support Chinese.
			try
			{
				std::locale loc(m_setting.getLocale().c_str());
				m_stream.imbue(loc);
			}catch (const std::exception & e)
			{
				printf("**** log stream.imbue(std::locale) %s exception, %s\n", m_setting.getLocale().c_str(), e.what());
			}catch(...)
			{
				printf("**** log stream.imbue(std::locale) %s exception\n", m_setting.getLocale().c_str());
			}
		}

		return true;
	}

	void afterLog(LogLevel level, const char * msg)
	{
		CLogInfo::pointer logInfo = CLogInfo::create(level, msg);
		//if (this->m_setting.isOneLineFormat())
		//{
		//	string_replace(logInfo->m_msg, _T("\n"), _T(" "));
		//	logInfo->m_msg.append(_T("\n"));
		//}
		//if (this->m_setting.isAddLevelString())
		//{
		//	logInfo->m_msg.insert(0, getLevelString(level));
		//}

		if (this->m_setting.isLogToStderr())
		{
			m_logs.add(logInfo);
			return;	// **
			//std::cerr << m_name.c_str() << _T(": ") << logInfo->m_msg.c_str();// << std::endl;
		}

		if (this->m_setting.isLogToFile())
		{
			m_logs.add(logInfo);
			return;
		}

		// ???
		//if (this->m_setting.isLogToSystem()) {}
		//if (this->m_setting.isLogToCallback()) {}
		//if (this->m_setting.isLogToLogger()) {}
	}

	virtual bool callService(int function, const cgcValueInfo::pointer& inParam, cgcValueInfo::pointer outParam)
	{
		if (inParam.get() == NULL) return false;

		LogLevel level = (LogLevel)function;
		tstring logmsg = inParam->toString();
		log(level, logmsg.c_str());
		return true;
	}
	virtual bool callService(const tstring& function, const cgcValueInfo::pointer& inParam, cgcValueInfo::pointer outParam)
	{
		if (inParam.get() == NULL) return false;

		tstring logmsg = inParam->toString();
		if (function == "debug")
		{
			log(LOG_DEBUG, logmsg.c_str());
		}else
		{
			log(LOG_INFO, logmsg.c_str());
		}

		return true;
	}
	virtual cgcAttributes::pointer getAttributes(void) const {return theAppAttributes;}
	/*
	virtual cgcValueInfo::pointer getProperty(const tstring& propertyName) const{
		return theAppAttributes.get() == NULL ? cgcNullValueInfo : theAppAttributes->getProperty(propertyName);
	}
	virtual bool getProperty(const tstring& propertyName, std::vector<cgcValueInfo::pointer>& outProperty) const {
		return theAppAttributes.get() == NULL ? false : theAppAttributes->getProperty(propertyName, outProperty);
	}
	virtual bool setProperty(const tstring& propertyName, const cgcValueInfo::pointer& inProperty) {
		if (inProperty.get() == NULL || theAppAttributes.get() == NULL) return false;
		theAppAttributes->delProperty(propertyName);
		theAppAttributes->setProperty(propertyName, inProperty->copy());
		return true;
	}
	virtual bool addProperty(const tstring& propertyName, const cgcValueInfo::pointer& inProperty) {
		if (inProperty.get() == NULL || theAppAttributes.get() == NULL) return false;
		theAppAttributes->setProperty(propertyName, inProperty->copy());
		return true;
	}*/

	virtual void log(LogLevel level, const char * format,...)
	{
		if (!beginLog(level)) return;

		try
		{
			char debugmsg[MAX_LOG_SIZE];
			memset(debugmsg, 0, MAX_LOG_SIZE);
			va_list   vl;
			va_start(vl, format);
			int len = vsnprintf(debugmsg, MAX_LOG_SIZE-1, format, vl);
			va_end(vl);
			if (len<0) {
				return;
			}
			if (len > MAX_LOG_SIZE)
				len = MAX_LOG_SIZE;
			debugmsg[len] = '\0';
			afterLog(level, debugmsg);
		}catch(std::exception const &)
		{
		}catch(...)
		{
		}
	}
	virtual void log(LogLevel level, const wchar_t * format,...)
	{
		if (!beginLog(level)) return;

		wchar_t debugmsg[MAX_LOG_SIZE];
		memset(debugmsg, 0, MAX_LOG_SIZE);
		try
		{
			va_list   vl;
			va_start(vl, format);
			int len = vswprintf(debugmsg, MAX_LOG_SIZE-1, format, vl);
			va_end(vl);
			if (len > MAX_LOG_SIZE)
				len = MAX_LOG_SIZE;
			debugmsg[len] = L'\0';
		}catch(std::exception const &)
		{
			return;
		}catch(...)
		{
			return;
		}
		const std::string sMsg(W2Char(debugmsg));
		afterLog(level, sMsg.c_str());
	}
	virtual void log2(LogLevel level, const char * format)
	{
		if (!beginLog(level)) return;
		afterLog(level, format);
	}
	virtual void log2(LogLevel level, const wchar_t * format)
	{
		if (!beginLog(level)) return;
		const std::string sMsg(W2Char(format));
		afterLog(level, sMsg.c_str());
	}
	virtual bool isLogLevel(LogLevel level) const
	{
		return ((int)(m_setting.getLogLevel() & level) != (int)level)?false:true;
	}

	virtual std::string W2Char(const wchar_t* strSource) const
	{
		std::string result = "";
		size_t targetLen = wcsrtombs(NULL, &strSource, 0, NULL);
		if (targetLen > 0)
		{
			char * pTargetData = new char[targetLen];
			memset(pTargetData, 0, targetLen);
			//memset(pTargetData, 0, (targetLen)*sizeof(char));
			wcsrtombs(pTargetData, &strSource, targetLen, NULL);
			result = pTargetData;
			delete[] pTargetData;
		}
		return result;
	}

	// cgcOnTimerHandler
	virtual void OnTimeout(unsigned int nIDEvent, const void * pvParam)
	{
		initlog();
	}

	tstring getLevelString(LogLevel level)
	{
		tstring result;
		switch (level)
		{
		case LOG_TRACE:
			result = "[TRACE] ";
			break;
		case LOG_DEBUG:
			result = "[DEBUG] ";
			break;
		case LOG_INFO:
			result = "[INFO] ";
			break;
		case LOG_WARNING:
			result = "[WARNING] ";
			break;
		case LOG_ERROR:
			result = "[ERROR] ";
			break;
		case LOG_ALERT:
			result = "[ALERT] ";
			break;
		default:
			result = "[LOG] ";
			break;
		}
		return result;
	}
public:
	CLogService(const tstring& name, const tstring& xmlfile)
		: m_name(name), m_xmlfile(xmlfile), m_currentBackup(0)
		, m_nIndex(0)
	{
	}
	~CLogService(void)
	{
		finalService();
	}

private:
	tstring m_name;
	tstring m_xmlfile;
	boost::shared_ptr<boost::thread> m_threadLog;
	int m_currentBackup;
	XmlParseLogSetting m_setting;

	unsigned int m_nIndex;
	tfstream m_stream;
	CLockList<CLogInfo::pointer> m_logs;

};

const int ATTRIBUTE_NAME = 1;
unsigned int theCurrentIdEvent = 0;


extern "C" bool CGC_API CGC_Module_Init(void)
{
	theAppAttributes = theApplication->getAttributes(true);
	assert (theAppAttributes.get() != NULL);
	return true;
}

extern "C" void CGC_API CGC_Module_Free(void)
{
	theApplication->KillAllTimer();

	VoidObjectMapPointer mapLogServices = theAppAttributes->getVoidAttributes(ATTRIBUTE_NAME, false);
	if (mapLogServices.get() != NULL)
	{
		CObjectMap<void*>::iterator iter;
		for (iter=mapLogServices->begin(); iter!=mapLogServices->end(); iter++)
		{
			CLogService::pointer logService = CGC_OBJECT_CAST<CLogService>(iter->second);
			if (logService.get() != NULL)
			{
				logService->finalService();
			}
		}
	}

	theAppAttributes.reset();
}

extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	if (parameter.get() == NULL || parameter->getStr().empty()) return;

	char settingFile[256];
	memset(settingFile, 0, 256);
	sprintf(settingFile, "%s/%s.xml", theApplication->getAppConfPath().c_str(), parameter->getStr().c_str());

	CLogService::pointer logService = CLogService::create(parameter->getStr(), settingFile);
	logService->initlog();
	logService->initService();

	outService = logService;
	theApplication->SetTimer(++theCurrentIdEvent, 60*1000, logService);

	cgcAttributes::pointer attributes = theApplication->getAttributes();
	assert (attributes.get() != NULL);

	theAppAttributes->setAttribute(ATTRIBUTE_NAME, outService.get(), logService);
}

extern "C" void CGC_API CGC_ResetService(const cgcServiceInterface::pointer & inService)
{
	if (inService.get() == NULL) return;

	theAppAttributes->removeAttribute(ATTRIBUTE_NAME, inService.get());
	inService->finalService();
}
