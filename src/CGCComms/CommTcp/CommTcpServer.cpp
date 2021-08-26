/*
    MYCP is a HTTP and C++ Web Application Server.
	CommTcpServer is a TCP socket server.
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

// CommTcpServer.cpp : Defines the exported functions for the DLL application.
// 
#ifdef WIN32
#pragma warning(disable:4018 4267 4311 4996)
#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0501     // Change this to the appropriate value to target other versions of Windows.
#endif
#include <winsock2.h>
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
//#pragma comment(lib, "libeay32.lib")  
//#pragma comment(lib, "ssleay32.lib") 
#else
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <semaphore.h>
#include <time.h>
//#define USES_TCP_TEST_CONNECT	// linux
#endif // WIN32
//#define USES_TCP_TEST_CONNECT	// **all

//#define USES_PRINT_DEBUG
// cgc head
#include <CGCBase/comm.h>
//#define USES_PROTOCOL_HSOTP
#ifdef USES_PROTOCOL_HSOTP
#include <CGCClass/cgcclassinclude.h>
#endif
#include <ThirdParty/stl/locklist.h>
#include <ThirdParty/stl/lockmap.h>
#include <ThirdParty/Boost/asio/IoService.h>
#include <ThirdParty/Boost/asio/TcpAcceptor.h>
//#ifdef USES_TCP_TEST_CONNECT
#include <ThirdParty/Boost/asio/TcpClient.h>	// for uses mycp::asio::TcpClient::Test_To_SSL_library_init();
//#endif
using namespace mycp;

#include "../CgcRemoteInfo.h"
#define USES_SUPPORT_HTTP_CONNECTION
#define USES_CONNECTION_CLOSE_EVENT

class CRemoteHandler
{
public:
//#ifndef USES_SUPPORT_HTTP_CONNECTION
	virtual void onCloseConnection(const mycp::asio::TcpConnectionPointer& connection, int nWaitSecond, bool bCloseSocket) = 0;
//#endif
};

//#define USES_DEBUG_REMOTE_COUNT
#ifdef USES_DEBUG_REMOTE_COUNT
int theRemoteCount = 0;
int theConnectionCount = 0;
int theAcceptRemoteCount = 0;
#endif

////////////////////////////////////////

// CcgcRemote
class CcgcRemote
	: public cgcRemote
{
private:
	CRemoteHandler * m_handler;
	mycp::asio::TcpConnectionPointer m_connection;
#ifdef USES_SUPPORT_HTTP_CONNECTION
	bool m_bIsInvalidate;
	time_t m_tCloseTime;
#else
	mycp::asio::TcpConnectionPointer m_pCloseConnection;
#endif
	tstring m_sServerAddr;			// string of IP address
	tstring m_sRemoteAddr;			// string of IP address
	unsigned long m_commId;
	unsigned long m_remoteId;
	unsigned long m_ipAddress;
	int m_protocol;
	int m_serverPort;
#ifdef USES_PROTOCOL_HSOTP
	cgcParserHttp::pointer m_pParserHttp;
#endif

#ifdef USES_CONNECTION_CLOSE_EVENT
#ifdef WIN32
	HANDLE m_hDoCloseEvent;
#else
	sem_t m_semDoClose;
#endif
#endif

public:
	CcgcRemote(CRemoteHandler * handler, const mycp::asio::TcpConnectionPointer& pConnection,unsigned long commId,  unsigned long remoteId, int protocol)
		: m_handler(handler), m_connection(pConnection)
#ifdef USES_SUPPORT_HTTP_CONNECTION
		, m_bIsInvalidate(false), m_tCloseTime(0)
#endif
		, m_commId(commId),m_remoteId(remoteId),m_ipAddress(0), m_protocol(protocol)
		, m_serverPort(0)
	{
#ifdef USES_CONNECTION_CLOSE_EVENT
#ifdef WIN32
		m_hDoCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
		sem_init(&m_semDoClose, 0, 0);
#endif // WIN32
#endif

#ifdef USES_DEBUG_REMOTE_COUNT
		theRemoteCount++;
		theConnectionCount++;
		printf("**** CcgcRemote(), theRemoteCount=%d,theConnectionCount=%d\n",theRemoteCount,theConnectionCount);
#endif

		BOOST_ASSERT(m_handler != 0);
		BOOST_ASSERT(pConnection.get() != 0);

		try
		{
			m_ipAddress = m_connection->getIpAddress();
			tcp::endpoint remoteEndpoint = m_connection->remote_endpoint();
			const unsigned short port = remoteEndpoint.port();
			boost::system::error_code ignored_error;
			const std::string address(remoteEndpoint.address().to_string(ignored_error));
			char bufferTemp[256];
			memset(bufferTemp, 0, 256);
			sprintf(bufferTemp, "%s:%d", address.c_str(), port);
			m_sRemoteAddr = bufferTemp;
			m_sServerAddr = m_connection->server_address();
		}catch (std::exception & e)
		{
#ifdef WIN32
			printf("CcgcRemote->remote_endpoint exception. %s, lasterror=%d\n", e.what(), GetLastError());
#else
			printf("CcgcRemote->remote_endpoint exception. %s\n", e.what());
#endif
		}catch (boost::exception &)
		{}catch(...)
		{}
		m_nRequestSize = 0;
	}
	virtual ~CcgcRemote(void)
	{
#ifdef USES_CONNECTION_CLOSE_EVENT
#ifdef WIN32
		CloseHandle(m_hDoCloseEvent);
#else
		sem_destroy(&m_semDoClose);
#endif // WIN32
#endif

#ifdef USES_SUPPORT_HTTP_CONNECTION
		m_handler->onCloseConnection(m_connection, 5, false);
		m_connection.reset();
#else
		if (m_pCloseConnection.get()!=NULL)
		{
			m_handler->onCloseConnection(m_pCloseConnection, 5);
			m_pCloseConnection.reset();
		}
#endif
#ifdef USES_DEBUG_REMOTE_COUNT
		theRemoteCount--;
		theConnectionCount--;
		printf("**** ~CcgcRemote(), theRemoteCount=%d,theConnectionCount=%d\n",theRemoteCount,theConnectionCount);
#endif
	}
	
	time_t GetCloseTime(void) const {return m_tCloseTime;}
	bool getIsInvalidate(void) const {return m_bIsInvalidate;}
	void UpdateConnection(const mycp::asio::TcpConnectionPointer& pConnection)
	{
		if (pConnection.get()==NULL || pConnection->is_closed()) return;
		if (m_connection.get()!=pConnection.get())
		{
			boost::mutex::scoped_lock lock(m_pConnectionMutex);
#ifdef USES_SUPPORT_HTTP_CONNECTION
			m_handler->onCloseConnection(m_connection, 5, false);
#endif
			m_connection = pConnection;
			m_remoteId = m_connection->getId(); 
			m_ipAddress = m_connection->getIpAddress();
		}
#ifdef USES_SUPPORT_HTTP_CONNECTION
		m_bIsInvalidate = false;
		m_tCloseTime = 0;
#endif
	}
	void SetServerPort(int v) {m_serverPort = v;}
#ifdef USES_PROTOCOL_HSOTP
	void SetParserHttp(const cgcParserHttp::pointer& p) {m_pParserHttp=p;}
	const cgcParserHttp::pointer& GetParserhttp(void) const {return m_pParserHttp;}
#endif

#ifdef USES_CONNECTION_CLOSE_EVENT
	void SetCloseEvent(void)
	{
#ifdef WIN32
		SetEvent(m_hDoCloseEvent);
#else
		sem_post(&m_semDoClose);
#endif // WIN32
	}
	void WaitForCloseEvent(void)
	{
#ifdef WIN32
		WaitForSingleObject(m_hDoCloseEvent, 3000);
#else
		//sem_wait(&m_semDoClose);
		//while (true)
		{
			struct timespec timeSpec;
			clock_gettime(CLOCK_REALTIME, &timeSpec);
			timeSpec.tv_sec += 3;

			sem_timedwait(&m_semDoClose, &timeSpec);
			//int s = sem_timedwait(&m_semDoClose, &timeSpec);
			//if (s == -1 || errno == EINTR)
			////if (s == -1 && errno == EINTR)
			//	break;

			//printf("1: sem_timedwait s=%d\n", s);
			//timeSpec.tv_sec += 1;
			//s = sem_timedwait(&m_semDoStop, &timeSpec);
			//if (s == -1 || errno == EINTR)
			////if (s == -1 && errno == EINTR)
			//	break;
			//printf("2: sem_timedwait s=%d\n", s);
		}
		//printf("***** OnRemoteClose OK\n");
#endif
	}
#endif


	//void AddPrevData(const char* lData,size_t nSize)
	//{
	//	m_sPrevData.append(lData,nSize);
	//}
	size_t m_nRequestSize;
	std::string m_sPrevData;
private:
	virtual int getProtocol(void) const {return m_protocol;}
	virtual int getServerPort(void) const {return m_serverPort;}
	virtual const tstring& getServerAddr(void) const {return m_sServerAddr;}
	virtual unsigned long getCommId(void) const {return m_commId;}
	virtual unsigned long getRemoteId(void) const {return m_remoteId;}
	virtual unsigned long getIpAddress(void) const {return m_ipAddress;}
	//virtual unsigned long getRemoteId(void) const {return m_connection.get() == 0 ? 0 : m_connection->getId();}
	//virtual unsigned long getIpAddress(void) const {return m_connection.get()==NULL?0:m_connection->getIpAddress();}
	virtual tstring getScheme(void) const
	{
		if (m_protocol & (int)PROTOCOL_HTTP)
			return "HTTP";
		else if (m_protocol & (int)PROTOCOL_HSOTP)
			return "HTTP SOTP";
		else
			return "TCP";
	}
	virtual const tstring & getRemoteAddr(void) const {return m_sRemoteAddr;}
	boost::mutex m_pConnectionMutex;
	virtual int sendData(const unsigned char * data, size_t size)
	{
		if (data == NULL || size==0) return -1;
		//if (data == NULL || isInvalidate()) return -1;
		boost::mutex::scoped_lock lock(m_pConnectionMutex);
		if (m_connection.get()==NULL || m_connection->is_closed()) return -1;
		try
		{
			//printf("******** sendData:%lu size=%d\n",m_remoteId,size);

#ifndef WIN32
			// 忽略broken pipe信號
			//signal(SIGPIPE, SIG_IGN);
#endif
#ifdef USES_PROTOCOL_HSOTP
			if ((m_protocol & (int)PROTOCOL_HSOTP) && m_pParserHttp.get()!=NULL)
			{
				m_pParserHttp->reset();
				if (m_pParserHttp->getHttpMethod() == HTTP_OPTIONS)
				{
					m_pParserHttp->setHeader("Origin", "null");
					m_pParserHttp->setHeader("Accept", "*/*");
					//m_pParserHttp->addContentLength(false);	// 设置Content-Length: 0，客户端不会立即断开
				}else
				{
					/*
					//const std::string sScriptTest("<script type='text/javascript'>alert(\"11111111\");</script>");
					const std::string sScriptTest("<script type='text/javascript'>onsotpdata(\"11111111\");</script>");
					char lpszChuckData[1024];
					
					// 会执行jsavscript，只收到第一条，并且会立即关闭
					// 带不带 \0都一样
					m_pParserHttp->addContentLength(false);
					m_pParserHttp->setHeader("Cache-Control: no-cache, must-revalidate");
					m_pParserHttp->setHeader("Transfer-Encoding: chunked");
					sprintf(lpszChuckData,"%x\r\n%s",sScriptTest.size(),sScriptTest.c_str());
					m_pParserHttp->write(lpszChuckData,strlen(lpszChuckData));
					size_t outSize = 0;
					const char * responseData = m_pParserHttp->getHttpResult(outSize);
					printf("****response****\n%s\n",responseData);
					boost::system::error_code ignored_error;
					boost::asio::write(m_connection->socket(), boost::asio::buffer(responseData,outSize), boost::asio::transfer_all(), ignored_error);
#ifdef WIN32
					Sleep(1000);
#else
					usleep(1000000);
#endif // WIN32
					return 0;
					*/

					//// 第二个没用
					//sprintf(lpszChuckData,"\r\n\r\n%x\r\n%s\r\n0\r\n\r\n",sScriptTest.size(),sScriptTest.c_str());
					//outSize = strlen(lpszChuckData);
					//boost::asio::write(m_connection->socket(), boost::asio::buffer(lpszChuckData,outSize), boost::asio::transfer_all(), ignored_error);
					//return 0;

					/*
					// 会执行Script
					// 带Content-Length: xxx的话，会有Connection reset by peer.
					// 不带Content-Length: xxx的话，要结束的时候才能处理数据，并且第二次数据，带了HTTP/1.1 200 OK...头
					m_pParserHttp->addContentLength(false);
					m_pParserHttp->write(sScriptTest);
					size_t outSize = 0;
					const char * responseData = m_pParserHttp->getHttpResult(outSize);
					printf("****response****\n%s\n",responseData);
					boost::system::error_code ignored_error;
					boost::asio::write(m_connection->socket(), boost::asio::buffer(responseData,outSize), boost::asio::transfer_all(), ignored_error);
					return 0;
					*/

					//static int theIndex=0;
					//int nStatusCode = (int)STATUS_CODE_206+(theIndex++);
					//m_pParserHttp->setStatusCode((HTTP_STATUSCODE)nStatusCode);	// 没用
					/*
					//m_pParserHttp->write((const char*)data,size);
					// 可以收到一条数据
					m_pParserHttp->addContentLength(false);
					m_pParserHttp->setHeader("Transfer-Encoding: chunked");	// 12004
					//m_pParserHttp->setHeader("Cache-Control: no-cache, must-revalidate");
					char lpszBuffer[20];
					sprintf(lpszBuffer,"%x\r\n",size);
					m_pParserHttp->write(lpszBuffer);
					m_pParserHttp->write((const char*)data,size);
					m_pParserHttp->write("\r\n0\r\n\r\n");
					*/
					//static bool bEnd = false;
					//if (bEnd)
					//	m_pParserHttp->write("\r\n0");
					//else
					//	bEnd = true;
					// 不行
					/*
					char lpszBuffer[100];
					sprintf(lpszBuffer,"%d",size);
					m_pParserHttp->write("<script type='text/javascript'>\r\n");
					m_pParserHttp->write("var comet = window.parent.comet;\r\n");
					m_pParserHttp->write("comet.printServerTime('");
					m_pParserHttp->write(lpszBuffer);
					m_pParserHttp->write("');\r\n");
					m_pParserHttp->write("</script>\r\n\r\n\r\n");
					*/

					//m_pParserHttp->addContentLength(false);
					//m_pParserHttp->setHeader("Transfer-Encoding: chunked");
					//std::string sOnSotopData;
					//sOnSotopData.append("<script type='text/javascript'>\r\n");
					//sOnSotopData.append("$(\"p\").html('11111111111');\r\n");
					//sOnSotopData.append("onsotpdata('222222222');\r\n");
					//sOnSotopData.append("</script>");
					//char lpszBuffer[100];
					//sprintf(lpszBuffer,"%d\r\n",sOnSotopData.size());
					//m_pParserHttp->write(lpszBuffer);
					//m_pParserHttp->write(sOnSotopData);



					////m_pParserHttp->write("onsotpdata(\"abcadfadsfadsf\");");	// 
					//m_pParserHttp->write("\r\nvar sotpdata=\"");
					//m_pParserHttp->write((const char*)data,size);
					//m_pParserHttp->write("\";\r\n");
					//m_pParserHttp->write("onsotpdata(sotpdata);");			// 
					////m_pParserHttp->write("javascript:onsotpdata(\"");
					////m_pParserHttp->write((const char*)data,size);
					////m_pParserHttp->write("\")");
					//m_pParserHttp->write("<script type='text/javascript'>");
					//m_pParserHttp->write("comet.printServerTime('");
					//m_pParserHttp->write("2012-11-11");
					//m_pParserHttp->write("');");
					//m_pParserHttp->write("</script>");
				}
				//m_pParserHttp->addContentLength(false);
				//m_pParserHttp->setHeader("Cache-Control: no-cache, must-revalidate");
				//m_pParserHttp->setHeader("Transfer-Encoding: chunked");	// 12004
				//m_pParserHttp->setHeader("Content-Length: 0");				// 0客户端，收不到数据
				//m_pParserHttp->setContentType("application/sotp; charset=utf-8");
				//m_pParserHttp->setContentType("application/x-www-form-urlencoded; charset=UTF-8");
				m_pParserHttp->write((const char*)data,size);

				size_t outSize = 0;
				const char * responseData = m_pParserHttp->getHttpResult(outSize);
				//printf("****response****\n%s\n",responseData);
				boost::system::error_code ignored_error;
				boost::asio::write(*m_connection->socket(), boost::asio::buffer(responseData,outSize), boost::asio::transfer_all(), ignored_error);
				if (!m_pParserHttp->isKeepAlive())
				{
#ifdef WIN32
					Sleep(100);
#else
					usleep(100000);
#endif // WIN32
					invalidate(true);
				}
			}else
