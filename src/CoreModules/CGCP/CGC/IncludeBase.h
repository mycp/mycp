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

// includebase.h file here
#ifndef _INCLUDEBASE_HEAD_VER_1_0_0_0__INCLUDED_
#define _INCLUDEBASE_HEAD_VER_1_0_0_0__INCLUDED_

#ifdef WIN32
#pragma warning(disable:4819 4311 4267 4996)
#endif // WIN32

//
// system
#include "map"
#include "iostream"
#include "fstream"
#include "string"

//
// boost
#include <boost/lexical_cast.hpp>
#include "boost/regex.hpp"
#include <boost/function.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//
// owner
// ? #include "CWSSOAPServer.h"

#include <CGCBase/includeall.h>
#include <CGCClass/cgcclassinclude.h>
#include <ThirdParty/stl/locklist.h>
#include <ThirdParty/stl/lockmap.h>

//
// typedef
#ifdef _UNICODE
//typedef std::wcout					stdtcout;

typedef boost::wformat				tformat;
typedef boost::filesystem::wpath	tpath;
#else
//typedef std::cout					stdtcout;

typedef boost::format				tformat;
typedef boost::filesystem::path		tpath;
#endif // _UNICODE

typedef std::map<unsigned int, boost::thread*> BThreadUIntMap;
typedef std::pair<unsigned int, boost::thread*> BThreadUIntMapPair;

///////////////////////////////////////////
//


using namespace mycp;

#endif
