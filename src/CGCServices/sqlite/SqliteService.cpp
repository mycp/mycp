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

#define USES_SQLITECDBC		1		// [0,1]

#ifdef WIN32
#pragma warning(disable:4819 4267 4311)
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
#include <boost/thread/shared_mutex.hpp>

#if (USES_SQLITECDBC)

#include <sqlite3.h>
#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib,"sqlite3sd.lib")
#else
#pragma comment(lib,"sqlite3s.lib")
#endif
#endif // WIN32
mycp::cgcApplication2::pointer theApplication2;

//#define USES_STMT

class CCDBCResultSet
	//: public cgcObject
{
public:
	typedef boost::shared_ptr<CCDBCResultSet> pointer;

	CCDBCResultSet::pointer copyNew(void) const
	{
		CCDBCResultSet::pointer result = CCDBCResultSet::pointer(new CCDBCResultSet((char**)NULL,m_rows,m_fields));
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

	mycp::bigint size(void) const {return m_rows;}
	int cols(void) const {return (int)m_fields;}
	mycp::bigint index(void) const {return m_currentIndex;}
	cgcValueInfo::pointer cols_name(void) const
	{
		if (m_rows == 0 || m_fields==0) return cgcNullValueInfo;
		if (m_pColsName.get()==NULL)
		{
#ifdef USES_STMT
			if (m_resultset == NULL && m_pstmt==NULL) return cgcNullValueInfo;
#else
			if (m_resultset == NULL) return cgcNullValueInfo;
#endif
			std::vector<cgcValueInfo::pointer> record;
			try
			{
				const mycp::bigint offset = 0;
				for (int i=0 ; i<m_fields; i++ )
				{
#ifdef USES_STMT
					const char * sColumnName = m_pstmt!=NULL?sqlite3_column_name(m_pstmt,i) : m_resultset[offset+i];
					tstring s(sColumnName==NULL?"":sColumnName);
#else
					tstring s(m_resultset[offset+i]==NULL ? "" : (const char*)m_resultset[offset+i]);
#endif
					const std::string::size_type find = s.find(".");
					if (find!=std::string::npos)
						s = s.substr(find+1);
					record.push_back(CGC_VALUEINFO(s));
				}
			}catch(...)
			{
				// ...
			}
			const_cast<CCDBCResultSet*>(this)->m_pColsName = CGC_VALUEINFO(record);
		}
		return m_pColsName;
	}
	cgcValueInfo::pointer index(mycp::bigint moveIndex)
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
#ifdef USES_STMT
		if (m_pstmt!=NULL)
		{
			try
			{
				sqlite3_finalize( m_pstmt );
			}catch(...)
			{}
			m_pstmt = NULL;
		}
#endif
		if (m_resultset != NULL)
		{
			try
			{
				sqlite3_free_table( m_resultset );
			}catch(...)
			{}
			m_resultset = NULL;
		}
		//m_rows = 0;
		//m_fields = 0;
		//m_pColsName.reset();
		//m_pResults.clear();
	}

#ifdef USES_STMT
	CCDBCResultSet(sqlite3_stmt *pstmt,mycp::bigint rows, short fields)
		: m_resultset(NULL), m_pstmt(pstmt), m_rows(rows), m_fields(fields)
		, m_tMemoryTimeout(0)//, m_tCacheTime(0)
	{
		m_tCreateTime = time(0);
	}
#endif
	CCDBCResultSet(char** resultset, mycp::bigint rows, short fields)
		: m_resultset(resultset)
#ifdef USES_STMT
		, m_pstmt(NULL)
#endif
		, m_rows(rows), m_fields(fields)
		, m_tMemoryTimeout(0)//, m_tCacheTime(0)
	{
		m_tCreateTime = time(0);
	}
	virtual ~CCDBCResultSet(void)
	{
		reset();
		m_rows = 0;
		m_fields = 0;
		m_pColsName.reset();
		m_pResults.clear();
	}

protected:
	cgcValueInfo::pointer getCurrentRecord(void) const
	{
		cgcValueInfo::pointer result;
		if (!m_pResults.find(m_currentIndex,result))
		{
#ifdef USES_STMT
			if (m_resultset == NULL && m_pstmt==NULL) return cgcNullValueInfo;
#else
			if (m_resultset == NULL) return cgcNullValueInfo;
#endif
			assert (m_currentIndex >= 0 && m_currentIndex < m_rows);
			std::vector<cgcValueInfo::pointer> record;
			try
			{
#ifdef USES_STMT
				if (m_pstmt!=NULL)
				{
					for (int row=0; row<(int)m_rows; row++)
					{
						const int ret = sqlite3_step(m_pstmt);
						if ( ret != SQLITE_ROW ) break;

						std::vector<cgcValueInfo::pointer> record;
						for (int i=0; i<m_fields; i++)
						{
							const char * sColumnText = (const char*)sqlite3_column_text(m_pstmt, i);
							CGC_LOG((mycp::LOG_INFO, "**** row %d, ColumnText %d=%s\n", row,i,sColumnText));
							record.push_back(CGC_VALUEINFO((const char*)(sColumnText==NULL?"":sColumnText)));
						}
						cgcValueInfo::pointer p = CGC_VALUEINFO(record);
						result = CGC_VALUEINFO(record);
						const_cast<CCDBCResultSet*>(this)->m_pResults.insert(row,p);
						if (row==m_currentIndex)
							result = p;
					}
				}else
#endif
				{
					const mycp::bigint offset = (m_currentIndex+1)*m_fields;	// +1 because 0 is column head name
					for (int i=0 ; i<m_fields; i++ )
					{
						const tstring s( m_resultset[offset+i]==NULL ? "" : (const char*)m_resultset[offset+i]);
						record.push_back(CGC_VALUEINFO(s));
					}
				}
			}catch(...)
			{
				// ...
			}
			result = CGC_VALUEINFO(record);
			const_cast<CCDBCResultSet*>(this)->m_pResults.insert(m_currentIndex,result);
		}
		return result;
	}

private:
	char** m_resultset;
#ifdef USES_STMT
	sqlite3_stmt *m_pstmt;
#endif
	mycp::bigint	m_rows;
	short		m_fields;
	mycp::bigint	m_currentIndex;
	cgcValueInfo::pointer m_pColsName;
	CLockMap<mycp::bigint, cgcValueInfo::pointer> m_pResults;
	time_t m_tMemoryTimeout;
	time_t m_tCreateTime;
};