#endif
			{
				//boost::asio::async_write(m_connection->socket(), boost::asio::buffer(data, size), boost::bind(&TcpConnection::write_handler,m_connection,boost::asio::placeholders::error));
				boost::system::error_code ignored_error;
				boost::asio::write(*m_connection->socket(), boost::asio::buffer(data, size), boost::asio::transfer_all(), ignored_error);
			}
		}catch (std::exception&)
		{
			//std::cerr << e.what() << std::endl;
			return -2;
		}catch (boost::exception&)
		{
			return -2;
		}catch(...)
		{
			return -2;
		}
		return 0;
	}
	virtual void invalidate(bool bClose)
	{
#ifdef USES_SUPPORT_HTTP_CONNECTION
		m_bIsInvalidate = true;
		if (m_tCloseTime==0)
			m_tCloseTime = time(0);
#else
		try
		{
			boost::mutex::scoped_lock lock(m_pConnectionMutex);
			if (m_connection.get()!=NULL)
			{
				if (bClose)
					m_handler->onCloseConnection(m_connection, 10);
				else
					m_pCloseConnection = m_connection;
				m_connection.reset();
			}else if (bClose && m_pCloseConnection.get()!=NULL)
			{
				m_handler->onCloseConnection(m_pCloseConnection, 10);
				m_pCloseConnection.reset();
			}
		}catch (std::exception&)
		{
		}catch (boost::exception&)
		{
		}catch(...)
		{
		}
#endif

		//if (m_connection.get() != NULL)
		//{
		//	m_handler->onCloseConnection(m_connection);
		//	m_connection.reset();
		//}
	}
	virtual bool isInvalidate(void) const {
#ifdef USES_SUPPORT_HTTP_CONNECTION
		if (m_bIsInvalidate) return true;
#endif
		boost::mutex::scoped_lock lock(const_cast<boost::mutex&>(m_pConnectionMutex));
		return (m_connection.get() == 0 || m_connection->is_closed())?true:false;
	}

};

