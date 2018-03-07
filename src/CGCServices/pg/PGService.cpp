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

#define USES_PGCDBC		1		// [0,1]


#ifdef WIN32
#pragma warning(disable:4819 4267)
#include <winsock2.h>
#include <windows.h>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#else
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#endif // WIN32

#include <CGCBase/cdbc.h>
#include <CGCBase/cgcApplication2.h>
using namespace mycp;

#if (USES_PGCDBC)
#include "db/pool.h"

#ifdef WIN32
#pragma comment(lib,"libpq.lib")
#endif // WIN32
mycp::cgcApplication2::pointer theApplication2;

class CCDBCResultSet
{
public:
	typedef boost::shared_ptr<CCDBCResultSet> pointer;

	CCDBCResultSet::pointer copyNew(void) const
	{
		CCDBCResultSet::pointer result = CCDBCResultSet::pointer(new CCDBCResultSet(m_rows,m_cols));
		result->m_pColsName = this->m_pColsName;
		CLockMap<mycp::bigint, cgcValueInfo::pointer>::const_iterator pIter = m_pResults.begin();
		for (; pIter!=m_pResults.end(); pIter++)
		{
			result->m_pResults.insert(pIter->first, pIter->second, NULL, NULL, false);
			//cgcValueInfo::pointer pResult = pIter->second->copy();
			//result->m_pResults.insert(pIter->first, pResult, NULL, NULL, false);
		}
		return result;
	}
	void BuildAllRecords(void)
	{
		cols_name();
		cgcValueInfo::pointer record = first();
		while (record.get()!=NULL)
		{
			record = next();
		}
		m_currentIndex = 0;
	}
	
	//void SetCacheTime(time_t t) {m_tCacheTime = t;}
	bool IsCreateTimeout(int nCacehMinutes) const {return (m_tCreateTime+nCacehMinutes*60)<time(0)?true:false;}
	void SetMemoryTimeout(time_t t) {m_tMemoryTimeout = t;}
	bool IsMemoryTimeout(time_t tNow) {return (m_tMemoryTimeout>0 && m_tMemoryTimeout<tNow)?true:false;}

	mycp::bigint size(void) const{return m_rows;}
	mycp::bigint cols(void) const{return m_cols;}
	mycp::bigint index(void) const {return m_currentIndex;}
	cgcValueInfo::pointer cols_name(void) const
	{
		if (m_pColsName.get()==NULL)
		{
			if (m_resultset == NULL || m_rows == 0 || m_cols==0) return cgcNullValueInfo;
			std::vector<cgcValueInfo::pointer> record;
			try
			{
				for(int i=0; i<m_cols; i++)
				{
					const tstring s = result_fname(m_sink, m_resultset, i);
					record.push_back(CGC_VALUEINFO(s));
				}
			}catch(std::exception&)
			{
			}catch(...)
			{
			}
			const_cast<CCDBCResultSet*>(this)->m_pColsName = CGC_VALUEINFO(record);
		}
		return m_pColsName;
	}
	cgcValueInfo::pointer index(int moveIndex)
	{
		if (m_rows == 0) return cgcNullValueInfo;
		//if (m_resultset == NULL || m_rows == 0) return cgcNullValueInfo;
		if (moveIndex < 0 || (moveIndex+1) > m_rows) return cgcNullValueInfo;
		m_currentIndex = moveIndex;
		return getCurrentRecord();
	}
	cgcValueInfo::pointer first(void)
	{
		if (m_rows == 0) return cgcNullValueInfo;
		//if (m_resultset == NULL || m_rows == 0) return cgcNullValueInfo;
		m_currentIndex = 0;
		return getCurrentRecord();
	}
	cgcValueInfo::pointer next(void)
	{
		if (m_rows == 0) return cgcNullValueInfo;
		//if (m_resultset == NULL || m_rows == 0) return cgcNullValueInfo;
		if (m_currentIndex+1 == m_rows) return cgcNullValueInfo;
		++m_currentIndex;
		return getCurrentRecord();
	}
	cgcValueInfo::pointer previous(void)
	{
		if (m_rows == 0) return cgcNullValueInfo;
		//if (m_resultset == NULL || m_rows == 0) return cgcNullValueInfo;
		if (m_currentIndex == 0) return cgcNullValueInfo;
		--m_currentIndex;
		return getCurrentRecord();
	}
	cgcValueInfo::pointer last(void)
	{
		if (m_rows == 0) return cgcNullValueInfo;
		//if (m_resultset == NULL || m_rows == 0) return cgcNullValueInfo;
		m_currentIndex = m_rows - 1;
		return getCurrentRecord();
	}
	void reset(void)
	{
		if (m_resultset!=NULL)
		{
			try
			{
				result_clean(m_sink, m_resultset);
				sink_pool_put (m_sink);
			}catch(std::exception&)
			{
			}catch(...)
			{}
			m_resultset = NULL;
		}
		//m_rows = 0;
		//m_cols = 0;
		//m_pColsName.reset();
		//m_pResults.clear();
	}

