/*
	MYCP is a HTTP and C++ Web Application Server.
    ParserSotp is a software library that parse Simple Object Transmission Protocol(SOTP).
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

// ParserSotp.cpp : Defines the exported functions for the DLL application.
#ifdef WIN32
#pragma warning(disable:4267 4996)
#include <CGCLib/CGCLib.h>	// uses for mycp_client
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
#else
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#endif // WIN32

// cgc head
#include <CGCBase/app.h>
#include <CGCLib/cgc_sotpclient.h>
#include <CGCClass/cgcclassinclude.h>
using namespace mycp;

//#ifdef WIN32
//#pragma comment(lib, "libeay32.lib")  
//#pragma comment(lib, "ssleay32.lib") 
//#endif // WIN32
const unsigned int TIMERID_10_SECOND = 3;		// 10秒检查一次

CLockMap<tstring,mycp::CCGCSotpClient::pointer> theModuleSotpClientList;
class CSotpClientService
	: public cgcOnTimerHandler
{
public:
	typedef boost::shared_ptr<CSotpClientService> pointer;
	virtual void OnTimeout(unsigned int nIDEvent, const void * pvParam)
	{
		if (TIMERID_10_SECOND==nIDEvent)
		{
			//printf("**** OnCheckTimeout(size=%d)...\n",(int)theModuleSotpClientList.size());
			const time_t tNow = time(0);
			//int nEraseCount = 0;
			BoostWriteLock wtLock(theModuleSotpClientList.mutex());
			CLockMap<tstring, mycp::CCGCSotpClient::pointer>::iterator pIter=theModuleSotpClientList.begin();
			for (;pIter!=theModuleSotpClientList.end();)
			{
				const mycp::CCGCSotpClient::pointer& pPOPSotpClient = pIter->second;
				const time_t tSendRecvTime = pPOPSotpClient->sotp()->doGetLastSendRecvTime();
				const unsigned int nSendReceiveTimeout = pPOPSotpClient->GetUserData();
				//printf("**** OnCheckTimeout lasttime=%lld, timeout=%d\n",(bigint)tSendRecvTime,nSendReceiveTimeout);
				if (tSendRecvTime > 0 && (tSendRecvTime+(time_t)nSendReceiveTimeout)<tNow)
				{
					// 已经通过 XX 秒没有数据，清空该sotp client
					//printf("**** OnCheckTimeout clear um client\n");
					CGC_LOG((mycp::LOG_TRACE, "OnCheckTimeout clear um client\n"));
					theModuleSotpClientList.erase(pIter++);
					//theModuleSotpClientList.erase(pIter);
					//if ((++nEraseCount)>=500 || theModuleSotpClientList.empty(false))
					//	break;
					//else
					//	pIter = theModuleSotpClientList.begin();
				}else
				{
					pIter++;
				}
			}
		}
	}
};
CSotpClientService::pointer theSotpClientService;

extern "C" bool CGC_API CGC_Module_Init2(MODULE_INIT_TYPE nInitType)
//extern "C" bool CGC_API CGC_Module_Init(void)
{
	if (theSotpClientService.get() != NULL) {
		CGC_LOG((mycp::LOG_ERROR, "CGC_Module_Init2 rerun error, InitType=%d.\n", nInitType));
		return true;
	}
	theSotpClientService = CSotpClientService::pointer(new CSotpClientService());
#if (USES_TIMER_HANDLER_POINTER==1)
	theApplication->SetTimer(TIMERID_10_SECOND, 10*1000, theSotpClientService.get());	// 10秒检查一次
#else
	theApplication->SetTimer(TIMERID_10_SECOND, 10*1000, theSotpClientService);	// 10秒检查一次
#endif
	return true;
}

extern "C" void CGC_API CGC_Module_Free2(MODULE_FREE_TYPE nFreeType)
//extern "C" void CGC_API CGC_Module_Free(void)
{
	theApplication->KillAllTimer();
	theModuleSotpClientList.clear();
	theSotpClientService.reset();
}

extern "C" bool CGC_API CGC_GetSotpClientHandler(DoSotpClientHandler::pointer& pOutSotpClientHandler, const cgcValueInfo::pointer& parameter)
{
	if (parameter.get()==NULL || parameter->getType()!=cgcValueInfo::TYPE_MAP) return false;

	const CLockMap<tstring, cgcValueInfo::pointer>& pInfoList = parameter->getMap();
	cgcValueInfo::pointer pValueInfo;
	// address
	if (!pInfoList.find("address",pValueInfo))
	{
		return false;
	}
	const tstring sAddress(pValueInfo->getStrValue());
	// app_name
	if (!pInfoList.find("app_name",pValueInfo))
	{
		return false;
	}
	const tstring sAppName(pValueInfo->getStrValue());
	// socket_type
	if (!pInfoList.find("socket_type",pValueInfo))
	{
		return false;
	}
	const CCgcAddress::SocketType nAddressSocketType = (CCgcAddress::SocketType)pValueInfo->getIntValue();
	// bind_port
	int nBindPort = 0;
	if (pInfoList.find("bind_port",pValueInfo))
	{
		nBindPort = pValueInfo->getIntValue();
	}
	// thread_stack_size
	int nThreadStackSize = 200;
	if (pInfoList.find("thread_stack_size",pValueInfo))
	{
		nThreadStackSize = pValueInfo->getIntValue();
	}

	const tstring sKey = sAddress+sAppName;
	mycp::CCGCSotpClient::pointer pPOPSotpClient;
	if (theModuleSotpClientList.find(sKey,pPOPSotpClient))	// ***
	{
		if (pPOPSotpClient->sotp()->doIsSessionOpened() && !pPOPSotpClient->sotp()->doIsInvalidate())
		{
			pOutSotpClientHandler = pPOPSotpClient->sotp();
			return true;
		}
		pPOPSotpClient->sotp()->doSendCloseSession();
		theModuleSotpClientList.remove(sKey);
	}

	const CCgcAddress pCgcAddress(sAddress,nAddressSocketType);
	pPOPSotpClient = mycp::CCGCSotpClient::create();
	if (!pPOPSotpClient->StartClient(pCgcAddress,nBindPort,nThreadStackSize) || !pPOPSotpClient->IsClientStarted())
	{
		CGC_LOG((mycp::LOG_ERROR, "StartClient(%s) error.\n",sAddress.c_str()));
		pPOPSotpClient.reset();
		return false;
	}
	//CGC_LOG((mycp::LOG_INFO, "StartClient(%s) appname=%s, socket_type=%d ok\n",sAddress.c_str(),sAppName.c_str(),(int)nAddressSocketType));
	pPOPSotpClient->SetAppName(sAppName);
	pPOPSotpClient->sotp()->doSetConfig(SOTP_CLIENT_CONFIG_SOTP_VERSION,SOTP_PROTO_VERSION_21);
	// user_ssl
	bool bUseSsl = false;
	if (pInfoList.find("uses_ssl",pValueInfo))
	{
		bUseSsl = pValueInfo->getIntValue()==1?true:false;
	}
	pPOPSotpClient->sotp()->doSetConfig(SOTP_CLIENT_CONFIG_USES_SSL,(int)(bUseSsl?1:0));
	// encoding
	if (pInfoList.find("encoding",pValueInfo))
	{
		pPOPSotpClient->sotp()->doSetEncoding(pValueInfo->getStrValue());
	}
	// open_session
	if (pInfoList.find("open_session",pValueInfo) && pValueInfo->getIntValue()==1)
	{
		pPOPSotpClient->sotp()->doSendOpenSession();
		if (nAddressSocketType==CCgcAddress::ST_TCP)
			pPOPSotpClient->sotp()->doStartActiveThread(30);
		else
			pPOPSotpClient->sotp()->doStartActiveThread(20);
	}else if (bUseSsl)	// * for request ssl_password
	{
		pPOPSotpClient->sotp()->doSendOpenSession();
	}
	// timeout
	int nTimeout = 30;
	if (pInfoList.find("timeout",pValueInfo))
	{
		nTimeout = pValueInfo->getIntValue();
	}
	pPOPSotpClient->SetUserData(nTimeout);
	pOutSotpClientHandler = pPOPSotpClient->sotp();
	theModuleSotpClientList.insert(sKey,pPOPSotpClient,false);
	return true;
}
