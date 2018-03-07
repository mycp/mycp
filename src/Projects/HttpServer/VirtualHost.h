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

// VirtualHost.h file here
#ifndef __VirtualHost_h__
#define __VirtualHost_h__

namespace mycp {

class CVirtualHost
{
public:
	typedef boost::shared_ptr<CVirtualHost> pointer;

	const tstring& getHost(void) const {return m_host;}

	void setServerName(const tstring& v) {m_serverName = v;}
	const tstring& getServerName(void) const {return m_serverName;}

	void setDocumentRoot(const tstring& v) {m_documentRoot = v;}
	const tstring& getDocumentRoot(void) const {return m_documentRoot;}
	//void setRootType(int v) {m_rootType = v;}
	//int getRootType(void) const {return m_rootType;}
	void setIndex(const tstring& v) {m_index = v;}
	const tstring& getIndex(void) const {return m_index;}

	void setPropertys(cgcAttributes::pointer v) {m_propertys = v;}
	cgcAttributes::pointer getPropertys(void) const {return m_propertys;}
	void delProperty(const tstring& key)
	{
		if (m_propertys.get()!=NULL)
			m_propertys->delProperty(key);
	}
	bool m_bBuildDocumentRoot;

	CVirtualHost(const tstring& host)
		: m_host(host), m_serverName("")
		, m_documentRoot(""), /*m_rootType(0), */m_index("index.csp")
		, m_bBuildDocumentRoot(false)
	{}
	virtual ~CVirtualHost(void)
	{
		if (m_propertys.get() != NULL)
		{
			m_propertys->cleanAllPropertys();
			m_propertys->clearAllAtrributes();
		}
	}
private:
	tstring m_host;
	tstring m_serverName;
	tstring m_documentRoot;
	//int		m_rootType;
	tstring m_index;

	cgcAttributes::pointer m_propertys;
};
const CVirtualHost::pointer NullVirtualHost;

#define VIRTUALHOST_INFO(h) CVirtualHost::pointer(new CVirtualHost(h))

class CFastcgiInfo
{
public:
	typedef boost::shared_ptr<CFastcgiInfo> pointer;
	static CFastcgiInfo::pointer create(const std::string& sName, const std::string& sUrl)
	{
		return CFastcgiInfo::pointer(new CFastcgiInfo(sName, sUrl));
	}
	std::string m_sScriptName;	// php,...
	std::string m_sFastcgiPass;	// 127.0.0.1:9000
	std::string m_sFastcgiIndex;	// index.php
	tstring m_sFastcgiPath;		// php-cgi.exe
	int m_nResponseTimeout;
	int m_nMinProcess;
	int m_nMaxProcess;
	int m_nMaxRequestRestart;

	std::string m_sFcgiPassIp;
	int m_nFcgiPassPort;
	boost::shared_mutex m_mutex; 
	int m_nCurrentFcgiPassPortIndex;

	CFastcgiInfo(void){}
	CFastcgiInfo(const std::string& sName, const std::string& sPass)
		: m_sScriptName(sName), m_sFastcgiPass(sPass)
		, m_nResponseTimeout(30)
		, m_nMinProcess(5), m_nMaxProcess(20), m_nMaxRequestRestart(500)
		, m_nFcgiPassPort(0), m_nCurrentFcgiPassPortIndex(0)
	{}
};
const CFastcgiInfo::pointer NullFastcgiInfo;

} // namespace mycp

#endif // __VirtualHost_h__
