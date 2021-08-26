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
#define USES_OPENSSL
#ifdef USES_OPENSSL
#ifdef WIN32
//#pragma comment(lib, "libeay32.lib")  
//#pragma comment(lib, "ssleay32.lib") 
#endif // WIN32
#endif // USES_OPENSSL
//#include <ThirdParty/Boost/asio/CgcTcpClient.h>
#include "CgcTcpClient.h"
#ifdef WIN32
#pragma warning(disable:4267 4996)
//#include <windows.h>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

// cgc head
#include <CGCBase/http.h>
#include <CGCBase/cgcApplication2.h>
#include <CGCBase/cgcServices.h>
#include <CGCBase/cgcCDBCService.h>
#include <CGCBase/cgcutils.h>
//#include <CGCClass/cgcclassinclude.h>
using namespace mycp;

#include "MycpHttpServer.h"
#include "XmlParseHosts.h"
#include "XmlParseApps.h"
#include "XmlParseDSs.h"
#include "md5.h"
//#include <ThirdParty/fastcgi/fastcgi.h>
#include "fastcgi.h"
//#include "cgcaddress.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b)) 
#endif // min

//#define USES_BODB_SCRIPT
#ifdef WIN32
#include "shellapi.h"
#pragma comment(lib, "shell32.lib")  
#include "tlhelp32.h"
#include "Psapi.h"
#pragma comment(lib, "Psapi.lib")
void KillAllProcess(const char* lpszProcessName)
{
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);  
	if (hProcessSnap==NULL) return;
	PROCESSENTRY32 pe32;  
	memset(&pe32,0,sizeof(pe32));
	pe32.dwSize=sizeof(PROCESSENTRY32);  
	if (::Process32First(hProcessSnap,&pe32))  
	{  
		do  
		{
			const std::string sExeFile(pe32.szExeFile);
			std::string::size_type find = sExeFile.find(lpszProcessName);
			if (find != std::string::npos)
			{
				HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
				if(NULL != hProcess)
				{
					TerminateProcess(hProcess, 0);
				}
			}
		}while(::Process32Next(hProcessSnap,&pe32));   
	}
	CloseHandle(hProcessSnap);

	HWND hHwnd = FindWindow(NULL,lpszProcessName);
	if (hHwnd!=NULL)
	{
		DWORD dwWndProcessId = 0;
		GetWindowThreadProcessId(hHwnd,&dwWndProcessId);
		if (dwWndProcessId>0)
		{
			HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, dwWndProcessId);
			if(NULL != hProcess)
			{
				TerminateProcess(hProcess, 0);
			}
		}
	}
}
#else
#include <semaphore.h>
#endif

#ifdef USES_BODB_SCRIPT
cgcCDBCService::pointer theCdbcService;
#endif
//std::string theFastcgiPHPServer;
CFastcgiInfo::pointer thePHPFastcgiInfo;
bool theEnableDataSource = false;
cgcServiceInterface::pointer theFileSystemService;
//cgcServiceInterface::pointer theStringService;
XmlParseHosts theVirtualHosts;
XmlParseApps::pointer theApps;
XmlParseDSs::pointer theDataSources;
CVirtualHost::pointer theDefaultHost;
CLockMap<tstring, CFileScripts::pointer> theFileScripts;
CLockMap<tstring, CCSPFileInfo::pointer> theFileInfos;
CLockMap<tstring, CCSPFileInfo::pointer> theIdleFileInfos;	// 记录空闲文件，超过7天无访问，删除数据库数据；
cgcAttributes::pointer theAppAttributes;
const int ATTRIBUTE_FILE_INFO = 8;				// filepath->
const unsigned int TIMERID_1_SECOND = 1;		// 1秒检查一次
cgcParameterMap::pointer theAppInitParameters;
//CLockMap<int,bool> theWaitDataList;

class CResInfo : public cgcObject
{
public:
	typedef std::shared_ptr<CResInfo> pointer;
	static CResInfo::pointer create(const std::string& sFileName, const std::string& sMimeType)
	{
		return CResInfo::pointer(new CResInfo(sFileName,sMimeType));
	}
	CResInfo(const std::string& sFileName, const std::string& sMimeType)
		: m_sFileName(sFileName),m_sMimeType(sMimeType)
		, m_tModifyTime(0),m_tRequestTime(0)
		, m_nSize(0),m_pData(NULL),m_nDataBufferSize(0),m_pFile(NULL)
	{
	}
	CResInfo(void)
		: m_tModifyTime(0),m_tRequestTime(0)
		, m_nSize(0),m_pData(NULL),m_nDataBufferSize(0), m_pFile(NULL)
	{
	}
	virtual ~CResInfo(void)
	{
		if (m_pData!=NULL)
		{
			delete[] m_pData;
			m_pData = NULL;
		}
		if (m_pFile!=NULL)
		{
			fclose(m_pFile);
			m_pFile = NULL;
		}
		//m_fs.close();
	}
	virtual cgcObject::pointer copyNew(void) const
	{
		CResInfo::pointer result = CResInfo::create(m_sFileName,m_sMimeType);
		result->m_tModifyTime = m_tModifyTime;
		result->m_sModifyTime = m_sModifyTime;
		result->m_sETag = m_sETag;
		result->m_tRequestTime = m_tRequestTime;
		result->m_nSize = m_nSize;
		// ???
		//result->m_pData = m_pData;
		//result->m_nDataBufferSize = m_nDataBufferSize;
		//result->m_pFile = m_pFile;
		return result;
	}

	boost::mutex m_mutex;
	std::string m_sFileName;
	std::string m_sMimeType;
	//std::fstream m_fs;
	time_t m_tModifyTime;
	std::string m_sModifyTime;
	std::string m_sETag;
	time_t m_tRequestTime;
	unsigned int m_nSize;
	boost::mutex	m_mutexFile;
	char * m_pData;
	size_t m_nDataBufferSize;
	FILE * m_pFile;
};

enum REQUEST_INFO_STATE
{
	REQUEST_INFO_STATE_WAITTING_RESPONSE
	, REQUEST_INFO_STATE_FCGI_DOING
	, REQUEST_INFO_STATE_FCGI_STDOUT
	, REQUEST_INFO_STATE_FCGI_STDERR
	, REQUEST_INFO_STATE_FCGI_END_REQUEST
	//, REQUEST_INFO_STATE_FCGI_DISCONNECTED
};
#define DEFAULT_SEND_BUFFER_SIZE (32*1024)

#define USES_FASTCGI_KEEP_CONN

class CRequestPassInfo
{
public:
	typedef std::shared_ptr<CRequestPassInfo> pointer;
	static CRequestPassInfo::pointer create(int nRequestId, const std::string& sFastcgiPass)
	{
		return CRequestPassInfo::pointer(new CRequestPassInfo(nRequestId, sFastcgiPass));
	}
	int GetRequestId(void) const {return m_nRequestId;}
	const std::string& GetFastcgiPass(void) const {return m_sFastcgiPass;}
	void SetProcessId(unsigned int v) {m_nProceessId = v;}
	unsigned int GetProcessId(void) const {return m_nProceessId;}
	void SetProcessHandle(void* v) {m_pProcessHandle = v;}
	void* GetProcessHandle(void) const {return m_pProcessHandle;}
	int GetRequestCount(void) const  {return m_nRequestCount;}
	int IncreaseRequestCount(time_t tNow) {m_tLastRequestTime=tNow;return ++m_nRequestCount;}
	time_t GetLastRequestTime(void) const {return m_tLastRequestTime;}
	//void ResetRequestCount(void) {m_nRequestCount=0;}
	int IncreaseErrorCount(void) {return ++m_nErrorCount;}
	void ResetErrorCount(void) {if (m_nErrorCount!=0) m_nErrorCount=0;}
	//time_t GetKillTime(void) const {return m_tKillTime;}
	//void ResetKillTime(void) {if (m_tKillTime>0) m_tKillTime = 0;}

#ifdef USES_FASTCGI_KEEP_CONN
	mycp::httpserver::CgcTcpClient::pointer m_pFastCgiServer;
#endif

	void KillProcess(void)
	{
		//printf("******** KillProcess PID=%d\n",(int)m_nProceessId);
#ifdef USES_FASTCGI_KEEP_CONN
		if (m_pFastCgiServer.get()!=0) {
			m_pFastCgiServer->stopClient();
			m_pFastCgiServer.reset();
		}
#endif
		if (m_nProceessId>0) {
			//if (bSetKillTime)
			//	m_tKillTime = time(0);
#ifdef WIN32
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, m_nProceessId);
			if (hProcess!=0) {
				TerminateProcess(hProcess, 0);
			}
			else if (m_pProcessHandle!=0) {
				TerminateProcess((HANDLE)m_pProcessHandle, 0);
			}
#else
			if (!m_sFastcgiPass.empty()) {
				/// ps -ef|grep '127.0.0.1:9000' | awk '{print $2}' | xargs kill
				char lpszBuffer[256];
				sprintf(lpszBuffer,"ps -ef|grep -v 'grep'|grep '%s' | awk '{print $2}' | xargs kill -9",m_sFastcgiPass.c_str());
				system(lpszBuffer);
			}
#endif
			m_nProceessId = 0;
			m_pProcessHandle = 0;
		}
		m_nRequestCount = 0;
		//printf("******** KillProcess end\n");
	}
//	void SetCloseEvent(void)
//	{
//#ifdef WIN32
//		SetEvent(m_hDoCloseEvent);
//#else
//		sem_post(&m_semDoClose);
//#endif // WIN32
//	}
//	void WaitEvent(int nSecond = 30)
//	{
//#ifdef WIN32
//		HANDLE hObject[2];
//		hObject[0] = m_hDoStopServer;
//		hObject[1] = m_hDoCloseEvent;
//		WaitForMultipleObjects(2, hObject, FALSE, nSecond*1000);
//#else
//		struct timespec timeSpec;
//		clock_gettime(CLOCK_REALTIME, &timeSpec);
//		timeSpec.tv_sec += nSecond;
//		sem_timedwait(&m_semDoClose, &timeSpec);
//#endif // WIN32
//	}

	CRequestPassInfo(int nRequestId, const std::string& sFastcgiPass)
		: m_nRequestId(nRequestId), m_sFastcgiPass(sFastcgiPass)
		, m_nProceessId(0), m_pProcessHandle(NULL)
		, m_nRequestCount(0)
		, m_nErrorCount(0)//, m_tKillTime(0)
	{
//#ifdef WIN32
//		m_hDoCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//		m_hDoStopServer = CreateEvent(NULL, FALSE, FALSE, NULL);
//#else
//		sem_init(&m_semDoClose, 0, 0);
//		sem_init(&m_semDoStop, 0, 0);
//#endif // WIN32
	}
	CRequestPassInfo(void)
		: m_nRequestId(0)
		, m_nProceessId(0), m_pProcessHandle(NULL)
		, m_nRequestCount(0)
		, m_nErrorCount(0)//, m_tKillTime(0)
	{
//#ifdef WIN32
//		m_hDoCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//		m_hDoStopServer = CreateEvent(NULL, FALSE, FALSE, NULL);
//#else
//		sem_init(&m_semDoClose, 0, 0);
//		sem_init(&m_semDoStop, 0, 0);
//#endif // WIN32
	}
	virtual ~CRequestPassInfo(void)
	{
//#ifdef WIN32
//		CloseHandle(m_hDoCloseEvent);
//		CloseHandle(m_hDoStopServer);
//#else
//		sem_destroy(&m_semDoClose);
//		sem_destroy(&m_semDoStop);
//#endif // WIN32
		KillProcess();
	}
private:
	int m_nRequestId;
	std::string m_sFastcgiPass;
	unsigned int m_nProceessId;
	void* m_pProcessHandle;
	int m_nRequestCount;
	time_t m_tLastRequestTime;
	int m_nErrorCount;
	//time_t m_tKillTime;
//#ifdef WIN32
//	HANDLE m_hDoCloseEvent;
//	HANDLE m_hDoStopServer;
//#else
//	sem_t m_semDoClose;
//	sem_t m_semDoStop;
//#endif // WIN32
};
enum SCRIPT_FILE_TYPE
{
	SCRIPT_FILE_TYPE_UNKNOWN
	, SCRIPT_FILE_TYPE_CSP
	, SCRIPT_FILE_TYPE_PHP
};

class CFastcgiRequestInfo
{
public:
	typedef std::shared_ptr<CFastcgiRequestInfo> pointer;
	static CFastcgiRequestInfo::pointer create(const CRequestPassInfo::pointer& pRequestPassInfo, const cgcHttpResponse::pointer& response)
	{
		return CFastcgiRequestInfo::pointer(new CFastcgiRequestInfo(pRequestPassInfo, response));
	}
	
#ifndef USES_FASTCGI_KEEP_CONN
	mycp::httpserver::CgcTcpClient::pointer m_pFastCgiServer;
#endif
	const CRequestPassInfo::pointer& GetRequestPassInfo(void) const {return m_pRequestPassInfo;}
	int GetRequestId(void) const {return m_pRequestPassInfo->GetRequestId();}
	const std::string& GetFastcgiPass(void) const {return m_pRequestPassInfo->GetFastcgiPass();}
	const cgcHttpResponse::pointer& response(void) const {return m_response;}
	time_t GetRequestTime(void) const {return m_tRequestTime;}
	void SetResponseState(REQUEST_INFO_STATE nResponseState, bool bResponseError=false) {m_nResponseState = nResponseState; m_bResponseError=bResponseError;}
	REQUEST_INFO_STATE GetResponseState(void) const {return m_nResponseState;}
	bool GetResponseError(void) const {return m_bResponseError;}
	void SetParsedHead(bool b) {m_bParsedHead = b;}
	bool GetParsedHead(void) const {return m_bParsedHead;}
	void SetResponsePaddingLength(int nPaddingLength) {m_nResponsePaddingLength = nPaddingLength;}
	int GetResponsePaddingLength(void) const {return m_nResponsePaddingLength;}
	//void SetLastPaddingData(const std::string& sData) {m_sLastPaddingData = sData;}
	//const std::string& GetLastPaddingData(void) const {return m_sLastPaddingData;}
	void SetLastData(const std::string& sData) {m_sLastData = sData;}
	//void AddLastData(const std::string& sData) {m_sLastData.append(sData);}
	void AddLastData(const char* sData, size_t nSize) {m_sLastData.append(sData, nSize);}
	const std::string& GetLastData(void) const {return m_sLastData;}
	void ClearLastData(void) {m_sLastData.clear();}
	void SetPrevPaddingLen(int v) {m_nPrevPaddingLen = v;}
	int GetPrevPaddingLen(void) const {return m_nPrevPaddingLen;}

	void SendData(const mycp::httpserver::CgcTcpClient::pointer& pFastCgiServer, const unsigned char* pData, size_t nSize, bool bForceSend=false)
	{
		if (m_bSendBufferSize>0 && (m_bSendBufferSize+nSize)>=DEFAULT_SEND_BUFFER_SIZE)
		{
			pFastCgiServer->sendData(m_lpszSendBuffer,m_bSendBufferSize);
			m_bSendBufferSize = 0;
		}
		if (nSize>=DEFAULT_SEND_BUFFER_SIZE)
		{
			pFastCgiServer->sendData(pData,nSize);
			return;
		}
		memcpy(m_lpszSendBuffer+m_bSendBufferSize,pData,nSize);
		m_bSendBufferSize += nSize;
		if (bForceSend)
		{
			pFastCgiServer->sendData(m_lpszSendBuffer,m_bSendBufferSize);
			m_bSendBufferSize = 0;
		}
	}

	unsigned char* GetBuffer(void) {return m_lpszBuffer;}

	CFastcgiRequestInfo(const CRequestPassInfo::pointer& pRequestPassInfo, const cgcHttpResponse::pointer& response)
		: m_pRequestPassInfo(pRequestPassInfo), m_response(response), m_nResponseState(REQUEST_INFO_STATE_WAITTING_RESPONSE), m_bResponseError(false)
		, m_nResponsePaddingLength(0)
		, m_bParsedHead(false)
	//CFastcgiRequestInfo(int nRequestId, const std::string& sFastcgiPass, const cgcHttpResponse::pointer& response)
	//	: m_nRequestId(nRequestId), m_sFastcgiPass(sFastcgiPass), m_response(response), m_nResponseState(REQUEST_INFO_STATE_WAITTING_RESPONSE)
	//	, m_nResponsePaddingLength(0)
	{
		m_tRequestTime = time(0);
		m_nPrevPaddingLen = 0;
		m_lpszBuffer = new unsigned char[DEFAULT_SEND_BUFFER_SIZE+1];
		m_lpszSendBuffer = new unsigned char[DEFAULT_SEND_BUFFER_SIZE+1];
		m_bSendBufferSize = 0;
	}
	virtual ~CFastcgiRequestInfo(void)
	{
#ifndef USES_FASTCGI_KEEP_CONN
		if (m_pFastCgiServer.get()!=NULL)
		{
			m_pFastCgiServer->stopClient();
			m_pFastCgiServer.reset();
		}
#endif
		delete[] m_lpszBuffer;
		delete[] m_lpszSendBuffer;
	}
private:
	CRequestPassInfo::pointer m_pRequestPassInfo;
	//int m_nRequestId;
	//std::string m_sFastcgiPass;
	cgcHttpResponse::pointer m_response;
	time_t m_tRequestTime;
	REQUEST_INFO_STATE m_nResponseState;
	bool m_bResponseError;
	int m_nResponsePaddingLength;
	std::string m_sLastData;
	int m_nPrevPaddingLen;
	//std::string m_sLastPaddingData;
	unsigned char* m_lpszBuffer;
	unsigned char* m_lpszSendBuffer;
	size_t m_bSendBufferSize;
	bool m_bParsedHead;
};
const CFastcgiRequestInfo::pointer NullFastcgiRequestInfo;

