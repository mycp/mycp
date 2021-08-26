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
#pragma warning(disable:4267 4819 4996)
#include <windows.h>
#else
unsigned long GetLastError(void)
{
	return 0;
}
#endif

#include "MycpHttpServer.h"

class CUploadFiles
	: public cgcObject
{
public:
	typedef std::shared_ptr<CUploadFiles> pointer;

	std::vector<cgcUploadFile::pointer> m_files;

	~CUploadFiles(void)
	{
		m_files.clear();
	}
	virtual cgcObject::pointer copyNew(void) const
	{
		CUploadFiles::pointer result = CUploadFiles::pointer(new CUploadFiles());
		// ???
		return result;
	}
};

CMycpHttpServer::CMycpHttpServer(const cgcHttpRequest::pointer & req, cgcHttpResponse::pointer res)
: request(req), response(res)
, m_servletName("")

{
	m_pageParameters = req->getPageAttributes();
}
CMycpHttpServer::~CMycpHttpServer(void)
{
	m_pageParameters.reset();
}

void CMycpHttpServer::setSystem(const cgcSystem::pointer& v)
{
	assert (v.get() != NULL);
	m_sysParameters = v->getInitParameters();
}

void CMycpHttpServer::setApplication(const cgcApplication::pointer& v)
{
	assert (v.get() != NULL);
	theApplication = v;

	if (m_pageParameters.get()==NULL)
		m_pageParameters = theApplication->createAttributes();
	// Initialize Temp Variant
	m_pageParameters->setProperty(CSP_TEMP_VAR_INDEX, CGC_VALUEINFO((int)0));
	m_pageParameters->setProperty(CSP_TEMP_VAR_VALUE, CGC_VALUEINFO(""));
	//m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO(false));

	//m_pageParameters->setProperty(CSP_TEMP_VAR_FILENAME, CGC_VALUEINFO(""));
	//m_pageParameters->setProperty(CSP_TEMP_VAR_FILEPATH, CGC_VALUEINFO(""));
	//m_pageParameters->setProperty(CSP_TEMP_VAR_FILESIZE, CGC_VALUEINFO((int)0));
	//m_pageParameters->setProperty(CSP_TEMP_VAR_FILETYPE, CGC_VALUEINFO(""));

	
	m_initParameters = theApplication->getInitParameters();
}

int CMycpHttpServer::doIt(const CFileScripts::pointer& fileScripts)
{
	assert (fileScripts.get() != NULL);

	const std::vector<CScriptItem::pointer> & scripts = fileScripts->getScripts();

	for (size_t i=0; i<scripts.size(); i++)
	{
		const CScriptItem::pointer& scriptItem = scripts[i];
		const int ret = doScriptItem(scriptItem);
		if (ret == -1)
		{
			OutputScriptItem(scriptItem);
			return -1;
		}else if (ret == 3)
			break;
	}
	return 0;
}

cgcAttributes::pointer CMycpHttpServer::getAttributes(const tstring& scopy) const
{
	if (scopy == CSP_SCOPE_TYPE_APPLICATIONL)//"application")
	{
		assert (m_virtualHost.get() != NULL && m_virtualHost->getPropertys().get() != NULL);
		return m_virtualHost->getPropertys();
	}else if (scopy == CSP_SCOPE_TYPE_SESSION)//"session")
	{
		return request->getSession()->getAttributes(true);
	}else if (scopy == CSP_SCOPE_TYPE_REQUEST)//"request")
	{
		return request->getAttributes(true);
	}else if (scopy == CSP_SCOPE_TYPE_SERVER)//"server")
	{
		return theApplication->getAttributes(true);
	}else
	{
		return m_pageParameters;
	}
}

