#ifdef WIN32
#pragma warning(disable:4267 4819 4996)
#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0501     // Change this to the appropriate value to target other versions of Windows.
#endif
#include <winsock2.h>
#include <windows.h>
#endif // WIN32

#include "CgcTcpClient.h"
#include <CGCBase/cgcaddress.h>

namespace mycp {
namespace httpservice {

CgcTcpClient::CgcTcpClient(TcpClient_Callback* pCallback)
: m_connectReturned(false)
, m_bDisconnect(true)
, m_bException(false)
, m_tLastTime(0)
, m_pCallback(pCallback)
#ifdef USES_PARSER_HTTP
, m_bHttpResponseOk(false)
, m_bDeleteFile(true)
#else
, m_sReceiveData(_T(""))
#endif

{
}
CgcTcpClient::CgcTcpClient(void)
: m_connectReturned(false)
, m_bDisconnect(true)
, m_bException(false)
, m_tLastTime(0)
, m_pCallback(NULL)
#ifdef USES_PARSER_HTTP
, m_bHttpResponseOk(false)
, m_bDeleteFile(true)
#else
, m_sReceiveData(_T(""))
#endif
{
}
CgcTcpClient::~CgcTcpClient(void)
{
	stopClient();
}
bool CgcTcpClient::IsIpAddress(const char* pString, size_t nLen)
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
std::string CgcTcpClient::GetHostIp(const char * lpszHostName,const char* lpszDefault)
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