class CHttpTimeHandler
	: public cgcOnTimerHandler
	, public mycp::httpserver::TcpClient_Callback	// for tcp
{
public:
	typedef std::shared_ptr<CHttpTimeHandler> pointer;
	static CHttpTimeHandler::pointer create(void)
	{
		return CHttpTimeHandler::pointer(new CHttpTimeHandler());
	}
	CHttpTimeHandler(void)
	{
		m_tNow = time(0);
	}
	virtual ~CHttpTimeHandler(void)
	{
		m_pRequestPassInfoList.clear();
		m_pFastcgiClearList.clear();
		m_pIdlePassInfoList.clear();
	}

#ifndef WIN32
	int FindPidByName(const char* lpszName)
	{
		int nPid = 0;
		char lpszCmd[128];
		sprintf(lpszCmd,"ps -ef|grep -v 'grep'|grep '%s' | awk '{print $2}'",lpszName);
		//sprintf(lpszCmd,"ps -ef|grep '%s' | awk '{print $2}'",lpszName);
		FILE * f = popen(lpszCmd, "r");
		if (f==NULL)
			return -1;
		memset(lpszCmd,0,sizeof(lpszCmd));
		int index = 0;
		while (fgets(lpszCmd,128,f)!=NULL)
		{
			const int ret = atoi(lpszCmd);
			if (ret>0)
			{
				pclose(f);
				nPid = ret;
				//printf("**** 11 pid=%d,%s\n",nPid,lpszCmd);
				return nPid;
			}
			if ( (index++)>100 ) {
				pclose(f);
				return 0;
			}
		}
		pclose(f);
		nPid = atoi(lpszCmd);
		//printf("**** 22 pid=%d,%s\n",nPid,lpszCmd);
		return nPid;
	}
#endif

	CFastcgiRequestInfo::pointer GetFastcgiRequestInfo(SCRIPT_FILE_TYPE nScriptFileType, const cgcHttpResponse::pointer& response)
	{
		CRequestPassInfo::pointer pRequestPassInfo;
		if (!m_pRequestPassInfoList.front(pRequestPassInfo))
		{
			if (!m_pIdlePassInfoList.front(pRequestPassInfo))
			{
				BoostWriteLock wtlock(thePHPFastcgiInfo->m_mutex);
				if (thePHPFastcgiInfo->m_nCurrentFcgiPassPortIndex>=thePHPFastcgiInfo->m_nMaxProcess)
				{
					/// 超过最大进程数，使用默认等待
					return NullFastcgiRequestInfo;
				}else
				{
					const int nRequestId = 1+(thePHPFastcgiInfo->m_nCurrentFcgiPassPortIndex++);
					wtlock.unlock();
					char lpszBuffer[256];
					sprintf(lpszBuffer,"%s:%d",thePHPFastcgiInfo->m_sFcgiPassIp.c_str(),(int)thePHPFastcgiInfo->m_nFcgiPassPort+nRequestId-1);
					pRequestPassInfo = CRequestPassInfo::create(nRequestId,lpszBuffer);
					CreateRequestPassInfoProcess(pRequestPassInfo);
				}
			}else if (pRequestPassInfo->GetProcessId()==0)
			{
				//if (pRequestPassInfo->GetKillTime()>0)
				//{
				//	if (pRequestPassInfo->GetKillTime()+10>time(0))
				//	{
				//		printf("**** (%s) wait killtime, get next...\n",pRequestPassInfo->GetFastcgiPass().c_str());
				//		CFastcgiRequestInfo::pointer pFastcgiRequestInfo = GetFastcgiRequestInfo(nScriptFileType,response);	// get next
				//		m_pIdlePassInfoList.add(pRequestPassInfo);
				//		return pFastcgiRequestInfo;
				//	}
				//	printf("**** (%s) reset killtime\n",pRequestPassInfo->GetFastcgiPass().c_str());
				//	pRequestPassInfo->ResetKillTime();
				//	pRequestPassInfo->ResetErrorCount();
				//}
				CreateRequestPassInfoProcess(pRequestPassInfo);
			}
			pRequestPassInfo->IncreaseRequestCount(this->m_tNow);
		}else if (pRequestPassInfo->IncreaseRequestCount(this->m_tNow)>=thePHPFastcgiInfo->m_nMaxRequestRestart)
		{
			pRequestPassInfo->KillProcess();
			m_pIdlePassInfoList.add(pRequestPassInfo);
			return GetFastcgiRequestInfo(nScriptFileType,response);	/// get next
		}
		CFastcgiRequestInfo::pointer pFastcgiRequestInfo = CFastcgiRequestInfo::create(pRequestPassInfo,response);
		m_pFastcgiRequestList.insert(pRequestPassInfo->GetRequestId(),pFastcgiRequestInfo);
		return pFastcgiRequestInfo;
	}
	bool RemoveRequestInfo(const CFastcgiRequestInfo::pointer& pFastcgiRequestInfo)
	{
		if (m_pFastcgiRequestList.remove(pFastcgiRequestInfo->GetRequestId()))
		{
			m_pRequestPassInfoList.add(pFastcgiRequestInfo->GetRequestPassInfo());
			return true;
		}
		return false;
	}

	void CreateRequestPassInfoProcess(const CRequestPassInfo::pointer& pRequestPassInfo)
	{
		if (pRequestPassInfo.get()==0 || (pRequestPassInfo->GetProcessId()>0 && pRequestPassInfo->GetProcessHandle()!=0)) {
			return;
		}
		char lpszBuffer[512];
#ifdef WIN32
		/// php-cgi.exe -b 127.0.0.1:9000 -c php.ini
		if (thePHPFastcgiInfo->m_sFastcgiPath.empty())
			sprintf(lpszBuffer,"php-cgi.exe -b %s -c php.ini",pRequestPassInfo->GetFastcgiPass().c_str());
		else
			sprintf(lpszBuffer,"\"%s/php-cgi.exe\" -b %s -c \"%s/php.ini\"",thePHPFastcgiInfo->m_sFastcgiPath.c_str(),pRequestPassInfo->GetFastcgiPass().c_str(),thePHPFastcgiInfo->m_sFastcgiPath.c_str());
		//printf("*** CreateRequestPassInfoProcess... (%s)\n",lpszBuffer);
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		memset(&si,0,sizeof(si));
		si.cb=sizeof(si);
		si.wShowWindow=SW_HIDE;
		si.dwFlags=STARTF_USESHOWWINDOW;
		CreateProcess(NULL,lpszBuffer,NULL,FALSE,FALSE,0,NULL,NULL,&si,&pi);
		pRequestPassInfo->SetProcessHandle(pi.hProcess);
		pRequestPassInfo->SetProcessId(pi.dwProcessId);
		//printf("**** CreateProcess Id=%d,Handle=%d,LastError=%d\n",pi.dwProcessId,(int)pi.hProcess,GetLastError());
#else
		/// php-cgi -b 127.0.0.1:9000 -c php.ini
		if (thePHPFastcgiInfo->m_sFastcgiPath.empty())
			sprintf(lpszBuffer,"php-cgi -b %s -c \"/etc/php.ini\" &",pRequestPassInfo->GetFastcgiPass().c_str());
		else
		{
			sprintf(lpszBuffer,"cd \"%s\" ; ./php-cgi -b %s -c php.ini &",thePHPFastcgiInfo->m_sFastcgiPath.c_str(),pRequestPassInfo->GetFastcgiPass().c_str());
			//sprintf(lpszBuffer,"cd \"%s\" && ./php-cgi -b %s -c php.ini &",thePHPFastcgiInfo->m_sFastcgiPath.c_str(),pRequestPassInfo->GetFastcgiPass().c_str());
		}
		system(lpszBuffer);
		for (int i=0;i<100;i++)	// 5S
		{
			usleep(50000);
			const int nPid = FindPidByName(pRequestPassInfo->GetFastcgiPass().c_str());
			if (nPid>0)
			{
				pRequestPassInfo->SetProcessHandle((void*)nPid);
				pRequestPassInfo->SetProcessId(nPid);
				break;
			}
		}
#endif
#ifdef WIN32
		Sleep(1200);
#else
		usleep(1200000);
#endif
	}
	void PHP_FCGIPM(void)
	{
		if (thePHPFastcgiInfo.get()==0) {
			return;
		}

		int nFastcgiProcessCount = 0;
		BoostWriteLock wtlock(m_pRequestPassInfoList.mutex());
		CLockList<CRequestPassInfo::pointer>::iterator pIter = m_pRequestPassInfoList.begin();
		for (; pIter!=m_pRequestPassInfoList.end(); ) {
			const CRequestPassInfo::pointer& pRequestPassInfo = *pIter;
			if (pRequestPassInfo->GetProcessId()==0 || pRequestPassInfo->GetProcessHandle()==0) {
				pIter++;
				continue;
			}
			nFastcgiProcessCount++;

			if (pRequestPassInfo->GetRequestCount()>=thePHPFastcgiInfo->m_nMaxRequestRestart ||		/// 超时最大请求次数，关闭
				(nFastcgiProcessCount>thePHPFastcgiInfo->m_nMinProcess && (pRequestPassInfo->GetLastRequestTime()+60)<this->m_tNow ))	/// 超过最小进程数量，并且超过60秒没有访问，关闭
			{
				//printf("**** RequestId=%d,RequestCount=%d\n",pRequestPassInfo->GetRequestId(),pRequestPassInfo->GetRequestCount());
				/// * 控制一次只退出一个process
				const CRequestPassInfo::pointer pRequestPassInfoTemp = pRequestPassInfo;
				m_pRequestPassInfoList.erase(pIter);
				wtlock.unlock();
				pRequestPassInfoTemp->KillProcess();
				m_pIdlePassInfoList.add(pRequestPassInfoTemp);
				break;
			}
#ifdef WIN32
			DWORD ExitCode = STILL_ACTIVE;
			GetExitCodeProcess((HANDLE)pRequestPassInfo->GetProcessHandle(),&ExitCode);
			//printf("**** GetExitCodeProcess %d -> %d\n",pRequestPassInfo->GetProcessId(),ExitCode);
			if (ExitCode!=STILL_ACTIVE) {
#else
			const int nPID = FindPidByName(pRequestPassInfo->GetFastcgiPass().c_str());
			if (nPID<=0) {
#endif
				const CRequestPassInfo::pointer pRequestPassInfoTemp = pRequestPassInfo;
				m_pRequestPassInfoList.erase(pIter++);
				pRequestPassInfoTemp->KillProcess();
				m_pIdlePassInfoList.add(pRequestPassInfoTemp);
				break;	/// 一次只退出一个
				//pRequestPassInfo->KillProcess();
				//CreateRequestPassInfoProcess(pRequestPassInfo);
			}
			else {
				pIter++;
			}
		}
	}

	time_t m_tNow;
	virtual void OnTimeout(unsigned int nIDEvent, const void * pvParam)
	{
		if (nIDEvent==TIMERID_1_SECOND)
		{
			static unsigned int theSecondIndex = 0;
			theSecondIndex++;
			m_tNow = time(0);

			if ((theSecondIndex%10)==9)		/// 10秒处理一次
			{
				PHP_FCGIPM();
				m_pFastcgiClearList.clear();
			}

			if ((theSecondIndex%(24*3600))==23*3600)	/// 一天处理一次
			{
				//std::vector<tstring> pRemoveFileNameList;
				if (!theIdleFileInfos.empty())
				{
					// 删除超过10天没用文件数据；
#ifdef USES_BODB_SCRIPT
					char sql[512];
#endif
					BoostWriteLock wtlock(theIdleFileInfos.mutex());
					//BoostReadLock rdlock(theIdleFileInfos.mutex());
					CLockMap<tstring, CCSPFileInfo::pointer>::iterator pIter = theIdleFileInfos.begin();
					for (; pIter!=theIdleFileInfos.end(); )
					{
						CCSPFileInfo::pointer fileInfo = pIter->second;
						if (fileInfo->m_tRequestTime==0 || (fileInfo->m_tRequestTime+(10*24*3600))<m_tNow)	// ** 10 days
						{
							const tstring& sFileName = pIter->first;
							//pRemoveFileNameList.push_back(sFileName);
							theFileInfos.remove(sFileName);
							theFileScripts.remove(sFileName);
#ifdef USES_BODB_SCRIPT
							if (theCdbcService.get()!=NULL)
							{
								tstring sFileNameTemp(sFileName);
								theCdbcService->escape_string_in(sFileNameTemp);
								sprintf(sql, "DELETE FROM scriptitem_t WHERE filename='%s'", sFileNameTemp.c_str());
								theCdbcService->execute(sql);
								sprintf(sql, "DELETE FROM cspfileinfo_t WHERE filename='%s')",sFileNameTemp.c_str());
								theCdbcService->execute(sql);
							}
#endif
							theIdleFileInfos.erase(pIter++);
						}else
						{
							pIter++;
						}
					}
				}
				//for (size_t i=0; i<pRemoveFileNameList.size(); i++)
				//{
				//	theIdleFileInfos.remove(pRemoveFileNameList[i]);
				//}
				//pRemoveFileNameList.clear();
			}
			if ((theSecondIndex%3600)==3500)	// 3600=60分钟处理一次
			{
				//std::vector<tstring> pRemoveFileNameList;
				if (!theFileInfos.empty())
				{
					// 超过2小时没有访问文件， 放到空闲列表；
					BoostWriteLock wtlock(theFileInfos.mutex());
					//BoostReadLock rdlock(theFileInfos.mutex());
					CLockMap<tstring, CCSPFileInfo::pointer>::iterator pIter = theFileInfos.begin();
					for (; pIter!=theFileInfos.end(); )
					{
						const CCSPFileInfo::pointer& fileInfo = pIter->second;
						if (fileInfo->m_tRequestTime>0 && (fileInfo->m_tRequestTime+(3*3600))<m_tNow)	// ** 3 hours
						{
							if (!theFileScripts.remove(pIter->first))
							{
								//pRemoveFileNameList.push_back(pIter->first);
								theIdleFileInfos.insert(pIter->first,pIter->second,false);
								theFileInfos.erase(pIter++);
								continue;
							}
						}
						pIter++;
					}
				}
				//for (size_t i=0; i<pRemoveFileNameList.size(); i++)
				//{
				//	theFileInfos.remove(pRemoveFileNameList[i]);
				//}
				//pRemoveFileNameList.clear();

				StringObjectMapPointer pFileInfoList = theAppAttributes->getStringAttributes(ATTRIBUTE_FILE_INFO,false);
				if (pFileInfoList.get() != NULL && !pFileInfoList->empty())
				{
					//std::vector<tstring> pRemoveFilePathList;
					{
						BoostWriteLock wtlock(pFileInfoList->mutex());
						//BoostReadLock rdlock(pFileInfoList->mutex());
						CObjectMap<tstring>::iterator pIter = pFileInfoList->begin();
						for (;pIter!=pFileInfoList->end();)
						{
							const CResInfo::pointer pResInfo = CGC_OBJECT_CAST<CResInfo>(pIter->second);
							if (pResInfo->m_tRequestTime>0 && (pResInfo->m_tRequestTime+(3*3600))<m_tNow)	// ** 3 hours
							{
								pFileInfoList->erase(pIter++);
								//pRemoveFilePathList.push_back(pIter->first);
							}else
							{
								pIter++;
							}
						}
					}
					//for (size_t i=0; i<pRemoveFilePathList.size(); i++)
					//{
					//	pFileInfoList->remove(pRemoveFilePathList[i]);
					//	//theAppAttributes->removeAttribute(ATTRIBUTE_FILE_INFO, pRemoveFilePathList[i]);
					//}
					//pRemoveFilePathList.clear();
				}
			}

		}
	}

	virtual void OnDisconnect(int nUserData)
	{
		//theWaitDataList.remove(nUserData);
		//CGC_LOG((mycp::LOG_TRACE, "OnDisconnect UserData=%d\n", nUserData));
		//printf("******** OnDisconnect UserData=%d\n",nUserData);
		const int nTcpRequestId = nUserData;
		CFastcgiRequestInfo::pointer pFastcgiRequestInfo;
		if (!m_pFastcgiRequestList.find(nTcpRequestId,pFastcgiRequestInfo,true))
		{
			return;
		}
		for (int i=0;i<100;i++)
		{
			if (pFastcgiRequestInfo->GetResponseState()!=REQUEST_INFO_STATE_FCGI_DOING)
				break;
#ifdef WIN32
			Sleep(10);
#else
			usleep(10000);
#endif
		}
		//pFastcgiRequestInfo->SetResponseState(REQUEST_INFO_STATE_FCGI_DISCONNECTED);
		m_pRequestPassInfoList.add(pFastcgiRequestInfo->GetRequestPassInfo());
		m_pFastcgiClearList.add(pFastcgiRequestInfo);	// *避免挂死
	}
	void ParseFastcgi(CFastcgiRequestInfo::pointer pFastcgiRequestInfo, const char* pData, size_t nSize, int nUserData)
	{
		if (pData==NULL || nSize==0) return;

		const int nWaitPaddingSize = pFastcgiRequestInfo->GetResponsePaddingLength();
		//printf("******** nWaitPaddingSize=%d\n",nWaitPaddingSize);
		if (nWaitPaddingSize>0)// && (int)nSize>=nWaitPaddingSize)
		{
			//const std::string s(pData,30);
			//printf("******** s=%d\n",s.c_str());
			const int nPrevPaddingLen = pFastcgiRequestInfo->GetPrevPaddingLen();
			if ((int)nSize>=nWaitPaddingSize)
			{
				pFastcgiRequestInfo->response()->writeData(pData,nWaitPaddingSize);
				pFastcgiRequestInfo->SetResponsePaddingLength(0);
				if (nPrevPaddingLen>0)
				{
					//printf("******** AAAAAAAAA WaitSize=%d,nPrevPaddingLen=%d\n", nWaitPaddingSize, nPrevPaddingLen);
					pFastcgiRequestInfo->SetPrevPaddingLen(0);
					ParseFastcgi(pFastcgiRequestInfo,pData+nWaitPaddingSize+nPrevPaddingLen,nSize-nWaitPaddingSize-nPrevPaddingLen,nUserData);
				}else
				{
					ParseFastcgi(pFastcgiRequestInfo,pData+nWaitPaddingSize,nSize-nWaitPaddingSize,nUserData);
				}
			}else
			{
				//printf("******** WaitSize=%d,%d\n", nWaitPaddingSize, nWaitPaddingSize-nSize);
				//if (pFastcgiRequestInfo->GetPrevPaddingLen()>0)
				//{
				//	printf("******** BBBBBBBBBBBBBB WaitSize=%d,nPrevPaddingLen=%d\n", nWaitPaddingSize, pFastcgiRequestInfo->GetPrevPaddingLen());
				//}
				pFastcgiRequestInfo->response()->writeData(pData,nSize);
				pFastcgiRequestInfo->SetResponsePaddingLength(nWaitPaddingSize-nSize);
			}
			return;
		}
		if (nSize<FCGI_HEADER_LEN)
		{
			if (pData!=NULL && nSize>0)
			{
				//CGC_LOG((mycp::LOG_TRACE, "********* UserData=%d,LastDataSize=%d\n", nUserData,nSize));
				//pFastcgiRequestInfo->SetLastData(std::string(pData,nSize));
				//return;
				pFastcgiRequestInfo->AddLastData(pData,nSize);
				const std::string& sLastData = pFastcgiRequestInfo->GetLastData();
				if (sLastData.size()<FCGI_HEADER_LEN)
					return;
			}else
			{
				return;
			}
		}
		const int nTcpRequestId = nUserData;
		FCGI_Header header;
		const std::string& sLastData = pFastcgiRequestInfo->GetLastData();
		const int nLastDataSize = (int)sLastData.size();
		if (nLastDataSize==0)
		{
			memcpy(&header,pData,FCGI_HEADER_LEN);
		}else
		{
			if (nLastDataSize>=FCGI_HEADER_LEN)
			{
				//pFastcgiRequestInfo->ClearLastData();
				//return;	// *
				memcpy(&header,sLastData.c_str(),FCGI_HEADER_LEN);
				if (nLastDataSize>FCGI_HEADER_LEN)
				{
					const std::string sLastDataTemp(sLastData.substr(FCGI_HEADER_LEN));
					pFastcgiRequestInfo->response()->writeData(sLastDataTemp.c_str(), sLastDataTemp.size());
				}
			}else
			{
				char lpszBuf[FCGI_HEADER_LEN+1];
				memcpy(lpszBuf,sLastData.c_str(),nLastDataSize);
				memcpy(lpszBuf+nLastDataSize,pData,FCGI_HEADER_LEN-nLastDataSize);
				memcpy(&header,lpszBuf,FCGI_HEADER_LEN);
			}
			//CGC_LOG((mycp::LOG_TRACE, "********* UserData=%d,LastDataSize=%d process ok\n", nUserData,nLastDataSize));
		}

		if(header.version != FCGI_VERSION_1) {
			pFastcgiRequestInfo->SetPrevPaddingLen(0);
			pFastcgiRequestInfo->ClearLastData();
			return ;//FCGX_UNSUPPORTED_VERSION;
		}
		const int nRequestId = (header.requestIdB1 << 8) + header.requestIdB0;
		if (nRequestId!=nTcpRequestId)
			return;
		const int nContentLen = (header.contentLengthB1 << 8)	+ header.contentLengthB0;
		const int nPaddingLen = header.paddingLength;
		//CGC_LOG((mycp::LOG_TRACE, "RequestId=%d,type=%d,DataSize=%d,ContentLen=%d,PaddingLen=%d\n", nRequestId,(int)header.type,nSize,nContentLen,nPaddingLen));
		//printf("******** RequestId=%d,type=%d,DataSize=%d,ContentLen=%d,PaddingLen=%d\n",nRequestId,(int)header.type,nSize,nContentLen,nPaddingLen);
		//pFastcgiRequestInfo->SetResponsePaddingLength(nPaddingLen);
		if (nLastDataSize>0)
		{
			pFastcgiRequestInfo->SetPrevPaddingLen(nPaddingLen);
			pFastcgiRequestInfo->ClearLastData();
			if (nLastDataSize>FCGI_HEADER_LEN)
			{
				pFastcgiRequestInfo->SetResponsePaddingLength(nContentLen-(nLastDataSize-FCGI_HEADER_LEN));
				ParseFastcgi(pFastcgiRequestInfo,pData,nSize,nUserData);
			}else
			{
				pFastcgiRequestInfo->SetResponsePaddingLength(nContentLen);
				ParseFastcgi(pFastcgiRequestInfo,pData+(FCGI_HEADER_LEN-nLastDataSize),nSize-(FCGI_HEADER_LEN-nLastDataSize),nUserData);
			}
			return;
		}else if (pFastcgiRequestInfo->GetPrevPaddingLen()>0)
		{
			pFastcgiRequestInfo->SetPrevPaddingLen(0);
		}

		try
		{
			if (header.type == FCGI_STDOUT)
			{
				//const bool bNeedParseHeader = pFastcgiRequestInfo->GetResponseState()==REQUEST_INFO_STATE_WAITTING_RESPONSE?true:false;
				pFastcgiRequestInfo->SetResponseState(REQUEST_INFO_STATE_FCGI_DOING);
				int nBodyLength = min(nContentLen,(int)nSize-FCGI_HEADER_LEN);
				//printf("********\n%s\n", data->data()+8);
				//const char * findSearch = NULL;
				if (pFastcgiRequestInfo->GetParsedHead())
				{
					if (nBodyLength>0)
					{
						//CGC_LOG((mycp::LOG_TRACE, "response()->writeData size=%d\n", nBodyLength));
						pFastcgiRequestInfo->response()->writeData(pData+FCGI_HEADER_LEN,nBodyLength);
					}
				}else
				{
					pFastcgiRequestInfo->SetParsedHead(true);
					const char * findSearchEnd = NULL;
					const char * httpRequest = (const char*)(pData+FCGI_HEADER_LEN);
					bool bFindHttpHead = false;
					while (nBodyLength>0 && httpRequest != NULL)
					{
						findSearchEnd = strstr(httpRequest, "\r\n");
						if (findSearchEnd==NULL)
						{
							//printf("******** 1 length=%d,data=%s\n",nBodyLength,httpRequest);
							if (nBodyLength>0)
							{
								//CGC_LOG((mycp::LOG_TRACE, "response()->writeData size=%d\n", nBodyLength));
								pFastcgiRequestInfo->response()->writeData(httpRequest,nBodyLength);
							}
							break;
						}else if (findSearchEnd==httpRequest)
						{
							if (bFindHttpHead)
							{
								httpRequest = httpRequest+2;
								nBodyLength -= 2;
							}else
							{
								httpRequest = httpRequest+4;
								nBodyLength -= 4;
							}
							//printf("******** 2 length=%d,data=%s\n",nBodyLength,httpRequest);
							if (nBodyLength>0)
							{
								//CGC_LOG((mycp::LOG_TRACE, "response()->writeData size=%d\n", nBodyLength));
								pFastcgiRequestInfo->response()->writeData(httpRequest,nBodyLength);
							}
							break;
						}
						const std::string sLine(httpRequest,findSearchEnd-httpRequest);
						const std::string::size_type find = sLine.find(":");
						if (find==std::string::npos)
						{
							break;
						}
						const tstring sParamReal(sLine.substr(0,find));
						tstring param(sParamReal);
						std::transform(param.begin(), param.end(), param.begin(), ::tolower);
						const short nOffset = sLine.c_str()[find+1]==' '?2:1;	// 带空格2，不带空格1
						const tstring value(sLine.substr(find+nOffset));

						bFindHttpHead = true;
						//CGC_LOG((mycp::LOG_TRACE, "%s:%s\n", sParamReal.c_str(), value.c_str()));
						//if (param=="content-type")
						//{
						//	pFastcgiRequestInfo->response()->setContentType(value);
						//	//}else if (param=="content-length")
						//	//{
						//	//	//pFastcgiRequestInfo->response()->setcon(value);
						//}else
						if (param=="status")
						{
							const HTTP_STATUSCODE nHttpState = (HTTP_STATUSCODE)atoi(value.c_str());
							pFastcgiRequestInfo->response()->setStatusCode(nHttpState);
						}else
						{
							pFastcgiRequestInfo->response()->setHeader(sParamReal,value);
						}
						nBodyLength -= (int)(findSearchEnd-httpRequest);
						nBodyLength -= 2;	// \r\n
						httpRequest = findSearchEnd+2;
					}
				}
			}else if (header.type == FCGI_STDERR)
			{
				const int nBodyLength = min(nContentLen,(int)nSize-FCGI_HEADER_LEN);
				pFastcgiRequestInfo->response()->writeData((const char*)(pData+FCGI_HEADER_LEN),nBodyLength);
				//pFastcgiRequestInfo->response()->writeData((const char*)(pData+FCGI_HEADER_LEN),nContentLen);
				//CGC_LOG((mycp::LOG_ERROR, "RequestId=%d FCGI_STDERR:%s\n", nRequestId, std::string(pData+FCGI_HEADER_LEN,nContentLen).c_str()));
				pFastcgiRequestInfo->SetResponseState(REQUEST_INFO_STATE_FCGI_END_REQUEST,false);
				return;
			}else if (header.type == FCGI_END_REQUEST)
			{
				int nEndStatus = FCGI_REQUEST_COMPLETE;
				if ((nSize-nContentLen)==8)
				{
					//FCGI_REQUEST_COMPLETE：请求的正常结束。
					//FCGI_CANT_MPX_CONN：拒绝新请求。这发生在Web服务器通过一条线路向应用发送并发的请求时，后者被设计为每条线路每次处理一个请求。
					//FCGI_OVERLOADED：拒绝新请求。这发生在应用用完某些资源时，例如数据库连接。
					//FCGI_UNKNOWN_ROLE：拒绝新请求。这发生在Web服务器指定了一个应用不能识别的角色时。
					FCGI_EndRequestBody pEndRequestBody;
					memcpy(&pEndRequestBody,pData+FCGI_HEADER_LEN,8);
					nEndStatus = pEndRequestBody.protocolStatus;
				}

				//CGC_LOG((mycp::LOG_TRACE, "RequestId=%d set REQUEST_INFO_STATE_FCGI_END_REQUEST EndStatus=%d\n", nRequestId, nEndStatus));
				const bool bResponseError = nEndStatus == FCGI_REQUEST_COMPLETE?false:true;
				pFastcgiRequestInfo->SetResponseState(REQUEST_INFO_STATE_FCGI_END_REQUEST,bResponseError);
				//pFastcgiRequestInfo->GetRequestPassInfo()->SetCloseEvent();
//#ifdef USES_FASTCGI_KEEP_CONN
//				RemoveRequestInfo(pFastcgiRequestInfo);
//#endif
				return;
			}
			const int nUsedSize = FCGI_HEADER_LEN+nContentLen+nPaddingLen;
			//printf("******** UsedSize=%d\n",nUsedSize);
			if ((int)nSize-nUsedSize>=FCGI_HEADER_LEN)
			{
				ParseFastcgi(pFastcgiRequestInfo,pData+nUsedSize,nSize-nUsedSize,nUserData);
			}else
			{
				const int nWaitSize = (nContentLen+nPaddingLen)-((int)nSize-FCGI_HEADER_LEN);
				if (nWaitSize>0)
				{
					//printf("******** WaitSize=%d\n",nWaitSize);
					//const int nPrevWaitSize = pFastcgiRequestInfo->GetResponsePaddingLength();
					//printf("******** WaitSize=%d, PrevWaitSize=%d\n",nWaitSize,nPrevWaitSize);
					pFastcgiRequestInfo->SetResponsePaddingLength(nWaitSize);
				}
				if (header.type == FCGI_STDOUT)
					pFastcgiRequestInfo->SetResponseState(REQUEST_INFO_STATE_FCGI_STDOUT);
				else if (header.type == FCGI_STDERR)
					pFastcgiRequestInfo->SetResponseState(REQUEST_INFO_STATE_FCGI_STDERR);
			}
		}catch(std::exception const &)
		{
		}catch(...)
		{}
	}
	virtual void OnReceiveData(const mycp::asio::ReceiveBuffer::pointer& data, int nUserData)
	{
		//theWaitDataList.remove(nUserData);

		//CGC_LOG((mycp::LOG_TRACE, "OnReceiveData size=%d,UserData=%d\n", data->size(),nUserData));
		//printf("******** OnReceiveData size=%d,UserData=%d\n",data->size(),nUserData);
		const int nTcpRequestId = nUserData;
		CFastcgiRequestInfo::pointer pFastcgiRequestInfo;
		if (m_pFastcgiRequestList.find(nTcpRequestId,pFastcgiRequestInfo))
		{
			ParseFastcgi(pFastcgiRequestInfo,(const char*)data->data(),data->size(),nUserData);
		}
	}
private:
	CLockList<CRequestPassInfo::pointer> m_pRequestPassInfoList;
	CLockList<CRequestPassInfo::pointer> m_pIdlePassInfoList;
	CLockMap<int,CFastcgiRequestInfo::pointer> m_pFastcgiRequestList;
	CLockList<CFastcgiRequestInfo::pointer> m_pFastcgiClearList;
};
CHttpTimeHandler::pointer theTimerHandler;

static FCGI_Header MakeHeader(
        int type,
        int requestId,
        int contentLength,
        int paddingLength)
{
    FCGI_Header header;
    //ASSERT(contentLength >= 0 && contentLength <= FCGI_MAX_LENGTH);
    //ASSERT(paddingLength >= 0 && paddingLength <= 0xff);
    header.version = FCGI_VERSION_1;
    header.type             = (unsigned char) type;
    header.requestIdB1      = (unsigned char) ((requestId     >> 8) & 0xff);
    header.requestIdB0      = (unsigned char) ((requestId         ) & 0xff);
    header.contentLengthB1  = (unsigned char) ((contentLength >> 8) & 0xff);
    header.contentLengthB0  = (unsigned char) ((contentLength     ) & 0xff);
    header.paddingLength    = (unsigned char) paddingLength;
    header.reserved         =  0;
    return header;
}
//static unsigned char* FCGI_BuildStdinBody(int nRequestId, const char *name,int nameLen, int * pOutSize, int * pOutPaddingSize)
//{
//	unsigned char* lpszBuffer = new unsigned char[FCGI_HEADER_LEN+nameLen+8];
//	int nPaddingLength = nameLen%8;
//	if (nPaddingLength>0)
//		nPaddingLength = 8-nPaddingLength;
//	//nPaddingLength = 0;
//	const FCGI_Header header = MakeHeader(FCGI_STDIN,nRequestId,nameLen,nPaddingLength);
//	memcpy(lpszBuffer,&header, FCGI_HEADER_LEN);
//	memcpy(lpszBuffer+FCGI_HEADER_LEN,name, nameLen);
//	if (nPaddingLength>0)
//		memset(lpszBuffer+(FCGI_HEADER_LEN+nameLen),0, nPaddingLength);
//	*pOutSize = FCGI_HEADER_LEN+nameLen+nPaddingLength;
//	*pOutPaddingSize = nPaddingLength;
//	return lpszBuffer;
//}
static unsigned char* FCGI_BuildStdinHeader(int nRequestId, unsigned char *lpszBuffer,int bodyLen, int * pOutSize)
{
	int nPaddingLength = bodyLen%8;
	if (nPaddingLength>0)
		nPaddingLength = 8-nPaddingLength;
	const FCGI_Header header = MakeHeader(FCGI_STDIN,nRequestId,bodyLen,nPaddingLength);
	memcpy(lpszBuffer,&header, FCGI_HEADER_LEN);
	if (nPaddingLength>0)
		memset(lpszBuffer+(FCGI_HEADER_LEN+bodyLen),0, nPaddingLength);
	*pOutSize = FCGI_HEADER_LEN+bodyLen+nPaddingLength;
	return lpszBuffer;
}
static unsigned char* FCGI_BuildParamsBody(int nRequestId, const char *name,int nameLen,const char *value,int valueLen, int * pOutSize, unsigned char* lpszInBuffer)
{
	unsigned char* lpszBuffer = lpszInBuffer!=NULL?lpszInBuffer:new unsigned char[FCGI_HEADER_LEN+16+nameLen+valueLen];
	int nOffset = FCGI_HEADER_LEN;
	if (nameLen < 0x80) {
		lpszBuffer[nOffset] = (unsigned char) nameLen;
		nOffset += 1;
	} else {
		lpszBuffer[nOffset]   = (unsigned char) ((nameLen >> 24) | 0x80);
		lpszBuffer[nOffset+1] = (unsigned char) (nameLen >> 16);
		lpszBuffer[nOffset+2] = (unsigned char) (nameLen >> 8);
		lpszBuffer[nOffset+3] = (unsigned char) nameLen;
		nOffset += 4;
	}
	if (valueLen < 0x80) {
		lpszBuffer[nOffset] = (unsigned char) valueLen;
		nOffset += 1;
	} else {
		lpszBuffer[nOffset]   = (unsigned char) ((valueLen >> 24) | 0x80);
		lpszBuffer[nOffset+1] = (unsigned char) (valueLen >> 16);
		lpszBuffer[nOffset+2] = (unsigned char) (valueLen >> 8);
		lpszBuffer[nOffset+3] = (unsigned char) valueLen;
		nOffset += 4;
	}
	int nPaddingLength = (nOffset+nameLen+valueLen)%8;
	if (nPaddingLength>0)
		nPaddingLength = 8-nPaddingLength;
	//printf("*** nPaddingLength=%d\n",nPaddingLength);
	const FCGI_Header header = MakeHeader(FCGI_PARAMS,nRequestId,nOffset-FCGI_HEADER_LEN+nameLen+valueLen,nPaddingLength);
	memcpy(lpszBuffer,&header, FCGI_HEADER_LEN);
	memcpy(lpszBuffer+nOffset,name, nameLen);
	memcpy(lpszBuffer+(nOffset+nameLen),value, valueLen);
	if (nPaddingLength>0)
		memset(lpszBuffer+(nOffset+nameLen+valueLen),0, nPaddingLength);
	*pOutSize = nOffset+nameLen+valueLen+nPaddingLength;
	return lpszBuffer;
}

class CContentTypeInfo
{
public:
	typedef std::shared_ptr<CContentTypeInfo> pointer;
	static CContentTypeInfo::pointer create(const tstring& sContentType, bool bDownload, bool bImageOrBinary=false, SCRIPT_FILE_TYPE nScriptFileType = SCRIPT_FILE_TYPE_UNKNOWN)
	{
		return CContentTypeInfo::pointer(new CContentTypeInfo(sContentType, bDownload, bImageOrBinary, nScriptFileType));
	}
	tstring m_sContentType;
	bool m_bDownload;
	bool m_bImageOrBinary;
	SCRIPT_FILE_TYPE m_nScriptFileType;

	CContentTypeInfo(const tstring& sContentType, bool bDownload, bool bImageOrBinary=false, SCRIPT_FILE_TYPE nScriptFileType = SCRIPT_FILE_TYPE_UNKNOWN)
		: m_sContentType(sContentType), m_bDownload(bDownload), m_bImageOrBinary(bImageOrBinary), m_nScriptFileType(nScriptFileType)
	{}
	CContentTypeInfo(void)
		: m_bDownload(false), m_bImageOrBinary(false), m_nScriptFileType(SCRIPT_FILE_TYPE_UNKNOWN)
	{}
};
//mycp::cgcApplication2::pointer theApplication2;

extern "C" void CGC_API CGC_Module_ResetService(const tstring& sServiceName)
{
	// 重新启动，需要处理 
	//printf("**** HttpServer CGC_Module_ResetService(), sServiceName=%s\n",sServiceName.c_str());
	if (sServiceName=="FileSystemService")
	{
		cgcServiceInterface::pointer pService = theServiceManager->getService(sServiceName);
		if (pService.get()!=NULL)
			theFileSystemService = pService;	// ?
	}

	//printf("**** HttpServer CGC_Module_ResetService()\n");
#ifdef USES_OBJECT_LIST_APP
	ObjectListPointer apps = theAppAttributes->getListAttributes((int)OBJECT_APP, false);
	if (apps.get()!=NULL && !apps->empty())
	{
		BoostWriteLock wtlock(apps->mutex());
		CLockList<cgcObject::pointer>::iterator pIter = apps->begin();
		for (; pIter!=apps->end(); )
		{
			cgcValueInfo::pointer pVarApp = CGC_OBJECT_CAST<cgcValueInfo>(*pIter);
			if (pVarApp->getType() == cgcValueInfo::TYPE_OBJECT && pVarApp->getInt() == (int)OBJECT_APP && pVarApp->getStr()==sServiceName)
			{
				//printf("**** HttpServer CGC_Module_ResetService(), resetService AppName=%s\n",sServiceName.c_str());
				theServiceManager->resetService(CGC_OBJECT_CAST<cgcServiceInterface>(pVarApp->getObject()));
				apps->erase(pIter++);
			}else
			{
				pIter++;
			}
		}
	}
#else
	theAppAttributes->delProperty((int)OBJECT_APP);
#endif

	const CLockMap<tstring, CAppInfo::pointer>& pAppList = theApps->getApps();
	CLockMap<tstring, CAppInfo::pointer>::const_iterator pIter = pAppList.begin();
	for (; pIter!=pAppList.end(); pIter++)
	{
		const CAppInfo::pointer& pAppInfo = pIter->second;
		if (pAppInfo->getAppName()==sServiceName)
		{
			const tstring sAppName = VTI_APP+pAppInfo->getAppId();
			//printf("**** HttpServer CGC_Module_ResetService(), delAllProperty %s, AppName=%s\n",sServiceName.c_str(),sAppName.c_str());
			theVirtualHosts.delAllProperty(sAppName);
		}
	}

	//std::vector<cgcValueInfo::pointer> pAppList;
	//theAppAttributes->getProperty((int)OBJECT_APP, pAppList);
	//for (size_t i=0; i<pAppList.size(); i++)
	//{
	//	// 

	//}
}

const int const_one_hour_seconds = 60*60;
const int const_one_day_seconds = 24*const_one_hour_seconds;
const int const_memory_size = 50*1024*1024;		// max 50MB
//const int const_max_size = 120*1024*1024;		// max 120MB	// 50
unsigned int theMaxSize = 120*1024*1024;		// max 120MB	// 50

CLockMap<tstring,CContentTypeInfo::pointer> theContentTypeInfoList;
extern "C" bool CGC_API CGC_Module_Init2(MODULE_INIT_TYPE nInitType)
//extern "C" bool CGC_API CGC_Module_Init(void)
{
	if (theAppAttributes.get() != NULL) {
		CGC_LOG((mycp::LOG_ERROR, "CGC_Module_Init2 rerun error, InitType=%d.\n", nInitType));
		return true;
	}

	theFileSystemService = theServiceManager->getService("FileSystemService");
	if (theFileSystemService.get() == NULL)
	{
		CGC_LOG((mycp::LOG_ERROR, "FileSystemService Error.\n"));
		return false;
	}
	//theStringService = theServiceManager->getService("StringService");
	//if (theStringService.get() == NULL)
	//{
	//	CGC_LOG((mycp::LOG_ERROR, "StringService Error.\n"));
	//	return false;
	//}

	theAppInitParameters = theApplication->getInitParameters();

	// Load APP calls.
	tstring xmlFile(theApplication->getAppConfPath());
	xmlFile.append("/apps.xml");
	cgcValueInfo::pointer outProperty = CGC_VALUEINFO(false);
	theFileSystemService->callService("exists", CGC_VALUEINFO(xmlFile), outProperty);
	if (outProperty->getBoolean())
	{
		theApps = XMLPARSEAPPS;
		theApps->load(xmlFile);
	}

	//theApplication2 = CGC_APPLICATION2_CAST(theApplication);
	//assert(theApplication2.get()!=NULL);

	// Load DataSource.
	xmlFile = theApplication->getAppConfPath();
	xmlFile.append("/datasources.xml");
	outProperty->setBoolean(false);
	theFileSystemService->callService("exists", CGC_VALUEINFO(xmlFile), outProperty);
	if (outProperty->getBoolean())
	{
		theDataSources = XMLPARSEDSS;
		theDataSources->load(xmlFile);
	}

	// Load virtual hosts.
	xmlFile = theApplication->getAppConfPath();
	xmlFile.append("/hosts.xml");
	theVirtualHosts.load(xmlFile);
	thePHPFastcgiInfo = theVirtualHosts.getFastcgiInfo("php");
	if (thePHPFastcgiInfo.get()!=NULL)
	{
		tstring sThirdParth(theSystem->getServerPath());
		sThirdParth.append("/thirdparty");
		mycp::replace_string(thePHPFastcgiInfo->m_sFastcgiPath,$MYCP_THIRDPARTY_PATH,sThirdParth);
		const std::string::size_type find = thePHPFastcgiInfo->m_sFastcgiPass.find(":",1);
		if (find!=std::string::npos)
		{
			thePHPFastcgiInfo->m_sFcgiPassIp = thePHPFastcgiInfo->m_sFastcgiPass.substr(0,find);
			thePHPFastcgiInfo->m_nFcgiPassPort = atoi(thePHPFastcgiInfo->m_sFastcgiPass.substr(find+1).c_str());
		}

		// *预防前面异常退出，挂住，导致网络端口被占用错误；
#ifdef WIN32
		KillAllProcess("php-cgi.exe");
#else
		char lpszBuffer[256];
		sprintf(lpszBuffer,"ps -e|grep -v 'grep'|grep php-cgi | awk '{print $1}' | xargs kill -9");
		system(lpszBuffer);
		sprintf(lpszBuffer,"chmod +x \"%s/php-cgi\"",thePHPFastcgiInfo->m_sFastcgiPath.c_str());
		system(lpszBuffer);
#endif
	}

	theDefaultHost = theVirtualHosts.getVirtualHost("*");
	if (theDefaultHost.get() == NULL)
	{
		CGC_LOG((mycp::LOG_ERROR, "DefaultHost Error. %s\n",xmlFile.c_str()));
		return false;
	}
	tstring sDocumentRoot(theDefaultHost->getDocumentRoot());
	mycp::replace_string(sDocumentRoot,$MYCP_CONF_PATH,theSystem->getServerPath());
	theDefaultHost->setDocumentRoot(sDocumentRoot);
	CGC_LOG((mycp::LOG_INFO, "%s\n",theDefaultHost->getDocumentRoot().c_str()));
	theDefaultHost->setPropertys(theApplication->createAttributes());
	if (theDefaultHost->getDocumentRoot().empty())
		theDefaultHost->setDocumentRoot(theApplication->getAppConfPath());
	else
	{
		tstring sDocumentRoot(theDefaultHost->getDocumentRoot());
		namespace fs = boost::filesystem;
		fs::path src_path(sDocumentRoot.c_str());
		if (!fs::exists(src_path))
		{
			tstring sDocumentRootTemp(theDefaultHost->getDocumentRoot());
			sDocumentRoot = theApplication->getAppConfPath();
			while (!sDocumentRoot.empty() && sDocumentRootTemp.size()>4 && sDocumentRootTemp.substr(0,3)=="../")	// 处理相对路径
			{
				std::string::size_type find = sDocumentRoot.rfind("/HttpServer");
				if (find==std::string::npos)
					find = sDocumentRoot.rfind("\\HttpServer");
				if (find==std::string::npos)
					break;
				if (find==0)
					sDocumentRoot = "";
				else
					sDocumentRoot = sDocumentRoot.substr(0,find);
				sDocumentRootTemp = sDocumentRootTemp.substr(3);
			}
			sDocumentRoot.append("/");
			sDocumentRoot.append(sDocumentRootTemp);
			CGC_LOG((mycp::LOG_INFO, "%s\n",sDocumentRoot.c_str()));

			//sDocumentRoot = theApplication->getAppConfPath();
			//sDocumentRoot.append("/");
			//sDocumentRoot.append(theDefaultHost->getDocumentRoot());
			fs::path src_path2(sDocumentRoot.c_str());
			if (!fs::exists(src_path2))
			{
				CGC_LOG((mycp::LOG_ERROR, "DocumentRoot not exist. %s\n",sDocumentRoot.c_str()));
				return false;
			}

			theDefaultHost->setDocumentRoot(sDocumentRoot);
		}
	}
	theDefaultHost->m_bBuildDocumentRoot = true;

	theContentTypeInfoList.insert("ipa",CContentTypeInfo::create("application/application/vnd.iphone",true,true));
	theContentTypeInfoList.insert("apk",CContentTypeInfo::create("application/vnd.android.package-archive",true,true));
	theContentTypeInfoList.insert("ai",CContentTypeInfo::create("application/postscript",true,true));
	theContentTypeInfoList.insert("eps",CContentTypeInfo::create("application/postscript",true,true));
	theContentTypeInfoList.insert("exe",CContentTypeInfo::create("application/octet-stream",true,true));
	theContentTypeInfoList.insert("hlp",CContentTypeInfo::create("application/mshelp",true,true));
	theContentTypeInfoList.insert("chm",CContentTypeInfo::create("application/mshelp",true,true));
	theContentTypeInfoList.insert("doc",CContentTypeInfo::create("application/msword",true,true));	// application/vnd.ms-word
	theContentTypeInfoList.insert("dotx",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.wordprocessingml.template",true,true));
	theContentTypeInfoList.insert("docm",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.wordprocessingml.document",true,true));
	theContentTypeInfoList.insert("docx",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.wordprocessingml.document",true,true));
	theContentTypeInfoList.insert("odt",CContentTypeInfo::create("application/vnd.oasis.opendocument.text",true,true));
	theContentTypeInfoList.insert("xls",CContentTypeInfo::create("application/vnd.ms-excel",true,true));
	theContentTypeInfoList.insert("xla",CContentTypeInfo::create("application/vnd.ms-excel",true,true));
	theContentTypeInfoList.insert("xltx",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.spreadsheetml.template",true,true));
	theContentTypeInfoList.insert("xlsm",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",true,true));
	theContentTypeInfoList.insert("xlsx",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",true,true));
	theContentTypeInfoList.insert("ppt",CContentTypeInfo::create("application/vnd.ms-powerpoint",true,true));
	theContentTypeInfoList.insert("potx",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.presentationml.template",true,true));
	theContentTypeInfoList.insert("pptm",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.presentationml.presentation",true,true));
	theContentTypeInfoList.insert("ppsx",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.presentationml.slideshow",true,true));
	theContentTypeInfoList.insert("pptx",CContentTypeInfo::create("application/vnd.openxmlformats-officedocument.presentationml.presentation",true,true));
	theContentTypeInfoList.insert("pps",CContentTypeInfo::create("application/vnd.ms-powerpoint",true,true));
	theContentTypeInfoList.insert("pdf",CContentTypeInfo::create("application/pdf",false,true));
	theContentTypeInfoList.insert("xml",CContentTypeInfo::create("application/xml",false));
	theContentTypeInfoList.insert("swf",CContentTypeInfo::create("application/x-shockwave-flash",false,true));
	theContentTypeInfoList.insert("cab",CContentTypeInfo::create("application/x-shockwave-flash",false,true));
	// archives
	theContentTypeInfoList.insert("gz",CContentTypeInfo::create("application/x-gzip",true,true));
	theContentTypeInfoList.insert("tgz",CContentTypeInfo::create("application/x-gzip",true,true));
	theContentTypeInfoList.insert("bz",CContentTypeInfo::create("application/x-bzip2",true,true));
	theContentTypeInfoList.insert("bz",CContentTypeInfo::create("application/x-bzip2",true,true));
	theContentTypeInfoList.insert("tbz",CContentTypeInfo::create("application/x-bzip2",true,true));
	theContentTypeInfoList.insert("zip",CContentTypeInfo::create("application/zip",true,true));
	theContentTypeInfoList.insert("rar",CContentTypeInfo::create("application/x-rar",true,true));
	theContentTypeInfoList.insert("tar",CContentTypeInfo::create("application/x-tar",true,true));
	theContentTypeInfoList.insert("7z",CContentTypeInfo::create("application/x-7z-compressed",true,true));
	// texts
	theContentTypeInfoList.insert("csp",CContentTypeInfo::create("text/csp",false,false,SCRIPT_FILE_TYPE_CSP));
	theContentTypeInfoList.insert("txt",CContentTypeInfo::create("text/plain",false));
	theContentTypeInfoList.insert("php",CContentTypeInfo::create("text/x-php",false,false,SCRIPT_FILE_TYPE_PHP));
	theContentTypeInfoList.insert("shtml",CContentTypeInfo::create("text/html",false));
	theContentTypeInfoList.insert("html",CContentTypeInfo::create("text/html",false));
	theContentTypeInfoList.insert("htm",CContentTypeInfo::create("text/html",false));
	theContentTypeInfoList.insert("js",CContentTypeInfo::create("text/javascript",false));
	theContentTypeInfoList.insert("css",CContentTypeInfo::create("text/css",false));
	theContentTypeInfoList.insert("rtf",CContentTypeInfo::create("text/rtf",true));
	theContentTypeInfoList.insert("rtfd",CContentTypeInfo::create("text/rtfd",true));
	theContentTypeInfoList.insert("py",CContentTypeInfo::create("text/x-python",false));
	theContentTypeInfoList.insert("java",CContentTypeInfo::create("text/x-java-source",false));
	theContentTypeInfoList.insert("rb",CContentTypeInfo::create("text/x-ruby",false));
	theContentTypeInfoList.insert("sh",CContentTypeInfo::create("text/x-shellscript",false));
	theContentTypeInfoList.insert("pl",CContentTypeInfo::create("text/x-perl",false));
	theContentTypeInfoList.insert("sql",CContentTypeInfo::create("text/x-sql",false));
	// images
	theContentTypeInfoList.insert("bmp",CContentTypeInfo::create("image/x-ms-bmp",false,true));
	theContentTypeInfoList.insert("jpg",CContentTypeInfo::create("image/jpeg",false,true));
	theContentTypeInfoList.insert("jpeg",CContentTypeInfo::create("image/jpeg",false,true));
	theContentTypeInfoList.insert("gif",CContentTypeInfo::create("image/gif",false,true));
	theContentTypeInfoList.insert("png",CContentTypeInfo::create("image/png",false,true));
	theContentTypeInfoList.insert("tif",CContentTypeInfo::create("image/tiff",false,true));
	theContentTypeInfoList.insert("tiff",CContentTypeInfo::create("image/tiff",false,true));
	theContentTypeInfoList.insert("tga",CContentTypeInfo::create("image/x-targa",true,true));
	//audio
	theContentTypeInfoList.insert("mp3",CContentTypeInfo::create("audio/mpeg",false,true));
	theContentTypeInfoList.insert("mid",CContentTypeInfo::create("audio/midi",false,true));
	theContentTypeInfoList.insert("ogg",CContentTypeInfo::create("audio/ogg",false,true));
	theContentTypeInfoList.insert("mp4a",CContentTypeInfo::create("audio/mp4",false,true));
	theContentTypeInfoList.insert("wav",CContentTypeInfo::create("audio/wav",false,true));
	theContentTypeInfoList.insert("wma",CContentTypeInfo::create("audio/x-ms-wma",false,true));
	// video
	theContentTypeInfoList.insert("avi",CContentTypeInfo::create("video/x-msvideo",true,true));
	theContentTypeInfoList.insert("dv",CContentTypeInfo::create("video/x-dv",true,true));
	theContentTypeInfoList.insert("mp4",CContentTypeInfo::create("video/mp4",true,true));
	theContentTypeInfoList.insert("mpeg",CContentTypeInfo::create("video/mpeg",true,true));
	theContentTypeInfoList.insert("mpg",CContentTypeInfo::create("video/mpeg",true,true));
	theContentTypeInfoList.insert("mov",CContentTypeInfo::create("video/quicktime",true,true));
	theContentTypeInfoList.insert("wm",CContentTypeInfo::create("video/x-ms-wmv",true,true));
	theContentTypeInfoList.insert("flv",CContentTypeInfo::create("video/x-flv",true,true));
	theContentTypeInfoList.insert("mkv",CContentTypeInfo::create("video/x-matroska",true,true));

	theContentTypeInfoList.insert("woff",CContentTypeInfo::create("application/x-font-woff",false));
	theContentTypeInfoList.insert("woff2",CContentTypeInfo::create("application/x-font-woff",false));
	theContentTypeInfoList.insert("ttf",CContentTypeInfo::create("application/octet-stream",false));
	theContentTypeInfoList.insert("eot",CContentTypeInfo::create("application/octet-stream",false));
	theContentTypeInfoList.insert("svg",CContentTypeInfo::create("image/svg+xml",false));

	theMaxSize = (unsigned int)theAppInitParameters->getParameterValue("max-download-size", 120)*1024*1024;
	//theFastcgiPHPServer = theAppInitParameters->getParameterValue("fast_cgi_php_server");
	theEnableDataSource = theAppInitParameters->getParameterValue("enable-datasource", (int)0)==1?true:false;
#ifdef USES_BODB_SCRIPT
	if (theEnableDataSource)
	{
		tstring cdbcDataSource = theAppInitParameters->getParameterValue("CDBCDataSource", "ds_httpserver");
		theCdbcService = theServiceManager->getCDBDService(cdbcDataSource);
		if (theCdbcService.get() == NULL)
		{
			CGC_LOG((mycp::LOG_ERROR, "HttpServer DataSource Error.\n"));
			return false;
		}
		// 
		char selectSql[512];
		int cdbcCookie = 0;
		theCdbcService->select("SELECT filename,filesize,lasttime FROM cspfileinfo_t", cdbcCookie);
		//printf("**** ret=%d,cookie=%d\n",ret,cdbcCookie);
		cgcValueInfo::pointer record = theCdbcService->first(cdbcCookie);
		while (record.get() != NULL)
		{
			//assert (record->getType() == cgcValueInfo::TYPE_VECTOR);
			//assert (record->size() == 3);

			tstring sfilename(record->getVector()[0]->getStr());
			sprintf(selectSql, "SELECT code FROM scriptitem_t WHERE filename='%s' AND length(code)=3 LIMIT 1", sfilename.c_str());
			if (theCdbcService->select(selectSql)==0)
			{
				theCdbcService->escape_string_out(sfilename);
				cgcValueInfo::pointer var_filesize = record->getVector()[1];
				cgcValueInfo::pointer var_lasttime = record->getVector()[2];
				CCSPFileInfo::pointer fileInfo = CSP_FILEINFO(sfilename, var_filesize->getIntValue(), var_lasttime->getBigIntValue());
				theFileInfos.insert(fileInfo->getFileName(), fileInfo, false);
			}else
			{
				sprintf(selectSql, "DELETE FROM scriptitem_t WHERE filename='%s'", sfilename.c_str());
				theCdbcService->execute(selectSql);
				sprintf(selectSql, "DELETE FROM cspfileinfo_t WHERE filename='%s')",sfilename.c_str());
				theCdbcService->execute(selectSql);
			}

			record = theCdbcService->next(cdbcCookie);
		}
		theCdbcService->reset(cdbcCookie);
	}
#endif

	theAppAttributes = theApplication->getAttributes(true);
	theTimerHandler = CHttpTimeHandler::create();
#if (USES_TIMER_HANDLER_POINTER==1)
	theApplication->SetTimer(TIMERID_1_SECOND, 1*1000, theTimerHandler.get());	// 1秒检查一次
#else
	theApplication->SetTimer(TIMERID_1_SECOND, 1*1000, theTimerHandler);	// 1秒检查一次
#endif

	return true;
}

extern "C" void CGC_API CGC_Module_Free2(MODULE_FREE_TYPE nFreeType)
//extern "C" void CGC_API CGC_Module_Free(void)
{
	theAppInitParameters.reset();
	cgcAttributes::pointer attributes = theApplication->getAttributes();
	if (attributes.get() != NULL)
	{
		// C++ APP
#ifdef USES_OBJECT_LIST_APP
		ObjectListPointer apps = attributes->getListAttributes((int)OBJECT_APP, false);
		if (apps.get()!=NULL && !apps->empty())
		{
			BoostReadLock rdlock(apps->mutex());
			CLockList<cgcObject::pointer>::const_iterator pIter = apps->begin();
			for (; pIter!=apps->end(); pIter++)
			{
				cgcValueInfo::pointer pVarApp = CGC_OBJECT_CAST<cgcValueInfo>(*pIter);
				if (pVarApp->getType() == cgcValueInfo::TYPE_OBJECT && pVarApp->getInt() == (int)OBJECT_APP)
				{
					theServiceManager->resetService(CGC_OBJECT_CAST<cgcServiceInterface>(pVarApp->getObject()));
				}
			}
		}
#else
		std::vector<cgcValueInfo::pointer> apps;
		attributes->getProperty((int)OBJECT_APP, apps);
		for (size_t i=0; i<apps.size(); i++)
		{
			cgcValueInfo::pointer var = apps[i];
			if (var->getType() == cgcValueInfo::TYPE_OBJECT && var->getInt() == (int)OBJECT_APP)
			{
				theServiceManager->resetService(CGC_OBJECT_CAST<cgcServiceInterface>(var->getObject()));
			}
		}
#endif

		// C++ CDBC
#ifdef USES_OBJECT_LIST_CDBC
		ObjectListPointer cdbcs = attributes->getListAttributes((int)OBJECT_CDBC, false);
		if (cdbcs.get()!=NULL && !cdbcs->empty())
		{
			BoostReadLock rdlock(cdbcs->mutex());
			CLockList<cgcObject::pointer>::const_iterator pIter = cdbcs->begin();
			for (; pIter!=apps->end(); pIter++)
			{
				cgcValueInfo::pointer pVarCdbc = CGC_OBJECT_CAST<cgcValueInfo>(*pIter);
				if (pVarCdbc->getType() == cgcValueInfo::TYPE_OBJECT && pVarCdbc->getInt() == (int)OBJECT_CDBC)
				{
					theServiceManager->resetService(CGC_OBJECT_CAST<cgcServiceInterface>(pVarCdbc->getObject()));
				}
			}
		}
#else
		std::vector<cgcValueInfo::pointer> cdbcs;
		attributes->getProperty((int)OBJECT_CDBC, cdbcs);
		for (size_t i=0; i<cdbcs.size(); i++)
		{
			cgcValueInfo::pointer var = cdbcs[i];
			if (var->getType() == cgcValueInfo::TYPE_OBJECT && var->getInt() == (int)OBJECT_CDBC)
			{
				theServiceManager->resetService(CGC_OBJECT_CAST<cgcServiceInterface>(var->getObject()));
			}
		}
#endif
		attributes->cleanAllPropertys();
	}
	theApplication->KillAllTimer();
	if (theAppAttributes.get()!=NULL)
	{
		theAppAttributes->clearAllAtrributes();
		theAppAttributes.reset();
	}

	theContentTypeInfoList.clear();
#ifdef USES_BODB_SCRIPT
	theServiceManager->retCDBDService(theCdbcService);
#endif
	theFileSystemService.reset();
	//theStringService.reset();
	theDefaultHost.reset();
	theVirtualHosts.clear();
	theApps.reset();
	theDataSources.reset();
	theFileScripts.clear();
	theFileInfos.clear();
	theIdleFileInfos.clear();
	theTimerHandler.reset();
	if (thePHPFastcgiInfo.get()!=NULL)
	{
#ifdef WIN32
		KillAllProcess("php-cgi.exe");
#else
		char lpszBuffer[256];
		sprintf(lpszBuffer,"ps -e|grep -v 'grep'|grep php-cgi | awk '{print $1}' | xargs kill -9");
		system(lpszBuffer);
#endif
		thePHPFastcgiInfo.reset();
	}
	//theApplication2.reset();
}

/*
application/postscript                *.ai *.eps *.ps         Adobe Postscript-Dateien 
application/x-httpd-php             *.php *.phtml         PHP-Dateien 
audio/basic                             *.au *.snd             Sound-Dateien 
audio/x-midi                            *.mid *.midi             MIDI-Dateien 
audio/x-mpeg                         *.mp2             MPEG-Dateien 
image/x-windowdump             *.xwd             X-Windows Dump 
video/mpeg                          *.mpeg *.mpg *.mpe         MPEG-Dateien 
video/vnd.rn-realvideo            *.rmvb              realplay-Dateien 
video/quicktime                     *.qt *.mov             Quicktime-Dateien 
video/vnd.vivo                       *viv *.vivo             Vivo-Dateien
*/

void GetScriptFileType(const tstring& filename, tstring& outMimeType,bool& pOutImageOrBinary,SCRIPT_FILE_TYPE& pOutScriptFileType)
{
	pOutScriptFileType = SCRIPT_FILE_TYPE_UNKNOWN;
	outMimeType = "application/octet-stream";
	//outMimeType = "text/html";
	std::string::size_type find = filename.rfind(".");
	if (find == std::string::npos)
	{
		//outMimeType = "application/octet-stream";
		return;
	}

	pOutImageOrBinary = true;
	tstring ext(filename.substr(find+1));
	std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
	CContentTypeInfo::pointer pContentTypeInfo;
	if (theContentTypeInfoList.find(ext,pContentTypeInfo))
	{
		pOutScriptFileType = pContentTypeInfo->m_nScriptFileType;
		outMimeType = pContentTypeInfo->m_sContentType;
		pOutImageOrBinary = pContentTypeInfo->m_bImageOrBinary;
		//pOutImageOrBinary = false;
	}else if (theAppInitParameters.get()!=NULL)
	{
		cgcParameter::pointer pParam = theAppInitParameters->getParameter(ext);
		if (pParam.get()!=NULL)
		{
			pOutScriptFileType = SCRIPT_FILE_TYPE_UNKNOWN;
			outMimeType = pParam->getStrValue();
			pOutImageOrBinary = false;
			theContentTypeInfoList.insert(ext,CContentTypeInfo::create(outMimeType,false,false));
		}
	}

	//if (ext == "csp")
	//{
	//	pOutScriptFileType = SCRIPT_FILE_TYPE_CSP;
	//	pOutImageOrBinary = false;
	//	return;
	//}else if (ext == "php")
	//{
	//	pOutScriptFileType = SCRIPT_FILE_TYPE_PHP;
	//	pOutImageOrBinary = false;
	//	return;
	//}else if (ext == "gif")
	//	outMimeType = "image/gif";
	//else if(ext == "jpeg" || ext == "jpg" || ext == "jpe")
	//	outMimeType = "image/jpeg";
	//else if(ext == "htm" || ext == "html" || ext == "shtml")
	//{
	//	pOutImageOrBinary = false;
	//	outMimeType = "text/html";
	//}else if(ext == "js")
	//{
	//	pOutImageOrBinary = false;
	//	outMimeType = "text/javascript";
	//}else if(ext == "css")
	//{
	//	pOutImageOrBinary = false;
	//	outMimeType = "text/css";
	//}else if(ext == "txt")
	//{
	//	pOutImageOrBinary = false;
	//	outMimeType = "text/plain";
	//}else if(ext == "xls" || ext == "xla")
	//	outMimeType = "application/msexcel";
	//else if(ext == "hlp" || ext == "chm")
	//	outMimeType = "application/mshelp";
	////else if(ext == "ppt" || ext == "ppz" || ext == "pps" || ext == "pot")
	////	outMimeType = "application/mspowerpoint";
	//else if(ext == "ppt")
	//	outMimeType = "application/vnd.ms-powerpoint";
	//else if(ext == "potx")
	//	outMimeType = "application/vnd.openxmlformats-officedocument.presentationml.template";
	//else if(ext == "pptm")
	//	outMimeType = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	//else if(ext == "pptm")
	//	outMimeType = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	//else if(ext == "doc" || ext == "dot")
	//	outMimeType = "application/msword";
	//else if(ext == "exe")
	//	outMimeType = "application/octet-stream";
	//else if(ext == "pdf")
	//	outMimeType = "application/pdf";
	//else if(ext == "rtf")
	//	outMimeType = "application/rtf";
	//else if(ext == "zip")
	//	outMimeType = "application/zip";
	//else if(ext == "jar")
	//	outMimeType = "application/java-archive";
	//else if(ext == "swf" || ext == "cab")
	//	outMimeType = "application//x-shockwave-flash";
	//else if(ext == "mp3")
	//	outMimeType = "audio/mpeg";
	//else if(ext == "wav")
	//	outMimeType = "audio/x-wav";
	//else
	//	pOutImageOrBinary = false;

	//return false;
}
#ifdef USES_BODB_SCRIPT
void insertScriptItem(const tstring& code, const tstring & sFileNameTemp, const CScriptItem::pointer& scriptItem, int sub = 0)
{
	std::string sValue(scriptItem->getValue());
	theCdbcService->escape_string_in(sValue);
	const int sqlsize = sValue.size()+1000;
	char * sql = new char[sqlsize];

	sprintf(sql, "INSERT INTO scriptitem_t (filename,code,sub,itemtype,object1,object2,id,scopy,name,property,type,value) VALUES('%s','%s',%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s')",
				 sFileNameTemp.c_str(), code.c_str(), sub,(int)scriptItem->getItemType(),(int)scriptItem->getOperateObject1(),(int)scriptItem->getOperateObject2(),
				 scriptItem->getId().c_str(), scriptItem->getScope().c_str(),scriptItem->getName().c_str(), scriptItem->getProperty().c_str(),scriptItem->getType().c_str(), sValue.c_str());
	theCdbcService->execute(sql);
	delete[] sql;

	char bufferCode[64];
	for (size_t i=0; i<scriptItem->m_subs.size(); i++)
	{
		CScriptItem::pointer subScriptItem = scriptItem->m_subs[i];
		sprintf(bufferCode, "%s1%03d", code.c_str(), i);
		insertScriptItem(bufferCode, sFileNameTemp, subScriptItem, 1);
	}
	for (size_t i=0; i<scriptItem->m_elseif.size(); i++)
	{
		CScriptItem::pointer subScriptItem = scriptItem->m_elseif[i];
		sprintf(bufferCode, "%s2%03d", code.c_str(), i);
		insertScriptItem(bufferCode, sFileNameTemp, subScriptItem, 2);
	}
	for (size_t i=0; i<scriptItem->m_else.size(); i++)
	{
		CScriptItem::pointer subScriptItem = scriptItem->m_else[i];
		sprintf(bufferCode, "%s3%03d", code.c_str(), i);
		insertScriptItem(bufferCode, sFileNameTemp, subScriptItem, 3);
	}
}
#endif

inline void SetExpiresCache(const cgcHttpResponse::pointer& response,time_t tRequestTime, bool bIsImageOrBinary)
{
	const int nExpireSecond = bIsImageOrBinary?(10*const_one_day_seconds):(2*const_one_day_seconds);	// 文本文件：2天；其他图片等10天；
	const time_t tExpiresTime = tRequestTime + nExpireSecond;
	struct tm * tmExpiresTime = gmtime(&tExpiresTime);
	char lpszBuffer[64];
	strftime(lpszBuffer, 64, "%a, %d %b %Y %H:%M:%S GMT", tmExpiresTime);	// Tue, 19 Aug 2015 07:00:19 GMT
	response->setHeader("Expires",lpszBuffer);
	sprintf(lpszBuffer, "max-age=%d", nExpireSecond);
	response->setHeader("Cache-Control",lpszBuffer);
}
const unsigned int theOneMB = 1024*1024;
inline bool ReadFileDataToResponse(FILE * f, mycp::bigint nFileSize, const cgcHttpResponse::pointer& response)
{
	const unsigned int nPackSize = (unsigned int)(nFileSize>theOneMB?theOneMB:nFileSize);
	char * lpszBuffer = new char[nPackSize+1];
	if (lpszBuffer==NULL) return false;
	mycp::bigint nReadIndex = 0;
	while (true)
	{
		const size_t nOut = min(nPackSize,nFileSize-nReadIndex);
		if (nOut==0) break;
		nReadIndex += nOut;
		const size_t nReadSize = fread(lpszBuffer,1,nOut,f);
		if (nReadSize==0) break;
		response->writeData((const char*)lpszBuffer, nReadSize);
	}
	delete[] lpszBuffer;
	return true;
}
inline bool GetFileMd5(FILE* f,mycp::bigint& pOutFileSize,tstring& pOutFileMd5)
{
	if (f == NULL)
	{
		return false;
	}
#ifdef WIN32
	_fseeki64(f, 0, SEEK_END);
	pOutFileSize = _ftelli64(f);
#else
	fseeko(f, 0, SEEK_END);
	pOutFileSize = ftello(f);
#endif
	// 获取文件MD5
	MD5 md5;
	const unsigned int nPackSize = (unsigned int)(pOutFileSize>theOneMB?theOneMB:pOutFileSize);
	unsigned char * lpszBuffer = new unsigned char[nPackSize+1];
#ifdef WIN32
	_fseeki64(f, 0, SEEK_SET);
#else
	fseeko(f, 0, SEEK_SET);
#endif
	while (true)
	{
		const size_t nReadSize = fread(lpszBuffer,1,nPackSize,f);
		if (nReadSize==0)
			break;
		md5.update(lpszBuffer, (unsigned int)nReadSize);
	}
	md5.finalize();
	pOutFileMd5 = md5.hex_digest();
	delete[] lpszBuffer;
	return true;
}
inline const tstring & replace(tstring & strSource, const tstring & strFind, const tstring &strReplace)
{
	std::string::size_type pos=0;
	std::string::size_type findlen=strFind.size();
	std::string::size_type replacelen=strReplace.size();
	while ((pos=strSource.find(strFind, pos)) != std::string::npos)
	{
		strSource.replace(pos, findlen, strReplace);
		pos += replacelen;
	}
	return strSource;
}
extern "C" HTTP_STATUSCODE CGC_API doHttpServer(const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
{
	HTTP_STATUSCODE statusCode = STATUS_CODE_200;
	const tstring sIfModifiedSince(request->getHeader("if-modified-since"));	// for Last-Modified
	const tstring sIfNoneMatch(request->getHeader("if-none-match"));			// for ETag
	// If-None-Match,ETag
	//const tstring sIfRange = request->getHeader("If-Range");
	//printf("************** If-Modified-Since: %s\n",sIfModifiedSince.c_str());
	//printf("************** If-Range: %s\n",sIfRange.c_str());

	const tstring host(request->getHost());
	//printf("**** host=%s\n",host.c_str());
	CVirtualHost::pointer requestHost = theVirtualHosts.getVirtualHost(host);
	if (requestHost.get() == NULL && theVirtualHosts.getHosts().size() > 1)
	{
		std::string::size_type find = host.find(":");
		if (find != std::string::npos)
		{
			tstring address = host.substr(0, find);
			address.append(":*");
			requestHost = theVirtualHosts.getVirtualHost(address);
			if (requestHost.get() == NULL)
			{
				address = "*";
				address.append(host.substr(find));
				requestHost = theVirtualHosts.getVirtualHost(address);
			}
		}

		if (requestHost.get() != NULL)
		{
			requestHost->setPropertys(theApplication->createAttributes());
			theVirtualHosts.addVirtualHst(host, requestHost);
		}
	}
	if (requestHost.get() != NULL && !requestHost->m_bBuildDocumentRoot)
	{
		if (requestHost->getDocumentRoot().empty())
			requestHost->setDocumentRoot(theApplication->getAppConfPath());
		else
		{
			tstring sDocumentRoot(requestHost->getDocumentRoot());
			namespace fs = boost::filesystem;
			fs::path src_path(sDocumentRoot.c_str());
			if (!fs::exists(src_path))
			{
				tstring sDocumentRootTemp(requestHost->getDocumentRoot());
				sDocumentRoot = theApplication->getAppConfPath();
				while (!sDocumentRoot.empty() && sDocumentRootTemp.size()>4 && sDocumentRootTemp.substr(0,3)=="../")	// 处理相对路径
				{
					std::string::size_type find = sDocumentRoot.rfind("/HttpServer");
					if (find==std::string::npos)
						find = sDocumentRoot.rfind("\\HttpServer");
					if (find==std::string::npos)
						break;
					if (find==0)
						sDocumentRoot = "";
					else
						sDocumentRoot = sDocumentRoot.substr(0,find);
					sDocumentRootTemp = sDocumentRootTemp.substr(3);
				}
				sDocumentRoot.append("/");
				sDocumentRoot.append(sDocumentRootTemp);
				//printf("**** sDocumentRoot=%s\n",sDocumentRoot.c_str());
				//sDocumentRoot = theApplication->getAppConfPath();
				//sDocumentRoot.append("/");
				//sDocumentRoot.append(requestHost->getDocumentRoot());
				fs::path src_path2(sDocumentRoot.c_str());
				if (!fs::exists(src_path2))
				{
					requestHost.reset();
					CGC_LOG((mycp::LOG_ERROR, "DocumentRoot not exist. %s\n",sDocumentRoot.c_str()));
				}else
					requestHost->setDocumentRoot(sDocumentRoot);
			}
		}
		if (requestHost.get() != NULL)
			requestHost->m_bBuildDocumentRoot = true;
	}

	if (requestHost.get() == NULL)
		requestHost = theDefaultHost;

	const tstring& sModuleName = request->getModuleName();
	const tstring& sFunctionName = request->getFunctionName();
	tstring sFileName;
	if (sModuleName.empty() || sFunctionName.empty())
	{
		sFileName = request->getRequestURI();
		//printf("**** sFileName=%s\n",sFileName.c_str());
		if (!request->isFrowardFrom())
		{
			mycp::replace_string(sFileName,"../","");
		}
		if (sFileName == "/")
			sFileName.append(requestHost->getIndex());
		else if (sFileName.substr(0,1) != "/")
			sFileName.insert(0, "/");
	}else
	{
		// ** /rest/v03/modulename/func.php
		// ** /rest/v03/modulename/path/func.php
		sFileName = "/rest/";
		if (!request->getRestVersion().empty())
		{
			sFileName.append(request->getRestVersion());
			sFileName.append("/");
		}
		sFileName.append(sModuleName);
		sFileName.append("/");
		sFileName.append(sFunctionName);
	}

	//tstring sFileName(request->getRequestURI());
	//printf("**** FileName=%s\n",sFileName.c_str());
	//if (sFileName == "/")
	//	sFileName.append(requestHost->getIndex());
	//else if (sFileName.substr(0,1) != "/")
	//	sFileName.insert(0, "/");
	tstring sMimeType;
	bool bIsImageOrBinary = false;
	SCRIPT_FILE_TYPE nScriptFileType = SCRIPT_FILE_TYPE_UNKNOWN;
	GetScriptFileType(sFileName,sMimeType,bIsImageOrBinary,nScriptFileType);

	// File not exist
	tstring sFilePath(requestHost->getDocumentRoot());
	sFilePath.append(sFileName);
	//printf("**** FilePath=%s,sFileName=%s\n",sFilePath.c_str(),sFileName.c_str());
	namespace fs = boost::filesystem;
	fs::path src_path(sFilePath.c_str());
	if (!fs::exists(src_path))
	{
		if (nScriptFileType==SCRIPT_FILE_TYPE_PHP && thePHPFastcgiInfo.get()!=NULL)
		{
			sFileName = thePHPFastcgiInfo->m_sFastcgiIndex;
			sFilePath = requestHost->getDocumentRoot();
			sFilePath.append("/");
			sFilePath.append(thePHPFastcgiInfo->m_sFastcgiIndex);
			src_path = fs::path(sFilePath.c_str());
		}else if (nScriptFileType==SCRIPT_FILE_TYPE_CSP)
		{
			sFileName = requestHost->getIndex();
			sFilePath = requestHost->getDocumentRoot();
			sFilePath.append("/");
			sFilePath.append(requestHost->getIndex());
			src_path = fs::path(sFilePath.c_str());
			if (!fs::exists(src_path))
			{
				response->println("HTTP Status 404 - %s", sFileName.c_str());
				return STATUS_CODE_404;
			}
		}else
		{
			response->println("HTTP Status 404 - %s", sFileName.c_str());
			return STATUS_CODE_404;
		}

	}else if (fs::is_directory(src_path))
	{
		// ??
		if (nScriptFileType==SCRIPT_FILE_TYPE_PHP && thePHPFastcgiInfo.get()!=NULL)
		{
			sFilePath.append(thePHPFastcgiInfo->m_sFastcgiIndex);
			src_path = fs::path(sFilePath.c_str());
		}else
		{
			response->println("HTTP Status 404 - %s", sFileName.c_str());
			return STATUS_CODE_404;
		}
	}

	//printf(" **** doHttpServer: filename=%s\n",sFileName.c_str());
	if (nScriptFileType==SCRIPT_FILE_TYPE_CSP)
	{
		//fs::path src_path(sFilePath);
		const size_t fileSize = (size_t)fs::file_size(src_path);
		const time_t lastTime = fs::last_write_time(src_path);

		bool buildCSPFile = false;
		CCSPFileInfo::pointer fileInfo;
		if (!theFileInfos.find(sFileName, fileInfo))
		{
			if (!theIdleFileInfos.find(sFileName, fileInfo,true))
			{
				buildCSPFile = true;
				fileInfo = CCSPFileInfo::pointer(new CCSPFileInfo(sFileName, fileSize, lastTime));
#ifdef USES_BODB_SCRIPT
				if (theCdbcService.get()!=NULL)
				{
					tstring sFileNameTemp(sFileName);
					theCdbcService->escape_string_in(sFileNameTemp);
					char sql[512];
					sprintf(sql, "INSERT INTO cspfileinfo_t (filename,filesize,lasttime) VALUES('%s',%d,%lld)",sFileNameTemp.c_str(), fileSize, (mycp::bigint)lastTime);
					theCdbcService->execute(sql);
				}
#endif
			}
			theFileInfos.insert(sFileName,fileInfo,false);
		}
		if (!buildCSPFile && fileInfo->isModified(fileSize, lastTime))
		{
			buildCSPFile = true;
			fileInfo->setFileSize(fileSize);
			fileInfo->setLastTime(lastTime);

#ifdef USES_BODB_SCRIPT
			if (theCdbcService.get()!=NULL)
			{
				tstring sFileNameTemp(sFileName);
				theCdbcService->escape_string_in(sFileNameTemp);
				char sql[512];
				sprintf(sql, "UPDATE cspfileinfo_t SET filesize=%d,lasttime=%lld WHERE filename='%s'",fileSize, (mycp::bigint)lastTime, sFileNameTemp.c_str());
				theCdbcService->execute(sql);
			}
#endif
		}
		fileInfo->m_tRequestTime = theTimerHandler->m_tNow;		// ***记录最新时间，用于定时清空太久没用资源

		CFileScripts::pointer fileScript;
		if (!theFileScripts.find(sFileName, fileScript))
		{
			buildCSPFile = true;
			fileScript = CFileScripts::pointer(new CFileScripts(sFileName));
			theFileScripts.insert(sFileName, fileScript,false);
		}
		CMycpHttpServer::pointer httpServer = CMycpHttpServer::pointer(new CMycpHttpServer(request, response));
		httpServer->setSystem(theSystem);
		httpServer->setApplication(theApplication);
		httpServer->setFileSystemService(theFileSystemService);
		httpServer->setServiceManager(theServiceManager);
		httpServer->setVirtualHost(requestHost);
		httpServer->setServletName(sFileName.substr(1));
		httpServer->setApps(theApps);
		httpServer->setDSs(theDataSources);

		//printf("**** buildCSPFile=%d\n",(int)(buildCSPFile?1:0));
		if (buildCSPFile)
		{
			if (!fileScript->parserCSPFile(sFilePath.c_str())) return STATUS_CODE_404;

#ifdef USES_BODB_SCRIPT
			if (theCdbcService.get()!=NULL)
			{
				char sql[512];
				tstring sFileNameTemp(sFileName);
				theCdbcService->escape_string_in(sFileNameTemp);
				sprintf(sql, "DELETE FROM scriptitem_t WHERE filename='%s'", sFileNameTemp.c_str());
				theCdbcService->execute(sql);

				const std::vector<CScriptItem::pointer>& scripts = fileScript->getScripts();
				//printf("**** buildCSPFile filename=%s, size=%d\n",sFileName.c_str(),(int)scripts.size());
				char bufferCode[5];
				for (size_t i=0; i<scripts.size(); i++)
				{
					CScriptItem::pointer scriptItem = scripts[i];
					sprintf(bufferCode, "%04d", i);
					insertScriptItem(bufferCode, sFileNameTemp, scriptItem);
				}
			}
		}else if (fileScript->empty() && theCdbcService.get()!=NULL)
		{
			char selectSql[512];
			tstring sFileNameTemp(sFileName);
			theCdbcService->escape_string_in(sFileNameTemp);
			sprintf(selectSql, "SELECT code,sub,itemtype,object1,object2,id,scopy,name,property,type,value FROM scriptitem_t WHERE filename='%s' ORDER BY code", sFileNameTemp.c_str());

			int cdbcCookie = 0;
			//theCdbcService->select(selectSql, cdbcCookie);
			const mycp::bigint ret = theCdbcService->select(selectSql, cdbcCookie);
			//printf("**** %lld=%s\n",ret,selectSql);
			CLockMap<tstring, CScriptItem::pointer> codeScripts;
			cgcValueInfo::pointer record = theCdbcService->first(cdbcCookie);
			while (record.get() != NULL)
			{
				//assert (record->getType() == cgcValueInfo::TYPE_VECTOR);
				//assert (record->size() == 11);
				cgcValueInfo::pointer var_code = record->getVector()[0];
				cgcValueInfo::pointer var_sub = record->getVector()[1];
				cgcValueInfo::pointer var_itemtype = record->getVector()[2];
				cgcValueInfo::pointer var_object1 = record->getVector()[3];
				cgcValueInfo::pointer var_object2 = record->getVector()[4];
				cgcValueInfo::pointer var_id = record->getVector()[5];
				cgcValueInfo::pointer var_scope = record->getVector()[6];
				cgcValueInfo::pointer var_name = record->getVector()[7];
				cgcValueInfo::pointer var_property = record->getVector()[8];
				cgcValueInfo::pointer var_type = record->getVector()[9];
				cgcValueInfo::pointer var_value = record->getVector()[10];
				//const int nvlen = record->getVector()[11]->getIntValue();

				CScriptItem::pointer scriptItem = CScriptItem::pointer(new CScriptItem((CScriptItem::ItemType)var_itemtype->getIntValue()));
				scriptItem->setOperateObject1((CScriptItem::OperateObject)var_object1->getIntValue());
				scriptItem->setOperateObject2((CScriptItem::OperateObject)var_object2->getIntValue());
				scriptItem->setId(var_id->getStr());
				scriptItem->setScope(var_scope->getStr());
				scriptItem->setName(var_name->getStr());
				scriptItem->setProperty(var_property->getStr());
				scriptItem->setType(var_type->getStr());

				std::string sValue(var_value->getStr());
				if (!sValue.empty())
				{
					theCdbcService->escape_string_out(sValue);
					scriptItem->setValue(sValue);
				}
				const tstring scriptCode(var_code->getStr());
				const size_t nScriptCodeSize = scriptCode.size();
				const tstring parentCode(nScriptCodeSize<=4?"":scriptCode.substr(0, nScriptCodeSize-4));
				//printf("**** code=%s;parent_code=%s;value=%s\n",scriptCode.c_str(),parentCode.c_str(),sValue.c_str());
				CScriptItem::pointer parentScriptItem;
				if (!parentCode.empty() && codeScripts.find(parentCode, parentScriptItem))
				{
					const int nSub = var_sub->getIntValue();
					if (nSub == 2)
						parentScriptItem->m_elseif.push_back(scriptItem);
					else if (nSub == 3)
						parentScriptItem->m_else.push_back(scriptItem);
					else
						parentScriptItem->m_subs.push_back(scriptItem);
				}else
				{
					fileScript->addScript(scriptItem);
				}
				codeScripts.insert(scriptCode, scriptItem, false);

				record = theCdbcService->next(cdbcCookie);
			}
			theCdbcService->reset(cdbcCookie);
			if (fileScript->empty())
				fileInfo->setFileSize(0);
#endif
		}

		try
		{
			//printf("**** doIt filename=%s, size=%d\n",sFileName.c_str(),(int)fileScript->getScripts().size());
			const int ret = httpServer->doIt(fileScript);
			if (ret == -1)
				return STATUS_CODE_400;
			statusCode = response->getStatusCode();
		}catch(std::exception const &e)
		{
			theApplication->log(LOG_ERROR, _T("exception, RequestURL \'%s\', lasterror=0x%x\n"), request->getRequestURI().c_str(), GetLastError());
			theApplication->log(LOG_ERROR, _T("'%s'\n"), e.what());
			response->println("EXCEPTION: LastError=%d; %s", e.what());
			return STATUS_CODE_500;
		}catch(...)
		{
			theApplication->log(LOG_ERROR, _T("exception, RequestURL \'%s\', lasterror=0x%x\n"),request->getRequestURI().c_str(), GetLastError());
			response->println("EXCEPTION: LastError=%d", GetLastError());
			return STATUS_CODE_500;
		}
		return statusCode;
	}
	
	if (nScriptFileType==SCRIPT_FILE_TYPE_PHP)
	{
		if (thePHPFastcgiInfo.get()==0 || thePHPFastcgiInfo->m_sFastcgiPass.empty()) {
			response->println("PHP fastcgi setting error.");
			CGC_LOG((mycp::LOG_ERROR, "PHP fastcgi setting error.\n"));
			return STATUS_CODE_501;
		}

		CFastcgiRequestInfo::pointer pFastcgiRequestInfo;
		for (int i=0;i<10; i++) {
			pFastcgiRequestInfo = theTimerHandler->GetFastcgiRequestInfo(nScriptFileType,response);
			if (pFastcgiRequestInfo.get()!=0) {
				pFastcgiRequestInfo->SetResponseState(REQUEST_INFO_STATE_WAITTING_RESPONSE);
				break;
			}
#ifdef WIN32
			Sleep(500);
#else
			usleep(500000);
#endif
		}
		if (pFastcgiRequestInfo.get()==0) {
			response->println("PHP fastcgi busy.");
			CGC_LOG((mycp::LOG_ERROR, "PHP fastcgi busy.\n"));
			return STATUS_CODE_501;
		}

		const int nRequestId = pFastcgiRequestInfo->GetRequestId();
		//CGC_LOG((mycp::LOG_TRACE, "RequestId=%d waitting...(%s)\n", nRequestId,sFileName.c_str()));
#ifdef USES_FASTCGI_KEEP_CONN
		mycp::httpserver::CgcTcpClient::pointer m_pFastCgiServer = pFastcgiRequestInfo->GetRequestPassInfo()->m_pFastCgiServer;
#else
		mycp::httpserver::CgcTcpClient::pointer m_pFastCgiServer = pFastcgiRequestInfo->m_pFastCgiServer;
#endif
		if (m_pFastCgiServer.get()!=0 && m_pFastCgiServer->IsDisconnection()) {
			m_pFastCgiServer->stopClient();
			m_pFastCgiServer.reset();
		}
		
		if (m_pFastCgiServer.get()==0) {
			m_pFastCgiServer = mycp::httpserver::CgcTcpClient::create(theTimerHandler.get(),nRequestId);
			//m_pFastCgiServer->SetIoService(theApplication2->getIoService(), false);
			if (m_pFastCgiServer->startClient(pFastcgiRequestInfo->GetFastcgiPass().c_str(),0)!=0 || m_pFastCgiServer->IsDisconnection()) {
				bool bConnectError = true;
				const CRequestPassInfo::pointer& pRequestPassInfo = pFastcgiRequestInfo->GetRequestPassInfo();
				if (pRequestPassInfo->GetProcessId()>0) {
					//printf("**** restart fastcgi\n");
					CGC_LOG((mycp::LOG_ERROR, "restart fastcgi (%s)\n", pFastcgiRequestInfo->GetFastcgiPass().c_str()));
					pRequestPassInfo->KillProcess();
					theTimerHandler->CreateRequestPassInfoProcess(pRequestPassInfo);
					m_pFastCgiServer->stopClient();
#ifdef WIN32
					Sleep(1000);
#else
					usleep(1000000);
#endif
					m_pFastCgiServer = mycp::httpserver::CgcTcpClient::create(theTimerHandler.get(),nRequestId);
					bConnectError = (m_pFastCgiServer->startClient(pFastcgiRequestInfo->GetFastcgiPass().c_str(),0)!=0 || m_pFastCgiServer->IsDisconnection());
					CGC_LOG((mycp::LOG_ERROR, "restart fastcgi (%s) ok=%d\n", pFastcgiRequestInfo->GetFastcgiPass().c_str(),(int)(bConnectError?0:1)));
				}
				if (bConnectError) {
					m_pFastCgiServer->stopClient();
					theTimerHandler->RemoveRequestInfo(pFastcgiRequestInfo);
					response->println("PHP fastcgi connect error, FastcgiPass=%s",thePHPFastcgiInfo->m_sFastcgiPass.c_str());
					CGC_LOG((mycp::LOG_ERROR, "PHP fastcgi connect error (%s)\n", pFastcgiRequestInfo->GetFastcgiPass().c_str()));
					return STATUS_CODE_501;
				}
			}
#ifdef USES_FASTCGI_KEEP_CONN
			pFastcgiRequestInfo->GetRequestPassInfo()->m_pFastCgiServer = m_pFastCgiServer;
#else
			pFastcgiRequestInfo->m_pFastCgiServer = m_pFastCgiServer;
#endif
		}

		FCGI_BeginRequestRecord pBeginRequestRecord;
		memset(&pBeginRequestRecord,0,sizeof(pBeginRequestRecord));
		pBeginRequestRecord.header = MakeHeader(FCGI_BEGIN_REQUEST,nRequestId,FCGI_BEGIN_REQUEST_BODY_LEN, 0);
		const int nRole = FCGI_RESPONDER;
		pBeginRequestRecord.body.roleB1  = (unsigned char) ((nRole >> 8) & 0xff);
		pBeginRequestRecord.body.roleB0  = (unsigned char) ((nRole     ) & 0xff);
#ifdef USES_FASTCGI_KEEP_CONN
		pBeginRequestRecord.body.flags  = FCGI_KEEP_CONN;
#endif
		//m_pFastCgiServer->sendData((const unsigned char*)&pBeginRequestRecord,sizeof(pBeginRequestRecord));
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,(const unsigned char*)&pBeginRequestRecord,sizeof(pBeginRequestRecord));
		unsigned char * lpszSendBuffer = pFastcgiRequestInfo->GetBuffer();
		char lpszIntBuf[12];

		std::string sParam1;
		std::string sValue1;
		int nNameValue1Size = 0;
		{
			//printf("******** FASTCGI_PARAM_SCRIPT_FILENAME=%s\n",sFilePath.c_str());
			FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_SCRIPT_FILENAME.c_str(),FASTCGI_PARAM_SCRIPT_FILENAME.size(),sFilePath.c_str(),sFilePath.size(),&nNameValue1Size,lpszSendBuffer);
			//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		}
		const tstring sHttpCookie(request->getHeader(Http_Cookie,""));
		//printf("******** sHttpCookie=%s\n",sHttpCookie.c_str());
		if (!sHttpCookie.empty()) {
			FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_HTTP_COOKIE.c_str(),FASTCGI_PARAM_HTTP_COOKIE.size(),sHttpCookie.c_str(),sHttpCookie.size(),&nNameValue1Size,lpszSendBuffer);
			//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		}
		const tstring sHttpUserAgent(request->getHeader(Http_UserAgent,""));
		//printf("******** sHttpUserAgent=%s\n",sHttpUserAgent.c_str());
		if (!sHttpUserAgent.empty()) {
			FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_HTTP_USER_AGENT.c_str(),FASTCGI_PARAM_HTTP_USER_AGENT.size(),sHttpUserAgent.c_str(),sHttpUserAgent.size(),&nNameValue1Size,lpszSendBuffer);
			//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		}
		{
			//printf("******** getRemoteAddr=%s\n",request->getRemoteAddr().c_str());
			mycp::CCgcAddress pAddress(request->getRemoteAddr());
			{
				sValue1 = pAddress.getip().c_str();
				FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_REMOTE_ADDR.c_str(),FASTCGI_PARAM_REMOTE_ADDR.size(),sValue1.c_str(),sValue1.size(),&nNameValue1Size,lpszSendBuffer);
				//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
			}
			{
				sprintf(lpszIntBuf,"%d",pAddress.getport());
				FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_REMOTE_PORT.c_str(),FASTCGI_PARAM_REMOTE_PORT.size(),lpszIntBuf,strlen(lpszIntBuf),&nNameValue1Size,lpszSendBuffer);
				//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
			}
		}
		// FASTCGI_PARAM_HTTP_HOST
		const tstring sHttpHost(request->getHeader(Http_Host,""));
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_HTTP_HOST.c_str(),FASTCGI_PARAM_HTTP_HOST.size(),sHttpHost.c_str(),sHttpHost.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		// FASTCGI_PARAM_DOCUMENT_ROOT
		const tstring& sDocumentRoot = requestHost->getDocumentRoot();
		//printf("******** FASTCGI_PARAM_DOCUMENT_ROOT=%s\n",sDocumentRoot.c_str());
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_DOCUMENT_ROOT.c_str(),FASTCGI_PARAM_DOCUMENT_ROOT.size(),sDocumentRoot.c_str(),sDocumentRoot.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		//// FASTCGI_PARAM_PHP_SELF
		////const tstring& sPHPSelf = sFileName;//request->sFileName
		//printf("******** sPHPSelf=%s\n",sFileName.c_str());
		//FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_PHP_SELF.c_str(),FASTCGI_PARAM_PHP_SELF.size(),sFileName.c_str(),sFileName.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		// FASTCGI_PARAM_SCRIPT_NAME
		//printf("******** FASTCGI_PARAM_SCRIPT_NAME=%s\n",sFileName.c_str());
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_SCRIPT_NAME.c_str(),FASTCGI_PARAM_SCRIPT_NAME.size(),sFileName.c_str(),sFileName.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);

		// SERVER_ADDR
		const std::string& sServerAddr = request->getServerAddr();
		//printf("******** sServerAddr=%s\n",sServerAddr.c_str());
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_SERVER_ADDR.c_str(),FASTCGI_PARAM_SERVER_ADDR.size(),sServerAddr.c_str(),sServerAddr.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		// SERVER_PORT
		sprintf(lpszIntBuf,"%d",request->getServerPort());
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_SERVER_PORT.c_str(),FASTCGI_PARAM_SERVER_PORT.size(),lpszIntBuf,strlen(lpszIntBuf),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		// SERVER_NAME
		sValue1 = "MYCP";
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_SERVER_NAME.c_str(),FASTCGI_PARAM_SERVER_NAME.size(),sValue1.c_str(),sValue1.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		// SERVER_SOFTWARE
		sValue1 = "MYCP/2.1";
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_SERVER_SOFTWARE.c_str(),FASTCGI_PARAM_SERVER_SOFTWARE.size(),sValue1.c_str(),sValue1.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		// SERVER_PROTOCOL
		const tstring& sHttpVersion = request->getHttpVersion();
		//printf("******** sHttpVersion=%s\n",sHttpVersion.c_str());
		if (!sHttpVersion.empty()) {
			FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_SERVER_PROTOCOL.c_str(),FASTCGI_PARAM_SERVER_PROTOCOL.size(),sHttpVersion.c_str(),sHttpVersion.size(),&nNameValue1Size,lpszSendBuffer);
			//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		}

		//const std::string FASTCGI_PARAM_REQUEST_URL			= "REQUEST_URL";
		//const tstring& sRequestURL = request->getRequestURL();
		//printf("******** sRequestURL=%s\n",sRequestURL.c_str());
		//FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_REQUEST_URL.c_str(),FASTCGI_PARAM_REQUEST_URL.size(),sRequestURL.c_str(),sRequestURL.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);

		// REQUEST_URI
		//const tstring& sRequestURI = request->getRequestURI();
		////printf("******** sRequestURI=%s\n",sRequestURI.c_str());
		FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_REQUEST_URI.c_str(),FASTCGI_PARAM_REQUEST_URI.size(),sFileName.c_str(),sFileName.size(),&nNameValue1Size,lpszSendBuffer);
		//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		// QUERY_STRING
		const tstring& sHttpQueryString = request->getQueryString();
		if (request->getHttpMethod()==HTTP_GET && !sHttpQueryString.empty()) {
			//printf("******** sQueryString=%s\n",sHttpQueryString.c_str());
			unsigned char* pBufferTemp = (sHttpQueryString.size()+FCGI_HEADER_LEN+16)>DEFAULT_SEND_BUFFER_SIZE?NULL:lpszSendBuffer;
			pBufferTemp = FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_QUERY_STRING.c_str(),FASTCGI_PARAM_QUERY_STRING.size(),sHttpQueryString.c_str(),sHttpQueryString.size(),&nNameValue1Size,pBufferTemp);
			//m_pFastCgiServer->sendData((const unsigned char*)pBufferTemp,nNameValue1Size);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,pBufferTemp,nNameValue1Size);
			if (pBufferTemp!=lpszSendBuffer) {
				delete[] pBufferTemp;
			}
		}
		else if (request->getHttpMethod()==HTTP_POST) {
			if (request->getContentLength()==0) {	/// *content-length=0，表示没有POST 数据
				if (!sHttpQueryString.empty()) {
					//printf("******** sQueryString=%s\n",sHttpQueryString.c_str());
					unsigned char* pBufferTemp = (sHttpQueryString.size()+FCGI_HEADER_LEN+16)>DEFAULT_SEND_BUFFER_SIZE?NULL:lpszSendBuffer;
					pBufferTemp = FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_QUERY_STRING.c_str(),FASTCGI_PARAM_QUERY_STRING.size(),sHttpQueryString.c_str(),sHttpQueryString.size(),&nNameValue1Size,pBufferTemp);
					//m_pFastCgiServer->sendData((const unsigned char*)pBufferTemp,nNameValue1Size);
					pFastcgiRequestInfo->SendData(m_pFastCgiServer,pBufferTemp,nNameValue1Size);
					if (pBufferTemp!=lpszSendBuffer) {
						delete[] pBufferTemp;
					}
				}
			}
			else {	/// *content-length>0，POST 数据由下面处理，这里只处理URL ?后面的 "p=v&..."
				const tstring& sRequestURL = request->getRequestURL();
				const std::string::size_type find = sRequestURL.find("?");
				if (find!=std::string::npos) {
					const tstring sQueryString(sRequestURL.substr(find+1));
					if (!sQueryString.empty()) {
						//printf("******** sQueryString=%s\n",sQueryString.c_str());
						unsigned char* pBufferTemp = (sQueryString.size()+FCGI_HEADER_LEN+16)>DEFAULT_SEND_BUFFER_SIZE?NULL:lpszSendBuffer;
						pBufferTemp = FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_QUERY_STRING.c_str(),FASTCGI_PARAM_QUERY_STRING.size(),sQueryString.c_str(),sQueryString.size(),&nNameValue1Size,pBufferTemp);
						//m_pFastCgiServer->sendData((const unsigned char*)pBufferTemp,nNameValue1Size);
						pFastcgiRequestInfo->SendData(m_pFastCgiServer,pBufferTemp,nNameValue1Size);
						if (pBufferTemp!=lpszSendBuffer) {
							delete[] pBufferTemp;
						}
					}
				}
			}
			//const tstring& sRequestURL = request->getRequestURL();
			//const std::string::size_type find = sRequestURL.find("?");
			//if (find==std::string::npos)
			//{
			//	const tstring& sQueryString = request->getQueryString();
			//	if (!sQueryString.empty())
			//	{
			//		//printf("******** sQueryString=%s\n",sQueryString.c_str());
			//		unsigned char* pBufferTemp = (sQueryString.size()+FCGI_HEADER_LEN+16)>DEFAULT_SEND_BUFFER_SIZE?NULL:lpszSendBuffer;
			//		pBufferTemp = FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_QUERY_STRING.c_str(),FASTCGI_PARAM_QUERY_STRING.size(),sQueryString.c_str(),sQueryString.size(),&nNameValue1Size,pBufferTemp);
			//		m_pFastCgiServer->sendData((const unsigned char*)pBufferTemp,nNameValue1Size);
			//		if (pBufferTemp!=lpszSendBuffer)
			//			delete[] pBufferTemp;
			//	}
			//}else
			//{
			//	tstring sQueryString = request->getQueryString();
			//	if (!sQueryString.empty())
			//	{
			//		sQueryString.append("&");
			//	}
			//	sQueryString.append(sRequestURL.substr(find+1));
			//	if (!sQueryString.empty())
			//	{
			//		//printf("******** sQueryString=%s\n",sQueryString.c_str());
			//		unsigned char* pBufferTemp = (sQueryString.size()+FCGI_HEADER_LEN+16)>DEFAULT_SEND_BUFFER_SIZE?NULL:lpszSendBuffer;
			//		pBufferTemp = FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_QUERY_STRING.c_str(),FASTCGI_PARAM_QUERY_STRING.size(),sQueryString.c_str(),sQueryString.size(),&nNameValue1Size,pBufferTemp);
			//		m_pFastCgiServer->sendData((const unsigned char*)pBufferTemp,nNameValue1Size);
			//		if (pBufferTemp!=lpszSendBuffer)
			//			delete[] pBufferTemp;
			//	}
			//}
		}
		// REQUEST_METHOD
		bool bAlreadySetContentType = false;
		if (request->getHttpMethod()==HTTP_GET) {
			sValue1 = "GET";
			FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_REQUEST_METHOD.c_str(),FASTCGI_PARAM_REQUEST_METHOD.size(),sValue1.c_str(),sValue1.size(),&nNameValue1Size,lpszSendBuffer);
			//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
		}
		else if (request->getHttpMethod()==HTTP_POST) {
			sValue1 = "POST";
			FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_REQUEST_METHOD.c_str(),FASTCGI_PARAM_REQUEST_METHOD.size(),sValue1.c_str(),sValue1.size(),&nNameValue1Size,lpszSendBuffer);
			//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);

			std::vector<cgcUploadFile::pointer> outFils;
			request->getUploadFile(outFils);
			//printf("******** UploadFile size=%d\n",outFils.size());
			if (!outFils.empty()) {
				bAlreadySetContentType = true;
				char lpszBoundary[24];
				sprintf(lpszBoundary,"------%lld%03d%03d",(mycp::bigint)theTimerHandler->m_tNow,(int)rand(),nRequestId);
				std::string sContentType("multipart/form-data; boundary=");
				sContentType.append(lpszBoundary);
				// CONTENT_TYPE
				//printf("******** CONTENT_TYPE=%s\n",sContentType.c_str());
				FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_CONTENT_TYPE.c_str(),FASTCGI_PARAM_CONTENT_TYPE.size(),sContentType.c_str(),sContentType.size(),&nNameValue1Size,lpszSendBuffer);
				//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
				int nContentLength = 0;
				std::vector<std::string> pBoundaryHeadList;
				std::vector<std::string> pBoundaryEndList;
				char lpszBuffer[512];
				for (size_t i=0;i<outFils.size();i++) {
					const cgcUploadFile::pointer& pUploadFile = outFils[i];
					sprintf(lpszBuffer,"--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",
						lpszBoundary,pUploadFile->getName().c_str(),pUploadFile->getFileName().c_str(),pUploadFile->getContentType().c_str());
					pBoundaryHeadList.push_back(lpszBuffer);
					nContentLength += strlen(lpszBuffer);
					nContentLength += pUploadFile->getFileSize();
					if (i+1==outFils.size()) {
						sprintf(lpszBuffer,"\r\n--%s--\r\n",lpszBoundary);
					}
					else {
						sprintf(lpszBuffer,"\r\n--%s\r\n",lpszBoundary);
					}
					pBoundaryEndList.push_back(lpszBuffer);
					nContentLength += strlen(lpszBuffer);
				}
				//Content_type:multipart/form-data; boundary=---------------------------2571883601823556076521314992  
				//Content_length:2668  
				//Standard input:  
				//-----------------------------2571883601823556076521314992  
				//Content-Disposition: form-data; name="file"; filename="nginx.conf"  
				//Content-Type: application/octet-stream  
				//
				//内容..........  
				//-----------------------------2571883601823556076521314992--
				//printf("******** CONTENT_LENGTH=%d\n",nContentLength);
				// CONTENT_LENGTH
				sprintf(lpszIntBuf,"%d",nContentLength);
				FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_CONTENT_LENGTH.c_str(),FASTCGI_PARAM_CONTENT_LENGTH.size(),lpszIntBuf,strlen(lpszIntBuf),&nNameValue1Size,lpszSendBuffer);
				//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);

				// end FCGI_PARAMS
				FCGI_Header header = MakeHeader(FCGI_PARAMS,nRequestId,0,0);
				//m_pFastCgiServer->sendData((const unsigned char*)&header,FCGI_HEADER_LEN);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,(const unsigned char*)&header,FCGI_HEADER_LEN,true);

				const int const_send_buffer = DEFAULT_SEND_BUFFER_SIZE;
				//int nPaddingLength = 0;
				for (size_t i=0;i<outFils.size();i++) {
					const std::string& sBoundaryHead = pBoundaryHeadList[i];
					const std::string& sBoundaryEnd = pBoundaryEndList[i];
					const cgcUploadFile::pointer& pUploadFile = outFils[i];
					const size_t nFileSize = pUploadFile->getFileSize();
					FILE * f = fopen(pUploadFile->getFilePath().c_str(),"rb");
					if (f==0) {
						CGC_LOG((mycp::LOG_ERROR, "File not exist (%s)\n",pUploadFile->getFilePath().c_str()));
						break;
					}

					size_t nToReadSize = min(nFileSize,const_send_buffer-FCGI_HEADER_LEN-sBoundaryHead.size()-8);	// 8=paddinglength
					size_t nReadedSize = 0;
					memcpy(lpszSendBuffer+FCGI_HEADER_LEN,sBoundaryHead.c_str(),sBoundaryHead.size());
					size_t nBufferOffset = FCGI_HEADER_LEN+sBoundaryHead.size();
					while (nReadedSize<nFileSize) {
						const size_t nReadSize = fread(lpszSendBuffer+nBufferOffset,1,nToReadSize,f);
						if (nReadSize==0) {
							break;
						}
						nReadedSize += nReadSize;
						nBufferOffset += nReadSize;
						if (nBufferOffset+sBoundaryEnd.size()<=(size_t)const_send_buffer) {
							memcpy(lpszSendBuffer+nBufferOffset,sBoundaryEnd.c_str(),sBoundaryEnd.size());
							nBufferOffset += sBoundaryEnd.size();
						}						
						FCGI_BuildStdinHeader(nRequestId,lpszSendBuffer,nBufferOffset-FCGI_HEADER_LEN,&nNameValue1Size);
						m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
						nToReadSize = min(const_send_buffer-FCGI_HEADER_LEN-8, nFileSize-nReadedSize);
						nBufferOffset = FCGI_HEADER_LEN;
					}
					fclose(f);
				}
			}
			else if (request->getContentLength()>0 && !sHttpQueryString.empty()) {	/// * 这里只处理 POST 数据
				bAlreadySetContentType = true;
				const tstring& sContentType = request->getContentType();
				//printf("******** sContentType=%s,sQueryString=%s,content_length=%d\n",sContentType.c_str(),sHttpQueryString.c_str(),(int)request->getContentLength());
				if (!sContentType.empty()) {
					FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_CONTENT_TYPE.c_str(),FASTCGI_PARAM_CONTENT_TYPE.size(),sContentType.c_str(),sContentType.size(),&nNameValue1Size,lpszSendBuffer);
					//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
					pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
				}

				// CONTENT_LENGTH
				sprintf(lpszIntBuf,"%d",(int)sHttpQueryString.size());
				FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_CONTENT_LENGTH.c_str(),FASTCGI_PARAM_CONTENT_LENGTH.size(),lpszIntBuf,strlen(lpszIntBuf),&nNameValue1Size,lpszSendBuffer);
				//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);

				// end FCGI_PARAMS
				FCGI_Header header = MakeHeader(FCGI_PARAMS,nRequestId,0,0);
				//m_pFastCgiServer->sendData((const unsigned char*)&header,FCGI_HEADER_LEN);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,(const unsigned char*)&header,FCGI_HEADER_LEN);

				memcpy(lpszSendBuffer+FCGI_HEADER_LEN,sHttpQueryString.c_str(),sHttpQueryString.size());
				const size_t nBufferOffset = FCGI_HEADER_LEN+sHttpQueryString.size();
				FCGI_BuildStdinHeader(nRequestId,lpszSendBuffer,nBufferOffset-FCGI_HEADER_LEN,&nNameValue1Size);
				//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
			}
		}
		// CONTENT_TYPE
		if (!bAlreadySetContentType) {
			const tstring& sContentType = request->getContentType();
			//printf("******** sContentType=%s\n",sContentType.c_str());
			if (!sContentType.empty()) {
				FCGI_BuildParamsBody(nRequestId,FASTCGI_PARAM_CONTENT_TYPE.c_str(),FASTCGI_PARAM_CONTENT_TYPE.size(),sContentType.c_str(),sContentType.size(),&nNameValue1Size,lpszSendBuffer);
				//m_pFastCgiServer->sendData((const unsigned char*)lpszSendBuffer,nNameValue1Size);
				pFastcgiRequestInfo->SendData(m_pFastCgiServer,lpszSendBuffer,nNameValue1Size);
			}
			// end FCGI_PARAMS
			FCGI_Header header = MakeHeader(FCGI_PARAMS,nRequestId,0,0);
			//m_pFastCgiServer->sendData((const unsigned char*)&header,FCGI_HEADER_LEN);
			pFastcgiRequestInfo->SendData(m_pFastCgiServer,(const unsigned char*)&header,FCGI_HEADER_LEN);
		}

		// end FCGI_STDIN
		FCGI_Header header = MakeHeader(FCGI_STDIN,nRequestId,0,0);
		//m_pFastCgiServer->sendData((const unsigned char*)&header,FCGI_HEADER_LEN);
		pFastcgiRequestInfo->SendData(m_pFastCgiServer,(const unsigned char*)&header,FCGI_HEADER_LEN,true);
		//CGC_LOG((mycp::LOG_TRACE, "RequestId=%d SendData ok\n", nRequestId));