VARIABLE_TYPE CMycpHttpServer::getVariableType(const tstring& varstring) const
{
	int leftIndex = 0;
	if (CFileScripts::sotpCompare(varstring.c_str(), VTI_USER.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_USER.size())
		return VARIABLE_USER;
	else if (CFileScripts::sotpCompare(varstring.c_str(), VTI_SYSTEM.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_SYSTEM.size())
		return VARIABLE_SYSTEM;
	else if (CFileScripts::sotpCompare(varstring.c_str(), VTI_INITPARAM.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_INITPARAM.size())
		return VARIABLE_INITPARAM;
	else if (CFileScripts::sotpCompare(varstring.c_str(), VTI_REQUESTPARAM.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_REQUESTPARAM.size())
		return VARIABLE_REQUESTPARAM;
	else if (CFileScripts::sotpCompare(varstring.c_str(), VTI_HEADER.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_HEADER.size())
		return VARIABLE_HEADER;
	else if (CFileScripts::sotpCompare(varstring.c_str(), VTI_TEMP.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_TEMP.size())
		return VARIABLE_TEMP;
	else if (CFileScripts::sotpCompare(varstring.c_str(), VTI_APP.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_APP.size())
		return VARIABLE_APP;
	else if (CFileScripts::sotpCompare(varstring.c_str(), VTI_DATASOURCE.c_str(), leftIndex) && leftIndex == 0 && varstring.size() > VTI_DATASOURCE.size())
		return VARIABLE_DATASOURCE;
	return VARIABLE_VALUE;
}

cgcValueInfo::pointer CMycpHttpServer::getStringValueInfo(const CScriptItem::pointer & scriptItem, const tstring& string, const tstring& scopy, bool create) const
{
	if (string.empty()) return cgcNullValueInfo;

	cgcValueInfo::pointer var_value;
	VARIABLE_TYPE var_type = getVariableType(string);
	switch (var_type)
	{
	case VARIABLE_DATASOURCE:
		{
			var_value = m_virtualHost->getPropertys()->getProperty(string);
			if (var_value.get() == NULL)
			{
				if (m_datasources.get() == NULL) return cgcNullValueInfo;

				CDSInfo::pointer dsInfo = m_datasources->getDSInfo(string.substr(VTI_DATASOURCE.size()));
				if (dsInfo.get() == NULL) return cgcNullValueInfo;

				// CDBC Service
				cgcCDBCService::pointer cdbcService = theServiceManager->getCDBDService(dsInfo->getDSCdbc());
				if (cdbcService.get() == NULL) return cgcNullValueInfo;
				var_value = CGC_VALUEINFO(CGC_OBJECT_CAST<cgcObject>(cdbcService));
				var_value->setInt((int)OBJECT_CDBC);
				var_value->setStr(dsInfo->getDSCdbc());
#ifdef USES_OBJECT_LIST_CDBC
				theApplication->getAttributes(true)->addListAttribute((int)OBJECT_CDBC, var_value, true);
#else
				theApplication->getAttributes(true)->setProperty((int)OBJECT_CDBC, var_value);
#endif
				var_value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
				m_virtualHost->getPropertys()->setProperty(string, var_value);
			}
		}break;
	case VARIABLE_APP:
		{
			// ? m_virtualHost
			var_value = m_virtualHost->getPropertys()->getProperty(string);
			if (var_value.get() == NULL)
			{
				if (m_apps.get() == NULL) return cgcNullValueInfo;

				CAppInfo::pointer appInfo = m_apps->getAppInfo(string.substr(VTI_APP.size()));
				if (appInfo.get() == NULL) return cgcNullValueInfo;

				cgcValueInfo::pointer initProperty;
				if (!appInfo->getInitParam().empty())
					initProperty = CGC_VALUEINFO(appInfo->getInitParam());
				else if (appInfo->getIntParam() != 0)
					initProperty = CGC_VALUEINFO(appInfo->getIntParam());

				//printf("**** OBJECT_APP %s=%s...\n",string.c_str(),appInfo->getAppName().c_str());
				// APP Service
				cgcServiceInterface::pointer serviceInterface = theServiceManager->getService(appInfo->getAppName(), initProperty);
				if (serviceInterface.get() == NULL) return cgcNullValueInfo;
				//printf("**** OBJECT_APP %s=%s ok\n",string.c_str(),appInfo->getAppName().c_str());
				var_value = CGC_VALUEINFO(CGC_OBJECT_CAST<cgcObject>(serviceInterface));
				var_value->setInt((int)OBJECT_APP);
				var_value->setStr(appInfo->getAppName());
#ifdef USES_OBJECT_LIST_APP
				theApplication->getAttributes(true)->addListAttribute((int)OBJECT_APP, var_value, true);
#else
				theApplication->getAttributes(true)->setProperty((int)OBJECT_APP, var_value);
#endif
				var_value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
				m_virtualHost->getPropertys()->setProperty(string, var_value);
			}
		}break;
	case VARIABLE_USER:
		{
			cgcAttributes::pointer attributes = getAttributes(scopy);
			assert (attributes.get() != NULL);
			std::string sStringTemp(string.c_str());
			rtrim(sStringTemp);
			var_value = attributes->getProperty(sStringTemp);
			//printf("**** VARIABLE_USER script_scope=%s,script_id=%s,script_value=%s,scope=%s,string=%s,value=0x%x\n",
			//	scriptItem->getScope().c_str(),scriptItem->getId().c_str(),scriptItem->getValue().c_str(),scopy.c_str(),string.c_str(),(int)var_value.get());
			if (var_value.get() == NULL && scopy.empty() && scopy!=CSP_SCOPE_TYPE_SESSION)
			{
				attributes = getAttributes(CSP_SCOPE_TYPE_SESSION);
				var_value = attributes->getProperty(sStringTemp);
				//printf("**** CSP_SCOPE_TYPE_SESSION, string=%s, value=0x%x\n",string.c_str(),(int)var_value.get());
				//if (var_value.get() != NULL)
				//{
				//	scriptItem->setScope(CSP_SCOPE_TYPE_SESSION);
				//}
			}
			if (var_value.get() == NULL && scopy.empty() && scopy!=CSP_SCOPE_TYPE_REQUEST)
			{
				attributes = getAttributes(CSP_SCOPE_TYPE_REQUEST);
				var_value = attributes->getProperty(sStringTemp);
				//if (var_value.get() != NULL)
				//{
				//	scriptItem->setScope(CSP_SCOPE_TYPE_REQUEST);
				//}
			}
			if (var_value.get() == NULL && scopy.empty() && scopy!=CSP_SCOPE_TYPE_APPLICATIONL)
			{
				attributes = getAttributes(CSP_SCOPE_TYPE_APPLICATIONL);
				var_value = attributes->getProperty(sStringTemp);
				//if (var_value.get() != NULL)
				//{
				//	scriptItem->setScope(CSP_SCOPE_TYPE_APPLICATIONL);
				//}
			}
			if (var_value.get() == NULL && scopy.empty() && scopy!=CSP_SCOPE_TYPE_SERVER)
			{
				attributes = getAttributes(CSP_SCOPE_TYPE_SERVER);
				var_value = attributes->getProperty(sStringTemp);
				//if (var_value.get() != NULL)
				//{
				//	scriptItem->setScope(CSP_SCOPE_TYPE_SERVER);
				//}
			}
			//if (var_value.get() != NULL)
			//{
			//	printf("**** value_string=%s\n",var_value->getStrValue().c_str());
			//}

			if (var_value.get() == NULL && create)
			{
				var_value = CGC_VALUEINFO((tstring)"");
				attributes->setProperty(sStringTemp, var_value);
				//var_value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
			}

		}break;
	case VARIABLE_REQUESTPARAM:
		{
			var_value = m_pageParameters->getProperty(string);
			//printf("**** VARIABLE_REQUESTPARAM script_scope=%s,script_id=%s,script_value=%s,scope=%s,string=%s,value=0x%x\n",
			//	scriptItem->getScope().c_str(),scriptItem->getId().c_str(),scriptItem->getValue().c_str(),scopy.c_str(),string.c_str(),(int)var_value.get());

			if (var_value.get() == NULL)
			{
				const tstring strParamName(string.substr(VTI_REQUESTPARAM.size()));
				var_value = request->getParameter(strParamName);
				if (var_value.get()==NULL && request->getAttributes().get() != NULL)
				{
					var_value = request->getProperty(strParamName);
				}
				//std::vector<cgcValueInfo::pointer> vectors;
				//bool bGet = false;
				//if (request->getParameter(strParamName, vectors))
				//{
				//	if (vectors.size() == 1)
				//	{
				//		var_value = vectors[0];
				//	}else
				//	{
				//		var_value = CGC_VALUEINFO(vectors);
				//	}
				//}else if (request->getAttributes().get() != NULL)
				//{
				//	var_value = request->getProperty(strParamName);
				//}
				if (var_value.get() != NULL)
				{
					var_value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(string, var_value);
				//}else if (create)
				//{
				//	var_value = CGC_VALUEINFO("");
				}
			}
		}break;
	case VARIABLE_INITPARAM:
		{
			var_value = m_pageParameters->getProperty(string);
			if (var_value.get() == NULL)
			{
				assert (m_initParameters.get() != NULL);
				cgcValueInfo::pointer var_temp = m_initParameters->getParameter(string.substr(VTI_INITPARAM.size()));
				if (var_temp.get() != NULL)
				{
					var_value = var_temp->copy();
					var_value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(string, var_value);
				}
			}
		}break;
	case VARIABLE_HEADER:
		{
			var_value = m_pageParameters->getProperty(string);
			if (var_value.get() == NULL)
			{
				tstring headValue = request->getHeader(string.substr(VTI_HEADER.size()));
				var_value = CGC_VALUEINFOA(headValue, cgcValueInfo::ATTRIBUTE_READONLY);
				m_pageParameters->setProperty(string, var_value);
			}
		}break;
	case VARIABLE_SYSTEM:
		{
			if (string == CSP_SYS_VAR_HEADNAMES)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_HEADNAMES);
				if (var_value.get() == NULL)
				{
					std::vector<cgcValueInfo::pointer> vectors;
					std::vector<cgcKeyValue::pointer> headers;
					if (request->getHeaders(headers))
					{
						for (size_t i=0; i<headers.size(); i++)
						{
							const cgcKeyValue::pointer& header = headers[i];
							vectors.push_back(CGC_VALUEINFO(header->getKey()));
						}
					}
					var_value = CGC_VALUEINFOA(vectors, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_HEADNAMES, var_value);
				}
			}else if (string == CSP_SYS_VAR_HEADVALUES)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_HEADVALUES);
				if (var_value.get() == NULL)
				{
					std::vector<cgcValueInfo::pointer> vectors;
					std::vector<cgcKeyValue::pointer> headers;
					if (request->getHeaders(headers))
					{
						for (size_t i=0; i<headers.size(); i++)
						{
							const cgcKeyValue::pointer& header = headers[i];
							vectors.push_back(CGC_VALUEINFO(header->getValue()->getStr()));
						}
					}
					var_value = CGC_VALUEINFOA(vectors, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_HEADVALUES, var_value);
				}
			}else if (string == CSP_SYS_VAR_HEADS)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_HEADS);
				if (var_value.get() == NULL)
				{
					std::vector<cgcValueInfo::pointer> vectors;
					std::vector<cgcKeyValue::pointer> headers;
					if (request->getHeaders(headers))
					{
						for (size_t i=0; i<headers.size(); i++)
						{
							const cgcKeyValue::pointer& header = headers[i];
							tstring sTemp(header->getKey());
							sTemp.append(": ");
							sTemp.append(header->getValue()->getStr());
							vectors.push_back(CGC_VALUEINFO(sTemp));
						}
					}
					var_value = CGC_VALUEINFOA(vectors, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_HEADS, var_value);
				}
			}else if (string == CSP_SYS_VAR_PARAMNAMES)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_PARAMNAMES);
				if (var_value.get() == NULL)
				{
					std::vector<cgcValueInfo::pointer> vectors;
					std::vector<cgcKeyValue::pointer> params;
					if (request->getParameters(params))
					{
						for (size_t i=0; i<params.size(); i++)
						{
							const cgcKeyValue::pointer& param = params[i];
							vectors.push_back(CGC_VALUEINFO(param->getKey()));
						}
					}
					var_value = CGC_VALUEINFOA(vectors, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_PARAMNAMES, var_value);
				}
			}else if (string == CSP_SYS_VAR_PARAMVALUES)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_PARAMVALUES);
				if (var_value.get() == NULL)
				{
					std::vector<cgcValueInfo::pointer> vectors;
					std::vector<cgcKeyValue::pointer> params;
					if (request->getParameters(params))
					{
						for (size_t i=0; i<params.size(); i++)
						{
							const cgcKeyValue::pointer& param = params[i];
							vectors.push_back(CGC_VALUEINFO(param->getValue()->getStr()));
						}
					}
					var_value = CGC_VALUEINFOA(vectors, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_PARAMVALUES, var_value);
				}
			}else if (string == CSP_SYS_VAR_PARAMS)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_PARAMS);
				if (var_value.get() == NULL)
				{
					std::vector<cgcValueInfo::pointer> vectors;
					std::vector<cgcKeyValue::pointer> params;
					if (request->getParameters(params))
					{
						for (size_t i=0; i<params.size(); i++)
						{
							const cgcKeyValue::pointer& param = params[i];
							tstring sTemp(param->getKey());
							sTemp.append("=");
							sTemp.append(param->getValue()->getStr());
							vectors.push_back(CGC_VALUEINFO(sTemp));
						}
					}
					var_value = CGC_VALUEINFOA(vectors, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_PARAMS, var_value);
				}
			}else if (string == CSP_SYS_VAR_PARAMNAMES)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_PARAMNAMES);
				if (var_value.get() == NULL)
				{
					std::vector<cgcValueInfo::pointer> vectors;
					std::vector<cgcKeyValue::pointer> params;
					if (request->getParameters(params))
					{
						for (size_t i=0; i<params.size(); i++)
						{
							const cgcKeyValue::pointer& param = params[i];
							vectors.push_back(CGC_VALUEINFO(param->getKey()));
						}
					}
					var_value = CGC_VALUEINFOA(vectors, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_PARAMNAMES, var_value);
				}
			}else if (string == CSP_SYS_VAR_CONTENTLENGTH)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_CONTENTLENGTH);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA((int)request->getContentLength(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_CONTENTLENGTH, var_value);
				}
			}else if (string == CSP_SYS_VAR_CONTENTTYPE)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_CONTENTTYPE);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getContentType(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_CONTENTTYPE, var_value);
				}
			}else if (string == CSP_SYS_VAR_SCHEME)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_SCHEME);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getScheme(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_SCHEME, var_value);
				}
			}else if (string == CSP_SYS_VAR_METHODSTRING)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_METHODSTRING);
				if (var_value.get() == NULL)
				{
					tstring sMethod = "NONE";
					if (request->getHttpMethod() == HTTP_GET)
						sMethod = "GET";
					else if (request->getHttpMethod() == HTTP_POST)
						sMethod = "POST";
					var_value = CGC_VALUEINFOA(sMethod, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_METHODSTRING, var_value);
				}
			}else if (string == CSP_SYS_VAR_PROTOCOL)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_PROTOCOL);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getHttpVersion(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_PROTOCOL, var_value);
				}
			}else if (string == CSP_SYS_VAR_METHOD)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_METHOD);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA((int)request->getHttpMethod(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_METHOD, var_value);
				}
			}else if (string == CSP_SYS_VAR_REQUESTURL)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_REQUESTURL);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getRequestURL(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_REQUESTURL, var_value);
				}
			}else if (string == CSP_SYS_VAR_REQUESTURI)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_REQUESTURI);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getRequestURI(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_REQUESTURI, var_value);
				}
			}else if (string == CSP_SYS_VAR_QUERYSTRING)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_QUERYSTRING);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getQueryString(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_QUERYSTRING, var_value);
				}
			}else if (string == CSP_SYS_VAR_REMOTEIP)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_REMOTEIP);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA((mycp::bigint)request->getIpAddress(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_REMOTEIP, var_value);
				}
			}else if (string == CSP_SYS_VAR_REMOTEADDR)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_REMOTEADDR);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getRemoteAddr(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_REMOTEADDR, var_value);
				}
			}else if (string == CSP_SYS_VAR_REMOTEHOST)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_REMOTEHOST);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getRemoteHost(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_REMOTEHOST, var_value);
				}
			}else if (string == CSP_SYS_VAR_AUTHACCOUNT)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_AUTHACCOUNT);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getRemoteAccount(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_AUTHACCOUNT, var_value);
				}
			}else if (string == CSP_SYS_VAR_AUTHSECURE)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_AUTHSECURE);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getRemoteSecure(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_AUTHSECURE, var_value);
				}
			}else if (string == CSP_SYS_VAR_SERVERNAME)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_SERVERNAME);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(MYCP_WEBSERVER_NAME, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_SERVERNAME, var_value);
				}
			}else if (string == CSP_SYS_VAR_SERVERPORT)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_SERVERPORT);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getServerPort(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_SERVERPORT, var_value);
				}
			}else if (string == CSP_SYS_VAR_CONTEXTPATH)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_CONTEXTPATH);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(m_virtualHost->getDocumentRoot(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_CONTEXTPATH, var_value);
				}
			}else if (string == CSP_SYS_VAR_SERVLETNAME)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_SERVLETNAME);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(m_servletName, cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_SERVLETNAME, var_value);
				}
			}else if (string == CSP_SYS_VAR_ISNEWSESSION)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_ISNEWSESSION);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getSession()->isNewSession(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_ISNEWSESSION, var_value);
				}
			}else if (string == CSP_SYS_VAR_ISINVALIDATE)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_ISINVALIDATE);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getSession()->isInvalidate(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_ISINVALIDATE, var_value);
				}
			}else if (string == CSP_SYS_VAR_SESSIONID)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_SESSIONID);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA(request->getSession()->getId(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_SESSIONID, var_value);
				}
			}else if (string == CSP_SYS_VAR_SESSIONCREATIONTIME)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_SESSIONCREATIONTIME);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA((mycp::bigint)request->getSession()->getCreationTime(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_SESSIONCREATIONTIME, var_value);
				}
			}else if (string == CSP_SYS_VAR_SESSIONLASTACCESSED)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_SESSIONLASTACCESSED);
				if (var_value.get() == NULL)
				{
					var_value = CGC_VALUEINFOA((mycp::bigint)request->getSession()->getLastAccessedtime(), cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(CSP_SYS_VAR_SESSIONLASTACCESSED, var_value);
				}
			}else if (string == CSP_SYS_VAR_UPLOADFILES)
			{
				var_value = m_pageParameters->getProperty(CSP_SYS_VAR_UPLOADFILES);
				if (var_value.get() == NULL)
				{
					CUploadFiles::pointer uploadFiles = CUploadFiles::pointer(new CUploadFiles());
					if (request->getUploadFile(uploadFiles->m_files))
					{
						var_value = CGC_VALUEINFO(CGC_OBJECT_CAST<CUploadFiles>(uploadFiles));
						var_value->setInt((int)OBJECT_UPLOADS);
						var_value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
						m_pageParameters->setProperty(CSP_SYS_VAR_UPLOADFILES, var_value);
					}
				}
			}else
			{
				assert (m_sysParameters.get() != NULL);
				cgcValueInfo::pointer var_temp = m_sysParameters->getParameter(string.substr(VTI_INITPARAM.size()));
				if (var_temp.get() != NULL)
				{
					var_value = var_temp->copy();
					var_value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
					m_pageParameters->setProperty(string, var_value);
				}
			}
		}break;
	case VARIABLE_TEMP:
		{
			var_value = m_pageParameters->getProperty(string);
		}break;
	default:
		break;
	}

	if (var_value.get() == NULL && create && var_type == VARIABLE_VALUE)
	{
		var_value = CGC_VALUEINFO(string);
	}

	return var_value;
}

cgcValueInfo::pointer CMycpHttpServer::getScriptValueInfo1(const CScriptItem::pointer & scriptItem, bool create)
{
	assert (scriptItem.get() != NULL);

	cgcValueInfo::pointer value;
	if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Id)
	{
		value = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), create);
	}else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Name)
	{
		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getName(), cgcEmptyTString, create);
		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return cgcNullValueInfo;

		cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
		if (var_property.get() == NULL) return cgcNullValueInfo;

		cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
		cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
		if (appAttributes.get() == NULL) return cgcNullValueInfo;
		cgcValueInfo::pointer var_value = appAttributes->getProperty(var_property->getStr());
		if (var_value.get() != NULL)
			value = var_value->copy();
		//value = appAttributes->getProperty(var_property->getStr());
	}else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Value)
	{
		//value = CGC_VALUEINFO(scriptItem->getValue());
		cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue(), cgcEmptyTString, create);
		if (var_value.get() != NULL)
			value = var_value->copy();
	}

	bool createNew = false;
	if (value.get() == NULL && create)
	{
		createNew = true;
		value = CGC_VALUEINFO("");
	}
	if (value.get() != NULL)
	{
		if (scriptItem->getType() == "int")
		{
			value->totype(cgcValueInfo::TYPE_INT);
		}else if (scriptItem->getType() == "bigint" || scriptItem->getType() == "time")
		{
			value->totype(cgcValueInfo::TYPE_BIGINT);
		//}else if (scriptItem->getType() == "time")
		//{
		//	value->totype(cgcValueInfo::TYPE_TIME);
		}else if (scriptItem->getType() == "boolean")
		{
			value->totype(cgcValueInfo::TYPE_BOOLEAN);
		}else if (scriptItem->getType() == "float")
		{
			value->totype(cgcValueInfo::TYPE_FLOAT);
		}else if (scriptItem->getType() == "vector")
		{
			value->totype(cgcValueInfo::TYPE_VECTOR);
			if (createNew)
				value->reset();
		}else if (scriptItem->getType() == "map")
		{
			value->totype(cgcValueInfo::TYPE_MAP);
			if (createNew)
				value->reset();
		}else
		{
			value->totype(cgcValueInfo::TYPE_STRING);
		}

		if (scriptItem->getProperty() == "const")
		{
			value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
		}
	}
	return value;
}

