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

// SessionMgr.h file here
#ifndef _SESSIONMGR_HEAD_INCLUDED__
#define _SESSIONMGR_HEAD_INCLUDED__

#include "IncludeBase.h"
#include <CGCClass/AttributesImpl.h>
//#include "AttributesImpl.h"
#include "SotpResponseImpl.h"
#include "HttpResponseImpl.h"
#include <CGCLib/CgcClientHandler.h>
#include <CGCBase/cgcSeqInfo.h>
#include <CGCBase/cgcfunc.h>

#define USES_DISABLE_PREV_DATA

#define DEFAULT_HTTP_SESSIONID_LENGTH 32
#define MAX_SEQ_MASKS_SIZE 500//120

//#define USES_MULTI_REMOTE_SESSION
class CHoldResponseInfo
{
public:
	typedef boost::shared_ptr<CHoldResponseInfo> pointer;
	static CHoldResponseInfo::pointer create(const cgcResponse::pointer& pHoldResponse,int nHoldSecond) {
		return CHoldResponseInfo::pointer(new CHoldResponseInfo(pHoldResponse,nHoldSecond));}
	CHoldResponseInfo(const cgcResponse::pointer& pHoldResponse,int nHoldSecond)
		: m_pHoldResponse(pHoldResponse)
	{
		SetHoldSecond(nHoldSecond);
	}
	CHoldResponseInfo(void)
		: m_tExpireTime(0)
	{}
	void SetHoldSecond(int nHoldSecond) {m_tExpireTime=time(0)+nHoldSecond;}
	bool IsExpireHold(void) const {return time(0)>m_tExpireTime;}

	cgcResponse::pointer m_pHoldResponse;
	time_t m_tExpireTime;
};

class CSessionModuleInfo
{
public:
	typedef boost::shared_ptr<CSessionModuleInfo> pointer;
	static CSessionModuleInfo::pointer create(const ModuleItem::pointer& pModuleItem,const cgcRemote::pointer& pRemote) {
		return CSessionModuleInfo::pointer(new CSessionModuleInfo(pModuleItem,pRemote));}
	CSessionModuleInfo(const ModuleItem::pointer& pModuleItem,const cgcRemote::pointer& pRemote)
		: m_pModuleItem(pModuleItem), m_pRemote(pRemote)
		, m_bOpenSessioned(false)
	{}
	CSessionModuleInfo(void)
		: m_bOpenSessioned(false)
	{}
	ModuleItem::pointer m_pModuleItem;
	cgcRemote::pointer m_pRemote;
	cgcResponse::pointer m_pLastResponse;
	CLockMap<unsigned long,bool> m_pRemoteList; // remoteid->
	bool m_bOpenSessioned;
};

