// OpenCommunication.cpp : Defines the initialization routines for the DLL.
//

#ifdef WIN32
#pragma warning(disable:4267 4996)
#include <windows.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif

//#include <boost/enable_shared_from_this.hpp>
// cgc head
#include <CGCBase/app.h>
#include <CGCBase/cgcCommunications.h>
using namespace mycp;

class CMyCommServer
	: public cgcCommHandler
	//, public std::enable_shared_from_this<CMyCommServer>
{
public:
	typedef std::shared_ptr<CMyCommServer> pointer;
	static CMyCommServer::pointer create(void)
	{
		return CMyCommServer::pointer(new CMyCommServer());
	}

private:
	virtual int onRemoteAccept(const cgcRemote::pointer& pcgcRemote)
	{
		theApplication->log(LOG_INFO, _T("**** onRemoteAccept event."));
		return 0;
	}

	virtual int onRecvData(const cgcRemote::pointer& pcgcRemote, const unsigned char * recvData, size_t recvLen)
	{
		theApplication->log(LOG_INFO, _T("**** onRecvData event."));
		theApplication->log(LOG_INFO, _T("Len=%d, Data=%s"), recvLen, recvData);

		std::string sResponseData = (const char*)recvData;
		sResponseData.append(". ACK");
		pcgcRemote->sendData((const unsigned char*)sResponseData.c_str(), sResponseData.length());
		return 0;
	}

	virtual int onRemoteClose(unsigned long remoteId,int nErrorCode)
	{
		theApplication->log(LOG_INFO, _T("**** onRemoteClose event."));
		return 0;
	}

};

//////////////////////////
CMyCommServer::pointer theMyCommServer;
unsigned long gCommId = 0;


cgcCommunication::pointer	theCommunication;

// 模块初始化函数，可选实现函数
extern "C" bool CGC_API CGC_Module_Init(void)
{
	cgcParameterMap::pointer initParameters = theApplication->getInitParameters();
	tstring commTcpServerName = initParameters->getParameterValue("CommTcpServerName", "CommTcpServer");

	theCommunication = CGC_COMMSERVICE_DEF(theServiceManager->getService(commTcpServerName));
	BOOST_ASSERT (theCommunication.get() != NULL);

	theMyCommServer = CMyCommServer::create();
	theCommunication->setCommHandler(theMyCommServer);

	std::vector<cgcValueInfo::pointer> list;
	list.push_back(CGC_VALUEINFO(8028));
	if (!theCommunication->initService(CGC_VALUEINFO(list)))
	{
		theServiceManager->resetService(theCommunication);
		theCommunication.reset();
		theMyCommServer.reset();
		return false;
	}
	return true;
}

// 模块退出函数，可选实现函数
extern "C" void CGC_API CGC_Module_Free(void)
{
	if (theCommunication.get() != NULL)
		theCommunication->finalService();

	theServiceManager->resetService(theCommunication);
	theCommunication.reset();
	theMyCommServer.reset();
}
