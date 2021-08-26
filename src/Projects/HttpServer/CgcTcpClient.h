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

// CgcTcpClient.h file here
#ifndef __mycp_asio_CgcTcpClient_h__
#define __mycp_asio_CgcTcpClient_h__

#ifndef USES_OPENSSL
#define USES_OPENSSL
#endif
#define USES_MYCP_TCPCLIENT

// include
#include <CGCBase/cgcSmartString.h>
#include <CGCBase/cgcaddress.h>
#include <ThirdParty/Boost/asio/IoService.h>
#include <ThirdParty/Boost/asio/boost_socket.h>
#ifdef USES_MYCP_TCPCLIENT
#include <ThirdParty/Boost/asio/TcpClient.h>
using namespace mycp::asio;
#else
#include "TcpClient.h"
#endif

namespace mycp {
namespace httpserver {

class TcpClient_Callback
{
public:
	typedef std::shared_ptr<TcpClient_Callback> pointer;
	virtual void OnReceiveData(const mycp::asio::ReceiveBuffer::pointer& data, int nUserData) = 0;
	virtual void OnConnected(int nUserData){}
	virtual void OnDisconnect(int nUserData){}
};

class CgcTcpClient
	: public TcpClient_Handler
	, public mycp::asio::IoService_Handler
	, public std::enable_shared_from_this<CgcTcpClient>
{
public:
	typedef std::shared_ptr<CgcTcpClient> pointer;

	static CgcTcpClient::pointer create(TcpClient_Callback* pCallback, int nUserData)
	{
		return CgcTcpClient::pointer(new CgcTcpClient(pCallback, nUserData));
	}

	CgcTcpClient(void)
		: m_bExitStopIoService(true)
		, m_connectReturned(false)
		, m_bDisconnect(true)
		, m_bException(false)
		, m_pCallback(NULL)
		//, m_sReceiveData(_T(""))
		, m_nUserData(0)
	{}
	CgcTcpClient(TcpClient_Callback* pCallback, int nUserData)
		: m_bExitStopIoService(true)
		, m_connectReturned(false)
		, m_bDisconnect(true)
		, m_bException(false)
		, m_pCallback(pCallback)
		, m_nUserData(nUserData)
		//, m_sReceiveData(_T(""))
	{}
	virtual ~CgcTcpClient(void)
	{
		stopClient();
	}
	static bool IsIpAddress(const char* pString, size_t nLen)
	{
		for (size_t i=0;i<nLen; i++)
		{
			const char pChar = pString[i];
			if (pChar== '.' || (pChar>='0' && pChar<='9'))
				continue;
			return false;
		}
		return true;
	}
	static std::string GetHostIp(const char * lpszHostName,const char* lpszDefault)
	{
		try
		{
			if (IsIpAddress(lpszHostName,strlen(lpszHostName))) return lpszHostName;
			struct hostent *host_entry;
			//struct sockaddr_in addr;
			/* 即要解析的域名或主机名 */
			host_entry=gethostbyname(lpszHostName);
			//printf("%s\n", dn_or_ip);
			if(host_entry!=0)
			{
				char lpszIpAddress[50];
				memset(lpszIpAddress, 0, sizeof(lpszIpAddress));
				//printf("解析IP地址: ");
				sprintf(lpszIpAddress, "%d.%d.%d.%d",
					(host_entry->h_addr_list[0][0]&0x00ff),
					(host_entry->h_addr_list[0][1]&0x00ff),
					(host_entry->h_addr_list[0][2]&0x00ff),
					(host_entry->h_addr_list[0][3]&0x00ff));
				return lpszIpAddress;
			}
		}catch(std::exception&)
		{
		}catch(...)
		{}
		return lpszDefault;
	}
#ifdef USES_OPENSSL
	int startClient(const mycp::tstring & sCgcServerAddr, unsigned int bindPort,boost::asio::ssl::context* ctx=NULL)
#else
	int startClient(const mycp::tstring & sCgcServerAddr, unsigned int bindPort)
#endif
	{
		try
		{
			if (m_tcpClient.get() != 0) return 0;

			mycp::CCgcAddress cgcAddress = mycp::CCgcAddress(sCgcServerAddr, mycp::CCgcAddress::ST_TCP);
			const unsigned short nPort = (unsigned short)cgcAddress.getport();
			if (nPort==0) return -1;

			const mycp::tstring sInIp = cgcAddress.getip();
			std::string sIp;
			for (int i=0;i<5;i++)
			{
				sIp = mycp::httpserver::CgcTcpClient::GetHostIp(sInIp.c_str(),"");
				if (!sIp.empty())
					break;
#ifdef WIN32
				Sleep(100);
#else
				usleep(100000);
#endif
			}
			if (sIp.empty())
				sIp = sInIp.c_str();

			if (m_ipService.get() == 0)
				m_ipService = mycp::asio::IoService::create();

			TcpClient_Handler::pointer clientHandler = std::enable_shared_from_this<CgcTcpClient>::shared_from_this();
			//CgcTcpClient::pointer clientHandler = std::static_pointer_cast<CgcTcpClient, CgcBaseClient>(std::enable_shared_from_this<CgcBaseClient>::shared_from_this());

			m_tcpClient = TcpClient::create(clientHandler);

			m_connectReturned = false;
			// ?? bindPort
			boost::system::error_code ec;
			tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string(sIp.c_str(),ec), nPort);
#ifdef USES_OPENSSL
			m_tcpClient->connect(m_ipService->ioservice(), endpoint,ctx);
#else
			m_tcpClient->connect(m_ipService->ioservice(), endpoint);
#endif
			m_ipService->start(shared_from_this());
			for (int i=0; i<1000; i++)	// max 10S
			{
				if (m_connectReturned) break;
#ifdef WIN32
				Sleep(10);
#else
				usleep(10000);
#endif
			}
			//	while (!m_connectReturned)
			//#ifdef WIN32
			//		Sleep(100);
			//#else
			//		uslee
			//if (!m_bDisconnect)
			//{
			//	m_tcpClient->socket()->get_socket()->set_option(boost::asio::socket_base::send_buffer_size(64*1024),ec);
			//	m_tcpClient->socket()->get_socket()->set_option(boost::asio::socket_base::receive_buffer_size(6*1024),ec);
			//}
			return 0;
		}catch(std::exception&)
		{
		}catch(...)
		{}
		return -1;
	}
	void stopClient(void)
	{
		m_pCallback = NULL;
		if (m_bExitStopIoService && m_ipService.get() != 0)
		{
			m_ipService->stop();
		}
		if (m_tcpClient.get() != 0)
		{
			//m_tcpClient->resetHandler();
			m_tcpClient->disconnect();
		}
		m_tcpClient.reset();
		m_ipService.reset();
	}
	size_t sendData(const unsigned char * data, size_t size)
	{
		//printf("******** CgcTcpClient::sendData1\n");
		if (IsDisconnection() || IsException() || data == 0 || isInvalidate()) return 0;
		//printf("******** CgcTcpClient::sendData2\n");
		//const size_t s = m_tcpClient->write(data, size);
		//m_tcpClient->async_read_some();
		//return s;
		return m_tcpClient->write(data, size);
		//return m_tcpClient->write(data, size);
	}
	//const tstring & GetReceiveData(void) const {return m_sReceiveData;}
	//void ClearData(void) {m_sReceiveData.clear();}
	bool IsDisconnection(void) const {return m_bDisconnect;}
	bool IsException(void) const {return m_bException;}
	void SetUserData(int nUserData) {m_nUserData = nUserData;}

