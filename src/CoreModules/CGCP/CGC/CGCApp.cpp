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

//#define USES_HDCID 1
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#else //WIN32
#include "dlfcn.h"
unsigned long GetLastError(void)
{
	return 0;
}
#endif

#include "CGCApp.h"
#include "md5.h"

#ifdef WIN32
#include <tchar.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include "../../../CGCClass/tchar.h"
#endif // WIN32

//#ifdef USES_HDCID
//#include "HDComputerID/def.h"
//#ifdef WIN32
//#ifdef _DEBUG
//#pragma comment(lib, L"HDComputerIDd.lib")
//#else
//#pragma comment(lib, L"HDComputerID.lib")
//#endif
//#endif
//
//tstring getAccountKey(int count, const tstring & sAccount, long & hdcid)
//{
//	hdcid = getHardDriveComputerID();
//
//	//
//	// to md5 & return
//	//
//	tstring sKeySrc = cgcString::Format("%d%s%d", count, sAccount.c_str(), hdcid);
//	tstring s = cgcString::toMd5(sKeySrc.c_str(), sKeySrc.length());
//	return  cgcString::toUpper(s);
//}
//#endif

namespace mycp {

#ifdef _UNICODE
typedef boost::filesystem::wpath boosttpath;
#else
typedef boost::filesystem::path boosttpath;
#endif // _UNICODE

CGCApp::CGCApp(const tstring & sPath)
: m_bStopedApp(true)
, m_bInitedApp(false)
, m_bExitLog(false)
, m_sModulePath(sPath)
, m_bLicensed(true)
, m_sLicenseAccount(_T(""))
, m_licenseModuleCount(1)
, m_fpGetSotpClientHandler(NULL)
, m_fpGetLogService(NULL), m_fpResetLogService(NULL), m_fpParserSotpService(NULL), m_fpParserHttpService(NULL)
, /*m_fpHttpStruct(NULL), */m_fpHttpServer(NULL), m_sHttpServerName("")
//, m_pRtpSession(true)
, m_tLastNewParserSotpTime(0), m_tLastNewParserHttpTime(0)

{
	m_bService = false;
//#ifdef WIN32
//	TCHAR chModulePath[MAX_PATH];
//	memset(&chModulePath, 0, MAX_PATH);
//	::GetModuleFileName(NULL, chModulePath, MAX_PATH);
//	TCHAR* temp = (TCHAR*)0;
//	temp = _tcsrchr(chModulePath, (unsigned int)'\\');
//	chModulePath[temp - chModulePath] = '\0';
//
//	m_sModulePath = chModulePath;
//#else
//	namespace fs = boost::filesystem;
//	fs::path currentPath( fs::initial_path());
//	m_sModulePath = currentPath.string();
//#endif
	//m_pRtpSession.SetSotpRtpCallback((CSotpRtpCallback*)this);
	m_logModuleImpl.setModulePath(m_sModulePath);
	m_tLastSystemParams = 0;
}

CGCApp::~CGCApp(void)
{
	AppExit();
}

void CGCApp::PrintHelp(void)
{
	std::cout << "\n********************* App Help *********************\n";
	std::cout << "\thelp\t\tPrint this help.\n";
	std::cout << "\tstart\t\tStart MYCP Service.\n";
	std::cout << "\tstop\t\tStop MYCP Service.\n";
	std::cout << "\trestart\t\tRestart MYCP Service.\n";
	std::cout << "\texit\t\tExit MYCP Server.\n";

}

/* ?
void do_event_loop(void)
{
	ACE_Reactor::instance()->run_reactor_event_loop();
}
*/

void CGCApp::do_dataresend(void)
{
	unsigned int theIndex = 0;
	bool bHasResendData = false;
	while (isInited())
	{
#ifdef WIN32
		Sleep(50);
#else
		usleep(50000);
#endif
		if (!bHasResendData && ((theIndex++)%20)!=19)
			continue;
		try
		{
			bHasResendData = ProcDataResend();
		}catch(std::exception const &)
		{
		}catch(...){
		}
	}
}
void CGCApp::do_sessiontimeout(void)
{
	//if (pCGCApp == NULL) return;

	// 1 秒检查一次，SESSION是否超时没有访问。自动清除无用SESSION
	unsigned int theIndex = 0;
	unsigned int theSecondIndex = 0;
	char lpszBuffer[32];
	const tstring sProtectDataFile = GetProtectDataFile();
	const int nIsService = GetIsService()?1:0;
	while (isInited())
	{
#ifdef WIN32
		Sleep(100);
#else
		usleep(100000);
#endif
		if (((theIndex++)%10)!=9)
			continue;
		try
		{
			ProcCheckParserPool();
			ProcNotKeepAliveRmote();

			if (((++theSecondIndex)%20)==19)	// 20秒处理一次
			{
				ProcLastAccessedTime();
			}
			//if (((theSecondIndex)%30)==29)		// 30秒处理一次 for test
			if (((theSecondIndex)%60)==59)		// 60秒处理一次
			{
				ProcCheckAutoUpdate();
			}

			if (!sProtectDataFile.empty() && (theSecondIndex%2)==0)	// 2秒处理一次
			{
				FILE * pfile = fopen(sProtectDataFile.c_str(),"w");
				if (pfile!=NULL)
				{
					sprintf(lpszBuffer,"%lld,%d",(bigint)time(0),nIsService);
					fwrite(lpszBuffer,1,strlen(lpszBuffer),pfile);
					fclose(pfile);
				}
			}

//			if (((++theSecondIndex)%20)!=19) continue;	// 20秒处理一次
//			// 如果没有超时SESSION，再继续等待
//			while(ProcLastAccessedTime())
//			{
//				// 如果有超时SESSION，继续处理；
//#ifdef WIN32
//				Sleep(10);
//#else
//				usleep(10000);
//#endif
//				ProcNotKeepAliveRmote();
//			}

//			if (theSecondIndex>30)
//			{
//				if ((theSecondIndex%20)==19)			// 20秒处理一次
//				{
//#ifdef USES_CMODULEMGR
//					m_pModuleMgr.OnCheckTimeout();
//#else
//					{
//						BoostReadLock rdlock(m_mapOpenModules.mutex());
//						CLockMap<void*, cgcApplication::pointer>::iterator pIter;
//						for (pIter=m_mapOpenModules.begin(); pIter!=m_mapOpenModules.end(); pIter++)
//						{
//							CModuleImpl * pModuleImpl = (CModuleImpl*)pIter->second.get();
//							pModuleImpl->OnCheckTimeout();
//						}
//					}
//#endif
//				}
//			}

			if ((theSecondIndex%3600)==3500)						// 一小时处理一次
			{
				CheckScriptExecute(SCRIPT_TYPE_HOURLY);
				this->m_mgrSession1.ProcInvalidRemoteIdList();
				this->m_mgrSession2.ProcInvalidRemoteIdList();
			}
			if (theSecondIndex%(3600*24)==(3600*23))				// 3600*24=60*24分钟=一天处理一次
			{
				CheckScriptExecute(SCRIPT_TYPE_DAILY);
			}
			if (theSecondIndex%(3600*24*7)==(3600*24*6))			// 每7天处理一次
				CheckScriptExecute(SCRIPT_TYPE_WEEKLY);
			if (theSecondIndex%(3600*24*30)==(3600*24*29))			// 每30天处理一次
				CheckScriptExecute(SCRIPT_TYPE_MONTHLY);
			if (theSecondIndex%(3600*24*365)==(3600*24*364))		// 每365天处理一次
				CheckScriptExecute(SCRIPT_TYPE_YEARLY);
		}catch(std::exception const &)
		{
		}catch(...){
		}
	}
	if (!sProtectDataFile.empty())
	{
		for (int i=0;i<30; i++)
		{
			boost::system::error_code ec;
			if (boost::filesystem::remove(boost::filesystem::path(sProtectDataFile.c_str()),ec))
			{
				break;
			}
#ifdef WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
		}
	}
}
inline bool FileIsExist(const char* lpszFile)
{
	FILE * f = fopen(lpszFile,"r");
	if (f==NULL)
		return false;
	fclose(f);
	return true;
}
inline void RenameAutoUpdateFile(const tstring& sAutoUpdateXmlFile,tstring* pOutDateDir=NULL)
{
	// ** 重命名自动更新文件
	const time_t tNow = time(0);
	const struct tm * pNotTime = localtime(&tNow);
	char lpszDateDir[60];
	sprintf(lpszDateDir,"%04d%02d%02d-%02d%02d",pNotTime->tm_year+1900,pNotTime->tm_mon+1,pNotTime->tm_mday,pNotTime->tm_hour,pNotTime->tm_min);
	if (pOutDateDir!=NULL)
		*pOutDateDir = lpszDateDir;
	char lpszBkFile[260];
	sprintf(lpszBkFile,"%s.%s",sAutoUpdateXmlFile.c_str(),lpszDateDir);
	namespace fs = boost::filesystem;
	fs::path pathfrom(sAutoUpdateXmlFile.string());
	fs::path pathto(lpszBkFile);
	boost::system::error_code ec;
	fs::rename(pathfrom,pathto,ec);
}
void CGCApp::FreeLibModule(const ModuleItem::pointer& moduleItem, bool bEraseApplication, Module_Free_Type nFreeType)
{
	void * hModule = moduleItem->getModuleHandle();
	if (hModule!=NULL)
	{
		cgcApplication::pointer pNewApplication;
#ifdef USES_CMODULEMGR
		if (m_pModuleMgr.m_mapModuleImpl.find(hModule, pNewApplication,bEraseApplication))
#else
		if (m_mapOpenModules.find(hModule, pNewApplication,bEraseApplication))
#endif
		{
			FreeLibModule(pNewApplication,nFreeType);
		}

		//printf("**** Free Module(0x%x) %02d -> %s\n", (int)hModule, i+1, moduleItem->getName().c_str());
		//m_logModuleImpl.log(LOG_INFO, _T("Free Module %02d : name=%s, module=0x%x\n"), i+1, moduleItem->getName().c_str(),hModule);
		if (hModule!=NULL)
		{
			try
			{
#ifdef WIN32
				FreeLibrary((HMODULE)hModule);
#else
				dlclose (hModule);
#endif
			}catch(std::exception const & e)
			{
				printf("**** name=%s, exception. 0x%x(%s)\n", moduleItem->getName().c_str(), GetLastError(), e.what());
			}catch(...){
				printf("**** name=%s, exception. 0x%x\n", moduleItem->getName().c_str(), GetLastError());
			}
			//printf("**** Free Module(0x%x) %02d ok\n", (int)hModule, i+1);
			moduleItem->setModuleHandle(NULL);
		}
		moduleItem->m_pApiProcAddressList.clear();

		if (pNewApplication.get()!=NULL)
		{
			CModuleImpl * pModuleImpl = (CModuleImpl*)pNewApplication.get();
			if (!pModuleImpl->m_sTempFile.empty())
			{
				remove(pModuleImpl->m_sTempFile.c_str());
				m_logModuleImpl.log(LOG_INFO, _T("Close temp file: %s\n"), pModuleImpl->m_sTempFile.c_str());
				pModuleImpl->m_sTempFile.clear();
			}
		}
	}
}
void CGCApp::RemoveAllAutoUpdateFile(const XmlParseAutoUpdate& pAutoUpdate)
{
	for (size_t i=0; i<pAutoUpdate.m_modules.size(); i++)
	{
		const ModuleItem::pointer moduleItem = pAutoUpdate.m_modules[i];
		FreeLibModule(moduleItem,true,MODULE_FREE_TYPE_NORMAL);

		const tstring& sAutoUpdateModuleFile = moduleItem->getParam();
		if (!sAutoUpdateModuleFile.empty())
			remove(sAutoUpdateModuleFile.c_str());
	}
}
#define USES_TEMP_MODULE_FILE2

void CGCApp::do_autoupdate(void)
{
	tstring sAutoUpdateXmlFile(m_sModulePath);
	sAutoUpdateXmlFile.append(_T("/auto_update/auto_update.xml"));
	if (!FileIsExist(sAutoUpdateXmlFile.c_str()))
	{
		DetachAutoUpdateThread();
		return;
	}
	// ** 需要更新组件
	XmlParseAutoUpdate pAutoUpdate;
	pAutoUpdate.load(sAutoUpdateXmlFile);
	m_logModuleImpl.log(LOG_INFO, _T("AutoUpdate: AutoUpdateType=%d, UpdateModules=%d\n"), pAutoUpdate.getAutpUpdateType(),(int)pAutoUpdate.m_modules.size());
	// 
	// * 检查更新组件，是否完整存在，和检查复制组件到指定路径是否成功
	int nErrorType = 0;
	std::string sErrorModuleFile;
	for (size_t i=0; i<pAutoUpdate.m_modules.size(); i++)
	{
		ModuleItem::pointer newModuleItem = pAutoUpdate.m_modules[i];
		const tstring& sModuleFile = newModuleItem->getModule();
		tstring sNewModulePath(m_sModulePath);
		sNewModulePath.append("/auto_update/");
		sNewModulePath.append(sModuleFile);
		m_logModuleImpl.log(LOG_INFO, _T("AutoUpdate: %d, ModuleFile=%s\n"), i, sModuleFile.c_str());
		//printf("**** sNewModulePath=%s\n",sNewModulePath.c_str());
		if (!FileIsExist(sNewModulePath.c_str()))
		{
			nErrorType = 1;
			sErrorModuleFile = sModuleFile.string();
			break;
		}else
		{
			tstring sCurrentModulePath1(m_sModulePath);						// 1=XXX
			sCurrentModulePath1.append("/modules/");
			sCurrentModulePath1.append(sModuleFile);
			tstring sCurrentModulePath2(sCurrentModulePath1);			// 2=XXX.autoupdate
			sCurrentModulePath2.append(".autoupdate");
			//printf("**** sCurrentModulePath1=%s\n",sCurrentModulePath1.c_str());
			//printf("**** sCurrentModulePath2=%s\n",sCurrentModulePath2.c_str());

			// 复制更新组件到指定目录
			boost::system::error_code ec;
			namespace fs = boost::filesystem;
			fs::path pathfrom(sNewModulePath.string());
			fs::path pathto1(sCurrentModulePath1.string());
			if (!FileIsExist(sCurrentModulePath1.c_str()) || fs::remove(pathto1,ec))	// * 先删除
			{
				fs::rename(pathfrom,pathto1,ec);	// * 再重命名
				if (!ec)
				{
					newModuleItem->setParam(sCurrentModulePath1);	// ?保存临时文件路径
					continue;	// 1 ok
				}
			}
			ec.clear();
			fs::path pathto2(sCurrentModulePath2.string());
			if (!FileIsExist(sCurrentModulePath2.c_str()) || fs::remove(pathto2,ec))	// * 先删除
			{
				fs::rename(pathfrom,pathto2,ec);	// * 再重命名
				if (!ec)
				{
					newModuleItem->setParam(sCurrentModulePath2);	// ?保存临时文件路径
					continue;	// 2 ok
				}
			}
			// error
			nErrorType = 2;
			sErrorModuleFile = sModuleFile.string();
			break;
		}
	}
	if (nErrorType!=0)
	{
		// ** 重命名自动更新文件
		tstring sDateDir;
		RenameAutoUpdateFile(sAutoUpdateXmlFile,&sDateDir);
		// ** 记录更新错误日志
		char lpszAutoUpdateLogFile[260];
		sprintf(lpszAutoUpdateLogFile,"%s.%s.error",sAutoUpdateXmlFile.c_str(),sDateDir.c_str());
		FILE * f = fopen(lpszAutoUpdateLogFile,"w");
		if (f!=NULL)
		{
			fwrite(sErrorModuleFile.c_str(),1,sErrorModuleFile.size(),f);
			std::string sLog;
			if (nErrorType==1)
				sLog = " not exist";
			else if (nErrorType==2)
				sLog = " copy file error";
			fwrite(sLog.c_str(),1,sLog.size(),f);
			fclose(f);
		}
		// ** 删除临时更新文件
		RemoveAllAutoUpdateFile(pAutoUpdate);
		// ** 清空线程资源
		DetachAutoUpdateThread();
		return;
	}
	// ** 更新组件
	for (size_t i=0; i<pAutoUpdate.m_modules.size(); i++)
	{
		const ModuleItem::pointer newModuleItem = pAutoUpdate.m_modules[i];
		const tstring& sAutoUpdateModuleFile = newModuleItem->getParam();
		if (sAutoUpdateModuleFile.empty()) continue;
		// a 通过 file 获取所有已经启动的 cgcApplication::pointer(ModuleItem)
		std::vector<cgcApplication::pointer> pModuleItemList;
#ifdef USES_CMODULEMGR
		CLockMap<void*, cgcApplication::pointer>::iterator iterApp = m_pModuleMgr.m_mapModuleImpl.begin();
		for (; iterApp!=m_pModuleMgr.m_mapModuleImpl.end(); iterApp++)
#else
		for (iterApp=m_mapOpenModules.begin(); iterApp!=m_mapOpenModules.end(); iterApp++)
#endif
		{
			cgcApplication::pointer application = iterApp->second;
			if (application->getModuleType()==MODULE_COMM) continue;	// ??底层通讯组件，暂时不支持自动更新
			CModuleImpl * pModuleImpl = (CModuleImpl*)application.get();
			if (pModuleImpl->getModuleItem()->getModule()==newModuleItem->getModule())	// file
			{
				pModuleItemList.push_back(application);
			}
		}
		bool bUpdateOk = false;
		// b 复制一份，然后调用 openlibrary and CGC_Module_Init2；(cgcAttributes::pointer CModuleImpl::getAttributes(bool create) 数据，重新指向即可；
		for (size_t j=0; j<pModuleItemList.size(); j++)
		{
			const cgcApplication::pointer pUpdateFromApplication = pModuleItemList[j];
			cgcAttributes::pointer pOldAttributes = pUpdateFromApplication->getAttributes(false);
			CModuleImpl * pModuleImpl = (CModuleImpl*)pUpdateFromApplication.get();
			ModuleItem::pointer pCurrentModuleItem = pModuleImpl->getModuleItem();

			const void* pCurrentModuleHandle = pCurrentModuleItem->getModuleHandle();
			const std::string sCurrentTempFile = pModuleImpl->m_sTempFile;	// 最后要删除这个临时文件
			m_logModuleImpl.log(LOG_INFO, _T("%d, ModuleName=%s, AutoUpdate...\n"), j, pCurrentModuleItem->getName().c_str());

			std::string sTempFile;
			tstring sModuleName(sAutoUpdateModuleFile);	// 1=XXX or 2=XXX.autoupdate
			//m_logModuleImpl.log(LOG_INFO, _T("**** sModuleName=%s\n"), sModuleName.c_str());
#ifdef USES_TEMP_MODULE_FILE2
			if (!pCurrentModuleItem->getName().empty() && pCurrentModuleItem->getModule().find(pCurrentModuleItem->getName())==std::string::npos)
			{
				// * 新建临时文件
				namespace fs = boost::filesystem;
				boosttpath pathFileFrom(sModuleName.c_str());
				char lpszBuffer[260];
				sprintf(lpszBuffer,"%s.%s",sModuleName.c_str(),pCurrentModuleItem->getName().c_str());
				sTempFile = lpszBuffer;
				//m_logModuleImpl.log(LOG_INFO, _T("**** sTempFile=%s\n"), sTempFile.c_str());
				boosttpath pathFileTo1(sTempFile);
				boost::system::error_code ec;
				boost::filesystem::copy_file(pathFileFrom,pathFileTo1,fs::copy_option::overwrite_if_exists,ec);
				if (ec)
				{
					sprintf(lpszBuffer,"%s.%s.autoupdate",sModuleName.c_str(),pCurrentModuleItem->getName().c_str());
					sTempFile = lpszBuffer;
					boosttpath pathFileTo2(sTempFile);
					ec.clear();
					boost::filesystem::copy_file(pathFileFrom,pathFileTo2,fs::copy_option::overwrite_if_exists,ec);
					if (ec)
					{
						m_logModuleImpl.log(LOG_ERROR, _T("copy_file: %s!\n"), sTempFile.c_str());
						continue;
					}
				}
				m_logModuleImpl.log(LOG_INFO, _T("Open temp file1: %s\n"), sTempFile.c_str());
				sModuleName = sTempFile;
			}
#endif
			//m_logModuleImpl.log(LOG_INFO, _T("OpenLibrarys:%s\n"), sModuleName.c_str());
#ifdef WIN32
			void * hModule = LoadLibrary(sModuleName.c_str());
#else
			void * hModule = dlopen(sModuleName.c_str(), RTLD_LAZY);
#endif
			if (hModule == NULL)
			{
				m_logModuleImpl.log(LOG_ERROR, _T("Cannot open library: %s \'%d\'!\n"), sModuleName.c_str(), GetLastError());
				// ???

				if (!sTempFile.empty())
					remove(sTempFile.c_str());
				continue;
				//return false;
			}
			pModuleImpl->m_sTempFile = sTempFile;
#ifdef USES_MODULE_SERVICE_MANAGER
			CModuleImpl::pointer pNewApplication = boost::static_pointer_cast<CModuleImpl,cgcApplication>(pUpdateFromApplication);
#else
			CModuleImpl::pointer pNewApplication(pUpdateFromApplication);
#endif
			SetModuleHandler(hModule, pNewApplication);
			bool bInitOk = false;
			if (pModuleImpl->getModuleState() == 0)
			{
				// 旧组件未启动成功
				bInitOk = InitLibModule(pUpdateFromApplication, pCurrentModuleItem);
			}else
			{
				// 旧组件启动成功，新组件先初始化一次后，再...
				if (InitLibModule(hModule, pUpdateFromApplication, MODULE_INIT_TYPE_AUTO_UPDATE))
				{
					pCurrentModuleItem->setModuleHandle(hModule);																												// ***A（A到***B有一个时间差，这个时间差，如果访问该组件，可能会失败?）
					bInitOk = InitLibModule(pUpdateFromApplication, pCurrentModuleItem, MODULE_INIT_TYPE_AUTO_UPDATE);	// ***B
					if (!bInitOk)
					{
						pCurrentModuleItem->setModuleHandle((void*)pCurrentModuleHandle);	// *
						FreeLibModule(hModule, MODULE_FREE_TYPE_AUTO_UPDATE);
						// 执行多一次，修正 CGC_GetService 相关地址
						InitLibModule(pUpdateFromApplication, pCurrentModuleItem, MODULE_INIT_TYPE_AUTO_UPDATE);
					}else
					{
//#ifdef WIN32
						if (pOldAttributes.get()!=NULL)
						{
							// 更换
							//printf("**** new AttributesImpl()...\n");
#ifdef WIN32
							FPCGC_Module_AutoUpdateData farProc_AutoUpdateData = (FPCGC_Module_AutoUpdateData)GetProcAddress((HMODULE)hModule, "CGC_Module_AutoUpdateData");
#else
							FPCGC_Module_AutoUpdateData farProc_AutoUpdateData = (FPCGC_Module_AutoUpdateData)dlsym(hModule, "CGC_Module_AutoUpdateData");
#endif
							if (farProc_AutoUpdateData!=NULL)
							{
								try
								{
									farProc_AutoUpdateData(pOldAttributes);
								}catch (const std::exception&)
								{
								}catch (...)
								{
								}
							}
							// **
							pOldAttributes->clearAllAtrributes();
							pOldAttributes->cleanAllPropertys();
							pOldAttributes.reset();
						}
//#endif
					}
				}
			}
			if (!bInitOk)
			{
				// 失败，清空数据
				m_logModuleImpl.log(LOG_INFO, _T("%d, ModuleName=%s, AutoUpdate error.\n"), j, pCurrentModuleItem->getName().c_str());
#ifdef USES_CMODULEMGR
				m_pModuleMgr.m_mapModuleImpl.remove(hModule);
#else
				m_mapOpenModules.remove(hModule);
#endif
				try
				{
#ifdef WIN32
					FreeLibrary((HMODULE)hModule);
#else
					dlclose (hModule);
#endif
				}catch(std::exception const &)
				{
				}catch(...){
				}
				if (!sTempFile.empty())
					remove(sTempFile.c_str());
				continue;
			}
			// 成功，切换到新的组件
			// c 切换到新的组件，停止旧的 ModuleItem::pointer，调用 CGC_Module_Free2 （~~不要清空 m_attributes 数据）（**可以清空）
			bUpdateOk = true;
			pCurrentModuleItem->setModuleHandle(hModule);
			m_logModuleImpl.log(LOG_INFO, _T("%d, ModuleName=%s, AutoUpdate ok.\n"), j, pCurrentModuleItem->getName().c_str());
			pOldAttributes.reset();
			//m_logModuleImpl.log(LOG_INFO, "**** hModule=%x, pCurrentModuleHandle=%x\n",(int)hModule,(int)pCurrentModuleHandle);
			// 删除旧的 modulehandle 对应数据
			//pCurrentModuleHandle = NULL;	// for test
			if (pCurrentModuleHandle!=NULL)
			{
				m_parsePortApps.resetByOldModuleHandle((void*)pCurrentModuleHandle);
				
				std::vector<cgcServiceInterface::pointer> pResetServiceList;
				{
					BoostReadLock rdServiceModileLock(m_mapServiceModule.mutex());
					CLockMap<cgcServiceInterface::pointer, void*>::iterator pServiceModuleItem = m_mapServiceModule.begin();
					for (; pServiceModuleItem!=m_mapServiceModule.end(); pServiceModuleItem++)
					{
						if (pServiceModuleItem->second==pCurrentModuleHandle)
						{
							pResetServiceList.push_back(pServiceModuleItem->first);
						}
					}
				}
				// ** 找到使用该组件的其他组件，通知重置组件 SendModuleResetService
				for (size_t n=0; n<pResetServiceList.size(); n++)
				{
					cgcServiceInterface::pointer pService = pResetServiceList[n];
					std::vector<const CModuleImpl*> pFromModuleList;
					if (m_pFromServiceList.find(pService.get(),pFromModuleList,true))
					{
						for (size_t m=0; m<pFromModuleList.size(); m++)
						{
							const CModuleImpl* pFromModuleImpl = pFromModuleList[m];
							SendModuleResetService(pFromModuleImpl,pService->getOrgServiceName());
						}
					}
					ResetService((void*)pCurrentModuleHandle, pService);
				}
				pResetServiceList.clear();

				m_mapServiceModule.removet((void*)pCurrentModuleHandle);

				// ** 等待2秒，用于某些旧调用退出。
#ifdef WIN32
				Sleep(2000);
#else
				usleep(2000000);
#endif
				FreeLibModule((void*)pCurrentModuleHandle, MODULE_FREE_TYPE_AUTO_UPDATE);
#ifdef USES_CMODULEMGR
				m_pModuleMgr.m_mapModuleImpl.remove((void*)pCurrentModuleHandle);
#else
				m_mapOpenModules.remove((void*)pCurrentModuleHandle);
#endif
				try
				{
#ifdef WIN32
					FreeLibrary((HMODULE)pCurrentModuleHandle);
#else
					dlclose ((void*)pCurrentModuleHandle);
#endif
				}catch(std::exception const &)
				{
				}catch(...){
				}
			}
			// ** 自动更新完成，删除旧组件临时文件
			if (!sCurrentTempFile.empty())
				remove(sCurrentTempFile.c_str());

			//pModuleImpl->getModuleItem()->setModule());
			//ModuleItem::pointer pNewModuleItem = ModuleItem::create();
			//const MODULETYPE modultType = pModuleImpl->getModuleItem()->getType();
			//if (modultType == MODULE_COMM)
			//{
			//	pNewModuleItem->setCommPort(pModuleImpl->getModuleItem()->getCommPort());
			//}
			//pNewModuleItem->setName(pModuleImpl->getModuleItem()->getName());
			//pNewModuleItem->setModule(pModuleImpl->getModuleItem()->getModule());	// ?? 这是旧的 module-file，还是使用新的 module-file
			//pNewModuleItem->setParam(pModuleImpl->getModuleItem()->getParam());
			//pNewModuleItem->setType(modultType);
			//pNewModuleItem->setProtocol(pModuleImpl->getModuleItem()->getProtocol());
			//pNewModuleItem->setAllowAll(pModuleImpl->getModuleItem()->getAllowAll());
			//pNewModuleItem->setAuthAccount(pModuleImpl->getModuleItem()->getAuthAccount());
			//pNewModuleItem->setLockState(pModuleImpl->getModuleItem()->getLockState());
			//if (!OpenModuleLibrary(pNewModuleItem, pUpdateFromApplication))
			//{
			//	// ??? 记录错误信息；
			//	// ** 删除临时更新文件
			//	RemoveAllAutoUpdateFile(pAutoUpdate);
			//	// ** 清空线程资源
			//	DetachAutoUpdateThread();
			//	return;
			//}
			// 
//			cgcApplication::pointer pNewApplication;
//#ifdef USES_CMODULEMGR
//			if (!m_pModuleMgr.m_mapModuleImpl.find(pNewModuleItem->getModuleHandle(), pNewApplication))
//#else
//			if (!m_mapOpenModules.find(pNewModuleItem->getModuleHandle(), pNewApplication))
//#endif
//			{
//				// ??? 记录错误信息；
//
//				// ** 删除临时更新文件
//				RemoveAllAutoUpdateFile(pAutoUpdate);
//				// ** 清空线程资源
//				DetachAutoUpdateThread();
//				return;
//			}

			//try
			//{
			//	if (InitLibModule(pUpdateFromApplication, pCurrentModuleItem))
			//	{
			//		CModuleImpl * pModuleImpl = (CModuleImpl*)pUpdateFromApplication.get();
			//		pModuleImpl->SetInited(true);
			//		m_logModuleImpl.log(LOG_INFO, _T("MODULE \'%s\' load succeeded\n"), pUpdateFromApplication->getApplicationName().c_str());
			//	}else
			//	{
			//		m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' load failed\n"), pUpdateFromApplication->getApplicationName().c_str());
			//	}
			//}catch(const std::exception & e)
			//{
			//	m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' load exception.\n"), pUpdateFromApplication->getApplicationName().c_str());
			//	m_logModuleImpl.log(LOG_ERROR, _T("%s\n"), e.what());
			//}catch(...)
			//{
			//	m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' load exception\n"), pUpdateFromApplication->getApplicationName().c_str());
			//}

			//if (pUpdateFromApplication->isInited())
			//{
			//	// 成功
			//	// c 切换到新的组件，停止旧的 ModuleItem::pointer，调用 CGC_Module_Free2 （不要清空 m_attributes 数据）
			//	// 
			//	ModuleItem::pointer pOldModuleItem = m_parseModules.swapModuleItemByName(pNewModuleItem);
			//	if (pOldModuleItem.get()!=NULL)
			//	{
			//		FreeLibModule(pOldModuleItem,true,MODULE_FREE_TYPE_AUTO_UPDATE);
			//	}
			//}else
			//{
			//	// ???失败

			//}
		}
		// ** 完成更新
		if (bUpdateOk)
		{
			// 删除组件更新文件
			const tstring& sModuleFile = newModuleItem->getModule();
			tstring sNewModulePath(m_sModulePath);
			sNewModulePath.append("/auto_update/");
			sNewModulePath.append(sModuleFile);
			remove(sNewModulePath.c_str());
			// 删除旧组件文件
			tstring sRemoveFile;
			const std::string::size_type nFind = sAutoUpdateModuleFile.find(".autoupdate");
			if (nFind==std::string::npos)
			{
				sRemoveFile = sAutoUpdateModuleFile + ".autoupdate";
			}else
			{
				sRemoveFile = sAutoUpdateModuleFile.substr(0,nFind);
			}
			remove(sRemoveFile.c_str());
			//printf("**** sAutoUpdateModuleFile=%s, sRemoveFile=%s\n",sAutoUpdateModuleFile.c_str(),sRemoveFile.c_str());

		}
	}
	// ** 重命名自动更新文件
	RenameAutoUpdateFile(sAutoUpdateXmlFile);
	DetachAutoUpdateThread();
}

void CGCApp::ProcNotKeepAliveRmote(void)
{
	// 检查没有keep-alive连接；
	const time_t tNow = time(0);
	CNotKeepAliveRemote::pointer pNotKeepAliveRemote;
	while (m_pNotKeepAliveRemoteList.front(pNotKeepAliveRemote))
	{
		if (!pNotKeepAliveRemote->IsExpireTime(tNow, 2))
		{
			m_pNotKeepAliveRemoteList.pushfront(pNotKeepAliveRemote); 
			break;
		}
	}
}
void CGCApp::ProcLastAccessedTime(void)
{
	std::vector<std::string> sCloseSidList;
	this->m_mgrSession1.ProcLastAccessedTime(sCloseSidList);
	if (!sCloseSidList.empty())
	{
		for (size_t i=0; i<sCloseSidList.size(); i++)
		{
			m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' closed\n"), sCloseSidList[i].c_str());
		}
		sCloseSidList.clear();
	}
	this->m_mgrSession2.ProcLastAccessedTime(sCloseSidList);
	if (!sCloseSidList.empty())
	{
		for (size_t i=0; i<sCloseSidList.size(); i++)
		{
			m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' closed\n"), sCloseSidList[i].c_str());
		}
		sCloseSidList.clear();
	}

	//static unsigned int nIndex = 0;
	//if ((nIndex++)%2==1)	// 2*20=40秒处理一次；
	//std::vector<int> pRemoveList;
	{
		BoostWriteLock wtlock(m_pRtpSession.mutex());
		//BoostReadLock rdlock(m_pRtpSession.mutex());
		CLockMap<int,CSotpRtpSession::pointer>::iterator pIter = m_pRtpSession.begin();
		for (; pIter!=m_pRtpSession.end(); )
		{
			const CSotpRtpSession::pointer& pRtpSession = pIter->second;
			pRtpSession->CheckRegisterSourceLive(59, this, 0);
			if (pRtpSession->IsRoomEmpty())
			{
				m_pRtpSession.erase(pIter++);
				//pRemoveList.push_back(pIter->first);
			}else
			{
				pIter++;
			}
		}
	}
	//if (!pRemoveList.empty())
	//{
	//	for (size_t i=0; i<pRemoveList.size(); i++)
	//	{
	//		m_pRtpSession.remove(pRemoveList[i]);
	//	}
	//}
	//return false;
	//// 检查mysessioninfo
	//{
	//	boost::mutex::scoped_lock lock(m_pMySessionInfoList.mutex());
	//	CLockMap<tstring,CMySessionInfo::pointer>::iterator pIter = m_pMySessionInfoList.begin();
	//	for (; pIter!=m_pMySessionInfoList.end(); pIter++)
	//	{
	//		CMySessionInfo::pointer pMySessionInfo = pIter->second;
	//		if (pMySessionInfo->GetRequestTime()+10*60 < time(0))
	//		{
	//			// mysessionif过期
	//			m_pMySessionInfoList.erase(pIter);
	//			break;	// 2秒检查一个，时间应该足够，不够以后再优化
	//		}
	//	}
	//}
}

void CGCApp::AppInit(bool bNTService)
{
	// init parameter
	tstring xmlFile(m_sModulePath);
	xmlFile.append(_T("/conf/params.xml"));
	namespace fs = boost::filesystem;
	boosttpath pathXmlFile(xmlFile.c_str());
	if (fs::exists(pathXmlFile))
	{
		m_logModuleImpl.m_moduleParams.load(xmlFile);
	}
	if (m_logModuleImpl.m_moduleParams.getLogFile().empty())
		m_logModuleImpl.m_moduleParams.setLogFile(_T("MYCP.log"));
	//m_logModuleImpl.m_moduleParams.setLts(false);
	
	if (bNTService)
	{
		// ????
		/*
		tstring sAceLogConf(m_sModulePath);
		sAceLogConf.append(L"/cwss_acelog.conf");
		if (ACE_LOG_MSG->open(sAceLogConf.c_str(), ACE_Log_Msg::LOGGER, ACE_DEFAULT_LOGGER_KEY) < 0)
		{
			//		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("Service Config open")), 1);
			m_logModuleImpl.log(DL_WARNING, L"Service Config open error, LastError = 0x%x!\n", GetLastError());
		}*/
	}
//#ifndef WIN32
//	pthread_attr_t    attr;
//	size_t            size;
//	/* do some calculation for optimal stack size or ... */
//	size = PTHREAD_STACK_MIN;
//	printf("**** PTHREAD_STACK_MIN=%d\n",size);
//	pthread_attr_init(&attr);
//	pthread_attr_setstacksize(&attr, size);
//#endif
	// 必须放在前面
	m_bInitedApp = true;
	m_bExitLog = false;
#ifdef USES_THREAD_STACK_SIZE
	boost::thread_attributes attrs;
	attrs.set_stack_size(CGC_THREAD_STACK_MAX);	// CGC_THREAD_STACK_MIN
	m_pProcSessionTimeout = boost::shared_ptr<boost::thread>(new boost::thread(attrs,boost::bind(&CGCApp::do_sessiontimeout, this)));
	m_pProcDataResend = boost::shared_ptr<boost::thread>(new boost::thread(attrs,boost::bind(&CGCApp::do_dataresend, this)));
#else
	m_pProcSessionTimeout = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CGCApp::do_sessiontimeout, this)));
	m_pProcDataResend = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CGCApp::do_dataresend, this)));