const int ATTRIBUTE_NAME	= 1;
//const int EVENT_ATTRIBUTE	= 2;

#define MAIN_MGR_EVENT_ID	1
#define MIN_EVENT_THREAD	10
#define MAX_EVENT_THREAD	600

//static unsigned int theCurrentIdEvent	= 0;
//class CIDEvent : public cgcObject
//{
//public:
//	unsigned int m_nCurIDEvent;
//	int m_nCapacity;
//};
cgcAttributes::pointer theAppAttributes;
tstring theSSLPasswd;
int theLimitRestartCount = 0;	/// 限制检查重启次数；默认 0 次，0 不重启
int theCheckRestartCount = 0;	/// 已经重启次数，不限制，不计数
int theThreadStackSize = 1024;	/// 

bool FileIsExist(const char* pFile)
{
	FILE * f = fopen(pFile,"r");
	if (f==NULL)
		return false;
	fclose(f);
	return true;
}
//#define USES_PRINT_DEBUG
#ifdef USES_TCP_TEST_CONNECT
//short theLocalIpTestCount = 0;
class CTcpTestConnect
	: public mycp::asio::TcpClient_Handler
	, public std::enable_shared_from_this<CTcpTestConnect>
{
public:
	typedef std::shared_ptr<CTcpTestConnect> pointer;
	static CTcpTestConnect::pointer create(void)
	{
		return CTcpTestConnect::pointer(new CTcpTestConnect());
	}

	bool TestConnect(const char* sIp, int nPort, bool bSsl)
	{
		bool bResult = false;
		try
		{
			boost::asio::ssl::context * sslctx = NULL;
			if (bSsl) {
				if (m_sslctx==NULL) {
					mycp::asio::TcpClient::Test_To_SSL_library_init();
					namespace ssl = boost::asio::ssl;
					m_sslctx = new boost::asio::ssl::context (ssl::context::sslv23_client);	// OK
					m_sslctx->set_default_verify_paths();
					m_sslctx->set_options(ssl::context::verify_peer);
					m_sslctx->set_verify_mode(ssl::verify_peer);
					//m_sslctx->set_verify_callback(ssl::rfc2818_verification("smtp.163.com"));
				}
				sslctx = m_sslctx;
			}

			if (m_ipService.get()==NULL)
				m_ipService = mycp::asio::IoService::create();
			if (m_tcpClient.get()==NULL)
				m_tcpClient = mycp::asio::TcpClient::create(shared_from_this());
			boost::system::error_code ec;
			tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string(sIp,ec), nPort);
			m_tcpClient->connect(m_ipService->ioservice(), endpoint, sslctx, false);
			m_ipService->start();

			for (int i=0;i<50;i++) {	// * 5S
				if (m_connectReturned || !theApplication->isInited())
					break;
#ifdef WIN32
				Sleep(100);
#else
				usleep(100000);
#endif
			}
			bResult = !m_bDisconnect;
			if (bResult) {
				m_tcpClient->write((const unsigned char*)"test",4);
			}
#ifdef USES_PRINT_DEBUG
			printf("**** %d TestConnect.... bResult=%d\n",nPort,(int)bResult?1:0);
#endif
			m_tcpClient->disconnect();
#ifdef USES_PRINT_DEBUG
			printf("**** %d TestConnect.... disconnect ok\n",nPort);
#endif
			m_ipService->stop();
#ifdef USES_PRINT_DEBUG
			printf("**** %d TestConnect.... stop ok\n",nPort);
#endif
			m_tcpClient.reset();
#ifdef USES_PRINT_DEBUG
			printf("**** %d TestConnect.... m_tcpClient.reset ok\n",nPort);
#endif
			m_ipService.reset();
#ifdef USES_PRINT_DEBUG
			printf("**** %d TestConnect.... m_ipService.reset ok finished\n",nPort);
#endif
			//printf("**** OK\n");
		}
		catch (const std::exception &) {
		}
		catch (const boost::exception &) {
		}
		catch(...) {
		}	
		return bResult;
	}
	CTcpTestConnect(void)
		: m_connectReturned(false), m_bDisconnect(true)
		, m_sslctx(NULL)
	{}
	virtual ~CTcpTestConnect(void)
	{
		if (m_ipService.get() != 0)
		{
			m_ipService->stop();
		}
		if (m_tcpClient.get() != 0)
		{
			m_tcpClient->disconnect();
		}
		m_tcpClient.reset();
		m_ipService.reset();
		if (m_sslctx!=0)
		{
			delete m_sslctx;
			m_sslctx = 0;
		}
	}
private:
	///////////////////////////////////////////////
	// for TcpClient_Handler
	virtual void OnConnected(const mycp::asio::TcpClientPointer& tcpClient){m_connectReturned=true;m_bDisconnect=false;}
	virtual void OnConnectError(const mycp::asio::TcpClientPointer& tcpClient, const boost::system::error_code & error){m_connectReturned = true;m_bDisconnect=true;}
	virtual void OnReceiveData(const mycp::asio::TcpClientPointer& tcpClient, const mycp::asio::ReceiveBuffer::pointer& data){}
	virtual void OnDisconnect(const mycp::asio::TcpClientPointer& tcpClient, const boost::system::error_code & error){m_connectReturned = true;m_bDisconnect=true;}

	bool m_connectReturned;
	bool m_bDisconnect;
	mycp::asio::IoService::pointer m_ipService;
	mycp::asio::TcpClient::pointer m_tcpClient;
	boost::asio::ssl::context * m_sslctx;
};
#endif

class CRemoteWaitData
{
public:
	typedef std::shared_ptr<CRemoteWaitData> pointer;
	static CRemoteWaitData::pointer create(void)
	{
		return CRemoteWaitData::pointer(new CRemoteWaitData());
	}

	CLockListPtr<CCommEventData*> m_listMgr;
	CRemoteWaitData(void)
	{
	}
	virtual ~CRemoteWaitData(void)
	{
		m_listMgr.clear();
	}
};