	//struct hostent *host_entry;
	////struct sockaddr_in addr;
	///* 即要解析的域名或主机名 */
	//host_entry=gethostbyname(lpszHostName);
	////printf("%s\n", dn_or_ip);
	//char lpszIpAddress[50];
	//memset(lpszIpAddress, 0, sizeof(lpszIpAddress));
	//if(host_entry!=0)
	//{
	//	//printf("解析IP地址: ");
	//	sprintf(lpszIpAddress, "%d.%d.%d.%d",
	//		(host_entry->h_addr_list[0][0]&0x00ff),
	//		(host_entry->h_addr_list[0][1]&0x00ff),
	//		(host_entry->h_addr_list[0][2]&0x00ff),
	//		(host_entry->h_addr_list[0][3]&0x00ff));
	//	return lpszIpAddress;
	//}else
	//{
	//	return lpszDefault;
	//}
}

#ifdef USES_OPENSSL
int CgcTcpClient::startClient(const tstring & sCgcServerAddr, unsigned int bindPort,boost::asio::ssl::context* ctx)
#else
int CgcTcpClient::startClient(const tstring & sCgcServerAddr, unsigned int bindPort)
#endif
{
	if (m_tcpClient.get() != 0) return 0;

	CCgcAddress cgcAddress = CCgcAddress(sCgcServerAddr, CCgcAddress::ST_TCP);
	const unsigned short nPort = (unsigned short)cgcAddress.getport();
	const tstring sInIp = cgcAddress.getip();
	tstring sIp;
	for (int i=0;i<5;i++)
	{
		sIp = CgcTcpClient::GetHostIp(sInIp.c_str(),"");
		if (!sIp.empty())
			break;
#ifdef WIN32
		Sleep(100);
#else
		usleep(100000);
#endif
	}
	if (sIp.empty())
		sIp = sInIp;

	if (m_ipService.get() == 0)
		m_ipService = mycp::asio::IoService::create();

	TcpClient_Handler::pointer clientHandler = boost::enable_shared_from_this<CgcTcpClient>::shared_from_this();
	//CgcTcpClient::pointer clientHandler = boost::static_pointer_cast<CgcTcpClient, CgcBaseClient>(boost::enable_shared_from_this<CgcBaseClient>::shared_from_this());

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
	while (!m_connectReturned)
#ifdef WIN32
		Sleep(100);
#else
		usleep(100000);
#endif
	return 0;
}

void CgcTcpClient::stopClient(void)
{
	m_pCallback = NULL;
	if (m_ipService.get() != 0)
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

size_t CgcTcpClient::sendData(const unsigned char * data, size_t size)
{
	if (IsDisconnection() || IsException() || data == 0 || isInvalidate()) return 0;
	//const size_t s = m_tcpClient->write(data, size);
	//m_tcpClient->async_read_some();
	//return s;
	m_tLastTime = time(0);
	return m_tcpClient->write(data, size);
	//return m_tcpClient->write(data, size);
}

bool CgcTcpClient::isInvalidate(void) const
{
	return m_tcpClient.get() == 0 || !m_tcpClient->is_open();
}

void CgcTcpClient::OnConnected(const TcpClientPointer& tcpClient)
{
	m_connectReturned = true;
	m_bDisconnect = false;
	//if (tcpClient->is_open())
	//{
	//	tcp::endpoint local_endpoint = tcpClient->socket()->local_endpoint();
	//	tcp::endpoint remote_endpoint = tcpClient->socket()->remote_endpoint();
	//	m_ipLocal = CCgcAddress(local_endpoint.address().to_string(), local_endpoint.port(), CCgcAddress::ST_TCP);
	//	m_ipRemote = CCgcAddress(remote_endpoint.address().to_string(), remote_endpoint.port(), CCgcAddress::ST_TCP);
	//}
}

void CgcTcpClient::OnReceiveData(const TcpClientPointer& tcpClient, const mycp::asio::ReceiveBuffer::pointer& data)
{
	BOOST_ASSERT (tcpClient.get() != 0);
	BOOST_ASSERT (data.get() != 0);
	if (data->size() <= 0) return;
	if (m_pCallback != NULL)
		m_pCallback->OnReceiveData(data);
	//unsigned char lpszBuffer[1024];
	//memcpy(lpszBuffer,data->data(),data->size());
	//m_tSendRecv = time(0);
	//this->parseData(CCgcData::create(data->data(), data->size()));
#ifdef USES_PARSER_HTTP
	if (m_bHttpResponseOk)
	{
		//printf("******** %s, OnReceiveData(size=%d) HttpResponseOk=1 error\n",m_sResponseSaveFile.c_str(), data->size());
		return;
	}
	m_tLastTime = time(0);
	if (m_pParserHttp.get()==NULL)
	{
		CPpHttp* parserHttp = new CPpHttp();
		parserHttp->SetResponseSaveFile(m_sResponseSaveFile);
		parserHttp->theUpload.setDeleteFile(m_bDeleteFile);
		parserHttp->theUpload.setEnableAllContentType(true);
		parserHttp->theUpload.setEnableUpload(true);
		parserHttp->theUpload.setMaxFileCount(0);
		parserHttp->theUpload.setMaxFileSize(0);
		parserHttp->theUpload.setMaxUploadSize(0);
		//parserHttp->theUpload.load(theXmlFile);
		//parserHttp->setFileSystemService(theFileSystemService);
		m_pParserHttp = cgcParserHttp::pointer(parserHttp);
	}
	//printf("***%d\n%s\n***\n",data->size(),data->data());
	if (m_pParserHttp->doParse(data->data(),data->size()))
		m_bHttpResponseOk = true;
	//const int nReceiveSize = ((CPpHttp*)m_pParserHttp.get())->GetReceiveSize();
	//printf("******** (%s), OnReceiveData(size=%d), ContentLength=%d, ReceiveSize=%d, HttpResponseOk=%d\n",(const char*)data->data(),data->size(),m_pParserHttp->getContentLength(),nReceiveSize,(int)(m_bHttpResponseOk?1:0));
	//printf("******** %s, OnReceiveData(size=%d), ContentLength=%d, ReceiveSize=%d, HttpResponseOk=%d\n",m_sResponseSaveFile.c_str(),data->size(),m_pParserHttp->getContentLength(),nReceiveSize,(int)(m_bHttpResponseOk?1:0));
#else
	m_sReceiveData.append(tstring((const char*)data->data(), data->size()));
#endif
}

} // namespace httpservice
} // namespace mycp