#endif
}

void CGCApp::AppStart(int nWaitSeconds)
{
	if (!m_bStopedApp)
	{
		return;
	}
	m_pSotpParserPool.clear();
	m_pHttpParserPool.clear();
	//m_bStopedApp = false;

	LoadDefaultConf();
	m_logModuleImpl.log(LOG_INFO, _T("Starting %s Service......\n"), m_parseDefault.getCgcpName().c_str());

	// optional conf
	LoadClustersConf();
//	LoadAuthsConf();
	LoadSystemParams();

	nWaitSeconds += m_parseDefault.getWaitSleep();
	//printf("****** nWaitSeconds=%d\n",nWaitSeconds);
	for (int i=0; i<nWaitSeconds; i++)
	{
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
		//if (m_bStopedApp)
		//	break;
	}

	LoadModulesConf();
	m_bStopedApp = false;
}

void CGCApp::AppStop(void)
{
	if (m_bStopedApp)
	{
		return;
	}
	m_logModuleImpl.log(LOG_INFO, _T("Stop %s Service......\n"), m_parseDefault.getCgcpName().c_str());
	m_bStopedApp = true;
	m_parsePortApps.clear();
	m_pSotpParserPool.clear();
	m_pHttpParserPool.clear();
	m_pRtpSession.clear();
	m_mgrSession1.invalidates(true);
	m_mgrSession2.invalidates(true);
	FreeLibModules(MODULE_COMM);
	FreeLibModules(MODULE_APP);		// ***先停止APP应用
	FreeLibModules(MODULE_SERVER);
	FreeLibModules(MODULE_PARSER);
	m_mgrSession1.FreeHandle();
	m_mgrSession2.FreeHandle();
	//m_mgrHttpSession.FreeHandle();

	if (m_logModuleImpl.logService().get() != NULL)
	{
		if (m_fpResetLogService != NULL)
			m_fpResetLogService(m_logModuleImpl.logService());
		m_logModuleImpl.logService(cgcNullLogService);
	}
	m_logModuleImpl.StopModule();
	FreeLibModules(MODULE_LOG);
	m_logModuleImpl.log(LOG_INFO, _T("**** %s Service stop succeeded ****\n"), m_parseDefault.getCgcpName().c_str());

	m_cdbcServices.clear();
	FreeLibrarys();
	m_pNotKeepAliveRemoteList.clear();
	m_parseModules.FreeHandle();
//	m_parseClusters.FreeHandle();
//	m_parseAuths.m_mapAuths.clear();
	m_systemParams.clearParameters();
	m_tLastSystemParams = 0;
	m_cdbcs.FreeHandle();
	m_parseWeb.FreeHandle();
	m_fpGetSotpClientHandler = NULL;
	m_fpGetLogService = NULL;
	m_fpResetLogService = NULL;
	m_fpParserSotpService = NULL;
	m_fpParserHttpService = NULL;
	//m_fpHttpStruct = NULL;
	m_fpHttpServer = NULL;
	m_sHttpServerName = "";
	m_mapRemoteOpenSes.clear();
	//m_mapServiceModule.clear();
	m_mapMultiParts.clear();	// ??
	m_attributes.reset();
	//clearAllAtrributes();
	//cleanAllPropertys();
}

void CGCApp::AppExit(void)
{
	if (!m_bInitedApp) return;
	m_bInitedApp = false;

	m_logModuleImpl.log(LOG_INFO, _T("Exiting %s Server......\n"), m_parseDefault.getCgcpName().c_str());

	if (m_pProcSessionTimeout.get()!=NULL)
	{
		if (m_pProcSessionTimeout->joinable())
			m_pProcSessionTimeout->join();
		m_pProcSessionTimeout.reset();
	}
	if (m_pProcDataResend.get()!=NULL)
	{
		if (m_pProcDataResend->joinable())
			m_pProcDataResend->join();
		m_pProcDataResend.reset();
	}
	if (m_pProcAutoUpdate.get()!=NULL)
	{
		if (m_pProcAutoUpdate->joinable())
			m_pProcAutoUpdate->join();
		m_pProcAutoUpdate.reset();
	}

	AppStop();
}

time_t theFreeModuleTime = 0;
#ifndef WIN32
bool theKilledApp = false;
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
void kill_op(int signum,siginfo_t *info,void *myact)
{
	printf("receive signal %d\n", signum);
	//sleep(5);
	if (theKilledApp && theFreeModuleTime>0 && (theFreeModuleTime+20)<time(0))
	{
		exit(0);
	}
	theKilledApp = true;
}
//void pipeSignalProc(int sig)
//{
//	switch(sig)
//	{
//	case SIGPIPE:
//		//p_log(LOG_DEBUG, "SIGPIPE");
//		break;
//	default:
//		//p_log(LOG_DEBUG, "Unknow signal!");
//		break;
//	}
//}
#endif

void CGCApp::CheckScriptExecute(int nScriptType)
{
	char lpszScriptDir[260];
#ifdef WIN32
	sprintf(lpszScriptDir,"%s\\script",m_sModulePath.c_str());
#else
	sprintf(lpszScriptDir,"%s/script",m_sModulePath.c_str());
#endif

	tstring sUpdateScriptFile(lpszScriptDir);
	switch (nScriptType)
	{
	case SCRIPT_TYPE_ONCE:
		{
#ifdef WIN32
			sUpdateScriptFile.append("\\mycp_execute_script_once.bat");
#else
			sUpdateScriptFile.append("/mycp_execute_script_once.sh");
#endif
		}break;
	case SCRIPT_TYPE_HOURLY:
		{
#ifdef WIN32
			sUpdateScriptFile.append("\\mycp_execute_script_hourly.bat");
#else
			sUpdateScriptFile.append("/mycp_execute_script_hourly.sh");
#endif
		}break;
	case SCRIPT_TYPE_DAILY:
		{
#ifdef WIN32
			sUpdateScriptFile.append("\\mycp_execute_script_daily.bat");
#else
			sUpdateScriptFile.append("/mycp_execute_script_daily.sh");
#endif
		}break;
	case SCRIPT_TYPE_WEEKLY:
		{
#ifdef WIN32
			sUpdateScriptFile.append("\\mycp_execute_script_weekly.bat");
#else
			sUpdateScriptFile.append("/mycp_execute_script_weekly.sh");
#endif
		}break;
	case SCRIPT_TYPE_MONTHLY:
		{
#ifdef WIN32
			sUpdateScriptFile.append("\\mycp_execute_script_monthly.bat");
#else
			sUpdateScriptFile.append("/mycp_execute_script_monthly.sh");
#endif
		}break;
	case SCRIPT_TYPE_YEARLY:
		{
#ifdef WIN32
			sUpdateScriptFile.append("\\mycp_execute_script_yearly.bat");
#else
			sUpdateScriptFile.append("/mycp_execute_script_yearly.sh");
#endif
		}break;
	default:
		return;
		break;
	}
	if (FileIsExist(sUpdateScriptFile.c_str()))
	{
		// *
#ifdef WIN32
		char lpszCurrentDir[260];
		GetCurrentDirectory(260,lpszCurrentDir);
		::SetCurrentDirectory(lpszScriptDir);
		::WinExec(sUpdateScriptFile.c_str(),SW_HIDE);
		::SetCurrentDirectory(lpszCurrentDir);
#else
		char lpszCommand[1024];
		sprintf(lpszCommand,"chmod +x \"%s\"",sUpdateScriptFile.c_str());
		system(lpszCommand);

		sprintf(lpszCommand,"cd \"%s\" ; \"%s\" ; cd ..",lpszScriptDir,sUpdateScriptFile.c_str());
		system(lpszCommand);
#endif
		if (nScriptType==SCRIPT_TYPE_ONCE)
		{
			const time_t tNow = time(0);
			const struct tm * pNotTime = localtime(&tNow);
			char lpszDateDir[60];
			sprintf(lpszDateDir,"%04d%02d%02d-%02d%02d",pNotTime->tm_year+1900,pNotTime->tm_mon+1,pNotTime->tm_mday,pNotTime->tm_hour,pNotTime->tm_min);
			char lpszBkFile[260];
			sprintf(lpszBkFile,"%s.%s.txt",sUpdateScriptFile.c_str(),lpszDateDir);

			namespace fs = boost::filesystem;
			fs::path pathfrom(sUpdateScriptFile.string());
			fs::path pathto(lpszBkFile);
			boost::system::error_code ec;
			fs::copy_file(pathfrom,pathto,ec);
			remove(sUpdateScriptFile.c_str());
		}
	}
}

int CGCApp::MyMain(int nWaitSeconds,bool bService, const std::string& sProtectDataFile)
{
	CheckScriptExecute(SCRIPT_TYPE_ONCE);
	m_bService = bService;
	m_sProtectDataFile = sProtectDataFile;
	AppInit(false);
	AppStart(nWaitSeconds);

	//for (int i=0;i<20;i++)
	//{
	//	m_pRsa.SetPrivatePwd(GetSaltString());
	//	m_pRsa.rsa_generatekey_mem(1024);
	//	if (m_pRsa.GetPublicKey().empty() || m_pRsa.GetPrivateKey().empty())
	//	{
	//		m_pRsa.SetPrivatePwd("");
	//		continue;
	//	}
	//	if (!m_pRsa.rsa_open_private_mem())
	//	{
	//		m_pRsa.SetPrivatePwd("");
	//		continue;
	//	}
	//	const std::string sFrom("mycp");
	//	unsigned char* pTo = NULL;
	//	m_pRsa.rsa_private_encrypt((const unsigned char*)sFrom.c_str(),sFrom.size(),&pTo);
	//	if (pTo!=NULL)
	//	{
	//		delete[] pTo;
	//		m_pRsa.rsa_close_private();
	//		break;
	//	}
	//	m_pRsa.rsa_close_private();
	//	m_pRsa.SetPrivatePwd("");
	//}
	//if (m_pRsa.GetPrivatePwd().empty())
	//	std::cout << "[ERROR]Generate SSL Key error.\n";
	if (m_bService)
	{
		std::cout << "\n********************* App Help *********************\n";
		std::cout << "App Service Running...\n";
	}else
	{
		PrintHelp();
	}
#ifndef WIN32
		struct sigaction act;	
		sigemptyset(&act.sa_mask);
		act.sa_flags=SA_SIGINFO;
		act.sa_sigaction=kill_op;
		if(sigaction(SIGTERM,&act,NULL) < 0)
		{
			printf("install SIGTERM sigal error/n");
		}
		// 11) SIGSEGV 试图访问未分配给自己的内存, 或试图往没有写权限的内存地址写数据. 
		//if(sigaction(SIGSEGV ,&act,NULL) < 0)
		//{
		//	printf("install SIGSEGV sigal error/n");
		//}
		//signal(SIGPIPE, pipeSignalProc);
		// 忽略broken pipe信號
		// 11) SIGPIPE
		signal(SIGPIPE, SIG_IGN);
#endif
	//while (!m_bExitedApp)
#ifdef WIN32
	while (m_bInitedApp)
#else
	while (m_bInitedApp && !theKilledApp)
#endif
	{
		if (m_bService)
		{
#ifdef WIN32
			Sleep(10);
#else
			usleep(10000);
#endif
			continue;
		}
		std::wcout << _T("CMD:");
		std::string command;
		try
		{
			std::getline(std::cin, command);
			if (command.length() == 0)
			{
#ifdef WIN32
				//std::wcin.clear(std::ios_base::goodbit, true);
				std::wcin.clear(std::ios_base::goodbit);
#else
				//std::wcin.clear(std::ios_base::goodbit, true);
#endif
				continue;
			}
		}catch (const std::exception & e)
		{
			std::cout << e.what() << std::endl;
			continue;
		}catch (...)
		{
			std::cout << "some exception" << std::endl;
			continue;
		}

		//if (m_bExitedApp ||
		if (!m_bInitedApp ||
			command.compare(_T("exit")) == 0 ||
			command.compare(_T("x")) == 0)
		{
			break;
		}else if (command.compare(_T("help")) == 0 ||
			command.compare(_T("?")) == 0 ||
			command.compare(_T("h")) == 0)
		{
			PrintHelp();
		}else if (command.compare(_T("start")) == 0)
		{
			AppStart(0);
		}else if (command.compare(_T("stop")) == 0)
		{
			AppStop();
		}else if (command.compare(_T("restart")) == 0)
		{
			AppStop();
			AppStart(0);
		//}else if (command.compare(_T("update")) == 0)
		//{
		//	// *** for test
		//	do_autoupdate();
		}else
		{
			std::cout << "ERROR CMD: ";
			std::cout << command.c_str() << "\n'h' or '?' for help!" << std::endl;
		}
	}

	AppExit();
	return 0;
}

void CGCApp::LoadDefaultConf(void)
{
	tstring xmlFile(m_sModulePath);
	xmlFile.append("/conf/default.xml");
	m_parseDefault.load(xmlFile.c_str());

	m_logModuleImpl.log(LOG_DEBUG, _T("Server Name = '%s'\n"), m_parseDefault.getCgcpName().c_str());
	//m_logModuleImpl.log(LOG_DEBUG, _T("Server Address = '%s'\n"), m_parseDefault.getCgcpAddr().c_str());

	xmlFile = m_sModulePath.c_str();
	xmlFile.append("/conf/port-apps.xml");
	m_parsePortApps.load(xmlFile);

#ifdef USES_HDCID
	tstring iniLicenseFile(m_sModulePath);
	iniLicenseFile.append(L"/conf/license.ini");

	tstring sCode;

	//
	// read account & code from license.ini
	//
	char lpBuf[60];
	memset(lpBuf, 0, sizeof(lpBuf));
	::GetPrivateProfileStringA("license", L"account", L"", lpBuf, sizeof(lpBuf), iniLicenseFile.c_str());
	m_sLicenseAccount = lpBuf;
	::GetPrivateProfileStringA("license", L"code", L"", lpBuf, sizeof(lpBuf), iniLicenseFile.c_str());
	sCode = lpBuf;

	m_licenseModuleCount = ::GetPrivateProfileIntA("license", L"count", 1, iniLicenseFile.c_str());

	long hdcid = 0;
	tstring sAccountKey = getAccountKey(m_licenseModuleCount, m_sLicenseAccount, hdcid);
	if (sCode.empty() || sAccountKey.compare(sCode) != 0)
	{
		m_bLicensed = false;
		m_licenseModuleCount = 1;

		//
		// write to license.ini
		//
		tstring sHdcid = cgcString::Format("%d", hdcid);
		::WritePrivateProfileStringA("license", L"hdcid", sHdcid.c_str(), iniLicenseFile.c_str());
	}else
	{
		m_bLicensed = true;
	}

#endif
}

void CGCApp::LoadClustersConf(void)
{
	tstring xmlFile(m_sModulePath);
	xmlFile.append(_T("/conf/clusters.xml"));

	// ? SOTP/2.0
	/*
	int retCode = theXmlParse.ParseXmlFile(xmlFile.c_str(), &this->m_parseClusters);
	if (retCode == 0)
	{
		m_logModuleImpl.log(LOG_DEBUG, _T("Cluster Servers '%d'\n"), m_parseClusters.getClusters().size());
	}

	return retCode;
	*/
}

