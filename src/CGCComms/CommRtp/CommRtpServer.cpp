/*
    MYCP is a HTTP and C++ Web Application Server.
	CommRtpServer is a RTP socket server.
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

// CommRtpServer.cpp : Defines the exported functions for the DLL application.
//

//////////////////////////////////////////////////////////////
// const
#ifdef WIN32
#pragma warning(disable:4267 4311 4996)

#include <winsock2.h>
#include <windows.h>

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "Ws2_32.lib")
#else
#pragma comment(linker,"/NODEFAULTLIB:libcmt.lib") 
#pragma comment(lib, "Ws2_32.lib")
#endif // _DEBUG
#endif // WIN32

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
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // WIN32

// cgc head
#include <CGCBase/comm.h>
#include <CGCServices/Rtp/cgcRtp.h>
#include <ThirdParty/stl/locklist.h>
#include <ThirdParty/stl/lockmap.h>
using namespace cgc;

#if (USES_RTP)

#include "../CgcRemoteInfo.h"

class CRtpRemoteInfo
	: public CCommEventData
{
public:
	typedef boost::shared_ptr<CRtpRemoteInfo> pointer;

	static CRtpRemoteInfo::pointer create(CCommEventData::CommEventType commEventType)
	{
		return CRtpRemoteInfo::pointer(new CRtpRemoteInfo(commEventType));
	}

	void recvdata(CRTPData::pointer newv) {m_recvdata = newv;}
	CRTPData::pointer recvdata(void) const {return m_recvdata;}

public:
	CRtpRemoteInfo(CCommEventData::CommEventType commEventType)
		: CCommEventData(commEventType)
	{}
private:
	CRTPData::pointer m_recvdata;
};

class CRemoteHandler
{
public:
	//virtual void onInvalidate(const DoRtpHandler::pointer& doRtpHandler) = 0;

	virtual unsigned long getCommId(void) const = 0;
	virtual int getProtocol(void) const = 0;
	virtual int getServerPort(void) const = 0;
};

////////////////////////////////////////
// CcgcRemote
class CcgcRemote
	: public cgcRemote
{
private:
	u_long m_destIp;
	u_int m_destPort;
	CRemoteHandler * m_handler;
	DoRtpHandler::pointer m_pDoRtpHandler;
	tstring m_sRemoteAddr;			// string of IP address

public:
	CcgcRemote(u_long destIp, u_int destPort, CRemoteHandler * handler, DoRtpHandler::pointer doRtpHandler)
		: m_destIp(destIp), m_destPort(destPort)
		, m_handler(handler), m_pDoRtpHandler(doRtpHandler)
	{
		assert (m_handler != NULL);

		struct in_addr addr;
#ifdef WIN32
		addr.S_un.S_addr = htonl(destIp);
#else
		addr.s_addr = htonl(destIp);
#endif
		char * buffer = inet_ntoa(addr);

		char bufferTemp[256];
		memset(bufferTemp, 0, 256);
		sprintf(bufferTemp, "%s:%d", buffer, destPort);
		m_sRemoteAddr = bufferTemp;
	}
	virtual ~CcgcRemote(void){}

	void SetRemote(u_long destIp, u_int destPort)
	{
		 m_destIp = destIp;
		m_destPort = destPort;
	}

private:
	virtual int getProtocol(void) const {return m_handler->getProtocol();}
	virtual int getServerPort(void) const {return m_handler->getServerPort();}
	virtual unsigned long getCommId(void) const {return m_handler->getCommId();}
	virtual unsigned long getRemoteId(void) const {return m_pDoRtpHandler.get() == 0 ? 0 : m_destIp+m_destPort;}
	virtual unsigned long getIpAddress(void) const {return m_pDoRtpHandler.get()==NULL?0:m_destIp;}
	virtual tstring getScheme(void) const {return _T("RTP");}
	virtual const tstring & getRemoteAddr(void) const {return m_sRemoteAddr;}
	virtual int sendData(const unsigned char * data, size_t size)
	{
		BOOST_ASSERT(data != 0);
		if (isInvalidate()) return -1;

		m_pDoRtpHandler->doClearDest();
		m_pDoRtpHandler->doAddDest(m_destIp, m_destPort);
		return m_pDoRtpHandler->doSendData(data, size, 0);
	}
	virtual void invalidate(bool bClose)
	{
		if (m_pDoRtpHandler.get() != NULL)
		{
			if (bClose)
				m_handler->onInvalidate(m_pDoRtpHandler);
			m_pDoRtpHandler.reset();
		}
	}
	virtual bool isInvalidate(void) const {return m_pDoRtpHandler.get() == 0;}

};

const int ATTRIBUTE_NAME	= 1;
const int EVENT_ATTRIBUTE	= 2;
static unsigned int theCurrentIdEvent	= 0;
class CIDEvent : public cgcObject
{
public:
	unsigned int m_nCurIDEvent;
	int m_nCapacity;
};
cgcAttributes::pointer theAppAttributes;

/////////////////////////////////////////
// CRtpServer
class CRtpServer
	: public OnRtpHandler
	, public cgcOnTimerHandler
	, public cgcCommunication
	, public CRemoteHandler
	, public boost::enable_shared_from_this<CRtpServer>
{
public:
	typedef boost::shared_ptr<CRtpServer> pointer;
	static CRtpServer::pointer create(cgcRtp::pointer rtpService)
	{
		return CRtpServer::pointer(new CRtpServer(rtpService));
	}

private:
	cgcRtp::pointer m_rtpService;
	int m_commPort;
	int m_capacity;
	int m_protocol;

	DoRtpHandler::pointer m_pDoRtpHandler;
	CLockMap<unsigned long, cgcRemote::pointer> m_mapCgcRemote;
	int m_nDoCommEventCouter;

	CLockList<CRtpRemoteInfo::pointer> m_listMgr;

public:
	CRtpServer(cgcRtp::pointer rtpService)
		: m_rtpService(rtpService), m_commPort(0), m_capacity(1), m_protocol(0)
		, m_nDoCommEventCouter(0)
	{
		BOOST_ASSERT (m_rtpService.get() != NULL);
	}
	virtual ~CRtpServer(void)
	{
		finalService();
		m_rtpService.reset();
	}

	unsigned long getId(void) const {return (unsigned long)this;}
	virtual tstring serviceName(void) const {return _T("RTPSERVER");}
	virtual bool initService(cgcValueInfo::pointer parameter)
	{
		if (m_commHandler.get() == NULL) return false;
		if (m_bServiceInited) return true;

		if (parameter.get() != NULL && parameter->getType() == cgcValueInfo::TYPE_VECTOR)
		{
			const std::vector<cgcValueInfo::pointer>& lists = parameter->getVector();
			if (lists.size() > 2)
				m_protocol = lists[2]->getInt();
			if (lists.size() > 1)
				m_capacity = lists[1]->getInt();
			if (lists.size() > 0)
				m_commPort = lists[0]->getInt();
			else
				return false;
		}

		if (m_pDoRtpHandler.get() == 0)
		{
			m_pDoRtpHandler = m_rtpService->startRtp(m_commPort);
			if (m_pDoRtpHandler.get() == 0)
				return false;
			m_pDoRtpHandler->doSetMediaType(1);
			m_pDoRtpHandler->doSetRtpHandler(this);
		}

		m_capacity = m_capacity < 1 ? 1 : m_capacity;
		CIDEvent * pIDEvent = new CIDEvent();
		pIDEvent->m_nCurIDEvent = theCurrentIdEvent+1;
		pIDEvent->m_nCapacity = m_capacity;
		theAppAttributes->setAttribute(EVENT_ATTRIBUTE, this, cgcObject::pointer(pIDEvent));

		for (int i=0; i<m_capacity; i++)
		{
			theApplication->SetTimer(++theCurrentIdEvent, m_capacity, shared_from_this());
		}

		m_bServiceInited = true;
		CGC_LOG((LOG_INFO, _T("**** [%s:%d] Start succeeded ****\n"), serviceName().c_str(), m_commPort));
		return true;
	}
	virtual void finalService(void)
	{
		if (!m_bServiceInited) return;

		cgcObject::pointer eventPointer = theAppAttributes->removeAttribute(EVENT_ATTRIBUTE, this);
		CIDEvent * pIDEvent = (CIDEvent*)eventPointer.get();
		if (pIDEvent != NULL)
		{
			for (unsigned int i=pIDEvent->m_nCurIDEvent; i<pIDEvent->m_nCurIDEvent+pIDEvent->m_nCapacity; i++)
				theApplication->KillTimer(i);
		}

		m_mapCgcRemote.clear();
		m_listMgr.clear();

		DoRtpHandler::pointer pDoRtpHandlerTemp = m_pDoRtpHandler;
		m_pDoRtpHandler.reset();
		m_rtpService->stopRtp(pDoRtpHandlerTemp);
		m_bServiceInited = false;
		CGC_LOG((LOG_INFO, _T("**** [%s:%d] Stop succeeded ****\n"), serviceName().c_str(), m_commPort));
	}

protected:
	bool eraseIsInvalidated(void)
	{
		boost::mutex::scoped_lock lock(m_mapCgcRemote.mutex());
		CLockMap<unsigned long, cgcRemote::pointer>::iterator pIter;
		for (pIter=m_mapCgcRemote.begin(); pIter!=m_mapCgcRemote.end(); pIter++)
		{
			cgcRemote::pointer pCgcRemote = pIter->second;
			if (pCgcRemote->isInvalidate())
			{
				m_mapCgcRemote.erase(pIter);
				return true;
			}
		}
		return false;
	}

	// cgcOnTimerHandler
	virtual void OnTimeout(unsigned int nIDEvent, const void * pvParam)
	{
		if (++m_nDoCommEventCouter > 120000)
		{
			if (!eraseIsInvalidated())
			{
				m_nDoCommEventCouter = 0;
			}
		}

		if (m_commHandler.get() == NULL) return;
		CRtpRemoteInfo::pointer pCommEventData;
		if (!m_listMgr.front(pCommEventData)) return;
		BOOST_ASSERT (pCommEventData->recvdata() != 0);

		switch (pCommEventData->getCommEventType())
		{
		case CCommEventData::CET_Recv:
			m_commHandler->onRecvData(pCommEventData->getRemote(), pCommEventData->recvdata()->data(), pCommEventData->recvdata()->size());
			break;
		default:
			break;
		}
	}

	// CRemoteHandler
	virtual void onInvalidate(const DoRtpHandler::pointer& doRtpHandler)
	{
		m_rtpService->stopRtp(doRtpHandler);
	}
	virtual unsigned long getCommId(void) const {return getId();}
	virtual int getProtocol(void) const {return m_protocol;}
	virtual int getServerPort(void) const {return m_commPort;}

	// OnRtpHandler
	virtual void onReceiveEvent(CRTPData::pointer receiveData, DoRtpHandler::pointer pDoRtpHandler, void * rtpParam)
	{
		BOOST_ASSERT (receiveData.get() != 0);
		if (m_commHandler.get() != NULL)
		{
			// ?
			unsigned long destIp = receiveData->destip();
			unsigned int  destPort = receiveData->destport();
			u_long remoteAddrHash = destIp + destPort;

			cgcRemote::pointer pCgcRemote;
			if (!m_mapCgcRemote.find(remoteAddrHash, pCgcRemote, false))
			{
				pCgcRemote = cgcRemote::pointer(new CcgcRemote(destIp, destPort, (CRemoteHandler*)this, m_pDoRtpHandler));
				m_mapCgcRemote.insert(remoteAddrHash, pCgcRemote);
			}else if (pCgcRemote->isInvalidate())
			{
				m_mapCgcRemote.remove(remoteAddrHash);
				pCgcRemote = cgcRemote::pointer(new CcgcRemote(destIp, destPort, (CRemoteHandler*)this, m_pDoRtpHandler));
				m_mapCgcRemote.insert(remoteAddrHash, pCgcRemote);
			}else
			{
				((CcgcRemote*)pCgcRemote.get())->SetRemote(destIp, destPort);
			}
			CRtpRemoteInfo::pointer pEventData = CRtpRemoteInfo::create(CCommEventData::CET_Recv);
			pEventData->setRemote(pCgcRemote);
			pEventData->setRemoteId(pCgcRemote->getRemoteId());
			pEventData->recvdata(receiveData);
			m_listMgr.add(pEventData);
		}
	}

	virtual void onRtpKilledEvent(DoRtpHandler::pointer pDoRtpHandler, void * rtpParam)
	{
		while (true)
		{
			if (m_listMgr.empty())
				break;

#ifdef WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
		}

	}
};


cgcRtp::pointer gRtpService;

extern "C" bool CGC_API CGC_Module_Init(void)
{
#ifdef WIN32
	WSADATA wsaData;
	int err = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	if ( err != 0 ) {
		theApplication->log(LOG_ERROR, _T("WSAStartup' error!\n"));
		return false;
	}
#endif // WIN32

	cgcParameterMap::pointer initParameters = theApplication->getInitParameters();
	tstring rtpServiceName = initParameters->getParameterValue("RtpServiceName", "RtpService");

	gRtpService = CGC_RTPSERVICE_DEF(theServiceManager->getService(rtpServiceName));
	BOOST_ASSERT (gRtpService.get() != NULL);

	theAppAttributes = theApplication->getAttributes(true);
	assert (theAppAttributes.get() != NULL);
	return true;
}

// 模块退出函数，可选实现函数
extern "C" void CGC_API CGC_Module_Free(void)
{
	VoidObjectMapPointer mapLogServices = theAppAttributes->getVoidAttributes(ATTRIBUTE_NAME, false);
	if (mapLogServices.get() != NULL)
	{
		CObjectMap<void*>::iterator iter;
		for (iter=mapLogServices->begin(); iter!=mapLogServices->end(); iter++)
		{
			CRtpServer::pointer commServer = CGC_OBJECT_CAST<CRtpServer>(iter->second);
			if (commServer.get() != NULL)
			{
				commServer->finalService();
			}
		}
	}

	theAppAttributes.reset();
	theApplication->KillAllTimer();
	theServiceManager->resetService(gRtpService);
	gRtpService.reset();

#ifdef WIN32
	WSACleanup();
#endif // WIN32
}

extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	CRtpServer::pointer commServer = CRtpServer::create(gRtpService);
	outService = commServer;
	theAppAttributes->setAttribute(ATTRIBUTE_NAME, outService.get(), commServer);
}

extern "C" void CGC_API CGC_ResetService(const cgcServiceInterface::pointer & inService)
{
	if (inService.get() == NULL) return;
	theAppAttributes->removeAttribute(ATTRIBUTE_NAME, inService.get());
	inService->finalService();
}

#endif // USES_RTP

