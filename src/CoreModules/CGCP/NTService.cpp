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
** 文件名: 后台服务器操作;
** 描述:   主函数名:Run();
** 功能:   启动服务线程,接收服务管理器控制消息.
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
std::wstring CNTService::m_sServiceName;        //服务程序内部名称，名字不能有空格
std::wstring CNTService::m_sServiceDisplayName; //SERVICE程序在管理器中的显示名称，任意字符
std::wstring CNTService::m_sServiceDescription; // 服务功能描述
#else
std::string CNTService::m_sServiceName;        //服务程序内部名称，名字不能有空格
std::string CNTService::m_sServiceDisplayName; //SERVICE程序在管理器中的显示名称，任意字符
std::string CNTService::m_sServiceDescription; // 服务功能描述
#endif // _UNICODE

TCHAR CNTService::m_lpServicePathName[512];    //SERVICE程序的EXE文件路径
CNTService * CNTService::m_pService;
SERVICE_STATUS			CNTService::m_ssServiceStatus ;//SERVICE程序的状态struct
SERVICE_STATUS_HANDLE	CNTService::m_sshServiceStatusHandle ;//SERVICE程序状态的HANDLE

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNTService::CNTService()
{

    GetModuleFileName(NULL, m_lpServicePathName, 512); //取当前执行文件路径

    m_sServiceName           = _T("Service");  //服务名称
    m_sServiceDisplayName    = _T("服务器");  //显示的名称
	m_sServiceDescription    = _T("");
    m_pService = this ;
    m_ssServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_ssServiceStatus.dwServiceSpecificExitCode = 0 ;
}

CNTService::~CNTService()
{

}

/*********************************************************************
** 将执行文件安装进NT SERVICE管理器内
*********************************************************************/
BOOL CNTService::InstallService()
{
    SC_HANDLE schSCManager;      //打开SCM用的句柄
    SC_HANDLE schService;        //建立SERVICE用的句柄

    schSCManager = OpenSCManager(        //调用打开SERVICE管理器API
		NULL,        //机器名称，设置NULL为本地机器
		NULL,        //数据库名称，NULL为缺省数据库
		SC_MANAGER_CREATE_SERVICE //或 SC_MANAGER_ALL_ACCESS
		);                      //希望打开的操作权限，详见MSDN

    if( schSCManager )        // 如果打开SERVICE管理器成功
    {
        schService = CreateService(        //调用建立SERVICE的API
            schSCManager,                //SERVICE管理器数据库的句柄
			CNTService::m_sServiceName.c_str(),            //服务名称
			CNTService::m_sServiceDisplayName.c_str(),        //显示的名称
            SERVICE_ALL_ACCESS,            //希望得到的运行权限
            SERVICE_WIN32_OWN_PROCESS,    //SERVICE的类型
            SERVICE_AUTO_START,            //启动的方式
            SERVICE_ERROR_NORMAL,        //错误控制类型
            m_lpServicePathName,        //可执行文件的路径名
            NULL,        //一组服务装入时的顺序
            NULL,        //检查人标记
            _T("RPCSS\0"),        //从属
            NULL,        //本地USER名
            NULL);        //密码

        if( schService )    //如果建立SERVICE成功
        {
			//修改描述
			SERVICE_DESCRIPTION sd;
			if (!CNTService::m_sServiceDescription.empty())
				sd.lpDescription = (LPTSTR)CNTService::m_sServiceDescription.c_str();
			else
				sd.lpDescription = (LPTSTR)CNTService::m_sServiceDisplayName.c_str();
			ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd);
			//ChangeServiceConfig2(schService,0, NULL);
            CloseServiceHandle(schService);        //释放SERVICE句柄，准备退出
        }else
        {
            return FALSE ; //建立SERVICE失败返回
        }
        CloseServiceHandle(schSCManager);        //释放SERVICE管理器句柄
    }else
    {
        return FALSE; //打开管理器失败返回
    }
    return TRUE; //一切正常返回
}

