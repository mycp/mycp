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

#include "ScriptItem.h"

namespace mycp {

CScriptItem::CScriptItem(ItemType type)
: m_itemType(type)
, m_operateObject2(CSP_Operate_None)
, m_id(""), m_scope("")
, m_name(""), m_property("")
, m_value(""), m_type("")
{
	m_operateObject1 =  m_itemType == CSP_NewLine ? CSP_Operate_Value : CSP_Operate_None;
	//m_operateObject1 = (m_itemType >= CSP_Println && m_itemType <= CSP_NewLine) ? CSP_Operate_Value : CSP_Operate_None;
}

CScriptItem::CScriptItem(ItemType type, const std::string& value)
: m_itemType(type)
, m_operateObject1(CSP_Operate_Value), m_operateObject2(CSP_Operate_None)
, m_id(""), m_scope("")
, m_name(""), m_property("")
, m_value(value), m_type("")
{
	validataHtmlText(m_value);
}

CScriptItem::~CScriptItem(void)
{
	m_subs.clear();
	m_elseif.clear();
	m_else.clear();
}

bool CScriptItem::isInvalidateChar(char in)
{
	return ' ' == in || '\t' == in;
}

bool CScriptItem::isHtmlText(void) const
{
	return m_operateObject1 == CSP_Operate_Value && m_itemType >= CSP_Println && m_itemType <= CSP_NewLine;
}

void CScriptItem::addHtmlText(const CScriptItem::pointer& scriptItem)
{
	assert (scriptItem.get() != NULL);
	assert (scriptItem->isHtmlText());
	assert (this->isHtmlText());

	switch (scriptItem->getItemType())
	{
	case CScriptItem::CSP_Println:
		{
			if (this->getItemType() == CScriptItem::CSP_NewLine)
			{
				m_itemType = scriptItem->getItemType();
				m_value = "\r\n";
				m_value.append(scriptItem->getValue());
			}else if (this->getItemType() == CScriptItem::CSP_Println)
			{
				m_value.append("\r\n");
				m_value.append(scriptItem->getValue());
			}else if (this->getItemType() == CScriptItem::CSP_Write)
			{
				m_value.append(scriptItem->getValue());
				m_value.append("\r\n");
			}
		}break;
	case CScriptItem::CSP_Write:
		{
			if (this->getItemType() == CScriptItem::CSP_NewLine)
			{
				m_itemType = scriptItem->getItemType();
				m_value = "\r\n";
				m_value.append(scriptItem->getValue());
			}else if (this->getItemType() == CScriptItem::CSP_Println)
			{
				m_value.append(scriptItem->getValue());
			}else if (this->getItemType() == CScriptItem::CSP_Write)
			{
				m_value.append(scriptItem->getValue());
			}
		}break;
	case CScriptItem::CSP_NewLine:
		{
			if (this->getItemType() == CScriptItem::CSP_NewLine)
			{
				m_itemType = CScriptItem::CSP_Write;
				m_value = "\r\n\r\n";
			}else if (this->getItemType() == CScriptItem::CSP_Println)
			{
				m_value.append("\r\n");
			}else if (this->getItemType() == CScriptItem::CSP_Write)
			{
				m_value.append("\r\n");
			}
		}break;
	default:
		break;
	}

}

void CScriptItem::validataHtmlText(std::string& htmlText) const
{
	for (size_t i=0; i<htmlText.size(); i++)
	{
		if (!isInvalidateChar(htmlText.c_str()[i]))
		{
			return;
		}
	}

	htmlText = "";
}

} // namespace mycp
