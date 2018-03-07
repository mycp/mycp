/*
	MYCP is a HTTP and C++ Web Application Server.
    DLLTestClient is a example application to use DLLTest app module.
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

// DLLTestClient.cpp : Defines the entry point for the console application.
//

#ifdef WIN32
#pragma warning(disable:4267 4819 4996)
#endif // WIN32

// system
#include <stdio.h>
#include "iostream"

#include <CGCLib/CGCLib.h>
using namespace mycp;

const unsigned long const_CallId_HelloUser	= 0x0001;

//
// Define an event handler.
//
bool bCallResult = false;
class MyCgcClientHandler
	: public CgcClientHandler
{
public:
	MyCgcClientHandler(void)
		: m_bOpenResponsed(false)
	{
	}
	bool IsOpenResponsed(void) const {return m_bOpenResponsed;}

private:
	virtual void OnCgcResponse(const cgcParserSotp & response)
	{
		// Open DLLTest module session return.
		if (response.isResulted() && response.isOpenType())
		{
			std::cout << "SESSION: ";
			std::cout << response.getSid().c_str() << std::endl;
			m_bOpenResponsed = true;
		}

		// Call HelloUser() function return.
		if (response.getSign() == const_CallId_HelloUser)
		{
			cgcParameter::pointer pHi = response.getRecvParameter(_T("Hi"));
			if (pHi.get() != NULL)
				std::cout << _T("[RETURN]:") << pHi->getStr().c_str() << std::endl;


			// response.getResultString() == response.getResultValue()
			std::cout << _T("[ResultCode]:") << response.getResultValue() << std::endl;
			bCallResult = true;
			return;
		}
	}
	virtual void OnCgcResponse(const unsigned char * recvData, size_t dataSize)
	{
		std::cout << _T("[OnCgcResponse]:") << recvData << std::endl;
	}
	virtual void OnCidTimeout(unsigned long callid, unsigned long sign, bool canResendAgain){}

private:
	bool m_bOpenResponsed;
};


CSotpClient gCgcClient;
DoSotpClientHandler::pointer gSotpClientHandler;

//
// Instantiation event handler.
MyCgcClientHandler gMyCgcClientHandler;

//
// All functions.
void cgc_start(void);
void cgc_stop(void);
void cgc_call_hellouser(void);

//
// main function

int main(int argc, const char* argv[])
{
	// Open conect DLLTest module session.
	cgc_start();

	// Call HelloUser function.
	cgc_call_hellouser();

	// Close session.
	cgc_stop();

	getchar();
	return 0;
}


void cgc_start(void)
{
	std::cout << _T("ADDR(ip:port):");
	CCgcAddress::SocketType st(CCgcAddress::ST_UDP);
	std::string sIp;
	std::getline(std::cin, sIp);
	if (sIp.empty())
	{
		sIp = _T("127.0.0.1:8012");
	}

	cgc_stop();
	gSotpClientHandler = gCgcClient.startClient(CCgcAddress(sIp, st));
	BOOST_ASSERT(gSotpClientHandler.get() != NULL);

	// Specifies an event handler.
	gSotpClientHandler->doSetResponseHandler(&gMyCgcClientHandler);

	std::cout << _T("APP Name:");
	std::string appname;
	std::getline(std::cin, appname);
	if (appname.empty())
		appname = _T("DLLTest");

	std::cout << _T("StartClient ") << sIp.c_str() << "..." << std::endl;

	gSotpClientHandler->doSetAccount("admin", "manager");
	// Specify the connection module name.
	gSotpClientHandler->doSetAppName(appname);
	// add by hd 2015-03-24
	gSotpClientHandler->doSetConfig(SOTP_CLIENT_CONFIG_SOTP_VERSION,SOTP_PROTO_VERSION_21);
	gSotpClientHandler->doSetConfig(SOTP_CLIENT_CONFIG_USES_SSL,1);
	gSotpClientHandler->doSendOpenSession();
	while(true)
	{
		if (!gSotpClientHandler->doGetSessionId().empty() ||
			gMyCgcClientHandler.IsOpenResponsed()) {
			break;
		}
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
}

void cgc_stop(void)
{
	if (gSotpClientHandler.get() != NULL)
	{
		gCgcClient.stopClient(gSotpClientHandler);
		gSotpClientHandler.reset();
	}
}

void cgc_call_hellouser(void)
{
	BOOST_ASSERT(gSotpClientHandler.get() != NULL);

	std::cout << "UserName:";
	std::string userName;
	std::getline(std::cin, userName);
	if (userName.empty())
		userName = _T("Akeeyang");

	// Set UserName parameter.
	gSotpClientHandler->doAddParameter(CGC_PARAMETER("UserName", userName));

	// Call HelloUser() function.
	gSotpClientHandler->doSendAppCall(const_CallId_HelloUser, _T("HelloUser"));

	// Wait for HelloUser function event return.
	bCallResult = false;
	while (!bCallResult) {
#ifdef WIN32
		Sleep(2000);
#else
		sleep(2);
#endif
	}
}