#define CDBC_RESULTSET(r,row,field) CCDBCResultSet::pointer(new CCDBCResultSet(r,row,field))

//const int escape_size = 1;
//const std::string escape_in[] = {"''"};
//const std::string escape_out[] = {"'"};

//const int escape_size = 9;
//const std::string escape_in[] = {"//","''","/[","]","/%","/$","/_","/(","/)"};
//const std::string escape_out[] = {"/","'","[","]","%","$","_","(",")"};

const int escape_in_size = 1;
const tstring escape_in[] = {"''"};
const tstring escape_out[] = {"'"};
//const int escape_old_out_size = 2;	// ?兼容旧版本
//const tstring escape_old_in[] = {"&lsquo;","&pge0;"};
//const tstring escape_old_out[] = {"'","\\"};

//const int escape_size = 2;
//const std::string escape_in[] = {"&lsquo;","&mse0;"};
//const std::string escape_out[] = {"'","\\"};

class CSqliteCdbc
	: public cgcCDBCService
{
public:
	typedef boost::shared_ptr<CSqliteCdbc> pointer;

	CSqliteCdbc(const tstring& path)
		: m_pSqlite(NULL), m_tLastTime(0), m_sAppConfPath(path)
	{}
	virtual ~CSqliteCdbc(void)
	{
		finalService();
	}
	virtual bool initService(cgcValueInfo::pointer parameter)
	{
		if (isServiceInited()) return true;
		m_bServiceInited = true;
		return isServiceInited();
	}
	virtual void finalService(void)
	{
		//printf("**** CSqliteCdbc::finalService()\n");
		close();
		m_cdbcInfo.reset();
		m_bServiceInited = false;
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
				{
					//CGC_LOG((mycp::LOG_INFO, "**** erase data\n"));
					m_pMemorySqlResults.erase(pIter++);
				}else
					pIter++;
			}
		}
		// 
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
	virtual tstring serviceName(void) const {return _T("SQLITECDBC");}

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
		if (cdbcInfo.get() == NULL)
		{
			printf("**** CSqliteCdbc::open() cdbcInfo is NULL\n");
			return false;
		}
		if (!isServiceInited())
		{
			printf("**** CSqliteCdbc::open() isServiceInited is FALSE\n");
			return false;
		}
		if (m_cdbcInfo.get() == cdbcInfo.get() && this->isopen()) return true;

		// close old database;
		close();
		m_cdbcInfo = cdbcInfo;
		m_tLastTime = time(0);

		try
		{
			//  rc = sqlite3_open(":memory:", &db);
			//  ATTACH DATABASE ':memory:' AS aux1;
			// 
			tstring sDatabase = m_cdbcInfo->getDatabase();
			if (sDatabase!=":memory:")
			{
				const tstring& sConnection = m_cdbcInfo->getConnection();
				if (!sConnection.empty() && sConnection.find(sDatabase)!=std::string::npos)
				{
#ifdef WIN32
					sDatabase = convert(sConnection.c_str(), CP_ACP, CP_UTF8);
#else
					sDatabase = sConnection;
#endif
				}else
				{
#ifdef WIN32
					sDatabase = convert(m_sAppConfPath.c_str(), CP_ACP, CP_UTF8);
					sDatabase.append("\\");
#else
					sDatabase = m_sAppConfPath + "/";
#endif
					sDatabase.append(m_cdbcInfo->getDatabase());
				}
			}
			//sqlite3_initialize()
			//sqlite3_config(SQLITE_CONFIG_SERIALIZED)
			const int rc = sqlite3_open(sDatabase.c_str(), &m_pSqlite);
			if ( rc!=SQLITE_OK )  
			{  
				CGC_LOG((mycp::LOG_ERROR, "Can't open database: %s(%s)\n", sDatabase.c_str(),sqlite3_errmsg(m_pSqlite)));
				sqlite3_close(m_pSqlite);
				m_pSqlite = 0;
				return false;  
			}  
		}catch(...)
		{
			return false;
		}

		return true;
	}
	virtual bool open(void) {return open(m_cdbcInfo);}
	virtual void close(void)
	{
		try
		{
			if (m_pSqlite!=0)
			{
				//printf("**** CSqliteCdbc::close\n");
				sqlite3_close(m_pSqlite);
				m_pSqlite = 0;
				m_tLastTime = time(0);
			}
		}catch(...)
		{
		}
		m_results.clear();
		m_pMemorySqlResults.clear();
	}
	virtual bool isopen(void) const
	{
		return (m_pSqlite==0)?false:true;
	}
	virtual time_t lasttime(void) const {return m_tLastTime;}

	virtual bool backup_database(const char * sBackupTo, const char* sProgram)
	{
		if (!isServiceInited() || sProgram==NULL || sBackupTo==NULL) return false;
		if (!open()) return false;

		bool ret = false;
		int rc;                   /* Function return code */
		sqlite3 *pFile = NULL;    /* Database connection opened on zFilename */
		sqlite3_backup *pBackup;  /* Backup object used to copy data */
		sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
		sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */
		/* Open the database file identified by zFilename. Exit early if this fails
		** for any reason. */
#ifdef WIN32
		const std::string sBackupToTemp = convert(sBackupTo, CP_ACP, CP_UTF8);
		rc = sqlite3_open(sBackupToTemp.c_str(), &pFile);
#else
		rc = sqlite3_open(sBackupTo, &pFile);
#endif
		if( rc==SQLITE_OK ){
			/* If this is a 'load' operation (isSave==0), then data is copied
			** from the database file just opened to database pInMemory. 
			** Otherwise, if this is a 'save' operation (isSave==1), then data
			** is copied from pInMemory to pFile.  Set the variables pFrom and
			** pTo accordingly. */
			pFrom = m_pSqlite;
			pTo   = pFile;
			/* Set up the backup procedure to copy from the "main" database of 
			** connection pFile to the main database of connection pInMemory.
			** If something goes wrong, pBackup will be set to NULL and an error
			** code and  message left in connection pTo.
			**
			** If the backup object is successfully created, call backup_step()
			** to copy data from pFile to pInMemory. Then call backup_finish()
			** to release resources associated with the pBackup object.  If an
			** error occurred, then  an error code and message will be left in
			** connection pTo. If no error occurred, then the error code belonging
			** to pTo is set to SQLITE_OK.
			*/
			//boost::mutex::scoped_lock lock(m_mutex);
			pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
			if ( pBackup ){
				(void)sqlite3_backup_step(pBackup, -1);
				(void)sqlite3_backup_finish(pBackup);
				rc = sqlite3_errcode(pTo);
				ret = rc==SQLITE_OK?true:false;
			}
		}
		/* Close the database connection opened on database file zFilename
		** and return the result of this function. */
		(void)sqlite3_close(pFile);
		return ret;
	}
	//static int sqlite_callback(
	//	void* pv,    /* 由 sqlite3_exec() 的第四个参数传递而来 */
	//	int argc,        /* 表的列数 */
	//	char** argv,    /* 指向查询结果的指针数组, 可以由 sqlite3_column_text() 得到 */
	//	char** col        /* 指向表头名的指针数组, 可以由 sqlite3_column_name() 得到 */
	//	)
	//{
	//	return 0;
	//}
	virtual mycp::bigint execute(const char * exeSql, int nTransaction)
	{
		if (exeSql == NULL || !isServiceInited()) return -1;
		if (!open()) return -1;

		//rc = sqlite3_exec(db, "BEGIN;", 0, 0, &zErrMsg); 
		////执行SQL语句 
		//rc = sqlite3_exec(db, "COMMIT;", 0, 0, &zErrMsg);

		mycp::bigint ret = 0;
		try
		{
			// * 去掉前面乱码
			const size_t nSqlLen = strlen(exeSql);
			for (size_t i=0; i<nSqlLen; i++)
			{
				if (exeSql[0]<0)
					exeSql = exeSql+1;
				else
					break;
			}
			char *zErrMsg = 0;
			boost::mutex::scoped_lock lock(m_mutex);
			int rc = sqlite3_exec( m_pSqlite , exeSql, 0, 0, &zErrMsg);
			if (rc==SQLITE_BUSY)
			{
				sqlite3_free(zErrMsg);
				rc = sqlite3_exec( m_pSqlite , exeSql, 0, 0, &zErrMsg);
			}
			if ( rc!=SQLITE_OK )
			{
				lock.unlock();
				CGC_LOG((mycp::LOG_WARNING, "Can't execute SQL: %d=%s\n", rc,zErrMsg));
				CGC_LOG((mycp::LOG_WARNING, "Can't execute SQL: (%s)\n", exeSql));
				sqlite3_free(zErrMsg);
				//if (nTransaction!=0)
				//	trans_rollback(nTransaction);
				return -1;
			}
			if (nTransaction==0)
				ret = (mycp::bigint)sqlite3_changes(m_pSqlite);
			const tstring& sSyncName = this->get_datasource();
			if (!sSyncName.empty())
				theApplication2->sendSyncData(sSyncName,0,exeSql,strlen(exeSql),false);
		}catch(...)
		{
			//if (nTransaction!=0)
			//	trans_rollback(nTransaction);
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
			//CGC_LOG((mycp::LOG_INFO, "**** %s, Get Exist Data\n", sSqlString.c_str()));
			CCDBCResultSet::pointer pSqlResultSetNew = pSqlResultSetTemp->copyNew();
			outCookie = (int)pSqlResultSetNew.get();
			m_results.insert(outCookie, pSqlResultSetNew);
			return pSqlResultSetNew->size();
		}

		mycp::bigint rows = 0;
		try
		{
			// * 去掉前面乱码
			const size_t nSqlLen = strlen(selectSql);
			for (size_t i=0; i<nSqlLen; i++)
			{
				if (selectSql[0]<0)
					selectSql = selectSql+1;
				else
					break;
			}
			//sqlite3_stmt *pstmt = 0;
			//int rettemp = sqlite3_prepare_v2(m_pSqlite, selectSql, -1, &pstmt, NULL);
			//CGC_LOG((mycp::LOG_INFO, "**** sqlite3_prepare=%d,(%s)\n", rettemp,selectSql));
			//if (rettemp==SQLITE_OK)
			//{
			//	rows = (mycp::bigint)sqlite3_data_count(pstmt);
			//	const int ncolumn = sqlite3_column_count(pstmt);
			//	CGC_LOG((mycp::LOG_INFO, "**** nrow=%lld,nColumnCount=%d\n", rows,ncolumn));

			//	//if (pstmt != NULL && rows>0 && ncolumn>0)
			//	//{
			//	//	outCookie = (int)pstmt;
			//	//	CCDBCResultSet::pointer pSqlResultSet = CDBC_RESULTSET(pstmt, rows, (short)ncolumn);
			//	//	m_results.insert(outCookie, pSqlResultSet);
			//	//	if (nCacheMinutes>0)
			//	//		//if (nCacheMinutes>0 && pSqlResultSetTemp.get()==NULL)
			//	//	{
			//	//		pSqlResultSet->BuildAllRecords();
			//	//		const time_t tNow = time(0);
			//	//		//pSqlResultSet->SetCacheTime(tNow);
			//	//		pSqlResultSet->SetMemoryTimeout(tNow+nCacheMinutes*60);
			//	//		m_pMemorySqlResults.insert(sSqlString,pSqlResultSet);
			//	//	}
			//	//}else
			//	//{
			//	//	rows = 0;
			//	//	sqlite3_finalize(pstmt);
			//	//}

			//	//for (int i=0; i<nColumnCount; i++)
			//	//{
			//	//	const char * sColumnName = sqlite3_column_name(pstmt,i);
			//	//	CGC_LOG((mycp::LOG_INFO, "**** ColumnName %d=%s\n", i,sColumnName));
			//	//}

			//	//int nRow = 0;
			//	//while (nRow<nDataCount)
			//	//{
			//	//	rettemp = sqlite3_step(pstmt);
			//	//	if( rettemp != SQLITE_ROW ) break;

			//	//	for (int i=0; i<nColumnCount; i++)
			//	//	{
			//	//		const char * sColumnText = (const char*)sqlite3_column_text(pstmt, i);
			//	//		CGC_LOG((mycp::LOG_INFO, "**** row %d, ColumnText %d=%s\n", i,sColumnText));
			//	//	}
			//	//	nRow++;
			//	//	//void * value = sqlite3_column_blob(pstmt, 1);
			//	//	//int len = sqlite3_column_bytes(pstmt,1 );
			//	//}
			//	//sqlite3_finalize(pstmt);
			////}else
			////{
			////	CGC_LOG((mycp::LOG_WARNING, "Can't select SQL: (%s); %d=%s\n", selectSql,rettemp,sqlite3_errmsg(m_pSqlite)));
			////	return -1;
			//}

			int nrow = 0, ncolumn = 0;  
			char *zErrMsg = 0;
			char **azResult = 0; //二维数组存放结果  
			//boost::mutex::scoped_lock lock(m_mutex);
			int rc = sqlite3_get_table( m_pSqlite , selectSql , &azResult , &nrow , &ncolumn , &zErrMsg);
			if (rc==SQLITE_BUSY)
			{
				sqlite3_free(zErrMsg);
				rc = sqlite3_get_table( m_pSqlite , selectSql , &azResult , &nrow , &ncolumn , &zErrMsg);
			}
			if ( rc!=SQLITE_OK )
			{
				CGC_LOG((mycp::LOG_WARNING, "Can't select SQL: (%s); %d=%s\n", selectSql,rc,zErrMsg));
				sqlite3_free(zErrMsg);
				return -1;
			}
			if (azResult != NULL && nrow>0 && ncolumn>0)
			{
				outCookie = (int)azResult;
				CCDBCResultSet::pointer pSqlResultSet = CDBC_RESULTSET(azResult, nrow, ncolumn);
				m_results.insert(outCookie, pSqlResultSet);
				rows = nrow;
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
				rows = 0;
				sqlite3_free_table( azResult );
			}
		}catch(...)
		{
			CGC_LOG((mycp::LOG_ERROR, "Select SQL exception: (%s) \n", selectSql));
			return -1;
		}
		return rows;
	}
	virtual mycp::bigint select(const char * selectSql)
	{
		if (selectSql == NULL || !isServiceInited()) return -1;
		if (!open()) return -1;
		mycp::bigint rows = 0;
		try
		{
			int nrow = 0, ncolumn = 0;  
			char *zErrMsg = 0;
			//const int rc = sqlite3_get_table( m_pSqlite , selectSql , 0, &nrow , &ncolumn , &zErrMsg );
			char **azResult = 0; //二维数组存放结果  
			//boost::mutex::scoped_lock lock(m_mutex);
			int rc = sqlite3_get_table( m_pSqlite , selectSql , &azResult , &nrow , &ncolumn , &zErrMsg );
			if (rc==SQLITE_BUSY)
			{
				sqlite3_free(zErrMsg);
				rc = sqlite3_get_table( m_pSqlite , selectSql , &azResult , &nrow , &ncolumn , &zErrMsg );
			}
			if ( rc!=SQLITE_OK )
			{
				CGC_LOG((mycp::LOG_WARNING, "Can't select SQL: (%s); %d=%s\n", selectSql,rc,zErrMsg));
				sqlite3_free(zErrMsg);
				return -1;
			}
			rows = nrow;
			sqlite3_free_table( azResult );
		}catch(...)
		{
			CGC_LOG((mycp::LOG_ERROR, "Select SQL exception: (%s) \n", selectSql));
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
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->cols() : -1;
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
		return m_results.find(cookie, cdbcResultSet) ? cdbcResultSet->index(moveIndex) : cgcNullValueInfo;
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
		if (m_results.find(cookie, cdbcResultSet, true))
		{
			cdbcResultSet->reset();
		}
	}
	// * new version
	virtual int trans_begin(void)
	{
		if (auto_commit(false))
			return 1;
		return 0;
	}
	virtual bool trans_rollback(int nTransaction)
	{
		return rollback();
	}
	virtual mycp::bigint trans_commit(int nTransaction)
	{
		if (!isopen()) return -1;
		try
		{
			char *zErrMsg = 0;
			const int rc = sqlite3_exec( m_pSqlite , "COMMIT;", 0, 0, &zErrMsg);
			if ( rc!=SQLITE_OK )
			{
				CGC_LOG((mycp::LOG_WARNING, "Can't COMMIT: %s\n", zErrMsg));
				sqlite3_free(zErrMsg);
				return -1;
			}
			return (mycp::bigint)sqlite3_changes(m_pSqlite);
		}catch(...)
		{
			return -1;
		}
		return -1;
	}

	virtual bool auto_commit(bool autocommit)
	{
		if (!isopen()) return false;
		try
		{
			char *zErrMsg = 0;
			const int rc = sqlite3_exec( m_pSqlite , "BEGIN;", 0, 0, &zErrMsg);
			if ( rc!=SQLITE_OK )
			{
				CGC_LOG((mycp::LOG_WARNING, "Can't BEGIN: %s\n", zErrMsg));
				sqlite3_free(zErrMsg);
				return false;
			}
			return true;
		}catch(...)
		{
			return false;
		}
		return false;
	}
	virtual bool commit(void)
	{
		if (!isopen()) return false;
		try
		{
			char *zErrMsg = 0;
			const int rc = sqlite3_exec( m_pSqlite , "COMMIT;", 0, 0, &zErrMsg);
			if ( rc!=SQLITE_OK )
			{
				CGC_LOG((mycp::LOG_WARNING, "Can't COMMIT: %s\n", zErrMsg));
				sqlite3_free(zErrMsg);
				return false;
			}
			return true;
		}catch(...)
		{
			return false;
		}
		return false;
	}
	virtual bool rollback(void)
	{
		if (!isopen()) return false;
		try
		{
			char *zErrMsg = 0;
			const int rc = sqlite3_exec( m_pSqlite , "ROLLBACK;", 0, 0, &zErrMsg);
			if ( rc!=SQLITE_OK )
			{
				CGC_LOG((mycp::LOG_WARNING, "Can't ROLLBACK: %s\n", zErrMsg));
				sqlite3_free(zErrMsg);
				return false;
			}
			return true;
		}catch(...)
		{
			return false;
		}
		return false;
	}
	
#ifdef WIN32
	static std::string convert(const char * strSource, int sourceCodepage, int targetCodepage)
	{
		int unicodeLen = MultiByteToWideChar(sourceCodepage, 0, strSource, -1, NULL, 0);
		if (unicodeLen <= 0) return "";

		wchar_t * pUnicode = new wchar_t[unicodeLen];
		memset(pUnicode,0,(unicodeLen)*sizeof(wchar_t));

		MultiByteToWideChar(sourceCodepage, 0, strSource, -1, (wchar_t*)pUnicode, unicodeLen);

		char * pTargetData = 0;
		int targetLen = WideCharToMultiByte(targetCodepage, 0, (wchar_t*)pUnicode, -1, (char*)pTargetData, 0, NULL, NULL);
		if (targetLen <= 0)
		{
			delete[] pUnicode;
			return "";
		}

		pTargetData = new char[targetLen];
		memset(pTargetData, 0, targetLen);

		WideCharToMultiByte(targetCodepage, 0, (wchar_t*)pUnicode, -1, (char *)pTargetData, targetLen, NULL, NULL);

		std::string result = pTargetData;
		//	tstring result(pTargetData, targetLen);
		delete[] pUnicode;
		delete[] pTargetData;
		return   result;
	}
#endif
private:
	sqlite3 * m_pSqlite;
	boost::mutex m_mutex;

	time_t m_tLastTime;
	CLockMap<int, CCDBCResultSet::pointer> m_results;			// cookie->
	CLockMap<tstring, CCDBCResultSet::pointer> m_pMemorySqlResults;	// sql->
	cgcCDBCInfo::pointer m_cdbcInfo;
	tstring m_sAppConfPath;
};