class CSessionImpl
	: public cgcSession
	, public CResponseHandler
{
private:
	bool m_bKilled;			// Used for control the thread exit.
	bool m_bIsNewSession;
	bool m_bRemoteClosed;
	time_t m_tCreationTime;
	time_t m_tLastAccessedtime;
	tstring m_sSessionId;
	int m_interval;
	//tstring m_sAccount;
	//tstring m_sPasswd;
	// ssl
	tstring m_sSslPublicKey;
	tstring m_sSslPassword;

	cgcAttributes::pointer m_attributes;

	// Used for save the previous pending data.
#ifndef USES_DISABLE_PREV_DATA
	std::string m_sPrevData;
	// Used for processing XML format error, timeout for remote data is returned.
	boost::shared_ptr<boost::thread> m_pProcPrevData;
	time_t m_tNewProcPrevThread;
	time_t m_tPrevDataTime;
#endif

	bool m_bInvalidate;
	cgcRemote::pointer m_pcgcRemote;	// default remote
	cgcParserBase::pointer m_pcgcParser;
	cgcResponse::pointer m_pLastResponse;
	//cgcCDBCService::pointer m_pCdbcService;	// for sync

	// session ¶ÔÓ¦µÄModuleItem
	CLockMap<tstring,CSessionModuleInfo::pointer> m_pSessionModuleList;	// name->
	//ModuleItem::pointer m_pModuleItem;
	CLockMap<unsigned long,CHoldResponseInfo::pointer> m_pHoldResponseList;	// remoteid->

	unsigned int m_nDataIndex;
	time_t m_tLastSeq;
	boost::mutex m_recvSeq;
	int m_pReceiveSeqMasks[MAX_SEQ_MASKS_SIZE];
	CLockMap<unsigned short, cgcSeqInfo::pointer> m_mapSeqInfo;
	tstring m_sUserAgent;	// FOR http
	CLockMap<int,time_t> m_tLockApiList;
public:
	CSessionImpl(const ModuleItem::pointer& pModuleItem,const cgcRemote::pointer& pcgcRemote,const cgcParserBase::pointer& pcgcParser);
	virtual ~CSessionImpl(void);

public:
	bool IsKilled(void) const {return m_bKilled;}
	bool lockSessionApi(int nLockType, int nLockSeconds=0);
	void unlockSessionApi(int nLockType);
	//bool isInSessionApiLocked(const std::string& sApi, int nLockSeconds);
	//void removeSessionApiLock(const std::string& sApi);

	//void SetCDBCService(const cgcCDBCService::pointer& v) {m_pCdbcService = v;}
	//cgcCDBCService::pointer GetCDBCService(void) const {return m_pCdbcService;}

	// invoke module CGC_Session_Open()
	bool OnRunCGC_Session_Open(const ModuleItem::pointer& pModuleItem,const cgcRemote::pointer& pcgcRemote);

	// invoke module CGC_Session_Close()
	void OnRunCGC_Remote_Change(const cgcRemote::pointer& pcgcRemote);
	void OnRunCGC_Remote_Change(const CSessionModuleInfo::pointer& pSessionModuleInfo,const cgcRemote::pointer& pcgcRemote);
	void OnRunCGC_Remote_Close(unsigned long nRemoteId, int nErrorCode);
	void OnRunCGC_Remote_Close(const CSessionModuleInfo::pointer& pSessionModuleInfo,unsigned long nRemoteId,int nErrorCode);
	void OnRunCGC_Session_Close(void);
	void OnRunCGC_Session_Close(const CSessionModuleInfo::pointer& pSessionModuleInfo);
	void OnServerStop(void);

	void ProcHoldResponseTimeout(void);
	void HoldResponse(const cgcResponse::pointer& pResponse, int nHoldSecond);
	void removeResponse(unsigned long nRemoteId);
	void onRemoteClose(unsigned long remoteId, int nErrorCode);
	bool isRemoteClosed(void) const {return m_bRemoteClosed;}

	const tstring& SetSessionId(const cgcParserBase::pointer& pcgcParser);
	//void SetUserAgent(const tstring& sUserAgent) {m_sUserAgent = sUserAgent;}
	//const tstring& GetUserAgent(void) const {return m_sUserAgent;}
	void SetSslPublicKey(const tstring& newValue);
	const tstring& GetSslPublicKey(void) const {return m_sSslPublicKey;}
	bool IsSslRequest(void) const {return m_sSslPublicKey.empty()?false:true;}
	void SetSslPassword(const tstring& newv) {m_sSslPassword = newv;}
	const tstring& GetSslPassword(void) const {return m_sSslPassword;}

	// cgcResponse
	void setDataResponseImpl(const tstring& sModuleName,const cgcRemote::pointer& wssRemote, const cgcParserBase::pointer& pcgcParser);
	void setDataResponseImpl(const tstring& sModuleName,const cgcRemote::pointer& wssRemote);	// sModuleName="": setdefault
	bool isEmpty(void) const;
	//void setAttributes(const cgcAttributes::pointer& pAttributes) {m_attributes = pAttributes;}

#ifndef USES_DISABLE_PREV_DATA
	void do_prevdata(void);

	time_t getNewPrevDataThreadTime(void) const {return m_tNewProcPrevThread;}
	void newPrevDataThread(void);
	void delPrevDatathread(bool bJoin);

	// The previous pending data.
	bool isHasPrevData(void) const {return !m_sPrevData.empty();}
	const std::string & addPrevData(const char * sUnPrecData);
	size_t getPrevDataLength(void) const {return m_sPrevData.size();}
	void clearPrevData(void) {m_sPrevData.clear();m_tPrevDataTime=0;delPrevDatathread(false);}
	time_t getPrevDataTime(void) const {return m_tPrevDataTime;}
#endif

	// module
	CSessionModuleInfo::pointer addModuleItem(const ModuleItem::pointer& pModuleItem,const cgcRemote::pointer& wssRemote);
	ModuleItem::pointer getModuleItem(const tstring& sModuleName, bool bGetDefault) const;
	bool existModuleItem(const tstring& sModuleName) const {return m_pSessionModuleList.exist(sModuleName);}
	//void setModuleItem(ModuleItem::pointer newValue) {m_pModuleItem = newValue;}
	//ModuleItem::pointer getModuleItem(void) const {return m_pModuleItem;}

	bool ProcessDataResend(void);
	void ProcessAckSeq(unsigned short seq);
	bool ProcessDuplationSeq(unsigned short seq);

	// account & passwd
	//void setAccontInfo(const tstring & sAccount, const tstring & sPasswd);
	//const tstring & GetAccount(void) const {return m_sAccount;}
protected:
	// CResponseHandler
	boost::mutex m_mutexReq;
	unsigned short m_nCurrentSeq;
	virtual unsigned short onGetNextSeq(void);
	time_t m_tLastSeqInfoTime;
	virtual int onAddSeqInfo(const unsigned char * callData, unsigned int dataSize, unsigned short seq, unsigned long cid, unsigned long sign);
	virtual int onAddSeqInfo(unsigned char * callData, unsigned int dataSize, unsigned short seq, unsigned long cid, unsigned long sign);

	// session info
	virtual int getProtocol(void) const {return m_pcgcRemote->getProtocol();}
	virtual bool isNewSession(void) const{return m_bIsNewSession;}
	virtual time_t getCreationTime(void) const{return m_tCreationTime;}
	virtual void invalidate(void);
	virtual bool isInvalidate(void) const{return m_bInvalidate || m_pcgcRemote->isInvalidate();}
	virtual const tstring & getId(void) const {return m_sSessionId;}
	virtual unsigned long getRemoteId(void) const {return m_pcgcRemote->getRemoteId();}
	//virtual const tstring & getAccount(void) const {return m_sAccount;}
	//virtual const tstring & getSecure(void) const {return m_sPasswd;}
	virtual time_t getLastAccessedtime(void) const {return m_tLastAccessedtime;}

	virtual void setMaxInactiveInterval(int interval);
	virtual int getMaxInactiveInterval(void) const {return m_interval;}

	virtual cgcAttributes::pointer getAttributes(bool create);
	virtual cgcAttributes::pointer getAttributes(void) const {return m_attributes;}
	//cgcSession::pointer cgc_shared_from_this(void);
	virtual cgcResponse::pointer getLastResponse(const tstring& moduleName) const;
	virtual cgcResponse::pointer getHoldResponse(unsigned long nRemoteId);
};