/*
void CGCApp::LoadAuthsConf(void)
{
	tstring xmlFile(m_sModulePath);
	xmlFile.append(_T("/conf/auths.xml"));

	namespace fs = boost::filesystem;
	boosttpath pathXmlFile(xmlFile);
	if (boost::filesystem::exists(pathXmlFile))
	{
		m_parseAuths.load(xmlFile);
	}
}
*/

void CGCApp::LoadModulesConf(void)
{
	//int initModuleCount = 0;
	tstring xmlFile(m_sModulePath);
	xmlFile.append(_T("/conf/modules.xml"));

	m_parseModules.load(xmlFile);

	m_logModuleImpl.log(LOG_DEBUG, _T("MODULES = %d\n"), m_parseModules.m_modules.size());

	OpenLibrarys();
	InitLibModules(MODULE_LOG);
	InitLibModules(MODULE_SERVER|MODULE_PARSER|MODULE_APP);
	InitLibModules(MODULE_COMM);

	if (m_fpParserSotpService == NULL)
	{
		m_logModuleImpl.log(LOG_WARNING, _T("ParserService == NULL.\n"));
	}else
	{
		m_logModuleImpl.log(LOG_INFO, _T("**** %s Server start succeeded ****\n"), m_parseDefault.getCgcpName().c_str());
	}
}

void CGCApp::LoadSystemParams(void)
{
	// init parameter
	tstring xmlFile(m_sModulePath);
	//xmlFile.append(_T("/conf/params.xml"));
	//m_systemParams.load(xmlFile.c_str());
	//m_logModuleImpl.log(LOG_DEBUG, _T("SystemParams = %d\n"), m_systemParams.getParameters()->size());

	xmlFile = m_sModulePath + "/conf/cdbcs.xml";
	namespace fs = boost::filesystem;
	boosttpath pathXmlFile(xmlFile.c_str());
	if (boost::filesystem::exists(pathXmlFile))
	{
		m_cdbcs.load(xmlFile);
		m_logModuleImpl.log(LOG_DEBUG, _T("CDBCInfos = %d\n"), m_cdbcs.m_cdbcInfos.size());
		m_logModuleImpl.log(LOG_DEBUG, _T("DataSources = %d\n"), m_cdbcs.m_dsInfos.size());
	}

	xmlFile = m_sModulePath + "/conf/web.xml";
	pathXmlFile = boosttpath(xmlFile.c_str());
	if (boost::filesystem::exists(pathXmlFile))
	{
		m_parseWeb.load(xmlFile);
		m_logModuleImpl.log(LOG_DEBUG, _T("Servlets = %d\n"), m_parseWeb.m_servlets.size());
	}
}

cgcParameterMap::pointer CGCApp::getInitParameters(void) const
{
	const tstring xmlFile = m_sModulePath + "/conf/params.xml";
	namespace fs = boost::filesystem;
	fs::path src_path(xmlFile.string());
	const time_t lastTime = fs::last_write_time(src_path);
	cgcParameterMap::pointer result =  m_systemParams.getParameters();
	if (lastTime!=m_tLastSystemParams)
	//if (result->empty() || lastTime!=m_tLastSystemParams)
	{
		CGCApp * pOwner = const_cast<CGCApp*>(this);
		pOwner->m_tLastSystemParams = lastTime;
		pOwner->m_systemParams.load(xmlFile.c_str());
		result =  pOwner->m_systemParams.getParameters();
		pOwner->m_logModuleImpl.log(LOG_DEBUG, _T("SystemParams = %d\n"), m_systemParams.getParameters()->size());
		//const_cast<time_t&>(m_tLastSystemParams) = lastTime;
		//const_cast<XmlParseParams&>(m_systemParams).load(xmlFile.c_str());
		//result =  m_systemParams.getParameters();
		//const_cast<CModuleImpl&>(m_logModuleImpl).log(LOG_DEBUG, _T("SystemParams = %d\n"), m_systemParams.getParameters()->size());
	}
	return result;
}

//#define USES_FUNC_HANDLE2
bool CGCApp::setPortAppModuleHandle(const CPortApp::pointer& portApp)
{
	BOOST_ASSERT (portApp.get() != NULL);
	if (portApp->getModuleHandle() == NULL)
	{
		ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
		if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
		{
			return false;
		}
		void * hModule = moduleItem->getModuleHandle();
		portApp->setModuleHandle(hModule);

#ifdef WIN32
		FPCGC_Rtp_Register_Source fp1 = (FPCGC_Rtp_Register_Source)GetProcAddress((HMODULE)hModule, "CGC_Rtp_Register_Source");
		FPCGC_Rtp_UnRegister_Source fp2 = (FPCGC_Rtp_UnRegister_Source)GetProcAddress((HMODULE)hModule, "CGC_Rtp_UnRegister_Source");
		FPCGC_Rtp_Register_Sink fp3 = (FPCGC_Rtp_Register_Sink)GetProcAddress((HMODULE)hModule, "CGC_Rtp_Register_Sink");
#else
		FPCGC_Rtp_Register_Source fp1 = (FPCGC_Rtp_Register_Source)dlsym(hModule, "CGC_Rtp_Register_Source");
		FPCGC_Rtp_UnRegister_Source fp2 = (FPCGC_Rtp_UnRegister_Source)dlsym(hModule, "CGC_Rtp_UnRegister_Source");
		FPCGC_Rtp_Register_Sink fp3 = (FPCGC_Rtp_Register_Sink)dlsym(hModule, "CGC_Rtp_Register_Sink");
#endif
		portApp->setFuncHandle1((void*)fp1);
		portApp->setFuncHandle2((void*)fp2);
		portApp->setFuncHandle3((void*)fp3);
	}
	return true;
}
bool CGCApp::onRegisterSource(bigint nRoomId, bigint nSourceId, bigint nParam, void* pUserData)
{
	const int nServerPort = pUserData==NULL?0:(int)pUserData;
	//const int nServerPort = pUserData==NULL?0:(*(int*)pUserData);
	CPortApp::pointer portApp = m_parsePortApps.getPortApp(nServerPort);
	if (portApp.get() == NULL)
	{
		return true;	// *
	}
#ifdef USES_FUNC_HANDLE2
	if (portApp->getModuleHandle() == NULL)
	{
		ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
		if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
		{
			return false;
		}
		portApp->setModuleHandle(moduleItem->getModuleHandle());
	}
#ifdef WIN32
	FPCGC_Rtp_Register_Source fp = (FPCGC_Rtp_Register_Source)GetProcAddress((HMODULE)portApp->getModuleHandle(), "CGC_Rtp_Register_Source");
#else
	FPCGC_Rtp_Register_Source fp = (FPCGC_Rtp_Register_Source)dlsym(portApp->getModuleHandle(), "CGC_Rtp_Register_Source");
#endif
#else	// USES_FUNC_HANDLE2
	setPortAppModuleHandle(portApp);
//	if (portApp->getModuleHandle() == NULL)
//	{
//		ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
//		if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
//		{
//			return false;
//		}
//		void * hModule = moduleItem->getModuleHandle();
//		portApp->setModuleHandle(hModule);
//
//#ifdef WIN32
//		FPCGC_Rtp_Register_Source fp1 = (FPCGC_Rtp_Register_Source)GetProcAddress((HMODULE)hModule, "CGC_Rtp_Register_Source");
//		FPCGC_Rtp_UnRegister_Source fp2 = (FPCGC_Rtp_UnRegister_Source)GetProcAddress((HMODULE)hModule, "CGC_Rtp_UnRegister_Source");
//		FPCGC_Rtp_Register_Sink fp3 = (FPCGC_Rtp_Register_Sink)GetProcAddress((HMODULE)hModule, "CGC_Rtp_Register_Sink");
//#else
//		FPCGC_Rtp_Register_Source fp1 = (FPCGC_Rtp_Register_Source)dlsym(hModule, "CGC_Rtp_Register_Source");
//		FPCGC_Rtp_UnRegister_Source fp2 = (FPCGC_Rtp_UnRegister_Source)dlsym(hModule, "CGC_Rtp_UnRegister_Source");
//		FPCGC_Rtp_Register_Sink fp3 = (FPCGC_Rtp_Register_Sink)dlsym(hModule, "CGC_Rtp_Register_Sink");
//#endif
//		portApp->setFuncHandle1((void*)fp1);
//		portApp->setFuncHandle2((void*)fp2);
//		portApp->setFuncHandle3((void*)fp3);
//	}
	FPCGC_Rtp_Register_Source fp = (FPCGC_Rtp_Register_Source)portApp->getFuncHandle1();
#endif
	if (fp==NULL)
		return false;
	try
	{
		return fp(nRoomId, nSourceId, nParam);
	}catch (const std::exception&)
	{
	}catch (...)
	{
	}
	return false;
}
void CGCApp::onUnRegisterSource(bigint nRoomId, bigint nSourceId, bigint nParam, void* pUserData)
{
	const int nServerPort = pUserData==NULL?0:(int)pUserData;
	//const int nServerPort = pUserData==NULL?0:(*(int*)pUserData);
	CPortApp::pointer portApp = m_parsePortApps.getPortApp(nServerPort);
	if (portApp.get() == NULL)
	{
		return;	// *
	}
#ifdef USES_FUNC_HANDLE2
	if (portApp->getModuleHandle() == NULL)
	{
		ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
		if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
		{
			return;
		}
		portApp->setModuleHandle(moduleItem->getModuleHandle());
	}
#ifdef WIN32
		FPCGC_Rtp_UnRegister_Source fp = (FPCGC_Rtp_UnRegister_Source)GetProcAddress((HMODULE)portApp->getModuleHandle(), "CGC_Rtp_UnRegister_Source");
#else
		FPCGC_Rtp_UnRegister_Source fp = (FPCGC_Rtp_UnRegister_Source)dlsym(portApp->getModuleHandle(), "CGC_Rtp_UnRegister_Source");
#endif
#else
	setPortAppModuleHandle(portApp);
	FPCGC_Rtp_UnRegister_Source fp = (FPCGC_Rtp_UnRegister_Source)portApp->getFuncHandle2();
#endif
	if (fp==NULL) return;
	try
	{
		fp(nRoomId, nSourceId, nParam);
	}catch (const std::exception&)
	{
	}catch (...)
	{
	}
}
bool CGCApp::onRegisterSink(bigint nRoomId, bigint nSourceId, bigint nDestId, void* pUserData)
{
	const int nServerPort = pUserData==NULL?0:(int)pUserData;
	//const int nServerPort = pUserData==NULL?0:(*(int*)pUserData);
	CPortApp::pointer portApp = m_parsePortApps.getPortApp(nServerPort);
	if (portApp.get() == NULL)
	{
		return true;	// *
	}
#ifdef USES_FUNC_HANDLE2
	if (portApp->getModuleHandle() == NULL)
	{
		ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
		if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
		{
			return false;
		}
		portApp->setModuleHandle(moduleItem->getModuleHandle());
	}
#ifdef WIN32
		FPCGC_Rtp_Register_Sink fp = (FPCGC_Rtp_Register_Sink)GetProcAddress((HMODULE)portApp->getModuleHandle(), "CGC_Rtp_Register_Sink");
#else
		FPCGC_Rtp_Register_Sink fp = (FPCGC_Rtp_Register_Sink)dlsym(portApp->getModuleHandle(), "CGC_Rtp_Register_Sink");
#endif
#else
	setPortAppModuleHandle(portApp);
	FPCGC_Rtp_Register_Sink fp = (FPCGC_Rtp_Register_Sink)portApp->getFuncHandle3();
#endif
	if (fp==NULL) return false;
	try
	{
		return fp(nRoomId, nSourceId, nDestId);
	}catch (const std::exception&)
	{
	}catch (...)
	{
	}
	return false;
}

tstring CGCApp::onGetSslPassword(const tstring& sSessionId) const
{
	cgcSession::pointer sessionImpl = sSessionId.size()==DEFAULT_HTTP_SESSIONID_LENGTH?m_mgrSession2.GetSessionImpl(sSessionId):m_mgrSession1.GetSessionImpl(sSessionId);
	if (sessionImpl.get() == NULL)
	{
		return "";
	}
	CSessionImpl * pSessionImpl = (CSessionImpl*)sessionImpl.get();
	return pSessionImpl->GetSslPassword();
}

void CGCApp::onReturnParserSotp(cgcParserSotp::pointer cgcParser)
{
	SetSotpParserPool(cgcParser);
}

void CGCApp::onReturnParserHttp(cgcParserHttp::pointer cgcParser)
{
	SetHttpParserPool(cgcParser);
}

int CGCApp::onRemoteAccept(const cgcRemote::pointer& pcgcRemote)
{
	// 记录下新的 SockRemote
	//try
	{
		//m_mgrSession.GetSessionImplByRemote(pcgcRemote->getRemoteId());
		return 0;
	//}catch (const std::exception & e)
	//{
	//	m_logModuleImpl.log(LOG_ERROR, _T("exception, OnRemoteAccept(), \'%s\', lasterror=0x%x\n"), e.what(), GetLastError());
	//}catch (...)
	//{
	//	m_logModuleImpl.log(LOG_ERROR, _T("exception, OnRemoteAccept(), lasterror=0x%x\n"), GetLastError());
	}

	return -111;
}

typedef int (FAR *FPCGC_DefaultFunc)(const cgcRequest::pointer & pRequest, cgcResponse::pointer & pResponse);
//typedef int (FAR *FPCGC_DefaultFunc)(cgcRemote::pointer & pcgcRemote, const unsigned char * recvData, size_t recvLen);

int CGCApp::onRecvData(const cgcRemote::pointer& pcgcRemote, const unsigned char * recvData, size_t recvLen)
{
	if (m_bStopedApp || recvData == 0 || recvLen == 0 || pcgcRemote->isInvalidate())
	{
		printf("CGCApp::onRecvData isInvalidate size=%d\n",recvLen);
		return -1;
	}else if (m_parseDefault.getLogReceiveData())
	{
		m_logModuleImpl.log(LOG_TRACE, _T("****onRecvData:Size=%d\n****%s\n****\n"), recvLen, recvData);
	}

	//printf("CGCApp::onRecvData:%d size=%d\n",pcgcRemote->getRemoteId(),recvLen);
	try
	{
		if (pcgcRemote->getProtocol() == PROTOCOL_SOTP || (pcgcRemote->getProtocol() & PROTOCOL_HSOTP))
		{
			if (m_fpParserSotpService == NULL) return -2;
			ProcCgcData(recvData, recvLen, pcgcRemote);
		}else if (pcgcRemote->getProtocol() & PROTOCOL_HTTP)
		{
			if (m_fpParserHttpService == NULL) return -2;
			ProcHttpData(recvData, recvLen, pcgcRemote);
		}else
		{
			// ??
			int serverPort = pcgcRemote->getServerPort();
			CPortApp::pointer portApp = m_parsePortApps.getPortApp(serverPort);
			if (portApp.get() == NULL)
			{
				pcgcRemote->invalidate();
				return 0;
			}
#ifdef USES_FUNC_HANDLE2
			if (portApp->getModuleHandle() == NULL)
			{
				ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
				if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
				{
					pcgcRemote->invalidate();
					return 0;
				}
				portApp->setModuleHandle(moduleItem->getModuleHandle());
				m_mgrSession1.SetSessionImpl(moduleItem,pcgcRemote,cgcNullParserBaseService);
				//CSessionImpl * pSessionImpl = (CSessionImpl*)sessionImpl.get();
			}
			const tstring sFunc = portApp->getFunc().empty() ? "doPortAppFunction" : portApp->getFunc();
#ifdef WIN32
			FPCGC_DefaultFunc fp = (FPCGC_DefaultFunc)GetProcAddress((HMODULE)portApp->getModuleHandle(), sFunc.c_str());
#else
			FPCGC_DefaultFunc fp = (FPCGC_DefaultFunc)dlsym(portApp->getModuleHandle(), sFunc.c_str());
#endif
			if (fp == NULL)
			{
				pcgcRemote->invalidate();
				return 0;
			}
#else
			if (portApp->getModuleHandle() == NULL)
			{
				ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
				if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
				{
					pcgcRemote->invalidate();
					return 0;
				}
				void * hModule = moduleItem->getModuleHandle();
				portApp->setModuleHandle(hModule);

				tstring sFunc = portApp->getFunc().empty() ? "doPortAppFunction" : portApp->getFunc();
#ifdef WIN32
				FPCGC_DefaultFunc fp = (FPCGC_DefaultFunc)GetProcAddress((HMODULE)hModule, sFunc.c_str());
#else
				FPCGC_DefaultFunc fp = (FPCGC_DefaultFunc)dlsym(hModule, sFunc.c_str());
#endif
				portApp->setFuncHandle1((void*)fp);

				m_mgrSession1.SetSessionImpl(moduleItem,pcgcRemote,cgcNullParserBaseService);
				//CSessionImpl * pSessionImpl = (CSessionImpl*)sessionImpl.get();
			}

			if (portApp->getFuncHandle1() == NULL)
			{
				pcgcRemote->invalidate();
				return 0;
			}
			FPCGC_DefaultFunc fp = (FPCGC_DefaultFunc)portApp->getFuncHandle1();
#endif
			cgcSession::pointer sessionImpl = m_mgrSession1.GetSessionImplByRemote(pcgcRemote->getRemoteId());
			if (sessionImpl.get() == NULL)
			{
				ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(portApp->getApp());
				if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
				{
					pcgcRemote->invalidate();
					return 0;
				}
				sessionImpl = m_mgrSession1.SetSessionImpl(moduleItem,pcgcRemote,cgcNullParserBaseService);
				CSessionImpl * pSessionImpl = (CSessionImpl*)sessionImpl.get();
			}else
			{
				CSessionImpl * pSessionImpl = (CSessionImpl*)sessionImpl.get();
				pSessionImpl->setDataResponseImpl("",pcgcRemote);
			}
			cgcRequest::pointer requestImpl(new CRequestImpl(pcgcRemote, cgcNullParserBaseService));
			cgcResponse::pointer responseImpl(new CResponseImpl(pcgcRemote, cgcNullParserBaseService));

			((CRequestImpl*)requestImpl.get())->setSession(sessionImpl);
			((CRequestImpl*)requestImpl.get())->setContent((const char*)recvData, recvLen);
			((CResponseImpl*)responseImpl.get())->setSession(sessionImpl);

			fp(requestImpl, responseImpl);
			//if (!sResponse.empty())
			//{
			//	pcgcRemote->sendData((const unsigned char*)sResponse.c_str(), sResponse.size());
			//}

			//((FPCGC_DefaultFunc)portApp->getFuncHandle())(pcgcRemote, recvData, recvLen);
		}
	}catch (const std::exception & e)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("exception, \'%s\', lasterror=0x%x\n"), e.what(), GetLastError());
	}catch (...)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("exception, lasterror=0x%x\n"), GetLastError());
	}
	return 0;
}

int CGCApp::onRemoteClose(unsigned long remoteId, int nErrorCode)
{
	try
	{
		m_mgrSession2.onRemoteClose(remoteId,nErrorCode);
		//m_mgrSession.setInterval(remoteId, 10);	// 10分钟，这里不处理，由应用在CGC_Session_Open()自行设置
		//m_mapMultiParts.remove(remoteId);
		return 0;
	}catch (const std::exception & e)
	{
		try{
			m_logModuleImpl.log(LOG_ERROR, _T("exception, OnRemoteClose(), '%s', lasterror=0x%x\n"), e.what(), GetLastError());
		}catch(...){}
	}catch (...)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("exception, OnRemoteClose(), lasterror=0x%x\n"), GetLastError());
	}

	return -111;
}


inline std::string::size_type findPathOrExt(const tstring& pBuffer, std::string::size_type off=0)
{
	if (off>=pBuffer.size()) return std::string::npos;
	int index = 0;
	const char * pCompare = pBuffer.c_str()+off;
	while (pCompare[index] != '\0')
	{
		const char c = pCompare[index];
		if (c=='/' || c=='.')
			return off+index;
		index++;
	}
	return std::string::npos;
}


#ifdef USES_MODULE_SERVICE_MANAGER
void CGCApp::exitModule(const CModuleImpl* pFromModuleImpl)
{
	m_pFromServiceList.removet(pFromModuleImpl,false);	// **true,退出会有异常
}
cgcCDBCInfo::pointer CGCApp::getCDBDInfo(const CModuleImpl* pFromModuleImpl, const tstring& datasource) const
#else
cgcCDBCInfo::pointer CGCApp::getCDBDInfo(const tstring& datasource) const
#endif
{
	cgcDataSourceInfo::pointer pDSInfo = m_cdbcs.getDataSourceInfo(datasource);
	return pDSInfo.get()==NULL?cgcNullCDBCInfo:pDSInfo->getCDBCInfo();
}
#ifdef USES_MODULE_SERVICE_MANAGER
cgcCDBCService::pointer CGCApp::getCDBDService(const CModuleImpl* pFromModuleImpl, const tstring& datasource)
#else
cgcCDBCService::pointer CGCApp::getCDBDService(const tstring& datasource)
#endif
{
	cgcCDBCService::pointer result;
	if (m_cdbcServices.find(datasource, result))
	{
#ifdef USES_MODULE_SERVICE_MANAGER
		m_pFromServiceList.insert(result.get(),pFromModuleImpl,false);
#endif
		return result;
	}
	
	cgcDataSourceInfo::pointer dataSourceInfo = m_cdbcs.getDataSourceInfo(datasource);
	if (dataSourceInfo.get() == NULL)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("getCDBDService getDataSourceInfo(%s) NULL\n"), datasource.c_str());
		return cgcNullCDBCService;
	}
#ifdef USES_MODULE_SERVICE_MANAGER
	cgcServiceInterface::pointer serviceInterface = getService(pFromModuleImpl,dataSourceInfo->getCDBCService());
#else
	cgcServiceInterface::pointer serviceInterface = getService(dataSourceInfo->getCDBCService());
#endif
	if (serviceInterface.get() == NULL)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("getCDBDService DataSource=%s, getService(%s) NULL\n"), datasource.c_str(),dataSourceInfo->getCDBCService().c_str());
		return cgcNullCDBCService;
	}
	result = CGC_CDBCSERVICE_DEF(serviceInterface);
	result->set_datasource(datasource);
	if (!result->initService())
	{
#ifdef USES_MODULE_SERVICE_MANAGER
		resetService(pFromModuleImpl,result);
#else
		resetService(result);
#endif
		m_logModuleImpl.log(LOG_ERROR, _T("getCDBDService DataSource=%s, initService() ERROR\n"), datasource.c_str());
		return cgcNullCDBCService;
	}

	if (!result->open(dataSourceInfo->getCDBCInfo()))
	{
#ifdef USES_MODULE_SERVICE_MANAGER
		resetService(pFromModuleImpl,result);
#else
		resetService(result);
#endif
		m_logModuleImpl.log(LOG_ERROR, _T("getCDBDService DataSource=%s, open() ERROR\n"), datasource.c_str());
		return cgcNullCDBCService;
	}

	ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(dataSourceInfo->getCDBCService());
	if (moduleItem.get() != NULL && moduleItem->getModuleHandle() != NULL)
	{
		void * hModule = moduleItem->getModuleHandle();
		cgcApplication::pointer application;
#ifdef USES_CMODULEMGR
		if (m_pModuleMgr.m_mapModuleImpl.find(hModule, application))
#else
		if (m_mapOpenModules.find(hModule, application))
#endif
		{
			((CModuleImpl*)application.get())->SetCdbcDatasource(datasource);
		}
	}
	// ***不需要处理，在 getService() 已经保存；
	//resultService->setOrgServiceName(dataSourceInfo->getCDBCService());
//#ifdef USES_MODULE_SERVICE_MANAGER
//	m_pFromServiceList.insert(result.get(),pFromModuleImpl,false);
//#endif

	m_cdbcServices.insert(datasource, result);
	return result;
}
#ifdef USES_MODULE_SERVICE_MANAGER
void CGCApp::retCDBDService(const CModuleImpl* pFromModuleImpl, cgcCDBCServicePointer& cdbcservice)
#else
void CGCApp::retCDBDService(cgcCDBCServicePointer& cdbcservice)
#endif
{
	if (cdbcservice.get()!=NULL)
	{
		// *** 如何设计，断开超时没用连接
#ifdef USES_MODULE_SERVICE_MANAGER
		m_pFromServiceList.remove(cdbcservice.get(),pFromModuleImpl,true);
		//printf("**** m_pFromServiceList.size=%d,(%s)sizek=%d\n",m_pFromServiceList.size(),cdbcservice->get_datasource().c_str(),m_pFromServiceList.sizek(cdbcservice.get()));
		// 检查存在返回，不存在继续执行下面。
		if (m_pFromServiceList.exist(cdbcservice.get()))
		{
			//printf("**** m_pFromServiceList (%s) exist\n",cdbcservice->get_datasource().c_str());
			cdbcservice.reset();	// **
			return;
		}
		m_cdbcServices.remove(cdbcservice->get_datasource());
		resetService(pFromModuleImpl,cdbcservice);
#else
		m_cdbcServices.remove(cdbcservice->get_datasource());
		resetService(cdbcservice);
#endif
		cdbcservice.reset();
	}
}
#ifdef USES_MODULE_SERVICE_MANAGER
cgcServiceInterface::pointer CGCApp::getService(const CModuleImpl* pFromModuleImpl, const tstring & serviceName, const cgcValueInfo::pointer& parameter)
#else
cgcServiceInterface::pointer CGCApp::getService(const tstring & serviceName, const cgcValueInfo::pointer& parameter)
#endif
{
	ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(serviceName);
	if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
		return cgcNullServiceInterface;

	void * hModule = moduleItem->getModuleHandle();

	cgcApplication::pointer application;
#ifdef USES_CMODULEMGR
	if (!m_pModuleMgr.m_mapModuleImpl.find(hModule, application)) return cgcNullServiceInterface;
#else
	if (!m_mapOpenModules.find(hModule, application)) return cgcNullServiceInterface;
#endif
	if (((CModuleImpl*)application.get())->getModuleState() < 1)
	{
		if (!InitLibModule(application, moduleItem)) return cgcNullServiceInterface;
	}

#ifdef USES_FUNC_HANDLE2x
#ifdef WIN32
	FPCGC_GetService fp = (FPCGC_GetService)GetProcAddress((HMODULE)hModule, "CGC_GetService");
#else
	FPCGC_GetService fp = (FPCGC_GetService)dlsym(hModule, "CGC_GetService");
#endif
#else
	FPCGC_GetService fp = (FPCGC_GetService)moduleItem->getFpGetService();
#endif
	cgcServiceInterface::pointer resultService;
	if (fp != NULL)
	{
		bool bException = false;
		try
		{
			fp(resultService, parameter);
		}catch (std::exception const & e)
		{
			bException = true;
			m_logModuleImpl.log(LOG_ERROR, _T("getService exception! serviceName=%s: %s\n"), serviceName.c_str(), e.what());
		}catch (...)
		{
			bException = true;
			m_logModuleImpl.log(LOG_ERROR, _T("getService exception! serviceName=%s\n"), serviceName.c_str());
		}

		if (resultService.get() != NULL)
		{
			if (bException)
			{
#ifdef USES_MODULE_SERVICE_MANAGER
				resetService(pFromModuleImpl,resultService);
#else
				resetService(resultService);
#endif
				resultService.reset();
			}else
			{
				resultService->setOrgServiceName(serviceName);
				m_mapServiceModule.insert(resultService, hModule);
#ifdef USES_MODULE_SERVICE_MANAGER
				m_pFromServiceList.insert(resultService.get(),pFromModuleImpl,false);
#endif
			}
		}
	}
	return resultService;
}

