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
#endif

#include <fstream>
#include <string.h>
#include "FileScripts.h"
#include "pstr.h"

namespace mycp {

void string_replace(std::string & strBig, const std::string & strsrc, const std::string &strdst)
{
	std::string::size_type pos=0;
	std::string::size_type srclen=strsrc.size();
	std::string::size_type dstlen=strdst.size();

	while ((pos=strBig.find(strsrc, pos)) != std::string::npos)
	{
		strBig.replace(pos, srclen, strdst);
		pos += dstlen;
	}
}

CFileScripts::CFileScripts(const std::string& filename)
: m_fileName(filename)
, m_bCodeBegin(false)
//, m_tRequestTime(0)

{
}
CFileScripts::~CFileScripts(void)
{
	m_scripts.clear();
}

bool CFileScripts::parserCSPFile(const char * filename)
{
	std::fstream stream;
	stream.open(filename, std::ios::in);
	//stream.open(filename, std::ios::in|std::ios::binary);
	if (!stream.is_open())
	{
		return false;
	}
	stream.seekg(0, std::ios::end);
	const int nTotal = stream.tellg();
	stream.seekg(0, std::ios::beg);

	char * buffer = new char[nTotal+1];
	memset(buffer, 0, nTotal+1);
	stream.read(buffer, nTotal);
	buffer[nTotal] = '\0';
	stream.close();

	m_scripts.clear();
	const bool ret = parserCSPContent(buffer);
	delete[] buffer;
	return ret;
}

bool CFileScripts::parserCSPContent(const char * cspContent)
{
	if (cspContent == NULL) return false;
	const char * pCurrentLine = cspContent;
	const size_t nSizeSrc = strlen(cspContent);
	size_t nSizeUse = 0;
	do
	{
		pCurrentLine = parseOneLine(pCurrentLine);
		nSizeUse = (size_t)(pCurrentLine-cspContent);	// ** 预防文件结束没有换行
	}while (pCurrentLine != NULL && nSizeUse<nSizeSrc);

	return true;
}

bool CFileScripts::getCSPObject(const char * pLineBuffer, const CScriptItem::pointer& scriptItem, int & leftIndex, int scriptObjectType)
{
	assert (scriptItem.get() !=	NULL);
	if (pLineBuffer == NULL) return false;

	bool result = false;
	char * tempBuffer = new char[MAX_VALUE_STRING_SIZE+1];
	leftIndex = 0;
	while (true)
	{
		int leftIndexTemp = 0;
		int wordLen = 0;

		if ((int)(scriptObjectType & TYPE_ID) == (int)TYPE_ID && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_ID.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_ID.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				break;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_None)
				scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Id);
			else
				scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Id);

			scriptItem->setId(std::string(tempBuffer, wordLen));
		//}else if ((int)(scriptObjectType & TYPE_SCOPE) == (int)TYPE_SCOPE &&
		//	(sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_SCOPE.c_str(), leftIndexTemp) || sotpCompare(pLineBuffer+leftIndex, "scopy", leftIndexTemp)))
		}else if ((int)(scriptObjectType & TYPE_SCOPE) == (int)TYPE_SCOPE && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_SCOPE.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_SCOPE.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				break;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}

			if (wordLen > 0)
				scriptItem->setScope(std::string(tempBuffer, wordLen));
		}else if ((int)(scriptObjectType & TYPE_VALUE) == (int)TYPE_VALUE && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_VALUE.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_VALUE.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				break;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_None)
				scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Value);
			else
				scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Value);

			if (wordLen > 0)
			{
				if (scriptItem->getItemType()==CScriptItem::CSP_OutPrint)	// *** 支持新版本
				//if (scriptItem->getItemType()==CScriptItem::CSP_OutPrint || scriptItem->getItemType()==CScriptItem::CSP_Write)	// *** 支持新版本
					scriptItem->setId(std::string(tempBuffer, wordLen));
				else
					scriptItem->setValue(std::string(tempBuffer, wordLen));
			}
		}else if ((int)(scriptObjectType & TYPE_OUT) == (int)TYPE_OUT && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_OUT.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_OUT.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				break;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			//if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_None)
			//	scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Value);
			//else
			//	scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Value);

			// out
			if (wordLen > 0)
				scriptItem->setValue(std::string(tempBuffer, wordLen));
		}else if ((int)(scriptObjectType & TYPE_URL) == (int)TYPE_URL && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_URL.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_URL.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				break;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			// url
			if (wordLen > 0)
				scriptItem->setValue(std::string(tempBuffer, wordLen));
		}else if ((int)(scriptObjectType & TYPE_NAME) == (int)TYPE_NAME && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_NAME.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_NAME.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				break;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_None)
				scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Name);
			else
				scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Name);

			scriptItem->setName(std::string(tempBuffer, wordLen));
		}else if ((int)(scriptObjectType & TYPE_FUNCTION) == (int)TYPE_FUNCTION && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_FUNCTION.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_FUNCTION.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				break;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			if (scriptItem->getOperateObject1() == CScriptItem::CSP_Operate_None)
				scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Name);
			else
				scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Name);

			// function
			scriptItem->setName(std::string(tempBuffer, wordLen));
		}else if ((int)(scriptObjectType & TYPE_PROPERTY) == (int)TYPE_PROPERTY && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_PROPERTY.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_PROPERTY.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				delete[] tempBuffer;
				return false;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			scriptItem->setProperty(std::string(tempBuffer, wordLen));
		}else if ((int)(scriptObjectType & TYPE_IN) == (int)TYPE_IN && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_IN.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_IN.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				delete[] tempBuffer;
				return false;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			// in
			scriptItem->setProperty(std::string(tempBuffer, wordLen));
		}/*else if ((int)(scriptObjectType & TYPE_SQL) == (int)TYPE_SQL && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_SQL.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_SQL.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				delete[] tempBuffer;
				return false;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			// sql
			scriptItem->setProperty(std::string(tempBuffer, wordLen));
		}*/else if ((int)(scriptObjectType & TYPE_INDEX) == (int)TYPE_INDEX && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_INDEX.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_INDEX.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				delete[] tempBuffer;
				return false;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			// index
			scriptItem->setProperty(std::string(tempBuffer, wordLen));
		}else if ((int)(scriptObjectType & TYPE_TYPE) == (int)TYPE_TYPE && sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_TYPE.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_TYPE.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				delete[] tempBuffer;
				return false;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			scriptItem->setType(std::string(tempBuffer, wordLen));
		}/*else if (sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_ATTRIBUTE.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_ATTRIBUTE.size());
			if (!sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_EQUAL.c_str(), leftIndexTemp))
			{
				delete[] tempBuffer;
				return false;
			}
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_EQUAL.size());
			if (!strGetValue(pLineBuffer+leftIndex, tempBuffer, leftIndexTemp, &wordLen))
			{
				break;
			}
			if (wordLen == 0)
			{
				break;
			}
			scriptItem->setAttribute(std::string(tempBuffer, wordLen));
		}*/
		else if (sotpCompare(pLineBuffer+leftIndex, CSP_TAB_OBJECT_END.c_str(), leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+CSP_TAB_OBJECT_END.size());
			result = true;
			break;
		}
		else if (sotpCompare(pLineBuffer+leftIndex, ">", leftIndexTemp))
		{
			leftIndex += (leftIndexTemp+1);
			result = true;
			break;
		}else
		{
			break;
		}
		leftIndex += (leftIndexTemp+wordLen+1);
	}

	delete[] tempBuffer;
	return result;
}

