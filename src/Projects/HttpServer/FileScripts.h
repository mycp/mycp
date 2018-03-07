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

// FileScripts.h file here
#ifndef __FileScripts_head__
#define __FileScripts_head__

//#ifdef WIN32
//	typedef unsigned __int64	ubigint;
//#else
//	typedef unsigned long long	ubigint;
//#endif

#include <CGCBase/cgcobject.h>
#include "tabconst.h"
#include "ScriptItem.h"

namespace mycp {

class CCSPFileInfo
{
public:
	typedef boost::shared_ptr<CCSPFileInfo> pointer;

	bool isModified(ubigint filesize, time_t lastTime) {return m_fileSize != filesize || m_lastTime != lastTime;}

	const std::string& getFileName(void) const {return m_fileName;}
	void setFileSize(ubigint v) {m_fileSize = v;}
	ubigint getFileSize(void) const {return m_fileSize;}
	time_t getLastTime(void) const {return m_lastTime;}
	void setLastTime(time_t v) {m_lastTime = v;}

	time_t m_tRequestTime;
	CCSPFileInfo(const std::string& filename, ubigint filesize, time_t lastTime)
		: m_fileName(filename), m_fileSize(filesize), m_lastTime(lastTime)
	{
		m_tRequestTime = time(0);
	}
private:
	std::string m_fileName;
	ubigint m_fileSize;
	time_t m_lastTime;
};

#define CSP_FILEINFO(filename,filesize,lasttime) CCSPFileInfo::pointer(new CCSPFileInfo(filename, filesize, lasttime))


class CFileScripts
{
public:
	typedef boost::shared_ptr<CFileScripts> pointer;

	typedef enum ScriptObject_Type
	{
		TYPE_NONE			= 0x0
		, TYPE_ID			= 0x1
		, TYPE_SCOPE		= 0x2
		, TYPE_VALUE		= 0x4
		, TYPE_NAME			= 0x8
		, TYPE_PROPERTY		= 0x10
		, TYPE_TYPE			= 0x20
		, TYPE_IN			= 0x40
		, TYPE_OUT			= 0x80
		//, TYPE_SQL			= 0x100
		, TYPE_INDEX		= 0x200
		, TYPE_URL			= 0x400
		, TYPE_FUNCTION		= 0x800
		, TYPE_SRC			= 0x1000


		, TYPE_APPCALL		= TYPE_ID|TYPE_SCOPE|TYPE_NAME|TYPE_IN|TYPE_OUT
		, TYPE_DEFAULT		= TYPE_ID|TYPE_SCOPE|TYPE_VALUE|TYPE_NAME|TYPE_PROPERTY|TYPE_TYPE
		, TYPE_ALL			= 0xFFFF


	}SCRIPTOBJECT_TYPE;

	static bool sotpCompare(const char * pBuffer, const char * pCompare, int & leftIndex,bool bSupportEnterChar=true);

private:
	std::string m_fileName;
	std::vector<CScriptItem::pointer> m_scripts;
	bool m_bCodeBegin;

public:
	bool parserCSPFile(const char * filename);

	void addScript(const CScriptItem::pointer& v) {m_scripts.push_back(v);}
	const std::vector<CScriptItem::pointer>& getScripts(void) const {return m_scripts;}
	bool empty(void) const {return m_scripts.empty();}

	void reset(void) {m_scripts.clear();}

private:
	bool parserCSPContent(const char * cspContent);

	bool getCSPObject(const char * pLineBuffer, const CScriptItem::pointer& scriptItem, int & leftIndex, int scriptObjectType = TYPE_DEFAULT);
	void getCSPScriptObject1(char * lpszBufferLine, int bufferSize, const CScriptItem::pointer& scriptItem);
	void getCSPScriptObject2(char * lpszBufferLine, int bufferSize, const CScriptItem::pointer& scriptItem);

	// <csp:equal
	// <csp:greater
	// ...
	// <csp:else>
	// <csp:end>
	bool getCSPIF(const char * pLineBuffer, const CScriptItem::pointer& scriptItem, int & leftIndex, int scriptObjectType = TYPE_DEFAULT, bool bEndReturn=false);
	bool isRootScriptItem(const CScriptItem::pointer& scriptItem) const;
	const char * parseOneLine(const char * pLineBuffer, const CScriptItem::pointer& scriptItem = NullScriptItem, bool bEndReturn=false);

	bool addMethodScriptItem(const CScriptItem::pointer& scriptItem, CScriptItem::ItemType scriptItemType, const std::string & sMethodParam, 
		int needParamCount, bool popLastScript, const std::string & sValue = "");
	void addScriptItem(std::vector<CScriptItem::pointer>& addTo, const CScriptItem::pointer& scriptItem) const;


	// ' ','\t'
	bool isInvalidateChar(char in) const;
	bool strGetValue(const char * pBuffer, char * outBuffer, int & leftIndex, int * outWordLen = 0) const;

public:
	CFileScripts(const std::string& filename);
	virtual ~CFileScripts(void);
	//time_t m_tRequestTime;
};

const CFileScripts::pointer NullFileScripts;

} // namespace mycp

#endif // __FileScripts_head__