	CCDBCResultSet(Sink * sink, Result * resultset, mycp::bigint rows)
		: m_sink(sink), m_resultset(resultset), m_rows(rows)
		, m_tMemoryTimeout(0)//, m_tCacheTime(0)
	{
		m_cols = result_cn(m_sink, m_resultset);
		m_tCreateTime = time(0);
	}
	CCDBCResultSet(mycp::bigint rows, int cols)
		: m_sink(NULL), m_resultset(NULL), m_rows(rows), m_cols(cols)
		, m_tMemoryTimeout(0)//, m_tCacheTime(0)
	{
		m_tCreateTime = time(0);
	}
	virtual ~CCDBCResultSet(void)
	{
		reset();
		m_rows = 0;
		m_cols = 0;
		m_pColsName.reset();
		m_pResults.clear();
	}

protected:
	cgcValueInfo::pointer getCurrentRecord(void) const
	{
		cgcValueInfo::pointer result;
		if (!m_pResults.find(m_currentIndex,result))
		{
			if (m_resultset == NULL) return cgcNullValueInfo;
			//assert (m_resultset != NULL);
			assert (m_currentIndex >= 0 && m_currentIndex < m_rows);
			std::vector<cgcValueInfo::pointer> record;
			try
			{
				for(int i=0; i<m_cols; i++)
				{
					const tstring s = result_get(m_sink, m_resultset, (int)m_currentIndex, i);
					record.push_back(CGC_VALUEINFO(s));
				}
			}catch(std::exception&)
			{
			}catch(...)
			{
			}
			result = CGC_VALUEINFO(record);
			const_cast<CCDBCResultSet*>(this)->m_pResults.insert(m_currentIndex,result);
		}
		return result;
	}

private:
	Sink *		m_sink;
	Result *	m_resultset;
	mycp::bigint			m_rows;
	int			m_cols;
	mycp::bigint			m_currentIndex;
	cgcValueInfo::pointer m_pColsName;
	CLockMap<mycp::bigint, cgcValueInfo::pointer> m_pResults;
	time_t m_tMemoryTimeout;
	time_t m_tCreateTime;
};

#define CDBC_RESULTSET(sink, res, rows) CCDBCResultSet::pointer(new CCDBCResultSet(sink, res, rows))

const int escape_in_size = 2;
const tstring escape_in[] = {"''","\\\\"};
const tstring escape_out[] = {"'","\\"};
//const int escape_old_out_size = 2;	// ?兼容旧版本
//const tstring escape_old_in[] = {"&lsquo;","&pge0;"};
//const tstring escape_old_out[] = {"'","\\"};
////const std::string escape_in[] = {"&lsquo;","&pge0;"};
////const std::string escape_out[] = {"'","\\"};

