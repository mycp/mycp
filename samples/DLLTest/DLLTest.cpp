/*
	MYCP is a HTTP and C++ Web Application Server.
    DLLTest is a example C++ APP module.
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

// DLLTest.cpp : Defines the initialization routines for the DLL.
//

#ifdef WIN32
#pragma warning(disable:4996)
#include <windows.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif

// Include cgc head files.
#include <CGCBase/app.h>
using namespace mycp;


// Get [UserName] parameter value, and return the [Hi] parameter value.
extern "C" int CGC_API HelloUser(const cgcSotpRequest::pointer & request, const cgcSotpResponse::pointer& response)
{
	// Get request input parameters.
	cgcParameter::pointer pUserName = CGC_REQ_PARAMETER(_T("UserName"));
	if (pUserName.get() == 0) return -1;

	// Set response output parameters.
	tstring sResponse(_T("Hello, "));
	sResponse.append(pUserName->getStr());
	sResponse.append(_T(", How are you!"));

	CGC_RES_LOCK();
	CGC_RES_PARAMETER(CGC_PARAMETER("Hi", sResponse));

	// Send response.
	return 1;
}