/*********************************************************************
** 将SERVICE程序从SCM管理器中移走
*********************************************************************/
BOOL CNTService::RemoveService()
{
    SC_HANDLE        schSCManager;    //打开SCM用的句柄
    SC_HANDLE        schService;        //建立SERVICE用的句柄

    schSCManager = OpenSCManager(        //调用打开SERVICE管理器API
		NULL,        //机器名称，设置NULL为本地机器
		NULL,        //数据库名称，NULL为缺省数据库
		SC_MANAGER_ALL_ACCESS //希望打开的操作权限，详见MSDN
		);

    if( schSCManager )        // 如果打开SERVICE管理器成功
    {
        schService = OpenService(    //获取SERVICE控制句柄的API
            schSCManager,            //SCM管理器句柄
            m_sServiceName.c_str(),        //SERVICE内部名称，控制名称
            SERVICE_ALL_ACCESS);    //打开的权限，删除就要全部权限

        if( schService )    //如果获取SERVICE句柄成功
        {
            if( ControlService(schService, SERVICE_CONTROL_STOP, &m_ssServiceStatus) )
            {   //直接向SERVICE发STOP命令，如果能够执行到这里，说明SERVICE正运行
                //那就需要停止程序执行后才能删除
                Sleep(3000) ; //等3秒使系统有时间执行STOP命令
                while( QueryServiceStatus(schService, &m_ssServiceStatus) )
                {                                            //循环检查SERVICE状态
                    if(m_ssServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
                    {                    //如果SERVICE还正在执行(PENDING)停止任务
                        Sleep(1000) ;    //那就等1秒钟后再检查SERVICE是否停止OK
                    }else break ;//STOP命令处理完毕，跳出循环
                }//循环检查SERVICE状态结束
                if(m_ssServiceStatus.dwCurrentState != SERVICE_STOPPED)
                {                    //如果SERVICE接受STOP命令后还没有STOPPED
                    return FALSE;    //那就返回FALSE报错，用GetLastError取错误代码
                }
            }
            //删除指令在这里
            if(! DeleteService(schService) )    //删除这个SERVICE
            {
                CloseServiceHandle(schService);        //释放SERVICE控制句柄
                return FALSE;    //如果删除失败返回
            }else CloseServiceHandle(schService);    //释放SERVICE控制句柄
        }else //取SERVICE句柄不成功
        {
            return FALSE; //获取SERVICE句柄失败，或没有找到SERVICE名字返回
        }
        CloseServiceHandle(schSCManager); //释放SCM管理器句柄
    }else    //打开管理器不成功
    {
        return FALSE; //打开管理器失败返回
    }
    return TRUE; //正常删除返回
}

/*********************************************************************
** 从系统中取最后一次错误代码，并转换成字符串返回
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
** 执行缺省的服务控制Dispatcher，服务开始前执行的缺省注册
*********************************************************************/
BOOL CNTService::StartDispatch()
{
    SERVICE_TABLE_ENTRY DispatchTable[] =
    {
        {(LPTSTR)m_sServiceName.c_str(), ServiceMain},
        {NULL, NULL}
    };                            //如果只有一个SERVICE就定义成2，如果有
	//二个以上的SERVICE在一个文件里，这个
	//表就要定义成3维或更多
    m_bService = TRUE ;
    if( !StartServiceCtrlDispatcher(DispatchTable))
	{
		DWORD dwLastError = GetLastError();
        return FALSE;
	}
    return TRUE;
}

/*********************************************************************
** 服务程序开始的主程序，主要的初始化及主体服务程序均在此,
** 此函数内须处理STOP、PAUSE、CONTINUE等事件
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
** 此函数接受系统发来的STOP、PAUSE，并做出相应处理发给Service_Main,
** 每一个指令执行后需报告当前状态给SCM管理器
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
** 将成功信息加进系统EVENT管理器中
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

	_stprintf(szMsg, TEXT("\n%s 提示信息: %d"), m_sServiceDisplayName.c_str(), GetLastError());
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
** 将错误信息加进系统EVENT管理器中;
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

    _stprintf(szMsg, TEXT("\n%s 提示信息: %d"), m_sServiceDisplayName.c_str(), GetLastError());
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
** 将警告信息加进系统EVENT管理器中;
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

    _stprintf(szMsg, TEXT("\n%s 提示信息: %d"), m_sServiceDisplayName.c_str(), GetLastError());
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
** 将程序当前状态报告给SCM管理器，使管理器的显示与当前程序同步;
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
** 主函数;
*********************************************************************/
void CNTService::Run()
{
	//在此调用服务的实际程序
	//进入消息循环;
	MSG msg;
	while(GetMessage(&msg,0,0,0)){
		AddToMessageLog(_T("收到消息"));
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

