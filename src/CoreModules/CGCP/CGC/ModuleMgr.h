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

// ModuleMgr.h file here
#ifndef _MODULEMGR_HEAD_INCLUDED__
#define _MODULEMGR_HEAD_INCLUDED__

#include "IncludeBase.h"
#include <CGCBase/cgcApplication2.h>
#include "XmlParseParams.h"
//#include "XmlParseLocks.h"
#include <CGCClass/AttributesImpl.h>
#include "TimerInfo.h"
//#include <CGCLib/cgc_sotpclient.h>

#define USES_FPCGC_GET_SOTP_CLIENT

namespace mycp {

class CSyncDataInfo
{
public:
	typedef std::shared_ptr<CSyncDataInfo> pointer;
	static CSyncDataInfo::pointer create(bigint nId, int nType, const std::string& sName, bool bBackup)
	{
		return CSyncDataInfo::pointer(new CSyncDataInfo(nId, nType, sName, bBackup));
	}
	static CSyncDataInfo::pointer create(bigint nId, int nType, const std::string& sName, const std::string& sData, bool bBackup)
	{
		return CSyncDataInfo::pointer(new CSyncDataInfo(nId, nType, sName, sData, bBackup));
	}
	bigint m_nDataId;
	int m_nType;
	std::string m_sName;
	std::string m_sData;
	bool m_bBackup;
	bool m_bFromDatabase;

	CSyncDataInfo(bigint nId, int nType, const std::string& sName, const std::string& sData, bool bBackup)
		: m_nDataId(nId), m_nType(nType), m_sName(sName), m_sData(sData), m_bBackup(bBackup)
		, m_bFromDatabase(false)
	{}
	CSyncDataInfo(bigint nId, int nType, const std::string& sName, bool bBackup)
		: m_nDataId(nId), m_nType(nType), m_sName(sName), m_bBackup(bBackup)
		, m_bFromDatabase(false)
	{}
	CSyncDataInfo(void)
		: m_nDataId(0), m_nType(0), m_bBackup(false)
		, m_bFromDatabase(false)
	{}
};
class CCGCParameterList
	: public cgcParameterMap
{
public:
	typedef std::shared_ptr<CCGCParameterList> pointer;
	static CCGCParameterList::pointer create(void)
	{
		return CCGCParameterList::pointer(new CCGCParameterList());
	}
	void SetParameter(const cgcParameter::pointer & pParameter, bool bSetForce = true)
	{
		if (pParameter.get() != NULL)
		{
			if (!bSetForce && pParameter->getType() == cgcValueInfo::TYPE_STRING && pParameter->empty())
				return;
			cgcParameterMap::insert(pParameter->getName(), pParameter);
		}
	}
};
const CCGCParameterList NullCGCParameterList;

//class CPOPSotpResponseInfo
//	: public cgcObject
//{
//public:
//	typedef std::shared_ptr<CPOPSotpResponseInfo> pointer;
//	static CPOPSotpResponseInfo::pointer create(unsigned long nCallId, unsigned long nCallSign, int nResultValue)
//	{
//		return CPOPSotpResponseInfo::pointer(new CPOPSotpResponseInfo(nCallId, nCallSign, nResultValue));
//	}
//	CPOPSotpResponseInfo(unsigned long nCallId, unsigned long nCallSign, int nResultValue)
//		: m_nCallId(nCallId)
//		, m_nCallSign(nCallSign)
//		, m_nResultValue(nResultValue)
//	{
//	}
//
//	unsigned long GetCallId(void) const {return m_nCallId;}
//	unsigned long GetCallSign(void) const {return m_nCallSign;}
//	int GetResultValue(void) const {return m_nResultValue;}
//	time_t GetResponseTime(void) const {return m_nResponseTime;}
//	CCGCParameterList m_pResponseList;
//	cgcAttachment::pointer m_pAttachMent;
//private:
//	unsigned long	m_nCallId;
//	unsigned long	m_nCallSign;
//	int				m_nResultValue;
//	time_t			m_nResponseTime;
//};
class CCGCSotpRequestInfo
{
public:
	typedef std::shared_ptr<CCGCSotpRequestInfo> pointer;
	static CCGCSotpRequestInfo::pointer create(unsigned long nCallId)
	{
		return CCGCSotpRequestInfo::pointer(new CCGCSotpRequestInfo(nCallId));
	}
	CCGCSotpRequestInfo(unsigned long nCallId)
		: m_nCallId(nCallId)
	{
		//m_nRequestTime = time(0);
		//m_nRequestTimeout = 20;
		//m_nResponseTime = 0;
	}
	virtual ~CCGCSotpRequestInfo(void)
	{
		//m_pAttachList.clear();
	}
	CSyncDataInfo::pointer m_pSyncDataInfo;

