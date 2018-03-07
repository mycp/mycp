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
	// ����֧�� mysql_query �Զ�������mysql_ping Ҳ�ܹ��Զ��������ݿ�
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
						// ����һ���ӣ���������һ�Σ�
						//printf("******* Reconnect 111");
						if (!pSink->Reconnect(&pOutErrorCode))
						{
							SinkPut(pSink);
							if (IsServerError(pOutErrorCode))
								return NULL;
							continue;
						}
						// **�������ӳɹ�����������
					}else
					{
						SinkPut(pSink);
						continue;
					}
				}

				rdlock.unlock();
				if (m_nSinkSize>m_nSinkMin && i<=(m_nSinkMin-2))	// ֻʹ������Сֵ�������
				{
					theminnumbercount++;
					if ((i<(m_nSinkMin-2) && theminnumbercount>=30) ||	// ����30����ֻ�õ���С������������һ�����ݿ�����
						theminnumbercount>=100)								// ����100���������ݿ����ӵ������������������һ�����ݿ����ӣ�
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

		if (m_nSinkSize==0 || (nosinkcount++)>30)	// ȫ��æ�����ҵ�25�����ϣ�25*10=250ms���ң�������һ�����ݿ����ӣ���**25���ֵ����̫�ͣ����˲��ã�**)
		{
			nosinkcount = 0;	// ����Ҳ������������¿�ʼ��
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

		//�ȴ�һ��ʱ��
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
	case 2004:	// ���ܴ���TCP/IP�׽���(%d) 
	case 2005:	// δ֪��MySQL����������'%s' (%d) 
	case 2006:	// MySQL�����������á�
	case 2012:	// ���������ֹ����г���
	case 2013:	// ��ѯ�����ж�ʧ����MySQL������������
	case 2024:	// ���ӵ��ӷ�����ʱ����
	case 2025:	// ���ӵ���������ʱ����
	case 2026:	// SSL���Ӵ���
	case 2048:	// ��Ч�����Ӿ��
	case 1042:	// 
	case 1044:	// 
	case 1053:	// 
	case 1081:
	case 1158:
	case 1159:
	case 1160:
	case 1161:
	case 1189:	// ��ȡ������ʱ�����������
	case 1190:	// д��������ʱ�����������
	case 1218:	// ��������������%sʱ����
	case 1255:	// �ӷ�������ֹͣ��
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