/////////////////////////////////////////
// CTcpServer
class CTcpServer
	: public mycp::asio::TcpConnection_Handler
	, public mycp::asio::IoService_Handler
	, public cgcOnTimerHandler
	, public cgcCommunication
	, public CRemoteHandler
	, public std::enable_shared_from_this<CTcpServer>
{
public:
	typedef std::shared_ptr<CTcpServer> pointer;
	static CTcpServer::pointer create(int nIndex)
	{
		return CTcpServer::pointer(new CTcpServer(nIndex));
	}

private:
	int m_nIndex;
	int m_commPort;
	//int m_capacity;
	int m_protocol;
	bool m_bIsHttp;
	bool m_bIsSsl;
	bool m_bCheckComm;
	int m_nSendTestDataCount;
	int m_nCheckCommSeconds;

#ifdef USES_OPENSSL
	boost::asio::ssl::context* m_sslctx;
	std::string m_sSSLPublicCrtFile;	// public.crt
	std::string m_sSSLServerChainFile;	// server-chain.key
	std::string m_sSSLPrivateKeyFile;	// private.key
#endif
	mycp::asio::IoService::pointer m_ioservice;
	mycp::asio::TcpAcceptor::pointer m_acceptor;
	CLockMap<unsigned long, cgcRemote::pointer> m_mapCgcRemote;

	// 
	CLockListPtr<CCommEventData*> m_listMgr;
	CCommEventDataPool m_pCommEventDataPool;
	int m_nCurrentThread;
	int m_nNullEventDataCount;
	int m_nFindEventDataCount;

#ifdef WIN32
#ifndef USES_CONNECTION_CLOSE_EVENT
	HANDLE m_hDoCloseEvent;
#endif
	HANDLE m_hDoStopServer;
#else
#ifndef USES_CONNECTION_CLOSE_EVENT
	sem_t m_semDoClose;
#endif
	sem_t m_semDoStop;
#endif // WIN32
	//boost::mutex m_mutexRemoteId;
	//unsigned long m_nCurrentRemoteId;
public:
	CTcpServer(int nIndex)
		: m_nIndex(nIndex), m_commPort(0), /*m_capacity(1), */m_protocol(0), m_bIsHttp(false), m_bIsSsl(false), m_bCheckComm(true), m_nSendTestDataCount(0),m_nCheckCommSeconds(20)
		, m_pCommEventDataPool(mycp::asio::Max_ReceiveBuffer_ReceiveSize,30,300)
		, m_nCurrentThread(0), m_nNullEventDataCount(0), m_nFindEventDataCount(0)
#ifdef USES_OPENSSL
		, m_sslctx(NULL)
#endif
		//, m_nLastRemoteId(0)
		//, m_nCurrentRemoteId(0)
	{
#ifdef WIN32
#ifndef USES_CONNECTION_CLOSE_EVENT
		m_hDoCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
		m_hDoStopServer = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
#ifndef USES_CONNECTION_CLOSE_EVENT
		sem_init(&m_semDoClose, 0, 0);
#endif
		sem_init(&m_semDoStop, 0, 0);
#endif // WIN32
		m_tNowTime = time(0);
		m_tProcessReceDataTime = 0;
	}
	virtual ~CTcpServer(void)
	{
		finalService();

#ifdef WIN32
#ifndef USES_CONNECTION_CLOSE_EVENT
		CloseHandle(m_hDoCloseEvent);
#endif
		CloseHandle(m_hDoStopServer);
#else
#ifndef USES_CONNECTION_CLOSE_EVENT
		sem_destroy(&m_semDoClose);
#endif
		sem_destroy(&m_semDoStop);
#endif // WIN32
	}
	//bool verify_certificate(bool preverified,boost::asio::ssl::verify_context& ctx)
	//{
	//	// The verify callback can be used to check whether the certificate that is
	//	// being presented is valid for the peer. For example, RFC 2818 describes
	//	// the steps involved in doing this for HTTPS. Consult the OpenSSL
	//	// documentation for more details. Note that the callback is called once
	//	// for each certificate in the certificate chain, starting from the root
	//	// certificate authority.

	//	// In this example we will simply print the certificate's subject name.
	//	char subject_name[256];
	//	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	//	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	//	//std::cout << "Verifying " << subject_name << "\n";
	//	return true;
	//	return preverified;
	//}
	std::string get_password(void) const
    {
		//printf("**** get_password...\n");
        return theSSLPasswd;
    }
	unsigned long getId(void) const {return (unsigned long)this;}
	virtual tstring serviceName(void) const {return _T("TCPSERVER");}
	virtual bool initService(cgcValueInfo::pointer parameter)
	{
		if (m_commHandler.get() == NULL) return false;
		if (m_bServiceInited) return true;
		if (parameter.get() != NULL && parameter->getType() == cgcValueInfo::TYPE_VECTOR)
		{
			const std::vector<cgcValueInfo::pointer>& lists = parameter->getVector();
			if (lists.size() > 2)
				m_protocol = lists[2]->getInt();
			//if (lists.size() > 1)
			//	m_capacity = lists[1]->getInt();
			if (lists.size() > 0)
				m_commPort = lists[0]->getInt();
			else
				return false;
			m_bIsHttp = (m_protocol & (int)PROTOCOL_HTTP)==PROTOCOL_HTTP;
			m_bIsSsl = (m_protocol & (int)PROTOCOL_SSL)==PROTOCOL_SSL;
		}

		//  handle_handshake session id context uninitialized,336433429
		if (m_ioservice.get() == NULL)
			m_ioservice = mycp::asio::IoService::create();
		if (m_acceptor.get()==NULL)
			m_acceptor = mycp::asio::TcpAcceptor::create();
		//m_acceptor->SetReadSleep(10);
		m_ioservice->start(shared_from_this());
#ifdef USES_OPENSSL
		if (m_bIsSsl)
		{
			mycp::asio::TcpClient::Test_To_SSL_library_init();
			namespace ssl = boost::asio::ssl;
			m_sslctx = new ssl::context(ssl::context::sslv23);
			//m_sslctx = new ssl::context(m_ioservice->ioservice(),ssl::context::sslv23);
			m_sslctx->set_options(ssl::context::default_workarounds|ssl::context::verify_none);	// verify_none
			boost::system::error_code error;
			m_sslctx->set_verify_mode(ssl::verify_none,error);	// verify_none old:ok
			m_sslctx->set_verify_depth(10,error);
			//m_sslctx->set_verify_callback(boost::bind(&CTcpServer::verify_certificate, this, _1, _2));
			if (!theSSLPasswd.empty())
				m_sslctx->set_password_callback(boost::bind(&CTcpServer::get_password, this));
			// XXX.crt OK
			//std::string m_sSSLCAFile = theApplication->getAppConfPath()+"/ssl/ca.crt";
			m_sSSLPublicCrtFile = theApplication->getAppConfPath().c_str();
			m_sSSLPublicCrtFile.append("/ssl/public.crt");
			if (!FileIsExist(m_sSSLPublicCrtFile.c_str()))
			{
				m_bCheckComm = false;
				m_sSSLPublicCrtFile.clear();
			}
			m_sSSLServerChainFile = theApplication->getAppConfPath().c_str();
			m_sSSLServerChainFile.append("/ssl/server-chain.crt");
			if (!FileIsExist(m_sSSLServerChainFile.c_str()))
				m_sSSLServerChainFile.clear();
			else if (!m_sSSLPublicCrtFile.empty())
				m_sSSLServerChainFile = m_sSSLPublicCrtFile;
			m_sSSLPrivateKeyFile = theApplication->getAppConfPath().c_str();
			m_sSSLPrivateKeyFile.append("/ssl/private.key");
			if (!FileIsExist(m_sSSLPrivateKeyFile.c_str()))
			{
				m_bCheckComm = false;
				m_sSSLPrivateKeyFile.clear();
			}

			// 不会报no shared cipher错误
			//EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
			//SSL_CTX_set_tmp_ecdh(m_sslctx->native_handle(), ecdh);
			//EC_KEY_free(ecdh);
			// AES128-SHA
			// HIGH
			//OpenSSL_add_all_algorithms();
			//SSL_CTX_set_cipher_list(m_sslctx->native_handle(),"AES128-SHA");
			//m_sslctx->load_verify_file(m_sSSLCAFile,error);
			//if (error)
			//	CGC_LOG((LOG_ERROR, _T("load_verify_file(%s),error=%s;%d\n"),m_sSSLCAFile.c_str(),error.message().c_str(),error.value()));
			if (!m_sSSLPublicCrtFile.empty())
			{
				m_sslctx->use_certificate_file(m_sSSLPublicCrtFile,ssl::context_base::pem,error);
				if (error)
				{
					m_bCheckComm = false;
					CGC_LOG((LOG_ERROR, _T("use_certificate_file(%s),error=%s;%d\n"),m_sSSLPublicCrtFile.c_str(),error.message().c_str(),error.value()));
				}
			}
			if (!m_sSSLServerChainFile.empty())
			{
				m_sslctx->use_certificate_chain_file(m_sSSLServerChainFile,error);
				if (error)
				{
					m_bCheckComm = false;
					CGC_LOG((LOG_ERROR, _T("use_certificate_chain_file(%s),error=%s;%d\n"),m_sSSLServerChainFile.c_str(),error.message().c_str(),error.value()));
				}
			}
			if (!m_sSSLPrivateKeyFile.empty())
			{
				// 没有设置use_private_key_file()；就会报"handle_handshake no shared cipher"错误
				m_sslctx->use_private_key_file(m_sSSLPrivateKeyFile,ssl::context_base::pem,error);
				if (error)
				{
					m_bCheckComm = false;
					CGC_LOG((LOG_ERROR, _T("use_private_key_file(%s),error=%s;%d\n"),m_sSSLPrivateKeyFile.c_str(),error.message().c_str(),error.value()));
				}
			}
			tstring sTempFile = theApplication->getAppConfPath() + "/ssl";
			m_sslctx->add_verify_path(sTempFile.c_str(),error);
			//m_sslctx->set_default_verify_paths(error);	// 有问题
			//m_sslctx->load_verify_file("/eb/www.entboost.com/www.entboost.com.crt",ignored_error);
			//printf("** load_verify_file,error=%s;%d\n",ignored_error.message().c_str(),ignored_error.value());
			//m_sslctx->add_verify_path("/eb/www.entboost.com",ignored_error);
			//printf("** add_verify_path,error=%s;%d\n",ignored_error.message().c_str(),ignored_error.value());

			//m_sslctx = new ssl::context(m_ioservice->ioservice(),ssl::context::sslv23);
			m_acceptor->set_ssl_ctx(m_sslctx);
		}
#endif
		if (!m_acceptor->start(m_ioservice->ioservice(), m_commPort, shared_from_this()))
		{
			m_acceptor.reset();
			m_ioservice.reset();
			return false;
		}

		m_nCurrentThread = m_bCheckComm?MIN_EVENT_THREAD:1;
		//m_nCurrentThread = MIN_EVENT_THREAD;
		for (int i=1; i<=m_nCurrentThread; i++)
		{
#if (USES_TIMER_HANDLER_POINTER==1)
			theApplication->SetTimer((this->m_nIndex*MAX_EVENT_THREAD)+i, 10, (cgcOnTimerHandler*)this, false, 0, theThreadStackSize);	// 10ms
#else
			theApplication->SetTimer((this->m_nIndex*MAX_EVENT_THREAD)+i, 10, shared_from_this(), false, 0, theThreadStackSize);	// 10ms
#endif
		}

		//m_capacity = m_capacity < 1 ? 1 : m_capacity;
		//CIDEvent * pIDEvent = new CIDEvent();
		//pIDEvent->m_nCurIDEvent = theCurrentIdEvent+1;
		//pIDEvent->m_nCapacity = m_capacity;
		//theAppAttributes->setAttribute(EVENT_ATTRIBUTE, this, cgcObject::pointer(pIDEvent));

		//for (int i=0; i<m_capacity; i++)
		//{
		//	theApplication->SetTimer(++theCurrentIdEvent, m_capacity, shared_from_this());
		//}

		m_bServiceInited = true;
		theApplication->log(LOG_INFO, _T("**** [*:%d] Start succeeded ****\n"), m_commPort);
		return true;
	}

	virtual void finalService(void)
	{
		if (!m_bServiceInited) return;

		for (unsigned int i=this->m_nIndex*MAX_EVENT_THREAD+1; (int)i<=this->m_nIndex*MAX_EVENT_THREAD+m_nCurrentThread; i++)
			theApplication->KillTimer(i);
		//cgcObject::pointer eventPointer = theAppAttributes->removeAttribute(EVENT_ATTRIBUTE, this);
		//CIDEvent * pIDEvent = (CIDEvent*)eventPointer.get();
		//if (pIDEvent != NULL)
		//{
		//	for (unsigned int i=pIDEvent->m_nCurIDEvent; i<pIDEvent->m_nCurIDEvent+pIDEvent->m_nCapacity; i++)
		//		theApplication->KillTimer(i);
		//}

#ifdef WIN32
		if (m_hDoStopServer)
		{
			SetEvent(m_hDoStopServer);
		}
#else
		sem_post(&m_semDoStop);
#endif
		m_listMgr.clear();
//#ifndef USES_SUPPORT_HTTP_CONNECTION
		theCloseList.clear();
//#endif
		m_pRecvRemoteIdWaitList.clear();
		m_pCommEventDataPool.Clear();
		m_mapCgcRemote.clear();
		m_acceptor.reset();
		m_ioservice.reset();
#ifdef USES_OPENSSL
		if (m_sslctx)
		{
			delete m_sslctx;
			m_sslctx = NULL;
		}
#endif
		m_bServiceInited = false;
		theApplication->log(LOG_INFO, _T("**** [%s:%d] Stop succeeded ****\n"), serviceName().c_str(), m_commPort);
	}
	
	CLockMap<unsigned long,CRemoteWaitData::pointer> m_pRecvRemoteIdWaitList;
	CLockMap<unsigned long,bool> m_pRecvRemoteIdList;	// *记录正在处理接收数据remoteid（包括普通TCP和HTTP等）
	//unsigned long m_nLastRemoteId;
protected:
	// cgcOnTimerHandler
	time_t m_tNowTime;
	time_t m_tProcessReceDataTime;
#ifdef USES_TCP_TEST_CONNECT
	void ProcCommTest(size_t nSize)
	{
		// 不检查重启，或者已经超过配置重启次数
		if (!m_bCheckComm || theCheckRestartCount>=theLimitRestartCount) return;

		static unsigned int theIndex = 0;
		theIndex++;
		if ((theIndex%1000)!=999) return;	// 10 second
		const time_t tNowTime = time(0);
		static time_t theLastCheckTime = 0;
		if (nSize>0) {
			//m_bCheckComm = false;	// **检查一次，有数据就退出
			theLastCheckTime = tNowTime;	// ** 有数据记下最新时间，延长下次5分钟时间
		}
		else if (theLastCheckTime==0 || (tNowTime-theLastCheckTime)>m_nCheckCommSeconds) {	// 开始检查一次，后期每5分钟检查一次
		//else if (theLastCheckTime==0 || (tNowTime-theLastCheckTime)>20) {	// 开始检查一次，后期每20秒检查一次
			theLastCheckTime = tNowTime;
			if ((++m_nSendTestDataCount)>1) {
				m_nCheckCommSeconds = 20;	// ** 第二次，开始设为20秒后继续测试
			}
			//CGC_LOG((LOG_INFO, _T("**** TestConnect... m_nSendTestDataCount=%d,m_nCheckCommSeconds=%d ****\n"),m_nSendTestDataCount,m_nCheckCommSeconds));
			CTcpTestConnect::pointer pTestConnect = CTcpTestConnect::create();
			pTestConnect->TestConnect("127.0.0.1",this->m_commPort,m_bIsSsl);	// 
			//if (!pTestConnect->TestConnect(sLocalIp.c_str(),this->m_commPort,m_bIsSsl))
			//if (!pTestConnect->TestConnect("127.0.0.1",this->m_commPort,m_bIsSsl))
			{
				//CGC_LOG((LOG_ERROR, _T("**** TestConnect(%d) Error ssl=%d ****\n"),m_commPort,(int)(m_bIsSsl?1:0)));
				if (m_nSendTestDataCount>2) {	// 123 ，总共测试三次
					m_nSendTestDataCount = 0;
				//if ((++theLocalIpTestCount)>=2){
					//theLocalIpTestCount = 0;
					CGC_LOG((LOG_ERROR, _T("**** TestConnect(%d) OnIoServiceException Restart ****\n"),m_commPort));
					if (theLimitRestartCount>0) {
						theCheckRestartCount++;
					}
					OnIoServiceException();
				}
				//}else
				//{
				//	CGC_LOG((LOG_INFO, _T("**** TestConnect(%s:%d) OK ssl=%d ****\n"),sLocalIp.c_str(),m_commPort,(int)(m_bIsSsl?1:0)));
				//	// * 发送数据成功，对方会自动设为 false
				//	//m_bCheckComm = false;			// ** 检查成功，直接退出，只检查一次
			}
			pTestConnect.reset();
		}
	}
#endif
	virtual void OnTimeout(unsigned int nIDEvent, const void * pvParam)
	{
		if (m_commHandler.get() == NULL) return;
		if ((int)nIDEvent==(this->m_nIndex*MAX_EVENT_THREAD)+MAIN_MGR_EVENT_ID)	// 这是管理线程；
		{
			const size_t nSize = m_listMgr.size();
			const bool bCreateNewThread = (nSize>0 && m_tProcessReceDataTime>0 && (time(0)-m_tProcessReceDataTime)>1)?true:false;
			if (bCreateNewThread || (int)nSize>(m_nCurrentThread+20)) {
				if (m_nCurrentThread)
					m_tProcessReceDataTime = 0;
				m_nNullEventDataCount = 0;
				m_nFindEventDataCount++;
				if (bCreateNewThread || 
					(m_nCurrentThread<MAX_EVENT_THREAD && ((int)nSize>(MAX_EVENT_THREAD*2) || ((int)nSize>(m_nCurrentThread*2)&&m_nFindEventDataCount>20) || m_nFindEventDataCount>100)))	// 100*10ms=1S
				{
					m_nFindEventDataCount = 0;
					const unsigned int nNewTimerId = (this->m_nIndex*MAX_EVENT_THREAD)+(++m_nCurrentThread);
					CGC_LOG((LOG_INFO, _T("**** TCPServer:NewTimerId=%d size=%d ****\n"),nNewTimerId,nSize));
#if (USES_TIMER_HANDLER_POINTER==1)
					theApplication->SetTimer(nNewTimerId, 10, (cgcOnTimerHandler*)this, false, 0, theThreadStackSize);	// 10ms
#else
					theApplication->SetTimer(nNewTimerId, 10, shared_from_this(), false, 0, theThreadStackSize);	// 10ms
#endif
				}
			}
			else {
				m_nFindEventDataCount = 0;
				m_nNullEventDataCount++;
				if (m_nCurrentThread>MIN_EVENT_THREAD &&
					((m_nCurrentThread>20&&(int)nSize<(m_nCurrentThread/8)&&m_nNullEventDataCount>30) ||
					((int)nSize<(m_nCurrentThread/3)&&m_nNullEventDataCount>500) ||
					m_nNullEventDataCount>600))	// 300*10ms=3S
				{
					m_nNullEventDataCount = 0;
					const unsigned int nKillTimerId = (this->m_nIndex*MAX_EVENT_THREAD)+m_nCurrentThread;
					CGC_LOG((LOG_INFO, _T("**** TCPServer:KillTimerId=%d ****\n"),nKillTimerId));
					theApplication->KillTimer(nKillTimerId);
					m_nCurrentThread--;
				}

#ifdef USES_TCP_TEST_CONNECT
				ProcCommTest(nSize);
#endif
			}

			static unsigned int theIndex = 0;
			theIndex++;
			if ((theIndex%400)==399) {	// 3 second
				m_tNowTime = time(0);
				//#ifndef USES_SUPPORT_HTTP_CONNECTION
				CCloseConnectionInfo::pointer pCloseInfo;
				CCloseConnectionInfo::pointer pFirstErrorCloseInfo;
				while (theCloseList.front(pCloseInfo)) {
					if (pCloseInfo->CheckClose(m_tNowTime)) {
						continue;
					}
					if (theCloseList.empty()) {									// * last
						theCloseList.add(pCloseInfo);
						break;
					}
					if (pFirstErrorCloseInfo.get()==NULL) {						// * first
						pFirstErrorCloseInfo = pCloseInfo;
						theCloseList.add(pCloseInfo);
					}
					else if (pFirstErrorCloseInfo.get()==pCloseInfo.get()) {	// * first again
						theCloseList.add(pCloseInfo);
						break;
					}
				}
				//#endif
			}

			return;
		}

		CCommEventData * pCommEventData = m_listMgr.front();
		if (pCommEventData == NULL)
		{
			if ((int)nIDEvent==(this->m_nIndex*MAX_EVENT_THREAD)+MAIN_MGR_EVENT_ID+1)
			{
				m_pCommEventDataPool.Idle();
			}
			m_tProcessReceDataTime = 0;
			return;
#ifdef USES_TCP_TEST_CONNECT
		}else if (m_bCheckComm)
		{
			if (m_nSendTestDataCount>0) {
				m_nSendTestDataCount = 0;
			}
			if (m_nCheckCommSeconds<300) {
				m_nCheckCommSeconds = 300;	// **5分钟
			}
			//m_bCheckComm = false;	// ** 检查一次，有数据就退出
#endif
		}

		m_tProcessReceDataTime = time(0);
		switch (pCommEventData->getCommEventType())
		{
		case CCommEventData::CET_Accept:
			m_commHandler->onRemoteAccept(pCommEventData->getRemote());
			break;
		case CCommEventData::CET_Close:
			{
				for (int i=0;i<50;i++) {
					CRemoteWaitData::pointer pHttpRemoteWaitData;
					if (m_bIsHttp && m_pRecvRemoteIdWaitList.find(pCommEventData->getRemoteId(),pHttpRemoteWaitData) && !pHttpRemoteWaitData->m_listMgr.empty()) {
#ifdef WIN32
						Sleep(100);
#else
						usleep(100000);
#endif // WIN32
						continue;
					}

					if (!m_pRecvRemoteIdList.exist(pCommEventData->getRemoteId())) {
						break;
					}
#ifdef WIN32
					Sleep(100);
#else
					usleep(100000);
#endif // WIN32
				}
				if (m_bIsHttp) {
					m_pRecvRemoteIdWaitList.remove(pCommEventData->getRemoteId());
				}

				//printf("**** CCommEventData::CET_Close: remoteid=%d\n",pCommEventData->getRemoteId());
				//printf("**** CommTcpServer CET_Close, 111\n");
				m_commHandler->onRemoteClose(pCommEventData->getRemoteId(),pCommEventData->GetErrorCode());
				cgcRemote::pointer pCgcRemote = pCommEventData->getRemote();
				pCgcRemote->invalidate(true);
				//pCommEventData->getRemote()->invalidate(true);
//				printf("**** CommTcpServer CET_Close, 222\n");
#ifdef USES_CONNECTION_CLOSE_EVENT
				((CcgcRemote*)pCgcRemote.get())->SetCloseEvent();
				pCgcRemote.reset();
				//((CcgcRemote*)pCommEventData->getRemote().get())->SetCloseEvent();
#else
#ifdef WIN32
				SetEvent(m_hDoCloseEvent);
#else
				sem_post(&m_semDoClose);
#endif // WIN32
#endif
				//printf("**** CommTcpServer CET_Close, end\n");
			}break;
		case CCommEventData::CET_Recv:
			{
				const unsigned long nRemoteId = pCommEventData->getRemoteId();
				if (!m_pRecvRemoteIdList.insert(nRemoteId,true,false) && m_bIsHttp) {	// *HTTP失败（前面已经在处理）返回，其他失败不返回
					//m_pCommEventDataPool.Set(pCommEventData);	// *** 不能放进去
					return;
				}
				CRemoteWaitData::pointer pHttpRemoteWaitData;
				if (m_bIsHttp) {
					if (!m_pRecvRemoteIdWaitList.find(nRemoteId,pHttpRemoteWaitData)) {
						m_pCommEventDataPool.Set(pCommEventData);
						m_pRecvRemoteIdList.remove(nRemoteId);
						return;
					}
					// ** 取第一个，下面会 while 处理所有数据
					pCommEventData = pHttpRemoteWaitData->m_listMgr.front();
				}
				while (pCommEventData != NULL) {
#ifdef USES_PRINT_DEBUG
					static FILE * f = NULL;
					if (f==NULL)
						f = fopen("c:\\http_on_data.txt","w");
					if (f!=NULL) {
						char lpszBuf[100];
						sprintf(lpszBuf,"**** index=%d, size=%d\r\n",pCommEventData->GetErrorCode(),pCommEventData->getRecvSize());
						fwrite(lpszBuf,1,strlen(lpszBuf),f);
						fflush(f);
					}
#endif
					//cgcRemote::pointer pCgcRemote = pCommEventData->getRemote();
					//if (pCgcRemote->isInvalidate())
					//{
					//	//printf("******** OnRemoteRecv:isInvalidate().UpdateConnection\n");
					//	((CcgcRemote*)pCgcRemote.get())->UpdateConnection(pRemote);
					//}
					m_commHandler->onRecvData(pCommEventData->getRemote(), pCommEventData->getRecvData(), pCommEventData->getRecvSize());
					m_pCommEventDataPool.Set(pCommEventData);
					pCommEventData = pHttpRemoteWaitData.get()==NULL?NULL:pHttpRemoteWaitData->m_listMgr.front();
				}
				m_pRecvRemoteIdList.remove(nRemoteId);
				if (m_bIsHttp) {
					return;	// *** HTTP 需要返回
				}
				//m_nLastRemoteId = 0;
			}break;
		case CCommEventData::CET_Exception:
			{
				//if (!m_listMgr.empty())
				{
					AUTO_WLOCK(m_listMgr);
					CLockListPtr<CCommEventData*>::iterator pIter = m_listMgr.begin();
					for (; pIter!=m_listMgr.end(); pIter++)
					{
						CCommEventData * pCloseCommEventData = m_listMgr.front();
						m_commHandler->onRemoteClose(pCloseCommEventData->getRemoteId(),pCloseCommEventData->GetErrorCode());
					}
					m_listMgr.clear(false,true);
				}
				m_mapCgcRemote.clear();
				const bool bFirstStart = m_ioservice.get()==NULL?true:false;
				if (m_ioservice.get()!=NULL)
				{
					m_ioservice->stop();
					m_ioservice.reset();
				}
				if (m_acceptor.get()!=NULL)
				{
					m_acceptor->stop();
					m_acceptor.reset();
				}
#ifdef USES_OPENSSL
				if (this->m_sslctx!=NULL)
				{
					delete m_sslctx;
					m_sslctx = NULL;
				}
#endif
				if (!theApplication->isInited())
					break;
				if (!bFirstStart)
				{
					CGC_LOG((LOG_ERROR, _T("[*:%d] tcp server restart\n"),m_commPort));
#ifdef WIN32
					Sleep(1000);
#else
					sleep(1);
#endif
				}
				m_acceptor = mycp::asio::TcpAcceptor::create();
				m_ioservice = mycp::asio::IoService::create();
				m_ioservice->start(shared_from_this());
#ifdef USES_OPENSSL
				if (m_bIsSsl)
				{
					mycp::asio::TcpClient::Test_To_SSL_library_init();
					namespace ssl = boost::asio::ssl;
					m_sslctx = new ssl::context(ssl::context::sslv23);
					//m_sslctx = new ssl::context(m_ioservice->ioservice(),ssl::context::sslv23);
					m_sslctx->set_options(ssl::context::default_workarounds|ssl::context::verify_none);
					boost::system::error_code error;
					m_sslctx->set_verify_mode(ssl::verify_none,error);
					m_sslctx->set_verify_depth(10,error);
					if (!theSSLPasswd.empty())
						m_sslctx->set_password_callback(boost::bind(&CTcpServer::get_password, this));
					if (!FileIsExist(m_sSSLPublicCrtFile.c_str())) {
						m_bCheckComm = false;
						m_sSSLPublicCrtFile.clear();
					}
					else {
						m_sslctx->use_certificate_file(m_sSSLPublicCrtFile,ssl::context_base::pem,error);
						//m_sslctx->use_certificate_chain_file(m_sSSLPublicCrtFile,error);
					}
					if (!FileIsExist(m_sSSLServerChainFile.c_str())) {
						m_bCheckComm = false;
						m_sSSLServerChainFile.clear();
					}
					else {
						m_sslctx->use_certificate_chain_file(m_sSSLServerChainFile,error);
					}
					if (!FileIsExist(m_sSSLPrivateKeyFile.c_str())) {
						m_bCheckComm = false;
						m_sSSLPrivateKeyFile.clear();
					}
					else {
						m_sslctx->use_private_key_file(m_sSSLPrivateKeyFile,ssl::context_base::pem,error);
					}
					const tstring sTempFile = theApplication->getAppConfPath() + "/ssl";
					m_sslctx->add_verify_path(sTempFile.c_str(),error);
					m_acceptor->set_ssl_ctx(m_sslctx);
				}
#endif
				m_acceptor->start(m_ioservice->ioservice(), m_commPort, shared_from_this());
			}break;
		default:
			break;
		}
		m_pCommEventDataPool.Set(pCommEventData);
		//delete pCommEventData;
	}

//#ifndef USES_SUPPORT_HTTP_CONNECTION
	class CCloseConnectionInfo
	{
	public:
		typedef std::shared_ptr<CCloseConnectionInfo> pointer;
		static CCloseConnectionInfo::pointer create(const mycp::asio::TcpConnectionPointer& pConnection, int nWaitSecond = 5, bool bCloseSocket=false)
		{
			return CCloseConnectionInfo::pointer(new CCloseConnectionInfo(pConnection, nWaitSecond, bCloseSocket));
		}

		bool CheckClose(time_t tNow)
		{
			if (m_pCloseConnection.get()==NULL) return true;
			if (tNow>m_tCloseTime || (tNow+600)<m_tCloseTime)
			{
				//theAcceptRemoteCount--;
				//printf("**** CheckClose CloseConnection, theAcceptRemoteCount=%d (%x)\n",theAcceptRemoteCount,(int)m_pCloseConnection.get());
				if (m_bCloseSocket)
					m_pCloseConnection->close();
#ifndef USES_SUPPORT_HTTP_CONNECTION
				m_pCloseConnection->close();
#endif
				m_pCloseConnection.reset();
				return true;
			}
			return false;
		}
		CCloseConnectionInfo(const mycp::asio::TcpConnectionPointer& pConnection, int nWaitSecond, bool bCloseSocket)
			: m_pCloseConnection(pConnection)
			, m_bCloseSocket(bCloseSocket)
		{
			m_tCloseTime = time(0)+nWaitSecond;
		}
		CCloseConnectionInfo(void)
			: m_tCloseTime(0)
			, m_bCloseSocket(false)
		{}
		virtual ~CCloseConnectionInfo(void)
		{
			CheckClose(0);
		}

	private:
		mycp::asio::TcpConnectionPointer m_pCloseConnection;
		time_t m_tCloseTime;
		bool m_bCloseSocket;
	};
	// CRemoteHandler
	CLockList<CCloseConnectionInfo::pointer> theCloseList;
	virtual void onCloseConnection(const mycp::asio::TcpConnectionPointer& connection, int nWaitSecond, bool bCloseSocket)
	{
		if (connection.get()!=NULL)
			theCloseList.add(CCloseConnectionInfo::create(connection, nWaitSecond, bCloseSocket));
	}
//#endif

	// IoService_Handler
	virtual void OnIoServiceException(void)
	{
		CCommEventData * pEventData = new CCommEventData(CCommEventData::CET_Exception);
		m_listMgr.add(pEventData);
	}

//#define USES_DEBUG_RECV_LOG

	// TcpConnection_Handler
	virtual bool OnRemoteRecv(const mycp::asio::TcpConnectionPointer& pRemote, const mycp::asio::ReceiveBuffer::pointer& data)
	{
		BOOST_ASSERT(pRemote.get() != 0);
		if (data->size() == 0 || pRemote.get() == 0) return true;
		if (m_commHandler.get() == NULL) return false;

		const unsigned long nRemoteId = pRemote->getId();
#ifdef USES_DEBUG_RECV_LOG
		printf("******** OnRemoteRecv:%lu size=%d\n",nRemoteId,data->size());
#endif
		//CGC_LOG((LOG_INFO, _T("OnRemoteRecv, %x RemoteId=%d, size=%d **********\n"), (int)pRemote.get(), (int)nRemoteId,(int)data->size() ));
		cgcRemote::pointer pCgcRemote;
		if (!m_mapCgcRemote.find(nRemoteId, pCgcRemote))
		{
#ifdef USES_DEBUG_RECV_LOG
			printf("******** OnRemoteRecv:%lu size=%d, m_mapCgcRemote.find not exist return.\n",nRemoteId,data->size());
#endif
			return false;
		}
		// 
		CcgcRemote* pRemoteTemp = (CcgcRemote*)pCgcRemote.get();
		if (pRemoteTemp->getIsInvalidate())	// **判断业务是否已经关闭，不需要判断 TcpConnectionPointer socket 连接状态；
			//if (pCgcRemote->isInvalidate())		// ** 这个会判断 TcpConnectionPointer socket 连接状态；
		{
			if (m_bIsHttp)
			{
#ifdef USES_DEBUG_RECV_LOG
				printf("******** OnRemoteRecv:%lu size=%d, m_bIsHttp close \n",nRemoteId,data->size());
#endif
				// *** 业务关闭连接，HTTP，直接返回并关闭；
				onCloseConnection(pRemote,2,true);
				//pRemote->close();
				//m_mapCgcRemote.remove(nRemoteId);	// add by 2017-06-12
#ifdef USES_DEBUG_RECV_LOG
				printf("******** OnRemoteRecv:%lu size=%d, m_bIsHttp close,ok**** \n",nRemoteId,data->size());
#endif
				return false;
				//if (this->m_tNowTime>(((CcgcRemote*)pCgcRemote.get())->GetCloseTime()+2))
				//{
				//	printf("******* set close flag\n");
				//	pRemote->close();
				//	return false;
				//}
				//theCloseList.add(CCloseConnectionInfo::create(pRemote, 2, true));
				//return true;	// **HTTP不处理，直接返回
			}
			//printf("******** OnRemoteRecv:isInvalidate().UpdateConnection\n");
			pRemoteTemp->UpdateConnection(pRemote);
		}
		//CCommEventData * pEventData = new CCommEventData(CCommEventData::CET_Recv);
		CCommEventData * pEventData = m_pCommEventDataPool.Get();
		pEventData->setCommEventType(CCommEventData::CET_Recv);
#ifdef USES_DEBUG_RECV_LOG
		printf("******** OnRemoteRecv:%lu size=%d, setRemote...\n",nRemoteId,data->size());
#endif
		pEventData->setRemote(pCgcRemote);
#ifdef USES_DEBUG_RECV_LOG
		printf("******** OnRemoteRecv:%lu size=%d, setRemote ok****\n",nRemoteId,data->size());
#endif
		// 这里就异常了
		pEventData->setRemoteId(nRemoteId);
		//pEventData->setRemoteId(pCgcRemote->getRemoteId());
#ifdef USES_PROTOCOL_HSOTP
		if (m_protocol & (int)PROTOCOL_HSOTP)
		{
			//printf("****recv****\n%s\n",data->data());
			const cgcParserHttp::pointer& pParserHttp = ((CcgcRemote*)pCgcRemote.get())->GetParserhttp();
			if (pParserHttp.get()!=NULL)
			{
				if (((CcgcRemote*)pCgcRemote.get())->m_nRequestSize == data->size())
				{
					// 之前http post错误的真正内容
					//printf("****sotp****\n%s\n",data->data());
					((CcgcRemote*)pCgcRemote.get())->m_nRequestSize = 0;
					pEventData->setRecvData(data->data(), data->size());
					//}else if (m_nRequestSize==data->size()+((CcgcRemote*)pCgcRemote.get())->m_sPrevData.size())
					//{
					//	std::string& sPrevData = ((CcgcRemote*)pCgcRemote.get())->m_sPrevData;
					//	sPrevData.append((const char*)data->data(), data->size());
					//	pEventData->setRecvData(sPrevData.c_str(),sPrevData.size());
					//	sPrevData.clear();
				}else
				{
					bool parseResult = pParserHttp->doParse(data->data(), data->size());
					if (parseResult)
					{
						//printf("****sotp****%d\n%s\n",(int)pParserHttp->getHttpMethod(),pParserHttp->getContentData());
						switch(pParserHttp->getHttpMethod())
						{
						case HTTP_OPTIONS:
							{
								pCgcRemote->sendData((const unsigned char*)"",0);
							}break;
						case HTTP_POST:
							{
								pEventData->setRecvData((const unsigned char*)pParserHttp->getContentData(), pParserHttp->getContentLength());
							}break;
						default:
							break;
						}
					}else
					{
						((CcgcRemote*)pCgcRemote.get())->m_nRequestSize = pParserHttp->getContentLength();
					}
				}
			}
		}else
#endif
		{
#ifdef USES_DEBUG_RECV_LOG
			printf("******** OnRemoteRecv:%lu size=%d, setRecvData...\n",nRemoteId,data->size());
#endif
			pEventData->setRecvData(data->data(), data->size());
#ifdef USES_DEBUG_RECV_LOG
			printf("******** OnRemoteRecv:%lu size=%d, setRecvData ok****\n",nRemoteId,data->size());
#endif
		}
		if (m_bIsHttp)
		{
			CRemoteWaitData::pointer pHttpRemoteWaitData;
			if (!m_pRecvRemoteIdWaitList.find(nRemoteId,pHttpRemoteWaitData))
			{
#ifdef USES_DEBUG_RECV_LOG
				printf("******** OnRemoteRecv:%lu size=%d, m_pRecvRemoteIdWaitList.find not exist, ***\n",nRemoteId,data->size());
#endif
				m_pCommEventDataPool.Set(pEventData);
				return false;
				//return true;
			}
#ifdef USES_DEBUG_RECV_LOG
			printf("******** OnRemoteRecv:%lu size=%d, pHttpRemoteWaitData->m_listMgr.add...\n",nRemoteId,data->size());
#endif
			pHttpRemoteWaitData->m_listMgr.add(pEventData);
#ifdef USES_DEBUG_RECV_LOG
			printf("******** OnRemoteRecv:%lu size=%d, pHttpRemoteWaitData->m_listMgr.add ok ****\n",nRemoteId,data->size());
#endif
		}

		//printf("******** m_listMgr.add:%d\n",nRemoteId);
#ifdef USES_PRINT_DEBUG
		static int theIndex = 0;
		pEventData->SetErrorCode(theIndex++);
		static FILE * f = NULL;
		if (f==NULL)
			f = fopen("c:\\http_remote_recv.txt","w");
		if (f!=NULL)
		{
			char lpszBuf[100];
			sprintf(lpszBuf,"**** index=%d, size=%d remoteid=%d\r\n",pEventData->GetErrorCode(),data->size(), nRemoteId);
			fwrite(lpszBuf,1,strlen(lpszBuf),f);
			fflush(f);
		}
#endif
#ifdef USES_DEBUG_RECV_LOG
		printf("******** OnRemoteRecv:%lu size=%d, m_listMgr.add...\n",nRemoteId,data->size());
#endif
		m_listMgr.add(pEventData);
#ifdef USES_DEBUG_RECV_LOG
		printf("******** OnRemoteRecv:%lu size=%d, m_listMgr.add ok**** end\n",nRemoteId,data->size());
#endif
		//CGC_LOG((LOG_INFO, _T("OnRemoteRecv, %x RemoteId=%d, size=%d ********** ok\n"), (int)pRemote.get(), (int)nRemoteId,(int)data->size() ));
		return true;
	}
	//unsigned long GetNextRemoteId(void)
	//{
	//	boost::mutex::scoped_lock lock(m_mutexRemoteId);
	//	return (++m_nCurrentRemoteId)==0?1:m_nCurrentRemoteId;
	//}
	virtual void OnRemoteAccept(const mycp::asio::TcpConnectionPointer& pRemote)
	{
		//BOOST_ASSERT(pRemote.get() != 0);
		//if (pRemote.get()==NULL) return;
		if (m_commHandler.get() != NULL)
		{
			const unsigned long nRemoteId = pRemote->getId();
			//CGC_LOG((LOG_INFO, _T("OnRemoteAccept, %x RemoteId=%d, m_mapCgcRemote.size=%d,m_pRecvRemoteIdWaitList.size=%d\n"),
			//	(int)pRemote.get(), (int)nRemoteId,(int)m_mapCgcRemote.size(),(int)m_pRecvRemoteIdWaitList.size() ));

			if (m_bIsHttp)
			{
				m_pRecvRemoteIdWaitList.insert(nRemoteId,CRemoteWaitData::create(),false);
			}
#ifdef USES_DEBUG_REMOTE_COUNT
			theAcceptRemoteCount++;
			printf("******** OnRemoteAccept theAcceptRemoteCount=%d\n",theAcceptRemoteCount);
#endif

			//printf("******** OnRemoteAccept:%d\n",nRemoteId);
			cgcRemote::pointer pCgcRemote;
			if (m_mapCgcRemote.find(nRemoteId, pCgcRemote))
			{
				// ?
				//((CcgcRemote*)pCgcRemote.get())->SetServerPort(m_commPort);
				((CcgcRemote*)pCgcRemote.get())->UpdateConnection(pRemote);
			}else
			{
				pCgcRemote = cgcRemote::pointer(new CcgcRemote((CRemoteHandler*)this,pRemote,getId(),nRemoteId,m_protocol));
				((CcgcRemote*)pCgcRemote.get())->SetServerPort(m_commPort);
#ifdef USES_PROTOCOL_HSOTP
				if (m_protocol & (int)PROTOCOL_HSOTP)
				{
					((CcgcRemote*)pCgcRemote.get())->SetParserHttp(cgcParserHttp::pointer(new CPpHttp()));
				}
#endif
				m_mapCgcRemote.insert(nRemoteId, pCgcRemote);
			}
			//CCommEventData * pEventData = new CCommEventData(CCommEventData::CET_Accept);
			CCommEventData * pEventData = m_pCommEventDataPool.Get();
			pEventData->setCommEventType(CCommEventData::CET_Accept);
			pEventData->setRemote(pCgcRemote);
			pEventData->setRemoteId(nRemoteId);
			//pEventData->setRemoteId(pCgcRemote->getRemoteId());
			m_listMgr.add(pEventData);
		}
	}
	virtual void OnRemoteClose(const mycp::asio::TcpConnection* pRemote,int nErrorCode)
	{
		BOOST_ASSERT(pRemote != 0);
		if (m_commHandler.get() != NULL)
		{
#ifdef USES_DEBUG_REMOTE_COUNT
			theAcceptRemoteCount--;
			printf("******** OnRemoteClose theAcceptRemoteCount=%d\n",theAcceptRemoteCount);
#endif

			// 9=Bad file descriptor
			const unsigned long nRemoteId = pRemote->getId();
			//CGC_LOG((LOG_INFO, _T("OnRemoteClose, %x RemoteId=%d, m_mapCgcRemote.size=%d,m_pRecvRemoteIdWaitList.size=%d\n"),
			//	(int)pRemote, (int)nRemoteId,(int)m_mapCgcRemote.size(),(int)m_pRecvRemoteIdWaitList.size() ));

			//printf("******** OnRemoteClose:%lu\n",nRemoteId);
			cgcRemote::pointer pCgcRemote;
			if (!m_mapCgcRemote.find(nRemoteId, pCgcRemote, true))
				return;

			//CCommEventData * pEventData = new CCommEventData(CCommEventData::CET_Close,nErrorCode);
			CCommEventData * pEventData = m_pCommEventDataPool.Get();
			pEventData->setCommEventType(CCommEventData::CET_Close);
			pEventData->SetErrorCode(nErrorCode);
			pEventData->setRemote(pCgcRemote);
			pEventData->setRemoteId(nRemoteId);
			//pEventData->setRemoteId(pCgcRemote->getRemoteId());
			m_listMgr.add(pEventData);
			// *** 需要做下面，可以让前面数据正常返回
			//return;
			// Do CET_Close Event, or StopServer
#ifdef USES_CONNECTION_CLOSE_EVENT
			((CcgcRemote*)pCgcRemote.get())->WaitForCloseEvent();
			//CGC_LOG((LOG_INFO, _T("OnRemoteClose, %x RemoteId=%d ok=============\n"), (int)pRemote, (int)nRemoteId ));
#else
#ifdef WIN32
			WaitForSingleObject(m_hDoCloseEvent, 3000);
			//HANDLE hObject[2];
			//hObject[0] = m_hDoStopServer;
			//hObject[1] = m_hDoCloseEvent;
			//WaitForMultipleObjects(2, hObject, FALSE, 3000);
			//WaitForMultipleObjects(2, hObject, FALSE, INFINITE);
#else
			//sem_wait(&m_semDoClose);
			//while (true)
			{
				struct timespec timeSpec;
				clock_gettime(CLOCK_REALTIME, &timeSpec);
				timeSpec.tv_sec += 3;

				sem_timedwait(&m_semDoClose, &timeSpec);
				//int s = sem_timedwait(&m_semDoClose, &timeSpec);
				//if (s == -1 || errno == EINTR)
				////if (s == -1 && errno == EINTR)
				//	break;

				//printf("1: sem_timedwait s=%d\n", s);
				//timeSpec.tv_sec += 1;
				//s = sem_timedwait(&m_semDoStop, &timeSpec);
				//if (s == -1 || errno == EINTR)
				////if (s == -1 && errno == EINTR)
				//	break;
				//printf("2: sem_timedwait s=%d\n", s);
			}
			//printf("***** OnRemoteClose OK\n");
#endif // WIN32
#endif
		}
	}
};