void CGCApp::SendModuleResetService(const CModuleImpl* pSentToModuleImpl,const tstring& sServiceName)
{
	BOOST_ASSERT (pSentToModuleImpl != NULL);
	//BOOST_ASSERT (service.get() != NULL);

	// *** 暂时不支持CDBC组件更新。
	//m_cdbcServices.insert(datasource, result);

#ifdef USES_CMODULEMGR
	CLockMap<void*, cgcApplication::pointer>::iterator iterApp;
	for (iterApp=m_pModuleMgr.m_mapModuleImpl.begin(); iterApp!=m_pModuleMgr.m_mapModuleImpl.end(); iterApp++)
#else
	for (iterApp=m_mapOpenModules.begin(); iterApp!=m_mapOpenModules.end(); iterApp++)
#endif
	{
		const CModuleImpl * pModuleImpl = (CModuleImpl*)iterApp->second.get();
		if (pModuleImpl == pSentToModuleImpl)
		{
			void * hModule = pModuleImpl->getModuleItem()->getModuleHandle();
			if (hModule==NULL) return;
#ifdef WIN32
			FPCGC_Module_ResetService farProc_ResetService = (FPCGC_Module_ResetService)GetProcAddress((HMODULE)hModule, "CGC_Module_ResetService");
#else
			FPCGC_Module_ResetService farProc_ResetService = (FPCGC_Module_ResetService)dlsym(hModule, "CGC_Module_ResetService");
#endif
			if (farProc_ResetService!=NULL)
			{
				try
				{
					farProc_ResetService(sServiceName);
				}catch (const std::exception&)
				{
				}catch (...)
				{
				}
			}

			break;
		}
	}


}

void CGCApp::ResetService(void * hModule, const cgcServiceInterface::pointer & service)
{
	BOOST_ASSERT (hModule != NULL);
	BOOST_ASSERT (service.get() != NULL);
#ifdef WIN32
	FPCGC_ResetService fp = (FPCGC_ResetService)GetProcAddress((HMODULE)hModule, "CGC_ResetService");
#else
	FPCGC_ResetService fp = (FPCGC_ResetService)dlsym(hModule, "CGC_ResetService");
#endif

	if (fp != NULL)
	{
		try
		{
			fp(service);
		}catch (std::exception const & e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("resetService exception! serviceName=%s: %s\n"), service->serviceName().c_str(), e.what());
		}catch (...)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("resetService exception! serviceName=%s\n"), service->serviceName().c_str());
		}
	}
}

#ifdef USES_MODULE_SERVICE_MANAGER
void CGCApp::resetService(const CModuleImpl* pFromModuleImpl, const cgcServiceInterface::pointer & service)
#else
void CGCApp::resetService(const cgcServiceInterface::pointer & service)
#endif
{
	if (service.get() ==  NULL) return;

	void * hModule = NULL;
	if (!m_mapServiceModule.find(service, hModule, false)) return;
#ifdef USES_MODULE_SERVICE_MANAGER
	m_pFromServiceList.remove(service.get(), pFromModuleImpl,true);
#endif
	ResetService(hModule, service);
//#ifdef WIN32
//	FPCGC_ResetService fp = (FPCGC_ResetService)GetProcAddress((HMODULE)hModule, "CGC_ResetService");
//#else
//	FPCGC_ResetService fp = (FPCGC_ResetService)dlsym(hModule, "CGC_ResetService");
//#endif
//
//	if (fp != NULL)
//	{
//		try
//		{
//			fp(service);
//		}catch (std::exception const & e)
//		{
//			m_logModuleImpl.log(LOG_ERROR, _T("resetService exception! serviceName=%s: %s\n"), service->serviceName().c_str(), e.what());
//		}catch (...)
//		{
//			m_logModuleImpl.log(LOG_ERROR, _T("resetService exception! serviceName=%s\n"), service->serviceName().c_str());
//		}
//	}
	m_mapServiceModule.remove(service);
}
#ifdef USES_MODULE_SERVICE_MANAGER
HTTP_STATUSCODE CGCApp::executeInclude(const CModuleImpl* pFromModuleImpl, const tstring & url, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
#else
HTTP_STATUSCODE CGCApp::executeInclude(const tstring & url, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
#endif
{
	BOOST_ASSERT (request.get() != NULL);
	BOOST_ASSERT (response.get() != NULL);

	cgcParserHttp::pointer phttpParser;

	cgcServiceInterface::pointer parserService;
	m_fpParserHttpService(parserService, cgcNullValueInfo);
	if (parserService.get() == NULL)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("executeInclude: ParserHttpService GetParser error! %s\n"), url.c_str());
		return STATUS_CODE_500;
	}
	phttpParser = CGC_PARSERHTTPSERVICE_DEF(parserService);
	phttpParser->forward(request->getRequestURI());	// 用于记录前面全目录
	phttpParser->forward(url);

	cgcHttpRequest::pointer requestImpl(new CHttpRequestImpl(((CHttpRequestImpl*)request.get())->getRemote(), phttpParser));
	((CHttpRequestImpl*)requestImpl.get())->setSession(request->getSession());
	((CHttpRequestImpl*)requestImpl.get())->setAttributes(request->getAttributes(true));
	requestImpl->setPageAttributes(request->getPageAttributes());
	//((CHttpResponseImpl*)responseImpl.get())->setSession(sessionImpl);

	HTTP_STATUSCODE statusCode = STATUS_CODE_500;
	try
	{
		const int MAX_FORWARDS = 100;
		int count = 0;
		do
		{
			if (count++ == MAX_FORWARDS) break;
			tstring sAppModuleName;
			if (((CHttpResponseImpl*)response.get())->getForward())
			{
				((CHttpResponseImpl*)response.get())->setForward(false);
				if (requestImpl->getSession().get() != NULL)
				{
					if (phttpParser->isServletRequest() && findPathOrExt(phttpParser->getFunctionName())==std::string::npos)
					//if (phttpParser->isServletRequest())
						sAppModuleName = phttpParser->getModuleName();
					else
						sAppModuleName = m_sHttpServerName;
					if (!((CSessionImpl*)requestImpl->getSession().get())->existModuleItem(sAppModuleName))
					{
						ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(sAppModuleName);
						if (moduleItem.get() == NULL)
						{
							statusCode = STATUS_CODE_500;
							break;
						}
						cgcRemote::pointer pRemote = ((CHttpResponseImpl*)response.get())->getCgcRemote();
						if (((CSessionImpl*)requestImpl->getSession().get())->OnRunCGC_Session_Open(moduleItem,pRemote))
						{
							m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\'.%s opened\n"), requestImpl->getSession()->getId().c_str(),moduleItem->getName().c_str());
						}else
						{
							statusCode = STATUS_CODE_401;
							break;
							// 删除 sessionimpl
							//m_mgrSession.RemoveSessionImpl(sessionImpl);
						}
					}
				}
			}

			if (phttpParser->isServletRequest() && sAppModuleName!=m_sHttpServerName)
				statusCode = ProcHttpAppProto(sAppModuleName,requestImpl, response, phttpParser);
			else
				statusCode = m_fpHttpServer == NULL ? STATUS_CODE_503 : m_fpHttpServer(requestImpl, response);
		}while (statusCode == STATUS_CODE_200 && ((CHttpResponseImpl*)response.get())->getForward());

	}catch (std::exception const & e)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("executeInclude exception! url=%s: %s\n"), url.c_str(), e.what());
	}catch (...)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("executeInclude exception! url=%s\n"), url.c_str());
	}

	return statusCode;
}

#ifdef USES_MODULE_SERVICE_MANAGER
HTTP_STATUSCODE CGCApp::executeService(const CModuleImpl* pFromModuleImpl, const tstring & serviceName, const tstring& function, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response, tstring & outExecuteResult)
#else
HTTP_STATUSCODE CGCApp::executeService(const tstring & serviceName, const tstring& function, const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response, tstring & outExecuteResult)
#endif
{
	BOOST_ASSERT (request.get() != NULL);
	BOOST_ASSERT (response.get() != NULL);

	ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(serviceName);
	if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
		return STATUS_CODE_404;

	void * hModule = moduleItem->getModuleHandle();

	cgcApplication::pointer application;
#ifdef USES_CMODULEMGR
	if (!m_pModuleMgr.m_mapModuleImpl.find(hModule, application)) return STATUS_CODE_404;
#else
	if (!m_mapOpenModules.find(hModule, application)) return STATUS_CODE_404;
#endif
	if (((CModuleImpl*)application.get())->getModuleState() < 1)
	{
		if (!InitLibModule(application, moduleItem)) return STATUS_CODE_501;
	}

	tstring doFunction("do");
	doFunction.append(function);

#ifdef WIN32
	FPCGCHttpApi fp = (FPCGCHttpApi)GetProcAddress((HMODULE)hModule, doFunction.c_str());
#else
	FPCGCHttpApi fp = (FPCGCHttpApi)dlsym(hModule, doFunction.c_str());
#endif

	HTTP_STATUSCODE result = (fp == NULL) ? STATUS_CODE_404 : STATUS_CODE_500;
	if (fp != NULL)
	{
		try
		{
			application->clearExecuteResult();
			result = fp(request, response);
			outExecuteResult = application->getExecuteResult();
		}catch (std::exception const & e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("runService exception! serviceName=%s; %s: %s\n"), serviceName.c_str(), function.c_str(), e.what());
		}catch (...)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("runService exception! serviceName=%s; %s\n"), serviceName.c_str(), function.c_str());
		}
	}
	return result;
}

cgcResponse::pointer CGCApp::getLastResponse(const tstring & sessionId,const tstring& moduleName) const
{
	cgcSession::pointer pSession = sessionId.size()==DEFAULT_HTTP_SESSIONID_LENGTH?m_mgrSession2.GetSessionImpl(sessionId):m_mgrSession1.GetSessionImpl(sessionId);
	return pSession.get() == NULL ? cgcNullResponse : pSession->getLastResponse(moduleName);
}
cgcResponse::pointer CGCApp::getHoldResponse(const tstring& sessionId,unsigned long remoteId)
{
	cgcSession::pointer pSession = sessionId.size()==DEFAULT_HTTP_SESSIONID_LENGTH?m_mgrSession2.GetSessionImpl(sessionId):m_mgrSession1.GetSessionImpl(sessionId);
	return pSession.get() == NULL ? cgcNullResponse : pSession->getHoldResponse(remoteId);
}

cgcAttributes::pointer CGCApp::getAttributes(bool create)
{
	if (create && m_attributes.get() == NULL)
		m_attributes = cgcAttributes::pointer(new AttributesImpl());
	return m_attributes;
}

cgcAttributes::pointer CGCApp::getAppAttributes(const tstring & appName) const
{
	ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(appName);
	if (moduleItem.get() == NULL || moduleItem->getModuleHandle() == NULL)
		return cgcNullAttributes;

	cgcApplication::pointer applicationImpl;
#ifdef USES_CMODULEMGR
	m_pModuleMgr.m_mapModuleImpl.find(moduleItem->getModuleHandle(), applicationImpl);
#else
	m_mapOpenModules.find(moduleItem->getModuleHandle(), applicationImpl);
#endif
	return applicationImpl.get() == NULL ? cgcNullAttributes : applicationImpl->getAttributes();
}

tstring CGCApp::SetNewMySessionId(cgcParserHttp::pointer& phttpParser,const tstring& sSessionId)
{
	if (sSessionId.empty())
	{
		char lpszMySessionId[64];
		static unsigned int the_sid_index=0;
		sprintf(lpszMySessionId, "%d%d%d",(int)time(0),(int)rand(),(the_sid_index++));
		MD5 md5;
		md5.update((const unsigned char*)lpszMySessionId,strlen(lpszMySessionId));
		md5.finalize();
		const std::string sMySessionId = md5.hex_digest();
		phttpParser->setCookieMySessionId(sMySessionId);
	}else
	{
		phttpParser->setCookieMySessionId(sSessionId);
	}
	//phttpParser->setHeader("P3P","CP=\"CAO PSA OUR\"");	// 解决IE多窗口SESSION不正常问题
	//phttpParser->setHeader("P3P","\"policyref=\"http://test-lc.entboost.com/w3c/p3p.xml\" CP=\"ALL DSP COR CUR OUR IND PUR\"");
	return phttpParser->getCookieMySessionId();
}

void CGCApp::GetHttpParserPool(cgcParserHttp::pointer& phttpParser)
{
	if (m_pHttpParserPool.front(phttpParser))
	{
		phttpParser->init();
		return;
	}
	m_tLastNewParserHttpTime = time(0);
	cgcServiceInterface::pointer parserService;
	m_fpParserHttpService(parserService, cgcNullValueInfo);
	if (parserService.get() != NULL)
	{
		phttpParser = CGC_PARSERHTTPSERVICE_DEF(parserService);
	}
}
void CGCApp::SetHttpParserPool(const cgcParserHttp::pointer& phttpParser)
{
	//return;
	// *** 200 max pool size
	if (m_pHttpParserPool.size()<200)
	{
		m_pHttpParserPool.add(phttpParser);
	}
}
void CGCApp::CheckHttpParserPool(void)
{
	// *** 20 min pool size
	if (m_pHttpParserPool.size()>20 && (time(0)-m_tLastNewParserHttpTime)>5)
	{
		cgcParserHttp::pointer phttpParser;
		m_pHttpParserPool.front(phttpParser);
	}
}

//void CGCApp::OnReceiveData(const ReceiveBuffer::pointer& data)
//{
//	printf("******** OnReceiveData size=%d\n",data->size());
//	if (data->size()>=FCGI_HEADER_LEN)
//	{
//		FCGI_Header header;
//		memcpy(&header,data->data(),8);
//		
//		if(header.version != FCGI_VERSION_1) {
//			return ;//FCGX_UNSUPPORTED_VERSION;
//		}
//		int requestId =        (header.requestIdB1 << 8)
//			+ header.requestIdB0;
//		int contentLen = (header.contentLengthB1 << 8)
//			+ header.contentLengthB0;
//		int paddingLen = header.paddingLength;
//
//		printf("******** RequestId=%d,type=%d,contentLen=%d\n",requestId,(int)header.type,contentLen);
//
//		if (header.type == FCGI_STDOUT)
//		{
//			printf("********\n%s\n", data->data()+8);
//		}else if (header.type == FCGI_STDERR)
//		{
//			printf("********\n%s\n", data->data()+8);
//		}else if (header.type == FCGI_END_REQUEST)
//		{
//		}
//
//	}
//
//}
//static FCGI_Header MakeHeader(
//        int type,
//        int requestId,
//        int contentLength,
//        int paddingLength)
//{
//    FCGI_Header header;
//    //ASSERT(contentLength >= 0 && contentLength <= FCGI_MAX_LENGTH);
//    //ASSERT(paddingLength >= 0 && paddingLength <= 0xff);
//    header.version = FCGI_VERSION_1;
//    header.type             = (unsigned char) type;
//    header.requestIdB1      = (unsigned char) ((requestId     >> 8) & 0xff);
//    header.requestIdB0      = (unsigned char) ((requestId         ) & 0xff);
//    header.contentLengthB1  = (unsigned char) ((contentLength >> 8) & 0xff);
//    header.contentLengthB0  = (unsigned char) ((contentLength     ) & 0xff);
//    header.paddingLength    = (unsigned char) paddingLength;
//    header.reserved         =  0;
//    return header;
//}
//static unsigned char* FCGI_BuildNameValueBody(int nRequestId, const char *name,int nameLen,const char *value,int valueLen, int * pOutSize)
//{
//	unsigned char* lpszBuffer = new unsigned char[FCGI_HEADER_LEN+8+nameLen+valueLen];
//	int nOffset = FCGI_HEADER_LEN;
//	if (nameLen < 0x80) {
//		lpszBuffer[nOffset] = (unsigned char) nameLen;
//		nOffset += 1;
//	} else {
//		lpszBuffer[nOffset]   = (unsigned char) ((nameLen >> 24) | 0x80);
//		lpszBuffer[nOffset+1] = (unsigned char) (nameLen >> 16);
//		lpszBuffer[nOffset+2] = (unsigned char) (nameLen >> 8);
//		lpszBuffer[nOffset+3] = (unsigned char) nameLen;
//		nOffset += 4;
//	}
//	if (valueLen < 0x80) {
//		lpszBuffer[nOffset] = (unsigned char) valueLen;
//		nOffset += 1;
//	} else {
//		lpszBuffer[nOffset]   = (unsigned char) ((valueLen >> 24) | 0x80);
//		lpszBuffer[nOffset+1] = (unsigned char) (valueLen >> 16);
//		lpszBuffer[nOffset+2] = (unsigned char) (valueLen >> 8);
//		lpszBuffer[nOffset+3] = (unsigned char) valueLen;
//		nOffset += 4;
//	}
//	const FCGI_Header header = MakeHeader(FCGI_PARAMS,nRequestId,nOffset-FCGI_HEADER_LEN+nameLen+valueLen,0);
//	memcpy(lpszBuffer,&header, FCGI_HEADER_LEN);
//	memcpy(lpszBuffer+nOffset,name, nameLen);
//	memcpy(lpszBuffer+(nOffset+nameLen),value, valueLen);
//	*pOutSize = nOffset+nameLen+valueLen;
//	return lpszBuffer;
//}
HTTP_STATUSCODE CGCApp::ProcHttpData(const unsigned char * recvData, size_t dataSize,const cgcRemote::pointer& pcgcRemote)
{
	bool findMultiPartEnd = false;
	cgcParserHttp::pointer phttpParser;
	bool bCanSetParserToPool = false;

	cgcMultiPart::pointer currentMultiPart;
	if (m_mapMultiParts.find(pcgcRemote->getRemoteId(), currentMultiPart))
	{
		phttpParser = CGC_PARSERHTTPSERVICE_DEF(currentMultiPart->getParser());
	}
	if (phttpParser.get() == NULL)
	{
		GetHttpParserPool(phttpParser);
		if (phttpParser.get() == NULL)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("ParserHttpService GetParser error! %s\n"), recvData);
			return STATUS_CODE_500;
		}
		bCanSetParserToPool = true;
		//cgcServiceInterface::pointer parserService;
		//m_fpParserHttpService(parserService, cgcNullValueInfo);
		//if (parserService.get() == NULL)
		//{
		//	m_logModuleImpl.log(LOG_ERROR, _T("ParserHttpService GetParser error! %s\n"), recvData);
		//	return STATUS_CODE_500;
		//}
		//phttpParser = CGC_PARSERHTTPSERVICE_DEF(parserService);
	}
	cgcHttpResponse::pointer responseImpl;
	//cgcHttpRequest::pointer requestImpl(new CHttpRequestImpl(pcgcRemote, phttpParser));
	//cgcHttpResponse::pointer responseImpl(new CHttpResponseImpl(pcgcRemote, phttpParser));
	//((CHttpRequestImpl*)requestImpl.get())->setContent((const char*)recvData, dataSize);
	HTTP_STATUSCODE statusCode = STATUS_CODE_200;
	const bool parseResult = phttpParser->doParse(recvData, dataSize);
	if (!parseResult)
	{
		cgcMultiPart::pointer multiPart = phttpParser->getMultiPart();
		//printf("******** doParse false:%d, multiparth=%d\n",pcgcRemote->getRemoteId(),multiPart.get()==NULL?0:1);
		if (multiPart.get() != NULL)
		{
			if (currentMultiPart.get() != NULL && currentMultiPart.get() != multiPart.get())
			{
				m_mapMultiParts.remove(pcgcRemote->getRemoteId());
				currentMultiPart.reset();
			}
			if (currentMultiPart.get() == NULL)
			{
				multiPart->setParser(phttpParser);
				//printf("******** m_mapMultiParts.insert:%d\n",pcgcRemote->getRemoteId());
				m_mapMultiParts.insert(pcgcRemote->getRemoteId(), multiPart);
			}
			tstring sExpect(phttpParser->getHeader("expect",""));
			//printf("******** Expect: %s\n",sExpect.c_str());
			std::transform(sExpect.begin(), sExpect.end(), sExpect.begin(), ::tolower);
			if (sExpect=="100-continue")
			{
				phttpParser->setStatusCode(STATUS_CODE_100);
				CHttpResponseImpl pResponseImpl(pcgcRemote, phttpParser);
				pResponseImpl.sendResponse();
				phttpParser->setStatusCode(STATUS_CODE_200);
			}
			return STATUS_CODE_100;	// ***不需要返回，客户端会一直上传；
		}else
		{
			statusCode = phttpParser->getStatusCode();
			if (statusCode==STATUS_CODE_200)
				statusCode = STATUS_CODE_501;
			m_logModuleImpl.log(LOG_WARNING, _T("ProcHttpData doParse false: state=%d:size=%d\n"), (int)statusCode,dataSize);
		}
		responseImpl = cgcHttpResponse::pointer(new CHttpResponseImpl(pcgcRemote, phttpParser));
	}else// if (parseResult)
	{
		//printf("**** phttpParser->getStatusCode()=%d!\n", phttpParser->getStatusCode());
		if (phttpParser->getStatusCode()!=STATUS_CODE_200)
		{
			statusCode = phttpParser->getStatusCode();
			CHttpResponseImpl pResponseImpl(pcgcRemote, phttpParser);
			pResponseImpl.sendResponse();
			pcgcRemote->invalidate(true);
			if (bCanSetParserToPool)
				SetHttpParserPool(phttpParser);
			return statusCode;
		}

		cgcHttpRequest::pointer requestImpl(new CHttpRequestImpl(pcgcRemote, phttpParser));
		responseImpl = cgcHttpResponse::pointer(new CHttpResponseImpl(pcgcRemote, phttpParser));
		((CHttpRequestImpl*)requestImpl.get())->setContent((const char*)recvData, dataSize);
		// Parse OK.
		//printf("******** doParse true:%d\n",pcgcRemote->getRemoteId());
		if (currentMultiPart.get() != NULL)
		{
			//printf("******** m_mapMultiParts.remove:%d\n",pcgcRemote->getRemoteId());
			m_mapMultiParts.remove(pcgcRemote->getRemoteId());
			currentMultiPart.reset();
		}

		bool bSetRemoveSession = false;
		cgcSession::pointer sessionImpl;
		if (phttpParser->isEmptyCookieMySessionId())
		{
			sessionImpl = m_mgrSession2.GetSessionImplByRemote(pcgcRemote->getRemoteId());
			if (sessionImpl.get()!=NULL)
			{
				SetNewMySessionId(phttpParser,sessionImpl->getId());
			}else
			{
				SetNewMySessionId(phttpParser);
			}
		}else
		{
			sessionImpl = m_mgrSession2.GetSessionImpl(phttpParser->getCookieMySessionId());
			if (sessionImpl.get()==NULL)
			{
				sessionImpl = m_mgrSession2.GetSessionImplByRemote(pcgcRemote->getRemoteId());
				if (sessionImpl.get()!=NULL)
				{
					SetNewMySessionId(phttpParser,sessionImpl->getId());
				}else
				{
					printf("**** Can not find Session'%s',changed!\n", phttpParser->getCookieMySessionId().c_str());
					SetNewMySessionId(phttpParser);
				}
				//printf("**** Can not find Session'%s',changed!\n", phttpParser->getCookieMySessionId().c_str());
				//SetNewMySessionId(phttpParser);
			}else
			{
				bSetRemoveSession = true;
			}
		}

		/*
		cgcSession::pointer sessionImpl = m_mgrSession.GetSessionImplByRemote(pcgcRemote->getRemoteId());
		if (phttpParser->isEmptyCookieMySessionId())
		{
			const tstring sSessionId = sessionImpl.get()==NULL?"":sessionImpl->getId();	// add by hd 2014-10-29
			SetNewMySessionId(phttpParser,sSessionId);
		}else if (sessionImpl.get()==NULL)	// add by hd 2014-10-29
		//}else
		{
			sessionImpl = m_mgrSession.GetSessionImpl(phttpParser->getCookieMySessionId());
			if (sessionImpl.get()==NULL)
			{
				printf("**** Can not find Session'%s',changed!\n", phttpParser->getCookieMySessionId().c_str());
				SetNewMySessionId(phttpParser);
			}
		}else if (sessionImpl.get()!=NULL && sessionImpl->getId()!=phttpParser->getCookieMySessionId())
		{
			// add by hd 2014-10-29
			printf("**** Find old session '%s',changed to '%s'!\n",phttpParser->getCookieMySessionId().c_str(),sessionImpl->getId().c_str());
			SetNewMySessionId(phttpParser,sessionImpl->getId());
		}
		*/

		CSessionImpl * pHttpSessionImpl = (CSessionImpl*)sessionImpl.get();
		if (phttpParser->getStatusCode()==STATUS_CODE_200 && bSetRemoveSession)
		{
			m_mgrSession2.SetRemoteSession(pcgcRemote->getRemoteId(),sessionImpl);
			//m_mgrSession2.SetRemoteSession(pcgcRemote->getRemoteId(),phttpParser->getCookieMySessionId());
		}

		// ****解决IE多窗口COOKIE丢失问题
		phttpParser->setHeader("P3P","CP=\"CAO PSA OUR\"");

		tstring sAppModuleName(phttpParser->getModuleName());
		if (phttpParser->isServletRequest())
		{
			CServletInfo::pointer servletInfo = m_parseWeb.getServletInfo(sAppModuleName);
			if (servletInfo.get() != NULL)
			{
				sAppModuleName = servletInfo->getServletApp();
				if (sAppModuleName.empty())
					sAppModuleName = m_sHttpServerName;
			}else if (findPathOrExt(phttpParser->getFunctionName())!=std::string::npos)
			{
				sAppModuleName = m_sHttpServerName;
			}
		}else
		{
			sAppModuleName = m_sHttpServerName;
		}
		if (sessionImpl.get() == NULL || !pHttpSessionImpl->existModuleItem(sAppModuleName))
		{
			// 新SESSION，或者换MODULE
			ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(sAppModuleName);
			if (moduleItem.get() != NULL)
			{
				if (sessionImpl.get() == NULL)
				{
					bCanSetParserToPool = false;
					sessionImpl = m_mgrSession2.SetSessionImpl(moduleItem,pcgcRemote,phttpParser);
					pHttpSessionImpl = (CSessionImpl*)sessionImpl.get();
				}
				if (pHttpSessionImpl->OnRunCGC_Session_Open(moduleItem,pcgcRemote))
				{
					m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\'.%s opened\n"), sessionImpl->getId().c_str(),moduleItem->getName().c_str());
				}else
				{
					statusCode = STATUS_CODE_401;
					sessionImpl.reset();
					pHttpSessionImpl = NULL;
				}
			}
		}

		if (sessionImpl.get() != NULL)
		{
			pHttpSessionImpl->setDataResponseImpl(sAppModuleName,pcgcRemote);
		}
//		if (sessionImpl.get() == NULL)
//		{
//			// 这段代码没用了
////			unsigned long remoteId = pcgcRemote->getRemoteId();
////			bool bOpenSession = false;
////			if (m_mapRemoteOpenSes.find(remoteId, bOpenSession))
////			{
////				// Wait for open the session
////#ifdef WIN32
////				Sleep(100);
////#else
////				usleep(100000);
////#endif
////			}else
////			{
////				m_mapRemoteOpenSes.insert(remoteId, true);
////			}
////			sessionImpl = m_mgrSession.GetSessionImplByRemote(remoteId);
////
////			// 客户端没有OPEN SESSION, sSessionId=APPNAME.
////			// 临时打开SESSION
////			if (sessionImpl.get() == NULL)
////			{
////				tstring sAppModuleName;
////				if (phttpParser->isServletRequest())
////					sAppModuleName = phttpParser->getModuleName();
////				else
////					sAppModuleName = m_sHttpServerName;
////				ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(sAppModuleName);
////				if (moduleItem.get() != 0)
////				{
////					//cgcServiceInterface::pointer parserService;
////					//m_fpParserHttpService(parserService, cgcNullValueInfo);
////					//if (parserService.get() == NULL) return STATUS_CODE_500;
////					//sessionImpl = m_mgrSession.SetSessionImpl(pcgcRemote, pcgcParserLastSession);
////					sessionImpl = m_mgrSession.SetSessionImpl(pcgcRemote, phttpParser);
////					pHttpSessionImpl = (CSessionImpl*)sessionImpl.get();
////					// 调用模块 CGC_Session_Open
////					if (pHttpSessionImpl->OnRunCGC_Session_Open(moduleItem))
////					{
////						m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\'.%s opened\n"), sessionImpl->getId().c_str(),moduleItem->getName().c_str());
////					}else
////					{
////						// 删除 sessionimpl
////						//m_mgrSession.RemoveSessionImpl(sessionImpl);
////						statusCode = STATUS_CODE_401;
////						sessionImpl.reset();
////						pHttpSessionImpl = NULL;
////					}
////				}
////			}
////			m_mapRemoteOpenSes.remove(remoteId);
//		}else
//		{
//			// Record cgcRemote
//			pHttpSessionImpl->setDataResponseImpl(sAppModuleName,pcgcRemote);
//		}

		try
		{
			if (bCanSetParserToPool)
			{
				bCanSetParserToPool = false;
				((CHttpResponseImpl*)responseImpl.get())->SetParserHttpHandler(this);
			}

			if (sessionImpl.get() != NULL)
			{
				if (phttpParser->getKeepAlive() > 0)
					sessionImpl->setMaxInactiveInterval(phttpParser->getKeepAlive());

				//if (m_fpHttpStruct != NULL)
				//	m_fpHttpStruct(requestImpl, responseImpl);
				const int MAX_FORWARDS = 100;
				int count = 0;
				do
				{
					if (count++ == MAX_FORWARDS) break;
					if (((CHttpResponseImpl*)responseImpl.get())->getForward())
					{
						((CHttpResponseImpl*)responseImpl.get())->setForward(false);

						//tstring sAppModuleName;
						if (phttpParser->isServletRequest())
						{
							sAppModuleName = phttpParser->getModuleName();
							CServletInfo::pointer servletInfo = m_parseWeb.getServletInfo(sAppModuleName);
							if (servletInfo.get() != NULL)
							{
								sAppModuleName = servletInfo->getServletApp();
								if (sAppModuleName.empty())
									sAppModuleName = m_sHttpServerName;
							}else if (findPathOrExt(phttpParser->getFunctionName())!=std::string::npos)
							{
								sAppModuleName = m_sHttpServerName;
							}
						}else
						{
							sAppModuleName = m_sHttpServerName;
						}
						if (!pHttpSessionImpl->existModuleItem(sAppModuleName))
						{
							ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(sAppModuleName);
							if (moduleItem.get() == NULL)
							{
								statusCode = STATUS_CODE_500;
								break;
							}
							if (pHttpSessionImpl->OnRunCGC_Session_Open(moduleItem,pcgcRemote))
							{
								m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\'.%s opened\n"), sessionImpl->getId().c_str(),moduleItem->getName().c_str());
							}else
							{
								statusCode = STATUS_CODE_401;
								break;
								// 删除 sessionimpl
								//m_mgrSession.RemoveSessionImpl(sessionImpl);
							}
						}
					}

					((CHttpRequestImpl*)requestImpl.get())->setSession(sessionImpl);
					((CHttpResponseImpl*)responseImpl.get())->setSession(sessionImpl);
					if (phttpParser->isServletRequest() && sAppModuleName!=m_sHttpServerName)
						statusCode = ProcHttpAppProto(sAppModuleName, requestImpl, responseImpl, phttpParser);
					else
						statusCode = m_fpHttpServer == NULL ? STATUS_CODE_503 : m_fpHttpServer(requestImpl, responseImpl);
				}while (statusCode == STATUS_CODE_200 && ((CHttpResponseImpl*)responseImpl.get())->getForward());
				//printf("**** RemoteId=%lu, isKeepAlive =%d (sid=%s)\n",pcgcRemote->getRemoteId(),(int)(requestImpl->isKeepAlive()?1:0),sessionImpl->getId().c_str());
				//if (!requestImpl->isKeepAlive() && sessionImpl->getMaxInactiveInterval()>1)
				//{
				//	//printf("**** isKeepAlive is false (sid=%s)\n",sessionImpl->getId().c_str());
				//	sessionImpl->setMaxInactiveInterval(1);
				//}
			}
		}catch(std::exception const &e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("exception, RequestURL \'%s\', lasterror=0x%x\n"),
				phttpParser->getRequestURL().c_str(),
				GetLastError());
			m_logModuleImpl.log(LOG_ERROR, _T("'%s'\n"), e.what());
			statusCode = STATUS_CODE_500;
		}catch(...)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("exception, RequestURL \'%s\', lasterror=0x%x\n"),
				phttpParser->getRequestURL().c_str(),
				GetLastError());
			statusCode = STATUS_CODE_500;
		}
	//}else
	//{
	//	//if (pHttpSessionImpl)
	//	//{
	//	//	// 第一次处理不成功，保存未处理数据
	//	//	if (!pHttpSessionImpl->isHasPrevData())
	//	//	{
	//	//		pHttpSessionImpl->delPrevDatathread(false);
	//	//		pHttpSessionImpl->addPrevData((const char*)recvData);

	//	//		// 创建一个超时处理线程
	//	//		pHttpSessionImpl->newPrevDataThread();
	//	//		statusCode = STATUS_CODE_100;
	//	//	}else
	//	//	{
	//	//		//pHttpSessionImpl->addPrevData((const char*)recvData);
	//	//		statusCode = STATUS_CODE_400;
	//	//	}
	//	//}else
	//	{
	//		// 之前没有SESSION，直接返回错误
	//		statusCode = STATUS_CODE_400;
	//	}
	}

	//if (statusCode != STATUS_CODE_200)
	{
		responseImpl->setStatusCode(statusCode);
		//if (responseImpl->getBodySize() == 0)
		//	responseImpl->println(cgcGetStatusCode(statusCode));
	}
	//if (statusCode!=STATUS_CODE_200)
	//	responseImpl->setKeepAlive(-1);
	// 自动发送
	//printf("**** ProcHttpData::sendResponse...\n");
	((CHttpResponseImpl*)responseImpl.get())->sendResponse();
	//const int ret = ((CHttpResponseImpl*)responseImpl.get())->sendResponse();
	//printf("**** ProcHttpData::sendResponse ret=%d,isKeepAlive()=%d\n",ret,(int)(phttpParser->isKeepAlive()?1:0));
	//if (statusCode != STATUS_CODE_200)
	//if (statusCode == STATUS_CODE_413)
	//if (!requestImpl->isKeepAlive())
	if (!phttpParser->isKeepAlive())
	//if (!phttpParser->isKeepAlive() || (statusCode!=STATUS_CODE_200 && statusCode!=STATUS_CODE_206 && statusCode!=STATUS_CODE_100))
	{
		if (statusCode != STATUS_CODE_200)	// STATUS_CODE_413
			pcgcRemote->invalidate(true);
		else
			m_pNotKeepAliveRemoteList.add(CNotKeepAliveRemote::create(pcgcRemote));
	}
	
	if (bCanSetParserToPool)
		SetHttpParserPool(phttpParser);
	return statusCode;
}


