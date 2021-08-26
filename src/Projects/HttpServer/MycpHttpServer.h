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

// MycpHttpServer.h file here
#ifndef __mycphttpserver_head__
#define __mycphttpserver_head__

#ifndef WIN32
unsigned long GetLastError(void);
#endif

#define USES_OBJECT_LIST_APP
#define USES_OBJECT_LIST_CDBC

// cgc head
#include <CGCBase/includeall.h>
#include <CGCBase/cgcCDBCService.h>
using namespace mycp;

const tstring MYCP_WEBSERVER_NAME = "MYCP WebServer";

#include "VirtualHost.h"
#include "FileScripts.h"
#include "XmlParseApps.h"
#include "XmlParseDSs.h"

namespace mycp {

typedef enum
{
	OBJECT_APP	= 1
	, OBJECT_CDBC
	, OBJECT_UPLOADS	= 0x10
}OBJECT_TYPE;

class CMycpHttpServer
{
public:
	typedef std::shared_ptr<CMycpHttpServer> pointer;

private:
	cgcAttributes::pointer m_pageParameters;	// page propertys
	cgcParameterMap::pointer m_sysParameters;
	cgcParameterMap::pointer m_initParameters;
	cgcServiceInterface::pointer m_fileSystemService;
	CVirtualHost::pointer m_virtualHost;	// virtual host
	XmlParseApps::pointer m_apps;
	XmlParseDSs::pointer m_datasources;

	cgcApplication::pointer theApplication;
	cgcServiceManager::pointer theServiceManager;
	const cgcHttpRequest::pointer & request;
	cgcHttpResponse::pointer		response;

	tstring m_servletName;
public:
	void setSystem(const cgcSystem::pointer& v);
	void setApplication(const cgcApplication::pointer& v);
	void setFileSystemService(cgcServiceInterface::pointer v) {m_fileSystemService = v;}
	void setServiceManager(cgcServiceManager::pointer v) {theServiceManager = v;}
	void setVirtualHost(CVirtualHost::pointer v) {m_virtualHost = v;}
	void setServletName(const tstring& v) {m_servletName = v;}
	void setApps(XmlParseApps::pointer v) {m_apps = v;}
	void setDSs(XmlParseDSs::pointer v) {m_datasources = v;}

	// -1 : error
	int doIt(const CFileScripts::pointer& fileScripts);

private:

	// -1 : error
	// 0  : continue
	// ~1  : break;			<csp:break
	// 1  : compare return true
	// 2  : re continue;	<csp:continue
	// 3  : page return		<csp:page:return>
	// 4  : break;				<csp:break
	int doScriptItem(const CScriptItem::pointer & scriptItem);
	int doScriptElse(const CScriptItem::pointer & scriptItem);

	cgcAttributes::pointer getAttributes(const tstring& scopy) const;
	cgcValueInfo::pointer getScriptValueInfo1(const CScriptItem::pointer & scriptItem, bool create = true);
	cgcValueInfo::pointer getScriptValueInfo2(const CScriptItem::pointer & scriptItem, bool create = true);
	bool getCompareValueInfo(const CScriptItem::pointer & scriptItem, cgcValueInfo::pointer& compare1_value, cgcValueInfo::pointer& compare2_value);

	VARIABLE_TYPE getVariableType(const tstring& varstring) const;
	cgcValueInfo::pointer getStringValueInfo(const CScriptItem::pointer & scriptItem,const tstring& string, const tstring& scopy = cgcEmptyTString, bool create = true) const;

	void OutputScriptItem(const CScriptItem::pointer & scriptItem);

public:
	CMycpHttpServer(const cgcHttpRequest::pointer & req, cgcHttpResponse::pointer res);
	~CMycpHttpServer(void);
};
const CMycpHttpServer::pointer NullMycpHttpServer;

} // namespace mycp

#endif // __mycphttpserver_head__
