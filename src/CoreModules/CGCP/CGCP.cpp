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

// CGCP.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include "iostream"
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#include <MMSystem.h>
#include <ShellApi.h>

#else
#include <sys/sysinfo.h>
#include <time.h>
#include <errno.h>
#endif // WIN32
#include "CGC/CGCApp.h"

#if (USES_NEWVERSION)
typedef int (FAR *FPCGC_app_main)(const char * lpszPath);
#endif


#ifdef WIN32
tstring theModulePath;
#include "tlhelp32.h"
#include "Psapi.h"
#pragma comment(lib, "Psapi.lib")
static BOOL CALLBACK EnumWindowsCloseWindow(HWND hwnd,LPARAM lParam)
{
	TCHAR lpszBuffer[255];
	memset(lpszBuffer, 0, sizeof(lpszBuffer));
	::GetWindowText(hwnd, lpszBuffer, sizeof(lpszBuffer));
	if (strstr(lpszBuffer, "CGCP.exe")==lpszBuffer)
	{
		const DWORD nCurrentProcessId = (DWORD)lParam;
		DWORD dwWndProcessId = 0;
		GetWindowThreadProcessId(hwnd,&dwWndProcessId);
		if (dwWndProcessId>0 && dwWndProcessId!=nCurrentProcessId)
		{
			HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, dwWndProcessId);
			if(NULL != hProcess)
			{
				TerminateProcess(hProcess, 0);
			}
		}
		//::SetWindowPos(hwnd,HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
		//::SetActiveWindow(hwnd);
		//::SetForegroundWindow(hwnd);
		//RECT rect;
		//::GetWindowRect(hwnd,&rect);
		//int x = rect.right-18;
		//int y = rect.top+5;
		//::SetCursorPos(x, y);
		//mouse_event (MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP, x, y, 0, 0);//鼠标按下
		return FALSE;
	}
	return TRUE;
}
void KillCGCP(void)
{
	const DWORD nCurrentProcessId = GetCurrentProcessId();
	const std::string strExeName(theModulePath.c_str());

	TCHAR szPath[MAX_PATH];
	Sleep(1000);
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);  
	if (hProcessSnap==NULL) return;
	PROCESSENTRY32 pe32;  
	memset(&pe32,0,sizeof(pe32));
	pe32.dwSize=sizeof(PROCESSENTRY32);  
	bool bExistApp = false;
	if (::Process32First(hProcessSnap,&pe32))  
	{  
		do  
		{
			if (nCurrentProcessId!=pe32.th32ProcessID)
			{
				std::string::size_type find = strExeName.find(pe32.szExeFile);
				if (find != std::string::npos)
				{
					std::string sExePath(strExeName.substr(0,find));
					find = sExePath.find(":\\");
					if (find!=std::string::npos)
						sExePath = sExePath.substr(find+2);

					HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE|PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
					//HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
					if(NULL != hProcess)
					{
						DWORD nSize = MAX_PATH;
						memset(szPath,0,nSize);
						GetProcessImageFileName(hProcess, szPath, MAX_PATH);
						std::string sFindPath(szPath);
						if (sFindPath.empty() || sFindPath.find(sExePath)!=std::string::npos)
							TerminateProcess(hProcess, 0);
						else
							CloseHandle(hProcess);
						//break;
					}
				}
			}
		}while(::Process32Next(hProcessSnap,&pe32));   
	}
	CloseHandle(hProcessSnap);

	EnumWindows(EnumWindowsCloseWindow, (LPARAM)nCurrentProcessId);
	//HWND hHwnd = FindWindow(NULL,"CGCP.exe");
	//if (hHwnd!=NULL)
	//{
	//	DWORD dwWndProcessId = 0;
	//	GetWindowThreadProcessId(hHwnd,&dwWndProcessId);
	//	if (dwWndProcessId>0 && dwWndProcessId!=nCurrentProcessId)
	//	{
	//		HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, dwWndProcessId);
	//		if(NULL != hProcess)
	//		{
	//			TerminateProcess(hProcess, 0);
	//		}
	//	}
	//}
}