void CGCApp::GetSotpParserPool(cgcParserSotp::pointer& pcgcParser)
{
	if (m_pSotpParserPool.front(pcgcParser))
	{
		pcgcParser->init();
		return;
	}
	m_tLastNewParserSotpTime = time(0);
	cgcServiceInterface::pointer parserService;
	m_fpParserSotpService(parserService, cgcNullValueInfo);
	if (parserService.get() != NULL)
		pcgcParser = CGC_PARSERSOTPSERVICE_DEF(parserService);
}
void CGCApp::SetSotpParserPool(const cgcParserSotp::pointer& pcgcParser)
{
	//return;
	// *** 200 max pool size
	if (m_pSotpParserPool.size()<200)
	{
		m_pSotpParserPool.add(pcgcParser);
	}
}
void CGCApp::CheckSotpParserPool(void)
{
	// *** 20 min pool size
	//printf("** m_pSotpParserPool.size()=%d\n",m_pSotpParserPool.size());
	if (m_pSotpParserPool.size()>20 && (time(0)-m_tLastNewParserSotpTime)>5)
	{
		cgcParserSotp::pointer pcgcParser;
		m_pSotpParserPool.front(pcgcParser);
	}
}
void CGCApp::ProcCheckParserPool(void)
{
	static unsigned int theIndex = 0;
	theIndex++;
	if ((theIndex%3)==2)
	{
		CheckSotpParserPool();
		CheckHttpParserPool();
	}
}
void CGCApp::ProcCheckAutoUpdate(void)
{
	if (m_bStopedApp) return;
	if (m_pProcAutoUpdate.get()!=NULL)
	{
		// * processing auto update
		return;
	}
	tstring xmlFile(m_sModulePath);
	xmlFile.append(_T("/auto_update/auto_update.xml"));
	if (!FileIsExist(xmlFile.c_str()))
	{
		return;
	}
#ifdef USES_THREAD_STACK_SIZE
	boost::thread_attributes attrs;
	attrs.set_stack_size(CGC_THREAD_STACK_MIN);
	m_pProcAutoUpdate = boost::shared_ptr<boost::thread>(new boost::thread(attrs,boost::bind(&CGCApp::do_autoupdate, this)));
#else
	m_pProcAutoUpdate = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CGCApp::do_autoupdate, this)));
#endif
}
void CGCApp::DetachAutoUpdateThread(void)
{
	if (m_pProcAutoUpdate.get()!=NULL)
	{
		m_pProcAutoUpdate->detach();	// *
		m_pProcAutoUpdate.reset();
	}
}

int CGCApp::ProcCgcData(const unsigned char * recvData, size_t dataSize, const cgcRemote::pointer& pcgcRemote)
{
	cgcParserSotp::pointer pcgcParser;
	GetSotpParserPool(pcgcParser);
	if (pcgcParser.get() == NULL)
	//cgcServiceInterface::pointer parserService;
	//m_fpParserSotpService(parserService, cgcNullValueInfo);
	//if (parserService.get() == NULL)
	{
		m_logModuleImpl.log(LOG_ERROR, _T("ParserSotpService GetParser error! %s\n"), recvData);
		return -1;
	}

	//cgcParserSotp::pointer pcgcParser = CGC_PARSERSOTPSERVICE_DEF(parserService);
	pcgcParser->setParseCallback(this);
	const bool parseResult = pcgcParser->doParse(recvData, dataSize);
	if (pcgcParser->getProtoType()==SOTP_PROTO_TYPE_RTP)
	{
		if (pcgcParser->isRtpCommand())
		{
			//tagSotpRtpCommand pRtpCommand = pcgcParser->getRtpCommand();
			//printf("**** command=%d,roomid=%lld,srcid=%lld,size=%d\n",pRtpCommand.m_nCommand,pRtpCommand.m_nRoomId, pRtpCommand.m_nSrcId,SOTP_RTP_COMMAND_SIZE);
			const int serverPort = pcgcRemote->getServerPort();
			CSotpRtpSession::pointer pRtpSession;
			if (!m_pRtpSession.find(serverPort,pRtpSession))
			{
				pRtpSession = CSotpRtpSession::create(true);
				CSotpRtpSession::pointer pRtpSessionTemp;
				m_pRtpSession.insert(serverPort, pRtpSession, false, &pRtpSessionTemp);
				if (pRtpSessionTemp.get()!=NULL)
					pRtpSession = pRtpSessionTemp;
			}
			//pRtpSession->doRtpCommand(pcgcParser->getRtpCommand(),pcgcRemote,false,this,(void*)&serverPort);
			pRtpSession->doRtpCommand(pcgcParser->getRtpCommand(),pcgcRemote,false,this,(void*)serverPort);
		}else if (pcgcParser->isRtpData())
		{
			const int serverPort = pcgcRemote->getServerPort();
			CSotpRtpSession::pointer pRtpSession;
			if (m_pRtpSession.find(serverPort,pRtpSession))
			{
				pRtpSession->doRtpData(pcgcParser->getRtpDataHead(),pcgcParser->getRecvAttachment(),pcgcRemote);
			}

			//tagSotpRtpDataHead pRtpDataHead = pcgcParser->getRtpDataHead();
			////printf("**** data-head : roomid=%lld,srcid=%lld,size=%d\n",pRtpDataHead.m_nRoomId,pRtpDataHead.m_nSrcId,SOTP_RTP_DATA_HEAD_SIZE);
			//m_pRtpSession.doRtpData(pcgcParser->getRtpDataHead(),pcgcParser->getRecvAttachment(),pcgcRemote);
		}
		SetSotpParserPool(pcgcParser);
		return 1;
	}

	cgcSession::pointer sessionImpl = m_mgrSession1.GetSessionImplByRemote(pcgcRemote->getRemoteId());
	cgcSotpRequest::pointer requestImpl(new CSotpRequestImpl(pcgcRemote, pcgcParser));

	//
	CSotpRequestImpl * pRequestImpl = (CSotpRequestImpl*)requestImpl.get();
	pRequestImpl->setContent((const char*)recvData, dataSize);
	CSessionImpl * pSessionImpl = (CSessionImpl*)sessionImpl.get();
	if (pSessionImpl)
	{
		((CSotpRequestImpl*)requestImpl.get())->setSession(sessionImpl);
		// Record cgcRemote
		pSessionImpl->setDataResponseImpl("",pcgcRemote);
	}

	//bool parseResult = false;
	//pcgcParser->setParseCallback(this);
	//if (pSessionImpl != NULL && pSessionImpl->isHasPrevData())
	//{
	//	// 之前有未处理数据
	//	const std::string & sRecvData = pSessionImpl->addPrevData((const char*)recvData);
	//	parseResult = pcgcParser->doParse((const unsigned char*)sRecvData.c_str(), sRecvData.size());
	//}else
	//{
	//	parseResult = pcgcParser->doParse(recvData, dataSize);
	//}

	//printf("**** doParse ok=%d\n",parseResult?1:0);

	bool bCanSetParserToPool = true;
	if (parseResult)
	{
		// 成功解析
		//printf("%s\n", recvData);
		if (pcgcParser->hasSeq())
		{
			const short seq = pcgcParser->getSeq();
			if (pcgcParser->isAckProto())
			{
				if (pSessionImpl)
					pSessionImpl->ProcessAckSeq(seq);
				SetSotpParserPool(pcgcParser);
				return 1;
			}
			if (pcgcParser->isNeedAck())
			{
				const tstring responseData(pcgcParser->getAckResult(seq));
				pcgcRemote->sendData((const unsigned char*)responseData.c_str(), responseData.length());
			}
			//size_t nContentLength = dataSize;
			if (pSessionImpl)
			{
				//if (pSessionImpl->isHasPrevData())
				//	nContentLength = pSessionImpl->getPrevDataLength();
#ifndef USES_DISABLE_PREV_DATA
				pSessionImpl->clearPrevData();
#endif

				//printf("seq=%d,sid=%s\n", seq, sessionImpl->getId().c_str());
				if (pSessionImpl->ProcessDuplationSeq(seq))
				{
					//printf("seq=%d,duplation \n", seq);
					SetSotpParserPool(pcgcParser);
					return 1;
				}
			}
		}
		try
		{
			if (pcgcParser->isSessionProto())
			{
				ProcSesProto(requestImpl, pcgcParser, pcgcRemote, sessionImpl);
			}else if (pcgcParser->isAppProto() || pcgcParser->getProtoType()==SOTP_PROTO_TYPE_SYNC)
			{
				bCanSetParserToPool = false;
				cgcSotpResponse::pointer responseImpl(new CSotpResponseImpl(pcgcRemote, pcgcParser, pSessionImpl,this));
				responseImpl->setEncoding(m_parseDefault.getDefaultEncoding());
				((CSotpResponseImpl*)responseImpl.get())->setSession(sessionImpl);
				ProcAppProto(requestImpl, responseImpl, pcgcParser, sessionImpl);
			//}else if (pcgcParser->getProtoType()==SOTP_PROTO_TYPE_SYNC)
			//{
			//	ProcSyncProto(requestImpl, pcgcParser, pcgcRemote, sessionImpl);
			}/* ? else if (pRequestImpl->m_cwsInvoke.isClusterProto())
			{
				ProcCluProto(pRequestImpl, cwssRemote);
			}*/else
			{
				m_logModuleImpl.log(LOG_WARNING, _T("invalidate cgc proto!\n"));
				//retCode = -115;
			}
		}catch(std::exception const &e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("exception, sid \'%s\', cid \'%d\', cs \'%d\', lasterror=0x%x\n"),
				pcgcParser->getSid().c_str(),
				pcgcParser->getCallid(),
				pcgcParser->getSign(),
				GetLastError());
			m_logModuleImpl.log(LOG_ERROR, _T("'%s'\n"), e.what());
			//retCode = -111;
		}catch(...)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("exception, sid \'%s\', cid \'%d\', cs \'%d\', lasterror=0x%x\n"),
				pcgcParser->getSid().c_str(),
				pcgcParser->getCallid(),
				pcgcParser->getSign(),
				GetLastError());
			//retCode = -111;
		}
	}else
	{
		if (pSessionImpl)
		{
			// 第一次处理不成功，保存未处理数据
#ifdef USES_DISABLE_PREV_DATA
			CSotpResponseImpl responseImpl(pcgcRemote, pcgcParser, pSessionImpl,NULL);
			responseImpl.SetEncoding(m_parseDefault.getDefaultEncoding());
			responseImpl.sendAppCallResult(-106);
#else
			if (!pSessionImpl->isHasPrevData())
			{
				pSessionImpl->delPrevDatathread(false);
				pSessionImpl->addPrevData((const char*)recvData);

				// 创建一个超时处理线程
				pSessionImpl->newPrevDataThread();
			}
#endif
		}else
		{
			// 之前没有SESSION，直接返回XML错误
			CSotpResponseImpl responseImpl(pcgcRemote, pcgcParser, pSessionImpl,NULL);
			responseImpl.SetEncoding(m_parseDefault.getDefaultEncoding());
			responseImpl.sendAppCallResult(-112);
		}
	}

	if (bCanSetParserToPool)
		SetSotpParserPool(pcgcParser);
	return parseResult ? 0 : -1;
}

int CGCApp::ProcSesProto(const cgcSotpRequest::pointer& pRequestImpl, const cgcParserSotp::pointer& pcgcParser, const cgcRemote::pointer& pcgcRemote, cgcSession::pointer& remoteSessionImpl)
{
	if (!m_bInitedApp || m_bStopedApp || pcgcParser.get() == NULL) return -1;

	if (pcgcRemote->isInvalidate()) return -1;
	if (!pcgcParser->isSessionProto()) return -1;

	long retCode = 0;
	tstring sSessionId;
	unsigned long nCallId = pcgcParser->getCallid();

	CSessionImpl * pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
	// session
	if (pcgcParser->isOpenType())
	{
		// session open
		const tstring & sAppModuleName = pcgcParser->getModuleName();

		m_logModuleImpl.log(LOG_DEBUG, _T("SESSION OPENING..., APP_NAME=%s\n"), sAppModuleName.c_str());

		cgcApplication::pointer applicationImpl;
		ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(sAppModuleName);
		if (moduleItem.get() == NULL)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("no allow module \'%s\'!\n"), sAppModuleName.c_str());
			retCode = -101;
		}else if (moduleItem->getModuleHandle() == NULL)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("invalidate module handle \'%s\'!\n"), sAppModuleName.c_str());
			retCode = -102;
		}else if (!moduleItem->authAccount(pcgcParser->getAccount(), pcgcParser->getSecure()))
		{
			m_logModuleImpl.log(LOG_ERROR, _T("'%s' account auth failed!\n"), pcgcParser->getAccount().c_str());
			retCode = -104;
#ifdef USES_CMODULEMGR
		}else if (!m_pModuleMgr.m_mapModuleImpl.find(moduleItem->getModuleHandle(), applicationImpl) || !applicationImpl->isInited())
#else
		}else if (!m_mapOpenModules.find(moduleItem->getModuleHandle(), applicationImpl) || !applicationImpl->isInited())