	void SetCallId(unsigned long value) {m_nCallId = value;}
	unsigned long GetCallId(void) const {return m_nCallId;}
	//void SetRequestTime(time_t value = time(0)) {m_nRequestTime = value;}
	//time_t GetRequestTime(void) const {return m_nRequestTime;}
	//void SetRequestTimeout(int v) {m_nRequestTimeout = v;}
	//int	GetRequestTimeout(void) const {return m_nRequestTimeout;}
	//void SetResponseTime(time_t value = time(0)) {m_nResponseTime = value;}
	//time_t GetResponseTime(void) const {return m_nResponseTime;}
	CCGCParameterList m_pRequestList;
private:
	unsigned long	m_nCallId;
	//time_t			m_nRequestTime;
	//int				m_nRequestTimeout;
	//time_t			m_nResponseTime;
};
const CCGCSotpRequestInfo::pointer NullCGCSotpRequestInfo;

#define USES_MODULE_SERVICE_MANAGER
class CModuleImpl;
class CModuleHandler
{
public:
	virtual void* onGetFPGetSotpClientHandler(void) const = 0;

#ifdef USES_MODULE_SERVICE_MANAGER
	virtual void exitModule(const CModuleImpl* pFromModuleImpl) = 0;
	virtual cgcServiceInterface::pointer getService(const CModuleImpl* pFromModuleImpl, const tstring & serviceName, const cgcValueInfo::pointer& parameter = cgcNullValueInfo)=0;
	virtual void resetService(const CModuleImpl* pFromModuleImpl, const cgcServiceInterface::pointer & service) = 0;
	virtual cgcCDBCInfo::pointer getCDBDInfo(const CModuleImpl* pFromModuleImpl, const tstring& datasource) const = 0;
	virtual cgcCDBCService::pointer getCDBDService(const CModuleImpl* pFromModuleImpl, const tstring& datasource) = 0;
	virtual void retCDBDService(const CModuleImpl* pFromModuleImpl, cgcCDBCServicePointer& cdbcservice) = 0;
	virtual HTTP_STATUSCODE executeInclude(const CModuleImpl* pFromModuleImpl, const tstring & url, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response) = 0;
	virtual HTTP_STATUSCODE executeService(const CModuleImpl* pFromModuleImpl, const tstring & serviceName, const tstring& function, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response, tstring & outExecuteResult) = 0;
#endif
};

// CModuleImpl
class CModuleImpl
	: public cgcApplication2
#ifdef USES_MODULE_SERVICE_MANAGER
	, public cgcServiceManager
#endif
	, public CgcClientHandler
{
public:
#ifdef USES_MODULE_SERVICE_MANAGER
	typedef std::shared_ptr<CModuleImpl> pointer;
#endif
private:
	ModuleItem::pointer m_module;
	cgcAttributes::pointer m_attributes;
	unsigned long m_callRef;					// for lockstate
	TimerTable m_timerTable;			// for timer
	tstring m_sModulePath;
	cgcLogService::pointer m_logService;

	int m_moduleState;	// 0:LoadLibrary; 1:load xml and init()	-1 : free
	bool m_nInited;
	CLockMap<tstring,bool> m_pDataSourceList;

	tstring m_sExecuteResult;
	boost::mutex m_mutexLog1;
	boost::mutex m_mutexLog2;
	char* m_debugmsg1;
	wchar_t* m_debugmsg2;
	// for sync
	cgcCDBCServicePointer m_pSyncCdbcService;
#ifndef USES_MODULE_SERVICE_MANAGER
	cgcServiceManager* m_pServiceManager;
#endif
	CModuleHandler* m_pModuleHandler;
	bool m_pSyncErrorStoped;
	bool m_pSyncThreadKilled;
	std::shared_ptr<boost::thread> m_pSyncThread;
	CLockList<CSyncDataInfo::pointer> m_pSyncList;
	//CCGCSotpClient::pointer m_pCGCSotpClient;
	std::vector<tstring> m_pHostList;
	CLockMap<tstring,mycp::DoSotpClientHandler::pointer> m_pSyncSotpClientList;
	time_t m_tProcSyncHostsTime;
	bool m_bLoadBackupFromCdbcService;
	CLockMap<unsigned long, CCGCSotpRequestInfo::pointer> m_pSyncRequestList;			// callid->
	// CgcClientHandler
	virtual void OnCgcResponse(const cgcParserSotp & response);
	virtual void OnCidTimeout(unsigned long callid, unsigned long sign, bool canResendAgain);
	void ProcSyncResponse(unsigned long nCallId, int nResultValue);
	bool ProcSyncHosts(void);
	void StartSyncThread(void);

	boost::mutex m_mutexCid;
	unsigned long m_nCurrentCallId;
	unsigned long getNextCallId(void);

//#ifndef USES_FPCGC_GET_SOTP_CLIENT
//	CLockMap<tstring,mycp::CCGCSotpClient::pointer> m_pModuleSotpClientList;
//#endif
	mycp::asio::IoService::pointer m_pIoService;
	int m_nDefaultThreadStackSize;
public:
	//XmlParseLocks m_moduleLocks;
	XmlParseParams m_moduleParams;
	std::string m_sTempFile;

public:
	CModuleImpl(void);
#ifdef USES_MODULE_SERVICE_MANAGER
	CModuleImpl(const ModuleItem::pointer& module, CModuleHandler* pModuleHandler);
#else
	CModuleImpl(const ModuleItem::pointer& module, cgcServiceManager* pServiceManager, CModuleHandler* pModuleHandler);
#endif
	virtual ~CModuleImpl(void);

	void setModulePath(const tstring & modulePath) {m_sModulePath = modulePath;}
	//void setCommId(unsigned long commId) {m_commId = commId;}
	//unsigned long getCommId(void) const {return m_commId;}
	void setDefaultThreadStackSize(int size);

	void SetInited(bool nInited) {m_nInited = nInited;}
	void SetCdbcDatasource(const tstring& sDatasource) {m_pDataSourceList.insert(sDatasource,true,false);}
	void StopModule(void);
	//void OnCheckTimeout(void);

	unsigned long getCallRef(void) const {return m_callRef;}
	unsigned long addCallRef(void) {return ++m_callRef;}
	unsigned long releaseCallRef(void) {return m_callRef==0 ? 0 : --m_callRef;}
	boost::mutex m_mutexCallWait;	// for LS_WAIT

	ModuleItem::pointer getModuleItem(void) const {return m_module;}

	void setModuleState(int v) {m_moduleState = v;}
	int getModuleState(void) const {return m_moduleState;}

	static bool FileIsExist(const char* lpszFile);
	bool loadSyncData(bool bCreateForce);
	void procSyncThread(void);
	void setAttributes(const cgcAttributes::pointer& pAttributes) {m_attributes = pAttributes;}

private:
#ifdef USES_MODULE_SERVICE_MANAGER
	// cgcServiceManager handler
	virtual cgcServiceInterface::pointer getService(const tstring & serviceName, const cgcValueInfo::pointer& parameter = cgcNullValueInfo);
	virtual void resetService(const cgcServiceInterface::pointer & service);
	virtual cgcCDBCInfo::pointer getCDBDInfo(const tstring& datasource) const;
	virtual cgcCDBCService::pointer getCDBDService(const tstring& datasource);
	virtual void retCDBDService(cgcCDBCServicePointer& cdbcservice);
	virtual HTTP_STATUSCODE executeInclude(const tstring & url, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response);
	virtual HTTP_STATUSCODE executeService(const tstring & serviceName, const tstring& function, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response, tstring & outExecuteResult);
#endif

	// cgcApplication handler
	virtual cgcParameterMap::pointer getInitParameters(void) const {return m_moduleParams.getParameters();}

	virtual unsigned long getApplicationId(void) const {return (unsigned long)m_module.get();}
	virtual tstring getApplicationName(void) const {return m_module.get() == 0 ? _T("") : m_module->getName();}
	virtual tstring getApplicationFile(void) const {return m_module.get() == 0 ? _T("") : m_module->getModule();}
	virtual tstring getApplicationType(void) const {return m_module.get() == 0 ? _T("") : m_module->getTypeString();}
	virtual MODULETYPE getModuleType(void) const {return m_module.get() == 0 ? MODULE_UNKNOWN : m_module->getType();}
	virtual tstring getAppConfPath(void) const;
	virtual bool isInited(void) const {return m_nInited;}
	virtual long getMajorVersion(void) const;
	virtual long getMinorVersion(void) const;

	// timer
#if (USES_TIMER_HANDLER_POINTER==1)
	virtual unsigned int SetTimer(unsigned int nIDEvent, unsigned int nElapse, cgcOnTimerHandler* handler, bool bOneShot, const void * pvParam, int nThreadStackSize);
#else
	virtual unsigned int SetTimer(unsigned int nIDEvent, unsigned int nElapse, cgcOnTimerHandler::pointer handler, bool bOneShot, const void * pvParam, int nThreadStackSize);
#endif
	virtual void KillTimer(unsigned int nIDEvent);
	virtual void KillAllTimer(void);

	virtual cgcAttributes::pointer getAttributes(bool create);
	virtual cgcAttributes::pointer getAttributes(void) const {return m_attributes;}
	virtual cgcAttributes::pointer createAttributes(void) const {return cgcAttributes::pointer(new AttributesImpl());}

	virtual tstring getExecuteResult(void) const {return m_sExecuteResult;}
	virtual void setExecuteResult(const tstring & executeResult) {m_sExecuteResult = executeResult;}

	// io service
	virtual mycp::asio::IoService::pointer getIoService(bool bCreateAndStart = true);
	virtual void resetIoService(bool bStopIoService = true);

	virtual int sendSyncData(const tstring& sSyncName, int nDataType, const char* sDataString, size_t nDataSize, bool bBackup);
	virtual int sendSyncFile(const tstring& sSyncName, int nDataType, const tstring& sFileName, const tstring& sFilePath, bool bBackup);
	virtual DoSotpClientHandler::pointer getSotpClientHandler(const tstring& sAddress, const tstring& sAppName, SOTP_CLIENT_SOCKET_TYPE nSocketType=SOTP_CLIENT_SOCKET_TYPE_UDP, bool bUserSsl=false, bool bKeepAliveSession=false, int bindPort=0, int nThreadStackSize=200);
public:
	virtual cgcLogService::pointer logService(void) const {return m_logService;}
	virtual void logService(cgcLogService::pointer logService) {m_logService = logService;}
	virtual void log(LogLevel level, const char* format,...);
	virtual void log(LogLevel level, const wchar_t* format,...);
	virtual void log2(LogLevel level, const char* sData);
	virtual void log2(LogLevel level, const wchar_t* sData);

};

// CModuleMgr
class CModuleMgr
{
public:
	CModuleMgr(void);
	virtual ~CModuleMgr(void);

	//void OnCheckTimeout(void);

	//CModuleImplMap m_mapModuleImpl;
	CLockMap<void*, cgcApplication::pointer> m_mapModuleImpl;

public:
	void StopModule(void);
	void FreeHandle(void);
};

} // namespace mycp

#endif // _MODULEMGR_HEAD_INCLUDED__