cgcValueInfo::pointer CMycpHttpServer::getScriptValueInfo2(const CScriptItem::pointer & scriptItem, bool create)
{
	assert (scriptItem.get() != NULL);

	cgcValueInfo::pointer value;
	if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Name)
	{
		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getName(), cgcEmptyTString, create);
		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return cgcNullValueInfo;

		cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty(), cgcEmptyTString, create);
		if (var_property.get() == NULL) return cgcNullValueInfo;

		cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
		cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
		if (appAttributes.get() == NULL) return cgcNullValueInfo;
		cgcValueInfo::pointer var_value = appAttributes->getProperty(var_property->getStr());
		if (var_value.get() == NULL) return cgcNullValueInfo;
		value = var_value->copy();
	}else if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Value)
	{
		//value = CGC_VALUEINFO(scriptItem->getValue());
		//cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue(), scriptItem->getScope(), create);	// ** 这里有问题
		cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue(), cgcEmptyTString, create);
		//printf("**** CSP_Operate_Value script_scope=%s,script_id=%s,script_name=%s,script_value=%s,value=0x%x\n",
		//	scriptItem->getScope().c_str(),scriptItem->getId().c_str(),scriptItem->getName().c_str(),scriptItem->getValue().c_str(),(int)var_value.get());
		if (var_value.get() != NULL)
		{
			value = var_value->copy();
			//printf("**** value_string2=%s\n",value->getStrValue().c_str());
		}
	}else if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Id)
	{
		// support for:
		// <?csp
		// $var1 = $var2
		// ?>
		cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue(), cgcEmptyTString);
		//cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue(), scriptItem->getScope());
		//printf("**** CSP_Operate_Id script_scope=%s,script_id=%s,script_name=%s,script_value=%s,value=0x%x\n",
		//	scriptItem->getScope().c_str(),scriptItem->getId().c_str(),scriptItem->getName().c_str(),scriptItem->getValue().c_str(),(int)var_value.get());
		if (var_value.get() == NULL)
		{
			return cgcNullValueInfo;
		}
		//printf("**** value_string1=%s\n",var_value->getStrValue().c_str());

		if (var_value->getType() == cgcValueInfo::TYPE_VECTOR && !scriptItem->getProperty2().empty())
		//if (var_value->getType() == cgcValueInfo::TYPE_VECTOR && !scriptItem->getProperty().empty())
		{
			const int nIndex = atoi(scriptItem->getProperty2().c_str());
			//int nIndex = atoi(scriptItem->getProperty().c_str());
			const std::vector<cgcValueInfo::pointer>& vectors = var_value->getVector();
			if (nIndex >=0 && nIndex < (int)vectors.size())
			{
				value = vectors[nIndex]->copy();
			}
		}else if (var_value->getType() == cgcValueInfo::TYPE_MAP && !scriptItem->getProperty2().empty())
		//}else if (var_value->getType() == cgcValueInfo::TYPE_MAP && !scriptItem->getProperty().empty())
		{
			// ?? $var_time['timestamp'] = $var_getresponse['ts']; 用于获取后面 ts
			const tstring & sIndex = !scriptItem->getType().empty() ? scriptItem->getType() : scriptItem->getProperty2();
			//const tstring & sIndex = !scriptItem->getType().empty() ? scriptItem->getType() : scriptItem->getProperty();
			cgcValueInfo::pointer findValue;
			if (var_value->getMap().find(sIndex, findValue))
			{
				value = findValue->copy();
			}
		}else
		{
			value = var_value->copy();
		}
		if (value.get() != NULL)
		{
			//printf("**** value_string2=%s\n",value->getStrValue().c_str());
			return value;
		}
	}

	bool createNew = false;
	if (value.get() == NULL && create)
	{
		createNew = true;
		value = CGC_VALUEINFO("");
	}
	if (value.get() != NULL)
	{
		if (scriptItem->getType() == "int")
		{
			value->totype(cgcValueInfo::TYPE_INT);
		}else if (scriptItem->getType() == "bigint" || scriptItem->getType() == "time")
		{
			value->totype(cgcValueInfo::TYPE_BIGINT);
		//}else if (scriptItem->getType() == "time")
		//{
		//	value->totype(cgcValueInfo::TYPE_TIME);
		}else if (scriptItem->getType() == "boolean")
		{
			value->totype(cgcValueInfo::TYPE_BOOLEAN);
		}else if (scriptItem->getType() == "float")
		{
			value->totype(cgcValueInfo::TYPE_FLOAT);
		}else if (scriptItem->getType() == "vector")
		{
			value->totype(cgcValueInfo::TYPE_VECTOR);
			if (createNew)
				value->reset();
		}else if (scriptItem->getType() == "map")
		{
			value->totype(cgcValueInfo::TYPE_MAP);
			if (createNew)
				value->reset();
		}else
		{
			value->totype(cgcValueInfo::TYPE_STRING);
		}

		if (scriptItem->getProperty() == "const")
		{
			value->setAttribute(cgcValueInfo::ATTRIBUTE_READONLY);
		}
	}
	return value;
}

bool CMycpHttpServer::getCompareValueInfo(const CScriptItem::pointer & scriptItem, cgcValueInfo::pointer& compare1_value, cgcValueInfo::pointer& compare2_value)
{
	assert (scriptItem.get() != NULL);

	if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Name)
	{
		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getName());
		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return false;

		cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
		if (var_property.get() == NULL) return false;

		cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
		cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
		if (appAttributes.get() == NULL) return false;
		cgcValueInfo::pointer var_value = appAttributes->getProperty(var_property->getStr());
		if (var_value.get() != NULL)
			compare1_value = var_value->copy();
		//compare1_value = appAttributes->getProperty(var_property->getStr());
		if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Id)
		{
			compare2_value = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			// ? totype
		}else if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Value)
		{
			//compare2_value = CGC_VALUEINFO(scriptItem->getValue());
			compare2_value = getStringValueInfo(scriptItem, scriptItem->getValue());
			if (compare1_value.get() != NULL && compare2_value.get() != NULL)
				compare2_value->totype(compare1_value->getType());
		}else
		{
			return false;
		}

	}else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Id)
	{
		compare1_value = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
		if (compare1_value.get()!=NULL && compare1_value->getType() == cgcValueInfo::TYPE_VECTOR && !scriptItem->getProperty().empty())
		{
			const int nIndex = atoi(scriptItem->getProperty().c_str());
			const std::vector<cgcValueInfo::pointer>& vectors = compare1_value->getVector();
			if (nIndex >=0 && nIndex < (int)vectors.size())
			{
				cgcValueInfo::pointer valuetemp = vectors[nIndex];
				compare1_value = valuetemp;
			}else
			{
				compare1_value.reset();
			}
		}else if (compare1_value.get()!=NULL && compare1_value->getType() == cgcValueInfo::TYPE_MAP && !scriptItem->getProperty().empty())
		{
			const tstring& sIndex = scriptItem->getProperty();
			cgcValueInfo::pointer findValue;
			if (compare1_value->getMap().find(sIndex, findValue))
			{
				compare1_value = findValue;
			}else
			{
				compare1_value.reset();
			}
		}

		if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Name)
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return false;

			cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
			if (var_property.get() == NULL) return false;

			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
			cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
			if (appAttributes.get() == NULL) return false;
			cgcValueInfo::pointer var_value = appAttributes->getProperty(var_property->getStr());
			if (var_value.get() != NULL)
				compare2_value = var_value->copy();
			//compare2_value = appAttributes->getProperty(var_property->getStr());
			// ? totype
		}else if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Value)
		{
			//compare2_value = CGC_VALUEINFO(scriptItem->getValue());
			compare2_value = getStringValueInfo(scriptItem, scriptItem->getValue());
			//printf("******* getCompareValueInfo compare2_value,%s=%x\n",scriptItem->getValue().c_str(),(int)compare2_value.get());
			if (compare1_value.get() != NULL && compare2_value.get() != NULL)
				compare2_value->totype(compare1_value->getType());
		}else if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Id)
		{
			// ** support if ('abcd'==$var) {}
			compare2_value = getStringValueInfo(scriptItem, scriptItem->getValue(), scriptItem->getScope());
		}else
		{
			return false;
		}
	}
	else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Value)
	{
		// ** support if ('abcd'==$var) {}
		//compare1_value = CGC_VALUEINFO(scriptItem->getId());
		compare1_value = getStringValueInfo(scriptItem, scriptItem->getId());
		//printf("******* getCompareValueInfo compare1_value,%s=%x\n",scriptItem->getId().c_str(),(int)compare1_value.get());
		if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Id)
		{
			compare2_value = getStringValueInfo(scriptItem, scriptItem->getValue(), scriptItem->getScope());
			//printf("******* getCompareValueInfo compare2_value,%s=%x\n",scriptItem->getValue().c_str(),(int)compare2_value.get());
			if (compare2_value.get()!=NULL && compare2_value->getType() == cgcValueInfo::TYPE_VECTOR && !scriptItem->getProperty2().empty())
			{
				const int nIndex = atoi(scriptItem->getProperty2().c_str());
				const std::vector<cgcValueInfo::pointer>& vectors = compare2_value->getVector();
				if (nIndex >=0 && nIndex < (int)vectors.size())
				{
					cgcValueInfo::pointer valuetemp = vectors[nIndex];
					compare2_value = valuetemp;
				}else
				{
					compare2_value.reset();
				}
			}else if (compare2_value.get()!=NULL && compare2_value->getType() == cgcValueInfo::TYPE_MAP && !scriptItem->getProperty2().empty())
			{
				const tstring& sIndex = scriptItem->getProperty2();
				cgcValueInfo::pointer findValue;
				if (compare2_value->getMap().find(sIndex, findValue))
				{
					compare2_value = findValue;
				}else
				{
					compare2_value.reset();
				}
			}
			if (compare1_value.get() != NULL && compare2_value.get() != NULL && scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Value)
				compare1_value->totype(compare2_value->getType());
		}
		else if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Name)
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return false;

			cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
			if (var_property.get() == NULL) return false;

			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
			cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
			if (appAttributes.get() == NULL) return false;
			cgcValueInfo::pointer var_value = appAttributes->getProperty(var_property->getStr());
			if (var_value.get() != NULL)
				compare2_value = var_value->copy();
			//compare2_value = appAttributes->getProperty(var_property->getStr());
			// ? totype
		}
		else if (scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_Value)
		{
			//compare2_value = CGC_VALUEINFO(scriptItem->getValue());
			compare2_value = getStringValueInfo(scriptItem, scriptItem->getValue());
			if (compare1_value.get() != NULL && compare2_value.get() != NULL)
				compare2_value->totype(compare1_value->getType());
		}else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