#endif
		{
			retCode = -105;
		}else
		{
			cgcParserSotp::pointer pcgcParserLastSession;
			GetSotpParserPool(pcgcParserLastSession);
			if (pcgcParserLastSession.get() == NULL) return -1;
			//cgcServiceInterface::pointer parserService;
			//m_fpParserSotpService(parserService, cgcNullValueInfo);
			//if (parserService.get() == NULL) return -1;
			//cgcParserSotp::pointer pcgcParserLastSession = CGC_PARSERSOTPSERVICE_DEF(parserService);
			if (NULL == pRemoteSessionImpl)
			{
				remoteSessionImpl = m_mgrSession1.SetSessionImpl(moduleItem,pcgcRemote,pcgcParserLastSession);
				pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
			}

			//pRemoteSessionImpl->setAccontInfo(pcgcParser->getAccount(), pcgcParser->getSecure());
			sSessionId = remoteSessionImpl->getId();
			// call CGC_Session_Open
			if (pRemoteSessionImpl->OnRunCGC_Session_Open(moduleItem,pcgcRemote))
			{
				retCode = 200;
				m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\'.%s opened\n"), sSessionId.c_str(),moduleItem->getName().c_str());
				pRemoteSessionImpl->SetSslPublicKey(pcgcParser->getSslPublicKey());
			}else
			{
				// delete sessionimpl
				m_mgrSession1.RemoveSessionImpl(remoteSessionImpl,true);
				retCode = -105;
			}
		}
	}else if (pcgcParser->isCloseType())
	{
		// session close
		sSessionId = pcgcParser->getSid();
		if (NULL == remoteSessionImpl.get())
		{
			remoteSessionImpl = m_mgrSession1.GetSessionImpl(sSessionId);	// close session, 不需要记录，因为后面也会删除
			//remoteSessionImpl = m_mgrSession1.GetSessionImpl(sSessionId, pcgcRemote->getRemoteId());
			if (remoteSessionImpl.get()!=NULL)
			{
				//m_mgrSession1.SetRemoteSession(pcgcRemote->getRemoteId(),sSessionId);
				((CSessionImpl*)remoteSessionImpl.get())->setDataResponseImpl("",pcgcRemote);
			}
		}
		pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
		retCode = (pRemoteSessionImpl == NULL) ? -103 : 0;
	}else if (pcgcParser->isActiveType())
	{
		// active session
		sSessionId = pcgcParser->getSid();
		if (NULL == remoteSessionImpl.get())
		{
			remoteSessionImpl = m_mgrSession1.GetSessionImpl(sSessionId, pcgcRemote->getRemoteId());
			if (remoteSessionImpl.get()!=NULL)
			{
				//m_mgrSession1.SetRemoteSession(pcgcRemote->getRemoteId(),sSessionId);
				((CSessionImpl*)remoteSessionImpl.get())->setDataResponseImpl("",pcgcRemote);
				((CSessionImpl*)remoteSessionImpl.get())->OnRunCGC_Remote_Change(pcgcRemote);
			}
		}
		pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
		retCode = (pRemoteSessionImpl == NULL) ? -103 : 0;
	}else
	{
		m_logModuleImpl.log(LOG_WARNING, _T("invalidate cgc proto type \'%d\'!\n"), pcgcParser->getProtoType());
		retCode = -116;
	}

	CSotpResponseImpl responseImpl(pcgcRemote, pcgcParser, pRemoteSessionImpl,NULL);
	responseImpl.setSession(remoteSessionImpl);
	responseImpl.SetEncoding(m_parseDefault.getDefaultEncoding());
	// **暂时不需要返回public key
	//if (pRemoteSessionImpl!=NULL)
	//{
	//	responseImpl.SetSslPublicKey(m_pRsa.GetPublicKey());
	//}
	responseImpl.sendSessionResult(retCode, sSessionId);

	if (pcgcParser->isCloseType() && pRemoteSessionImpl != NULL)
	{
		m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' closed\n"), sSessionId.c_str());
		remoteSessionImpl->invalidate();
		m_mgrSession1.RemoveSessionImpl(remoteSessionImpl,true);
	}
	return 0;
}
int CGCApp::ProcSyncProto(const cgcSotpRequest::pointer& pRequestImpl, const cgcParserSotp::pointer& pcgcParser, const cgcRemote::pointer& pcgcRemote, cgcSession::pointer& remoteSessionImpl)
{
	if (!m_bInitedApp || m_bStopedApp || pcgcParser.get() == NULL) return -1;

	if (pcgcRemote->isInvalidate()) return -1;
	//if (!pcgcParser->isSessionProto()) return -1;

	//long retCode = 0;
	//tstring sSessionId;
	//unsigned long nCallId = pcgcParser->getCallid();

	CSessionImpl * pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
	// session
	if (pcgcParser->isResulted())
	{

	}else
	{
		//const tstring & sAppModuleName = pcgcParser->getModuleName();
		//m_logModuleImpl.log(LOG_DEBUG, _T("SESSION OPENING..., APP_NAME=%s\n"), sAppModuleName.c_str());

	}

	//CSotpResponseImpl responseImpl(pcgcRemote, pcgcParser, pRemoteSessionImpl,NULL);
	//responseImpl.setSession(remoteSessionImpl);
	//responseImpl.SetEncoding(m_parseDefault.getDefaultEncoding());
	//// **暂时不需要返回public key
	////if (pRemoteSessionImpl!=NULL)
	////{
	////	responseImpl.SetSslPublicKey(m_pRsa.GetPublicKey());
	////}
	//responseImpl.sendSessionResult(retCode, sSessionId);

	//if (pcgcParser->isCloseType() && pRemoteSessionImpl != NULL)
	//{
	//	m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' closed\n"), sSessionId.c_str());
	//	remoteSessionImpl->invalidate();
	//	m_mgrSession1.RemoveSessionImpl(remoteSessionImpl);
	//}
	return 0;
}
HTTP_STATUSCODE CGCApp::ProcHttpAppProto(const tstring& sAppModuleName,const cgcHttpRequest::pointer& requestImpl,const cgcHttpResponse::pointer& responseImpl,const cgcParserHttp::pointer& pcgcParser)
{
	assert (requestImpl.get() != NULL);
	assert (requestImpl->getSession() != NULL);
	assert (responseImpl.get() != NULL);
	assert (pcgcParser.get() != NULL);

	if (!m_bInitedApp || m_bStopedApp) return STATUS_CODE_503;
	//if (responseImpl->isInvalidate()) return STATUS_CODE_503;

	cgcSession::pointer remoteSessionImpl = requestImpl->getSession();
	CSessionImpl * pHttpRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
	
	HTTP_STATUSCODE statusCode = STATUS_CODE_200;
	const tstring & sCallName = pcgcParser->getFunctionName();
	tstring methodName(sCallName);
	tstring sSessionId(remoteSessionImpl->getId());

	//tstring sAppModuleName;
	//if (pcgcParser->isServletRequest())
	//	sAppModuleName = pcgcParser->getModuleName();
	//else
	//	sAppModuleName = m_sHttpServerName;
	ModuleItem::pointer pModuleItem = pHttpRemoteSessionImpl->getModuleItem(sAppModuleName,true);
	assert (pModuleItem.get() != NULL);
	if (!pModuleItem->getAllowMethod(sCallName, methodName))
	{
		m_logModuleImpl.log(LOG_ERROR, _T("no allow to call \'%s\'!\n"), sCallName.c_str());
		statusCode = STATUS_CODE_403;
	}else
	{
		if (pModuleItem->getType() == MODULE_SERVER && pModuleItem->getAllowAll() && !pModuleItem->getParam().empty())
			methodName = pModuleItem->getParam();

		//m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' call '%s.%s.%s'\n"), sSessionId.c_str(),requestImpl->getRestVersion().c_str(),pModuleItem->getName().c_str(), methodName.c_str());
		bool bDisableLogApi = false;

		///////////// app call /////////////////
		boost::mutex::scoped_lock * lockWait = NULL;
		CModuleImpl * pModuleImpl = NULL;
		cgcApplication::pointer applicationImpl;
#ifdef USES_CMODULEMGR
		if (!m_pModuleMgr.m_mapModuleImpl.find(pModuleItem->getModuleHandle(), applicationImpl) || !applicationImpl->isInited())
#else
		if (!m_mapOpenModules.find(pModuleItem->getModuleHandle(), applicationImpl) || !applicationImpl->isInited())
#endif
		{
			m_logModuleImpl.log(LOG_ERROR, _T("invalidate module handle \'%s\'!\n"), pModuleItem->getName().c_str());
			statusCode = STATUS_CODE_500;
		}else
		{
			pModuleImpl = (CModuleImpl*)applicationImpl.get();

			bDisableLogApi = pModuleImpl->m_moduleParams.isDisableLogApi(methodName);
			if (!bDisableLogApi)
				m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' call '%s.%s.%s'\n"), sSessionId.c_str(),requestImpl->getRestVersion().c_str(),pModuleItem->getName().c_str(), methodName.c_str());

/*			// lock state
			if (pModuleItem->getLockState() == ModuleItem::LS_WAIT)
			{
				// callref
				pModuleImpl->addCallRef();

				// lock
				lockWait = new boost::mutex::scoped_lock(pModuleImpl->m_mutexCallWait);
			}else if (pModuleItem->getLockState() == ModuleItem::LS_RETURN)
			{
				if (pModuleImpl->getCallRef() == 0)
				{
					// callref
					pModuleImpl->addCallRef();
				}else
				{
					m_logModuleImpl.log(LOG_ERROR, _T("module \'%s\' lock return = \'%d\'!\n"), pModuleItem->getName().c_str(), pModuleImpl->getCallRef());
					//statusCode = -107;
				}
			}else
			{
				// callref
				pModuleImpl->addCallRef();
			}
			*/
		}

		if (statusCode == STATUS_CODE_200)
		{
			try
			{
				statusCode = ProcHttpLibMethod(pModuleItem, methodName, requestImpl, responseImpl);
			}catch(std::exception const & e)
			{
				m_logModuleImpl.log(LOG_ERROR, _T("exception, sessionid=\'%s\', callname=\'%s\', lasterror=0x%x\n"), sSessionId.c_str(), methodName.c_str(), GetLastError());
				m_logModuleImpl.log(LOG_ERROR, _T("'%s'\n"), e.what());
				statusCode = STATUS_CODE_500;
			}catch(...)
			{
				m_logModuleImpl.log(LOG_ERROR, _T("exception, sessionid=\'%s\', callname=\'%s\', lasterror=0x%x\n"), sSessionId.c_str(), methodName.c_str(), GetLastError());
				statusCode = STATUS_CODE_500;
			}
			if (!bDisableLogApi)
				m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' returnCode '%d'\n"), sSessionId.c_str(), statusCode);

			// callref
			//if (pModuleImpl)
			//	pModuleImpl->releaseCallRef();
		}

		// unlock
		//if (lockWait)
		//	delete lockWait;
	}
	return statusCode;
}

int CGCApp::ProcAppProto(const cgcSotpRequest::pointer& requestImpl, const cgcSotpResponse::pointer& responseImpl, const cgcParserSotp::pointer& pcgcParser,cgcSession::pointer& remoteSessionImpl)
{
	if (!m_bInitedApp || m_bStopedApp) return -1;
	if (responseImpl->isInvalidate()) return -1;
	if (!pcgcParser->isAppProto() && pcgcParser->getProtoType()!=SOTP_PROTO_TYPE_SYNC) return -1;

	CSotpResponseImpl * pResponseImpl = (CSotpResponseImpl*)responseImpl.get();
	CSessionImpl * pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
	bool bOpenNewSession = false;
	long retCode = 0;
	if (pcgcParser->isCallType() || pcgcParser->getProtoType()==SOTP_PROTO_TYPE_SYNC)
	{
		const unsigned long nCallId = pcgcParser->getCallid();
		const unsigned long nSign = pcgcParser->getSign();

		const tstring & sCallName = pcgcParser->getFunctionName();
		tstring methodName(sCallName);
		tstring sSessionId(pcgcParser->getSid());
		if (pRemoteSessionImpl == NULL && !sSessionId.empty())
		{
			//printf("**** ProcAppProto Session NULL sid=%s\n",sSessionId.c_str());
			remoteSessionImpl = m_mgrSession1.GetSessionImpl(sSessionId, pResponseImpl->getCgcRemote()->getRemoteId());
			pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
			//printf("**** ProcAppProto Session NULL sid=%s,session=%d\n",sSessionId.c_str(),(int)pRemoteSessionImpl);
			if (pRemoteSessionImpl!=NULL)
			{
				const cgcRemote::pointer pcgcRemote = pResponseImpl->getCgcRemote();
				//m_mgrSession1.SetRemoteSession(pcgcRemote->getRemoteId(),sSessionId);
				pRemoteSessionImpl->setDataResponseImpl("",pcgcRemote);
				pRemoteSessionImpl->OnRunCGC_Remote_Change(pcgcRemote);
			}else
			{
				retCode = -103;
			}
		}
		if (retCode==0 && pRemoteSessionImpl == NULL)
		{
			unsigned long remoteId = 0;
			cgcRemote::pointer pcgcRemote = pResponseImpl->getCgcRemote();
			if (pcgcRemote != NULL)
			{
				remoteId = pcgcRemote->getRemoteId();
				bool bOpenSession = false;
				if (m_mapRemoteOpenSes.find(remoteId, bOpenSession, false))
				{
					// Wait for open the session
#ifdef WIN32
					Sleep(100);
#else
					usleep(100000);
#endif
				}else
				{
					m_mapRemoteOpenSes.insert(remoteId, true);
				}
				remoteSessionImpl = m_mgrSession1.GetSessionImplByRemote(remoteId);
				pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();
			}

			//
			// 客户端没有OPEN SESSION, sSessionId=APPNAME.
			// 临时打开SESSION
			if (retCode==0 && pRemoteSessionImpl == 0)
			{
				tstring sAppModuleName = pcgcParser->getModuleName();
				ModuleItem::pointer moduleItem = m_parseModules.getModuleItem(sAppModuleName);
				if (moduleItem.get() != 0) {
					
					if (!moduleItem->authAccount(pcgcParser->getAccount(), pcgcParser->getSecure())) {
						m_logModuleImpl.log(LOG_ERROR, _T("'%s' account auth failed!\n"), pcgcParser->getAccount().c_str());
						retCode = -104;
					}
					else {

						cgcParserSotp::pointer pcgcParserLastSession;
						GetSotpParserPool(pcgcParserLastSession);
						if (pcgcParserLastSession.get() == NULL) return -1;

						//cgcServiceInterface::pointer parserService;
						//m_fpParserSotpService(parserService, cgcNullValueInfo);
						//if (parserService.get() == NULL) return -1;
						//cgcParserSotp::pointer pcgcParserLastSession = CGC_PARSERSOTPSERVICE_DEF(parserService);
						remoteSessionImpl = m_mgrSession1.SetSessionImpl(moduleItem,pcgcRemote, pcgcParserLastSession);
						pRemoteSessionImpl = (CSessionImpl*)remoteSessionImpl.get();

						//pRemoteSessionImpl->setAccontInfo(pcgcParser->getAccount(), pcgcParser->getSecure());
						sSessionId = remoteSessionImpl->getId();
						// 调用模块 CGC_Session_Open
						if (pRemoteSessionImpl->OnRunCGC_Session_Open(moduleItem,pcgcRemote))
						{
							bOpenNewSession = true;
							pResponseImpl->SetResponseHandler(pRemoteSessionImpl);
							m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\'.%s opened\n"), sSessionId.c_str(),moduleItem->getName().c_str());
						}else
						{
							// 删除 sessionimpl
							m_mgrSession1.RemoveSessionImpl(remoteSessionImpl,true);
							pRemoteSessionImpl = NULL;
							retCode = -105;
						}
					}
				}
			}
			m_mapRemoteOpenSes.remove(remoteId);
		}

		ModuleItem::pointer pModuleItem;
		if (pRemoteSessionImpl!=NULL)
		{
			pModuleItem = pRemoteSessionImpl->getModuleItem("",true);	// ?GET DEFAULT
			//printf("**** ProcAppProto pModuleItem=%d\n",(int)pModuleItem.get());
		}

		if (pRemoteSessionImpl == NULL || pModuleItem.get() == NULL)
		{
			m_logModuleImpl.log(LOG_WARNING, _T("invalidate session handle \'%s\'!\n"), sSessionId.c_str());
			//m_logModuleImpl.log(LOG_ERROR, _T("invalidate session handle \'%s\'!\n"), sSessionId.c_str());
			retCode = -103;
		}else if (pcgcParser->getProtoType()!=SOTP_PROTO_TYPE_SYNC && !pModuleItem->getAllowMethod(sCallName, methodName))
		{
			m_logModuleImpl.log(LOG_ERROR, _T("no allow to call \'%s\'!\n"), sCallName.c_str());
			retCode = -106;
		}else
		{
			//m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' call '%s', cid '%d', sign=\"%d\"\n"), sSessionId.c_str(), methodName.c_str(), nCallId, nSign);

			bool bDisableLogSign = false;
			///////////// app call /////////////////
			int nApiLockSeconds = -1;
			boost::mutex::scoped_lock * lockWait = NULL;
			CModuleImpl * pModuleImpl = NULL;
			cgcApplication::pointer applicationImpl;
#ifdef USES_CMODULEMGR
			if (!m_pModuleMgr.m_mapModuleImpl.find(pModuleItem->getModuleHandle(), applicationImpl))
#else
			if (!m_mapOpenModules.find(pModuleItem->getModuleHandle(), applicationImpl))
#endif
			{
				m_logModuleImpl.log(LOG_ERROR, _T("invalidate module handle \'%s\'!\n"), pModuleItem->getName().c_str());
				retCode = -102;
			}else if (!applicationImpl->isInited())
			{
				retCode = -105;
			}else
			{
				pModuleImpl = (CModuleImpl*)applicationImpl.get();
				retCode = 0;

				// ???
				bDisableLogSign = pModuleImpl->m_moduleParams.isDisableLogSign((int)nSign);
				if (!bDisableLogSign)
					m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' call '%s', cid '%d', sign=\"%d\"\n"), sSessionId.c_str(), methodName.c_str(), nCallId, nSign);

				//if (pModuleImpl->m_moduleLocks.isLock(methodName,nApiLockSeconds) && nApiLockSeconds>=0)
				//{
				//	// nApiLockSeconds=-1; lock(?)
				//	// nApiLockSeconds>=0; lock return
				//	printf("********* lock api %s, seconds=%d\n",methodName.c_str(),nApiLockSeconds);
				//	const time_t tNow = time(0);
				//	time_t tLastLockTime = 0;
				//	pRemoteSessionImpl->m_tLockApiList.insert(methodName,tNow,false,&tLastLockTime);
				//	if (tLastLockTime>0)
				//	{
				//		if (nApiLockSeconds==0 || (tLastLockTime+nApiLockSeconds)>=tNow)
				//		{
				//			printf("********* lock api %s, seconds=%d, %d waitting...\n",methodName.c_str(),nApiLockSeconds,(int)(tNow-tLastLockTime));
				//			((CSotpRequestImpl*)requestImpl.get())->SetSessionApiLocked(true);
				//		}else
				//		{
				//			pRemoteSessionImpl->m_tLockApiList.insert(methodName,tNow,true);
				//		}
				//	}
				//}else
				if (pModuleItem->getLockState() == ModuleItem::LS_WAIT)
				{
					// callref
					pModuleImpl->addCallRef();

					// lock
					lockWait = new boost::mutex::scoped_lock(pModuleImpl->m_mutexCallWait);
				}else if (pModuleItem->getLockState() == ModuleItem::LS_RETURN)
				{
					if (pModuleImpl->getCallRef() == 0)
					{
						// callref
						pModuleImpl->addCallRef();
					}else
					{
						m_logModuleImpl.log(LOG_ERROR, _T("module \'%s\' lock return = \'%d\'!\n"), pModuleItem->getName().c_str(), pModuleImpl->getCallRef());
						retCode = -107;
					}
				}else
				{
					// callref
					pModuleImpl->addCallRef();
				}
			}

			if (retCode == 0)
			{
				if (bOpenNewSession)
					pResponseImpl->sendSessionResult(200, sSessionId);

				try
				{
					// ***
					if (pcgcParser->getProtoType()==SOTP_PROTO_TYPE_SYNC)
					{
						const int nExtData = requestImpl->getParameterValue(_T("ext"), (int)0);
						const int nSyncType = requestImpl->getParameterValue(_T("type"), (int)0);
						const tstring sSyncData(requestImpl->getParameterValue(_T("data"), ""));
						const tstring& sSyncName = methodName;
						const bool bDatasource = (nExtData&1)==1?true:false;
						//printf("**** SOTP_PROTO_TYPE_SYNC (%s)\n",sSyncData.c_str());
						if (bDatasource)	// * cdbc
						{
							const tstring& sDatasource = sSyncName;
							cgcCDBCService::pointer pCdbcService = getCDBDService(NULL,sDatasource);
							if (pCdbcService.get()==NULL)
							{
								retCode = -117;
							}else
							{
								const bigint ret = pCdbcService->execute(sSyncData.c_str());
								// ??? LOG
								//pCdbcService->
								printf("**** %lld,%s\n",ret,sSyncData.c_str());
								if (ret<0)
									retCode = 0;
								else if (ret==0)
									retCode = 1;
								else
									retCode = 2;
								//retCDBDService(pCdbcService);	// sync 不需要 retCDBDService
							}
						}else
						{
							retCode = ProcLibSyncMethod(pModuleItem, sSyncName, nSyncType, sSyncData);
						}
					}else
					{
						((CSotpRequestImpl*)requestImpl.get())->setSession(remoteSessionImpl);
						//((CSotpRequestImpl*)requestImpl.get())->SetApi(methodName);
						((CSotpResponseImpl*)responseImpl.get())->setSession(remoteSessionImpl);
						retCode = ProcLibMethod(pModuleItem, methodName, requestImpl, responseImpl);
					}
					if (!bDisableLogSign)
						m_logModuleImpl.log(LOG_INFO, _T("SID \'%s\' cid '%d', returnCode '%d'\n"), sSessionId.c_str(), nCallId, retCode);
				}catch(std::exception const & e)
				{
					m_logModuleImpl.log(LOG_ERROR, _T("exception, sessionid=\'%s\', callname=\'%s\', lasterror=0x%x\n"), sSessionId.c_str(), methodName.c_str(), GetLastError());
					m_logModuleImpl.log(LOG_ERROR, _T("'%s'\n"), e.what());
					retCode = -111;
				}catch(...)
				{
					m_logModuleImpl.log(LOG_ERROR, _T("exception, sessionid=\'%s\', callname=\'%s\', lasterror=0x%x\n"), sSessionId.c_str(), methodName.c_str(), GetLastError());
					retCode = -111;
				}

				// callref
				if (pModuleImpl)
					pModuleImpl->releaseCallRef();
			}

			//
			// unlock
			if (lockWait)
				delete lockWait;
			
			//if (((CSotpRequestImpl*)requestImpl.get())->GetIsInSessionApiLocked()==0)
			//{
			//	pRemoteSessionImpl->removeSessionApiLock(methodName);
			//}
			//if (retCode>=0)
			//{
			//	const int nInSessionApiLoekck = ((CSotpRequestImpl*)requestImpl.get())->GetIsInSessionApiLocked();
			//	if (nInSessionApiLoekck==0 ||
			//		(nInSessionApiLoekck>0 && retCode>=0))
			//	{
			//		pRemoteSessionImpl->removeSessionApiLock(methodName);
			//	}
			//}
			//if (pRemoteSessionImpl!=NULL && nApiLockSeconds>=0)
			//{
			//	if (!requestImpl->isInSessionApiLocked() ||					// 未锁定，正常调用
			//		(requestImpl->isInSessionApiLocked() && retCode>=0))	// 被锁定，正常调用
			//	{
			//		pRemoteSessionImpl->m_tLockApiList.remove(methodName);
			//	}
			//}
		}
	}else
	{
		m_logModuleImpl.log(LOG_WARNING, _T("invalidate cgc proto type \'%d\'!\n"), pcgcParser->getProtoType());
		retCode = -116;
	}
	if (!pResponseImpl->isResponseSended())
	{
		//responseImpl->lockResponse();	// *
		pResponseImpl->SetResponseHandler(pRemoteSessionImpl);
		pResponseImpl->sendAppCallResult(retCode);
	}
	return 0;
}

HTTP_STATUSCODE CGCApp::ProcHttpLibMethod(const ModuleItem::pointer& moduleItem,const tstring& sMethodName,const cgcHttpRequest::pointer& pRequest,const cgcHttpResponse::pointer& pResponse)
{
	assert (moduleItem.get() != NULL);
	void * hModule = moduleItem->getModuleHandle();
	if (hModule == NULL)
	{
		return STATUS_CODE_404;
	}
#ifdef WIN32
	FPCGCHttpApi farProc = (FPCGCHttpApi)GetProcAddress((HMODULE)hModule, sMethodName.c_str());
#else
	FPCGCHttpApi farProc = (FPCGCHttpApi)dlsym(hModule, sMethodName.c_str());
#endif
	return (farProc == NULL) ? STATUS_CODE_404 : farProc(pRequest, pResponse);
}

int CGCApp::ProcLibMethod(const ModuleItem::pointer& moduleItem, const tstring & sMethodName, const cgcSotpRequest::pointer& pRequest, const cgcSotpResponse::pointer& pResponse)
{
	assert (moduleItem.get() != NULL);

	int result(0);
	void * hModule = moduleItem->getModuleHandle();
	if (hModule == NULL)
	{
		result = -113;
		return result;
	}
#ifdef WIN32
	//std::string pOutTemp = cgcString::W2Char(sMethodName);
	FPCGCApi farProc = (FPCGCApi)GetProcAddress((HMODULE)hModule, sMethodName.c_str());
#else
	FPCGCApi farProc = (FPCGCApi)dlsym(hModule, sMethodName.c_str());
#endif
	if (farProc == NULL)
	{
		result = -114;
		return result;
	}

	result = farProc(pRequest, pResponse);

	return result;
}
int CGCApp::ProcLibSyncMethod(const ModuleItem::pointer& moduleItem, const tstring& sSyncName, int nSyncType, const tstring & sSyncData)
{
	assert (moduleItem.get() != NULL);

	int result(0);
	void * hModule = moduleItem->getModuleHandle();
	if (hModule == NULL)
	{
		result = -113;
		return result;
	}
#ifdef WIN32
	//std::string pOutTemp = cgcString::W2Char(sMethodName);
	FPCGC_SYNC farProc = (FPCGC_SYNC)GetProcAddress((HMODULE)hModule, "CGC_SYNC");
#else
	FPCGC_SYNC farProc = (FPCGC_SYNC)dlsym(hModule, "CGC_SYNC");
#endif
	if (farProc == NULL)
	{
		result = -114;
		return result;
	}

	result = farProc(sSyncName, nSyncType, sSyncData);
	return result;
}

void CGCApp::InitLibModules(unsigned int mt)
{
	//boost::mutex::scoped_lock lock(m_parseModules.m_modules.mutex());
	//CLockMap<void*, cgcApplication::pointer>::iterator iter;
	//for (iter=m_mapOpenModules.begin(); iter!=m_mapOpenModules.end(); iter++)
	//{
		//cgcApplication::pointer application = iter->second;
		//MODULETYPE moduleMT = application->getModuleType();
		//if ((mt & (int)moduleMT) != (int)moduleMT)
		//	continue;

	//CLockMap<tstring, ModuleItem::pointer,DisableCompare<tstring> >::iterator iter;
	//for (iter=m_parseModules.m_modules.begin(); iter!=m_parseModules.m_modules.end(); iter++)
	int nExceptionTryCount = 0;
	for (int i=0;i<(int)m_parseModules.m_modules.size();i++)
	{
		const ModuleItem::pointer& moduleItem = m_parseModules.m_modules[i];
		//ModuleItem::pointer moduleItem = iter->second;
		if (moduleItem->getDisable())
			continue;
		//else if ((moduleItem->getProtocol() & MODULE_PROTOCOL_WEB_PROXY)==MODULE_PROTOCOL_WEB_PROXY)
		//	continue;

		void * hModule = moduleItem->getModuleHandle();
		cgcApplication::pointer application;
#ifdef USES_CMODULEMGR
		if (!m_pModuleMgr.m_mapModuleImpl.find(hModule, application)) continue;
#else
		if (!m_mapOpenModules.find(hModule, application)) continue;
#endif
		const MODULETYPE moduleMT = application->getModuleType();
		if ((mt & (int)moduleMT) != (int)moduleMT)
		{
			continue;
		}

		try
		{
			if (InitLibModule(application, moduleItem))
			{
				CModuleImpl * pModuleImpl = (CModuleImpl*)application.get();
				pModuleImpl->SetInited(true);
				m_logModuleImpl.log(LOG_INFO, _T("MODULE \'%s\' load succeeded\n"), application->getApplicationName().c_str());
				nExceptionTryCount = 0;
			}else
			{
				m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' load failed\n"), application->getApplicationName().c_str());
				if (moduleMT==MODULE_COMM)
				{
					if ((++nExceptionTryCount)>5)
					{
#ifdef WIN32
						nExceptionTryCount = 0;	// ?
						continue;
#else
						exit(0);
						break;
#endif
					}
#ifdef WIN32
					Sleep(3000);
#else
					sleep(3);
#endif
					i-=1;	// retry again
					m_logModuleImpl.log(LOG_INFO, _T("MODULE \'%s\' retry again(%d)...\n"), application->getApplicationName().c_str(),nExceptionTryCount);
				}
			}
#ifdef USES_HDCID
			if (!m_bLicensed)
			{
				m_logModuleImpl.log(DL_WARNING, L"Unlicense account '%s', can only load one module.\n", m_sLicenseAccount.c_str());
				break;
			}
#endif
		}catch(const std::exception & e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' load exception.\n"), application->getApplicationName().c_str());
			m_logModuleImpl.log(LOG_ERROR, _T("%s\n"), e.what());
			if (moduleMT==MODULE_COMM)
			{
				if ((++nExceptionTryCount)>5)
				{
#ifdef WIN32
					nExceptionTryCount = 0;	// ?
					continue;
#else
					exit(0);
					break;
#endif
				}
#ifdef WIN32
				Sleep(3000);
#else
				sleep(3);
#endif
				i-=1;	// retry again
				m_logModuleImpl.log(LOG_INFO, _T("MODULE \'%s\' retry again(%d)...\n"), application->getApplicationName().c_str(),nExceptionTryCount);
			}
		}catch(...)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' load exception\n"), application->getApplicationName().c_str());
			if (moduleMT==MODULE_COMM)
			{
				if ((++nExceptionTryCount)>5)
				{
#ifdef WIN32
					nExceptionTryCount = 0;	// ?
					continue;
#else
					exit(0);
					break;
#endif
				}
#ifdef WIN32
				Sleep(3000);
#else
				sleep(3);
#endif
				i-=1;	// retry again
				m_logModuleImpl.log(LOG_INFO, _T("MODULE \'%s\' retry again(%d)...\n"), application->getApplicationName().c_str(),nExceptionTryCount);
			}
		}
	}
}

