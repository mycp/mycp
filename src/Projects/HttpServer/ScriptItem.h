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

// ScriptItem.h file here
#ifndef __scriptitem__head__
#define __scriptitem__head__

// cgc head
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace mycp {

/////////////////////////////////////////////////////////
const int MAX_VALUE_STRING_SIZE = 256;

class CScriptItem
{
public:
	typedef std::shared_ptr<CScriptItem> pointer;
	enum ItemType
	{
		CSP_Unknown								// 未知
		, CSP_Println		= 0x100		// 不要轻易改
		, CSP_Write
		, CSP_NewLine
		, CSP_OutPrint
		, CSP_Define			= 0x120

		, CSP_Add				= 0x130
		, CSP_Subtract
		, CSP_Multi
		, CSP_Division
		, CSP_Modulus
		, CSP_Increase
		, CSP_Decrease

		, CSP_Execute			= 0x160

		, CSP_AppInit			= 0x200
		, CSP_AppFinal
		, CSP_AppCall
		, CSP_AppGet
		, CSP_AppSet
		, CSP_AppAdd
		, CSP_AppDel
		, CSP_AppInfo
		, CSP_CDBCInit			= 0x210
		, CSP_CDBCFinal
		, CSP_CDBCOpen
		, CSP_CDBCClose
		, CSP_CDBCExec
		, CSP_CDBCSelect
		, CSP_CDBCMoveIndex
		, CSP_CDBCFirst
		, CSP_CDBCNext
		, CSP_CDBCPrev
		, CSP_CDBCLast
		, CSP_CDBCReset
		, CSP_CDBCSize
		, CSP_CDBCIndex

		, CSP_IfEqual			= 0x300
		, CSP_IfNotEqual
		, CSP_IfGreater
		, CSP_IfGreaterEqual
		, CSP_IfLess
		, CSP_IfLessEqual
		, CSP_IfAnd
		, CSP_IfOr
		, CSP_IfEmpty
		, CSP_IfNotEmpty
		, CSP_IfExist
		, CSP_IfNotExist
		, CSP_IfElse
		, CSP_WhileEqual		= 0x310
		, CSP_WhileNotEqual
		, CSP_WhileGreater
		, CSP_WhileGreaterEqual
		, CSP_WhileLess
		, CSP_WhileLessEqual
		, CSP_WhileAnd
		, CSP_WhileOr
		, CSP_WhileEmpty
		, CSP_WhileNotEmpty
		, CSP_WhileExist
		, CSP_WhileNotExist
		, CSP_Foreach			= 0x320
		, CSP_Break
		, CSP_Continue
		, CSP_End

		, CSP_Empty			= 0x400
		, CSP_Reset
		, CSP_Sizeof
		, CSP_Typeof
		, CSP_ToType
		, CSP_Index

		, CSP_PageContentType		= 0x500
		, CSP_PageReturn
		, CSP_PageReset
		, CSP_PageForward
		, CSP_PageLocation
		, CSP_PageInclude
		, CSP_Authenticate			= 0x510

		//, CSP_GetHead		= 0x500	// VALUE
		//, CSP_GetParam

	};

	enum OperateObject
	{
		CSP_Operate_None
		, CSP_Operate_Name
		, CSP_Operate_Id
		, CSP_Operate_Value
	};

	static CScriptItem::pointer create(ItemType type)
	{
		return CScriptItem::pointer(new CScriptItem(type));
	}
	static CScriptItem::pointer create(ItemType type, const std::string& value)
	{
		return CScriptItem::pointer(new CScriptItem(type, value));
	}

	std::vector<CScriptItem::pointer> m_subs;
	std::vector<CScriptItem::pointer> m_elseif;
	std::vector<CScriptItem::pointer> m_else;

	void setItemType(ItemType newv) {m_itemType = newv;}
	ItemType getItemType(void) const {return m_itemType;}
	bool isItemType(ItemType type) const {return m_itemType == type;}

	void setOperateObject1(OperateObject newv) {m_operateObject1 = newv;}
	OperateObject getOperateObject1(void) const {return m_operateObject1;}
	void setOperateObject2(OperateObject newv) {m_operateObject2 = newv;}
	OperateObject getOperateObject2(void) const {return m_operateObject2;}

	void setId(const std::string& id) {m_id = id;}
	const std::string& getId(void) const {return m_id;}

	void setScope(const std::string& scope) {m_scope = scope;}
	const std::string& getScope(void) const {return m_scope;}

	void setName(const std::string& name) {m_name = name;}
	const std::string& getName(void) const {return m_name;}

	void setProperty(const std::string& property) {m_property = property;}
	const std::string& getProperty(void) const {return m_property;}

	void setProperty2(const std::string& property) {m_property2 = property;}
	const std::string& getProperty2(void) const {return m_property2;}

	void setValue(const std::string& value) {m_value = value;}
	const std::string& getValue(void) const {return m_value;}

	void setType(const std::string& v) {m_type = v;}
	const std::string& getType(void) const {return m_type;}

	bool isHtmlText(void) const;
	void addHtmlText(const CScriptItem::pointer& scriptItem);
	void validataHtmlText(std::string& htmlText) const;

	static bool isInvalidateChar(char in);

public:
	CScriptItem(ItemType type);
	CScriptItem(ItemType type, const std::string& value);
	~CScriptItem(void);

protected:
	ItemType m_itemType;
	OperateObject m_operateObject1;
	OperateObject m_operateObject2;
	std::string m_id;
	std::string m_scope;
	std::string m_name;
	std::string m_property;
	std::string m_property2;
	std::string m_value;
	std::string m_type;
};
const CScriptItem::pointer NullScriptItem;

} // namespace mycp

#endif // __scriptitem__head__