const int ATTRIBUTE_NAME = 1;
cgcAttributes::pointer theAppAttributes;
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
			CSqliteCdbc::pointer service = CGC_OBJECT_CAST<CSqliteCdbc>(iter->second);
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
	if (theAppAttributes.get() != NULL) {
		CGC_LOG((mycp::LOG_ERROR, "CGC_Module_Init2 rerun error, InitType=%d.\n", nInitType));
		return true;
	}

	theApplication2 = CGC_APPLICATION2_CAST(theApplication);
	assert (theApplication2.get() != NULL);
	theAppAttributes = theApplication->getAttributes(true);
	assert (theAppAttributes.get() != NULL);

	theTimerHandler = CTimeHandler::create();
#if (USES_TIMER_HANDLER_POINTER==1)
	theApplication->SetTimer(TIMER_CHECK_ONE_SECOND, 1000, theTimerHandler.get());	// 1秒检查一次
#else
	theApplication->SetTimer(TIMER_CHECK_ONE_SECOND, 1000, theTimerHandler);	// 1秒检查一次
#endif

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
			CSqliteCdbc::pointer service = CGC_OBJECT_CAST<CSqliteCdbc>(iter->second);
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

extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	if (theAppAttributes.get() == NULL) return;
	CSqliteCdbc::pointer bodbCdbc = CSqliteCdbc::pointer(new CSqliteCdbc(theAppConfPath));
	outService = bodbCdbc;
	theAppAttributes->setAttribute(ATTRIBUTE_NAME, outService.get(), outService);
}

extern "C" void CGC_API CGC_ResetService(const cgcServiceInterface::pointer & inService)
{
	if (inService.get() == NULL || theAppAttributes.get() == NULL) return;
	theAppAttributes->removeAttribute(ATTRIBUTE_NAME, inService.get());
	inService->finalService();
}
//
//extern "C" int CGC_API CGC_SYNC(const tstring& sSyncName, int nSyncType, const tstring& sSyncData)
//{
//
//	return 0;
//}

#endif // USES_SQLITECDBC
