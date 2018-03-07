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

/*********************************************************************
** �ļ���: ��̨����������;
** ����:   ��������:Run();
** ����:   ���������߳�,���շ��������������Ϣ.
**--------------------------------------------------------------------
*********************************************************************/
//////////////////////////////////////////////////////////////////////
//
// NTService.cpp: implementation of the CNTService class.//
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4996)

//#include "stdafx.h"
//#include "Resource.h"
#include "NTService.h"

DWORD CNTService::dwThreadId;
#ifdef _UNICODE
std::wstring CNTService::m_sServiceName;        //��������ڲ����ƣ����ֲ����пո�
std::wstring CNTService::m_sServiceDisplayName; //SERVICE�����ڹ������е���ʾ���ƣ������ַ�
std::wstring CNTService::m_sServiceDescription; // ����������
#else
std::string CNTService::m_sServiceName;        //��������ڲ����ƣ����ֲ����пո�
std::string CNTService::m_sServiceDisplayName; //SERVICE�����ڹ������е���ʾ���ƣ������ַ�
std::string CNTService::m_sServiceDescription; // ����������
#endif // _UNICODE

TCHAR CNTService::m_lpServicePathName[512];    //SERVICE�����EXE�ļ�·��
CNTService * CNTService::m_pService;
SERVICE_STATUS			CNTService::m_ssServiceStatus ;//SERVICE�����״̬struct
SERVICE_STATUS_HANDLE	CNTService::m_sshServiceStatusHandle ;//SERVICE����״̬��HANDLE

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNTService::CNTService()
{

    GetModuleFileName(NULL, m_lpServicePathName, 512); //ȡ��ǰִ���ļ�·��

    m_sServiceName           = _T("Service");  //��������
    m_sServiceDisplayName    = _T("������");  //��ʾ������
	m_sServiceDescription    = _T("");
    m_pService = this ;
    m_ssServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_ssServiceStatus.dwServiceSpecificExitCode = 0 ;
}

CNTService::~CNTService()
{

}

/*********************************************************************
** ��ִ���ļ���װ��NT SERVICE��������
*********************************************************************/
BOOL CNTService::InstallService()
{
    SC_HANDLE schSCManager;      //��SCM�õľ��
    SC_HANDLE schService;        //����SERVICE�õľ��

    schSCManager = OpenSCManager(        //���ô�SERVICE������API
		NULL,        //�������ƣ�����NULLΪ���ػ���
		NULL,        //���ݿ����ƣ�NULLΪȱʡ���ݿ�
		SC_MANAGER_CREATE_SERVICE //�� SC_MANAGER_ALL_ACCESS
		);                      //ϣ���򿪵Ĳ���Ȩ�ޣ����MSDN

    if( schSCManager )        // �����SERVICE�������ɹ�
    {
        schService = CreateService(        //���ý���SERVICE��API
            schSCManager,                //SERVICE���������ݿ�ľ��
			CNTService::m_sServiceName.c_str(),            //��������
			CNTService::m_sServiceDisplayName.c_str(),        //��ʾ������
            SERVICE_ALL_ACCESS,            //ϣ���õ�������Ȩ��
            SERVICE_WIN32_OWN_PROCESS,    //SERVICE������
            SERVICE_AUTO_START,            //�����ķ�ʽ
            SERVICE_ERROR_NORMAL,        //�����������
            m_lpServicePathName,        //��ִ���ļ���·����
            NULL,        //һ�����װ��ʱ��˳��
            NULL,        //����˱��
            _T("RPCSS\0"),        //����
            NULL,        //����USER��
            NULL);        //����

        if( schService )    //�������SERVICE�ɹ�
        {
			//�޸�����
			SERVICE_DESCRIPTION sd;
			if (!CNTService::m_sServiceDescription.empty())
				sd.lpDescription = (LPTSTR)CNTService::m_sServiceDescription.c_str();
			else
				sd.lpDescription = (LPTSTR)CNTService::m_sServiceDisplayName.c_str();
			ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd);
			//ChangeServiceConfig2(schService,0, NULL);
            CloseServiceHandle(schService);        //�ͷ�SERVICE�����׼���˳�
        }else
        {
            return FALSE ; //����SERVICEʧ�ܷ���
        }
        CloseServiceHandle(schSCManager);        //�ͷ�SERVICE���������
    }else
    {
        return FALSE; //�򿪹�����ʧ�ܷ���
    }
    return TRUE; //һ����������
}