	void SetIoService(mycp::asio::IoService::pointer pIoService, bool bExitStop = false) {m_ipService = pIoService; m_bExitStopIoService=bExitStop;}

private:
	bool isInvalidate(void) const {return m_tcpClient.get() == 0 || !m_tcpClient->is_open();}

	// IoService_Handler
	void OnIoServiceException(void){m_bException = true;}
	///////////////////////////////////////////////
	// for TcpClient2_Handler
	void OnConnected(const TcpClientPointer& tcpClient)
	{
		//printf("******** CgcTcpClient::OnConnected\n");
		m_connectReturned = true;
		m_bDisconnect = false;
		if (m_pCallback != NULL)
			m_pCallback->OnConnected(m_nUserData);

		//if (tcpClient->is_open())
		//{
		//	tcp::endpoint local_endpoint = tcpClient->socket()->local_endpoint();
		//	tcp::endpoint remote_endpoint = tcpClient->socket()->remote_endpoint();
		//	m_ipLocal = CCgcAddress(local_endpoint.address().to_string(), local_endpoint.port(), CCgcAddress::ST_TCP);
		//	m_ipRemote = CCgcAddress(remote_endpoint.address().to_string(), remote_endpoint.port(), CCgcAddress::ST_TCP);
		//}
	}

	void OnConnectError(const TcpClientPointer& tcpClient, const boost::system::error_code & error)
	{
		//printf("******** CgcTcpClient::OnConnectError %d\n", error.value());
		m_connectReturned = true;
		m_bDisconnect=true;
	}
	void OnReceiveData(const TcpClientPointer& tcpClient, const mycp::asio::ReceiveBuffer::pointer& data)
	{
		BOOST_ASSERT (tcpClient.get() != 0);
		BOOST_ASSERT (data.get() != 0);
		//printf("******** CgcTcpClient::OnReceiveData\n");

		if (data->size() <= 0) return;
		if (m_pCallback != NULL)
			m_pCallback->OnReceiveData(data, m_nUserData);
	}

	void OnDisconnect(const TcpClientPointer& tcpClient, const boost::system::error_code & error)
	{
		//printf("******** CgcTcpClient::OnDisconnect %d\n",error.value());
		m_connectReturned = true;m_bDisconnect=true;
		if (m_pCallback != NULL)
			m_pCallback->OnDisconnect(m_nUserData);
	}

private:
	bool m_bExitStopIoService;
	mycp::asio::IoService::pointer m_ipService;
	TcpClient::pointer m_tcpClient;
	bool m_connectReturned;
	bool m_bDisconnect;
	bool m_bException;

	TcpClient_Callback* m_pCallback;
	int m_nUserData;
};

} // namespace httpserver
} // namespace mycp

#endif // __CgcTcpClient_h__
