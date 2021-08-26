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

// CGCApp.h file here
#ifndef _CGCApp_HEAD_VER_1_0_0_0__INCLUDED_
#define _CGCApp_HEAD_VER_1_0_0_0__INCLUDED_

#define CGC_IMPORTS 1

//#define USES_OPENSSL
//#include "CgcTcpClient.h"
#include "IncludeBase.h"
#include <CGCLib/CgcClientHandler.h>
#include <CGCBase/cgcSeqInfo.h>
#include <CGCBase/cgcfunc.h>

//#include "ACELib/inc/SockServer.h"

#include <CGCClass/AttributesImpl.h>
#include <CGCClass/SotpRtpSession.h>
//#include "AttributesImpl.h"
#include "XmlParseDefault.h"
#include "XmlParseModules.h"
#include "XmlParseParams.h"
#include "XmlParseCdbcs.h"
#include "XmlParseAllowMethods.h"
#include "XmlParseAuths.h"
#include "XmlParseWeb.h"
#include "XmlParseAutoUpdate.h"
//#include "XmlParseClusters.h"
#include "ModuleMgr.h"
#include "SessionMgr.h"
#include "DataServiceMgr.h"
#include "SotpRequestImpl.h"
#include "SotpResponseImpl.h"
#include "HttpRequestImpl.h"
#include "HttpResponseImpl.h"
#include "ResponseImpl.h"
#include <ThirdParty/stl/rsa.h>

#define SCRIPT_TYPE_ONCE	0
#define SCRIPT_TYPE_HOURLY	1
#define SCRIPT_TYPE_DAILY	2
#define SCRIPT_TYPE_WEEKLY	3
#define SCRIPT_TYPE_MONTHLY	4
#define SCRIPT_TYPE_YEARLY	5

namespace mycp {

class CMySessionInfo
{
public:
	typedef std::shared_ptr<CMySessionInfo> pointer;
	static CMySessionInfo::pointer create(const tstring & sMySessionId,const tstring& sUserAgent)
	{
		return CMySessionInfo::pointer(new CMySessionInfo(sMySessionId,sUserAgent));
	}