void trimstring(std::string & sString)
{
	if (sString.size() > 1)
	{
		if (sString.c_str()[0] == '\'')
			sString = sString.substr(1);
		if (sString.c_str()[sString.size()-1] == '\'')
			sString = sString.substr(0, sString.size()-1);
	}
}
void trimtstring(tstring & sString)
{
	if (sString.size() > 1)
	{
		if (sString.c_str()[0] == '\'')
			sString = sString.substr(1);
		if (sString.c_str()[sString.size()-1] == '\'')
			sString = sString.substr(0, sString.size()-1);
	}
}
void CFileScripts::getCSPScriptObject1(char * lpszBufferLine, int bufferSize, const CScriptItem::pointer& scriptItem)
{
	if (bufferSize==0) return;
	std::string sValue(lpszBufferLine);
	trim(sValue);					// ** 去掉空格，不可以去掉 ''
	int leftIndexTemp = 0;
	if (sotpCompare(sValue.c_str(), VTI_USER.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_SYSTEM.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_INITPARAM.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_REQUESTPARAM.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_HEADER.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_TEMP.c_str(), leftIndexTemp)
		//sotpCompare(lpszBufferLine, VTI_APP.c_str(), leftIndexTemp) ||
		//sotpCompare(lpszBufferLine, VTI_DATASOURCE.c_str(), leftIndexTemp)
		)
	{
		std::string sId(sValue.c_str());
		std::string::size_type nFindIndex1 = sId.find("[");
		std::string sIndex("");
		if (nFindIndex1 != std::string::npos)
		{
			sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
			trimstring(sIndex);
			scriptItem->setProperty(sIndex);
			scriptItem->setId(sId.substr(0,nFindIndex1));
		}else
		{
			scriptItem->setId(sId);
		}
		scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Id);
	}else
	{
		trimstring(sValue);
		scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Value);
		if (scriptItem->getItemType()==CScriptItem::CSP_Write)
			scriptItem->setValue(sValue);
		else
			scriptItem->setId(sValue);

		//if (lpszBufferLine[0] == '\'' && lpszBufferLine[bufferSize-1] =='\'')
		//{
		//	std::string sValue(lpszBufferLine);
		//	sValue = sValue.substr(1, bufferSize-2);
		//	//scriptItem->setId(sValue);
		//	if (scriptItem->getItemType()==CScriptItem::CSP_Write)
		//		scriptItem->setValue(sValue);
		//	else
		//		scriptItem->setId(sValue);
		//}else
		//{
		//	//scriptItem->setId(lpszBufferLine);
		//	if (scriptItem->getItemType()==CScriptItem::CSP_Write)
		//		scriptItem->setValue(lpszBufferLine);
		//	else
		//		scriptItem->setId(lpszBufferLine);
		//}
	}

	//int leftIndexTemp = 0;
	//if (sotpCompare(lpszBufferLine, VTI_USER.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_SYSTEM.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_INITPARAM.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_REQUESTPARAM.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_HEADER.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_TEMP.c_str(), leftIndexTemp)
	//	//sotpCompare(lpszBufferLine, VTI_APP.c_str(), leftIndexTemp) ||
	//	//sotpCompare(lpszBufferLine, VTI_DATASOURCE.c_str(), leftIndexTemp)
	//	)
	//{
	//	std::string sId(lpszBufferLine);
	//	std::string::size_type nFindIndex1 = sId.find("[");
	//	std::string sIndex("");
	//	if (nFindIndex1 != std::string::npos)
	//	{
	//		sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
	//		trimstring(sIndex);
	//		scriptItem->setProperty(sIndex);
	//		lpszBufferLine[nFindIndex1] = '\0';
	//	}

	//	scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Id);
	//	scriptItem->setId(lpszBufferLine);
	//}else
	//{
	//	// ?
	//	scriptItem->setOperateObject1(CScriptItem::CSP_Operate_Value);
	//	trimtstring(sValue);
	//	if (scriptItem->getItemType()==CScriptItem::CSP_Write)
	//		scriptItem->setValue(lpszBufferLine);
	//	else
	//		scriptItem->setId(lpszBufferLine);

	//	//if (lpszBufferLine[0] == '\'' && lpszBufferLine[bufferSize-1] =='\'')
	//	//{
	//	//	std::string sValue(lpszBufferLine);
	//	//	sValue = sValue.substr(1, bufferSize-2);
	//	//	//scriptItem->setId(sValue);
	//	//	if (scriptItem->getItemType()==CScriptItem::CSP_Write)
	//	//		scriptItem->setValue(sValue);
	//	//	else
	//	//		scriptItem->setId(sValue);
	//	//}else
	//	//{
	//	//	//scriptItem->setId(lpszBufferLine);
	//	//	if (scriptItem->getItemType()==CScriptItem::CSP_Write)
	//	//		scriptItem->setValue(lpszBufferLine);
	//	//	else
	//	//		scriptItem->setId(lpszBufferLine);
	//	//}
	//}
}
void CFileScripts::getCSPScriptObject2(char * lpszBufferLine, int bufferSize, const CScriptItem::pointer& scriptItem)
{
	if (bufferSize==0) return;
	std::string sValue(lpszBufferLine);
	trim(sValue);					// ** 去掉空格，不可以去掉 ''
	int leftIndexTemp = 0;
	if (sotpCompare(sValue.c_str(), VTI_USER.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_SYSTEM.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_INITPARAM.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_REQUESTPARAM.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_HEADER.c_str(), leftIndexTemp) ||
		sotpCompare(sValue.c_str(), VTI_TEMP.c_str(), leftIndexTemp)
		//sotpCompare(sValue.c_str(), VTI_APP.c_str(), leftIndexTemp) ||
		//sotpCompare(sValue.c_str(), VTI_DATASOURCE.c_str(), leftIndexTemp)
		)
	{
		std::string sId(sValue.c_str());
		std::string::size_type nFindIndex1 = sId.find("[");
		std::string sIndex("");
		if (nFindIndex1 != std::string::npos)
		{
			sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
			trimstring(sIndex);
			scriptItem->setProperty2(sIndex);
			lpszBufferLine[nFindIndex1] = '\0';
			scriptItem->setId(sId.substr(0,nFindIndex1));
		}else
		{
			scriptItem->setId(sId);
		}
		scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Id);
	}else
	{
		// ?
		trimstring(sValue);
		scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Value);
		scriptItem->setValue(sValue);
	}

	//int leftIndexTemp = 0;
	//if (sotpCompare(lpszBufferLine, VTI_USER.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_SYSTEM.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_INITPARAM.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_REQUESTPARAM.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_HEADER.c_str(), leftIndexTemp) ||
	//	sotpCompare(lpszBufferLine, VTI_TEMP.c_str(), leftIndexTemp)
	//	//sotpCompare(lpszBufferLine, VTI_APP.c_str(), leftIndexTemp) ||
	//	//sotpCompare(lpszBufferLine, VTI_DATASOURCE.c_str(), leftIndexTemp)
	//	)
	//{
	//	std::string sId(lpszBufferLine);
	//	std::string::size_type nFindIndex1 = sId.find("[");
	//	std::string sIndex("");
	//	if (nFindIndex1 != std::string::npos)
	//	{
	//		sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
	//		trimstring(sIndex);
	//		scriptItem->setProperty2(sIndex);
	//		lpszBufferLine[nFindIndex1] = '\0';
	//	}

	//	scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Id);
	//	scriptItem->setValue(lpszBufferLine);
	//}else
	//{
	//	// ?
	//	scriptItem->setOperateObject2(CScriptItem::CSP_Operate_Value);
	//	if (lpszBufferLine[0] == '\'' && lpszBufferLine[bufferSize-1] =='\'')
	//	{
	//		std::string sValue(lpszBufferLine);
	//		sValue = sValue.substr(1, bufferSize-2);
	//		scriptItem->setValue(sValue);
	//	}else
	//	{
	//		scriptItem->setValue(lpszBufferLine);
	//	}
	//}
}

bool CFileScripts::isRootScriptItem(const CScriptItem::pointer& scriptItem) const
{
	for (size_t i=0; i<m_scripts.size(); i++)
	{
		if (m_scripts[i].get()==scriptItem.get())
			return true;
	}
	return false;
}

#define USES_GETCSP_IF_V2

