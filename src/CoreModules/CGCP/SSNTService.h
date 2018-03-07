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

// SSNTService.h file here
#ifndef __CSSNTService_CLASS_INCLUDED__
#define __CSSNTService_CLASS_INCLUDED__

#include "NTService.h"
#include "CGC/CGCApp.h"


class CSSNTService
	: public CNTService
{
private:
	mycp::CGCApp::pointer m_App;

public:
	CSSNTService(void)
	{
		TCHAR chModulePath[MAX_PATH];
		memset(&chModulePath, 0, MAX_PATH);
		::GetModuleFileName(NULL, chModulePath, MAX_PATH);
		TCHAR* temp = (TCHAR*)0;
		temp = _tcsrchr(chModulePath, (unsigned int)'\\');
		chModulePath[temp - chModulePath] = '\0';

		m_App = mycp::CGCApp::create(chModulePath);
		m_sServiceName = _T("CGCP");
		m_sServiceDisplayName = _T("CGCP Server");
	}

private:
	virtual void Run(){
		m_App->AppInit();
		m_App->AppStart(0);
	}
	virtual void Stop(){
		m_App->AppExit();
	}
};


#endif // __CSSNTService_CLASS_INCLUDED__
