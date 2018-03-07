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

// tabconst.h file here
#ifndef __tabconst__head__
#define __tabconst__head__

// cgc head
#include <string>

namespace mycp {

typedef enum Variable_Type
{
	VARIABLE_USER
	, VARIABLE_SYSTEM
	, VARIABLE_INITPARAM
	, VARIABLE_REQUESTPARAM
	, VARIABLE_HEADER
	, VARIABLE_TEMP
	, VARIABLE_APP
	, VARIABLE_DATASOURCE
	, VARIABLE_VALUE		// Not a variable

}VARIABLE_TYPE;

// 最大循环次数
const int MAX_WHILE_TIMERS	= 1000;

const std::string CSP_SCOPE_TYPE_APPLICATIONL				= "application";
const std::string CSP_SCOPE_TYPE_SESSION						= "session";
const std::string CSP_SCOPE_TYPE_REQUEST						= "request";
const std::string CSP_SCOPE_TYPE_SERVER							= "server";
const std::string CSP_SCOPE_TYPE_PAGE								= "page";

// 变量类型标识 Variable Type Identiry
const std::string VTI_USER			= "$";
const std::string VTI_SYSTEM		= "S$";
const std::string VTI_INITPARAM		= "I$";
const std::string VTI_REQUESTPARAM	= "P$";	// HTTP and request parameter
const std::string VTI_HEADER		= "H$";
const std::string VTI_TEMP			= "_$";
const std::string VTI_APP			= "A$";
const std::string VTI_DATASOURCE	= "D$";

// 临时变量 Temporary Variable

const std::string CSP_TEMP_VAR_INDEX		= "_$index";		// int
const std::string CSP_TEMP_VAR_VALUE		= "_$value";
const std::string CSP_TEMP_VAR_SIZE			= "_$size";
const std::string CSP_TEMP_VAR_RESULT		= "_$result";
const std::string CSP_TEMP_VAR_FILENAME		= "_$filename";
const std::string CSP_TEMP_VAR_FILEPATH		= "_$filepath";
const std::string CSP_TEMP_VAR_FILESIZE		= "_$filesize";
const std::string CSP_TEMP_VAR_FILETYPE		= "_$filetype";

// 系统变量 System Variable
const std::string CSP_SYS_VAR_HEADNAMES		= "S$HeadNames";		// vector, HEAD,...
const std::string CSP_SYS_VAR_HEADVALUES	= "S$HeadValues";	// vector, VALUE,...
const std::string CSP_SYS_VAR_HEADS			= "S$Heads";			// vector, HEAD: VALUE,...
const std::string CSP_SYS_VAR_PARAMNAMES	= "S$ParamNames";	// vector, ParameterName,...
const std::string CSP_SYS_VAR_PARAMVALUES	= "S$ParamValues";	// vector, ParameterValue,...
const std::string CSP_SYS_VAR_PARAMS		= "S$Params";		// vector, ParameterName=ParameterValue,...

const std::string CSP_SYS_VAR_SCHEME		= "S$Scheme";		// "http","https","tcp",...
const std::string CSP_SYS_VAR_METHOD		= "S$Method";		// int, 1: GET, 2: POST
const std::string CSP_SYS_VAR_METHODSTRING	= "S$MethodString";	// string, 1: GET, 2: POST
const std::string CSP_SYS_VAR_PROTOCOL		= "S$Protocol";		// "HTTP/1.1"
const std::string CSP_SYS_VAR_CONTENTLENGTH	= "S$ContentLength";	// int
const std::string CSP_SYS_VAR_CONTENTTYPE	= "S$ContentType";	// "text/html",...

const std::string CSP_SYS_VAR_REQUESTURL	= "S$RequestURL";	// "/path/file.csp?p=v&..."
const std::string CSP_SYS_VAR_REQUESTURI	= "S$RequestURI";	// "/path/file.csp"
const std::string CSP_SYS_VAR_QUERYSTRING	= "S$QueryString";	// "p=v&..."

const std::string CSP_SYS_VAR_REMOTEIP		= "S$RemoteIp";		//
const std::string CSP_SYS_VAR_REMOTEADDR	= "S$RemoteAddr";		//
const std::string CSP_SYS_VAR_REMOTEHOST	= "S$RemoteHost";		//
const std::string CSP_SYS_VAR_AUTHACCOUNT	= "S$AuthAccount";	//
const std::string CSP_SYS_VAR_AUTHSECURE	= "S$AuthSecure";		//

const std::string CSP_SYS_VAR_SERVERNAME	= "S$ServerName";	//
const std::string CSP_SYS_VAR_SERVERPORT	= "S$ServerPort";	//
const std::string CSP_SYS_VAR_CONTEXTPATH	= "S$ContextPath";	//
const std::string CSP_SYS_VAR_SERVLETNAME	= "S$ServletName";	//

const std::string CSP_SYS_VAR_ISNEWSESSION			= "S$IsNewSession";	//
const std::string CSP_SYS_VAR_ISINVALIDATE			= "S$IsInvalidate";	//
const std::string CSP_SYS_VAR_SESSIONID				= "S$SessionId";	//
const std::string CSP_SYS_VAR_SESSIONCREATIONTIME	= "S$SessionCreationTime";	//
const std::string CSP_SYS_VAR_SESSIONLASTACCESSED	= "S$SessionLastAccessed";	//

const std::string CSP_SYS_VAR_UPLOADFILES			= "S$UploadFiles";


// 标签 TAB

const std::string CSP_TAB_COMMENT1_BEGIN		= "{{{";		// {{{ }}} CSP 注释 - 直接打印显示
const std::string CSP_TAB_COMMENT1_END			= "}}}";
const std::string CSP_TAB_COMMENT2_BEGIN		= "<%--";		// <%-- --%> CSP 注释 - 不做任何处理
const std::string CSP_TAB_COMMENT2_END			= "--%>";

const std::string CSP_TAB_OUTPUT_BEGIN			= "<%=";
const std::string CSP_TAB_OUTPUT_END			= "%>";

const std::string CSP_TAB_CODE_BEGIN			= "<?csp";
const std::string CSP_TAB_CDDE_END				= "?>";
const std::string CSP_TAB_CDDE_COMMENT_LINE		= "//";
const std::string CSP_TAB_CDDE_COMMENT_BEGIN	= "/*";
const std::string CSP_TAB_CDDE_COMMENT_END		= "*/";

const std::string CSP_TAB_DEFINE				= "<csp:define";
const std::string CSP_TAB_WRITE					= "<csp:write";
const std::string CSP_TAB_OUT_PRINT				= "<csp:out.print";


const std::string CSP_TAB_EQUAL					= "<csp:=";		// define
const std::string CSP_TAB_ADD					= "<csp:+=";
const std::string CSP_TAB_SUBTRACT				= "<csp:-=";
const std::string CSP_TAB_MULTI					= "<csp:*=";
const std::string CSP_TAB_DIVISION				= "<csp:/=";
const std::string CSP_TAB_MODULUS				= "<csp:%=";
const std::string CSP_TAB_INCREASE				= "<csp:++";
const std::string CSP_TAB_DECREASE				= "<csp:--";

const std::string CSP_TAB_EMPTY					= "<csp:empty";
const std::string CSP_TAB_RESET					= "<csp:reset";
const std::string CSP_TAB_SIZEOF				= "<csp:sizeof";
const std::string CSP_TAB_TYPEOF				= "<csp:typeof";
const std::string CSP_TAB_TOTYPE				= "<csp:totype";
const std::string CSP_TAB_INDEX					= "<csp:index";

const std::string CSP_TAB_PAGE_CONTENTTYPE		= "<page:contentType";
const std::string CSP_TAB_PAGE_RETURN			= "<page:return>";
const std::string CSP_TAB_PAGE_RESET			= "<page:reset";
const std::string CSP_TAB_PAGE_FORWARD			= "<page:forward";
const std::string CSP_TAB_PAGE_LOCATION			= "<page:location";
const std::string CSP_TAB_PAGE_INCLUDE			= "<page:include";

const std::string CSP_TAB_AUTHENTICATE			= "<csp:authenticate>";

const std::string CSP_TAB_IF_EQUAL				= "<csp:if:==";
const std::string CSP_TAB_IF_NOTEQUAL1			= "<csp:if:!=";
const std::string CSP_TAB_IF_NOTEQUAL2			= "<csp:if:<>";
const std::string CSP_TAB_IF_GREATEREQUAL		= "<csp:if:>=";
const std::string CSP_TAB_IF_GREATER			= "<csp:if:>";
const std::string CSP_TAB_IF_LESSEQUAL			= "<csp:if:<=";
const std::string CSP_TAB_IF_LESS				= "<csp:if:<";
const std::string CSP_TAB_IF_AND				= "<csp:if:&&";
const std::string CSP_TAB_IF_OR					= "<csp:if:||";
const std::string CSP_TAB_IF_EMPTY				= "<csp:if:empty";
const std::string CSP_TAB_IF_NOTEMPTY			= "<csp:if:notEmpty";
const std::string CSP_TAB_IF_EXIST				= "<csp:if:exist";
const std::string CSP_TAB_IF_NOTEXIST			= "<csp:if:notExist";
const std::string CSP_TAB_IF_ELSE				= "<csp:if:else>";
const std::string CSP_TAB_ELSEIF_EQUAL		= "<csp:else:if:==";
const std::string CSP_TAB_ELSEIF_NOTEQUAL1			= "<csp:else:if:!=";
const std::string CSP_TAB_ELSEIF_NOTEQUAL2			= "<csp:else:if:<>";
const std::string CSP_TAB_ELSEIF_GREATEREQUAL		= "<csp:else:if:>=";
const std::string CSP_TAB_ELSEIF_GREATER			= "<csp:else:if:>";
const std::string CSP_TAB_ELSEIF_LESSEQUAL			= "<csp:else:if:<=";
const std::string CSP_TAB_ELSEIF_LESS				= "<csp:else:if:<";
const std::string CSP_TAB_ELSEIF_AND				= "<csp:else:if:&&";
const std::string CSP_TAB_ELSEIF_OR					= "<csp:else:if:||";
const std::string CSP_TAB_ELSEIF_EMPTY				= "<csp:else:if:empty";
const std::string CSP_TAB_ELSEIF_NOTEMPTY			= "<csp:else:if:notEmpty";
const std::string CSP_TAB_ELSEIF_EXIST				= "<csp:else:if:exist";
const std::string CSP_TAB_ELSEIF_NOTEXIST			= "<csp:else:if:notExist";
const std::string CSP_TAB_WHILE_EQUAL			= "<csp:while:==";
const std::string CSP_TAB_WHILE_NOTEQUAL1		= "<csp:while:!=";
const std::string CSP_TAB_WHILE_NOTEQUAL2		= "<csp:while:<>";
const std::string CSP_TAB_WHILE_GREATEREQUAL	= "<csp:while:>=";
const std::string CSP_TAB_WHILE_GREATER			= "<csp:while:>";
const std::string CSP_TAB_WHILE_LESSEQUAL		= "<csp:while:<=";
const std::string CSP_TAB_WHILE_LESS			= "<csp:while:<";
const std::string CSP_TAB_WHILE_AND				= "<csp:while:&&";
const std::string CSP_TAB_WHILE_OR				= "<csp:while:||";
const std::string CSP_TAB_WHILE_EMPTY			= "<csp:while:empty";
const std::string CSP_TAB_WHILE_NOTEMPTY		= "<csp:while:notEmpty";
const std::string CSP_TAB_WHILE_EXIST			= "<csp:while:exist";
const std::string CSP_TAB_WHILE_NOTEXIST		= "<csp:while:notExist";
const std::string CSP_TAB_FOREACH				= "<csp:foreach";
const std::string CSP_TAB_BREAK					= "<csp:break";
const std::string CSP_TAB_CONTINUE				= "<csp:continue";
const std::string CSP_TAB_END					= "<csp:end>";

const std::string CSP_TAB_EXECUTE				= "<csp:execute";
const std::string CSP_TAB_APP_CALL				= "<csp:app:call";
const std::string CSP_TAB_APP_INIT				= "<csp:app:init";
const std::string CSP_TAB_APP_FINAL				= "<csp:app:final";
const std::string CSP_TAB_APP_GET				= "<csp:app:get";
const std::string CSP_TAB_APP_SET				= "<csp:app:set";
const std::string CSP_TAB_APP_ADD				= "<csp:app:add";
const std::string CSP_TAB_APP_DEL				= "<csp:app:del";
const std::string CSP_TAB_APP_INFO				= "<csp:app:info";

const std::string CSP_TAB_METHOD_OUT_PRINT		= "out.print";
const std::string CSP_TAB_METHOD_ECHO			= "echo";
const std::string CSP_TAB_METHOD_FORWARD		= "forward";
const std::string CSP_TAB_METHOD_LOCATION		= "location";
const std::string CSP_TAB_METHOD_INCLUDE		= "include";
const std::string CSP_TAB_METHOD_IS_EMPTY		= "is_empty";
const std::string CSP_TAB_METHOD_SIZEOF			= "sizeof";
const std::string CSP_TAB_METHOD_TYPEOF			= "typeof";
const std::string CSP_TAB_METHOD_TO_TYPE		= "to_type";
const std::string CSP_TAB_METHOD_RESET			= "reset";
const std::string CSP_TAB_CODE_RETURN				= "return";		// page:return
const std::string CSP_TAB_CODE_CONTINUE			= "continue";	// for while/foreach continue
const std::string CSP_TAB_CODE_BREAK				= "break";	// for while/foreach continue
const std::string CSP_TAB_METHOD_EXE_SERVLET	= "exe_servlet";
const std::string CSP_TAB_METHOD_APP_CALL		= "app_call";
const std::string CSP_TAB_METHOD_APP_GET		= "app_get";
const std::string CSP_TAB_METHOD_APP_SET		= "app_set";
const std::string CSP_TAB_METHOD_APP_ADD		= "app_add";
const std::string CSP_TAB_METHOD_APP_DEL		= "app_del";
const std::string CSP_TAB_METHOD_APP_INFO		= "app_info";
const std::string CSP_TAB_METHOD_CDBC_EXEC		= "cdbc_exec";
const std::string CSP_TAB_METHOD_CDBC_SELECT	= "cdbc_select";
const std::string CSP_TAB_METHOD_CDBC_MOVEINDEX	= "cdbc_move_index";
const std::string CSP_TAB_METHOD_CDBC_FIRST		= "cdbc_move_first";
const std::string CSP_TAB_METHOD_CDBC_NEXT		= "cdbc_move_next";
const std::string CSP_TAB_METHOD_CDBC_PREVIOUS	= "cdbc_move_previous";
const std::string CSP_TAB_METHOD_CDBC_LAST		= "cdbc_move_last";
const std::string CSP_TAB_METHOD_CDBC_INDEX		= "cdbc_get_index";
const std::string CSP_TAB_METHOD_CDBC_SIZE		= "cdbc_get_size";
const std::string CSP_TAB_METHOD_CDBC_RESET		= "cdbc_reset";

//const std::string CSP_TAB_CDBC_INIT				= "<csp:cdbc:init";
//const std::string CSP_TAB_CDBC_FINAL			= "<csp:cdbc:final";
//const std::string CSP_TAB_CDBC_OPEN				= "<csp:cdbc:open";
//const std::string CSP_TAB_CDBC_CLOSE			= "<csp:cdbc:close";
const std::string CSP_TAB_CDBC_EXEC				= "<csp:cdbc:exec";
const std::string CSP_TAB_CDBC_SELECT			= "<csp:cdbc:select";
const std::string CSP_TAB_CDBC_MOVEINDEX		= "<csp:cdbc:moveindex";
const std::string CSP_TAB_CDBC_FIRST			= "<csp:cdbc:movefirst";
const std::string CSP_TAB_CDBC_NEXT				= "<csp:cdbc:movenext";
const std::string CSP_TAB_CDBC_PREV				= "<csp:cdbc:moveprev";
const std::string CSP_TAB_CDBC_LAST				= "<csp:cdbc:movelast";
const std::string CSP_TAB_CDBC_SIZE				= "<csp:cdbc:getsize";
const std::string CSP_TAB_CDBC_INDEX			= "<csp:cdbc:getindex";
const std::string CSP_TAB_CDBC_RESET			= "<csp:cdbc:reset";

const std::string CSP_TAB_OBJECT_IN				= "in";
const std::string CSP_TAB_OBJECT_OUT			= "out";
const std::string CSP_TAB_OBJECT_SQL			= "sql";
const std::string CSP_TAB_OBJECT_INDEX			= "index";
const std::string CSP_TAB_OBJECT_URL			= "url";
const std::string CSP_TAB_OBJECT_FUNCTION		= "function";
const std::string CSP_TAB_OBJECT_ID				= "id";
const std::string CSP_TAB_OBJECT_NAME			= "name";
const std::string CSP_TAB_OBJECT_TYPE			= "type";
const std::string CSP_TAB_OBJECT_VALUE			= "value";
const std::string CSP_TAB_OBJECT_SCOPE			= "scope";
//const std::string CSP_TAB_OBJECT_SCOPY			= "scopy";
const std::string CSP_TAB_OBJECT_PROPERTY		= "property";
const std::string CSP_TAB_OBJECT_EQUAL			= "=";
const std::string CSP_TAB_OBJECT_END			= "/>";

const std::string FASTCGI_PARAM_SCRIPT_FILENAME		= "SCRIPT_FILENAME";
const std::string FASTCGI_PARAM_HTTP_COOKIE			= "HTTP_COOKIE";
const std::string FASTCGI_PARAM_HTTP_USER_AGENT		= "HTTP_USER_AGENT";
const std::string FASTCGI_PARAM_REMOTE_ADDR			= "REMOTE_ADDR";
const std::string FASTCGI_PARAM_REMOTE_PORT			= "REMOTE_PORT";
const std::string FASTCGI_PARAM_SERVER_ADDR			= "SERVER_ADDR";
const std::string FASTCGI_PARAM_SERVER_PORT			= "SERVER_PORT";
const std::string FASTCGI_PARAM_SERVER_NAME			= "SERVER_NAME";		// MYCP
const std::string FASTCGI_PARAM_SERVER_SOFTWARE		= "SERVER_SOFTWARE";	// MYCP/1.1
const std::string FASTCGI_PARAM_SERVER_PROTOCOL		= "SERVER_PROTOCOL";
const std::string FASTCGI_PARAM_SCRIPT_NAME			= "SCRIPT_NAME";
const std::string FASTCGI_PARAM_REQUEST_URI			= "REQUEST_URI";
const std::string FASTCGI_PARAM_DOCUMENT_URI		= "DOCUMENT_URI";
const std::string FASTCGI_PARAM_DOCUMENT_ROOT		= "DOCUMENT_ROOT";
const std::string FASTCGI_PARAM_QUERY_STRING		= "QUERY_STRING";
const std::string FASTCGI_PARAM_REQUEST_METHOD		= "REQUEST_METHOD";
const std::string FASTCGI_PARAM_CONTENT_TYPE		= "CONTENT_TYPE";
const std::string FASTCGI_PARAM_CONTENT_LENGTH		= "CONTENT_LENGTH";
const std::string FASTCGI_PARAM_PATH_INFO			= "PATH_INFO";
const std::string FASTCGI_PARAM_GATEWAY_INTERFACE	= "GATEWAY_INTERFACE";
const std::string FASTCGI_PARAM_HTTP_HOST			= "HTTP_HOST";
//const std::string FASTCGI_PARAM_PHP_SELF			= "PHP_SELF";

// trim from start
static inline std::string &ltrim(std::string &s) {
	s.erase(0,s.find_first_not_of(" "));
	return s;
}
// trim from end
static inline std::string &rtrim(std::string &s) {
	s.erase(s.find_last_not_of(" ") + 1);
	return s;
}
static inline std::string &trim(std::string &s) {return ltrim(rtrim(s));} 

} // namespace mycp

#endif // __tabconst__head__