// <csp:end>
bool CFileScripts::getCSPIF(const char * pLineBuffer, const CScriptItem::pointer& scriptItem, int & leftIndex, int scriptObjectType, bool bEndReturn)
{
	assert (scriptItem.get() !=	NULL);
	if (pLineBuffer == NULL) return false;

	CScriptItem::pointer scriptItemTemp = scriptItem;
	int leftIndexTemp = 0;
	if (m_bCodeBegin)
	{
		// 解析 {} 代码段
		//const std::string s(pLineBuffer,10);
		//printf("**** getCSPIF, s=%s\n",s.c_str());
		if (!sotpCompare(pLineBuffer, "{", leftIndexTemp,true))
		{
			return false;
		}
		leftIndex = leftIndexTemp+1;
		//const std::string s(pLineBuffer + leftIndex, 10);
		//printf("**** getCSPIF, find { s10=%s,leftIndex=%d\n",s.c_str(),leftIndex);
	}
	else
	{
		//const CScriptItem::pointer scriptItemOld = scriptItem;
		if (!getCSPObject(pLineBuffer, scriptItemTemp, leftIndexTemp, scriptObjectType))
		{
			return false;
		}
		leftIndex = leftIndexTemp+1;
	}
	//printf("**** getCSPIF, type=%x, id=%s, %x, size=%d\n", scriptItem->getItemType(),scriptItem->getId().c_str(),(int)scriptItem.get(),(int)m_scripts.size());

	const char * pCurrentLine = pLineBuffer + leftIndex;
	do
	{
		pLineBuffer = pCurrentLine;
		pCurrentLine = parseOneLine(pCurrentLine, scriptItemTemp);
		if (pCurrentLine == NULL) return false;

		if (m_bCodeBegin)
		{
			if (sotpCompare(pCurrentLine, "{", leftIndexTemp))
			{
				//printf("**** find '{' %x,%x\n",(int)scriptItemTemp.get(),(int)scriptItem.get());
				if (scriptItemTemp.get()==scriptItem.get()) return false;
				leftIndexTemp += 1;
				pCurrentLine += leftIndexTemp;
				//const std::string s1(pCurrentLine,10);
				//printf("**** s1=%s,leftIndexTemp=%d\n",s1.c_str(),leftIndexTemp);
			}
			else if (sotpCompare(pCurrentLine, "}", leftIndexTemp))
			{
				leftIndexTemp += 1;
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_End);
				scriptItem->m_subs.push_back(scriptItemNew);

				//const CScriptItem::ItemType nItemType = scriptItem->getItemType();

				// *结束返回
				int ntemp = 0;
				if (!bEndReturn && sotpCompare(pCurrentLine+leftIndexTemp, "else", ntemp))
				{
					leftIndexTemp += (ntemp+4);
					scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfElse);
					//const std::string s1(pCurrentLine+leftIndexTemp,10);
					//printf("**** find '}else' true,s1=%s\n",s1.c_str());
					if (sotpCompare(pCurrentLine+leftIndexTemp, "if", ntemp))
					{
						leftIndexTemp += (ntemp);	// ** 需要保留 'if'二个字符
						//printf("**** AA ELSE IF()\n");
						scriptItemNew->setProperty("elseif");
						scriptItem->m_elseif.push_back(scriptItemNew);
					}else
					{
						//printf("**** AA ELSE\n");
						scriptItem->m_else.push_back(scriptItemNew);
					}
					scriptItemTemp = scriptItemNew;
					pCurrentLine += leftIndexTemp;
					leftIndex += int(pCurrentLine - pLineBuffer);
					//const std::string s(pCurrentLine,10);
					//printf("**** after else s=%s\n",s.c_str());
					continue;
				}else
				{
					//printf("**** find '}' return true\n");
					leftIndex += int(pCurrentLine - pLineBuffer) + leftIndexTemp;
					return true;
				}
				//leftIndex += int(pCurrentLine - pLineBuffer) + leftIndexTemp;
				//return true;
			}
			else if (sotpCompare(pCurrentLine, "else", leftIndexTemp))
			{
				leftIndexTemp += 4;
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfElse);
				//const std::string s1(pCurrentLine+leftIndexTemp,10);
				//printf("**** find '}else' true,s1=%s\n",s1.c_str());
				int ntemp = 0;
				if (sotpCompare(pCurrentLine+leftIndexTemp, "if", ntemp))
				{
					leftIndexTemp += (ntemp);	// ** 需要保留 'if'二个字符
					//printf("**** BB ELSE IF()\n");
					scriptItem->m_elseif.push_back(scriptItemNew);
				}else
				{
					//printf("**** BB ELSE\n");
					scriptItem->m_else.push_back(scriptItemNew);
				}
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				leftIndex += int(pCurrentLine - pLineBuffer);
				//const std::string s(pCurrentLine,10);
				//printf("**** after else s=%s\n",s.c_str());
				continue;
			}
			else
			{
				leftIndexTemp = 0;
			}
		}
		else
		{
			if (sotpCompare(pCurrentLine, CSP_TAB_END.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_END.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_End);
				scriptItem->m_subs.push_back(scriptItemNew);
				//printf("**** CSP_TAB_END, type=%x, %x,%x, size=%d\n", scriptItem->getItemType(),(int)scriptItemTemp.get(),(int)scriptItemOld.get(),(int)m_scripts.size());
				//if (scriptItemTemp.get()==scriptItemOld.get() || scriptItemTemp->isItemType(CScriptItem::CSP_IfElse) || isRootScriptItem(scriptItemOld))
				////if (scriptItemTemp.get()==scriptItemOld.get() || isRootScriptItem(scriptItemOld))
				//{
				//	printf("**** CSP_TAB_END return true\n");
				//	leftIndex += int(pCurrentLine - pLineBuffer) + leftIndexTemp + 1;
				//	return true;
				//}else
				//{
				//	scriptItemTemp = scriptItemOld;
				//	pCurrentLine += leftIndexTemp+1;
				//	leftIndex += int(pCurrentLine - pLineBuffer);
				//	continue;
				//}
				leftIndex += int(pCurrentLine - pLineBuffer) + leftIndexTemp + 1;
				return true;
			}else if (sotpCompare(pCurrentLine, CSP_TAB_IF_ELSE.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_ELSE.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfElse);
				//printf("**** getCSPID CSP_TAB_IF_ELSE,%x \n", (int)scriptItemNew.get());
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				scriptItem->m_else.push_back(scriptItemNew);
				//else
				//	scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_EQUAL.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_EQUAL.size()+1;
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfEqual);
				//printf("**** getCSPID CSP_TAB_IF_EQUAL,%x \n", (int)scriptItemNew.get());
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				pCurrentLine += leftIndexTemp;
#ifdef USES_GETCSP_IF_V2
				if (!getCSPIF(pCurrentLine, scriptItemNew, leftIndexTemp))
#else
				scriptItemTemp = scriptItemNew;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
#endif
				{
					return false;
				}
#ifdef USES_GETCSP_IF_V2
				pCurrentLine += leftIndexTemp;
#else
				pCurrentLine += leftIndexTemp+1;
#endif
				leftIndex += int(pCurrentLine - pLineBuffer);
				//const std::string s1(pCurrentLine, 10);
				//printf("**** after getCSPIF s1=[%s.], leftIndex=%d\n", s1.c_str(),(int)leftIndex);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_EQUAL.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_EQUAL.size()+1;
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfEqual);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_NOTEQUAL.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_NOTEQUAL.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotEqual);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_NOTEQUAL1.c_str(), leftIndexTemp) || 
				sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_NOTEQUAL2.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_NOTEQUAL1.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotEqual);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_GREATEREQUAL.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_GREATEREQUAL.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfGreaterEqual);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_GREATEREQUAL.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_GREATEREQUAL.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfGreaterEqual);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_GREATER.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_GREATER.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfGreater);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_GREATER.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_GREATER.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfGreater);
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_LESSEQUAL.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_LESSEQUAL.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfLessEqual);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_LESSEQUAL.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_LESSEQUAL.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfLessEqual);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_LESS.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_LESS.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfLess);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_LESS.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_LESS.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfLess);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_AND.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_AND.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfAnd);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_AND.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_AND.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfAnd);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_OR.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_OR.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfOr);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_OR.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_OR.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfOr);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_EMPTY.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_EMPTY.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfEmpty);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_EMPTY.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_EMPTY.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfEmpty);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_NOTEMPTY.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_NOTEMPTY.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotEmpty);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_NOTEMPTY.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_NOTEMPTY.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotEmpty);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_EXIST.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_EXIST.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfExist);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_EXIST.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_EXIST.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfExist);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#ifndef USES_GETCSP_IF_V2
			else if (sotpCompare(pCurrentLine, CSP_TAB_IF_NOTEXIST.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_IF_NOTEXIST.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotExist);
				//if (scriptItem->getItemType() >= CScriptItem::CSP_IfEqual && scriptItem->getItemType() <= CScriptItem::CSP_IfElse)
				//	scriptItem->m_elseif.push_back(scriptItemNew);
				//else
				scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
#endif
			else if (sotpCompare(pCurrentLine, CSP_TAB_ELSEIF_NOTEXIST.c_str(), leftIndexTemp))
			{
				leftIndexTemp += CSP_TAB_ELSEIF_NOTEXIST.size();
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotExist);
				scriptItem->m_elseif.push_back(scriptItemNew);
				scriptItemTemp = scriptItemNew;
				pCurrentLine += leftIndexTemp;
				if (!getCSPObject(pCurrentLine, scriptItemNew, leftIndexTemp))
				{
					return false;
				}
				pCurrentLine += leftIndexTemp+1;
				leftIndex += int(pCurrentLine - pLineBuffer);
				continue;
			}
			/*else if (sotpCompare(pCurrentLine, CSP_TAB_IF_EQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_NOTEQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_GREATEREQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_GREATER.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_LESSEQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_LESS.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_EMPTY.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_NOTEMPTY.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_EXIST.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_IF_NOTEXIST.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_EQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_NOTEQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_GREATEREQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_GREATER.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_LESSEQUAL.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_LESS.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_EMPTY.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_NOTEMPTY.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_EXIST.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_WHILE_NOTEXIST.c_str(), leftIndexTemp)
			|| sotpCompare(pCurrentLine, CSP_TAB_FOREACH.c_str(), leftIndexTemp)
			)
			{
			scriptItemTemp.reset();
			leftIndexTemp = 0;
			}*/
			else
			{
				leftIndexTemp = 0;
			}
		}
		leftIndex += int(pCurrentLine - pLineBuffer) + leftIndexTemp;
	}while (pCurrentLine != NULL);

	return true;
}

const std::string & replace(std::string & strSource, const std::string & strFind, const std::string &strReplace)
{
	std::string::size_type pos=0;
	std::string::size_type findlen=strFind.size();
	std::string::size_type replacelen=strReplace.size();
	while ((pos=strSource.find(strFind, pos)) != std::string::npos)
	{
		strSource.replace(pos, findlen, strReplace);
		pos += replacelen;
	}
	return strSource;
}

// "a" -> a
//void strtrim_left(std::string & inOutString)
//{
//	if (!inOutString.empty() && inOutString.c_str()[0] == "\"")
//		inOutString = inOutString.substr(1);
//	if (!inOutString.empty() && inOutString.c_str()[inOutString.size()-1] == "\"")
//		inOutString = sInput.substr(0, inOutString.size()-1);
//}


// "a" -> a
void trim_csp_string(char * lpszBuffer)
{
	// remove ' ' '\t'
	pstrtrim_left<char>(lpszBuffer);
	pstrtrim_right<char>(lpszBuffer);

	// remove '"'
	pstrtrim_left<char>(lpszBuffer, '"');
	pstrtrim_right<char>(lpszBuffer, '"');
}

// ("a", "b", 3) -> a,b,3
bool GetMethodParams(const std::string & sInput, std::vector<std::string> & outParams)
{
	std::string::size_type find1 = sInput.find("(");
	if (find1 == std::string::npos)
		return false;
	std::string::size_type find2 = sInput.find(")", find1+1);
	if (find2 == std::string::npos)
		return false;

	char lpszParam[256];
	std::string sInputTemp = sInput.substr(find1+1, find2-find1-1);
	std::string::size_type find = std::string::npos;
	while ((find = sInputTemp.find(",")) != std::string::npos)
	{
		memset(lpszParam, 0, 256);
		strcpy(lpszParam, sInputTemp.substr(0, find).c_str());
		trim_csp_string(lpszParam);
		outParams.push_back(lpszParam);

		sInputTemp = sInputTemp.substr(find+1);
	}

	memset(lpszParam, 0, 256);
	strcpy(lpszParam, sInputTemp.c_str());
	trim_csp_string(lpszParam);
	outParams.push_back(lpszParam);
	return true;
}