unsigned long GetSystemBootTime(void)
{
	return timeGetTime()/1000;
}
#else
unsigned long GetSystemBootTime(void)
{
	struct sysinfo info;
	struct tm *ptm = NULL;
	if (sysinfo(&info)) {
		return 0;
	}
	return info.uptime;
}
#endif

#ifndef WIN32
inline int FindPidByName(const char* lpszName)
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

#if defined(WIN32) && !defined(__MINGW_GCC)
#include "SSNTService.h"
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	setlocale(LC_ALL, "");
	//setlocale(LC_ALL, ".936");
	const std::string sProgram(argv[0]);
	bool bService = false;
	bool bProtect = false;
//#ifdef WIN32
	bool bSingleProcess = false;
//#endif
	//int nWaitSeconds=0;
	if (argc>1)
	{
		for (int i=1; i<argc; i++)
		{
			const std::string sParam(argv[i]);
			if (sParam == "-S")
			{
				bService = true;
			}else if (sParam == "-P")
			{
				bProtect = true;
			//}else if (sParam == "-W" && (i+1)<argc)
			//{
			//	nWaitSeconds = atoi(argv[i+1]);
			//	//printf("******* nWaitSeconds = %d\n",nWaitSeconds);
			//	i++;
//#ifdef WIN32
			}else if (sParam == "-s")
			{
				bSingleProcess = true;
//#endif
			}
		}
	}

	const unsigned long nSystemBootTime = GetSystemBootTime();
	const int nWaitSeconds = nSystemBootTime<120?12:0;
	//const int nWaitSeconds = nSystemBootTime<300?10:0;
	//printf("**** nSystemBootTime=%d\n",(int)nSystemBootTime);

	tstring sModulePath;
#ifdef WIN32
	TCHAR chModulePath[MAX_PATH];
	memset(&chModulePath, 0, MAX_PATH);
	::GetModuleFileName(NULL, chModulePath, MAX_PATH);
	theModulePath = chModulePath;
	TCHAR* temp = (TCHAR*)0;
	temp = _tcsrchr(chModulePath, (unsigned int)'\\');
	chModulePath[temp - chModulePath] = '\0';

	sModulePath = chModulePath;
#else
	namespace fs = boost::filesystem;
	fs::path currentPath( fs::initial_path());
	sModulePath = currentPath.string();