/*********************************************************************
** ��SERVICE�����SCM������������
*********************************************************************/
BOOL CNTService::RemoveService()
{
    SC_HANDLE        schSCManager;    //��SCM�õľ��
    SC_HANDLE        schService;        //����SERVICE�õľ��

    schSCManager = OpenSCManager(        //���ô�SERVICE������API
		NULL,        //�������ƣ�����NULLΪ���ػ���
		NULL,        //���ݿ����ƣ�NULLΪȱʡ���ݿ�
		SC_MANAGER_ALL_ACCESS //ϣ���򿪵Ĳ���Ȩ�ޣ����MSDN
		);

    if( schSCManager )        // �����SERVICE�������ɹ�
    {
        schService = OpenService(    //��ȡSERVICE���ƾ����API
            schSCManager,            //SCM���������
            m_sServiceName.c_str(),        //SERVICE�ڲ����ƣ���������
            SERVICE_ALL_ACCESS);    //�򿪵�Ȩ�ޣ�ɾ����Ҫȫ��Ȩ��

        if( schService )    //�����ȡSERVICE����ɹ�
        {
            if( ControlService(schService, SERVICE_CONTROL_STOP, &m_ssServiceStatus) )
            {   //ֱ����SERVICE��STOP�������ܹ�ִ�е����˵��SERVICE������
                //�Ǿ���Ҫֹͣ����ִ�к����ɾ��
                Sleep(3000) ; //��3��ʹϵͳ��ʱ��ִ��STOP����
                while( QueryServiceStatus(schService, &m_ssServiceStatus) )
                {                                            //ѭ�����SERVICE״̬
                    if(m_ssServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
                    {                    //���SERVICE������ִ��(PENDING)ֹͣ����
                        Sleep(1000) ;    //�Ǿ͵�1���Ӻ��ټ��SERVICE�Ƿ�ֹͣOK
                    }else break ;//STOP�������ϣ�����ѭ��
                }//ѭ�����SERVICE״̬����
                if(m_ssServiceStatus.dwCurrentState != SERVICE_STOPPED)
                {                    //���SERVICE����STOP�����û��STOPPED
                    return FALSE;    //�Ǿͷ���FALSE������GetLastErrorȡ�������
                }
            }
            //ɾ��ָ��������
            if(! DeleteService(schService) )    //ɾ�����SERVICE
            {
                CloseServiceHandle(schService);        //�ͷ�SERVICE���ƾ��
                return FALSE;    //���ɾ��ʧ�ܷ���
            }else CloseServiceHandle(schService);    //�ͷ�SERVICE���ƾ��
        }else //ȡSERVICE������ɹ�
        {
            return FALSE; //��ȡSERVICE���ʧ�ܣ���û���ҵ�SERVICE���ַ���
        }
        CloseServiceHandle(schSCManager); //�ͷ�SCM���������
    }else    //�򿪹��������ɹ�
    {
        return FALSE; //�򿪹�����ʧ�ܷ���
    }
    return TRUE; //����ɾ������
}

/*********************************************************************
** ��ϵͳ��ȡ���һ�δ�����룬��ת�����ַ�������
*********************************************************************/
LPTSTR CNTService::GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ARGUMENT_ARRAY,
		NULL,
		GetLastError(),
		LANG_NEUTRAL,
		(LPTSTR)&lpszTemp,
		0,
		NULL );

    // supplied buffer is not long enough
    if(!dwRet||((long)dwSize<(long)dwRet+14))
		lpszBuf[0] = TEXT('\0');
    else{
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
        _stprintf( lpszBuf, TEXT("%s (%ld)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}

/*********************************************************************
** ִ��ȱʡ�ķ������Dispatcher������ʼǰִ�е�ȱʡע��
*********************************************************************/
BOOL CNTService::StartDispatch()
{
    SERVICE_TABLE_ENTRY DispatchTable[] =
    {
        {(LPTSTR)m_sServiceName.c_str(), ServiceMain},
        {NULL, NULL}
    };                            //���ֻ��һ��SERVICE�Ͷ����2�������
	//�������ϵ�SERVICE��һ���ļ�����
	//���Ҫ�����3ά�����
    m_bService = TRUE ;
    if( !StartServiceCtrlDispatcher(DispatchTable))
	{
		DWORD dwLastError = GetLastError();
        return FALSE;
	}
    return TRUE;
}

/*********************************************************************
** �������ʼ����������Ҫ�ĳ�ʼ����������������ڴ�,
** �˺������봦��STOP��PAUSE��CONTINUE���¼�
*********************************************************************/
void WINAPI CNTService::ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    m_sshServiceStatusHandle =
        RegisterServiceCtrlHandler(m_sServiceName.c_str(), ServiceHandle);
    if(!m_sshServiceStatusHandle){
        AddToMessageLog(_T("RegisterServiceCtrlHandler Error!"));
        return ;
    }

    if(SetSCMStatus(SERVICE_START_PENDING, NO_ERROR,3000)){
        m_pService->dwThreadId = GetCurrentThreadId() ;
        if(m_pService->Initialize()){
            m_pService->Run();
            m_pService->UnInitialize();
        }
    }
    SetSCMStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

/*********************************************************************
** �˺�������ϵͳ������STOP��PAUSE����������Ӧ������Service_Main,
** ÿһ��ָ��ִ�к��豨�浱ǰ״̬��SCM������
*********************************************************************/
void WINAPI CNTService::ServiceHandle(DWORD dwControl)
{
    switch(dwControl){
    case SERVICE_CONTROL_STOP:
        SetSCMStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);
        m_pService->Stop() ;
        PostThreadMessage(m_pService->dwThreadId, WM_QUIT, 0, 0 ) ;
        break;
    case SERVICE_CONTROL_PAUSE:
        m_pService->Pause() ;
        break;
    case SERVICE_CONTROL_CONTINUE:
        m_pService->Continue() ;
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        break;
    }
}

/*********************************************************************
** ���ɹ���Ϣ�ӽ�ϵͳEVENT��������
*********************************************************************/
void CNTService::AddToMessageLog(LPCTSTR lpszMsg)
{
    TCHAR    szMsg[256];
    HANDLE   hEventSource;
    LPCTSTR  lpszStrings[2];

    if(!m_pService->m_bService){
        _tprintf(_T("%s"), lpszMsg) ;
        return ;
    }

    // Use event logging to log the error.

    hEventSource = RegisterEventSource(0, m_sServiceName.c_str());

	_stprintf(szMsg, TEXT("\n%s ��ʾ��Ϣ: %d"), m_sServiceDisplayName.c_str(), GetLastError());
	lpszStrings[0] = szMsg;
    lpszStrings[1] = lpszMsg;

    if(hEventSource != NULL){

        ReportEvent(hEventSource,  // handle of event source
			EVENTLOG_SUCCESS,      // event type
			0,                     // event category
			0,                     // event ID
			NULL,                  // current user's SID
			2,                     // number of strings in lpszStrings
			0,                     // no bytes of raw data
			lpszStrings,           // array of error strings
			NULL);                 // no raw data
        (VOID) DeregisterEventSource(hEventSource);
    }
}

/*********************************************************************
** ��������Ϣ�ӽ�ϵͳEVENT��������;
*********************************************************************/
void CNTService::AddToErrorMessageLog(LPCTSTR lpszMsg)
{
    TCHAR    szMsg[256];
    HANDLE   hEventSource;
    LPCTSTR  lpszStrings[2];

    if(!m_pService->m_bService){
        printf("%s", lpszMsg) ;
        return ;
    }
    // Use event logging to log the error.

    hEventSource = RegisterEventSource(0, m_sServiceName.c_str());

    _stprintf(szMsg, TEXT("\n%s ��ʾ��Ϣ: %d"), m_sServiceDisplayName.c_str(), GetLastError());
    lpszStrings[0] = szMsg;
    lpszStrings[1] = lpszMsg;

    if(hEventSource != NULL){

        ReportEvent(hEventSource,		// handle of event source
			EVENTLOG_ERROR_TYPE,		// event type
			0,							// event category
			0,							// event ID
			NULL,						// current user's SID
			2,							// number of strings in lpszStrings
			0,							// no bytes of raw data
			lpszStrings,				// array of error strings
			NULL);						// no raw data
        (VOID) DeregisterEventSource(hEventSource);
    }

}

/*********************************************************************
** ��������Ϣ�ӽ�ϵͳEVENT��������;
*********************************************************************/
void CNTService::AddToWarningMessageLog(LPCTSTR lpszMsg)
{
    TCHAR    szMsg[256];
    HANDLE   hEventSource;
    LPCTSTR  lpszStrings[2];

    if(!m_pService->m_bService){
        _tprintf(_T("%s"), lpszMsg) ;
        return ;
    }

    // Use event logging to log the error.

    hEventSource = RegisterEventSource(0, m_sServiceName.c_str());

    _stprintf(szMsg, TEXT("\n%s ��ʾ��Ϣ: %d"), m_sServiceDisplayName.c_str(), GetLastError());
    lpszStrings[0] = szMsg;
    lpszStrings[1] = lpszMsg;

    if(hEventSource != NULL){

        ReportEvent(hEventSource,    // handle of event source
			EVENTLOG_WARNING_TYPE,   // event type
			0,                       // event category
			0,                       // event ID
			NULL,                    // current user's SID
			2,                       // number of strings in lpszStrings
			0,                       // no bytes of raw data
			lpszStrings,             // array of error strings
			NULL);                   // no raw data
        (VOID) DeregisterEventSource(hEventSource);
    }
}

/*********************************************************************
** ������ǰ״̬�����SCM��������ʹ����������ʾ�뵱ǰ����ͬ��;
*********************************************************************/
BOOL CNTService::SetSCMStatus(DWORD dwCurrentState,
							  DWORD dwWin32ExitCode,
							  DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;

    if (dwCurrentState == SERVICE_START_PENDING)
        m_ssServiceStatus.dwControlsAccepted = 0;
    else
        m_ssServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    m_ssServiceStatus.dwCurrentState = dwCurrentState;
    m_ssServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    m_ssServiceStatus.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) ||
        ( dwCurrentState == SERVICE_STOPPED ) )
        m_ssServiceStatus.dwCheckPoint = 0;
    else
        m_ssServiceStatus.dwCheckPoint = dwCheckPoint++;


    // Report the status of the service to the service control manager.
    //
    if (!(fResult = SetServiceStatus( m_sshServiceStatusHandle, &m_ssServiceStatus))){
        AddToMessageLog(TEXT("SetServiceStatus"));
    }
    return fResult;
}

/*********************************************************************
**
*********************************************************************/
BOOL CNTService::Initialize()
{
    if(!SetSCMStatus(SERVICE_RUNNING, NO_ERROR, 0))
        return FALSE ;

	return TRUE;
}

/*********************************************************************
**
*********************************************************************/
void CNTService::UnInitialize()
{

}

/*********************************************************************
**
*********************************************************************/
void CNTService::Stop()
{
}

/*********************************************************************
**
*********************************************************************/
void CNTService::Pause()
{
}

/*********************************************************************
**
*********************************************************************/
void CNTService::Continue()
{
}

/*********************************************************************
** ������;
*********************************************************************/
void CNTService::Run()
{
	//�ڴ˵��÷����ʵ�ʳ���
	//������Ϣѭ��;
	MSG msg;
	while(GetMessage(&msg,0,0,0)){
		AddToMessageLog(_T("�յ���Ϣ"));
		DispatchMessage(&msg);
	}
}

BOOL CNTService::IsService(void) const
{
	return m_bService;
}

DWORD CNTService::GetCurrentStatus(void) const
{
	return m_ssServiceStatus.dwCurrentState;
}

