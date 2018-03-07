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
#ifndef __CgcTcpClient_h__
#define __CgcTcpClient_h__

#ifndef USES_OPENSSL
#define USES_OPENSSL
#endif
#define USES_MYCP_TCPCLIENT
#define USES_PARSER_HTTP
//
#include <ThirdParty/Boost/asio/IoService.h>
#include <ThirdParty/Boost/asio/boost_socket.h>
#ifdef USES_MYCP_TCPCLIENT
#include <ThirdParty/Boost/asio/TcpClient.h>
using namespace mycp::asio;
#else
#include "TcpClient.h"
#endif
#include <CGCBase/cgcSmartString.h>
#ifdef USES_PARSER_HTTP
#include <CGCClass/cgcclassinclude.h>
#endif

namespace mycp {
namespace httpservice {

class TcpClient_Callback
{
public:
	typedef boost::shared_ptr<TcpClient_Callback> pointer;
	virtual void OnReceiveData(const mycp::asio::ReceiveBuffer::pointer& data) = 0;
};

class CgcTcpClient
	: public TcpClient_Handler
	, public mycp::asio::IoService_Handler
	, public boost::enable_shared_from_this<CgcTcpClient>
{
public:
	typedef boost::shared_ptr<CgcTcpClient> pointer;

	static CgcTcpClient::pointer create(TcpClient_Callback* pCallback)
	{
		return CgcTcpClient::pointer(new CgcTcpClient(pCallback));
	}

	CgcTcpClient(void);
	CgcTcpClient(TcpClient_Callback* pCallback);
	virtual ~CgcTcpClient(void);

	void setDeleteFile(bool v) {m_bDeleteFile = v;}
	void setResponseSaveFile(const tstring& s) {m_sResponseSaveFile = s;}
	static bool IsIpAddress(const char* pString, size_t nLen);
	static std::string GetHostIp(const char * lpszHostName,const char* lpszDefault);

#ifdef USES_OPENSSL
	virtual int startClient(const tstring & sCgcServerAddr, unsigned int bindPort,boost::asio::ssl::context* ctx=NULL);
#else
	virtual int startClient(const tstring & sCgcServerAddr, unsigned int bindPort);
#endif
	virtual void stopClient(void);
	virtual size_t sendData(const unsigned char * data, size_t size);
#ifdef USES_PARSER_HTTP
	mycp::cgcParserHttp::pointer GetParserHttp(void) const {return m_pParserHttp;}
	bool IsHttpResponseOk(void) const {return m_bHttpResponseOk;}
#else
	const tstring & GetReceiveData(void) const {return m_sReceiveData;}
	void ClearData(void) {m_sReceiveData.clear();}
#endif
	bool IsDisconnection(void) const {return m_bDisconnect;}
	bool IsException(void) const {return m_bException;}
	bool IsOvertime(int nSeconds) const {return (m_tLastTime>0 && (m_tLastTime+nSeconds)<time(0))?true:false;}

private:
	virtual bool isInvalidate(void) const;

	// IoService_Handler
	virtual void OnIoServiceException(void){m_bException = true;}
	///////////////////////////////////////////////
	// for TcpClient_Handler
	virtual void OnConnected(const TcpClientPointer& tcpClient);
	virtual void OnConnectError(const TcpClientPointer& tcpClient, const boost::system::error_code & error){m_connectReturned = true;m_bDisconnect=true;}
	virtual void OnReceiveData(const TcpClientPointer& tcpClient, const mycp::asio::ReceiveBuffer::pointer& data);
	virtual void OnDisconnect(const TcpClientPointer& tcpClient, const boost::system::error_code & error){m_connectReturned = true;m_bDisconnect=true;}

private:
	mycp::asio::IoService::pointer m_ipService;
	TcpClient::pointer m_tcpClient;
	bool m_connectReturned;
	bool m_bDisconnect;
	bool m_bException;
	time_t m_tLastTime;

	TcpClient_Callback* m_pCallback;
#ifdef USES_PARSER_HTTP
	mycp::cgcParserHttp::pointer m_pParserHttp;
	bool m_bHttpResponseOk;
	bool m_bDeleteFile;
	tstring m_sResponseSaveFile;
#else
	tstring m_sReceiveData;
#endif
};

} // namespace httpservice
} // namespace mycp

#endif // __CgcTcpClient_h__
