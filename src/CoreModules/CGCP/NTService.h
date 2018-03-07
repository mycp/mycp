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
    BOOL RemoveService();    //����������SCM��������ɾ����
    BOOL InstallService();    //���������װ��SCM��������

    LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize);

    CNTService();            //����
    virtual ~CNTService();    //����

#ifdef _UNICODE
    static std::wstring m_sServiceName;        //��������ڲ����ƣ����ֲ����пո�
    static std::wstring m_sServiceDisplayName; //SERVICE�����ڹ������е���ʾ���ƣ������ַ�
	static std::wstring m_sServiceDescription; // ����������
#else
    static std::string m_sServiceName;        //��������ڲ����ƣ����ֲ����пո�
    static std::string m_sServiceDisplayName; //SERVICE�����ڹ������е���ʾ���ƣ������ַ�
	static std::string m_sServiceDescription; // ����������
#endif // _UNICODE
private:
    static DWORD dwThreadId ;
    static TCHAR m_lpServicePathName[512];    //SERVICE�����EXE�ļ�·��
    static SERVICE_STATUS m_ssServiceStatus ;//SERVICE�����״̬struct
    static SERVICE_STATUS_HANDLE m_sshServiceStatusHandle ;//SERVICE����״̬��HANDLE

    BOOL m_bService;
};


#endif // _NTSERVICE_HEAD_INCLUDED__