const char * CFileScripts::parseOneLine(const char * pLineBuffer, const CScriptItem::pointer& scriptItem, bool bEndReturn)
{
	if (pLineBuffer == NULL) return NULL;
	//const std::string sLiineTemp(pLineBuffer,10);
	//printf("**** parseOneLine sLiineTemp=%s,scriptItem=%x,bEndReturn=%d\n", sLiineTemp.c_str(),(int)scriptItem.get(),(int)(bEndReturn?1:0));

	const char * pNextLineFind = strstr(pLineBuffer, "\n");
	//const char * pNextLineFind = m_bCodeBegin?strstr(pLineBuffer+1, "\n"):strstr(pLineBuffer, "\n");
	//const std::string sNextLineFind = pNextLineFind==NULL?"NULL":std::string(pNextLineFind,10);
	//printf("**** parseOneLine pNextLineFind=%s\n", sNextLineFind.c_str());
	int leftIndex = 0;

	bool bSotpCompareOK = true;
	if (!m_bCodeBegin)
	{
		if (sotpCompare(pLineBuffer, CSP_TAB_COMMENT1_BEGIN.c_str(), leftIndex))
		{
			// {{{ }}} CSP 注释 - 直接打印显示
			leftIndex += CSP_TAB_COMMENT1_BEGIN.size();
			pNextLineFind = strstr(pLineBuffer+leftIndex, CSP_TAB_COMMENT1_END.c_str());
			if (pNextLineFind == NULL)
				return NULL;

			std::string htmlText(pLineBuffer+leftIndex, pNextLineFind-pLineBuffer-leftIndex);
			replace(htmlText, "\r\n", "\r\n<br>");
			replace(htmlText, "\n", "\n<br>");
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Write, htmlText);
			if (scriptItem.get() == NULL)
				addScriptItem(m_scripts, scriptItemNew);
			else
				addScriptItem(scriptItem->m_subs, scriptItemNew);

			pNextLineFind += CSP_TAB_COMMENT1_BEGIN.size();
			if (pNextLineFind[0] == '\r')
				pNextLineFind += 1;
			if (pNextLineFind[0] == '\n')
				pNextLineFind += 1;
			return pNextLineFind;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_COMMENT2_BEGIN.c_str(), leftIndex))
		{
			// <%-- --%> CSP 注释 - 不做任何处理
			leftIndex += CSP_TAB_COMMENT2_BEGIN.size();
			pNextLineFind = strstr(pLineBuffer+leftIndex, CSP_TAB_COMMENT2_END.c_str());
			if (pNextLineFind != NULL)
			{
				pNextLineFind += CSP_TAB_COMMENT2_BEGIN.size();
				if (pNextLineFind[0] == '\r')
					pNextLineFind += 1;
				if (pNextLineFind[0] == '\n')
					pNextLineFind += 1;
			}
			return pNextLineFind;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_EXECUTE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_EXECUTE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Execute);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_FUNCTION))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_CALL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_CALL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppCall);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_APPCALL))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_INIT.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_INIT.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppInit);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_IN))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_FINAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_FINAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppFinal);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_GET.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_GET.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppGet);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME|TYPE_OUT))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_SET.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_SET.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppSet);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME|TYPE_IN))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_ADD.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_ADD.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppAdd);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME|TYPE_IN))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_DEL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_DEL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppDel);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_APP_INFO.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_APP_INFO.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_AppInfo);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_OUT))
			{
				return NULL;
			}
		}/*else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_INIT.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_CDBC_INIT.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCInit);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_FINAL.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_CDBC_FINAL.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCFinal);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_OPEN.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_CDBC_OPEN.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCOpen);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_CLOSE.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_CDBC_CLOSE.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCClose);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }*/else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_EXEC.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_EXEC.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCExec);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))	// TYPE_NAME = sql
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_SELECT.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_SELECT.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCSelect);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME|TYPE_OUT))	// TYPE_NAME = sql
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_MOVEINDEX.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_MOVEINDEX.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCMoveIndex);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME|TYPE_INDEX))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_FIRST.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_FIRST.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCFirst);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))	// TYPE_NAME = cookie
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_NEXT.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_NEXT.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCNext);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_PREV.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_PREV.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCPrev);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_LAST.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_LAST.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCLast);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_RESET.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_RESET.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCReset);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_SIZE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_SIZE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCSize);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDBC_INDEX.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDBC_INDEX.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_CDBCIndex);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_NAME))
			{
				return NULL;
			}
		}/*else if (sotpCompare(pLineBuffer, CSP_TAB_FILE_EXIST.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_FILE_EXIST.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_FileExist);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_FILE_SIZE.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_FILE_SIZE.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_FileSize);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_FILE_DELETEALL.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_FILE_DELETEALL.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_FileDeleteAll);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_FILE_DELETE.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_FILE_DELETE.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_FileDelete);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_FILE_RENAME.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_FILE_RENAME.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_FileRename);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }else if (sotpCompare(pLineBuffer, CSP_TAB_FILE_COPYTO.c_str(), leftIndex))
		 {
		 leftIndex += CSP_TAB_FILE_COPYTO.size();
		 CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_FileCopyto);
		 if (scriptItem.get() == NULL)
		 m_scripts.push_back(scriptItemNew);
		 else
		 scriptItem->m_subs.push_back(scriptItemNew);

		 int leftIndexTemp = 0;
		 if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
		 {
		 return NULL;
		 }
		 }*/else if (sotpCompare(pLineBuffer, CSP_TAB_DEFINE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_DEFINE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Define);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_EQUAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_EQUAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Define);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_ADD.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_ADD.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Add);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_SUBTRACT.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_SUBTRACT.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Subtract);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_MULTI.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_MULTI.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Multi);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_DIVISION.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_DIVISION.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Division);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_MODULUS.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_MODULUS.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Modulus);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_INCREASE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_INCREASE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Increase);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_DECREASE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_DECREASE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Decrease);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_EMPTY.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_EMPTY.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Empty);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_OUT))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_RESET.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_RESET.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Reset);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_SIZEOF.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_SIZEOF.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Sizeof);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_TYPEOF.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_TYPEOF.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Typeof);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_OUT))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_TOTYPE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_TOTYPE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_ToType);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_TYPE))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_INDEX.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_INDEX.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Index);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE|TYPE_INDEX|TYPE_OUT))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_PAGE_CONTENTTYPE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_PAGE_CONTENTTYPE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_PageContentType);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_TYPE))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_PAGE_RETURN.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_PAGE_RETURN.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_PageReturn);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
		}else if (sotpCompare(pLineBuffer, CSP_TAB_PAGE_RESET.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_PAGE_RESET.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_PageReset);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
		}else if (sotpCompare(pLineBuffer, CSP_TAB_PAGE_FORWARD.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_PAGE_FORWARD.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_PageForward);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_URL))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_PAGE_LOCATION.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_PAGE_LOCATION.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_PageLocation);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_URL|TYPE_PROPERTY))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_PAGE_INCLUDE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_PAGE_INCLUDE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_PageInclude);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_URL))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_AUTHENTICATE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_AUTHENTICATE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Authenticate);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			//int leftIndexTemp = 0;
			//if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_NONE))
			//{
			//	return NULL;
			//}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_EQUAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_EQUAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
			//printf("**** parseOneLine CSP_TAB_IF_EQUAL,%x \n", (int)scriptItemNew.get());
			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_NOTEQUAL1.c_str(), leftIndex) ||
			sotpCompare(pLineBuffer, CSP_TAB_IF_NOTEQUAL2.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_NOTEQUAL1.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_GREATEREQUAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_GREATEREQUAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfGreaterEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_GREATER.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_GREATER.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfGreater);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_LESSEQUAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_LESSEQUAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfLessEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_LESS.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_LESS.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfLess);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_AND.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_AND.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfAnd);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_OR.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_OR.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfOr);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_EMPTY.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_EMPTY.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfEmpty);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_NOTEMPTY.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_NOTEMPTY.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotEmpty);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_EXIST.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_EXIST.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfExist);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_IF_NOTEXIST.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_IF_NOTEXIST.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfNotExist);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_EQUAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_EQUAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_NOTEQUAL1.c_str(), leftIndex) ||
			sotpCompare(pLineBuffer, CSP_TAB_WHILE_NOTEQUAL2.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_NOTEQUAL1.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileNotEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_GREATEREQUAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_GREATEREQUAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileGreaterEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_GREATER.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_GREATER.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileGreater);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_LESSEQUAL.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_LESSEQUAL.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileLessEqual);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_LESS.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_LESS.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileLess);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_AND.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_AND.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileAnd);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_OR.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_OR.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileOr);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_EMPTY.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_EMPTY.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileEmpty);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_NOTEMPTY.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_NOTEMPTY.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileNotEmpty);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_EXIST.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_EXIST.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileExist);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WHILE_NOTEXIST.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WHILE_NOTEXIST.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_WhileNotExist);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_FOREACH.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_FOREACH.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Foreach);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPIF(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp, TYPE_ID|TYPE_SCOPE))
			{
				return NULL;
			}
			return pLineBuffer+leftIndex+leftIndexTemp+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_BREAK.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_BREAK.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Break);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
			// ** 支持 <csp:break> 和 <csp:break id="" value="" />
			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex, scriptItemNew, leftIndexTemp))
			//if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CONTINUE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CONTINUE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Continue);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
			// ** 支持 <csp:continue> 和 <csp:continue id="" value="" />
			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex, scriptItemNew, leftIndexTemp))
			//if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_WRITE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_WRITE.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Write);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_OUT_PRINT.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_OUT_PRINT.size();
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_OutPrint);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);

			int leftIndexTemp = 0;
			if (!getCSPObject(pLineBuffer+leftIndex+1, scriptItemNew, leftIndexTemp))
			{
				return NULL;
			}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CODE_BEGIN.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CODE_BEGIN.size();
			m_bCodeBegin = true;
			return pLineBuffer+leftIndex+1;
		//}else if (sotpCompare(pLineBuffer, CSP_TAB_CDDE_END.c_str(), leftIndex))
		//{
		//	leftIndex += CSP_TAB_CDDE_END.size();
		//	m_bCodeBegin = false;
		}else
		{
			bSotpCompareOK = false;
		}
	}else
	{
		if (sotpCompare(pLineBuffer, CSP_TAB_CDDE_END.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CDDE_END.size();
			m_bCodeBegin = false;
			return pLineBuffer+leftIndex;
		}
		else if (sotpCompare(pLineBuffer, CSP_TAB_CDDE_COMMENT_LINE.c_str(), leftIndex))
		{
			// 单行注释
			return pNextLineFind == NULL ? NULL : pNextLineFind+1;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_CDDE_COMMENT_BEGIN.c_str(), leftIndex))
		{
			// 多行注释
			leftIndex += CSP_TAB_CDDE_COMMENT_BEGIN.size();
			pNextLineFind = strstr(pLineBuffer+leftIndex, CSP_TAB_CDDE_COMMENT_END.c_str());
			if (pNextLineFind != NULL)
			{
				pNextLineFind += CSP_TAB_CDDE_COMMENT_BEGIN.size();
				if (pNextLineFind[0] == '\r')
					pNextLineFind += 1;
				if (pNextLineFind[0] == '\n')
					pNextLineFind += 1;
			}
			return pNextLineFind;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_OUT_PRINT.c_str(), leftIndex))
		{
			//printf("**** CSP_TAB_METHOD_OUT_PRINT, leftIndex=%d\n",(int)leftIndex);
			leftIndex += (CSP_TAB_METHOD_OUT_PRINT.size()+1);
			char lpszBufferLine[1024];
			int nFindIndex = 0;
			const size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';', &nFindIndex);
			if (size <= 0)
				return NULL;
			leftIndex += (nFindIndex+1);
			//printf("**** CSP_TAB_METHOD_OUT_PRINT,size=%d,lpszBufferLine=%s\n",(int)size,lpszBufferLine);

			int bufferSize = pstrtrim_left<char>(lpszBufferLine);
			//printf("**** CSP_TAB_METHOD_OUT_PRINT, pstrtrim_left bufferSize=%d\n",(int)bufferSize);
			if (bufferSize <= 0)
				return NULL;
			bufferSize = pstrtrim_right<char>(lpszBufferLine);
			//printf("**** CSP_TAB_METHOD_OUT_PRINT, pstrtrim_right bufferSize=%d\n",(int)bufferSize);
			if (bufferSize <= 0)
				return NULL;

			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_OutPrint);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
			getCSPScriptObject1(lpszBufferLine, bufferSize, scriptItemNew);
			//const std::string s(pLineBuffer+leftIndex,10);
			//printf("**** getCSPScriptObject1 ok,s=(%s)\n",s.c_str());
			//int leftIndexTemp = 0;
			//if (sotpCompare(lpszBufferLine, VTI_USER.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_SYSTEM.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_INITPARAM.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_REQUESTPARAM.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_HEADER.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_TEMP.c_str(), leftIndexTemp)
			//	//sotpCompare(lpszBufferLine, VTI_APP.c_str(), leftIndexTemp) ||
			//	//sotpCompare(lpszBufferLine, VTI_DATASOURCE.c_str(), leftIndexTemp)
			//	)
			//{
			//	std::string sId(lpszBufferLine);
			//	std::string::size_type nFindIndex1 = sId.find("[");
			//	std::string sIndex("");
			//	if (nFindIndex1 != std::string::npos)
			//	{
			//		sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
			//		trimstring(sIndex);
			//		scriptItemNew->setProperty(sIndex);
			//		lpszBufferLine[nFindIndex1] = '\0';
			//	}

			//	scriptItemNew->setOperateObject1(CScriptItem::CSP_Operate_Id);
			//	scriptItemNew->setId(lpszBufferLine);
			//}else
			//{
			//	if (lpszBufferLine[0] == '\'' && lpszBufferLine[bufferSize-1] =='\'')
			//	{
			//		std::string sValue(lpszBufferLine);
			//		sValue = sValue.substr(1, bufferSize-2);
			//		scriptItemNew->setValue(sValue);
			//	}else
			//		scriptItemNew->setValue(lpszBufferLine);
			//}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_ECHO.c_str(), leftIndex))
		{
			leftIndex += (CSP_TAB_METHOD_ECHO.size()+1);
			char lpszBufferLine[1024];
			int nFindIndex = 0;
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';', &nFindIndex);
			if (size <= 0)
				return NULL;
			leftIndex += (nFindIndex+1);
			//leftIndex += (size+1);

			int bufferSize = pstrtrim_left<char>(lpszBufferLine);
			if (bufferSize <= 0)
				return NULL;
			bufferSize = pstrtrim_right<char>(lpszBufferLine);
			if (bufferSize <= 0)
				return NULL;

			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Write);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
			getCSPScriptObject1(lpszBufferLine, bufferSize, scriptItemNew);
			//int leftIndexTemp = 0;
			//if (sotpCompare(lpszBufferLine, VTI_USER.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_SYSTEM.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_INITPARAM.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_REQUESTPARAM.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_HEADER.c_str(), leftIndexTemp) ||
			//	sotpCompare(lpszBufferLine, VTI_TEMP.c_str(), leftIndexTemp)
			//	//sotpCompare(lpszBufferLine, VTI_APP.c_str(), leftIndexTemp) ||
			//	//sotpCompare(lpszBufferLine, VTI_DATASOURCE.c_str(), leftIndexTemp)
			//	)
			//{
			//	std::string sId(lpszBufferLine);
			//	std::string::size_type nFindIndex1 = sId.find("[");
			//	std::string sIndex("");
			//	if (nFindIndex1 != std::string::npos)
			//	{
			//		sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
			//		trimstring(sIndex);
			//		scriptItemNew->setProperty(sIndex);
			//		lpszBufferLine[nFindIndex1] = '\0';
			//	}

			//	scriptItemNew->setOperateObject1(CScriptItem::CSP_Operate_Id);
			//	scriptItemNew->setId(lpszBufferLine);
			//}else
			//{
			//	if (lpszBufferLine[0] == '\'' && lpszBufferLine[bufferSize-1] =='\'')
			//	{
			//		std::string sValue(lpszBufferLine);
			//		sValue = sValue.substr(1, bufferSize-2);
			//		scriptItemNew->setValue(sValue);
			//	}else
			//		scriptItemNew->setValue(lpszBufferLine);
			//}
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_FORWARD.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_FORWARD.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_PageForward, sMethodParam, 1, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_LOCATION.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_LOCATION.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_PageLocation, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_INCLUDE.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_INCLUDE.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_PageInclude, sMethodParam, 1, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_IS_EMPTY.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_IS_EMPTY.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Empty, sMethodParam, 1, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_SIZEOF.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_SIZEOF.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Sizeof, sMethodParam, 1, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_TYPEOF.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_TYPEOF.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Typeof, sMethodParam, 1, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_TO_TYPE.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_TO_TYPE.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_ToType, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_RESET.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_RESET.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Reset, sMethodParam, 1, false))
				return NULL;
		}
		else if (sotpCompare(pLineBuffer, CSP_TAB_CODE_RETURN.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CODE_RETURN.size();
			char lpszBufferLine[1024];
			int nFindIndex = 0;
			const size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';', &nFindIndex);
			if (size <= 0 && nFindIndex==0)
				return NULL;
			leftIndex += (nFindIndex+1);
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_PageReturn);
			getCSPScriptObject1(lpszBufferLine, size, scriptItemNew);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
		}
		else if (sotpCompare(pLineBuffer, CSP_TAB_CODE_CONTINUE.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CODE_CONTINUE.size();
			int ntemp = 0;
			if (!sotpCompare(pLineBuffer+leftIndex, ";", ntemp))
			{
				return NULL;
			}
			leftIndex += (ntemp+1);
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Continue);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
		}
		else if (sotpCompare(pLineBuffer, CSP_TAB_CODE_BREAK.c_str(), leftIndex))
		{
			leftIndex += CSP_TAB_CODE_BREAK.size();
			int ntemp = 0;
			if (!sotpCompare(pLineBuffer+leftIndex, ";", ntemp))
			{
				return NULL;
			}
			leftIndex += (ntemp+1);
			CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Break);
			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
		}
		else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_EXE_SERVLET.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_EXE_SERVLET.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Execute, sMethodParam, 2, false))
				return NULL;

			//std::vector<std::string> methodParams;
			//GetMethodParams(sMethodParam, methodParams);
			//if (methodParams.size() != 2)
			//	return NULL;

			//CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Execute);
			//if (scriptItem.get() == NULL)
			//	m_scripts.push_back(scriptItemNew);
			//else
			//	scriptItem->m_subs.push_back(scriptItemNew);
			//scriptItemNew->setOperateObject1(CScriptItem::CSP_Operate_Id);
			//scriptItemNew->setId(methodParams[0]);
			//scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Name);
			//scriptItemNew->setName(methodParams[1]);
		}
		//else if (sotpCompare(pLineBuffer, "{", leftIndex))
		//{
		//	if (scriptItem.get() == NULL)
		//		return NULL;
		//	//printf("**** find '{'\n");
		//	leftIndex +=1;
		//}
		//else if (sotpCompare(pLineBuffer, "}", leftIndex))
		//{
		//	if (scriptItem.get() == NULL)
		//		return NULL;
		//	leftIndex +=1;
		//	const std::string s(pLineBuffer+leftIndex,10);
		//	printf("**** find '}', s=%s, bEndReturn=%d\n",s.c_str(),(int)(bEndReturn?1:0));
		//	CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_End);
		//	scriptItem->m_subs.push_back(scriptItemNew);
		//	// *结束返回
		//	// ??? 这里逻辑有点问题；
		//	int ntemp = 0;
		//	if (!bEndReturn && sotpCompare(pLineBuffer+leftIndex, "else", ntemp))
		//	{
		//		leftIndex += (ntemp+4);
		//		scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfElse);
		//		printf("**** find '}else' true\n");
		//		if (sotpCompare(pLineBuffer+leftIndex, "if", ntemp))
		//		{
		//			printf("**** ELSE IF()\n");
		//			scriptItem->m_elseif.push_back(scriptItemNew);
		//		}else
		//		{
		//			printf("**** ELSE\n");
		//			scriptItem->m_else.push_back(scriptItemNew);
		//		}
		//		const std::string s(pLineBuffer+leftIndex+ntemp, 10);
		//		printf("**** after else [%s],bEndReturn=%d\n",s.c_str(),(int)(bEndReturn?1:0));
		//		return parseOneLine(pLineBuffer+leftIndex+ntemp, scriptItemNew, bEndReturn);
		//		pLineBuffer = parseOneLine(pLineBuffer+leftIndex+ntemp, scriptItemNew, true);
		//		//没有退出来
		//		printf("**** after parseOneLine ok\n");
		//		return parseOneLine(pLineBuffer, scriptItem, bEndReturn);
		//	}else
		//	{
		//		printf("**** find '}' return true\n");
		//		leftIndex += ntemp;
		//		return pLineBuffer+leftIndex;
		//	}
		//	//if (bEndReturn)
		//	//	return pLineBuffer+leftIndex;
		//}
		//else if (sotpCompare(pLineBuffer, "else", leftIndex))
		//{
		//	//printf("**** sotpCompare else true\n");
		//	if (scriptItem.get() == NULL)
		//		return NULL;
		//	leftIndex +=4;
		//	CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfElse);
		//	int ntemp = 0;
		//	if (sotpCompare(pLineBuffer+leftIndex, "if", ntemp))
		//	{
		//		printf("**** ELSE IF()\n");
		//		scriptItem->m_elseif.push_back(scriptItemNew);
		//	}else
		//	{
		//		printf("**** ELSE\n");
		//		scriptItem->m_else.push_back(scriptItemNew);
		//	}
		//	//const std::string s(pLineBuffer+leftIndex+ntemp, 10);
		//	//printf("**** after else [%s],bEndReturn=%d\n",s.c_str(),(int)(bEndReturn?1:0));
		//	//return parseOneLine(pLineBuffer+leftIndex+ntemp, scriptItemNew, bEndReturn);
		//	pLineBuffer = parseOneLine(pLineBuffer+leftIndex+ntemp, scriptItemNew, true);
		//	// 没有退出来
		//	printf("**** after parseOneLine ok\n");
		//	return parseOneLine(pLineBuffer, scriptItem, bEndReturn);
		//}
		else if (sotpCompare(pLineBuffer, "if", leftIndex))
		{
			char lpszBufferLine[1024];
			const size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ')');
			//printf("**** sotpCompare if true=%s,size=%d\n",lpszBufferLine,(int)size);
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);
			// ** ' ($var=12'
			int ntemp = 0;
			if (!sotpCompare(lpszBufferLine+2, "(", ntemp))
				return NULL;
			// ** '$var=12'
			//const std::string sIfParam(lpszBufferLine+2+ntemp+1);
			//printf("**** if (%s)\n",sIfParam.c_str());
			//const char* pIfParamStart = sIfParam.c_str();
			//const size_t nSourceSize = sIfParam.size();
			const char* pIfParamStart = lpszBufferLine+2+ntemp+1;	// 1是'('
			const size_t nSourceSize = size-2-ntemp-1;
			const char* nFindIf = plstrstr<char>(pIfParamStart,"==",nSourceSize,2);
			CScriptItem::ItemType nFindIfItemType = CScriptItem::CSP_Unknown;
			if (nFindIf!=NULL)
			{
				nFindIfItemType = CScriptItem::CSP_IfElse;
				//const std::string s1(pIfParamStart,(std::string::size_type)(nFindIf-pIfParamStart));
				//const std::string s2(nFindIf+2);
				////printf("**** find s1=%s,s2=%s\n",s1.c_str(),s2.c_str());
				//bool bResetIf = false;
				//CScriptItem::pointer scriptItemNew;
				//if (scriptItem.get()!=NULL && scriptItem->getItemType()==CScriptItem::CSP_IfElse &&
				//	scriptItem->getOperateObject1()==CScriptItem::CSP_Operate_None && scriptItem->getProperty()=="elseif")
				//{
				//	scriptItem->setProperty("");
				//	scriptItemNew = scriptItem;
				//	scriptItemNew->setItemType(CScriptItem::CSP_IfEqual);
				//	bResetIf = true;
				//	//printf("**** reset CScriptItem::CSP_IfEqual\n");
				//}
				//else
				//{
				//	scriptItemNew = CScriptItem::create(CScriptItem::CSP_IfEqual);
				//	if (scriptItem.get() == NULL)
				//		m_scripts.push_back(scriptItemNew);
				//	else
				//		scriptItem->m_subs.push_back(scriptItemNew);
				//}
				//strcpy(lpszBufferLine,s1.c_str());
				//getCSPScriptObject1(lpszBufferLine, s1.size(), scriptItemNew);
				//strcpy(lpszBufferLine,s2.c_str());
				//getCSPScriptObject2(lpszBufferLine, s2.size(), scriptItemNew);
				////const std::string s(pLineBuffer+leftIndex,10);
				////printf("########### before getCSPIF, %x,%x s=%s\n", (int)scriptItem.get(),(int)scriptItemNew.get(),s.c_str());
				//const bool bEndReturnTemp = bResetIf;
				//int leftIndexTemp = 0;
				//if (!getCSPIF(pLineBuffer+leftIndex, scriptItemNew, leftIndexTemp, TYPE_DEFAULT, bEndReturnTemp))
				//{
				//	return NULL;
				//}
				////const std::string s20(pLineBuffer+leftIndex+leftIndexTemp,20);
				////printf("########### after getCSPIF, %x,%x,bEndReturn=%d s20=%s\n", (int)scriptItem.get(),(int)scriptItemNew.get(),(int)(bEndReturn?1:0),s20.c_str());
				////return parseOneLine(pLineBuffer+leftIndex+leftIndexTemp, scriptItem, bEndReturn);
				//return pLineBuffer+leftIndex+leftIndexTemp;
			}
			if (nFindIfItemType==CScriptItem::CSP_Unknown)
			{
				nFindIf = plstrstr<char>(pIfParamStart,"!=",nSourceSize,2);
				if (nFindIf==NULL)
					nFindIf = plstrstr<char>(pIfParamStart,"<>",nSourceSize,2);
				if (nFindIf!=NULL)
				{
					nFindIfItemType = CScriptItem::CSP_IfNotEqual;
				}
			}
			if (nFindIfItemType==CScriptItem::CSP_Unknown)
			{
				nFindIf = plstrstr<char>(pIfParamStart,">=",nSourceSize,2);
				if (nFindIf!=NULL)
				{
					nFindIfItemType = CScriptItem::CSP_IfGreaterEqual;
				}
			}
			if (nFindIfItemType==CScriptItem::CSP_Unknown)
			{
				nFindIf = plstrstr<char>(pIfParamStart,">",nSourceSize,2);
				if (nFindIf!=NULL)
				{
					nFindIfItemType = CScriptItem::CSP_IfGreater;
				}
			}
			if (nFindIfItemType==CScriptItem::CSP_Unknown)
			{
				nFindIf = plstrstr<char>(pIfParamStart,"<=",nSourceSize,2);
				if (nFindIf!=NULL)
				{
					nFindIfItemType = CScriptItem::CSP_IfLessEqual;
				}
			}
			if (nFindIfItemType==CScriptItem::CSP_Unknown)
			{
				nFindIf = plstrstr<char>(pIfParamStart,"<",nSourceSize,2);
				if (nFindIf!=NULL)
				{
					nFindIfItemType = CScriptItem::CSP_IfLess;
				}
			}

			if (nFindIfItemType!=CScriptItem::CSP_Unknown)
			{
				std::string s1(pIfParamStart,(std::string::size_type)(nFindIf-pIfParamStart));
				std::string s2(nFindIf+2);
				trim(s1);
				trim(s2);
				bool bResetIf = false;
				CScriptItem::pointer scriptItemNew;
				if (scriptItem.get()!=NULL && scriptItem->getItemType()==CScriptItem::CSP_IfElse &&
					scriptItem->getOperateObject1()==CScriptItem::CSP_Operate_None && scriptItem->getProperty()=="elseif")
				{
					scriptItem->setProperty("");
					scriptItemNew = scriptItem;
					scriptItemNew->setItemType(nFindIfItemType);
					bResetIf = true;
					//printf("**** reset CScriptItem::CSP_IfEqual\n");
				}
				else
				{
					scriptItemNew = CScriptItem::create(nFindIfItemType);
					if (scriptItem.get() == NULL)
						m_scripts.push_back(scriptItemNew);
					else
						scriptItem->m_subs.push_back(scriptItemNew);
				}
				strcpy(lpszBufferLine,s1.c_str());
				getCSPScriptObject1(lpszBufferLine, s1.size(), scriptItemNew);
				strcpy(lpszBufferLine,s2.c_str());
				getCSPScriptObject2(lpszBufferLine, s2.size(), scriptItemNew);
				const bool bEndReturnTemp = bResetIf;
				int leftIndexTemp = 0;
				if (!getCSPIF(pLineBuffer+leftIndex, scriptItemNew, leftIndexTemp, TYPE_DEFAULT, bEndReturnTemp))
				{
					return NULL;
				}
				return pLineBuffer+leftIndex+leftIndexTemp;
			}
		}
		else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_APP_CALL.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			const std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_APP_CALL.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppCall, sMethodParam, 2, false))
				return NULL;
		}
		else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_APP_GET.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_APP_GET.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppGet, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_APP_SET.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_APP_SET.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppSet, sMethodParam, 3, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_APP_ADD.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_APP_ADD.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppAdd, sMethodParam, 3, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_APP_DEL.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_APP_DEL.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppDel, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_APP_INFO.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_APP_INFO.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppInfo, sMethodParam, 1, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_EXEC.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_EXEC.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCExec, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_SELECT.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_SELECT.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCSelect, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_MOVEINDEX.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_MOVEINDEX.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCMoveIndex, sMethodParam, 3, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_FIRST.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_FIRST.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCFirst, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_NEXT.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_NEXT.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCNext, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_PREVIOUS.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_PREVIOUS.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCPrev, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_LAST.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_LAST.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCLast, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_INDEX.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_INDEX.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCIndex, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_SIZE.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_SIZE.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCSize, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, CSP_TAB_METHOD_CDBC_RESET.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';');
			if (size <= 0)
				return NULL;
			leftIndex += (size+1);

			std::string sMethodParam(lpszBufferLine+CSP_TAB_METHOD_CDBC_RESET.size());
			if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCReset, sMethodParam, 2, false))
				return NULL;
		}else if (sotpCompare(pLineBuffer, VTI_USER.c_str(), leftIndex))
		{
			char lpszBufferLine[1024];
			int nFindIndex = 0;
			size_t size = pgetstr<char>(lpszBufferLine, 1024, pLineBuffer+leftIndex, ';', &nFindIndex);
			if (size <= 0)
				return NULL;
			leftIndex += nFindIndex;
			//leftIndex += (size+1);

			CScriptItem::pointer scriptItemNew;
			//bool finded = false;
			char lpszBuffer1[256];
			memset(lpszBuffer1, 0, 256);
			char lpszBuffer2[256];
			memset(lpszBuffer2, 0, 256);
			int offset = 2;
			const char * lpszFind = pstrstrl<char>(lpszBufferLine, "+=", size, 2);
			if (lpszFind != NULL)
			{
				scriptItemNew = CScriptItem::create(CScriptItem::CSP_Add);
			}else if((lpszFind = pstrstrl<char>(lpszBufferLine, "-=", size, 2)) != NULL)
			{
				scriptItemNew = CScriptItem::create(CScriptItem::CSP_Subtract);
			}else if((lpszFind = pstrstrl<char>(lpszBufferLine, "*=", size, 2)) != NULL)
			{
				scriptItemNew = CScriptItem::create(CScriptItem::CSP_Multi);
			}else if((lpszFind = pstrstrl<char>(lpszBufferLine, "/=", size, 2)) != NULL)
			{
				scriptItemNew = CScriptItem::create(CScriptItem::CSP_Division);
			}else if((lpszFind = pstrstrl<char>(lpszBufferLine, "%=", size, 2)) != NULL)
			{
				scriptItemNew = CScriptItem::create(CScriptItem::CSP_Modulus);
			}else if((lpszFind = pstrstrl<char>(lpszBufferLine, "=", size, 1)) != NULL)
			{
				offset = 1;
				scriptItemNew = CScriptItem::create(CScriptItem::CSP_Define);
			}

			size_t buffer2Size = 0;
			if (scriptItemNew.get() != NULL)
			{
				// += -= *= /= %= =
				strncpy(lpszBuffer1, lpszBufferLine, lpszFind-lpszBufferLine);
				strncpy(lpszBuffer2, lpszFind+offset, size-(lpszFind-lpszBufferLine)-offset);

				buffer2Size = pstrtrim_left<char>(lpszBuffer2);
				if (buffer2Size <= 0)
					return NULL;
				buffer2Size = pstrtrim_right<char>(lpszBuffer2);
				if (buffer2Size <= 0)
					return NULL;
			}else
			{
				strcpy(lpszBuffer1, lpszBufferLine);
			}

			size = pstrtrim_left<char>(lpszBuffer1);
			if (size <= 2)
				return NULL;
			size = pstrtrim_right<char>(lpszBuffer1);
			if (size <= 2)
				return NULL;

			if (scriptItemNew.get() != NULL)
			{
				if (scriptItemNew->getItemType() == CScriptItem::CSP_Define)
				{
					std::string sId(lpszBuffer1);
					std::string::size_type nFindIndex1 = sId.find("[");
					std::string sIndex("");
					if (nFindIndex1 != std::string::npos)
					{
						sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
						trimstring(sIndex);
						scriptItemNew->setItemType(CScriptItem::CSP_Add);
						scriptItemNew->setProperty(sIndex);
						lpszBuffer1[nFindIndex1] = '\0';
					}
				}
			}else
			{
				if (lpszBuffer1[size-1] == '+' && lpszBuffer1[size-2] == '+')
				{
					scriptItemNew = CScriptItem::create(CScriptItem::CSP_Increase);
					lpszBuffer1[size-2] = '\0';
				}else if (lpszBuffer1[size-1] == '-' && lpszBuffer1[size-2] == '-')
				{
					scriptItemNew = CScriptItem::create(CScriptItem::CSP_Decrease);
					lpszBuffer1[size-2] = '\0';
				}else
				{
					return NULL;
				}
			}

			if (scriptItem.get() == NULL)
				m_scripts.push_back(scriptItemNew);
			else
				scriptItem->m_subs.push_back(scriptItemNew);
			scriptItemNew->setOperateObject1(CScriptItem::CSP_Operate_Id);
			scriptItemNew->setId(std::string(lpszBuffer1));

			if (buffer2Size > 0)
			{
				// 这里太多if，导致编译器有问题，所以分开
				int leftIndexTemp = 0;
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_IS_EMPTY.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_IS_EMPTY.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Empty, sMethodParam, 1, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_SIZEOF.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_SIZEOF.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Sizeof, sMethodParam, 1, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_TYPEOF.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_TYPEOF.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Typeof, sMethodParam, 1, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_EXE_SERVLET.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_EXE_SERVLET.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_Execute, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_APP_CALL.c_str(), leftIndexTemp))
				{
					const std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_APP_CALL.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppCall, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_APP_GET.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_APP_GET.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppGet, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_APP_INFO.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_APP_INFO.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_AppInfo, sMethodParam, 1, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_EXEC.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_EXEC.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCExec, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_SELECT.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_SELECT.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCSelect, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_MOVEINDEX.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_MOVEINDEX.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCMoveIndex, sMethodParam, 3, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_FIRST.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_FIRST.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCFirst, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
					return pLineBuffer+leftIndex+1;
				}
				if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_NEXT.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_NEXT.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCNext, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
				}else if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_PREVIOUS.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_PREVIOUS.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCPrev, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
				}else if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_LAST.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_LAST.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCLast, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
				}else if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_INDEX.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_INDEX.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCIndex, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
				}else if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_SIZE.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_SIZE.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCSize, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
				}else if (sotpCompare(lpszBuffer2, CSP_TAB_METHOD_CDBC_RESET.c_str(), leftIndexTemp))
				{
					std::string sMethodParam(lpszBuffer2+leftIndexTemp+CSP_TAB_METHOD_CDBC_RESET.size());
					if (!addMethodScriptItem(scriptItem, CScriptItem::CSP_CDBCReset, sMethodParam, 2, true, lpszBuffer1))
						return NULL;
				}else if (sotpCompare(lpszBuffer2, VTI_USER.c_str(), leftIndexTemp) ||
					sotpCompare(lpszBuffer2, VTI_SYSTEM.c_str(), leftIndexTemp) ||
					sotpCompare(lpszBuffer2, VTI_INITPARAM.c_str(), leftIndexTemp) ||
					sotpCompare(lpszBuffer2, VTI_REQUESTPARAM.c_str(), leftIndexTemp) ||
					sotpCompare(lpszBuffer2, VTI_HEADER.c_str(), leftIndexTemp) ||
					sotpCompare(lpszBuffer2, VTI_TEMP.c_str(), leftIndexTemp)
					//sotpCompare(lpszBuffer2, VTI_APP.c_str(), leftIndexTemp) ||
					//sotpCompare(lpszBuffer2, VTI_DATASOURCE.c_str(), leftIndexTemp)
					)
				{
					std::string sId(lpszBuffer2);
					std::string::size_type nFindIndex1 = sId.find("[");
					std::string sIndex("");
					if (nFindIndex1 != std::string::npos)
					{
						sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
						trimstring(sIndex);
						scriptItemNew->setProperty2(sIndex);
						//if (scriptItemNew->getProperty().empty())
						//	scriptItemNew->setProperty(sIndex);
						//else
						//	scriptItemNew->setType(sIndex);	// ???
						lpszBuffer2[nFindIndex1] = '\0';
					}

					scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Id);
					scriptItemNew->setValue(lpszBuffer2);

				}else if (strcmp(lpszBuffer2,"vector()") == 0)
				{
					// vector
					scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Value);
					scriptItemNew->setValue("");
					scriptItemNew->setType("vector");
				}else if (strcmp(lpszBuffer2,"map()") == 0)
				{
					// vector
					scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Value);
					scriptItemNew->setValue("");
					scriptItemNew->setType("map");
				}else if (lpszBuffer2[0] == '\'' && lpszBuffer2[buffer2Size-1] =='\'')
				{
					// string
					std::string sValue(lpszBuffer2);
					sValue = sValue.substr(1, buffer2Size-2);
					scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Value);
					scriptItemNew->setValue(sValue);
					scriptItemNew->setType("string");
				}else if (strstr(lpszBuffer2, ".") != NULL)
				{
					// float
					scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Value);
					scriptItemNew->setValue(lpszBuffer2);
					scriptItemNew->setType("float");
				}else if (buffer2Size >= 10)
				{
					// bigint
					scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Value);
					scriptItemNew->setValue(lpszBuffer2);
					scriptItemNew->setType("bigint");
				}else
				{
					// int
					scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Value);
					scriptItemNew->setValue(lpszBuffer2);
					scriptItemNew->setType("int");
				}
			}

			return pLineBuffer+leftIndex+1;
		}else
		{
			bSotpCompareOK = false;
		}
	}

	if (!bSotpCompareOK)
	{
		const size_t lineSize = pNextLineFind == NULL ? strlen(pLineBuffer) : (pNextLineFind-pLineBuffer);
		//printf("**** parseOneLine lineSize=%d,%x=%x\n", (int)lineSize,(int)pLineBuffer,(int)pNextLineFind);
		if (lineSize > 0)
		{
			char * pBufferLineTemp = new char[lineSize+1];
			strncpy(pBufferLineTemp, pLineBuffer, lineSize);
			pBufferLineTemp[lineSize] = '\0';
			int leftIndexTemp = 0;
			while (true)
			{
				int leftIndexTemp2 = 0;
				if (!m_bCodeBegin && sotpCompare(pBufferLineTemp+leftIndexTemp, CSP_TAB_WRITE.c_str(), leftIndexTemp2))
				{
					leftIndexTemp += leftIndexTemp2+CSP_TAB_WRITE.size();
					CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Write);
					if (scriptItem.get() == NULL)
						m_scripts.push_back(scriptItemNew);
					else
						scriptItem->m_subs.push_back(scriptItemNew);

					if (!getCSPObject(pBufferLineTemp+leftIndexTemp+1, scriptItemNew, leftIndexTemp2))
					{
						return NULL;
					}
					leftIndexTemp += leftIndexTemp2+1;
				}else if (!m_bCodeBegin && sotpCompare(pBufferLineTemp+leftIndexTemp, CSP_TAB_OUT_PRINT.c_str(), leftIndexTemp2))
				{
					leftIndexTemp += leftIndexTemp2+CSP_TAB_OUT_PRINT.size();
					CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_OutPrint);
					if (scriptItem.get() == NULL)
						m_scripts.push_back(scriptItemNew);
					else
						scriptItem->m_subs.push_back(scriptItemNew);

					if (!getCSPObject(pBufferLineTemp+leftIndexTemp+1, scriptItemNew, leftIndexTemp2))
					{
						return NULL;
					}
					leftIndexTemp += leftIndexTemp2+1;
				}
				const char * pFindBegin = strstr(pBufferLineTemp+leftIndexTemp, CSP_TAB_OUTPUT_BEGIN.c_str());
				if (pFindBegin == NULL)
				{
					// get all
					CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Write, std::string(pBufferLineTemp+leftIndexTemp));
					if (scriptItem.get() == NULL)
						addScriptItem(m_scripts, scriptItemNew);
					else
						addScriptItem(scriptItem->m_subs, scriptItemNew);
					break;
				}
				// get before of the "<%="
				CScriptItem::pointer scriptItemNew = CScriptItem::create(CScriptItem::CSP_Write, std::string(pBufferLineTemp+leftIndexTemp, pFindBegin-pBufferLineTemp-leftIndexTemp));
				if (scriptItem.get() == NULL)
					addScriptItem(m_scripts, scriptItemNew);
				else
					addScriptItem(scriptItem->m_subs, scriptItemNew);
				leftIndexTemp = int(pFindBegin-pBufferLineTemp) + CSP_TAB_OUTPUT_BEGIN.size();

				// get <%=abc%> abc value
				const char * pFindEnd = strstr(pBufferLineTemp+leftIndexTemp, CSP_TAB_OUTPUT_END.c_str());
				if (pFindEnd == NULL)
				{
					return NULL;
				}
				leftIndexTemp = int(pFindEnd - pBufferLineTemp) + CSP_TAB_OUTPUT_END.size();
				std::string sId(pFindBegin+CSP_TAB_OUTPUT_BEGIN.size(), pFindEnd-pFindBegin-CSP_TAB_OUTPUT_BEGIN.size());
				std::string::size_type nFindIndex1 = sId.find("[");
				std::string sIndex("");
				if (nFindIndex1 != std::string::npos)
				{
					sIndex = sId.substr(nFindIndex1+1, sId.size()-nFindIndex1-2);
					trimstring(sIndex);
					sId = sId.substr(0, nFindIndex1);
				}

				scriptItemNew = CScriptItem::create(CScriptItem::CSP_Write);
				scriptItemNew->setOperateObject1(CScriptItem::CSP_Operate_Id);
				scriptItemNew->setId(sId);
				if (scriptItem.get() == NULL)
					this->m_scripts.push_back(scriptItemNew);
				else
					scriptItem->m_subs.push_back(scriptItemNew);
				scriptItemNew->setProperty(sIndex);	// ??
			}

			delete[] pBufferLineTemp;
			if (pNextLineFind != NULL)
			{
				leftIndex += 1;
				if (!m_bCodeBegin)
				{
					// 后面还有数据
					if (scriptItem.get() == NULL)
						addScriptItem(m_scripts, CScriptItem::create(CScriptItem::CSP_NewLine));
					else
						addScriptItem(scriptItem->m_subs, CScriptItem::create(CScriptItem::CSP_NewLine));
				}
			}
		}else if (pNextLineFind != NULL)
		{
			if (!m_bCodeBegin)
				leftIndex += 1;
			//if (pLineBuffer[leftIndex]=='\n')
			//{
			//	leftIndex += 1;
			//	printf("******** leftIndex+1\n");
			//}
			if (!m_bCodeBegin)
			{
				// 后面还有数据
				if (scriptItem.get() == NULL)
					addScriptItem(m_scripts, CScriptItem::create(CScriptItem::CSP_NewLine));
				else
					addScriptItem(scriptItem->m_subs, CScriptItem::create(CScriptItem::CSP_NewLine));
			}
		}
	}

	if (m_bCodeBegin)
	{
		return pLineBuffer+leftIndex;
		//if (scriptItem.get()==NULL)
		//	return pLineBuffer+leftIndex;
		//else
		//{
		//	//const std::string sLiineTemp(pLineBuffer+leftIndex,10);
		//	//printf("**** before parseOneLine sLiineTemp=%s,scriptItem=%x,bEndReturn=%d\n", sLiineTemp.c_str(),(int)scriptItem.get(),(int)(bEndReturn?1:0));
		//	return parseOneLine(pLineBuffer+leftIndex, scriptItem, bEndReturn);
		//}
	}
	else
	{
		return pNextLineFind == NULL ? NULL : pNextLineFind+1;
	}
}

