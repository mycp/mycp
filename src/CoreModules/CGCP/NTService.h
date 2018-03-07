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

// NTService.h file here
#ifndef _NTSERVICE_HEAD_INCLUDED__
#define _NTSERVICE_HEAD_INCLUDED__

#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <tchar.h>
#include <string>

class CNTService
{
public:
    virtual void Continue();
    virtual void Pause();
    virtual void Stop();
    virtual void UnInitialize();
    virtual void Run();
    virtual BOOL Initialize();

	BOOL IsService(void) const;
	DWORD GetCurrentStatus(void) const;	// stoped: SERVICE_STOPPED, running: SERVICE_RUNNING

    static CNTService * m_pService;
    static void WINAPI ServiceHandle(DWORD dwControl);
    static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
    static BOOL SetSCMStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
    static void AddToMessageLog(LPCTSTR lpszMsg);
    static void AddToErrorMessageLog(LPCTSTR lpszMsg) ;
	static void AddToWarningMessageLog(LPCTSTR lpszMsg);

    BOOL StartDispatch();
    BOOL RemoveService();    //将服务程序从SCM管理器中删除掉
    BOOL InstallService();    //将服务程序安装进SCM管理器中

    LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize);

    CNTService();            //构造
    virtual ~CNTService();    //析构

#ifdef _UNICODE
    static std::wstring m_sServiceName;        //服务程序内部名称，名字不能有空格
    static std::wstring m_sServiceDisplayName; //SERVICE程序在管理器中的显示名称，任意字符
	static std::wstring m_sServiceDescription; // 服务功能描述
#else
    static std::string m_sServiceName;        //服务程序内部名称，名字不能有空格
    static std::string m_sServiceDisplayName; //SERVICE程序在管理器中的显示名称，任意字符
	static std::string m_sServiceDescription; // 服务功能描述
#endif // _UNICODE
private:
    static DWORD dwThreadId ;
    static TCHAR m_lpServicePathName[512];    //SERVICE程序的EXE文件路径
    static SERVICE_STATUS m_ssServiceStatus ;//SERVICE程序的状态struct
    static SERVICE_STATUS_HANDLE m_sshServiceStatusHandle ;//SERVICE程序状态的HANDLE

    BOOL m_bService;
};


#endif // _NTSERVICE_HEAD_INCLUDED__