	CMySessionInfo(void)
		: m_tRequestTime(0)
	{
	}
	CMySessionInfo(const tstring& sMySessionId,const tstring& sUserAgent)
		: m_sMySessionId(sMySessionId)
		, m_sUserAgent(sUserAgent)
	{
		m_tRequestTime = time(0);
	}
	const tstring& GetMySessionId(void) const {return m_sMySessionId;}
	const tstring& GetUserAgent(void) const {return m_sUserAgent;}
	const time_t GetRequestTime(void) const {return m_tRequestTime;}
	void UpdateRequestTime(void) {m_tRequestTime = time(0);}
private:
	tstring m_sMySessionId;
	tstring m_sUserAgent;
	time_t m_tRequestTime;
};
class CNotKeepAliveRemote
{
public:
	typedef std::shared_ptr<CNotKeepAliveRemote> pointer;
	static CNotKeepAliveRemote::pointer create(const cgcRemote::pointer& pcgcRemote)
	{
		return CNotKeepAliveRemote::pointer(new CNotKeepAliveRemote(pcgcRemote));
	}
	bool IsExpireTime(time_t tNow, int nExpireSecond=2) const {return tNow-m_tRequestTime>nExpireSecond;}
	CNotKeepAliveRemote(const cgcRemote::pointer& pcgcRemote)
		: m_pcgcRemote(pcgcRemote)
	{
		m_tRequestTime = time(0);
	}
	virtual ~CNotKeepAliveRemote(void)
	{
		m_pcgcRemote->invalidate(true);
	}
private:
	cgcRemote::pointer m_pcgcRemote;
	time_t m_tRequestTime;
};
#define USES_CMODULEMGR
class CGCApp
	: public cgcSystem
#ifndef USES_MODULE_SERVICE_MANAGER
	, public cgcServiceManager
#endif
	, public CModuleHandler
	, public cgcCommHandler
	, public cgcParserCallback
	, public CSotpRtpCallback	// for sotp rtp
	//, public TcpClient_Callback	// for tcp
	, public CParserSotpHandler
	, public CParserHttpHandler
	, public std::enable_shared_from_this<CGCApp>
{
public:
	typedef std::shared_ptr<CGCApp> pointer;

	static CGCApp::pointer create(const tstring & sPath)
	{
		return CGCApp::pointer(new CGCApp(sPath));
	}

	CGCApp(const tstring & sPath);
	virtual ~CGCApp(void);

public:
	int MyMain(int nWaitSeconds,bool bService = false, const std::string& sProtectDataFile="");

	bool GetIsService(void) const {return m_bService;}
	const tstring& GetProtectDataFile(void) const {return m_sProtectDataFile;}

	bool isInited(void) const {return m_bInitedApp;}
	bool isExitLog(void) const {return m_bExitLog;}
	void ProcNotKeepAliveRmote(void);
	void ProcLastAccessedTime(void);
	bool ProcDataResend(void) {return this->m_mgrSession1.ProcDataResend();}
	void ProcCheckParserPool(void);
	void ProcCheckAutoUpdate(void);
	void DetachAutoUpdateThread(void);

	void AppInit(bool bNTService = true);
	void AppStart(int nWaitSeconds);
	void AppStop(void);
	void AppExit(void);
	void PrintHelp(void);
	void CheckScriptExecute(int nScriptType);

	// CModuleHandler
	virtual void* onGetFPGetSotpClientHandler(void) const {return (void*)m_fpGetSotpClientHandler;}

private:
	void do_dataresend(void);
	void do_sessiontimeout(void);
	void FreeLibModule(const ModuleItem::pointer& moduleItem, bool bEraseApplication, Module_Free_Type nFreeType);
	void RemoveAllAutoUpdateFile(const XmlParseAutoUpdate& pAutoUpdate);
	void do_autoupdate(void);

	void LoadDefaultConf(void);
	void LoadClustersConf(void);
	void LoadAuthsConf(void);
	void LoadModulesConf(void);
	void LoadSystemParams(void);

	// CSotpRtpCallback
	bool setPortAppModuleHandle(const CPortApp::pointer& portApp);
	virtual bool onRegisterSource(bigint nRoomId, bigint nSourceId, bigint nParam, void* pUserData);
	virtual void onUnRegisterSource(bigint nRoomId, bigint nSourceId, bigint nParam, void* pUserData);
	virtual bool onRegisterSink(bigint nRoomId, bigint nSourceId, bigint nDestId, void* pUserData);

	// cgcParserCallback 
	virtual tstring onGetSslPrivateKey(void) const {return "";}
	virtual tstring onGetSslPrivatePwd(void) const {return "";}
	//virtual tstring onGetSslPrivateKey(void) const {return m_pRsa.GetPrivateKey();}
	//virtual tstring onGetSslPrivatePwd(void) const {return m_pRsa.GetPrivatePwd();}
	virtual tstring onGetSslPassword(const tstring& sSessionId) const;

	// CParserSotpHandler
	virtual void onReturnParserSotp(cgcParserSotp::pointer cgcParser);
	// CParserHttpHandler
	virtual void onReturnParserHttp(cgcParserHttp::pointer cgcParser);

	// cgcCommHandler handler
	virtual int onRemoteAccept(const cgcRemote::pointer& pcgcRemote);
	virtual int onRecvData(const cgcRemote::pointer& pcgcRemote, const unsigned char * recvData, size_t recvLen);
	virtual int onRemoteClose(unsigned long remoteId, int nErrorCode);

#ifdef USES_MODULE_SERVICE_MANAGER
	virtual void exitModule(const CModuleImpl* pFromModuleImpl);
	virtual cgcCDBCInfo::pointer getCDBDInfo(const CModuleImpl* pFromModuleImpl, const tstring& datasource) const;
	virtual cgcCDBCService::pointer getCDBDService(const CModuleImpl* pFromModuleImpl, const tstring& datasource);
	virtual void retCDBDService(const CModuleImpl* pFromModuleImpl, cgcCDBCServicePointer& cdbcservice);
	virtual cgcServiceInterface::pointer getService(const CModuleImpl* pFromModuleImpl, const tstring & serviceName, const cgcValueInfo::pointer& parameter = cgcNullValueInfo);
	void SendModuleResetService(const CModuleImpl* pSentToModuleImpl,const tstring& sServiceName);
	void ResetService(void * hModule, const cgcServiceInterface::pointer & service);
	virtual void resetService(const CModuleImpl* pFromModuleImpl, const cgcServiceInterface::pointer & service);
	virtual HTTP_STATUSCODE executeInclude(const CModuleImpl* pFromModuleImpl, const tstring & url, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response);
	virtual HTTP_STATUSCODE executeService(const CModuleImpl* pFromModuleImpl, const tstring & serviceName, const tstring& function, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response, tstring & outExecuteResult);
#else
	// cgcServiceManager handler
	virtual cgcCDBCInfo::pointer getCDBDInfo(const tstring& datasource) const;
	virtual cgcCDBCService::pointer getCDBDService(const tstring& datasource);
	virtual void retCDBDService(cgcCDBCServicePointer& cdbcservice);
	virtual cgcServiceInterface::pointer getService(const tstring & serviceName, const cgcValueInfo::pointer& parameter = cgcNullValueInfo);
	virtual void resetService(const cgcServiceInterface::pointer & service);
	virtual HTTP_STATUSCODE executeInclude(const tstring & url, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response);
	virtual HTTP_STATUSCODE executeService(const tstring & serviceName, const tstring& function, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response, tstring & outExecuteResult);
#endif
	// cgcSystem handler
	virtual cgcParameterMap::pointer getInitParameters(void) const;
	//virtual cgcCDBCInfo::pointer getCDBCInfo(const tstring& name) const {return m_cdbcs.getCDBCInfo(name);}

	virtual const tstring & getServerPath(void) const {return m_sModulePath;}
	virtual const tstring & getServerName(void) const {return m_parseDefault.getCgcpName();}
	virtual const tstring & getServerAddr(void) const {return m_parseDefault.getCgcpAddr();}
	virtual const tstring & getServerCode(void) const {return m_parseDefault.getCgcpCode();}
	virtual int getServerRank(void) const {return m_parseDefault.getCgcpRank();}
	virtual cgcSession::pointer getSession(const tstring & sessionId) const {return sessionId.size()==DEFAULT_HTTP_SESSIONID_LENGTH?m_mgrSession2.GetSessionImpl(sessionId):m_mgrSession1.GetSessionImpl(sessionId);}
	virtual cgcResponse::pointer getLastResponse(const tstring & sessionId,const tstring& moduleName) const;
	virtual cgcResponse::pointer getHoldResponse(const tstring& sessionId,unsigned long remoteId);

	virtual cgcAttributes::pointer getAttributes(bool create);
	virtual cgcAttributes::pointer getAttributes(void) const {return m_attributes;}
	virtual cgcAttributes::pointer getAppAttributes(const tstring & appName) const;

	tstring SetNewMySessionId(cgcParserHttp::pointer& phttpParser,const tstring& sSessionId="");
	void GetHttpParserPool(cgcParserHttp::pointer& phttpParser);
	void SetHttpParserPool(const cgcParserHttp::pointer& phttpParser);
	void CheckHttpParserPool(void);
	//CgcTcpClient::pointer m_pFastCgiServer;
	//virtual void OnReceiveData(const ReceiveBuffer::pointer& data);
	HTTP_STATUSCODE ProcHttpData(const unsigned char * recvData, size_t dataSize,const cgcRemote::pointer& pcgcRemote);
	HTTP_STATUSCODE ProcHttpAppProto(const tstring& sAppModuleName,const cgcHttpRequest::pointer& pRequestImpl,const cgcHttpResponse::pointer& pResponseImpl,const cgcParserHttp::pointer& pcgcParser);
	HTTP_STATUSCODE ProcHttpLibMethod(const ModuleItem::pointer& moduleItem,const tstring& sMethodName,const cgcHttpRequest::pointer& pRequest,const cgcHttpResponse::pointer& pResponse);
	void GetSotpParserPool(cgcParserSotp::pointer& pcgcParser);
	void SetSotpParserPool(const cgcParserSotp::pointer& pcgcParser);
	void CheckSotpParserPool(void);
	int ProcCgcData(const unsigned char * recvData, size_t dataSize, const cgcRemote::pointer& pcgcRemote);
	// pRemoteSessionImpl 可以为空；
	int ProcSesProto(const cgcSotpRequest::pointer& pRequestImpl, const cgcParserSotp::pointer& pcgcParser, const cgcRemote::pointer& pcgcRemote, cgcSession::pointer& pRemoteSessionImpl);
	int ProcSyncProto(const cgcSotpRequest::pointer& pRequestImpl, const cgcParserSotp::pointer& pcgcParser, const cgcRemote::pointer& pcgcRemote, cgcSession::pointer& pRemoteSessionImpl);
	int ProcAppProto(const cgcSotpRequest::pointer& pRequestImpl, const cgcSotpResponse::pointer& pResponseImpl, const cgcParserSotp::pointer& pcgcParser, cgcSession::pointer& pRemoteSessionImpl);
	int ProcLibMethod(const ModuleItem::pointer& moduleItem, const tstring & sMethodName, const cgcSotpRequest::pointer& pRequest, const cgcSotpResponse::pointer& pResponse);
	int ProcLibSyncMethod(const ModuleItem::pointer& moduleItem, const tstring& sSyncName, int nSyncType, const tstring & sSyncData);

	void SetModuleHandler(void* hModule,const CModuleImpl::pointer& pModuleImpl);
	//void SetModuleHandler(void* hModule,const CModuleImpl::pointer& pModuleImpl,bool* pOutModuleUpdateKey=NULL);
	bool OpenModuleLibrary(const ModuleItem::pointer& moduleItem);
	//bool OpenModuleLibrary(const ModuleItem::pointer& moduleItem,const cgcApplication::pointer& pUpdateFromApplication);
	void OpenLibrarys(void);
	void FreeLibrarys(void);
	void InitLibModules(unsigned int mt);
	bool InitLibModule(void* hModule, const cgcApplication::pointer& moduleImpl, Module_Init_Type nInitType=MODULE_INIT_TYPE_NORMAL);
	bool InitLibModule(const cgcApplication::pointer& pModuleImpl, const ModuleItem::pointer& moduleItem, Module_Init_Type nInitType=MODULE_INIT_TYPE_NORMAL);
	void FreeLibModules(unsigned int mt);
	void FreeLibModule(void* hModule, Module_Free_Type nFreeType);
	void FreeLibModule(const cgcApplication::pointer& pModuleImpl, Module_Free_Type nFreeType);

private:
	XmlParseDefault m_parseDefault;
	XmlParseModules m_parseModules;
	XmlParsePortApps m_parsePortApps;
	XmlParseWeb m_parseWeb;
//	XmlParseClusters m_parseClusters;
//	XmlParseAuths m_parseAuths;

	//CRSA m_pRsa;
	CLockMap<int,CSotpRtpSession::pointer> m_pRtpSession;
	//CSotpRtpSession m_pRtpSession;

	//
	time_t m_tLastSystemParams;
	XmlParseParams m_systemParams;
	XmlParseCdbcs m_cdbcs;
	CDataServiceMgr m_cdbcServices;
#ifdef USES_MODULE_SERVICE_MANAGER
	//CLockMultiMap<const cgcCDBCService*,const CModuleImpl*> m_pFromCDBCList;
	CLockMultiMap<const cgcServiceInterface*,const CModuleImpl*> m_pFromServiceList;
#endif
	cgcAttributes::pointer m_attributes;

	CModuleImpl m_logModuleImpl;
	CSessionMgr m_mgrSession1;
	CSessionMgr m_mgrSession2;	// for http
	//CHttpSessionMgr m_mgrHttpSession;

	tstring m_sModulePath;
	std::shared_ptr<boost::thread> m_pProcSessionTimeout;
	std::shared_ptr<boost::thread> m_pProcDataResend;
	std::shared_ptr<boost::thread> m_pProcAutoUpdate;
	//CLockMap<short, cgcSeqInfo::pointer> m_mapSeqInfo;

	bool m_bService;
	tstring m_sProtectDataFile;
	bool m_bInitedApp;
	bool m_bStopedApp;
	bool m_bExitLog;

	bool m_bLicensed;				// default true
	tstring m_sLicenseAccount;
	int m_licenseModuleCount;

	FPCGC_GetSotpClientHandler m_fpGetSotpClientHandler;
	FPCGC_GetService m_fpGetLogService;
	FPCGC_ResetService m_fpResetLogService;
	FPCGC_GetService m_fpParserSotpService;
	FPCGC_GetService m_fpParserHttpService;
	//FPCGCHttpApi m_fpHttpStruct;
	FPCGCHttpApi m_fpHttpServer;
	tstring m_sHttpServerName;

	CLockMap<unsigned long, bool> m_mapRemoteOpenSes;		// remoteid->boost::mutex
	CLockMap<cgcServiceInterface::pointer, void*> m_mapServiceModule;		// cgcService->MODULE HANDLE

	CLockMap<unsigned long, cgcMultiPart::pointer> m_mapMultiParts;		// remoteid->
#ifdef USES_CMODULEMGR
	CModuleMgr m_pModuleMgr;
#else
	CLockMap<void*, cgcApplication::pointer> m_mapOpenModules;			//
#endif
	//CLockMap<tstring,CMySessionInfo::pointer> m_pMySessionInfoList;		// mysessionid-> (后期考虑保存到硬盘)
	CLockList<CNotKeepAliveRemote::pointer> m_pNotKeepAliveRemoteList;
	CLockList<cgcParserSotp::pointer> m_pSotpParserPool;
	time_t m_tLastNewParserSotpTime;
	CLockList<cgcParserHttp::pointer> m_pHttpParserPool;
	time_t m_tLastNewParserHttpTime;
};

} // namespace mycp

#endif // _CGCApp_HEAD_VER_1_0_0_0__INCLUDED_
