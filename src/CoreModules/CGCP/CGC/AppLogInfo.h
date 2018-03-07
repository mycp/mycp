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

// AppLogInfo.h file here
#ifndef _APPLOGINFO_HEAD_INCLUDED__
#define _APPLOGINFO_HEAD_INCLUDED__

#include "../../CGCClass/cgcdef.h"
#include <sys/timeb.h>
#include <boost/shared_ptr.hpp>

class AppLogInfo
{
public:
	typedef boost::shared_ptr<AppLogInfo> pointer;

	static AppLogInfo::pointer create(DebugLevel dl, const tstring & msg)
	{
		return AppLogInfo::pointer(new AppLogInfo(dl, msg));
	}

public:
	DebugLevel m_dl;
	tstring m_msg;
	struct timeb m_logtime;

//public:
//	DebugLevel getdl(void) const {return m_dl;}
//	const tstring & getmsg(void) const {return m_msg;}
//	const struct timeb & getlogtime(void) const {return m_logtime;}

public:
	AppLogInfo(DebugLevel dl, const tstring & msg)
		: m_dl(dl)
		, m_msg(msg)
	{
		ftime(&m_logtime);
	}
	virtual ~AppLogInfo(void)
	{}

};

//typedef	std::list<AppLogInfo*> AppLogInfoList;
//typedef AppLogInfoList::iterator AppLogInfoListIter;
//typedef AppLogInfoList::const_iterator AppLogInfoListConstIter;

#endif // _APPLOGINFO_HEAD_INCLUDED__