void CGCApp::SetModuleHandler(void* hModule,const CModuleImpl::pointer& pModuleImpl)
//void CGCApp::SetModuleHandler(void* hModule,const CModuleImpl::pointer& pModuleImpl,bool* pOutModuleUpdateKey)
{
	BOOST_ASSERT(hModule != NULL);
	BOOST_ASSERT(pModuleImpl.get() != NULL);
#ifdef USES_MODULE_SERVICE_MANAGER
	cgcApplication::pointer moduleImpl = boost::static_pointer_cast<cgcApplication, CModuleImpl>(pModuleImpl);
#else
	cgcApplication::pointer moduleImpl(pModuleImpl);
#endif

#ifdef USES_CMODULEMGR
	m_pModuleMgr.m_mapModuleImpl.insert(hModule, moduleImpl);
	//void* pOldModule = pModuleImpl->getModuleItem()->getModuleHandle();
	//if (pOldModule!=NULL && pOldModule!=hModule && m_pModuleMgr.m_mapModuleImpl.exist(pOldModule,moduleImpl,true))
	//{
	//	//m_pModuleMgr.m_mapModuleImpl.updatek(pOldModule, hModule);
	//	if (pOutModuleUpdateKey!=NULL)
	//		*pOutModuleUpdateKey = true;
	//}else
	//	m_pModuleMgr.m_mapModuleImpl.insert(hModule, moduleImpl);
#else
	m_mapOpenModules.insert(hModule, moduleImpl);
#endif

	{
		//
		// CGC_SetApplicationHandler
		FPCGC_SetApplicationHandler fp = 0;
#ifdef WIN32
		fp = (FPCGC_SetApplicationHandler)GetProcAddress((HMODULE)hModule, "CGC_SetApplicationHandler");
#else
		fp = (FPCGC_SetApplicationHandler)dlsym(hModule, "CGC_SetApplicationHandler");
#endif
		if (fp)
			fp(moduleImpl);
	}

	//
	// CGC_SetSystemHandler
	{
		FPCGC_SetSystemHandler fp = 0;
#ifdef WIN32
		fp = (FPCGC_SetSystemHandler)GetProcAddress((HMODULE)hModule, "CGC_SetSystemHandler");
#else
		fp = (FPCGC_SetSystemHandler)dlsym(hModule, "CGC_SetSystemHandler");
#endif
		if (fp)
			fp(shared_from_this());
	}

	// FPCGC_SetServiceManagerHandler
	//if (!pModuleImpl->getModuleItem()->isServiceModule())
	{
#ifdef WIN32
		FPCGC_SetServiceManagerHandler fp = (FPCGC_SetServiceManagerHandler)GetProcAddress((HMODULE)hModule, "CGC_SetServiceManagerHandler");
#else
		FPCGC_SetServiceManagerHandler fp = (FPCGC_SetServiceManagerHandler)dlsym(hModule, "CGC_SetServiceManagerHandler");
#endif
		if (fp)
		{
#ifdef USES_MODULE_SERVICE_MANAGER
			const cgcServiceManager::pointer pServiceManager = boost::static_pointer_cast<cgcServiceManager, CModuleImpl>(pModuleImpl);
			fp(pServiceManager);
#else
			fp(shared_from_this());
#endif
		}
	}
	//pModuleImpl->getModuleItem()->setModuleHandle(hModule);
}
bool CGCApp::OpenModuleLibrary(const ModuleItem::pointer& moduleItem)
//bool CGCApp::OpenModuleLibrary(const ModuleItem::pointer& moduleItem,const cgcApplication::pointer& pUpdateFromApplication)
{
	if (moduleItem->getModuleHandle()!=NULL) return true;
	std::string sTempFile;
	void * hModule = moduleItem->getModuleHandle();
	if (hModule == NULL)
	{
		tstring sModuleName(m_sModulePath);
		sModuleName.append(_T("/modules/"));
		sModuleName.append(moduleItem->getModule());
		if (!FileIsExist(sModuleName.c_str()))
		{
			// 组件文件不存在，检查自动更新组件文件
			const tstring sAutoUpdateModuleName = sModuleName+".autoupdate";
			if (FileIsExist(sAutoUpdateModuleName.c_str()))
			{
				// 自动更新组件存在，重命名组件文件
				boost::system::error_code ec;
				namespace fs = boost::filesystem;
				fs::path pathfrom(sAutoUpdateModuleName.string());
				fs::path pathto1(sModuleName.string());
				fs::rename(pathfrom,pathto1,ec);
			}
		}
#ifdef USES_TEMP_MODULE_FILE2
		if (!moduleItem->getName().empty() && moduleItem->getModule().find(moduleItem->getName())==std::string::npos)
		{
			// * 新建临时文件
			namespace fs = boost::filesystem;
			boosttpath pathFileFrom(sModuleName.c_str());
			char lpszBuffer[260];
			sprintf(lpszBuffer,"%s.%s",sModuleName.c_str(),moduleItem->getName().c_str());
			sTempFile = lpszBuffer;
			boosttpath pathFileTo(sTempFile);
			boost::system::error_code ec;
			boost::filesystem::copy_file(pathFileFrom,pathFileTo,fs::copy_option::overwrite_if_exists,ec);
			m_logModuleImpl.log(LOG_INFO, _T("Open temp file1: %s\n"), sTempFile.c_str());
			sModuleName = sTempFile;
		}
#endif
		//m_logModuleImpl.log(LOG_INFO, _T("OpenLibrarys:%s\n"), sModuleName.c_str());

#ifdef WIN32
		hModule = LoadLibrary(sModuleName.c_str());
#else
		hModule = dlopen(sModuleName.c_str(), RTLD_LAZY);
#endif
		if (hModule == NULL)
		{
			sModuleName = moduleItem->getModule();
#ifdef WIN32
			hModule = LoadLibrary(sModuleName.c_str());
			if (hModule == NULL)
			{
				m_logModuleImpl.log(LOG_ERROR, _T("Cannot open library: %s \'%d\'!\n"), sModuleName.c_str(), GetLastError());
				return false;
			}
#else
			hModule = dlopen(sModuleName.c_str(), RTLD_LAZY);
			if (hModule == NULL)
			{
				m_logModuleImpl.log(LOG_ERROR, _T("Cannot open library: \'%s\'!\n"), dlerror());
				return false;
			}
#endif
		}
#ifdef USES_CMODULEMGR
		if (m_pModuleMgr.m_mapModuleImpl.exist(hModule))
#else
		if (m_mapOpenModules.exist(hModule))
#endif
		{
			// **多次打开，新建临时文件
			namespace fs = boost::filesystem;
			boosttpath pathFileFrom(sModuleName.c_str());
			char lpszBuffer[255];
			sprintf(lpszBuffer,"%s.%s",sModuleName.c_str(),moduleItem->getName().c_str());
			sTempFile = lpszBuffer;
			boosttpath pathFileTo(sTempFile);
			boost::filesystem::copy_file(pathFileFrom,pathFileTo,fs::copy_option::overwrite_if_exists);
#ifdef WIN32
			hModule = LoadLibrary(sTempFile.c_str());
#else
			hModule = dlopen(sTempFile.c_str(), RTLD_LAZY);
#endif
			if (hModule == NULL)
			{
				m_logModuleImpl.log(LOG_ERROR, _T("Cannot open library: %s!\n"), sTempFile.c_str());
				boost::filesystem::remove(boosttpath(sTempFile));
				return false;
			}
			m_logModuleImpl.log(LOG_INFO, _T("Open temp file2: %s\n"), sTempFile.c_str());
		}
	}
#ifdef USES_CMODULEMGR
	if (m_pModuleMgr.m_mapModuleImpl.exist(hModule))
#else
	if (m_mapOpenModules.exist(hModule))
#endif
	{
		//printf("**** Open Module(0x%x) Exist: %s\n", (int)hModule, moduleItem->getName().c_str());
		moduleItem->setModuleHandle(hModule);
		return true;
	}
	//printf("**** Open Module(0x%x) -> %s\n", (int)hModule, moduleItem->getName().c_str());

	//		if (moduleItem->getProtocol()==MODULE_PROTOCOL_SOTP_CLIENT_SERVICE && m_fpGetSotpClientHandler==NULL)
	//		{
	//#ifdef WIN32
	//			FPCGC_GetSotpClientHandler fpGetSotpClientHandler = (FPCGC_GetSotpClientHandler)GetProcAddress((HMODULE)hModule, theGetSotpClientHandlerApiName.c_str());
	//#else
	//			FPCGC_GetSotpClientHandler fpGetSotpClientHandler = (FPCGC_GetSotpClientHandler)dlsym(hModule, theGetSotpClientHandlerApiName.c_str());
	//#endif
	//			if (fpGetSotpClientHandler!=NULL)
	//			{
	//				m_fpGetSotpClientHandler = fpGetSotpClientHandler;
	//				//mycp::cgcValueInfo::pointer pValueInfo = CGC_VALUEINFO(cgcValueInfo::TYPE_MAP);
	//				//pValueInfo->insertMap("abc",CGC_VALUEINFO(123));
	//				//mycp::DoSotpClientHandler::pointer pSotpClientHandler;
	//				//fpGetSotpClientHandler(pSotpClientHandler, pValueInfo);
	//			}
	//		}

#ifdef USES_MODULE_SERVICE_MANAGER
	CModuleImpl::pointer pModuleImpl = CModuleImpl::pointer(new CModuleImpl(moduleItem,this));
#else
	CModuleImpl * pModuleImpl = new CModuleImpl(moduleItem,this,this);
#endif
	//if (pUpdateFromApplication.get()!=NULL)
	//{
	//	pModuleImpl->setAttributes(pUpdateFromApplication->getAttributes());
	//}

	pModuleImpl->setModulePath(m_sModulePath);
	pModuleImpl->setDefaultThreadStackSize(m_parseDefault.getThreadStackSize());
	pModuleImpl->loadSyncData(false);
	pModuleImpl->m_sTempFile = sTempFile;
	SetModuleHandler(hModule, pModuleImpl);
//#ifdef USES_MODULE_SERVICE_MANAGER
//	cgcApplication::pointer moduleImpl = boost::static_pointer_cast<cgcApplication, CModuleImpl>(pModuleImpl);
//#else
//	cgcApplication::pointer moduleImpl(pModuleImpl);
//#endif
//#ifdef USES_CMODULEMGR
//	m_pModuleMgr.m_mapModuleImpl.insert(hModule, moduleImpl);
//#else
//	m_mapOpenModules.insert(hModule, moduleImpl);
//#endif
//
//	{
//		//
//		// CGC_SetApplicationHandler
//		FPCGC_SetApplicationHandler fp = 0;
//#ifdef WIN32
//		fp = (FPCGC_SetApplicationHandler)GetProcAddress((HMODULE)hModule, "CGC_SetApplicationHandler");
//#else
//		fp = (FPCGC_SetApplicationHandler)dlsym(hModule, "CGC_SetApplicationHandler");
//#endif
//		if (fp)
//			fp(moduleImpl);
//	}
//
//	//
//	// CGC_SetSystemHandler
//	{
//		FPCGC_SetSystemHandler fp = 0;
//#ifdef WIN32
//		fp = (FPCGC_SetSystemHandler)GetProcAddress((HMODULE)hModule, "CGC_SetSystemHandler");
//#else
//		fp = (FPCGC_SetSystemHandler)dlsym(hModule, "CGC_SetSystemHandler");
//#endif
//		if (fp)
//			fp(shared_from_this());
//	}
//
//	// FPCGC_SetServiceManagerHandler
//	//if (!pModuleImpl->getModuleItem()->isServiceModule())
//	{
//#ifdef WIN32
//		FPCGC_SetServiceManagerHandler fp = (FPCGC_SetServiceManagerHandler)GetProcAddress((HMODULE)hModule, "CGC_SetServiceManagerHandler");
//#else
//		FPCGC_SetServiceManagerHandler fp = (FPCGC_SetServiceManagerHandler)dlsym(hModule, "CGC_SetServiceManagerHandler");
//#endif
//		if (fp)
//		{
//#ifdef USES_MODULE_SERVICE_MANAGER
//			const cgcServiceManager::pointer pServiceManager = boost::static_pointer_cast<cgcServiceManager, CModuleImpl>(pModuleImpl);
//			fp(pServiceManager);
//#else
//			fp(shared_from_this());
//#endif
//		}
//	}
//
	moduleItem->setModuleHandle(hModule);
	return true;
}
void CGCApp::OpenLibrarys(void)
{
	//CLockMap<tstring, ModuleItem::pointer,DisableCompare<tstring> >::iterator iter;
	//for (iter=m_parseModules.m_modules.begin(); iter!=m_parseModules.m_modules.end(); iter++)
	for (size_t i=0;i<m_parseModules.m_modules.size(); i++)
	{
		const ModuleItem::pointer& moduleItem = m_parseModules.m_modules[i];
		//ModuleItem::pointer moduleItem = iter->second;
		if (moduleItem->getDisable())
			continue;
		else if (moduleItem->getModuleHandle()!=NULL)
			continue;
		//else if ((moduleItem->getProtocol() & MODULE_PROTOCOL_WEB_PROXY)==MODULE_PROTOCOL_WEB_PROXY)
		//	continue;

		OpenModuleLibrary(moduleItem);
//		std::string sTempFile;
//		void * hModule = moduleItem->getModuleHandle();
//		if (hModule == NULL)
//		{
//			tstring sModuleName(m_sModulePath);
//			sModuleName.append(_T("/modules/"));
//			sModuleName.append(moduleItem->getModule());
//#ifdef USES_TEMP_MODULE_FILE2
//			if (!moduleItem->getName().empty() && moduleItem->getModule().find(moduleItem->getName())==std::string::npos)
//			{
//				// * 新建临时文件
//				namespace fs = boost::filesystem;
//				boosttpath pathFileFrom(sModuleName.c_str());
//				char lpszBuffer[260];
//				sprintf(lpszBuffer,"%s.%s",sModuleName.c_str(),moduleItem->getName().c_str());
//				sTempFile = lpszBuffer;
//				boosttpath pathFileTo(sTempFile);
//				boost::filesystem::copy_file(pathFileFrom,pathFileTo,fs::copy_option::overwrite_if_exists);
//				m_logModuleImpl.log(LOG_INFO, _T("Open temp file1: %s\n"), sTempFile.c_str());
//				sModuleName = sTempFile;
//			}
//#endif
//			//m_logModuleImpl.log(LOG_INFO, _T("OpenLibrarys:%s\n"), sModuleName.c_str());
//
//#ifdef WIN32
//			hModule = LoadLibrary(sModuleName.c_str());
//#else
//			hModule = dlopen(sModuleName.c_str(), RTLD_LAZY);
//#endif
//			if (hModule == NULL)
//			{
//				sModuleName = moduleItem->getModule();
//#ifdef WIN32
//				hModule = LoadLibrary(sModuleName.c_str());
//				if (hModule == NULL)
//				{
//					m_logModuleImpl.log(LOG_ERROR, _T("Cannot open library: %s \'%d\'!\n"), sModuleName.c_str(), GetLastError());
//					continue;
//				}
//#else
//				hModule = dlopen(sModuleName.c_str(), RTLD_LAZY);
//				if (hModule == NULL)
//				{
//					m_logModuleImpl.log(LOG_ERROR, _T("Cannot open library: \'%s\'!\n"), dlerror());
//					continue;
//				}
//#endif
//			}
//#ifdef USES_CMODULEMGR
//			if (m_pModuleMgr.m_mapModuleImpl.exist(hModule))
//#else
//			if (m_mapOpenModules.exist(hModule))
//#endif
//			{
//				// **多次打开，新建临时文件
//				namespace fs = boost::filesystem;
//				boosttpath pathFileFrom(sModuleName.c_str());
//				char lpszBuffer[255];
//				sprintf(lpszBuffer,"%s.%s",sModuleName.c_str(),moduleItem->getName().c_str());
//				sTempFile = lpszBuffer;
//				boosttpath pathFileTo(sTempFile);
//				boost::filesystem::copy_file(pathFileFrom,pathFileTo,fs::copy_option::overwrite_if_exists);
//#ifdef WIN32
//				hModule = LoadLibrary(sTempFile.c_str());
//#else
//				hModule = dlopen(sTempFile.c_str(), RTLD_LAZY);
//#endif
//				if (hModule == NULL)
//				{
//					m_logModuleImpl.log(LOG_ERROR, _T("Cannot open library: %s!\n"), sTempFile.c_str());
//					boost::filesystem::remove(boosttpath(sTempFile));
//					continue;
//				}
//				m_logModuleImpl.log(LOG_INFO, _T("Open temp file2: %s\n"), sTempFile.c_str());
//			}
//		}
//#ifdef USES_CMODULEMGR
//		if (m_pModuleMgr.m_mapModuleImpl.exist(hModule))
//#else
//		if (m_mapOpenModules.exist(hModule))
//#endif
//		{
//			moduleItem->setModuleHandle(hModule);
//			continue;
//		}
////		if (moduleItem->getProtocol()==MODULE_PROTOCOL_SOTP_CLIENT_SERVICE && m_fpGetSotpClientHandler==NULL)
////		{
////#ifdef WIN32
////			FPCGC_GetSotpClientHandler fpGetSotpClientHandler = (FPCGC_GetSotpClientHandler)GetProcAddress((HMODULE)hModule, theGetSotpClientHandlerApiName.c_str());
////#else
////			FPCGC_GetSotpClientHandler fpGetSotpClientHandler = (FPCGC_GetSotpClientHandler)dlsym(hModule, theGetSotpClientHandlerApiName.c_str());
////#endif
////			if (fpGetSotpClientHandler!=NULL)
////			{
////				m_fpGetSotpClientHandler = fpGetSotpClientHandler;
////				//mycp::cgcValueInfo::pointer pValueInfo = CGC_VALUEINFO(cgcValueInfo::TYPE_MAP);
////				//pValueInfo->insertMap("abc",CGC_VALUEINFO(123));
////				//mycp::DoSotpClientHandler::pointer pSotpClientHandler;
////				//fpGetSotpClientHandler(pSotpClientHandler, pValueInfo);
////			}
////		}
//
//#ifdef USES_MODULE_SERVICE_MANAGER
//		CModuleImpl::pointer pModuleImpl = CModuleImpl::pointer(new CModuleImpl(moduleItem,this));
//#else
//		CModuleImpl * pModuleImpl = new CModuleImpl(moduleItem,this,this);
//#endif
//		pModuleImpl->setModulePath(m_sModulePath);
//		pModuleImpl->loadSyncData(false);
//		pModuleImpl->m_sTempFile = sTempFile;
//#ifdef USES_MODULE_SERVICE_MANAGER
//		cgcApplication::pointer moduleImpl = boost::static_pointer_cast<cgcApplication, CModuleImpl>(pModuleImpl);
//#else
//		cgcApplication::pointer moduleImpl(pModuleImpl);
//#endif
//#ifdef USES_CMODULEMGR
//		m_pModuleMgr.m_mapModuleImpl.insert(hModule, moduleImpl);
//#else
//		m_mapOpenModules.insert(hModule, moduleImpl);
//#endif
//
//		{
//			//
//			// CGC_SetApplicationHandler
//			FPCGC_SetApplicationHandler fp = 0;
//#ifdef WIN32
//			fp = (FPCGC_SetApplicationHandler)GetProcAddress((HMODULE)hModule, "CGC_SetApplicationHandler");
//#else
//			fp = (FPCGC_SetApplicationHandler)dlsym(hModule, "CGC_SetApplicationHandler");
//#endif
//			if (fp)
//				fp(moduleImpl);
//		}
//
//		//
//		// CGC_SetSystemHandler
//		{
//			FPCGC_SetSystemHandler fp = 0;
//#ifdef WIN32
//			fp = (FPCGC_SetSystemHandler)GetProcAddress((HMODULE)hModule, "CGC_SetSystemHandler");
//#else
//			fp = (FPCGC_SetSystemHandler)dlsym(hModule, "CGC_SetSystemHandler");
//#endif
//			if (fp)
//				fp(shared_from_this());
//		}
//
//		// FPCGC_SetServiceManagerHandler
//		//if (!pModuleImpl->getModuleItem()->isServiceModule())
//		{
//#ifdef WIN32
//			FPCGC_SetServiceManagerHandler fp = (FPCGC_SetServiceManagerHandler)GetProcAddress((HMODULE)hModule, "CGC_SetServiceManagerHandler");
//#else
//			FPCGC_SetServiceManagerHandler fp = (FPCGC_SetServiceManagerHandler)dlsym(hModule, "CGC_SetServiceManagerHandler");
//#endif
//			if (fp)
//			{
//#ifdef USES_MODULE_SERVICE_MANAGER
//				const cgcServiceManager::pointer pServiceManager = boost::static_pointer_cast<cgcServiceManager, CModuleImpl>(pModuleImpl);
//				fp(pServiceManager);
//#else
//				fp(shared_from_this());
//#endif
//			}
//		}
//
//		moduleItem->setModuleHandle(hModule);
	}
}

void CGCApp::FreeLibrarys(void)
{
	m_mapServiceModule.clear();

#ifdef USES_CMODULEMGR
	m_pModuleMgr.StopModule();
#else
	CLockMap<void*, cgcApplication::pointer>::iterator iterApp;
	for (iterApp=m_mapOpenModules.begin(); iterApp!=m_mapOpenModules.end(); iterApp++)
	{
		cgcApplication::pointer application = iterApp->second;
		CModuleImpl * pModuleImpl = (CModuleImpl*)application.get();
		pModuleImpl->StopModule();
	}
#endif

	//printf("**** Module Size = %d\n", m_parseModules.m_modules.size());
	//CLockMap<tstring, ModuleItem::pointer,DisableCompare<tstring> >::iterator iter;
	//for (iter=m_parseModules.m_modules.begin(); iter!=m_parseModules.m_modules.end(); iter++)
	//m_logModuleImpl.log(LOG_INFO, _T("Module Size = %d\n"), m_parseModules.m_modules.size());
	for (size_t i=0;i<m_parseModules.m_modules.size();i++)
	{
		const ModuleItem::pointer& moduleItem = m_parseModules.m_modules[i];
		//ModuleItem::pointer moduleItem = iter->second;
		if (moduleItem->getDisable())
			continue;
		//else if ((moduleItem->getProtocol() & MODULE_PROTOCOL_WEB_PROXY)==MODULE_PROTOCOL_WEB_PROXY)
		//	continue;

		void * hModule = moduleItem->getModuleHandle();
		printf("**** Free Module(0x%x) %02d -> %s\n", (int)hModule, i+1, moduleItem->getName().c_str());
		//m_logModuleImpl.log(LOG_INFO, _T("Free Module %02d : name=%s, module=0x%x\n"), i+1, moduleItem->getName().c_str(),hModule);
		if (hModule!=NULL)
		{
			try
			{
#ifdef WIN32
				FreeLibrary((HMODULE)hModule);
#else
				dlclose (hModule);
#endif
			}catch(std::exception const & e)
			{
				printf("**** name=%s, exception. 0x%x(%s)\n", moduleItem->getName().c_str(), GetLastError(), e.what());
			}catch(...){
				printf("**** name=%s, exception. 0x%x\n", moduleItem->getName().c_str(), GetLastError());
			}
			//printf("**** Free Module(0x%x) %02d ok\n", (int)hModule, i+1);
			moduleItem->setModuleHandle(NULL);
		}
		moduleItem->m_pApiProcAddressList.clear();
	}

	// ** delete temp file
#ifdef USES_CMODULEMGR
	CLockMap<void*, cgcApplication::pointer>::iterator iterApp;
	for (iterApp=m_pModuleMgr.m_mapModuleImpl.begin(); iterApp!=m_pModuleMgr.m_mapModuleImpl.end(); iterApp++)
#else
	for (iterApp=m_mapOpenModules.begin(); iterApp!=m_mapOpenModules.end(); iterApp++)
#endif
	{
		cgcApplication::pointer application = iterApp->second;
		CModuleImpl * pModuleImpl = (CModuleImpl*)application.get();
		if (!pModuleImpl->m_sTempFile.empty())
		{
			remove(pModuleImpl->m_sTempFile.c_str());
			//namespace fs = boost::filesystem;
			//boosttpath pathTempFile(pModuleImpl->m_sTempFile);
			//boost::filesystem::remove(pathTempFile);
			m_logModuleImpl.log(LOG_INFO, _T("Close temp file: %s\n"), pModuleImpl->m_sTempFile.c_str());
			pModuleImpl->m_sTempFile.clear();
		}
	}
#ifdef USES_CMODULEMGR
	m_pModuleMgr.m_mapModuleImpl.clear();
#else
	m_mapOpenModules.clear();
#endif
}