int CMycpHttpServer::doScriptElse(const CScriptItem::pointer & scriptItem)
{
	//printf("******* doScriptElse else.size=%d\n",(int)scriptItem->m_else.size());
	for (size_t i=0; i<scriptItem->m_else.size(); i++)
	{
		const CScriptItem::pointer& subScriptItem = scriptItem->m_else[i];
		const int ret = doScriptItem(subScriptItem);
		//printf("******* %d ret=%d type=%x,id=%s,value=[%s]\n",(int)i,ret,(int)subScriptItem->getItemType(),subScriptItem->getId().c_str(),subScriptItem->getValue().c_str());
		if (ret==-1 || ret==3 || ret==4)
			return ret;
		//else if (ret == 1)
		//{
		//	if (subScriptItem->isItemType(CScriptItem::CSP_Break))
		//	{
		//		return 1;
		//		//break;
		//	}
		//}
	}
	return 0;					// continue;
}
int CMycpHttpServer::doScriptItem(const CScriptItem::pointer & scriptItem)
{
	// -1 : error
	// 0  : continue
	// 1  : break;			<csp:break
	// 2  : re continue;	<csp:continue
	// 3  : page return		<page:return>

	assert (scriptItem.get() != NULL);

	int result = 0;	// Default continue
	switch (scriptItem->getItemType())
	{
	case CScriptItem::CSP_Println:
		{
			response->println(scriptItem->getValue());
		}break;
	case CScriptItem::CSP_OutPrint:
		{
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Id)
			{
				cgcValueInfo::pointer value = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
				if (value.get() != NULL)
				{
					if (value->getType() == cgcValueInfo::TYPE_VECTOR && !scriptItem->getProperty().empty())
					{
						const int nIndex = atoi(scriptItem->getProperty().c_str());
						const std::vector<cgcValueInfo::pointer>& vectors = value->getVector();
						if (nIndex >=0 && nIndex < (int)vectors.size())
						{
							theApplication->log(LOG_INFO, vectors[nIndex]->getStrValue().c_str());
						}else
						{
							theApplication->log(LOG_INFO, "NULL");
							//theApplication->log(LOG_INFO, "");
						}
					}else if (value->getType() == cgcValueInfo::TYPE_MAP && !scriptItem->getProperty().empty())
					{
						const tstring& sIndex = scriptItem->getProperty();
						cgcValueInfo::pointer findValue;
						if (value->getMap().find(sIndex, findValue))
						{
							theApplication->log(LOG_INFO, findValue->getStrValue().c_str());
						}else
						{
							theApplication->log(LOG_INFO, "NULL");
							//theApplication->log(LOG_INFO, "");
						}
					}else
					{
						theApplication->log(LOG_INFO, value->getStrValue().c_str());
					}
				}else
				{
					theApplication->log(LOG_INFO, "NULL");
					//if (getVariableType(scriptItem->getId()) != VARIABLE_REQUESTPARAM)
					//	theApplication->log(LOG_INFO, "");
				}
			}else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Name)
			{
				cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getName());
				if (var_app.get() == NULL)
				{
					theApplication->log(LOG_INFO, "NULL");
					return 0;
				}
				else if (var_app->getType() != cgcValueInfo::TYPE_OBJECT)
				{
					theApplication->log(LOG_INFO, "NOT OBJECT");
					return 0;
				}
				//if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT)
				//{
				//	theApplication->log(LOG_INFO, "");
				//	//response->write("null");
				//	return 0;
				//}

				cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
				if (var_property.get() == NULL)
				{
					theApplication->log(LOG_INFO, "NULL");
					//theApplication->log(LOG_INFO, "");
					return 0;
				}

				cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
				cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
				if (appAttributes.get() == NULL)
				{
					theApplication->log(LOG_INFO, "NULL");
					//theApplication->log(LOG_INFO, "");
					return 0;
				}
				cgcValueInfo::pointer value = appAttributes->getProperty(var_property->getStr());
				if (value.get() != NULL)
				{
					theApplication->log(LOG_INFO, value->getStrValue().c_str());
				}else
				{
					theApplication->log(LOG_INFO, "NULL");
					//theApplication->log(LOG_INFO, "");
				}
			}
			else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Value)
			{
				// ** 支持新版本
				theApplication->log(LOG_INFO, scriptItem->getId().c_str());
				//cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getId());
				//if (var_value.get() != NULL)
				//{
				//	theApplication->log(LOG_INFO, var_value->getStrValue().c_str());
				//}else
				//{
				//	theApplication->log(LOG_INFO, "NULL");
				//}
			}
			else
			{
				cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue());
				if (var_value.get() != NULL)
				{
					theApplication->log(LOG_INFO, var_value->getStrValue().c_str());
				}else
				{
					theApplication->log(LOG_INFO, "NULL");
				}
			}
		}break;
	case CScriptItem::CSP_Write:
		{
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Id)
			{
				//printf("**** CSP_Write CScriptItem::CSP_Operate_Id=%s\n",scriptItem->getValue().c_str());
				cgcValueInfo::pointer value = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
				if (value.get() != NULL)
				{
					if (value->getType() == cgcValueInfo::TYPE_VECTOR && !scriptItem->getProperty().empty())
					{
						int nIndex = atoi(scriptItem->getProperty().c_str());
						const std::vector<cgcValueInfo::pointer>& vectors = value->getVector();
						if (nIndex >=0 && nIndex < (int)vectors.size())
						{
							response->write(vectors[nIndex]->toString());
						}else
						{
							//response->write("null");
							response->write("");
						}
					}else if (value->getType() == cgcValueInfo::TYPE_MAP && !scriptItem->getProperty().empty())
					{
						const tstring & sIndex = scriptItem->getProperty();
						cgcValueInfo::pointer findValue;
						if (value->getMap().find(sIndex, findValue))
						{
							response->write(findValue->toString());
						}else
						{
							response->write("");
							//response->write("null");
						}
					}else
					{
						response->write(value->toString());
					}
				}else
				{
					if (getVariableType(scriptItem->getId()) != VARIABLE_REQUESTPARAM)
						response->write("");
						//response->write("null");
				}
			}
			else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Name)
			{
				//printf("**** CSP_Write CScriptItem::CSP_Operate_Name=%s\n",scriptItem->getValue().c_str());
				cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getName());
				if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT)
				{
					response->write("");
					//response->write("null");
					return 0;
				}

				cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
				if (var_property.get() == NULL)
				{
					response->write("");
					//response->write("null");
					return 0;
				}

				cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
				cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
				if (appAttributes.get() == NULL)
				{
					response->write("");
					//response->write("null");
					return 0;
				}
				cgcValueInfo::pointer value = appAttributes->getProperty(var_property->getStr());
				if (value.get() != NULL)
				{
					 response->write(value->toString());
				}else
				{
					response->write("");
					//response->write("null");
				}
			}
			//else if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_Value)
			//{
			//	// ** 支持新版本（CSP_Write暂时不支持）
			//	response->write(scriptItem->getId());
			//}
			else
			{
				response->write(scriptItem->getValue());
				//cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue());
				//printf("**** CSP_Write else %s=%s\n",scriptItem->getValue().c_str(),var_value->getStrValue().c_str());
				//if (var_value.get() != NULL)
				//{
				//	response->write(var_value->getStr());
				////}else
				////{
				////	printf("**** CSP_Write NULL=%s\n",scriptItem->getValue().c_str());
				//}
			}
		}break;
	case CScriptItem::CSP_NewLine:
		{
			response->newline();
		}break;
	case CScriptItem::CSP_Define:
		{
			const VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;

			cgcValueInfo::pointer value;
			if (scriptItem->getType() == "app")
			{
				cgcValueInfo::pointer var_name = getStringValueInfo(scriptItem, scriptItem->getName());
				if (var_name.get() == NULL) return -1;

				cgcValueInfo::pointer var_initProperty = getStringValueInfo(scriptItem, scriptItem->getProperty());
				if (var_initProperty.get() == NULL)
					var_initProperty = getStringValueInfo(scriptItem, scriptItem->getValue());

				// APP Service
				cgcServiceInterface::pointer serviceInterface = theServiceManager->getService(var_name->getStr(), var_initProperty);
				if (serviceInterface.get() == NULL) return -1;
				value = CGC_VALUEINFO(CGC_OBJECT_CAST<cgcObject>(serviceInterface));
				value->setInt((int)OBJECT_APP);
				value->setStr(var_name->getStr());
#ifdef USES_OBJECT_LIST_APP
				theApplication->getAttributes(true)->addListAttribute((int)OBJECT_APP, value, true);
#else
				theApplication->getAttributes(true)->setProperty((int)OBJECT_APP, value);
#endif
			}else if (scriptItem->getType() == "cdbc")
			{
				cgcValueInfo::pointer var_name = getStringValueInfo(scriptItem, scriptItem->getName());
				if (var_name.get() == NULL) return -1;

				// CDBC Service
				cgcCDBCService::pointer cdbcService = theServiceManager->getCDBDService(var_name->getStr());
				if (cdbcService.get() == NULL) return -1;
				value = CGC_VALUEINFO(CGC_OBJECT_CAST<cgcObject>(cdbcService));
				value->setInt((int)OBJECT_CDBC);
				value->setStr(var_name->getStr());
#ifdef USES_OBJECT_LIST_CDBC
				theApplication->getAttributes(true)->addListAttribute((int)OBJECT_CDBC, value, true);
#else
				theApplication->getAttributes(true)->setProperty((int)OBJECT_CDBC, value);
#endif
			}else
			{
				value = getScriptValueInfo2(scriptItem);
				//printf("**** CSP_Define script_scope=%s,script_id=%s,script_name=%s,script_value=%s,value=0x%x\n",
				//	scriptItem->getScope().c_str(),scriptItem->getId().c_str(),scriptItem->getName().c_str(),scriptItem->getValue().c_str(),(int)value.get());
			}

			cgcAttributes::pointer attributes1 = getAttributes(scriptItem->getScope());
			assert (attributes1.get() != NULL);
			if (attributes1->getProperty(scriptItem->getId(),true).get()!=NULL)
			{
				if (value.get()!=NULL)
					attributes1->setProperty(scriptItem->getId(), value);
				break;
			}
			if (scriptItem->getScope().empty() && scriptItem->getScope()!=CSP_SCOPE_TYPE_SESSION)
			{
				cgcAttributes::pointer attributes = getAttributes(CSP_SCOPE_TYPE_SESSION);
				if (attributes->getProperty(scriptItem->getId(),true).get()!=NULL)
				{
					scriptItem->setScope(CSP_SCOPE_TYPE_SESSION);
					if (value.get()!=NULL)
						attributes->setProperty(scriptItem->getId(), value);
					break;
				}
			}
			if (scriptItem->getScope().empty() && scriptItem->getScope()!=CSP_SCOPE_TYPE_REQUEST)
			{
				cgcAttributes::pointer attributes = getAttributes(CSP_SCOPE_TYPE_REQUEST);
				if (attributes->getProperty(scriptItem->getId(),true).get()!=NULL)
				{
					scriptItem->setScope(CSP_SCOPE_TYPE_REQUEST);
					if (value.get()!=NULL)
						attributes->setProperty(scriptItem->getId(), value);
					break;
				}
			}
			if (scriptItem->getScope().empty() && scriptItem->getScope()!=CSP_SCOPE_TYPE_APPLICATIONL)
			{
				cgcAttributes::pointer attributes = getAttributes(CSP_SCOPE_TYPE_APPLICATIONL);
				if (attributes->getProperty(scriptItem->getId(),true).get()!=NULL)
				{
					scriptItem->setScope(CSP_SCOPE_TYPE_APPLICATIONL);
					if (value.get()!=NULL)
						attributes->setProperty(scriptItem->getId(), value);
					break;
				}
			}
			if (scriptItem->getScope().empty() && scriptItem->getScope()!=CSP_SCOPE_TYPE_SERVER)
			{
				cgcAttributes::pointer attributes = getAttributes(CSP_SCOPE_TYPE_SERVER);
				if (attributes->getProperty(scriptItem->getId(),true).get()!=NULL)
				{
					scriptItem->setScope(CSP_SCOPE_TYPE_SERVER);
					if (value.get()!=NULL)
						attributes->setProperty(scriptItem->getId(), value);
					break;
				}
			}

			//if (!scriptItem->getProperty().empty())
			//{
			//	attributes->getProperty(scriptScrItem);
			//	int nIndex = atoi(scriptItem->getProperty().c_str());
			//}else
			{
				if (value.get()==NULL)
					attributes1->delProperty(scriptItem->getId());
				else
					attributes1->setProperty(scriptItem->getId(), value, true);
			}
		}break;
	case CScriptItem::CSP_Execute:
		{
			cgcValueInfo::pointer var_servicename = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_servicename.get() == NULL) return -1;
			cgcValueInfo::pointer var_function = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_function.get() == NULL) return -1;

			tstring sExecuteResult;
			HTTP_STATUSCODE statusCode = theServiceManager->executeService(var_servicename->getStr(), var_function->getStr(), request, response, sExecuteResult);

			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO((int)statusCode), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO((int)statusCode), true);
			}
		}break;
	case CScriptItem::CSP_AppCall:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != (int)OBJECT_APP) return -1;

			//printf("**** CSP_AppCall, Id=%s, Name=%s\n",scriptItem->getId().c_str(),scriptItem->getName().c_str());
			cgcValueInfo::pointer var_callname = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_callname.get() == NULL) return -1;
			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
			//printf("**** CSP_AppCall, Id=%s, Name=%s, callname=%s, serviceInterface=%x\n",scriptItem->getId().c_str(),scriptItem->getName().c_str(), var_callname->getStrValue().c_str(),(int)serviceInterface.get());
			cgcValueInfo::pointer var_inProperty = getStringValueInfo(scriptItem, scriptItem->getProperty());
			cgcValueInfo::pointer var_outProperty = getStringValueInfo(scriptItem, scriptItem->getValue(), CSP_SCOPE_TYPE_REQUEST);	// ***
			//printf("**** CSP_AppCall, %s=%x\n",scriptItem->getValue().c_str(),(int)var_outProperty.get());
			//cgcValueInfo::pointer var_outProperty = getStringValueInfo(scriptItem, scriptItem->getValue());	// ***
			//if (var_outProperty.get()!=NULL)
			//	var_outProperty->reset();

			bool ret = false;
			try
			{
				// *** 解决调用组件，在线更新，异常问题；
				cgcValueInfo::pointer pVarTemp = CGC_VALUEINFO((int)0);
				if (var_callname->getType() == cgcValueInfo::TYPE_INT)
					ret = serviceInterface->callService(var_callname->getInt(), var_inProperty, pVarTemp);
				else
				{
					var_callname->totype(cgcValueInfo::TYPE_STRING);
					ret = serviceInterface->callService(var_callname->getStr(), var_inProperty, pVarTemp);
				}
				//printf("**** CSP_AppCall, Id=%s, Name=%s, call return=%d,var_outProperty=%x,[%s]\n",scriptItem->getId().c_str(),scriptItem->getName().c_str(),(int)(ret?1:0),(int)var_outProperty.get(),pVarTemp->getStrValue().c_str());
				if (var_outProperty.get()!=NULL)
				{
					var_outProperty->operator =(pVarTemp);
				}else if (var_inProperty.get()!=NULL)
				{
					var_inProperty->operator =(pVarTemp);
				}
				//if (var_callname->getType() == cgcValueInfo::TYPE_INT)
				//	ret = serviceInterface->callService(var_callname->getInt(), var_inProperty, var_outProperty);
				//else
				//{
				//	var_callname->totype(cgcValueInfo::TYPE_STRING);
				//	ret = serviceInterface->callService(var_callname->getStr(), var_inProperty, var_outProperty);
				//}
			}catch(std::exception const &e)
			{
				theApplication->log(LOG_ERROR, _T("exception, callService \'%s\', error %ld\n"), var_callname->getStr().c_str(), GetLastError());
				theApplication->log(LOG_ERROR, _T("'%s'\n"), e.what());
			}catch(...)
			{
				theApplication->log(LOG_ERROR, _T("exception, callService \'%s\', error %ld\n"), var_callname->getStr().c_str(), GetLastError());
			}
			//if (!scriptItem->getValue().empty())
			//{
			//	cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
			//	assert (attributes.get() != NULL);
			//	attributes->delProperty(scriptItem->getValue());
			//	attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(ret));
			//}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO(ret), true);
			}
		}break;
	case CScriptItem::CSP_AppGet:
		{
			//VARIABLE_TYPE var_type = getVariableType(scriptItem->getValue());
			//if (var_type != VARIABLE_USER) return -1;

			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_APP) return -1;

			cgcValueInfo::pointer var_name = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_name.get() == NULL) return -1;

			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
			cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
			cgcValueInfo::pointer var_get;
			if (appAttributes.get() != NULL)
			{
				//cgcValueInfo::pointer outProperty = appAttributes->getProperty(var_name->getStr());
				//m_pageParameters->delProperty(scriptItem->getValue());
				std::vector<cgcValueInfo::pointer> outValues;
				if (appAttributes->getProperty(var_name->getStr(), outValues))
				{
					if (outValues.size() == 1)
					{
						var_get = outValues[0]->copy();
						//m_pageParameters->setProperty(scriptItem->getValue(), outValues[0]->copy());
					}else
					{
						var_get = CGC_VALUEINFO(outValues);
						//m_pageParameters->setProperty(scriptItem->getValue(), CGC_VALUEINFO(outValues));
					}
				}
				//printf("**** CSP_AppGet var_name=%s\n",var_name->getStr().c_str());
			}

			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				if (var_get.get() == NULL)
					attributes->delProperty(scriptItem->getValue());
				else
					attributes->setProperty(scriptItem->getValue(), var_get, true);
			}else
			{
				if (var_get.get() == NULL)
					m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				else
					m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, var_get, true);
			}
		}break;
	case CScriptItem::CSP_AppSet:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_APP) return -1;

			cgcValueInfo::pointer var_name = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_name.get() == NULL) return -1;

			cgcValueInfo::pointer var_inProperty = getStringValueInfo(scriptItem, scriptItem->getProperty());
			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());

			cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
			if (appAttributes.get() != NULL)
			{
				//appAttributes->delProperty(var_name->getStr());
				appAttributes->setProperty(var_name->getStr(), var_inProperty, true);
			}
		}break;
	case CScriptItem::CSP_AppAdd:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_APP) return -1;

			cgcValueInfo::pointer var_name = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_name.get() == NULL) return -1;

			cgcValueInfo::pointer var_inProperty = getStringValueInfo(scriptItem, scriptItem->getProperty());
			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());

			cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
			if (appAttributes.get() != NULL)
			{
				appAttributes->setProperty(var_name->getStr(), var_inProperty);
			}
		}break;
	case CScriptItem::CSP_AppDel:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_APP) return -1;

			cgcValueInfo::pointer var_name = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_name.get() == NULL) return -1;

			//cgcValueInfo::pointer var_inProperty = getStringValueInfo(scriptItem, scriptItem->getProperty());
			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());

			cgcAttributes::pointer appAttributes = serviceInterface->getAttributes();
			if (appAttributes.get() != NULL)
			{
				appAttributes->delProperty(var_name->getStr());
			}
		}break;
	case CScriptItem::CSP_AppInfo:
		{
			//VARIABLE_TYPE var_type = getVariableType(scriptItem->getValue());
			//if (var_type != VARIABLE_USER) return -1;

			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_APP) return -1;

			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
			cgcValueInfo::pointer serviceInfo = serviceInterface->getServiceInfo();

			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), serviceInfo->copy(), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, serviceInfo->copy(), true);
			}
			//m_pageParameters->delProperty(scriptItem->getValue());
			//m_pageParameters->setProperty(scriptItem->getValue(), serviceInfo);
		}break;
	case CScriptItem::CSP_AppInit:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_APP) return -1;

			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
			cgcValueInfo::pointer var_inProperty = getStringValueInfo(scriptItem, scriptItem->getProperty());
			const bool ret = serviceInterface->initService(var_inProperty);
			//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
			m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO(ret), true);
		}break;
	case CScriptItem::CSP_AppFinal:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_APP) return -1;

			cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_app->getObject());
			serviceInterface->finalService();

			cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
			assert (attributes.get() != NULL);
			attributes->delProperty(scriptItem->getId());
		}break;
	//case CScriptItem::CSP_CDBCInit:
	//	{
	//		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
	//		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return -1;
	//		cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

	//		cgcValueInfo::pointer var_inProperty = getStringValueInfo(scriptItem, scriptItem->getProperty());
	//		bool ret = cdbcService->initService(var_inProperty);
	//		cgcValueInfo::pointer _resultValueInfo = m_pageParameters->getProperty(CSP_TEMP_VAR_RESULT);
	//		_resultValueInfo->totype(cgcValueInfo::TYPE_BOOLEAN);
	//		_resultValueInfo->setBoolean(ret);
	//	}break;
	//case CScriptItem::CSP_CDBCFinal:
	//	{
	//		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
	//		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return -1;
	//		cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

	//		cdbcService->finalService();

	//		cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
	//		assert (attributes.get() != NULL);
	//		attributes->delProperty(scriptItem->getId());
	//	}break;
	//case CScriptItem::CSP_CDBCOpen:
	//	{
	//		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
	//		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return -1;
	//		cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

	//		cgcValueInfo::pointer var_name = getStringValueInfo(scriptItem, scriptItem->getName());
	//		if (var_name.get() == NULL) return -1;
	//		cgcCDBCInfo::pointer cdbcInfo = gSystem->getCDBCInfo(var_name->getStr());
	//		if (cdbcInfo.get() == NULL) return -1;

	//		bool ret = cdbcService->open(cdbcInfo);
	//		cgcValueInfo::pointer _resultValueInfo = m_pageParameters->getProperty(CSP_TEMP_VAR_RESULT);
	//		_resultValueInfo->totype(cgcValueInfo::TYPE_BOOLEAN);
	//		_resultValueInfo->setBoolean(ret);
	//	}break;
	//case CScriptItem::CSP_CDBCClose:
	//	{
	//		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
	//		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT) return -1;
	//		cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

	//		cdbcService->close();
	//	}break;
	case CScriptItem::CSP_CDBCExec:
		{
			//VARIABLE_TYPE var_type = getVariableType(scriptItem->getValue());
			//if (var_type != VARIABLE_USER) return -1;
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_sql = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_sql.get() == NULL) return -1;

			const mycp::bigint ret = cdbcService->execute(var_sql->getStr().c_str());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(ret), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO(ret), true);
			}
		}break;
	case CScriptItem::CSP_CDBCSelect:
		{
			const VARIABLE_TYPE var_type = getVariableType(scriptItem->getValue());
			if (var_type != VARIABLE_USER) return -1;

			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_sql = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_sql.get() == NULL) return -1;

			int cdbcCookie = 0;
			const mycp::bigint ret = cdbcService->select(var_sql->getStr().c_str(), cdbcCookie);
			//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
			m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO(ret), true);
			//m_pageParameters->delProperty(scriptItem->getValue());
			//if (ret)
			{
				m_pageParameters->setProperty(scriptItem->getValue(), CGC_VALUEINFO(cdbcCookie), true);
			}
		}break;
	case CScriptItem::CSP_CDBCMoveIndex:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;
			cgcValueInfo::pointer var_index = getStringValueInfo(scriptItem, scriptItem->getProperty());
			if (var_index.get() == NULL) return -1;

			var_index->totype(cgcValueInfo::TYPE_INT);
			cgcValueInfo::pointer var_record = cdbcService->index(var_cdbccookie->getInt(), var_index->getInt());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), var_record, true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, var_record, true);
			}
		}break;
	case CScriptItem::CSP_CDBCFirst:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;

			cgcValueInfo::pointer var_record = cdbcService->first(var_cdbccookie->getInt());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), var_record, true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, var_record, true);
			}
		}break;
	case CScriptItem::CSP_CDBCNext:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;

			cgcValueInfo::pointer var_record = cdbcService->next(var_cdbccookie->getInt());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), var_record, true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, var_record, true);
			}
		}break;
	case CScriptItem::CSP_CDBCPrev:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;

			cgcValueInfo::pointer var_record = cdbcService->previous(var_cdbccookie->getInt());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), var_record, true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, var_record, true);
			}
		}break;
	case CScriptItem::CSP_CDBCLast:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;

			cgcValueInfo::pointer var_record = cdbcService->last(var_cdbccookie->getInt());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), var_record, true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, var_record, true);
			}
		}break;
	case CScriptItem::CSP_CDBCReset:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;

			cdbcService->reset(var_cdbccookie->getInt());
		}break;
	case CScriptItem::CSP_CDBCSize:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;

			const mycp::bigint size = cdbcService->size(var_cdbccookie->getInt());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(size), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_SIZE);
				m_pageParameters->setProperty(CSP_TEMP_VAR_SIZE, CGC_VALUEINFO(size), true);
			}
		}break;
	case CScriptItem::CSP_CDBCIndex:
		{
			cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
			cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

			cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
			if (var_cdbccookie.get() == NULL) return -1;

			const mycp::bigint index = cdbcService->index(var_cdbccookie->getInt());
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(index), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_INDEX);
				m_pageParameters->setProperty(CSP_TEMP_VAR_INDEX, CGC_VALUEINFO(index), true);
			}
		}break;
	//case CScriptItem::CSP_CDBCSetIndex:
	//	{
	//		cgcValueInfo::pointer var_app = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
	//		if (var_app.get() == NULL || var_app->getType() != cgcValueInfo::TYPE_OBJECT || var_app->getInt() != OBJECT_CDBC) return -1;
	//		cgcCDBCService::pointer cdbcService = CGC_OBJECT_CAST<cgcCDBCService>(var_app->getObject());

	//		cgcValueInfo::pointer var_cdbccookie = getStringValueInfo(scriptItem, scriptItem->getName());
	//		if (var_cdbccookie.get() == NULL) return -1;

	//		cgcValueInfo::pointer var_index = getStringValueInfo(scriptItem, scriptItem->getProperty());
	//		if (var_cdbccookie.get() == NULL) return -1;

	//		var_index->totype(cgcValueInfo::TYPE_INT);
	//		bool ret = cdbcService->index(var_cdbccookie->getInt(), var_index->getInt());
	//		if (!scriptItem->getValue().empty())
	//		{
	//			cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
	//			assert (attributes.get() != NULL);
	//			attributes->delProperty(scriptItem->getValue());
	//			attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(ret));
	//		}else
	//		{
	//			m_pageParameters->delProperty(CSP_TEMP_VAR_INDEX);
	//			m_pageParameters->setProperty(CSP_TEMP_VAR_INDEX, CGC_VALUEINFO(ret));
	//		}
	//	}break;
	case CScriptItem::CSP_Add:
		{
			VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;
			cgcValueInfo::pointer var_value = getScriptValueInfo2(scriptItem);
			//if (var_value.get() == NULL)	// *** 由下面判断处理
			//{
			//	break;	// ?
			//	//return -1;
			//}
			cgcValueInfo::pointer var_variable = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_variable.get() == NULL)
			{
				if (var_value.get()!=NULL)
				{
					cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
					assert (attributes.get() != NULL);
					var_variable = var_value->copy();
					attributes->setProperty(scriptItem->getId(), var_variable);
				}
			}else
			{
				//printf("****** type=%d\n",var_variable->getType());
				if (var_variable->getType() == cgcValueInfo::TYPE_VECTOR && !scriptItem->getProperty().empty())
				{
					const int nIndex = atoi(scriptItem->getProperty().c_str());
					std::vector<cgcValueInfo::pointer>& vectors = const_cast<std::vector<cgcValueInfo::pointer>&>(var_variable->getVector());
					if (nIndex < (int)vectors.size())
					{
						if (var_value.get()==NULL)
							vectors[nIndex]->reset();	// ?
						else
							vectors[nIndex] = var_value;
					}else if (var_value.get()!=NULL)
					{
						var_variable->operator +=(var_value);
					}
				}else if (var_variable->getType() == cgcValueInfo::TYPE_MAP && !scriptItem->getProperty().empty())
				{
					const tstring & sIndex = scriptItem->getProperty();
					CLockMap<tstring, cgcValueInfo::pointer>& maps = const_cast<CLockMap<tstring, cgcValueInfo::pointer>&>(var_variable->getMap());
					if (var_value.get()==NULL)
						maps.remove(sIndex);
					else
						maps.insert(sIndex, var_value);
				}else if (var_value.get()!=NULL)
				{
					var_variable->operator +=(var_value);
				}
			}
		}break;
	case CScriptItem::CSP_Subtract:
		{
			VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;
			cgcValueInfo::pointer var_value = getScriptValueInfo2(scriptItem);
			if (var_value.get() == NULL)
			{
				break;	// ?
				//return -1;
			}
			cgcValueInfo::pointer var_variable = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_variable.get() == NULL)
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				var_variable = var_value->copy();
				attributes->setProperty(scriptItem->getId(), var_variable);
			}else
			{
				var_variable->operator -=(var_value);
			}
		}break;
	case CScriptItem::CSP_Multi:
		{
			VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;
			cgcValueInfo::pointer var_value = getScriptValueInfo2(scriptItem);
			if (var_value.get() == NULL)
			{
				break;	// ?
				//return -1;
			}
			cgcValueInfo::pointer var_variable = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_variable.get() == NULL)
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				var_variable = var_value->copy();
				attributes->setProperty(scriptItem->getId(), var_variable);
			}else
			{
				var_variable->operator *=(var_value);
			}
		}break;
	case CScriptItem::CSP_Division:
		{
			VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;
			cgcValueInfo::pointer var_value = getScriptValueInfo2(scriptItem);
			if (var_value.get() == NULL)
			{
				break;	// ?
				//return -1;
			}
			cgcValueInfo::pointer var_variable = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_variable.get() == NULL)
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				var_variable = var_value->copy();
				attributes->setProperty(scriptItem->getId(), var_variable);
			}else
			{
				var_variable->operator /=(var_value);
			}
		}break;
	case CScriptItem::CSP_Modulus:
		{
			VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;
			cgcValueInfo::pointer var_value = getScriptValueInfo2(scriptItem);
			if (var_value.get() == NULL)
			{
				break;	// ?
				//return -1;
			}
			cgcValueInfo::pointer var_variable = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_variable.get() == NULL)
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				var_variable = var_value->copy();
				attributes->setProperty(scriptItem->getId(), var_variable);
			}else
			{
				var_variable->operator %=(var_value);
			}
		}break;
	case CScriptItem::CSP_Increase:
		{
			VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;

			cgcValueInfo::pointer var_variable = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_variable.get() == NULL)
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);

				var_variable = CGC_VALUEINFO((int)0);
				attributes->setProperty(scriptItem->getId(), var_variable);
			}else if (var_variable->getType() != cgcValueInfo::TYPE_INT)
			{
				var_variable->totype(cgcValueInfo::TYPE_INT);
				var_variable->setInt(var_variable->getInt()+1);
			}else
			{
				var_variable->setInt(var_variable->getInt()+1);
			}
		}break;
	case CScriptItem::CSP_Decrease:
		{
			VARIABLE_TYPE var_type = getVariableType(scriptItem->getId());
			if (var_type != VARIABLE_USER) return -1;

			cgcValueInfo::pointer var_variable = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (var_variable.get() == NULL)
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);

				var_variable = CGC_VALUEINFO((int)0);
				attributes->setProperty(scriptItem->getId(), var_variable);
			}else if (var_variable->getType() != cgcValueInfo::TYPE_INT)
			{
				var_variable->totype(cgcValueInfo::TYPE_INT);
				var_variable->setInt(var_variable->getInt()-1);
			}else
			{
				var_variable->setInt(var_variable->getInt()-1);
			}
		}break;
	case CScriptItem::CSP_Empty:
		{
			cgcValueInfo::pointer var_id = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			const bool empty = var_id.get() == NULL ? true : var_id->empty();
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(empty), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO(empty), true);
			}
		}break;
	case CScriptItem::CSP_Reset:
		{
			cgcValueInfo::pointer var_id = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_id.get() != NULL)
			{
				if (var_id->getType() == cgcValueInfo::TYPE_OBJECT)
				{
					if (var_id->getInt() == (int)OBJECT_APP || var_id->getInt() == (int)OBJECT_CDBC)
					{
						// APP&CDBC
						cgcServiceInterface::pointer serviceInterface = CGC_OBJECT_CAST<cgcServiceInterface>(var_id->getObject());
						serviceInterface->finalService();
					}
				}
				var_id->reset();
			}
		}break;
	case CScriptItem::CSP_Sizeof:
		{
			cgcValueInfo::pointer var_id = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			int size = 0;
			
			if (var_id.get() == NULL)
				size = - 1;
			else if (var_id->getType() == cgcValueInfo::TYPE_OBJECT && var_id->getInt() == (int)OBJECT_UPLOADS)
			{
				CUploadFiles::pointer uploadFiles = CGC_OBJECT_CAST<CUploadFiles>(var_id->getObject());
				size = (int)uploadFiles->m_files.size();
			}else
			{
				size = var_id->size();
			}
			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(size), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_SIZE);
				m_pageParameters->setProperty(CSP_TEMP_VAR_SIZE, CGC_VALUEINFO(size), true);
			}
		}break;
	case CScriptItem::CSP_Typeof:
		{
			cgcValueInfo::pointer var_id = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			tstring typeString;
			if (var_id.get() == NULL)
				typeString = "";
				//typeString = "null";
			else if (var_id->getType() == cgcValueInfo::TYPE_OBJECT)
				typeString = var_id->getInt() == 1 ? "app" : "cdbc";
			else
				typeString = var_id->typeString();

			if (!scriptItem->getValue().empty())
			{
				cgcAttributes::pointer attributes = getAttributes(scriptItem->getScope());
				assert (attributes.get() != NULL);
				//attributes->delProperty(scriptItem->getValue());
				attributes->setProperty(scriptItem->getValue(), CGC_VALUEINFO(typeString), true);
			}else
			{
				//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
				m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO(typeString), true);
			}

			//VARIABLE_TYPE var_type = getVariableType(scriptItem->getValue());
			//if (var_type == VARIABLE_USER)
			//{
			//	m_pageParameters->delProperty(scriptItem->getValue());
			//	m_pageParameters->setProperty(scriptItem->getValue(), CGC_VALUEINFO(typeString));
			//}
		}break;
	case CScriptItem::CSP_ToType:
		{
			cgcValueInfo::pointer var_id = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_id.get() != NULL)
			{
				if (var_id->getType() != cgcValueInfo::TYPE_OBJECT)
				{
					cgcValueInfo::ValueType newtype = cgcValueInfo::cgcGetValueType(scriptItem->getType());
					var_id->totype(newtype);
				}
			}
		}break;
	case CScriptItem::CSP_Index:
		{
			cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
			if (var_property.get() == NULL) return -1;
			cgcValueInfo::pointer var_id = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope(), false);
			if (var_id.get() == NULL) break;

			if (var_id->getType() == cgcValueInfo::TYPE_VECTOR)
			{
				if (!scriptItem->getValue().empty())
				{
					VARIABLE_TYPE var_type = getVariableType(scriptItem->getValue());
					if (var_type != VARIABLE_USER) return -1;
				}

				var_property->totype(cgcValueInfo::TYPE_INT);
				int getIndex = var_property->getInt();
				if (getIndex >= 0 && getIndex < var_id->size())
				{
					cgcValueInfo::pointer indexValue = var_id->getVector()[getIndex]->copy();
					if (scriptItem->getValue().empty())
					{
						//m_pageParameters->delProperty(CSP_TEMP_VAR_VALUE);
						m_pageParameters->setProperty(CSP_TEMP_VAR_VALUE, indexValue, true);
					}else
					{
						//m_pageParameters->delProperty(scriptItem->getValue());
						m_pageParameters->setProperty(scriptItem->getValue(), indexValue, true);
					}
				}
			}else if (var_id->getType() == cgcValueInfo::TYPE_MAP)
			{
				if (!scriptItem->getValue().empty())
				{
					VARIABLE_TYPE var_type = getVariableType(scriptItem->getValue());
					if (var_type != VARIABLE_USER) return -1;
				}

				const tstring & sIndex = var_property->toString();
				cgcValueInfo::pointer findValue;
				if (var_id->getMap().find(sIndex, findValue))
				{
					cgcValueInfo::pointer indexValue = findValue->copy();
					if (scriptItem->getValue().empty())
					{
						//m_pageParameters->delProperty(CSP_TEMP_VAR_VALUE);
						m_pageParameters->setProperty(CSP_TEMP_VAR_VALUE, indexValue, true);
					}else
					{
						//m_pageParameters->delProperty(scriptItem->getValue());
						m_pageParameters->setProperty(scriptItem->getValue(), indexValue, true);
					}
				}
			}else if (var_id->getType() == cgcValueInfo::TYPE_OBJECT)
			{
				if (var_id->getObject().get() == NULL) break;

				if (var_id->getInt() == (int)OBJECT_UPLOADS)
				{
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILENAME);
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILEPATH);
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILESIZE);
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILETYPE);
					CUploadFiles::pointer uploadFiles = CGC_OBJECT_CAST<CUploadFiles>(var_id->getObject());

					var_property->totype(cgcValueInfo::TYPE_INT);
					const int getIndex = var_property->getInt();
					if (getIndex >= 0 && getIndex < (int)uploadFiles->m_files.size())
					{
						const cgcUploadFile::pointer& uploadFile = uploadFiles->m_files[getIndex];
						m_pageParameters->setProperty(CSP_TEMP_VAR_FILENAME, CGC_VALUEINFO(uploadFile->getFileName()), true);
						m_pageParameters->setProperty(CSP_TEMP_VAR_FILEPATH, CGC_VALUEINFO(uploadFile->getFilePath()), true);
						m_pageParameters->setProperty(CSP_TEMP_VAR_FILESIZE, CGC_VALUEINFO((mycp::bigint)uploadFile->getFileSize()), true);
						m_pageParameters->setProperty(CSP_TEMP_VAR_FILETYPE, CGC_VALUEINFO(uploadFile->getFileName()), true);
					}else
					{
						m_pageParameters->delProperty(CSP_TEMP_VAR_FILENAME);
						m_pageParameters->delProperty(CSP_TEMP_VAR_FILEPATH);
						m_pageParameters->delProperty(CSP_TEMP_VAR_FILESIZE);
						m_pageParameters->delProperty(CSP_TEMP_VAR_FILETYPE);
					}
				}

			}else if (var_id->getType() == cgcValueInfo::TYPE_STRING)
			{
				// ???

			}
		}break;
	case CScriptItem::CSP_PageContentType:
		{
			cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getType());
			if (var_value.get() != NULL && !var_value->getStr().empty())
			{
				response->setContentType(var_value->getStr());
			}
		}break;
	case CScriptItem::CSP_PageReturn:
		{
			return 3;	// page return
		}break;
	case CScriptItem::CSP_PageReset:
		{
			response->reset();
		}break;
	case CScriptItem::CSP_PageForward:
		{
			cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue());
			if (var_value.get() == NULL || var_value->getStr().empty()) return -1;

			response->forward(var_value->getStr());
			return 3;
		}break;
	case CScriptItem::CSP_PageLocation:
		{
			cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue());
			if (var_value.get() == NULL || var_value->getStr().empty()) return -1;

			HTTP_STATUSCODE statusCode = STATUS_CODE_302;
			cgcValueInfo::pointer var_property = getStringValueInfo(scriptItem, scriptItem->getProperty());
			if (var_property.get() != NULL)
			{
				var_property->totype(cgcValueInfo::TYPE_INT);
				if (var_property->getInt() == 301)
					statusCode = STATUS_CODE_301;
			}

			response->setStatusCode(statusCode);
			response->location(var_value->getStr());
			return 3;
		}break;
	case CScriptItem::CSP_PageInclude:
		{
			cgcValueInfo::pointer var_value = getStringValueInfo(scriptItem, scriptItem->getValue());
			if (var_value.get() == NULL || var_value->getStr().empty()) return -1;

			request->setPageAttributes(m_pageParameters);
			HTTP_STATUSCODE statusCode = theServiceManager->executeInclude(var_value->getStr(), request, response);
			//m_pageParameters->delProperty(CSP_TEMP_VAR_RESULT);
			m_pageParameters->setProperty(CSP_TEMP_VAR_RESULT, CGC_VALUEINFO((int)statusCode), true);
		}break;
	case CScriptItem::CSP_Authenticate:
		{
			response->setStatusCode(STATUS_CODE_401);
		}break;
	case CScriptItem::CSP_Break:
		{
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_None && scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_None)
			{
				// <csp:break>
				return 4;
			}
			cgcValueInfo::pointer compare1_value;
			cgcValueInfo::pointer compare2_value;
			if (!getCompareValueInfo(scriptItem, compare1_value, compare2_value)) return -1;
			if (compare1_value.get() == NULL) return 0; // continue
			return compare1_value->operator == (compare2_value) ? 4 : 0;
		}break;
	case CScriptItem::CSP_Continue:
		{
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_None && scriptItem->getOperateObject2() == CScriptItem::CSP_Operate_None)
			{
				// <csp:continue>
				return 2;	// 2= retart continue
			}
			cgcValueInfo::pointer compare1_value;
			cgcValueInfo::pointer compare2_value;
			if (!getCompareValueInfo(scriptItem, compare1_value, compare2_value)) return -1;
			if (compare1_value.get() == NULL) return 0; // continue back
			return compare1_value->operator == (compare2_value) ? 2 : 0;	// 2= retart continue
		}break;
	case CScriptItem::CSP_Foreach:
		{
			cgcValueInfo::pointer idValueInfo = getStringValueInfo(scriptItem, scriptItem->getId(), scriptItem->getScope());
			if (idValueInfo.get() == NULL) return -1;

			cgcValueInfo::pointer copyValueInfo;
			int foreachSize = 0;
			switch (idValueInfo->getType())
			{
			case cgcValueInfo::TYPE_INT:
			case cgcValueInfo::TYPE_BOOLEAN:
			case cgcValueInfo::TYPE_STRING:
			case cgcValueInfo::TYPE_VALUEINFO:
			case cgcValueInfo::TYPE_POINTER:
				{
					copyValueInfo = idValueInfo->copy();
					foreachSize = 1;
				}break;
			case cgcValueInfo::TYPE_OBJECT:
				{
					if (idValueInfo->getInt() == (int)OBJECT_UPLOADS)
					{
						CUploadFiles::pointer uploadFiles = CGC_OBJECT_CAST<CUploadFiles>(idValueInfo->getObject());
						foreachSize = (int)uploadFiles->m_files.size();
					}else
					{
						copyValueInfo = idValueInfo->copy();
						foreachSize = 1;
					}
				}break;
			case cgcValueInfo::TYPE_VECTOR:
				{
					foreachSize = idValueInfo->getVector().size();
				}break;
			case cgcValueInfo::TYPE_MAP:
				{
					foreachSize = idValueInfo->getMap().size();
				}break;
			default:
				break;
			}
			if (foreachSize < 1) return 0;	// next continue

			cgcValueInfo::pointer _indexValueInfo = m_pageParameters->getProperty(CSP_TEMP_VAR_INDEX);
			cgcValueInfo::pointer _valueValueInfo = m_pageParameters->getProperty(CSP_TEMP_VAR_VALUE);
			CLockMap<tstring, cgcValueInfo::pointer>::const_iterator itermap;
			for (int forIndex=0; forIndex<foreachSize; forIndex++)
			{
				if (idValueInfo->getType() == cgcValueInfo::TYPE_MAP)
				{
					if (forIndex == 0)
						itermap = idValueInfo->getMap().begin();
					else
						itermap++;
					if (itermap == idValueInfo->getMap().end())
						break;

					_indexValueInfo->totype(cgcValueInfo::TYPE_STRING);
					_indexValueInfo->setStr(itermap->first);
				}else
				{
					_indexValueInfo->totype(cgcValueInfo::TYPE_INT);
					_indexValueInfo->setInt(forIndex);
				}

				// Save Index value
				if (idValueInfo->getType() == cgcValueInfo::TYPE_VECTOR)
				{
					copyValueInfo = idValueInfo->getVector()[forIndex];
					if (copyValueInfo.get() == NULL) return -1;
					_valueValueInfo->operator =(copyValueInfo);
				}else if (idValueInfo->getType() == cgcValueInfo::TYPE_MAP)
				{
					copyValueInfo = itermap->second;
					if (copyValueInfo.get() == NULL) return -1;
					_valueValueInfo->operator =(copyValueInfo);
				}else if (idValueInfo->getType() == cgcValueInfo::TYPE_OBJECT && idValueInfo->getInt() == (int)OBJECT_UPLOADS)
				{
					CUploadFiles::pointer uploadFiles = CGC_OBJECT_CAST<CUploadFiles>(idValueInfo->getObject());
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILENAME);
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILEPATH);
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILESIZE);
					//m_pageParameters->delProperty(CSP_TEMP_VAR_FILETYPE);
					const cgcUploadFile::pointer& uploadFile = uploadFiles->m_files[forIndex];
					m_pageParameters->setProperty(CSP_TEMP_VAR_FILENAME, CGC_VALUEINFO(uploadFile->getFileName()), true);
					m_pageParameters->setProperty(CSP_TEMP_VAR_FILEPATH, CGC_VALUEINFO(uploadFile->getFilePath()), true);
					m_pageParameters->setProperty(CSP_TEMP_VAR_FILESIZE, CGC_VALUEINFO((mycp::bigint)uploadFile->getFileSize()), true);
					m_pageParameters->setProperty(CSP_TEMP_VAR_FILETYPE, CGC_VALUEINFO(uploadFile->getContentType()), true);
				}else
				{
					if (copyValueInfo.get() == NULL) return -1;
					_valueValueInfo->operator =(copyValueInfo);
				}

				bool breakIfThan = false;
				for (size_t i=0; i<scriptItem->m_subs.size(); i++)
				{
					const CScriptItem::pointer& subScriptItem = scriptItem->m_subs[i];
					//if (breakIfThan && subScriptItem->getItemType() != CScriptItem::CSP_End)
					//	continue;
					//breakIfThan = false;

					const int ret = doScriptItem(subScriptItem);
					//printf("**** CSP_Foreach, index=%d/%d, %d/%d  doScriptItem=%d,(getItemType=0x%x,id=%s,name=%s,value=%s)\n",
					//	forIndex,foreachSize,i,(int)scriptItem->m_subs.size(),ret,(int)subScriptItem->getItemType(),subScriptItem->getId().c_str(),subScriptItem->getName().c_str(),
					//	subScriptItem->getValue().c_str());
					if (ret == -1)
					{
						OutputScriptItem(subScriptItem);
						return -1;
					//}else if (ret == 1)
					//{
					//	//breakIfThan = true;
					//	//if (subScriptItem->isItemType(CScriptItem::CSP_Break))
					//	{
					//		breakIfThan = true;
					//		break;
					//	}
					}else if (ret == 2)
					{
						break;
					}else if (ret == 3)
						return ret;
					else if (ret == 4)
					{
						breakIfThan = true;
						break;
					}
				}

				if (breakIfThan)
					break;

				// for(...) continue
			}
			return 0;	// next continue
		}break;
	case CScriptItem::CSP_WhileEqual:
	case CScriptItem::CSP_WhileNotEqual:
	case CScriptItem::CSP_WhileGreater:
	case CScriptItem::CSP_WhileGreaterEqual:
	case CScriptItem::CSP_WhileLess:
	case CScriptItem::CSP_WhileLessEqual:
	case CScriptItem::CSP_WhileAnd:
	case CScriptItem::CSP_WhileOr:
	case CScriptItem::CSP_WhileEmpty:			// *一个参数
	case CScriptItem::CSP_WhileNotEmpty:	// *一个参数
	case CScriptItem::CSP_WhileExist:			// *一个参数
	case CScriptItem::CSP_WhileNotExist:	// *一个参数
		{
			cgcValueInfo::pointer _indexValueInfo = m_pageParameters->getProperty(CSP_TEMP_VAR_INDEX);
			int index = 0;
			while (true)
			{
				if (index == MAX_WHILE_TIMERS) return -1;
				_indexValueInfo->setInt(index++);

				cgcValueInfo::pointer compare1_value;
				cgcValueInfo::pointer compare2_value;
				if (scriptItem->getItemType()>=CScriptItem::CSP_WhileEmpty && scriptItem->getItemType()<=CScriptItem::CSP_WhileNotExist)
				{
					compare1_value = getScriptValueInfo1(scriptItem, false);
				}else
				{
					if (!getCompareValueInfo(scriptItem, compare1_value, compare2_value)) return -1;
					if (compare1_value.get() == NULL)
					{
						return 0;	// next continue
					}
				}

				bool compareResult = false;
				switch (scriptItem->getItemType())
				{
				case CScriptItem::CSP_WhileEmpty:
					{
						if (compare1_value.get() == NULL) return -1;
						compareResult = compare1_value->empty()?true:false;
					}break;
				case CScriptItem::CSP_WhileNotEmpty:
					{
						if (compare1_value.get() == NULL) return -1;
						compareResult = compare1_value->empty()?false:true;
					}break;
				case CScriptItem::CSP_WhileExist:
					compareResult = (compare1_value.get() != NULL)?true:false;
					break;
				case CScriptItem::CSP_WhileNotExist:
					compareResult = (compare1_value.get() == NULL)?true:false;
					break;
				case CScriptItem::CSP_WhileEqual:
					compareResult = compare1_value->operator == (compare2_value);
					break;
				case CScriptItem::CSP_WhileNotEqual:
					compareResult = compare1_value->operator != (compare2_value);
					break;
				case CScriptItem::CSP_WhileGreater:
					compareResult = compare1_value->operator > (compare2_value);
					break;
				case CScriptItem::CSP_WhileGreaterEqual:
					compareResult = compare1_value->operator >= (compare2_value);
					break;
				case CScriptItem::CSP_WhileLess:
					compareResult = compare1_value->operator < (compare2_value);
					break;
				case CScriptItem::CSP_WhileLessEqual:
					compareResult = compare1_value->operator <= (compare2_value);
					break;
				case CScriptItem::CSP_WhileAnd:
					compareResult = compare1_value->operator && (compare2_value);
					break;
				case CScriptItem::CSP_WhileOr:
					compareResult = compare1_value->operator || (compare2_value);
					break;
				default:
					return -1;	// error;
				}

				if (!compareResult) return 0;	// next continue;

				// do while...
				bool breakIfThan = false;
				for (size_t i=0; i<scriptItem->m_subs.size(); i++)
				{
					const CScriptItem::pointer& subScriptItem = scriptItem->m_subs[i];
					//if (breakIfThan && subScriptItem->getItemType() != CScriptItem::CSP_End)
					//	continue;

					//breakIfThan = false;
					const int ret = doScriptItem(subScriptItem);
					if (ret == -1)
					{
						OutputScriptItem(subScriptItem);
						return -1;
					//}else if (ret == 1)
					//{
					//	//breakIfThan = true;
					//	//if (subScriptItem->isItemType(CScriptItem::CSP_Break))
					//	{
					//		breakIfThan = true;
					//		break;
					//	}
					}else if (ret == 2)
					{
						break;	// restart continue
					}else if (ret == 3)
						return 3;
					else if (ret==4)
					{
						breakIfThan = true;
						break;
					}

				}

				if (breakIfThan)
					break;
				// while continue
			}
			return 0; // next continue
		}break;
	//case CScriptItem::CSP_WhileEmpty:
	//case CScriptItem::CSP_WhileNotEmpty:
	//case CScriptItem::CSP_WhileExist:
	//case CScriptItem::CSP_WhileNotExist:
	//	{
	//		cgcValueInfo::pointer _indexValueInfo = m_pageParameters->getProperty(CSP_TEMP_VAR_INDEX);
	//		int index = 0;
	//		while (true)
	//		{
	//			if (index == MAX_WHILE_TIMERS) return -1;
	//			_indexValueInfo->setInt(index++);

	//			cgcValueInfo::pointer value = getScriptValueInfo1(scriptItem, false);

	//			bool compareResult = false;
	//			switch (scriptItem->getItemType())
	//			{
	//			case CScriptItem::CSP_WhileEmpty:
	//				{
	//					if (value.get() == NULL) return -1;
	//					compareResult = value->empty();
	//				}break;
	//			case CScriptItem::CSP_WhileNotEmpty:
	//				{
	//					if (value.get() == NULL) return -1;
	//					compareResult = !value->empty();
	//				}break;
	//			case CScriptItem::CSP_WhileExist:
	//				compareResult = value.get() != NULL;
	//				break;
	//			case CScriptItem::CSP_WhileNotExist:
	//				compareResult = value.get() == NULL;
	//				break;
	//			default:
	//				return -1;	// error;
	//			}

	//			if (!compareResult) return 0;	// next continue;

	//			// do while...
	//			bool breakIfThan = false;
	//			for (size_t i=0; i<scriptItem->m_subs.size(); i++)
	//			{
	//				const CScriptItem::pointer& subScriptItem = scriptItem->m_subs[i];
	//				//if (breakIfThan && subScriptItem->getItemType() != CScriptItem::CSP_End)
	//				//	continue;
	//				//breakIfThan = false;
	//				int ret = doScriptItem(subScriptItem);
	//				if (ret == -1)
	//				{
	//					OutputScriptItem(subScriptItem);
	//					return -1;
	//				}else if (ret == 1)
	//				{
	//					//breakIfThan = true;
	//					//if (subScriptItem->isItemType(CScriptItem::CSP_Break))
	//					{
	//						breakIfThan = true;
	//						break;
	//					}
	//				}else if (ret == 2)
	//				{
	//					break;
	//				}else if (ret == 3)
	//					return 3;
	//			}
	//			if (breakIfThan)
	//				break;
	//			// while continue
	//		}
	//		return 0; // next continue
	//	}break;
	case CScriptItem::CSP_IfEqual:
	case CScriptItem::CSP_IfNotEqual:
	case CScriptItem::CSP_IfGreater:
	case CScriptItem::CSP_IfGreaterEqual:
	case CScriptItem::CSP_IfLess:
	case CScriptItem::CSP_IfLessEqual:
	case CScriptItem::CSP_IfAnd:
	case CScriptItem::CSP_IfOr:
	case CScriptItem::CSP_IfEmpty:		// *只有一个参数
	case CScriptItem::CSP_IfNotEmpty:	// *只有一个参数
	case CScriptItem::CSP_IfExist:		// *只有一个参数
	case CScriptItem::CSP_IfNotExist:	// *只有一个参数
		{
			cgcValueInfo::pointer compare1_value;
			cgcValueInfo::pointer compare2_value;
			if (scriptItem->getItemType()>=CScriptItem::CSP_IfEmpty && scriptItem->getItemType()<=CScriptItem::CSP_IfNotExist)
			{
				compare1_value = getScriptValueInfo1(scriptItem, false);
			}else
			{
				if (!getCompareValueInfo(scriptItem, compare1_value, compare2_value)) return -1;
				if (compare1_value.get() == NULL) return 0; // continue
			}

			bool compareResult = false;
			switch (scriptItem->getItemType())
			{
			case CScriptItem::CSP_IfEmpty:
				{
					if (compare1_value.get() == NULL) return -1;
					compareResult = compare1_value->empty()?true:false;
				}break;
			case CScriptItem::CSP_IfNotEmpty:
				{
					if (compare1_value.get() == NULL) return -1;
					compareResult = compare1_value->empty()?false:true;
				}break;
			case CScriptItem::CSP_IfExist:
				compareResult = (compare1_value.get() != NULL)?true:false;
				break;
			case CScriptItem::CSP_IfNotExist:
				compareResult = (compare1_value.get() == NULL)?true:false;
				break;
			case CScriptItem::CSP_IfEqual:
				compareResult = compare1_value->operator == (compare2_value);
				break;
			case CScriptItem::CSP_IfNotEqual:
				compareResult = compare1_value->operator != (compare2_value);
				break;
			case CScriptItem::CSP_IfGreater:
				compareResult = compare1_value->operator > (compare2_value);
				break;
			case CScriptItem::CSP_IfGreaterEqual:
				compareResult = compare1_value->operator >= (compare2_value);
				break;
			case CScriptItem::CSP_IfLess:
				compareResult = compare1_value->operator < (compare2_value);
				break;
			case CScriptItem::CSP_IfLessEqual:
				compareResult = compare1_value->operator <= (compare2_value);
				break;
			case CScriptItem::CSP_IfAnd:
				compareResult = compare1_value->operator && (compare2_value);
				break;
			case CScriptItem::CSP_IfOr:
				compareResult = compare1_value->operator || (compare2_value);
				break;
			default:
				return 0;
			}

			//if (!compareResult) return 0;	// continue;
			//result = 1;						// do and break

			if (compareResult)
			{
				//bool breakIfThan = false;
				//printf("******* CSP_IfEqual compareResult, subs.size=%d\n",(int)scriptItem->m_subs.size());
				for (size_t i=0; i<scriptItem->m_subs.size(); i++)
				{
					const CScriptItem::pointer& subScriptItem = scriptItem->m_subs[i];
					//if (breakIfThan && subScriptItem->getItemType() != CScriptItem::CSP_End)
					//	continue;

					//breakIfThan = false;
					const int ret = doScriptItem(subScriptItem);
					//printf("******* %d ret=%d type=%x,id=%s,value=[%s]\n",(int)i,ret,(int)subScriptItem->getItemType(),subScriptItem->getId().c_str(),subScriptItem->getValue().c_str());
					if (ret==-1 || ret==3 || ret==4) return ret;
					//else if (ret == 1)
					//{
					//	//breakIfThan = true;
					//	if (subScriptItem->isItemType(CScriptItem::CSP_Break))
					//	{
					//		return 4;	// break;
					//		//break;
					//	}
					//	//break;
					//}
				}
			}else
			{
				bool breakIfThan = false;	// 其实是else if 比较成功返回
				for (size_t i=0; i<scriptItem->m_elseif.size(); i++)
				{
					const CScriptItem::pointer& subScriptItem = scriptItem->m_elseif[i];
					const int ret = doScriptItem(subScriptItem);
					if (ret==-1 || ret==3 || ret==4) return ret;
					else if (ret == 1)
					{
						//if (subScriptItem->isItemType(CScriptItem::CSP_Break))
						//{
						//	return 4;
						//	//breakIfThan = true;
						//	//break;
						//}
						if (subScriptItem->getItemType()>=CScriptItem::CSP_IfEqual && scriptItem->getItemType()<=CScriptItem::CSP_IfNotExist)
						{
							breakIfThan = true;	// 退出后，下面不执行 else
							break;
						}
					}
				}

				if (!breakIfThan)
				{
					const int ret = doScriptElse(scriptItem);
					if (ret==-1 || ret==3) return ret;
					//for (size_t i=0; i<scriptItem->m_else.size(); i++)
					//{
					//	int ret = doScriptItem(scriptItem->m_else[i]);
					//	if (ret == -1)
					//		return -1;
					//	else if (ret == 1)
					//		break;
					//	else if (ret == 3)
					//		return 3;
					//}
				}
			}

			//return 0;	// continue back
			return compareResult ? 1 : 0;	// 比较返回 1，实现 else if 上层可以判断，然后不执行最后的 else
		}break;
	//case CScriptItem::CSP_IfEmpty:
	//	{
	//		cgcValueInfo::pointer value = getScriptValueInfo1(scriptItem, false);
	//		if(value.get() == NULL) return -1;
	//		if (!value->empty())
	//		{
	//			return doScriptElse(scriptItem);
	//		}
	//		result = 1;						// do and break
	//	}break;
	//case CScriptItem::CSP_IfNotEmpty:
	//	{
	//		cgcValueInfo::pointer value = getScriptValueInfo1(scriptItem, false);
	//		if(value.get() == NULL) return -1;
	//		if (value->empty())
	//		{
	//			return doScriptElse(scriptItem);
	//		}
	//		result = 1;						// do and break
	//	}break;
	//case CScriptItem::CSP_IfExist:
	//	{
	//		//printf("**** CScriptItem::CSP_IfExist...,subs=%d,else=%d,elseif=%d\n",(int)scriptItem->m_subs.size(),(int)scriptItem->m_else.size(),(int)scriptItem->m_elseif.size());
	//		cgcValueInfo::pointer value = getScriptValueInfo1(scriptItem, false);
	//		//printf("**** CScriptItem::CSP_IfExist... %s=%x\n",scriptItem->getId().c_str(), (int)value.get());
	//		if(value.get() != NULL)
	//			result = 1;					// do and break
	//		else
	//		{
	//			return doScriptElse(scriptItem);
	//			//for (size_t i=0; i<scriptItem->m_else.size(); i++)
	//			//{
	//			//	int ret = doScriptItem(scriptItem->m_else[i]);
	//			//	if (ret == -1)
	//			//		return -1;
	//			//	else if (ret == 1)
	//			//		break;
	//			//	else if (ret == 3)
	//			//		return 3;
	//			//}
	//			//return 0;					// continue;
	//		}
	//	}break;
	//case CScriptItem::CSP_IfNotExist:
	//	{
	//		cgcValueInfo::pointer value = getScriptValueInfo1(scriptItem, false);
	//		if(value.get() == NULL)
	//			result = 1;					// do and break
	//		else
	//		{
	//			return doScriptElse(scriptItem);
	//		}
	//	}break;
	case CScriptItem::CSP_IfElse:
		//printf("**** CScriptItem::CSP_IfElse...,subs=%d,else=%d,elseif=%d\n",(int)scriptItem->m_subs.size(),(int)scriptItem->m_else.size(),(int)scriptItem->m_elseif.size());
		break;
	case CScriptItem::CSP_End:
		return 0;		// continue;
	default:
		break;
	}

	for (size_t i=0; i<scriptItem->m_subs.size(); i++)
	{
		const CScriptItem::pointer& subScriptItem = scriptItem->m_subs[i];
		const int ret = doScriptItem(subScriptItem);
		if (ret == -1)
		{
			OutputScriptItem(scriptItem->m_subs[i]);
			return -1;
		// ??
		//}else if (ret == 1)
		//{
		//	if (subScriptItem->isItemType(CScriptItem::CSP_Break))
		//		break;
		}else if (ret == 3 || ret==4)
			return ret;
	}

	return result;
}

void CMycpHttpServer::OutputScriptItem(const CScriptItem::pointer & scriptItem)
{
	assert (scriptItem.get() != NULL);
	response->println("[ERROR]: Type=%x; ID=%s; Name=%s; Property=%s; Value=%s", (int)scriptItem->getItemType(),scriptItem->getId().c_str(), scriptItem->getName().c_str(), scriptItem->getProperty().c_str(), scriptItem->getValue().c_str());
}