#endif
	//printf("** program=%s,%d\n",sProgram.c_str(),(int)(bService?1:0));
	tstring sProtectDataFile(sModulePath);
	sProtectDataFile.append("/CGCP.protect");
	char lpszBuffer[260];
	if (bProtect)
	{
		bool bWriteLog = false;
		const time_t tStartTime = time(0);
		int nErrorCount = 0;
		while (true)
		{
#ifdef WIN32
			Sleep(1000);
#else
			sleep(1);
			/// 
#endif

			const time_t tCurrentTime = time(0);
			if (tStartTime+20>tCurrentTime)
				continue;
			FILE * pfile = fopen(sProtectDataFile.c_str(),"r");
			if (pfile==NULL)
			{
				if ((nErrorCount++)>=3)
				{
					// 3 second protect file not exist, exit protect.
					break;
				}
			}else
			{
				nErrorCount = 0;
				memset(lpszBuffer,0,sizeof(lpszBuffer));
				fread(lpszBuffer,1,sizeof(lpszBuffer),pfile);
				fclose(pfile);
				const time_t tRunTime = cgc_atoi64(lpszBuffer);
				if (tRunTime>0 && (tCurrentTime-tRunTime)>=12) {	/// 8
#ifdef WIN32
					const int pid = 1;
#else
					sprintf(lpszBuffer,"%s -S",sProgram.c_str());
					const int pid = FindPidByName(lpszBuffer);
					/// 只打印一次
					if (pid > 0 && bWriteLog) {
						continue;
					}
#endif

					bWriteLog = true;
					tstring sProtectLogFile(sModulePath);
					sProtectLogFile.append("/CGCP.protect.log");
					FILE * pProtectLog = fopen(sProtectLogFile.c_str(),"a");
					if (pProtectLog!=NULL)
					{
						const struct tm *newtime = localtime(&tRunTime);
						char lpszDateDir[64];
						sprintf(lpszDateDir,"%04d-%02d-%02d %02d:%02d:%02d %d pid=%d\n",
							newtime->tm_year+1900,newtime->tm_mon+1,newtime->tm_mday,newtime->tm_hour,newtime->tm_min,newtime->tm_sec,(int)(tCurrentTime-tRunTime), pid);
						fwrite(lpszDateDir,1,strlen(lpszDateDir),pProtectLog);
						fclose(pProtectLog);
					}

#ifdef WIN32
					KillCGCP();
					if (strstr(lpszBuffer,",1")!=NULL)
						ShellExecute(NULL,"open",sProgram.c_str(),"-run -S",sModulePath.c_str(),SW_SHOW);
					else
						ShellExecute(NULL,"open",sProgram.c_str(),"",sModulePath.c_str(),SW_SHOW);
#else
					if (pid > 0) {
						continue;
					}
					sprintf(lpszBuffer,"\"%s\" -S &",sProgram.c_str());
					system(lpszBuffer);
#endif
					break;
				}
				else {
					bWriteLog = false;
				}
			}
		}
		return 0;
	}else
	{
		FILE * pfile = fopen(sProtectDataFile.c_str(),"w");
		if (pfile!=NULL)
		{
			sprintf(lpszBuffer,"%lld,%d",(bigint)time(0),(int)(bService?1:0));
			fwrite(lpszBuffer,1,strlen(lpszBuffer),pfile);
			fclose(pfile);
#ifdef WIN32
			if (!bSingleProcess)
			{
				ShellExecute(NULL,"open",sProgram.c_str(),"-P",sModulePath.c_str(),SW_HIDE);
			}
#else
			if (!bSingleProcess)
			{
				sprintf(lpszBuffer,"\"%s\" -P &",sProgram.c_str());
				system(lpszBuffer);
			}
#endif
		}
	}

#if (USES_NEWVERSION)
	std::string sModuleName = sModulePath;
	sModuleName.append("/modules/");
#ifdef WIN32
#ifdef _DEBUG

#if (_MSC_VER == 1200)
	sModuleName.append("MYCore60d.dll");
#elif (_MSC_VER == 1300)
	sModuleName.append("MYCore70d.dll");
#elif (_MSC_VER == 1310)
	sModuleName.append("MYCore71d.dll");
#elif (_MSC_VER == 1400)
	sModuleName.append("MYCore80d.dll");
#elif (_MSC_VER == 1500)
	sModuleName.append("MYCore90d.dll");
#endif

#else // _DEBUG

	#if (_MSC_VER == 1200)
	sModuleName.append("MYCore60.dll");
#elif (_MSC_VER == 1300)
	sModuleName.append("MYCore70.dll");
#elif (_MSC_VER == 1310)
	sModuleName.append("MYCore71.dll");
#elif (_MSC_VER == 1400)
	sModuleName.append("MYCore80.dll");
#elif (_MSC_VER == 1500)
	sModuleName.append("MYCore90.dll");
#endif

#endif // _DEBUG

#else // WIN32
	sModuleName.append("MYCore.so");
#endif // WIN32

	void * hModule = NULL;
#ifdef WIN32
	hModule = LoadLibrary(sModuleName.c_str());
#else
	hModule = dlopen(sModuleName.c_str(), RTLD_LAZY);
#endif
	if (hModule == NULL)
	{
#ifdef WIN32
		printf("Cannot open library: \'%d\'!\n", GetLastError());
#else
		printf("Cannot open library: \'%s\'!\n", dlerror());
#endif
		return 0;
	}

	FPCGC_app_main fp = 0;
#ifdef WIN32
	fp = (FPCGC_app_main)GetProcAddress((HMODULE)hModule, "app_main");
