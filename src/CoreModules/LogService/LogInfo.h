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

// LogInfo.h file here
#ifndef _loginfo_head_included__
#define _loginfo_head_included__

#include <sys/timeb.h>
#include <boost/shared_ptr.hpp>

class CLogInfo
{
public:
	typedef std::shared_ptr<CLogInfo> pointer;

	static CLogInfo::pointer create(LogLevel level, const tstring & msg)
	{
		return CLogInfo::pointer(new CLogInfo(level, msg));
	}

public:
	LogLevel m_level;
	tstring m_msg;
	struct timeb m_logtime;

public:
	CLogInfo(LogLevel level, const tstring & msg)
		: m_level(level)
		, m_msg(msg)
	{
		ftime(&m_logtime);
	}
};

#endif // _loginfo_head_included__