void CFileScripts::addScriptItem(std::vector<CScriptItem::pointer>& addTo, const CScriptItem::pointer& scriptItem) const
{
	assert(scriptItem.get() != NULL);

	if (!addTo.empty() && addTo[addTo.size()-1]->isHtmlText())
	{
		addTo[addTo.size()-1]->addHtmlText(scriptItem);
	}else
	{
		addTo.push_back(scriptItem);
	}
}

bool CFileScripts::addMethodScriptItem(const CScriptItem::pointer& scriptItem, CScriptItem::ItemType scriptItemType, const std::string & sMethodParam,
									   int needParamCount, bool popLastScript, const std::string & sValue)
{
	std::vector<std::string> methodParams;
	GetMethodParams(sMethodParam, methodParams);
	if (methodParams.size() < (size_t)needParamCount)
	//if (methodParams.size() != needParamCount)
		return false;

	if (popLastScript)
	{
		if (scriptItem.get() == NULL)
			m_scripts.pop_back();
		else
			scriptItem->m_subs.pop_back();
	}

	CScriptItem::pointer scriptItemNew = CScriptItem::create(scriptItemType);
	if (scriptItem.get() == NULL)
		m_scripts.push_back(scriptItemNew);
	else
		scriptItem->m_subs.push_back(scriptItemNew);
	switch (scriptItemType)
	{
	case CScriptItem::CSP_PageForward:
	case CScriptItem::CSP_PageLocation:
	case CScriptItem::CSP_PageInclude:
		{
			scriptItemNew->setValue(methodParams[0]);
			if (scriptItemType == CScriptItem::CSP_PageLocation)
				scriptItemNew->setProperty(methodParams[1]);
			return true;
		}break;
	default:
		{
			scriptItemNew->setOperateObject1(CScriptItem::CSP_Operate_Id);
			scriptItemNew->setId(methodParams[0]);
		}break;
	}
	if (methodParams.size() >= 2)
	{
		scriptItemNew->setOperateObject2(CScriptItem::CSP_Operate_Name);
		scriptItemNew->setName(methodParams[1]);
		scriptItemNew->setType(methodParams[1]);	// ?? CScriptItem::CSP_ToType
	}
	if (methodParams.size() >= 3)
	{
		scriptItemNew->setProperty(methodParams[2]);
	}
	scriptItemNew->setValue(sValue);

	return true;
}

