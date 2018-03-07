/*
	MYCP is a HTTP and C++ Web Application Server.
    ParserSotp is a software library that parse Simple Object Transmission Protocol(SOTP).
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

// ParserSotp.cpp : Defines the exported functions for the DLL application.
#ifdef WIN32
#pragma warning(disable:4267 4819 4996)
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
#endif // WIN32

// cgc head
#include <CGCBase/cgcParserSotp.h>
#include <CGCClass/cgcclassinclude.h>
using namespace mycp;

class CParserSotp : public CPPSotp2
{
private:
	virtual tstring serviceName(void) const {return _T("PARSERSOTP");}
	virtual void stopService(void) {}
};

extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	outService = cgcParserSotp::pointer(new CParserSotp());
}
