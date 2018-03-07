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
#ifdef WIN32
#pragma warning(disable:4267 4996)
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

//#include "Base64.h"
// cgc head
#include <CGCBase/app.h>
#include <CGCBase/cgcParserHttp.h>
#include <CGCClass/cgcclassinclude.h>
using namespace mycp;

//class CParserHttp
//	: public CPpHttp
//{
//private:
//	virtual tstring serviceName(void) const {return _T("PARSERSOTP");}
//	virtual void stopService(void) {}
//};

#include <CGCBase/cgcServices.h>
cgcServiceInterface::pointer theFileSystemService;
tstring theXmlFile;

extern "C" bool CGC_API CGC_Module_Init(void)
{
	theXmlFile = theApplication->getAppConfPath();
	theXmlFile.append(_T("/upload.xml"));
	theFileSystemService = theServiceManager->getService("FileSystemService");
	if (theFileSystemService.get() == NULL)
	{
		CGC_LOG((LOG_ERROR, "FileSystemService Error.\n"));
	}else
	{
		//cgcValueInfo::pointer inProperty = CGC_VALUEINFO(xmlFile);
		//cgcValueInfo::pointer outProperty = CGC_VALUEINFO(false);
		//theFileSystemService->callService("exists", inProperty, outProperty);
		//if (outProperty->getBoolean())
		//{
		//	theUpload.load(theXmlFile);
		//}
	}
	return true;
}

extern "C" void CGC_API CGC_Module_Free(void)
{
	theFileSystemService.reset();
	//theUpload.clear();
}

extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	CPpHttp * parserHttp = new CPpHttp();
	parserHttp->theUpload.load(theXmlFile);
	parserHttp->setFileSystemService(theFileSystemService);
	outService = cgcParserHttp::pointer(parserHttp);
}