class CPgCdbc
	: public cgcCDBCService
	, public cgcOnTimerHandler
	, public boost::enable_shared_from_this<CPgCdbc>
{
public:
	typedef boost::shared_ptr<CPgCdbc> pointer;

	CPgCdbc(int nIndex)
		: m_nIndex(nIndex), m_isopen(false)
		, m_tLastTime(0)
		, m_nDbPort(0)
	{}
	virtual ~CPgCdbc(void)
	{
		finalService();
	}
	virtual bool initService(cgcValueInfo::pointer parameter)
	{
		if (isServiceInited()) return true;
		m_bServiceInited = true;
		theApplication->SetTimer(m_nIndex+1, 2000, shared_from_this());
		return isServiceInited();
	}
	virtual void finalService(void)
	{
		theApplication->KillTimer(m_nIndex+1);
		close();
		m_cdbcInfo.reset();
		m_bServiceInited = false;
		m_sDbHost.clear();
		m_nDbPort = 0;
	}
	void CheckMemoryResultsTimeout(void)
	{
		{
			const time_t tNow = time(0);
			BoostWriteLock wtlock(m_pMemorySqlResults.mutex());
			CLockMap<tstring, CCDBCResultSet::pointer>::iterator pIter = m_pMemorySqlResults.begin();
			for (; pIter!=m_pMemorySqlResults.end(); )
			{
				const CCDBCResultSet::pointer& pResultSet = pIter->second;
				if (pResultSet->IsMemoryTimeout(tNow))
					m_pMemorySqlResults.erase(pIter++);
				else
					pIter++;
			}
		}
		static unsigned int theIndex = 0;
		theIndex++;
		if ((theIndex%60)==59)	// 10分钟判断一次	
		{
			BoostWriteLock wtlock(m_results.mutex());
			CLockMap<int, CCDBCResultSet::pointer>::iterator pIter = m_results.begin();
			for (; pIter!=m_results.end(); )
			{
				const CCDBCResultSet::pointer& pResultSet = pIter->second;
				if (pResultSet->IsCreateTimeout(30))	// *** 30分钟
				{
					m_results.erase(pIter++);
				}else
					pIter++;
			}
		}

	}
private:
	virtual tstring serviceName(void) const {return _T("PGCDBC");}
	boost::shared_mutex m_mutex; 
	virtual void OnTimeout(unsigned int nIDEvent, const void * pvParam)
	{
		try
		{
			BoostWriteLock wtlock(m_mutex);
			if (isopen())
			{
				// 主要用于整理数据库连接池；
				//printf("*********** OnTimeout...\n");
				Sink *sink = sink_pool_get();
				if (sink!=NULL)
					sink_pool_put(sink);
			}
		}catch(std::exception&)
		{
		}catch(...)
		{
		}
	}

	static const tstring & replace(tstring & strSource, const tstring & strFind, const tstring &strReplace)
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

	virtual void escape_string_in(tstring & str)
	{
		for (int i=0; i<escape_in_size; i++)
			replace(str, escape_out[i], escape_in[i]);
	}
	virtual void escape_string_out(tstring & str)
	{
		//return;
		//for (int i=0; i<escape_old_out_size; i++)
		//	replace(str, escape_old_in[i], escape_old_out[i]);
	}
	
	virtual bool open(const cgcCDBCInfo::pointer& cdbcInfo)
	{
		if (cdbcInfo.get() == NULL) return false;
		if (!isServiceInited()) return false;

		if (m_cdbcInfo.get() == cdbcInfo.get() && this->isopen()) return true;

		// close old database;
		close();
		m_cdbcInfo = cdbcInfo;
		m_tLastTime = time(0);

		try
		{
			const tstring sHost = m_cdbcInfo->getHost();
			std::string::size_type find = sHost.find(_T(":"));
			if (find == std::string::npos)
				return false;
			m_sDbHost = sHost.substr(0, find);
			m_nDbPort = atoi(sHost.substr(find+1).c_str());
			const unsigned short nMin = m_cdbcInfo->getMinSize();
			const unsigned short nMax = m_cdbcInfo->getMaxSize();

			const int ret = sink_pool_init(POSTSINK,
				nMin, nMax,
				m_sDbHost.c_str(),
				m_nDbPort,
				0,
				m_cdbcInfo->getDatabase().c_str(),
				m_cdbcInfo->getAccount().c_str(),
				m_cdbcInfo->getSecure().c_str());
			if (ret < 0)
			{
				CGC_LOG((mycp::LOG_ERROR, "Can't open database: %s\n", m_cdbcInfo->getDatabase().c_str()));
				//printf("%s\n", "connect to database failed!, start failed!");
				return false;
			}
		}catch(std::exception&)
		{
			return false;
		}catch(...)
		{
			return false;
		}

		m_isopen = true;
		return true;
	}
	virtual bool open(void) {return open(m_cdbcInfo);}
	virtual void close(void)
	{
		if (m_isopen)
		{
			m_isopen = false;
			m_tLastTime = time(0);
			try
			{
				BoostWriteLock wtlock(m_mutex);
				sink_pool_cleanup();
			}catch(const std::exception&)
			{
			}catch(...)
			{
			}
		}
		m_results.clear();
		m_pMemorySqlResults.clear();
	}
	virtual bool isopen(void) const
	{
		return m_isopen;
	}
	virtual time_t lasttime(void) const {return m_tLastTime;}

#ifdef WIN32
	bool ExecDosCmd(const char* sAppName,const char* sCommandLine)
	{
		TCHAR               CommandLine[1024*10]   = {0};
		GetSystemDirectory(CommandLine, MAX_PATH);
		strcat_s(CommandLine, MAX_PATH, _T("\\cmd.exe /c "));  
		strcat_s(CommandLine, 8*1024, sCommandLine);  

		STARTUPINFO si;  
		PROCESS_INFORMATION pi;
		memset(&si,0,sizeof(STARTUPINFO));
		memset(&pi,0,sizeof(PROCESS_INFORMATION));
		si.cb = sizeof(STARTUPINFO);  
		GetStartupInfo(&si);   
		si.wShowWindow = SW_HIDE;  
		si.dwFlags = STARTF_USESHOWWINDOW;  
		if (!CreateProcess(sAppName, CommandLine,NULL,NULL,TRUE,NULL,NULL,NULL,&si,&pi))
		{
			return false;
		}
		WaitForSingleObject(pi.hProcess,INFINITE);
		return true;
	}
#endif
	virtual bool backup_database(const char * sBackupTo, const char* sProgram)
	{
		if (!isServiceInited() || sProgram==NULL || sBackupTo==NULL) return false;
		if (!open()) return false;

		char lpszCommand[512];
#ifdef WIN32
		// pg_dump.exe -D -h localhost -p port -U user mydb >  mydb.bak
		sprintf(lpszCommand,"set PGPASSWORD=%s&\"%s\" -h %s -p %d -U %s --blobs %s > \"%s\"",m_cdbcInfo->getSecure().c_str(),sProgram,
			m_sDbHost.c_str(),m_nDbPort,m_cdbcInfo->getAccount().c_str(),m_cdbcInfo->getDatabase().c_str(),sBackupTo);
		return ExecDosCmd(NULL,lpszCommand);
#else
		sprintf(lpszCommand,"export PGPASSWORD=%s ; \"%s\" -h %s -p %d -U %s --blobs %s > \"%s\"",m_cdbcInfo->getSecure().c_str(),sProgram,
			m_sDbHost.c_str(),m_nDbPort,m_cdbcInfo->getAccount().c_str(),m_cdbcInfo->getDatabase().c_str(),sBackupTo);
		system(lpszCommand);
		return true;
#endif
	}

	virtual mycp::bigint execute(const char * exeSql, int nTransaction)
	{
		if (exeSql == NULL || !isServiceInited()) return -1;
		if (!open()) return -1;

		mycp::bigint ret = 0;
		Sink *sink = (Sink*)nTransaction;
		try
		{
			//printf("*********** execute(%s)...\n",exeSql);
			m_tLastTime = time(0);
			if (nTransaction==0)
				sink = sink_pool_get();
			if (sink==NULL)
				return -1;

			Result * res = sink_exec (sink, exeSql);
			int state = result_state (sink, res);
			if((state != RES_COMMAND_OK) &&
				(state != RES_TUPLES_OK)  &&
				(state != RES_COPY_IN)  &&
				(state != RES_COPY_OUT))
			{
				//result_clean(sink, res);
				//CGC_LOG((mycp::LOG_WARNING, "1 state=%d,%s\n", state, exeSql));
				//res = sink_exec (sink, exeSql);
				//state = result_state (sink, res);
				//if((state != RES_COMMAND_OK) &&
				//	(state != RES_TUPLES_OK)  &&
				//	(state != RES_COPY_IN)  &&
				//	(state != RES_COPY_OUT))
				{
					result_clean(sink, res);
					if (nTransaction==0)
						sink_pool_put (sink);
					//else
					//	trans_rollback(nTransaction);
					CGC_LOG((mycp::LOG_WARNING, "2 state=%d,%s\n", state, exeSql));
					return -1;
				}
			}
			//ret = result_rn (sink, res);
			const tstring& sSyncName = this->get_datasource();
			if (!sSyncName.empty())
				theApplication2->sendSyncData(sSyncName,0,exeSql,strlen(exeSql),false);

			if (nTransaction==0)
			{
				const char * sAffectedRows = result_affected_rows(sink,res);
				if (sAffectedRows!=NULL)
					ret = cgc_atoi64(sAffectedRows);
				result_clean (sink, res);
				sink_pool_put (sink);
			}
		}catch(std::exception&)
		{
			if (nTransaction==0)
				sink_pool_put (sink);
			//else
			//	trans_rollback(nTransaction);
			CGC_LOG((mycp::LOG_ERROR, "%s\n", exeSql));
			return -1;
		}catch(...)
		{
			if (nTransaction==0)
				sink_pool_put (sink);
			//else
			//	trans_rollback(nTransaction);
			CGC_LOG((mycp::LOG_ERROR, "%s\n", exeSql));
			return -1;
		}
		return ret;
	}

	virtual mycp::bigint select(const char * selectSql, int& outCookie, int nCacheMinutes)
	{
		outCookie = 0;
		if (selectSql == NULL || !isServiceInited()) return -1;
		if (!open()) return -1;

		const tstring sSqlString(selectSql);
		CCDBCResultSet::pointer pSqlResultSetTemp;
		if (nCacheMinutes>0 && m_pMemorySqlResults.find(sSqlString,pSqlResultSetTemp) && !pSqlResultSetTemp->IsCreateTimeout(nCacheMinutes))
		{
			CCDBCResultSet::pointer pSqlResultSetNew = pSqlResultSetTemp->copyNew();
			outCookie = (int)pSqlResultSetNew.get();
			m_results.insert(outCookie, pSqlResultSetNew);
			return pSqlResultSetNew->size();
		}

		mycp::bigint rows = 0;
		Sink *sink = NULL;
		try
		{
			//printf("*********** select(%s)...\n",selectSql);
			m_tLastTime = time(0);
			sink = sink_pool_get();
			if (sink==NULL)
				return -1;

			Result * res = sink_exec( sink, selectSql);
			const int state = result_state (sink, res);
			if((state != RES_COMMAND_OK) &&
				(state != RES_TUPLES_OK)  &&
				(state != RES_COPY_IN)    &&
				(state != RES_COPY_OUT))
			{
				result_clean(sink, res);
				sink_pool_put (sink);
				CGC_LOG((mycp::LOG_WARNING, "%s\n", selectSql));
				return -1;
				//return 0;
			}
			rows = (mycp::bigint)result_rn(sink, res);
			if (rows > 0)
			{
				outCookie = (int)res;
				CCDBCResultSet::pointer pSqlResultSet = CDBC_RESULTSET(sink, res, rows);
				m_results.insert(outCookie, pSqlResultSet);
				if (nCacheMinutes>0)
				//if (nCacheMinutes>0 && pSqlResultSetTemp.get()==NULL)
				{
					pSqlResultSet->BuildAllRecords();
					const time_t tNow = time(0);
					//pSqlResultSet->SetCacheTime(tNow);
					pSqlResultSet->SetMemoryTimeout(tNow+nCacheMinutes*60);
					m_pMemorySqlResults.insert(sSqlString,pSqlResultSet);
				}
			}else
			{
				result_clean(sink, res);
				sink_pool_put (sink);
			}
		}catch(std::exception&)
		{
			sink_pool_put (sink);
			CGC_LOG((mycp::LOG_ERROR, "%s\n", selectSql));
			return -1;
		}catch(...)
		{
			sink_pool_put (sink);
			CGC_LOG((mycp::LOG_ERROR, "%s\n", selectSql));
			return -1;
		}
		return rows;
	}
	virtual mycp::bigint select(const char * selectSql)
	{
		if (selectSql == NULL || !isServiceInited()) return -1;
		if (!open()) return -1;

		mycp::bigint rows = 0;
		Sink *sink = NULL;
		try
		{
			//printf("*********** select2(%s)...\n",selectSql);
			m_tLastTime = time(0);
			sink = sink_pool_get();
			if (sink==NULL)
				return -1;

			Result * res = sink_exec( sink, selectSql);
			const int state = result_state (sink, res);
			if((state != RES_COMMAND_OK) &&
				(state != RES_TUPLES_OK)  &&
				(state != RES_COPY_IN)    &&
				(state != RES_COPY_OUT))
			{
				result_clean(sink, res);
				sink_pool_put (sink);
				CGC_LOG((mycp::LOG_WARNING, "%s\n", selectSql));
				return -1;
				//return 0;
			}
			rows = (mycp::bigint)result_rn(sink, res);
			result_clean(sink, res);
			sink_pool_put (sink);
		}catch(std::exception&)
		{
			sink_pool_put (sink);
			CGC_LOG((mycp::LOG_ERROR, "%s\n", selectSql));
			return -1;
		}catch(...)
		{
			sink_pool_put (sink);
			CGC_LOG((mycp::LOG_ERROR, "%s\n", selectSql));
			return -1;
		}
		return rows;
	}

	virtual mycp::bigint size(int cookie) const
	{
		if (cookie==0) {
			return -1;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->size() : -1;
	}
	virtual int cols(int cookie) const
	{
		if (cookie==0) {
			return -1;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? (int)cdbcResultSet->cols() : -1;
	}

	virtual mycp::bigint index(int cookie) const
	{
		if (cookie==0) {
			return -1;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->index() : -1;
	}
	virtual cgcValueInfo::pointer cols_name(int cookie) const
	{
		if (cookie==0) {
			return cgcNullValueInfo;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->cols_name() : cgcNullValueInfo;
	}

	virtual cgcValueInfo::pointer index(int cookie, mycp::bigint moveIndex)
	{
		if (cookie==0) {
			return cgcNullValueInfo;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->index((int)moveIndex) : cgcNullValueInfo;
	}
	virtual cgcValueInfo::pointer first(int cookie)
	{
		if (cookie==0) {
			return cgcNullValueInfo;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->first() : cgcNullValueInfo;
	}
	virtual cgcValueInfo::pointer next(int cookie)
	{
		if (cookie==0) {
			return cgcNullValueInfo;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->next() : cgcNullValueInfo;
	}
	virtual cgcValueInfo::pointer previous(int cookie)
	{
		if (cookie==0) {
			return cgcNullValueInfo;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->previous() : cgcNullValueInfo;
	}
	virtual cgcValueInfo::pointer last(int cookie)
	{
		if (cookie==0) {
			return cgcNullValueInfo;
		}
		CCDBCResultSet::pointer cdbcResultSet;
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->last() : cgcNullValueInfo;
	}
	virtual void reset(int cookie)
	{
		CCDBCResultSet::pointer cdbcResultSet;
		if (cookie!=0 && m_results.find(cookie, cdbcResultSet, true))
		{
			cdbcResultSet->reset();
		}
	}
	virtual int trans_begin(void)
	{
		if (!isopen()) return 0;
		Sink *sink = NULL;
		try
		{
			sink = sink_pool_get();
			if (sink==NULL)
				return 0;

			if (::trans_begin(sink)==0)
			{
				return (int)sink;
			}
		}catch(std::exception&)
		{
			sink_pool_put (sink);
			CGC_LOG((mycp::LOG_ERROR, "trans_begin exception\n"));
		}catch(...)
		{
			sink_pool_put (sink);
			CGC_LOG((mycp::LOG_ERROR, "trans_begin exception\n"));
		}
		return 0;
	}
	virtual bool trans_rollback(int nTransaction)
	{
		if (!isopen()) return false;
		Sink *sink = (Sink*)nTransaction;
		if (sink==NULL) return false;
		bool result = false;
		try
		{
			::trans_rollback(sink);
			result = true;
		}catch(std::exception&)
		{
			CGC_LOG((mycp::LOG_ERROR, "trans_rollback exception\n"));
		}catch(...)
		{
			CGC_LOG((mycp::LOG_ERROR, "trans_rollback exception\n"));
		}
		sink_pool_put (sink);
		return result;
	}
	virtual mycp::bigint trans_commit(int nTransaction)
	{
		if (!isopen()) return -1;
		Sink *sink = (Sink*)nTransaction;
		if (sink==NULL) return -1;
		mycp::bigint result = -1;
		try
		{
			result = ::trans_commit(sink);
		}catch(std::exception&)
		{
			CGC_LOG((mycp::LOG_ERROR, "trans_commit exception\n"));
		}catch(...)
		{
			CGC_LOG((mycp::LOG_ERROR, "trans_commit exception\n"));
		}
		sink_pool_put (sink);
		return result;
	}

	virtual bool auto_commit(bool autocommit)
	{
		if (!isopen()) return false;

		return false;
		//return mysql_autocommit(m_mysql, autocommit ? 1 : 0) == 1;
	}
	virtual bool commit(void)
	{
		if (!isopen()) return false;
		return false;
		//return mysql_commit(m_mysql) == 1;
	}
	virtual bool rollback(void)
	{
		if (!isopen()) return false;
		return false;
		//return mysql_rollback(m_mysql) == 1;
	}

private:
	int m_nIndex;
	bool m_isopen;
	time_t m_tLastTime;
	CLockMap<int, CCDBCResultSet::pointer> m_results;
	CLockMap<tstring, CCDBCResultSet::pointer> m_pMemorySqlResults;	// sql->
	cgcCDBCInfo::pointer m_cdbcInfo;
	tstring m_sDbHost;
	int m_nDbPort;
};

const int ATTRIBUTE_NAME = 1;
cgcAttributes::pointer theAppAttributes;
//CPgCdbc::pointer theBodbCdbc;
tstring theAppConfPath;

const unsigned int TIMER_CHECK_ONE_SECOND = 1;	// 1秒钟检查一次
unsigned int theSecondIndex = 0;

void CheckMemoryResourcesTimeout(void)
{
	if (theAppAttributes.get()==NULL) return;
	VoidObjectMapPointer mapLogServices = theAppAttributes->getVoidAttributes(ATTRIBUTE_NAME, false);
	if (mapLogServices.get() != NULL)
	{
		BoostReadLock rdlock(mapLogServices->mutex());
		CObjectMap<void*>::iterator iter = mapLogServices->begin();
		for (; iter!=mapLogServices->end(); iter++)
		{
			CPgCdbc::pointer service = CGC_OBJECT_CAST<CPgCdbc>(iter->second);
			service->CheckMemoryResultsTimeout();
		}
	}
}
class CTimeHandler
	: public cgcOnTimerHandler
{
public:
	typedef boost::shared_ptr<CTimeHandler> pointer;
	static CTimeHandler::pointer create(void)
	{
		return CTimeHandler::pointer(new CTimeHandler());
	}
	virtual void OnTimeout(unsigned int nIDEvent, const void * pvParam)
	{
		if (nIDEvent==TIMER_CHECK_ONE_SECOND)
		{
			theSecondIndex++;
			if ((theSecondIndex%10)==9)	// 10秒处理一次
			{
				CheckMemoryResourcesTimeout();
			}
		}
	}

	CTimeHandler(void)
	{
	}
};
CTimeHandler::pointer theTimerHandler;

// 模块初始化函数，可选实现函数
extern "C" bool CGC_API CGC_Module_Init2(MODULE_INIT_TYPE nInitType)
//extern "C" bool CGC_API CGC_Module_Init(void)
{
	theApplication2 = CGC_APPLICATION2_CAST(theApplication);
	assert (theApplication2.get() != NULL);

	theAppAttributes = theApplication->getAttributes(true);
	assert (theAppAttributes.get() != NULL);

	theTimerHandler = CTimeHandler::create();
	theApplication->SetTimer(TIMER_CHECK_ONE_SECOND, 1000, theTimerHandler);	// 1秒检查一次

	theAppConfPath = theApplication->getAppConfPath();
	return true;
}

// 模块退出函数，可选实现函数
extern "C" void CGC_API CGC_Module_Free2(MODULE_FREE_TYPE nFreeType)
//extern "C" void CGC_API CGC_Module_Free(void)
{
	VoidObjectMapPointer mapLogServices = theAppAttributes->getVoidAttributes(ATTRIBUTE_NAME, false);
	if (mapLogServices.get() != NULL)
	{
		CObjectMap<void*>::iterator iter;
		for (iter=mapLogServices->begin(); iter!=mapLogServices->end(); iter++)
		{
			CPgCdbc::pointer service = CGC_OBJECT_CAST<CPgCdbc>(iter->second);
			if (service.get() != NULL)
			{
				service->finalService();
			}
		}
	}
	theAppAttributes.reset();
	if (nFreeType==MODULE_FREE_TYPE_NORMAL)
		theApplication->KillAllTimer();
	theApplication2.reset();
	theTimerHandler.reset();
}

int theServiceIndex = 0;
extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	if (theAppAttributes.get() == NULL) return;
	CPgCdbc::pointer bodbCdbc = CPgCdbc::pointer(new CPgCdbc(theServiceIndex++));
	outService = bodbCdbc;
	theAppAttributes->setAttribute(ATTRIBUTE_NAME, outService.get(), outService);
}

extern "C" void CGC_API CGC_ResetService(const cgcServiceInterface::pointer & inService)
{
	if (inService.get() == NULL || theAppAttributes.get() == NULL) return;
	theAppAttributes->removeAttribute(ATTRIBUTE_NAME, inService.get());
	inService->finalService();
}

#endif // USES_PGCDBC