cgcParameterMap::pointer theAppInitParameters;

extern "C" bool CGC_API CGC_Module_Init2(MODULE_INIT_TYPE nInitType)
//extern "C" bool CGC_API CGC_Module_Init(void)
{
	if (theAppAttributes.get() != NULL) {
		CGC_LOG((mycp::LOG_ERROR, "CGC_Module_Init2 rerun error, InitType=%d.\n", nInitType));
		return true;
	}
#ifdef WIN32
	WSADATA wsaData;
	int err = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	if ( err != 0 ) {
		theApplication->log(LOG_ERROR, _T("WSAStartup' error!\n"));
		return false;
	}
#endif // WIN32

	theAppInitParameters = theApplication->getInitParameters();
	theSSLPasswd = theAppInitParameters->getParameterValue("ssl-passwd", "");
	theLimitRestartCount = theAppInitParameters->getParameterValue("limit-restart-count", (int)0);
	//printf("**** %s\n",theSSLPasswd.c_str());
	theThreadStackSize = theAppInitParameters->getParameterValue("thread-stack-size", (int)256);	/// 1024?
	theApplication->log(LOG_INFO, _T("**** thread-stack-size=%dKB\n"), theThreadStackSize);

	theAppAttributes = theApplication->getAttributes(true);
	assert (theAppAttributes.get() != NULL);
	return true;
}