#else
	fp = (FPCGC_app_main)dlsym(hModule, "app_main");
#endif
	if (fp)
		fp(sModulePath.c_str());

	//DWORD error = GetLastError();

#ifdef WIN32
	FreeLibrary((HMODULE)hModule);
#else
	dlclose (hModule);
#endif
	return 0;
#else // USES_NEWVERSION

#ifdef _DEBUG
	mycp::CGCApp::pointer gApp = mycp::CGCApp::create(sModulePath);
	gApp->MyMain(nWaitSeconds,bService, sProtectDataFile);
#else // _DEBUG

#if defined(WIN32) && !defined(__MINGW_GCC)
	CSSNTService cService;
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
		{ (LPTSTR)cService.m_sServiceName.c_str(), (LPSERVICE_MAIN_FUNCTION)cService.ServiceMain},
        { NULL, NULL }
    };

	if((argc>1)&&((*argv[1]=='-')||(*argv[1]=='/')))
	{
		if (argc >= 3)
			cService.m_sServiceName = argv[2];
		if (argc >= 4)
			cService.m_sServiceDisplayName = argv[3];
		if (argc >= 5)
			cService.m_sServiceDescription = argv[4];

		if(_tcsicmp(_T("install"),argv[1]+1)==0)
		{

			std::cout << _T("installing ") << cService.m_sServiceName.c_str() << _T("...") << std::endl;
			if(cService.InstallService())
			{
				std::cout << cService.m_sServiceName.c_str() << _T(" install succeeded!") << std::endl;

				std::cout << _T("==Start ") << cService.m_sServiceName.c_str();
				std::cout << _T(": net start ") << cService.m_sServiceName.c_str() << std::endl;

				std::cout << _T("==Stop ") << cService.m_sServiceName.c_str();
				std::cout << _T(": net stop ") << cService.m_sServiceName.c_str() << std::endl;
			}else
			{
				std::cout << _T("Failed install ") << cService.m_sServiceName.c_str() << std::endl;
			}
		}else if(_tcsicmp(_T("remove"),argv[1]+1)==0)
		{
			std::wcout << _T("removeing ") << cService.m_sServiceName.c_str() << _T("...") << std::endl;
			if(cService.RemoveService())
			{
				std::cout << cService.m_sServiceName.c_str() << _T(" remove succeeded!") << std::endl;
			}else
			{
				std::cout << _T("Failed remove ") << cService.m_sServiceName.c_str() << std::endl;
			}
		}else if(_tcsicmp(_T("run"),argv[1]+1)==0)
		{
			mycp::CGCApp::pointer gApp = mycp::CGCApp::create(sModulePath);
			gApp->MyMain(nWaitSeconds,bService, sProtectDataFile);
		}else
		{
			std::cout << _T("-install install ") << cService.m_sServiceName.c_str() << std::endl;
			std::cout << _T("-remove remove ") << cService.m_sServiceName.c_str() << std::endl;
			std::cout << _T("-run run ") << cService.m_sServiceName.c_str() << std::endl;
		}
	}else
	{
		if(!cService.StartDispatch())
		{
			cService.AddToErrorMessageLog(TEXT("Failed start server！"));

			mycp::CGCApp::pointer gApp = mycp::CGCApp::create(sModulePath);
			gApp->MyMain(nWaitSeconds,bService, sProtectDataFile);
		}
	}
#else
	mycp::CGCApp::pointer gApp = mycp::CGCApp::create(sModulePath);
	gApp->MyMain(nWaitSeconds,bService, sProtectDataFile);
#endif
#endif // _DEBUG
#endif // USES_NEWVERSION
//	for (int i=0;i<30; i++)
//	{
//		if (boost::filesystem::remove(boost::filesystem::path(sProtectDataFile)))
//		{
//			break;
//		}
//#ifdef WIN32
//		Sleep(100);
//#else
//		usleep(100000);
//#endif
//	}
	remove(sProtectDataFile.c_str());
	return 0;
}