bool CFileScripts::sotpCompare(const char * pBuffer, const char * pCompare, int & leftIndex,bool bSupportEnterChar)
{
	int i1 = 0, i2 = 0;
	leftIndex = 0;
	// 判断前面空格或者‘TAB’键；
	while (' ' == pBuffer[leftIndex] || '\t' == pBuffer[leftIndex] || (bSupportEnterChar && ('\r' == pBuffer[leftIndex] || '\n' == pBuffer[leftIndex])))
	{
		leftIndex++;
	}

	i1 = leftIndex;
	while (pCompare[i2] != '\0')
	{
		if (pCompare[i2++] != pBuffer[i1] || '\0' == pBuffer[i1])
		{
			return false;
		}
		i1++;
	}
	return true;
}

// ' ','\t'
bool CFileScripts::isInvalidateChar(char in) const
{
	return ' ' == in || '\t' == in;
}

bool CFileScripts::strGetValue(const char * pBuffer, char * outBuffer, int & leftIndex, int * outWordLen) const
{
	int i1 = 0, i2 = 0;
	bool result = false;
	leftIndex = 0;

	// ' ','\t'
	while (isInvalidateChar(pBuffer[leftIndex]))
	{
		leftIndex++;
	}
	if (pBuffer[leftIndex] != '\"')
	{
		return false;
	}
	leftIndex++;

	i1 = leftIndex;
	while (pBuffer[i1] != '\0')
	{
		if (i2 == MAX_VALUE_STRING_SIZE)
		{
			return false;
		}
		if (pBuffer[i1] == '\"')
		{
			result = true;
			break;
		}
		outBuffer[i2++] = pBuffer[i1++];
	}

	if (outWordLen != 0)
		*outWordLen = i2;

	outBuffer[i2] = '\0';
	return result;
}

} // namespace mycp