extern "C" void CGC_API CGC_Module_Free2(MODULE_FREE_TYPE nFreeType)
//extern "C" void CGC_API CGC_Module_Free(void)
{
	VoidObjectMapPointer mapLogServices = theAppAttributes->getVoidAttributes(ATTRIBUTE_NAME, false);
	if (mapLogServices.get() != NULL)
	{
		CObjectMap<void*>::iterator iter;
		for (iter=mapLogServices->begin(); iter!=mapLogServices->end(); iter++)
		{
			CTcpServer::pointer commServer = CGC_OBJECT_CAST<CTcpServer>(iter->second);
			if (commServer.get() != NULL)
			{
				commServer->finalService();
			}
		}
	}

	theAppInitParameters.reset();
	theAppAttributes->clearAllAtrributes();
	theAppAttributes.reset();
	theApplication->KillAllTimer();
#ifdef WIN32
	WSACleanup();
#endif // WIN32
}

int theServiceIndex = 0;
extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	CTcpServer::pointer commServer = CTcpServer::create(theServiceIndex++);
	outService = commServer;
	theAppAttributes->setAttribute(ATTRIBUTE_NAME, outService.get(), commServer);
}

extern "C" void CGC_API CGC_ResetService(const cgcServiceInterface::pointer & inService)
{
	if (inService.get() == NULL) return;
	theAppAttributes->removeAttribute(ATTRIBUTE_NAME, inService.get());
	inService->finalService();
}
