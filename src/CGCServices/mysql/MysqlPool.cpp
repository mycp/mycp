#include "MysqlPool.h"

//#if (USES_MYSQLCDBC)

CMysqlSink::CMysqlSink(void)
: m_nPort(3306)
, m_mysql(NULL)
, m_busy(false)
, m_nCloseTime(0)

{
}

CMysqlSink::~CMysqlSink(void)
{
	Disconnect();
}

bool CMysqlSink::Init(void)
{
	if (m_mysql==NULL)
		m_mysql = mysql_init((MYSQL*)NULL);
	if (m_mysql==NULL)
		printf("**** mysql_init error.\n");
	return m_mysql!=NULL?true:false;
}

bool CMysqlSink::Connect(int* pOutErrorCode)
{
	if (!Init()) return false;

	if (!mysql_real_connect(m_mysql, m_sHost.c_str(), m_sAccount.c_str(),
		m_sSecure.c_str(), m_sDatabase.c_str(), m_nPort, NULL, CLIENT_MULTI_STATEMENTS))
	{
		const unsigned int nMysqlError = mysql_errno(m_mysql);
		if (pOutErrorCode!=NULL)
			*pOutErrorCode = nMysqlError;
		printf("**** mysql_real_connect error.(%d:%s)\n",nMysqlError,mysql_error(m_mysql));
		mysql_close(m_mysql);
		m_mysql = NULL;
		return false;
	}
	// 设置支持 mysql_query 自动重连；mysql_ping 也能够自动重连数据库
	char value = 1;
	mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &value);

	if (m_sCharset.empty())
	{
		mysql_query(m_mysql, "set names utf8;");
		mysql_set_character_set(m_mysql, "utf8");
	}else
	{
		char lpszCharsetCommand[100];
		sprintf(lpszCharsetCommand, "set names %s;", m_sCharset.c_str());
		mysql_query(m_mysql, lpszCharsetCommand);
		mysql_set_character_set(m_mysql, m_sCharset.c_str());
	}
	m_nCloseTime = 0;
	return true;
}
bool CMysqlSink::Disconnect(void)
{
	const bool result = (m_mysql!=NULL)?true:false;
	if (m_mysql!=NULL)
	{
		mysql_close(m_mysql);
		m_mysql = NULL;
	}
	m_nCloseTime = time(0);
	return result;
}
bool CMysqlSink::Reconnect(int * pOutErrorCode)
{
	try
	{
		if ( Disconnect() ) {
#ifdef WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		}
		return Connect(pOutErrorCode);
	}catch(std::exception&)
	{
	}catch(...)
	{}
	return false;
}

bool CMysqlSink::IsIdleAndSetBusy(void)
{
	BoostWriteLock wtlock(m_mutex);
	if (m_busy)
		return false;
	m_busy = true;
	return true;
}
void CMysqlSink::SetIdle(void)
{
	BoostWriteLock wtlock(m_mutex);
	m_busy = false;
}


CMysqlPool::CMysqlPool(void)
: m_nSinkSize(0),m_nSinkMin(0),m_nSinkMax(0)
, m_sinks(NULL)

{
}

CMysqlPool::~CMysqlPool(void)
{
	PoolExit();
}

bool CMysqlPool::IsOpen(void) const
{
	return m_sinks==NULL?false:true;
}
int CMysqlPool::PoolInit(int nMin, int nMax,const char* lpHost,unsigned int nPort,const char* lpAccount,const char* lpSecure,const char* lpszDatabase,const char* lpCharset)
{
	BoostWriteLock wtlock(m_mutex);
	if (m_sinks!=NULL)
	{
		return m_nSinkSize;
	}
	int result = 0;
	m_nSinkSize = nMin;
	m_nSinkMin = nMin;
	m_nSinkMax = nMax;
    m_sinks = (CMysqlSink**)malloc(sizeof(CMysqlSink*) * nMax);
	for(int i = 0; i < m_nSinkMax; i++)
	{
		m_sinks[i] = new CMysqlSink();
		m_sinks[i]->m_sHost = lpHost;
		m_sinks[i]->m_nPort = nPort;
		m_sinks[i]->m_sAccount = lpAccount;
		m_sinks[i]->m_sSecure = lpSecure;
		m_sinks[i]->m_sDatabase = lpszDatabase;
		m_sinks[i]->m_sCharset = lpCharset;
		if (i<m_nSinkMin)
		{
			//m_sinks[i]->Init();
			if (m_sinks[i]->Connect())
			{
				result++;
			}
		}
	}
	return result;
}

void CMysqlPool::PoolExit(void)
{
	m_nSinkSize = 0;
	m_nSinkMin = 0;
	m_nSinkMax = 0;
	BoostWriteLock wtlock(m_mutex);
	if (m_sinks!=NULL)
	{
		for(int i = 0; i < m_nSinkMax; i++)
		{
			delete m_sinks[i];
		}
		free(m_sinks);
		m_sinks = NULL;
	}
}

CMysqlSink* CMysqlPool::SinkGet(int & pOutErrorCode)
{
	int nosinkcount = 0;
	static int theminnumbercount = 0;
	int nErrorCount = 0;
	while(true)
	{
		{
			BoostReadLock rdlock(m_mutex);
			for (int i=0; i<m_nSinkSize; i++)
			{
				CMysqlSink* pSink = m_sinks[i];
				if (!pSink->IsIdleAndSetBusy()) {
					continue;
				}

				if (!pSink->IsConnect())
				{
					if ((time(0)-pSink->GetCloseTime())>60)
					{
						// 超过一分钟，重新连接一次；
						//printf("******* Reconnect 111");
						if (!pSink->Reconnect(&pOutErrorCode))
						{
							SinkPut(pSink);
							if (IsServerError(pOutErrorCode))
								return NULL;
							continue;
						}
						// **重新连接成功，继续下面
					}else
					{
						SinkPut(pSink);
						continue;
					}
				}

				rdlock.unlock();
				if (m_nSinkSize>m_nSinkMin && i<=(m_nSinkMin-2))	// 只使用了最小值后面二条
				{
					theminnumbercount++;
					if ((i<(m_nSinkMin-2) && theminnumbercount>=30) ||	// 连接30个，只用到最小连接数，减少一个数据库连接
						theminnumbercount>=100)								// 连续100个正常数据库连接低于最低连接数，减少一个数据库连接；
					{
						if (SinkDel())
						{
							theminnumbercount = 0;
							printf("*********** SinkDel OK size:%d\n",m_nSinkSize);
						}
					}
				}else
				{
					theminnumbercount = 0;
				}
				//const int nState = mysql_ping(pSink->GetMYSQL());
				//if (nState!=0)
				//{
				//	pSink->Disconnect();
				//	pSink->Connect();
				//}
				return pSink;
			}
		}

		if (m_nSinkSize==0 || (nosinkcount++)>30)	// 全部忙，并且等25次以上（25*10=250ms左右），增加一个数据库连接，（**25这个值不能太低，低了不好；**)
		{
			nosinkcount = 0;	// 如果找不到还可以重新开始等
			CMysqlSink* pSink = SinkAdd(&pOutErrorCode);
			if (pSink != NULL)
			{
				printf("*********** SinkAdd OK size:%d\n",m_nSinkSize);
				return pSink;
			}else if (IsServerError(pOutErrorCode) || (nErrorCount++)>=3)
			{
				return NULL;
			}
		}

		//等待一段时间
#ifdef WIN32
		Sleep(10);
#else
		usleep(10000);
#endif
	}
	return NULL;
}

void CMysqlPool::SinkPut(CMysqlSink* pMysql)
{
	if (pMysql!=NULL)
	{
		pMysql->SetIdle();
	}
}

bool CMysqlPool::IsServerError(int nMysqlError)
{
	switch (nMysqlError)
	{
	case 2002:	// CR_CONNECTION_ERROR
	case 2003:	// CR_CONN_HOST_ERROR
	case 2004:	// 不能创建TCP/IP套接字(%d) 
	case 2005:	// 未知的MySQL服务器主机'%s' (%d) 
	case 2006:	// MySQL服务器不可用。
	case 2012:	// 服务器握手过程中出错
	case 2013:	// 查询过程中丢失了与MySQL服务器的连接
	case 2024:	// 连接到从服务器时出错
	case 2025:	// 连接到主服务器时出错
	case 2026:	// SSL连接错误
	case 2048:	// 无效的连接句柄
	case 1042:	// 
	case 1044:	// 
	case 1053:	// 
	case 1081:
	case 1158:
	case 1159:
	case 1160:
	case 1161:
	case 1189:	// 读取主连接时出现网络错误。
	case 1190:	// 写入主连接时出现网络错误。
	case 1218:	// 连接至主服务器%s时出错。
	case 1255:	// 从服务器已停止。
		return true;
	default:
		return false;
	}
	return false;
}

CMysqlSink* CMysqlPool::SinkAdd(int * pOutErrorCode)
{
	BoostWriteLock wtlock(m_mutex);
	if (m_nSinkSize>=m_nSinkMax)
	{
		return NULL;
	}

	CMysqlSink* pSink = m_sinks[m_nSinkSize];
	if (!pSink->IsIdleAndSetBusy())
	{
		return NULL;
	}
	//printf("******* SinkAdd Cconnect %d\n",m_nSinkSize);
	if (!pSink->Connect(pOutErrorCode))
	{
		pSink->SetIdle();
		return NULL;
	}
	m_nSinkSize++;
	return pSink;
}
bool CMysqlPool::SinkDel(void)
{
	BoostWriteLock wtlock(m_mutex);
	if (m_nSinkSize<=m_nSinkMin)
	{
		return true;
	}
	CMysqlSink* pSink = m_sinks[m_nSinkSize-1];
	if (!pSink->IsIdleAndSetBusy())
	{
		return false;
	}
	pSink->Disconnect();
	pSink->SetIdle();
	m_nSinkSize--;
	return true;
}



//#endif
