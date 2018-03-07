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
    return TRUE;
}
#endif

// cgc head
#include <CGCBase/http.h>
using namespace cgc;
#include "XmlParseMvc.h"
#include "XmlParseStructs.h"

XmlParseMvc theMvc;

extern "C" bool CGC_API CGC_Module_Init(void)
{
	cgcParameterMap::pointer initParameters = theApplication->getInitParameters();

	// Load structs.
	tstring xmlFile = theApplication->getAppConfPath();
	xmlFile.append("/");
	theMvc.load(xmlFile, "mvc.xml");
	return true;
}

extern "C" void CGC_API CGC_Module_Free(void)
{
	theMvc.clear();
}

tstring getMethodString(HTTP_METHOD method)
{
	switch (method)
	{
	case HTTP_GET:
		return _T("GET");
	case HTTP_HEAD:
		return _T("HEAD");
	case HTTP_POST:
		return _T("POST");
	case HTTP_PUT:
		return _T("PUT");
	case HTTP_DELETE:
		return _T("DELETE");
	case HTTP_OPTIONS:
		return _T("OPTIONS");
	case HTTP_TRACE:
		return _T("TRACE");
	case HTTP_CONNECT:
		return _T("CONNECT");
	default:
		break;
	}
	return _T("OTHER");
}

//extern "C" HTTP_STATUSCODE CGC_API doGET(const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
extern "C" HTTP_STATUSCODE CGC_API doMVCServer(const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
{
	HTTP_STATUSCODE statusCode = STATUS_CODE_200;

	tstring sModuleName(request->getModuleName());
	tstring sFileName(request->getRequestURI());
	tstring sFunctionName(request->getFunctionName());
	HTTP_METHOD method = request->getHttpMethod();

	CMvcInfo::pointer mvcInfo = theMvc.getMvcInfo(sModuleName);
	if (mvcInfo.get() == NULL)
	{
		return STATUS_CODE_404;
	}
	XmlParseStructs::pointer theStructs = mvcInfo->m_parseStructs;

	CValidateInfo::pointer validateInfo = theStructs->getValidateInfo(sFileName);
	if (validateInfo.get() != NULL)
	{
		// ???
		//validateInfo->
	}

	CExecuteInfo::pointer executeInfo = theStructs->getExecuteInfo(sFunctionName);
	if (executeInfo.get() == NULL)
		executeInfo = theStructs->getExecuteInfo(sFileName);	// ?
	if (executeInfo.get() != NULL)
	{
		const tstring & sExecuteApp = executeInfo->getExecuteApp();
		if (!sExecuteApp.empty())
		{
			tstring sExecuteFunction = executeInfo->getExecuteFunction();
			if (sExecuteFunction.empty())
				sExecuteFunction = getMethodString(method);
			
			tstring sExecuteResult;
			statusCode = theServiceManager->executeService(sExecuteApp, sExecuteFunction, request, response, sExecuteResult);

			if (!sExecuteResult.empty())
			{
				tstring sForwardUrl;
				if (executeInfo->m_controlerForwardUrls.find(sExecuteResult, sForwardUrl))
				{
					response->forward(sForwardUrl);
					return statusCode;
				}else if (executeInfo->m_controlerLocationUrls.find(sExecuteResult, sForwardUrl))
				{
					response->setStatusCode(STATUS_CODE_301);
					response->location(sForwardUrl);
					return statusCode;
				}
			}
		}

		if (!executeInfo->getNormalForwardUrl().empty())
		{
			response->forward(executeInfo->getNormalForwardUrl());
			return statusCode;
		}

	}

	return statusCode;
}