#define SESSIONIMPL(M,R,P) cgcSession::pointer(new CSessionImpl(M,R,P))

class CSessionMgr
{
public:
	CSessionMgr(void);
	virtual ~CSessionMgr(void);
	
public:
	cgcSession::pointer SetSessionImpl(const ModuleItem::pointer& pModuleItem,const cgcRemote::pointer& pRemote,const cgcParserBase::pointer& pcgcParser);
	cgcSession::pointer GetSessionImplByRemote(unsigned long remoteId);
	cgcSession::pointer GetSessionImpl(const tstring & sSessionId) const;
	cgcSession::pointer GetSessionImpl(const tstring & sSessionId, unsigned long pNewRemoteId);
	//void ChangeSessionId(const tstring& sOldSessionId,const cgcRemote::pointer& wssRemote, const cgcParserBase::pointer& pcgcParser);	// for HTTP
	void SetRemoteSession(unsigned long remoteId,const cgcSession::pointer& pSession);
	//void SetRemoteSession(unsigned long remoteId,const tstring& sSessionId);

	void RemoveSessionImpl(const cgcSession::pointer & pSessionImpl, bool bRemoveSessionIdMap);
	//void setInterval(unsigned long remoteId, int interval);
	void onRemoteClose(unsigned long remoteId, int nErrorCode);

	bool ProcDataResend(void);
	void ProcLastAccessedTime(std::vector<std::string>& pOutCloseSidList);
	void ProcInvalidRemoteIdList(void);
	void invalidates(bool bStop);
	void FreeHandle(void);

private:
	int OnSessionClosedTimeout(const cgcSession::pointer & pSessionImpl, bool bRemoveSessionIdMap);

private:
#ifdef USES_MULTI_REMOTE_SESSION
	CLockMultiMap<unsigned long, tstring> m_mapRemoteSessionId;		// remoteid to sessionid
#else
	CLockMap<unsigned long, cgcSession::pointer> m_mapRemoteSessionId;		// remoteid to sessionimpl
	//CLockMap<unsigned long, tstring> m_mapRemoteSessionId;		// remoteid to sessionid
#endif
	CLockMap<tstring, cgcSession::pointer> m_mapSessionImpl;	// sessionid to sessionimpl

};

#endif // _SESSIONMGR_HEAD_INCLUDED__