bool CGCApp::InitLibModule(void* hModule, const cgcApplication::pointer& moduleImpl, Module_Init_Type nInitType)
{
	BOOST_ASSERT(hModule != 0);
	BOOST_ASSERT(moduleImpl.get() != 0);

#ifdef WIN32
	cgcAttributes::pointer pOldAttributes = moduleImpl->getAttributes(false);
	if (pOldAttributes.get()!=NULL && nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
	{
		// *** cgcAttributes.reset()
		((CModuleImpl*)moduleImpl.get())->setAttributes(cgcNullAttributes);
	}
#endif
	// CGC_Module_Init
#ifdef WIN32
	FPCGC_Module_Init2 farProc_Init2 = (FPCGC_Module_Init2)GetProcAddress((HMODULE)hModule, "CGC_Module_Init2");
#else
	FPCGC_Module_Init2 farProc_Init2 = (FPCGC_Module_Init2)dlsym(hModule, "CGC_Module_Init2");
#endif
#ifdef WIN32
	FPCGC_Module_Init farProc_Init = farProc_Init2!=NULL?NULL:(FPCGC_Module_Init)GetProcAddress((HMODULE)hModule, "CGC_Module_Init");
#else
	FPCGC_Module_Init farProc_Init = farProc_Init2!=NULL?NULL:(FPCGC_Module_Init)dlsym(hModule, "CGC_Module_Init");
#endif
	if (farProc_Init2!=NULL || farProc_Init!=NULL)
	{
		bool ret = false;
		try
		{
			if (farProc_Init2!=NULL)
				ret = farProc_Init2(nInitType);
			else
				ret = farProc_Init();
			static bool theCanRetry = true;
			if (!ret && moduleImpl->getModuleType()==MODULE_APP && theCanRetry)
			{
				theCanRetry = false;
				// CGC_Module_Free
#ifdef WIN32
				FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)GetProcAddress((HMODULE)hModule, "CGC_Module_Free2");
				FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)GetProcAddress((HMODULE)hModule, "CGC_Module_Free");
#else
				FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)dlsym(hModule, "CGC_Module_Free2");
				FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)dlsym(hModule, "CGC_Module_Free");
#endif
				const int nMaxTryCount = (nInitType!=MODULE_INIT_TYPE_NORMAL)?3:(m_parseDefault.getRetryCount()==0?0x7fffffff:m_parseDefault.getRetryCount());
				//printf("**** RetryCount=%d,%d\n",nMaxTryCount,m_parseDefault.getRetryCount());
				//const int nMaxTryCount = m_parseDefault.getRetryCount()>10?10:m_parseDefault.getRetryCount();
				for (int i=0;i<nMaxTryCount;i++)
				{
					m_logModuleImpl.log(LOG_INFO, _T("CGC_Module_Init '%s' retry %d...\n"), moduleImpl->getApplicationName().c_str(),i+1);
					if (farProc_Free2!=NULL)
						farProc_Free2(nInitType==MODULE_INIT_TYPE_NORMAL?MODULE_FREE_TYPE_NORMAL:MODULE_FREE_TYPE_AUTO_UPDATE);
					else if (farProc_Free!=NULL)
						farProc_Free();
#ifdef WIN32
					Sleep(3000);
#else
					sleep(3);
#endif
					if (farProc_Init2!=NULL)
						ret = farProc_Init2(nInitType);
					else
						ret = farProc_Init();
					if (ret || m_bStopedApp)
					{
						break;
					}
				}
			}
		}catch(std::exception const & e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("%s, 0x%x\n"), e.what(), GetLastError());
		}catch(...){
			m_logModuleImpl.log(LOG_ERROR, _T("0x%x\n"), GetLastError());
		}
		if (!ret)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("CGC_Module_Init '%s' load failed\n"), moduleImpl->getApplicationName().c_str());
			return false;
		}
	}

//#ifdef WIN32
//	if (pOldAttributes.get()!=NULL && nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
//	{
//		// 更换
//		CModuleImpl * pModuleImpl = (CModuleImpl*)moduleImpl.get();
//		printf("**** new AttributesImpl()...\n");
//		FPCGC_Module_AutoUpdateData farProc_AutoUpdateData = (FPCGC_Module_AutoUpdateData)GetProcAddress((HMODULE)hModule, "CGC_Module_AutoUpdateData");
//		if (farProc_AutoUpdateData!=NULL)
//			farProc_AutoUpdateData(pOldAttributes);
//
//		////pModuleImpl->setAttributes(((AttributesImpl*)pAttributes.get())->copyNew());
//		//AttributesImpl * pNewAttribute = new AttributesImpl();
//		//pNewAttribute->operator = ((AttributesImpl*)pAttributes.get());
//		//pModuleImpl->setAttributes(cgcAttributes::pointer(pNewAttribute));
//	}
//#endif
	return true;
}
bool CGCApp::InitLibModule(const cgcApplication::pointer& moduleImpl, const ModuleItem::pointer& moduleItem, Module_Init_Type nInitType)
{
	BOOST_ASSERT(moduleImpl.get() != 0);
	BOOST_ASSERT(moduleItem.get() != 0);

	CModuleImpl * pModuleImpl = (CModuleImpl*)moduleImpl.get();
	//ModuleItem::pointer moduleItem = pModuleImpl->getModuleItem();

	void * hModule = moduleItem->getModuleHandle();
	if (hModule == NULL) return false;

	if (pModuleImpl->getModuleState() == 0)
	{
		pModuleImpl->setModuleState(1);

		// Set log service.
		if (m_fpGetLogService != NULL)
		{
			cgcValueInfo::pointer parameter = CGC_VALUEINFO(pModuleImpl->getModuleItem()->getName());
			cgcServiceInterface::pointer logService;
			m_fpGetLogService(logService, parameter);
			if (logService.get() != NULL)
			{
				logService->initService();
				moduleImpl->logService(CGC_LOGSERVICE_DEF(logService));
			}
		}

		// load allow method
		//	if (!moduleItem->getAllowAll())
		{
			tstring xmlFile(moduleImpl->getAppConfPath());
			xmlFile.append(_T("/methods.xml"));

			namespace fs = boost::filesystem;
			boosttpath pathXmlFile(xmlFile.c_str());
			boost::system::error_code ec;
			if (boost::filesystem::exists(pathXmlFile,ec))
			{
				XmlParseAllowMethods parseAllowMethods;
				parseAllowMethods.load(xmlFile);
				{
					moduleItem->m_mapAllowMethods = parseAllowMethods.m_mapAllowMethods;
					parseAllowMethods.m_mapAllowMethods.clear();
				}
			}
		}

		//
		// load auths
		//
		if (moduleItem->getAuthAccount())
		{
			tstring xmlFile(moduleImpl->getAppConfPath());
			xmlFile.append(_T("/auths.xml"));

			namespace fs = boost::filesystem;
			boosttpath pathXmlFile(xmlFile.c_str());
			boost::system::error_code ec;
			if (fs::exists(pathXmlFile,ec))
			{
				XmlParseAuths parseAuths;
				parseAuths.load(xmlFile);
				{
					moduleItem->m_mapAuths = parseAuths.m_mapAuths;
					parseAuths.m_mapAuths.clear();
				}
			}

			if (moduleItem->m_mapAuths.empty())
				m_logModuleImpl.log(LOG_WARNING, _T("'%s' authAccount, please setting '%s'\n"), moduleItem->getModule().c_str(), xmlFile.c_str());
		}

		//// lock apis
		//{
		//	tstring xmlFile(moduleImpl->getAppConfPath());
		//	xmlFile.append(_T("/session-api-locks.xml"));

		//	namespace fs = boost::filesystem;
		//	boosttpath pathXmlFile(xmlFile);
		//	boost::system::error_code ec;
		//	if (fs::exists(pathXmlFile,ec))
		//	{
		//		pModuleImpl->m_moduleLocks.emptyLocks();
		//		pModuleImpl->m_moduleLocks.load(xmlFile);
		//	}
		//}

		// init parameter
		tstring xmlFile(moduleImpl->getAppConfPath());
		xmlFile.append(_T("/params.xml"));

		namespace fs = boost::filesystem;
		boosttpath pathXmlFile(xmlFile.c_str());
		boost::system::error_code ec;
		if (fs::exists(pathXmlFile,ec))
		{
			pModuleImpl->m_moduleParams.clearParameters();
			pModuleImpl->m_moduleParams.load(xmlFile);
		}

		if (!InitLibModule(hModule, moduleImpl, nInitType))
			return false;
//		// CGC_Module_Init
//#ifdef WIN32
//		FPCGC_Module_Init2 farProc_Init2 = (FPCGC_Module_Init2)GetProcAddress((HMODULE)hModule, "CGC_Module_Init2");
//#else
//		FPCGC_Module_Init2 farProc_Init2 = (FPCGC_Module_Init2)dlsym(hModule, "CGC_Module_Init2");
//#endif
//#ifdef WIN32
//		FPCGC_Module_Init farProc_Init = farProc_Init2!=NULL?NULL:(FPCGC_Module_Init)GetProcAddress((HMODULE)hModule, "CGC_Module_Init");
//#else
//		FPCGC_Module_Init farProc_Init = farProc_Init2!=NULL?NULL:(FPCGC_Module_Init)dlsym(hModule, "CGC_Module_Init");
//#endif
//		if (farProc_Init2!=NULL || farProc_Init!=NULL)
//		{
//			bool ret = false;
//			try
//			{
//				if (farProc_Init2!=NULL)
//					ret = farProc_Init2(nInitType);
//				else
//					ret = farProc_Init();
//				static bool theCanRetry = true;
//				if (!ret && moduleImpl->getModuleType()==MODULE_APP && theCanRetry)
//				{
//					theCanRetry = false;
//					// CGC_Module_Free
//#ifdef WIN32
//					FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)GetProcAddress((HMODULE)hModule, "CGC_Module_Free2");
//					FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)GetProcAddress((HMODULE)hModule, "CGC_Module_Free");
//#else
//					FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)dlsym(hModule, "CGC_Module_Free2");
//					FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)dlsym(hModule, "CGC_Module_Free");
//#endif
//					const int nMaxTryCount = m_parseDefault.getRetryCount()==0?0x7fffffff:m_parseDefault.getRetryCount();
//					//printf("**** RetryCount=%d,%d\n",nMaxTryCount,m_parseDefault.getRetryCount());
//					//const int nMaxTryCount = m_parseDefault.getRetryCount()>10?10:m_parseDefault.getRetryCount();
//					for (int i=0;i<nMaxTryCount;i++)
//					{
//						m_logModuleImpl.log(LOG_INFO, _T("CGC_Module_Init '%s' retry %d...\n"), moduleItem->getName().c_str(),i+1);
//						if (farProc_Free2!=NULL)
//							farProc_Free2(MODULE_FREE_TYPE_NORMAL);
//						else if (farProc_Free!=NULL)
//							farProc_Free();
//#ifdef WIN32
//						Sleep(3000);
//#else
//						sleep(3);
//#endif
//						if (farProc_Init2!=NULL)
//							ret = farProc_Init2(nInitType);
//						else
//							ret = farProc_Init();
//						if (ret || m_bStopedApp)
//						{
//							break;
//						}
//					}
//				}
//			}catch(std::exception const & e)
//			{
//				m_logModuleImpl.log(LOG_ERROR, _T("%s, 0x%x\n"), e.what(), GetLastError());
//			}catch(...){
//				m_logModuleImpl.log(LOG_ERROR, _T("0x%x\n"), GetLastError());
//			}
//			if (!ret)
//			{
//				m_logModuleImpl.log(LOG_ERROR, _T("CGC_Module_Init '%s' load failed\n"), moduleItem->getName().c_str());
//				return false;
//			}
//		}
	}

#ifdef WIN32
	FPCGC_GetService fpGetService = (FPCGC_GetService)GetProcAddress((HMODULE)hModule, "CGC_GetService");
	FPCGC_ResetService fpResetService = (FPCGC_ResetService)GetProcAddress((HMODULE)hModule, "CGC_ResetService");
#else
	FPCGC_GetService fpGetService = (FPCGC_GetService)dlsym(hModule, "CGC_GetService");
	FPCGC_ResetService fpResetService = (FPCGC_ResetService)dlsym(hModule, "CGC_ResetService");
#endif
	moduleItem->setFpGetService((void*)fpGetService);
	moduleItem->setFpResetService((void*)fpResetService);

	if (moduleItem->getProtocol()==MODULE_PROTOCOL_SOTP_CLIENT_SERVICE && (m_fpGetSotpClientHandler==NULL || nInitType==MODULE_INIT_TYPE_AUTO_UPDATE))
	{
#ifdef WIN32
		m_fpGetSotpClientHandler = (FPCGC_GetSotpClientHandler)GetProcAddress((HMODULE)hModule, theGetSotpClientHandlerApiName.c_str());
#else
		m_fpGetSotpClientHandler = (FPCGC_GetSotpClientHandler)dlsym(hModule, theGetSotpClientHandlerApiName.c_str());
#endif
		////printf("**** m_fpGetSotpClientHandler=0x%x\n",(int)(void*)fpGetSotpClientHandler);
		//if (fpGetSotpClientHandler!=NULL)
		//{
		//	m_fpGetSotpClientHandler = fpGetSotpClientHandler;
		//	//moduleItem->m_pApiProcAddressList.insert(theGetSotpClientHandlerApiName,(void*)fpGetSotpClientHandler);
		//}
	}

	switch (moduleItem->getType())
	{
	case MODULE_LOG:
		{
			if (m_fpGetLogService == NULL || nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
			{
				m_fpGetLogService = (FPCGC_GetService)moduleItem->getFpGetService();
				if (m_fpGetLogService != NULL)
				{
					// LogService owner
					cgcServiceInterface::pointer logService;
					m_fpGetLogService(logService, CGC_VALUEINFO(pModuleImpl->getModuleItem()->getName()));
					moduleImpl->logService(CGC_LOGSERVICE_DEF(logService));

					// MYCP logservice
					logService.reset();
					m_fpGetLogService(logService, CGC_VALUEINFO(m_parseDefault.getCgcpName()));
					m_logModuleImpl.logService(CGC_LOGSERVICE_DEF(logService));
				}
			}

			if (m_fpResetLogService == NULL || nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
			{
				m_fpResetLogService = (FPCGC_ResetService)moduleItem->getFpResetService();
			}
		}break;
	case MODULE_PARSER:
		{
			if (moduleItem->getProtocol() & PROTOCOL_HTTP)
			{
				// HTTP
				if (m_fpParserHttpService == NULL || nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
				{
					m_fpParserHttpService = (FPCGC_GetService)moduleItem->getFpGetService();
				}
			}else if (moduleItem->getProtocol() == PROTOCOL_SOTP)
			{
				// SOTP
				if (m_fpParserSotpService == NULL || nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
				{
					m_fpParserSotpService = (FPCGC_GetService)moduleItem->getFpGetService();
				}
			}else
			{
				// ??
			}
		}break;
	case MODULE_COMM:
		{
			if (nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
				return false;	// ???COMM 通讯组件暂时不支持自动更新
			if (moduleItem->getCommPort() > 0)
			{
#ifdef USES_MODULE_SERVICE_MANAGER
				cgcServiceInterface::pointer serviceInterface = getService(NULL,moduleImpl->getApplicationName());
#else
				cgcServiceInterface::pointer serviceInterface = getService(moduleImpl->getApplicationName());
#endif
				if (serviceInterface.get() != NULL)
				{
					int nCapacity = atoi(moduleItem->getParam().c_str());
					if (nCapacity < 1)
						nCapacity = 1;
					else
						nCapacity = nCapacity > 100 ? 100 : nCapacity;

					cgcCommunication::pointer commService = CGC_COMMSERVICE_DEF(serviceInterface);
					cgcCommHandler::pointer commHandler = shared_from_this();
					commService->setCommHandler(commHandler);

					//printf("port = %d\n",moduleItem->getCommPort());
					std::vector<cgcValueInfo::pointer> list;
					list.push_back(CGC_VALUEINFO(moduleItem->getCommPort()));
					list.push_back(CGC_VALUEINFO(nCapacity));
					list.push_back(CGC_VALUEINFO(moduleItem->getProtocol()));
					if (!commService->initService(CGC_VALUEINFO(list)))
					{
						if ((moduleItem->getProtocol()&PROTOCOL_HTTP)==0 || !commService->initService(CGC_VALUEINFO(list)))	// ** 多尝试一次，部分TCP端口会报 bind: Address already in use 错误
						{
#ifdef USES_MODULE_SERVICE_MANAGER
							this->resetService(NULL,serviceInterface);
#else
							this->resetService(serviceInterface);
#endif
							m_logModuleImpl.log(LOG_ERROR, _T("InitLibModule '%s' initService failed\n"), moduleItem->getName().c_str());
							return false;
						}
					}
				}
			}
		}break;
	case MODULE_SERVER:
		{
			if (moduleItem->getProtocol() & PROTOCOL_HTTP)
			{
				tstring functionName = moduleItem->getParam();
				if (functionName.empty())
					functionName = "doHttpServer";

				if (m_fpHttpServer == NULL || nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
				{
					m_sHttpServerName = moduleItem->getName();
#ifdef WIN32
					m_fpHttpServer = (FPCGCHttpApi)GetProcAddress((HMODULE)hModule, functionName.c_str());
#else
					m_fpHttpServer = (FPCGCHttpApi)dlsym(hModule, functionName.c_str());
#endif
				}
			}
		}break;
	case MODULE_APP:
		{
#ifdef WIN32
			FPCGC_Session_Open fpSessionOpen = (FPCGC_Session_Open)GetProcAddress((HMODULE)hModule, "CGC_Session_Open");
			FPCGC_Session_Close fpSessionClose = (FPCGC_Session_Close)GetProcAddress((HMODULE)hModule, "CGC_Session_Close");
			FPCGC_Remote_Change fpRemoteChange = (FPCGC_Remote_Change)GetProcAddress((HMODULE)hModule, "CGC_Remote_Change");
			FPCGC_Remote_Close fpRemoteClose = (FPCGC_Remote_Close)GetProcAddress((HMODULE)hModule, "CGC_Remote_Close");
#else
			FPCGC_Session_Open fpSessionOpen = (FPCGC_Session_Open)dlsym(hModule, "CGC_Session_Open");
			FPCGC_Session_Close fpSessionClose = (FPCGC_Session_Close)dlsym(hModule, "CGC_Session_Close");
			FPCGC_Remote_Change fpRemoteChange = (FPCGC_Remote_Change)dlsym(hModule, "CGC_Remote_Change");
			FPCGC_Remote_Close fpRemoteClose = (FPCGC_Remote_Close)dlsym(hModule, "CGC_Remote_Close");
#endif
			moduleItem->setFpSessionOpen((void*)fpSessionOpen);
			moduleItem->setFpSessionClose((void*)fpSessionClose);
			moduleItem->setFpRemoteChange((void*)fpRemoteChange);
			moduleItem->setFpRemoteClose((void*)fpRemoteClose);
		}break;
	default:
		break;
	}
//#ifdef WIN32
//	if (nInitType==MODULE_INIT_TYPE_AUTO_UPDATE)
//	{
//		// 更换
//		cgcAttributes::pointer pAttributes = moduleImpl->getAttributes(false);
//		if (pAttributes.get()!=NULL)
//		{
//			printf("**** new AttributesImpl()...\n");
//			pModuleImpl->setAttributes(((AttributesImpl*)pAttributes.get())->copyNew());
//			//AttributesImpl * pNewAttribute = new AttributesImpl();
//			//pNewAttribute->operator = ((AttributesImpl*)pAttributes.get());
//			//pModuleImpl->setAttributes(cgcAttributes::pointer(pNewAttribute));
//		}
//	}
//#endif
	return true;
}

void CGCApp::FreeLibModules(unsigned int mt)
{
	//boost::mutex::scoped_lock lock(m_parseModules.m_modules.mutex());

	// 反向遍历
	CLockMap<void*, cgcApplication::pointer>::reverse_iterator iter;
#ifdef USES_CMODULEMGR
	for (iter=m_pModuleMgr.m_mapModuleImpl.rbegin(); iter!=m_pModuleMgr.m_mapModuleImpl.rend(); iter++)
#else
	for (iter=m_mapOpenModules.rbegin(); iter!=m_mapOpenModules.rend(); iter++)
#endif
	{
		const cgcApplication::pointer& application = iter->second;
		MODULETYPE moduleMT = application->getModuleType();
		if ((mt & (int)moduleMT) != (int)moduleMT)
			continue;

		theFreeModuleTime = time(0);
		try
		{
			FreeLibModule(application,MODULE_FREE_TYPE_NORMAL);
			m_logModuleImpl.log(LOG_INFO, _T("MODULE \'%s\' free succeeded\n"), application->getApplicationName().c_str());
		}catch(const std::exception & e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' free exception.\n"), application->getApplicationName().c_str());
			m_logModuleImpl.log(LOG_ERROR, _T("%s\n"), e.what());
		}catch(...)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("MODULE \'%s\' free exception\n"), application->getApplicationName().c_str());
		}
		theFreeModuleTime = 0;
	}
}

void CGCApp::FreeLibModule(void* hModule, Module_Free_Type nFreeType)
{
	BOOST_ASSERT (hModule != NULL);
	// CGC_Module_Free
#ifdef WIN32
	FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)GetProcAddress((HMODULE)hModule, "CGC_Module_Free2");
	FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)GetProcAddress((HMODULE)hModule, "CGC_Module_Free");
#else
	FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)dlsym(hModule, "CGC_Module_Free2");
	FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)dlsym(hModule, "CGC_Module_Free");
#endif
	//if (farProc_Free2!=NULL || farProc_Free!=NULL)
	{
		//m_logModuleImpl.log(LOG_DEBUG, _T("CGC_Module_Free '%s'\n"), moduleItem->getAppName().c_str());
		try{
			if (farProc_Free2!=NULL)
				farProc_Free2(nFreeType);
			else if (farProc_Free!=NULL)
				farProc_Free();
		}catch(std::exception const & e)
		{
			m_logModuleImpl.log(LOG_ERROR, _T("%s, 0x%x\n"), e.what(), GetLastError());
		}catch(...){
			m_logModuleImpl.log(LOG_ERROR, _T("0x%x\n"), GetLastError());
		}
	}
}
void CGCApp::FreeLibModule(const cgcApplication::pointer& moduleImpl, Module_Free_Type nFreeType)
{
	if (moduleImpl.get() == NULL) return;

	CModuleImpl * pModuleImpl = (CModuleImpl*)moduleImpl.get();
	ModuleItem::pointer moduleItem = pModuleImpl->getModuleItem();
	if (moduleItem.get() == 0) return;

	void * hModule = moduleItem->getModuleHandle();
	if (hModule != NULL)
	{
		std::vector<cgcServiceInterface::pointer> pResetServiceList;
		CLockMap<cgcServiceInterface::pointer, void*>::iterator iterService = m_mapServiceModule.begin();
		for (; iterService!=m_mapServiceModule.end(); iterService++)
		{
			if (iterService->second == hModule)
			{
				pResetServiceList.push_back(iterService->first);
			}
		}
		for (size_t i=0; i<pResetServiceList.size(); i++)
		{
#ifdef USES_MODULE_SERVICE_MANAGER
			this->resetService(NULL,pResetServiceList[i]);
#else
			this->resetService(pResetServiceList[i]);
#endif
		}
		pResetServiceList.clear();

//		CLockMap<cgcServiceInterface::pointer, void*>::iterator iterService = m_mapServiceModule.begin();
//		for (; iterService!=m_mapServiceModule.end(); )
//		{
//			if (iterService->second == hModule)
//			{
//#ifdef USES_MODULE_SERVICE_MANAGER
//				this->resetService(NULL,iterService->first);
//#else
//				this->resetService(iterService->first);
//#endif
//				if (m_mapServiceModule.empty())
//					break;
//				iterService=m_mapServiceModule.begin();
//			}else
//			{
//				iterService++;
//			}
//		}

		if (moduleImpl->getModuleType() == MODULE_LOG)
		{
			if (m_fpResetLogService != NULL && moduleImpl->logService().get() != NULL)
			{
				m_fpResetLogService(moduleImpl->logService());
				moduleImpl->logService(cgcNullLogService);
			}
		}

		if (pModuleImpl->getModuleState() != -1)
		{
			pModuleImpl->setModuleState(-1);
			pModuleImpl->SetInited(false);
			
			FreeLibModule(hModule, nFreeType);
//			// CGC_Module_Free
//#ifdef WIN32
//			FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)GetProcAddress((HMODULE)hModule, "CGC_Module_Free2");
//			FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)GetProcAddress((HMODULE)hModule, "CGC_Module_Free");
//#else
//			FPCGC_Module_Free2 farProc_Free2 = (FPCGC_Module_Free2)dlsym(hModule, "CGC_Module_Free2");
//			FPCGC_Module_Free farProc_Free = farProc_Free2!=NULL?NULL:(FPCGC_Module_Free)dlsym(hModule, "CGC_Module_Free");
//#endif
//			//if (farProc_Free2!=NULL || farProc_Free!=NULL)
//			{
//				//m_logModuleImpl.log(LOG_DEBUG, _T("CGC_Module_Free '%s'\n"), moduleItem->getAppName().c_str());
//				try{
//					if (farProc_Free2!=NULL)
//						farProc_Free2(nFreeType);
//					else if (farProc_Free!=NULL)
//						farProc_Free();
//				}catch(std::exception const & e)
//				{
//					m_logModuleImpl.log(LOG_ERROR, _T("%s, 0x%x\n"), e.what(), GetLastError());
//				}catch(...){
//					m_logModuleImpl.log(LOG_ERROR, _T("0x%x\n"), GetLastError());
//				}
//			}
		}
	}
	if (m_fpResetLogService != NULL && moduleImpl->logService().get() != NULL)
		m_fpResetLogService(moduleImpl->logService());

	if (nFreeType==MODULE_FREE_TYPE_NORMAL)
	{
		moduleImpl->KillAllTimer();
		cgcAttributes::pointer attributes = moduleImpl->getAttributes();
		if (attributes.get() != NULL)
		{
			attributes->clearAllAtrributes();
			attributes->cleanAllPropertys();
		}
	}
}

} // namespace mycp
