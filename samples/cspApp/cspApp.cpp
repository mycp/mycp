/*
    MYCP is a HTTP and C++ Web Application Server.
	cspApp is a example MYCP C++ APP module.
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

// cspApp.cpp : Defines the exported functions for the DLL application.
//

#ifdef WIN32
#pragma warning(disable:4819 4267 4996)
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
#include <CGCBase/http.h>
#include <CGCBase/cgcServices.h>
using namespace mycp;

cgcAttributes::pointer theAppAttributes;

class CAppService
	: public cgcServiceInterface
{
public:
	typedef boost::shared_ptr<CAppService> pointer;

	static CAppService::pointer create(void)
	{
		return CAppService::pointer(new CAppService());
	}

	virtual tstring serviceName(void) const {return _T("AppService");}

protected:
	virtual bool callService(int function, const cgcValueInfo::pointer& inParam, cgcValueInfo::pointer outParam)
	{
		theApplication->log(LOG_INFO, "AppService callService = %d\n", function);
		return true;
	}
	virtual bool callService(const tstring& function, const cgcValueInfo::pointer& inParam, cgcValueInfo::pointer outParam)
	{
		theApplication->log(LOG_INFO, "AppService callService = %s\n", function.c_str());
		return true;
	}
	virtual cgcAttributes::pointer getAttributes(void) const {return theAppAttributes;}

public:
	CAppService(void)
	{
	}
	~CAppService(void)
	{
		finalService();
	}

private:

};

const int ATTRIBUTE_NAME = 1;
unsigned int theCurrentIdEvent = 0;


extern "C" bool CGC_API CGC_Module_Init(void)
{
	theAppAttributes = theApplication->getAttributes(true);
	assert (theAppAttributes.get() != NULL);
	return true;
}

extern "C" void CGC_API CGC_Module_Free(void)
{
	VoidObjectMapPointer mapLogServices = theAppAttributes->getVoidAttributes(ATTRIBUTE_NAME, false);
	if (mapLogServices.get() != NULL)
	{
		CObjectMap<void*>::iterator iter;
		for (iter=mapLogServices->begin(); iter!=mapLogServices->end(); iter++)
		{
			CAppService::pointer logService = CGC_OBJECT_CAST<CAppService>(iter->second);
			if (logService.get() != NULL)
			{
				logService->finalService();
			}
		}
	}

	theAppAttributes.reset();
}


extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	CAppService::pointer appService = CAppService::create();
	appService->initService();

	outService = appService;

	cgcAttributes::pointer attributes = theApplication->getAttributes();
	assert (attributes.get() != NULL);

	theAppAttributes->setAttribute(ATTRIBUTE_NAME, outService.get(), appService);
}

extern "C" void CGC_API CGC_ResetService(cgcServiceInterface::pointer inService)
{
	if (inService.get() == NULL) return;

	theAppAttributes->removeAttribute(ATTRIBUTE_NAME, inService.get());
	inService->finalService();
}

extern "C" HTTP_STATUSCODE CGC_API doServiceTest(const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
{
	response->println("<b>doServiceTest Function here.</b><br>");
	return STATUS_CODE_200;
}
