/*
    CGCP is a C++ General Communication Platform software library.
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

// CgcRemoteInfo.h file here
#ifndef __CgcRemoteInfo_h__
#define __CgcRemoteInfo_h__

#include <CGCBase/cgcCommunications.h>

//#define USES_CONNECTION_CLOSE_EVENT

namespace mycp {

class CCommEventData
{
public:
	enum CommEventType
	{
		CET_Accept	= 1
		, CET_Recv	= 2
		, CET_Close	= 3
		, CET_Exception	= 4
		//, CET_Other	= 0xff
	};

private:
	CommEventType m_cet;
	//void * m_remote;
	cgcRemote::pointer m_remote;
	unsigned long m_remoteId;
	unsigned char * m_recvData;
	unsigned int m_recvSize;
	unsigned int m_bufferSize;
	int m_nErrorCode;
	std::string m_sCodeMessage;
public:
	void setCommEventType(CommEventType newv) {m_cet = newv;}
	CommEventType getCommEventType(void) const {return m_cet;}

	//void setRemote(void * newv) {m_remote = newv;}
	//const void * getRemote(void) const {return m_remote;}
	void setRemote(const cgcRemote::pointer& newv) {m_remote = newv;}
	const cgcRemote::pointer& getRemote(void) const {return m_remote;}
	void resetRemote(void) {m_remote.reset();}

	void setRemoteId(unsigned long newv) {m_remoteId = newv;}
	unsigned long getRemoteId(void) const {return m_remoteId;}

	unsigned int getBufferSize(void) const {return m_bufferSize;}
	bool setBufferSize(unsigned int bufferSize)
	{
		if (bufferSize == 0)
			return false;
		if (m_recvData==NULL)
		{
			m_bufferSize = bufferSize;
			m_recvData = new unsigned char[m_bufferSize];
		}else if (m_bufferSize<bufferSize)
		{
			clearData(true);
			m_bufferSize = bufferSize;
			m_recvData = new unsigned char[m_bufferSize];
		}else
		{
			m_recvData[0] = '\0';
			return true;
		}
		if (m_recvData==NULL)
		{
			m_bufferSize = 0;
			return false;
		}
		m_recvData[0] = '\0';
		return true;
	}
	bool setRecvData(const unsigned char * recvData, unsigned int recvSize)
	{
		if (recvData == 0 || recvSize == 0)
			return false;
		if (!setBufferSize(recvSize+1))
			return false;
		//if (m_bufferSize<=recvSize)
		//{
		//	setBufferSize(recvSize+1);
		//}
		m_recvSize = recvSize;
		memcpy(m_recvData, recvData, m_recvSize);
		//memmove(m_recvData, recvData, m_recvSize);
		m_recvData[m_recvSize] = '\0';
		return true;
	}
	const unsigned char * getRecvData(void) const {return m_recvData;}
	unsigned int getRecvSize(void) const {return m_recvSize;}
	void SetErrorCode(int nErrorCode) {m_nErrorCode = nErrorCode;}
	int GetErrorCode(void) const {return m_nErrorCode;}
	void SetCodeMessage(const std::string& sMessage) {m_sCodeMessage=sMessage;}
	const std::string& GetCodeMessage(void) const {return m_sCodeMessage;}

	void clearData(bool bDelete)
	{
		m_recvSize = 0;
		if (m_recvData != NULL)
		{
			if (bDelete)
			{
				delete m_recvData;
				m_recvData = NULL;
				m_bufferSize = 0;
			}else
			{
				m_recvData[0] = '\0';
			}
		}
	}

public:
	CCommEventData(CommEventType commEventType,int nErrorCode=0)
		: m_cet(commEventType)
		//, m_remote(0)
		, m_remoteId(0)
		, m_recvData(0)
		, m_recvSize(0), m_bufferSize(0)
		, m_nErrorCode(nErrorCode)
#ifdef USES_CONNECTION_CLOSE_EVENT
#ifdef WIN32
		, m_hDoCloseEvent(NULL)
#else
		, m_semDoClose(NULL)
#endif
#endif
	{
	}
	virtual ~CCommEventData(void)
	{
		//clearData();
		if (m_recvData != NULL)
		{
			delete[] m_recvData;
			m_recvData = 0;
		}
#ifdef USES_CONNECTION_CLOSE_EVENT
#ifdef WIN32
		if (m_hDoCloseEvent!=NULL)
		{
			CloseHandle(m_hDoCloseEvent);
			m_hDoCloseEvent = NULL;
		}
#else
		if (m_semDoClose!=NULL)
		{
			sem_destroy(m_semDoClose);
			delete m_semDoClose;
			m_semDoClose = NULL;
		}
#endif
#endif
		m_remote.reset();
	}
#ifdef USES_CONNECTION_CLOSE_EVENT
#ifdef WIN32
	HANDLE m_hDoCloseEvent;
#else
	sem_t* m_semDoClose;
#endif // WIN32
#endif
};

class CCommEventDataPool
{
public:
	CCommEventData* Get(void)
	{
		CCommEventData * pResult = m_pPool.front();
		if (pResult==NULL)
		{
			pResult = New();
		}
		m_tIdle = 0;
		return pResult;
	}
	void Set(CCommEventData* pMsg)
	{
		if (pMsg!=NULL)
		{
			//delete pMsg;
			//return;
			if (m_pPool.size()<m_nMaxPoolSize)
			{
				pMsg->resetRemote();
				m_pPool.add(pMsg);
			}else
			{
				delete pMsg;
			}
		}
	}
	void Idle(void)
	{
		const time_t tNow = time(0);
		if (m_tIdle==0) {
			m_tIdle = tNow;
		}
		else if (tNow-m_tIdle>=3) {
			m_tIdle = tNow;
			// ??? uses while
			if (m_pPool.size()>m_nInitPoolSize) {
				CCommEventData * pEventData = m_pPool.front();
				if (pEventData!=0)
					delete pEventData;
			}
		}
	}

	void Clear(void)
	{
		m_pPool.clear();
	}
	
	CCommEventDataPool(mycp::uint32 nBufferSize, mycp::uint16 nInitPoolSize=30, mycp::uint16 nMaxPoolSize = 50)
		: m_nBufferSize(nBufferSize), m_nInitPoolSize(nInitPoolSize), m_nMaxPoolSize(nMaxPoolSize)
		, m_tIdle(0)
	{
		for (mycp::uint16 i=0;i<nInitPoolSize; i++)
		{
			m_pPool.add(New());
		}
	}
	//CCommEventDataPool(void)
	//	: m_nBufferSize(1024), m_nInitPoolSize(0), m_nMaxPoolSize(0)
	//{}
	virtual ~CCommEventDataPool(void)
	{
		m_pPool.clear();
	}

protected:
	CCommEventData* New(void) const
	{
		CCommEventData * pEventData = new CCommEventData(CCommEventData::CET_Recv);
		pEventData->setBufferSize(m_nBufferSize);
		return pEventData;
	}
private:
	CLockListPtr<CCommEventData*> m_pPool;
	mycp::uint32 m_nBufferSize;
	mycp::uint16 m_nInitPoolSize;
	mycp::uint16 m_nMaxPoolSize;
	time_t m_tIdle;
};

} // namespace mycp

#endif // __CgcRemoteInfo_h__
