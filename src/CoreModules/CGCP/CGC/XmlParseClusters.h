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

// XmlParseClusters.h file here
#ifndef _XMLPARSECLUSTERS_HEAD_INCLUDED__
#define _XMLPARSECLUSTERS_HEAD_INCLUDED__

#include "IncludeBase.h"


class XmlParseClusters
	: public _ParseEvent
{
public:
	XmlParseClusters(void);
	virtual ~XmlParseClusters(void);

public:
	void FreeHandle(void);
	int getClusterType(void) const {return m_clusterType;}
	const ClusterSvrList & getClusters(void) const {return m_listClusterSvr;}

	//
	// 根据群集类型和级别调整 LIST 链表位置
	void buildListByRank(void);

private:
    virtual int startElement(const XMLCh* const elementname, AttributeList& attributes);
    virtual int characters(const XMLCh* const chars, const unsigned int length);

	// RANK值小(高级别)放在链表前面
	int buildRankType1(ClusterSvr * clusterSvrCompare = 0);

private:
	int m_clusterType;
	ClusterSvrList m_listClusterSvr;
	bool m_bAlreadyBuild;

};

#endif // _XMLPARSECLUSTERS_HEAD_INCLUDED__
