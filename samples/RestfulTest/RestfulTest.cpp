/*
	MYCP is a HTTP and C++ Web Application Server.
    cspServlet is a example MYCP C++ Servlet module.
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

// cspServlet.cpp : Defines the initialization routines for the DLL.
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
#include <CGCBase/httpapp.h>
using namespace mycp;

extern "C" HTTP_STATUSCODE CGC_API doGET(const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
{
	cgcSession::pointer session = request->getSession();

	response->println("<HTML>");
	response->println("<TITLE>MYCP Web Server</TITLE>");
	response->println("<h1>Get Sample</h1>");

	response->println("<h2>Session Info:</h2>");
	if (session->isNewSession())
	{
		response->println("This is a new session.");
	}else
	{
		response->println("This is not a new session.");
	}

	response->println("<h2>RestVersion: %s</h2>", request->getRestVersion().c_str());

	response->println("<h2>Headers:</h2>");
	std::vector<cgcKeyValue::pointer> headers;
	if (request->getHeaders(headers))
	{
		for (size_t i=0; i<headers.size(); i++)
		{
			cgcKeyValue::pointer keyValue = headers[i];
			response->println("%s: %s", keyValue->getKey().c_str(), keyValue->getValue()->getStr().c_str());
		}
	}else
	{
		response->println("Not header info.");
	}

	response->println("<h2>Propertys:</h2>");
	std::vector<cgcKeyValue::pointer> parameters;
	if (request->getParameters(parameters))
	{
		for (size_t i=0; i<parameters.size(); i++)
		{
			cgcKeyValue::pointer keyValue = parameters[i];
			response->println("%s = %s<BR>", keyValue->getKey().c_str(), keyValue->getValue()->getStr().c_str());
		}
	}else
	{
		response->println("Not property.");
	}

	response->println("</HTML>");
	return STATUS_CODE_200;
}

extern "C" HTTP_STATUSCODE CGC_API dofunc1(const cgcHttpRequest::pointer & request, const cgcHttpResponse::pointer& response)
{
	/// JSON
	/// response->setHeader(Http_ContentType,"application/json; charset=UTF-8");
	response->println("<HTML>");
	response->println("<TITLE>MYCP Web Server</TITLE>");
	response->println("<h1>func1 Sample</h1>");

	response->println("<h2>RestVersion: %s</h2>", request->getRestVersion().c_str());

	const bool bRestVersion3 = request->getRestVersion()=="v03"?true:false;

	response->println("</HTML>");
	return STATUS_CODE_200;
}