//		for (int i=0;i<200;i++)	// max 2S
//		{
//			theWaitDataList.insert(nRequestId,true);
//			if (theWaitDataList.size()<=3)
//			{
//				pFastcgiRequestInfo->SendData(m_pFastCgiServer,(const unsigned char*)&header,FCGI_HEADER_LEN,true);
//				CGC_LOG((mycp::LOG_TRACE, "RequestId=%d SendData ok\n", nRequestId));
//				break;
//			}
//			theWaitDataList.remove(nRequestId);
//#ifdef WIN32
//			Sleep(10);
//#else
//			usleep(10000);
//#endif
//		}

		//const time_t tTimeBegin = time(0);
		const tstring sPhpTimerout = request->getHeader("Php-Timeout", "0");
		const int nPhpTimeout = atoi(sPhpTimerout.c_str());
		const int nWaitTimeout = nPhpTimeout>0?nPhpTimeout:thePHPFastcgiInfo->m_nResponseTimeout;
		for (int i=0;i<nWaitTimeout*200;i++) {	/// default 30S
			if (pFastcgiRequestInfo->GetResponseState()>=REQUEST_INFO_STATE_FCGI_END_REQUEST) {
				const bool bResponseError = pFastcgiRequestInfo->GetResponseError();
#ifdef USES_FASTCGI_KEEP_CONN
				if (bResponseError) {
					CRequestPassInfo::pointer pRequestPassInfo = pFastcgiRequestInfo->GetRequestPassInfo();
					pRequestPassInfo->KillProcess();
					theTimerHandler->CreateRequestPassInfoProcess(pRequestPassInfo);
				}
				else {
					pFastcgiRequestInfo->GetRequestPassInfo()->ResetErrorCount();
				}
				theTimerHandler->RemoveRequestInfo(pFastcgiRequestInfo);
#endif
				//CGC_LOG((mycp::LOG_TRACE, "RequestId=%d return STATUS_CODE_200, error=%d\n", nRequestId, (int)(bResponseError?1:0)));
				return STATUS_CODE_200;
			}
			else if (!theApplication->isInited()) {
				break;
			}
#ifdef WIN32
			Sleep(5);
#else
			usleep(5000);
#endif
			//if ((time(0) - tTimeBegin)>=thePHPFastcgiInfo->m_nResponseTimeout)
			//	break;
		}
		CRequestPassInfo::pointer pRequestPassInfo = pFastcgiRequestInfo->GetRequestPassInfo();
		CGC_LOG((mycp::LOG_ERROR, "PHP fastcgi timeout (requestid=%d, %s), errorCount=%d\n",
			nRequestId,pFastcgiRequestInfo->GetFastcgiPass().c_str(),pRequestPassInfo->IncreaseErrorCount()));
		//header = MakeHeader(FCGI_ABORT_REQUEST,nRequestId,0,0);
		//m_pFastCgiServer->sendData((const unsigned char*)&header,FCGI_HEADER_LEN);
		if (pRequestPassInfo->IncreaseErrorCount()>=2 && pRequestPassInfo->GetProcessId()>0)
		{
			pRequestPassInfo->KillProcess();
			theTimerHandler->CreateRequestPassInfoProcess(pRequestPassInfo);
		}
		theTimerHandler->RemoveRequestInfo(pFastcgiRequestInfo);
		return STATUS_CODE_200;
	}
	
	{
		// 返回
		//HTTP/1.0 200 OK
		//	Content-Length: 13057672
		//	Content-Type: application/octet-stream
		//	Last-Modified: Wed, 10 Oct 2005 00:56:34 GMT
		//	Accept-Ranges: bytes
		//	ETag: "2f38a6cac7cec51:160c"
		//	Server: Microsoft-IIS/6.0
		//	X-Powered-By: ASP.NET
		//	Date: Wed, 16 Nov 2005 01:57:54 GMT
		//	Connection: close 

		// Content-Range: bytes 500-999/1000 
		// Content-Range字段说明服务器返回了文件的某个范围及文件的总长度。这时Content-Length字段就不是整个文件的大小了，
		// 而是对应文件这个范围的字节数，这一点一定要注意。

		// 请求
		//Range: bytes=500-      表示读取该文件的500-999字节，共500字节。
		//Range: bytes=500-599   表示读取该文件的500-599字节，共100字节。

		CResInfo::pointer pResInfo = CGC_OBJECT_CAST<CResInfo>(theAppAttributes->getAttribute(ATTRIBUTE_FILE_INFO, sFilePath));
		if (pResInfo.get() == NULL)
		{
			pResInfo = CResInfo::create(sFilePath,sMimeType);
			theAppAttributes->setAttribute(ATTRIBUTE_FILE_INFO,sFilePath,pResInfo);
		}
		pResInfo->m_tRequestTime = theTimerHandler->m_tNow;		// ***记录最新时间，用于定时清空太久没用资源

		//fs::path src_path(sFilePath);
		const time_t lastTime = fs::last_write_time(src_path);
		{
			boost::mutex::scoped_lock lock(pResInfo->m_mutex);
			if (pResInfo->m_tModifyTime==0 || pResInfo->m_tModifyTime != lastTime)
			{
				// * 
				// response-body-only
				FILE * f = fopen(sFilePath.c_str(),"rb");
				if (f==NULL)
					return STATUS_CODE_404;
				mycp::bigint nFileSize = 0;
				tstring sFileMd5String;
				GetFileMd5(f,nFileSize,sFileMd5String);
				char lpszETag[33];
				sprintf(lpszETag,"\"%s\"",sFileMd5String.c_str());
				pResInfo->m_sETag = lpszETag;
				pResInfo->m_nSize = (unsigned int)nFileSize;
				pResInfo->m_tModifyTime = lastTime;
				struct tm * tmModifyTime = gmtime(&pResInfo->m_tModifyTime);
				char lpszModifyTime[128];
				strftime(lpszModifyTime, 128, "%a, %d %b %Y %H:%M:%S GMT", tmModifyTime);
				pResInfo->m_sModifyTime = lpszModifyTime;
				boost::mutex::scoped_lock lockFile(pResInfo->m_mutexFile);
				// 读文件内容到内存（只读取小于50MB，超过的直接从文件读取）
				if ((int)pResInfo->m_nSize<=const_memory_size)
				{
					if (pResInfo->m_pData==NULL)
					{
						pResInfo->m_nDataBufferSize = pResInfo->m_nSize;
						pResInfo->m_pData = new char[pResInfo->m_nDataBufferSize+1];
					}else if (pResInfo->m_nDataBufferSize<pResInfo->m_nSize)
					{
						delete[] pResInfo->m_pData;
						pResInfo->m_nDataBufferSize = pResInfo->m_nSize;
						pResInfo->m_pData = new char[pResInfo->m_nDataBufferSize+1];
					}
					if (pResInfo->m_pData==NULL)
						return STATUS_CODE_400;
#ifdef WIN32
					_fseeki64(f, 0, SEEK_SET);
#else
					fseeko(f, 0, SEEK_SET);
#endif
					fread(pResInfo->m_pData,1,pResInfo->m_nSize,f);
					pResInfo->m_pData[pResInfo->m_nSize] = '\0';
					fclose(f);
					if (pResInfo->m_pFile!=NULL)
						fclose(pResInfo->m_pFile);
					pResInfo->m_pFile = NULL;
				}else
				{
					if (pResInfo->m_pFile!=NULL)
						fclose(pResInfo->m_pFile);
					pResInfo->m_pFile = f;
					if (pResInfo->m_pData!=NULL)
					{
						delete[] pResInfo->m_pData;
						pResInfo->m_pData = NULL;
						pResInfo->m_nDataBufferSize = 0;
					}
				}
			}
		}
		if (sIfModifiedSince==pResInfo->m_sModifyTime)
		{
			SetExpiresCache(response,pResInfo->m_tRequestTime,bIsImageOrBinary);
			return STATUS_CODE_304;					// 304 Not Modified
		}
		if (!sIfNoneMatch.empty() && !pResInfo->m_sETag.empty() && sIfNoneMatch.find(pResInfo->m_sETag)!=std::string::npos)
		{
			SetExpiresCache(response,pResInfo->m_tRequestTime,bIsImageOrBinary);
			return STATUS_CODE_304;					// 304 Not Modified
		}

		//printf("**** %s\n",lpszFileName);
		unsigned int nRangeFrom = request->getRangeFrom();
		unsigned int nRangeTo = request->getRangeTo();
		const tstring sRange(request->getHeader(Http_Range));
		//printf("**** %s\n",sRange.c_str());
		//printf("**** Range %d-%d\n",nRangeFrom,nRangeTo);
		if (nRangeTo == 0 && pResInfo->m_nSize>0)
		{
			nRangeTo = pResInfo->m_nSize-1;
		}
		unsigned int nReadTotal = nRangeTo>0?(nRangeTo-nRangeFrom+1):0;			// 重设数据长度
		//printf("**** RangeFrom=%d, RangeTo=%d, nReadTotal=%d\n",nRangeFrom,nRangeTo,nReadTotal);
		//if (nReadTotal >= 1024)
		//{
		//	statusCode = STATUS_CODE_206;
		//	nReadTotal = 1024;
		//	nRangeTo = nRangeFrom+1024;
		//}
		if (nRangeTo>=pResInfo->m_nSize && pResInfo->m_nSize>0)
			return STATUS_CODE_416;					// 416 Requested Range Not Satisfiable
		//else if (nReadTotal>1024*1024)	// 分包下载
		else if (nReadTotal>theMaxSize)	// 分包下载
		{
			//statusCode = STATUS_CODE_206;				// 206 Partial Content
			//nReadTotal = 1024;
			//nRangeTo = nRangeFrom+nReadTotal-1;
			//printf("**** EB_MAX_MEMORY_SIZE data, pResInfo->m_sETag=%s\n",pResInfo->m_sETag.c_str());
			//if (!pResInfo->m_sETag.empty())
			//	response->setHeader("ETag",pResInfo->m_sETag);
			//response->setHeader("Accept-Ranges","bytes");
			//return STATUS_CODE_200;					// 413 Request Entity Too Large
			return STATUS_CODE_413;					// 413 Request Entity Too Large
		}else if (!sRange.empty())
		{
			statusCode = STATUS_CODE_206;				// 206 Partial Content
		}
		SetExpiresCache(response,pResInfo->m_tRequestTime,bIsImageOrBinary);
		//const int nExpireSecond = bIsImageOrBinary?(10*const_one_day_seconds):const_one_day_seconds;	// 文本文件：10分钟；其他图片等=3600=60分钟；
		//const time_t tExpiresTime = pResInfo->m_tRequestTime + nExpireSecond;
		//struct tm * tmExpiresTime = gmtime(&tExpiresTime);
		//char lpszBuffer[64];
		//strftime(lpszBuffer, 64, "%a, %d %b %Y %H:%M:%S GMT", tmExpiresTime);	// Tue, 19 Aug 2015 07:00:19 GMT
		//response->setHeader("Expires",lpszBuffer);
		//sprintf(lpszBuffer, "max-age=%d", nExpireSecond);
		//response->setHeader("Cache-Control",lpszBuffer);
		////response->setHeader("Cache-Control","max-age=8640000");	// 86400=1天；100天

		//char lpszBuffer[512];
		//sprintf(lpszBuffer,"attachment;filename=%s%s",sResourceId.c_str(),sExt.c_str());	// 附件方式下载
		//sprintf(lpszBuffer,"inline;filename=%s%s",sResourceId.c_str(),sExt.c_str());		// 网页打开
		//response->setHeader(Http_ContentDisposition,lpszBuffer);
		if (!pResInfo->m_sETag.empty())
			response->setHeader("ETag",pResInfo->m_sETag);
		response->setHeader("Last-Modified",pResInfo->m_sModifyTime);
		//printf("************ ContentType : %s\n",sMimeType.c_str());
		response->setContentType(sMimeType);
//		if (sRange.empty() && statusCode == STATUS_CODE_206 && pResInfo->m_pFile!=NULL)
//		{
//			boost::mutex::scoped_lock lockFile(pResInfo->m_mutexFile);
//#ifdef WIN32
//			_fseeki64(pResInfo->m_pFile, 0, SEEK_SET);
//#else
//			fseeko(pResInfo->m_pFile, 0, SEEK_SET);
//#endif
//			char lpszBuffer[64];
//			//HTTP/1.1 200 OK  
//			//Transfer-Encoding: chunked  
//			//...  
//			//5  
//			//Hello  
//			//6  
//			//world!  
//			//0 
//#define USES_CHUNKED
//			printf("**** response for chunked: %d-%d/%d\n",nRangeFrom,nRangeTo,pResInfo->m_nSize);
//#ifdef USES_CHUNKED
//			response->setHeader("Transfer-Encoding","chunked");	// 分段传输
//			//response->setHeader("Vary","Accept-Encoding");	// 分段传输
//			response->setHeader(Http_AcceptRanges,"bytes");
//			//sprintf(lpszBuffer,"bytes %d-%d/%d",nRangeFrom,nRangeTo,pResInfo->m_nSize);
//			response->setHeader("Connection","Keep-Alive");
//			//response->setContentType("multipart/byteranges");
//			//response->setContentType("application/octet-stream");	// 会提示下载
//			//response->setHeader(Http_ContentRange,lpszBuffer);
//#else
//			sprintf(lpszBuffer,"%d",(int)pResInfo->m_nSize);
//			response->setHeader("Connection","Keep-Alive");
//			response->setHeader("Content-Length",lpszBuffer);
//#endif
//			mycp::bigint nReadIndex = 0;
//			char* lpszReadBuffer = NULL;
//			while(!response->isInvalidate())
//			{
//				const size_t nReadBufferSize = min(8*1024,(pResInfo->m_nSize-nReadIndex));
//				//const size_t nReadBufferSize = min(theOneMB,(pResInfo->m_nSize-nReadIndex));
//				if (lpszReadBuffer==NULL)
//					lpszReadBuffer = new char[nReadBufferSize+1];
//				const size_t nReadSize = fread(lpszReadBuffer,1,nReadBufferSize,pResInfo->m_pFile);
//				nReadIndex += nReadSize;
//				printf("**** response %x,nReadIndex=%d\n", (int)nReadSize,(int)nReadIndex);
//#ifdef USES_CHUNKED
//				sprintf(lpszBuffer,"%x",(int)nReadSize);
//				response->writeData(lpszBuffer, strlen(lpszBuffer));
//				response->newline();
//				if (nReadSize==0)
//				{
//					response->newline();
//					response->newline();
//					response->sendResponse(STATUS_CODE_200);
//					break;
//				}
//				response->writeData(lpszReadBuffer, nReadSize);
//				response->newline();
//				if (nReadIndex>=pResInfo->m_nSize)
//				{
//					response->writeData("0", 1);
//					response->newline();
//					response->newline();
//					response->sendResponse(STATUS_CODE_200);
//					break;
//				}else
//				{
//					response->sendResponse(STATUS_CODE_206);
//				}
//#else
//				if (nReadSize==0)
//					break;
//				response->writeData(lpszReadBuffer, nReadSize);
//				if (nReadIndex>=pResInfo->m_nSize)
//				{
//					response->newline();
//					response->newline();
//					response->sendResponse(STATUS_CODE_200);
//					break;
//				}else
//				{
//					response->sendResponse(STATUS_CODE_206);
//					response->setHeader("response-body-only","");
//					//response->setHeader("reset-body-size","");
//				}
//#endif
////#ifdef WIN32
////				Sleep(10);
////#else
////				usleep(10000);
////#endif
//			}
//			if (lpszReadBuffer!=NULL)
//				delete[] lpszReadBuffer;
//			return STATUS_CODE_200;;
//		}

		if (statusCode == STATUS_CODE_206)
		//if (!sRange.empty() && statusCode == STATUS_CODE_206)
		{
			response->setHeader(Http_AcceptRanges,"bytes");
			char lpszBuffer[128];
			sprintf(lpszBuffer,"bytes %d-%d/%d",nRangeFrom,nRangeTo,pResInfo->m_nSize);
			response->setHeader(Http_ContentRange,lpszBuffer);
		}
		if (nReadTotal > 0)
		{
			//printf("*********** %d-%d send...\n",nRangeFrom,nReadTotal);
			//CGC_LOG((mycp::LOG_TRACE, "filepath=%s, Data=0x%x, Size=%d, nReadTotal=%d\n",sFilePath.c_str(),(int)pResInfo->m_pData,(int)pResInfo->m_nSize,(int)nReadTotal));
			boost::mutex::scoped_lock lockFile(pResInfo->m_mutexFile);
			if (pResInfo->m_pData != NULL)
			{
				// 读内存
				//printf("**** read from memory: %d-%d/%d\n",nRangeFrom,nRangeTo,pResInfo->m_nSize);
				try
				{
					response->writeData(pResInfo->m_pData+nRangeFrom, nReadTotal);
				}catch(std::exception const &)
				{
					return STATUS_CODE_500;
				}catch(...)
				{
					return STATUS_CODE_500;
				}
			}else
			{
				// 读文件
				//printf("**** read from file: %d-%d/%d\n",nRangeFrom,nRangeTo,pResInfo->m_nSize);
				if (pResInfo->m_pFile == NULL)
				{
					pResInfo->m_pFile = fopen(pResInfo->m_sFileName.c_str(),"rb");
					if (pResInfo->m_pFile==NULL)
						return STATUS_CODE_404;
				}
#ifdef WIN32
				_fseeki64(pResInfo->m_pFile, nRangeFrom, SEEK_SET);
#else
				fseeko(pResInfo->m_pFile, nRangeFrom, SEEK_SET);
#endif
				ReadFileDataToResponse(pResInfo->m_pFile,nReadTotal,response);
			}
			//response->sendResponse(statusCode);
			//printf("*********** %d-%d send ok\n",nRangeFrom,nReadTotal);
		}else
		{
			// *有可能是空文件
		}
	}

	//printf("**** return =%d\n",statusCode);
	//return STATUS_CODE_200;
	return statusCode;
}
