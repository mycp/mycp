//#include "StdAfx.h"
#include "CrecvBuffer.h"
#include <memory.h>

CrecvBuffer::CrecvBuffer(void)
: m_FirstSeqNum(0)
, m_CurrPos(0)

{
	m_BufferPtType = 1;
	for (int i = 0 ;i<MAX_BUFFER_COUNT;i++)
	{
		m_DataBuffer[i].isStart = false;
		m_DataBuffer[i].isEnd = false;
		m_DataBuffer[i].m_RtpSeqNum = 0;
		memset(m_DataBuffer[i].m_Buffer,0,MAX_BUFFER_SIZE);
		m_DataBuffer[i].m_BufferSize = 0;
		m_DataBuffer[i].m_timestamp = 0;
	}
}
CrecvBuffer::CrecvBuffer(int Type)
: m_FirstSeqNum(0)
, m_CurrPos(0)
{

}

CrecvBuffer::~CrecvBuffer(void)
{

}
	
int CrecvBuffer::InsertFrame(unsigned short rtpSeqNumber,bool isStart,bool isEnd,BYTE* data,int dataLen, unsigned int timestamp)
{
  if (m_FirstSeqNum == 0)
  {
	  if (!isStart)
	  {
		  return -1;
	  }
	  m_FirstSeqNum = rtpSeqNumber;
  }
  while (rtpSeqNumber - m_FirstSeqNum>MAX_BUFFER_COUNT) ///rtp当前序列号差大于缓存池
  {
		releaseFirstFragment();  
  }

  int index = (int)((m_CurrPos + rtpSeqNumber - m_FirstSeqNum) % MAX_BUFFER_COUNT);

  if (m_DataBuffer[index].m_RtpSeqNum != 0)
  {
	  return -2;
  }
  m_DataBuffer[index].m_RtpSeqNum = rtpSeqNumber;
  m_DataBuffer[index].isStart = isStart;
  m_DataBuffer[index].isEnd = isEnd;
  memcpy(m_DataBuffer[index].m_Buffer,data,dataLen);
  m_DataBuffer[index].m_BufferSize = dataLen;
  m_DataBuffer[index].m_timestamp = timestamp;
  return 1;
}
int CrecvBuffer::releaseFirstFragment()
{
	for (int i=0;i<MAX_BUFFER_COUNT;i++)
	{
		int pos = (m_CurrPos+i)%MAX_BUFFER_COUNT;
		int pos2 = (m_CurrPos+i+1)%MAX_BUFFER_COUNT;
		if (m_DataBuffer[pos].isEnd)
		{
			memset(&m_DataBuffer[pos], 0, sizeof(stuFragmentBuff));
			/*m_DataBuffer[pos].m_RtpSeqNum = 0;
			m_DataBuffer[pos].isStart = false;
			m_DataBuffer[pos].isEnd = false;
			memset(m_DataBuffer[pos].m_Buffer,0,MAX_BUFFER_SIZE);
			m_DataBuffer[pos].m_BufferSize = 0;*/
			m_FirstSeqNum = m_DataBuffer[pos2].m_RtpSeqNum;
		    m_CurrPos = pos2;
			return 1;
		}else
		{
			memset(&m_DataBuffer[pos], 0, sizeof(stuFragmentBuff));
			/*m_DataBuffer[pos].m_RtpSeqNum = 0;
			m_DataBuffer[pos].isStart = false;
			m_DataBuffer[pos].isEnd = false;
			memset(m_DataBuffer[pos].m_Buffer,0,MAX_BUFFER_SIZE);
			m_DataBuffer[pos].m_BufferSize = 0;*/
		}
		if ((m_DataBuffer[pos2].m_RtpSeqNum !=0)&&(m_DataBuffer[pos2].isStart))
		{
			m_FirstSeqNum = m_DataBuffer[pos2].m_RtpSeqNum;
			m_CurrPos = pos2;
			return 2;
		}
	}
	m_FirstSeqNum = 0;
	m_CurrPos = 0;
	return 0;
}
int CrecvBuffer::releaseBuffer()
{
	for (int i=0;i<MAX_BUFFER_COUNT;i++)
	{
		memset(&m_DataBuffer[i], 0, sizeof(stuFragmentBuff));
		/*m_DataBuffer[i].m_RtpSeqNum = 0;
		m_DataBuffer[i].isStart = false;
		m_DataBuffer[i].isEnd = false;
		memset(m_DataBuffer[i].m_Buffer,0,MAX_BUFFER_SIZE);
		m_DataBuffer[i].m_BufferSize = 0;*/
	}
	m_FirstSeqNum = 0;
	m_CurrPos = 0;
	return 1;
}

bool CrecvBuffer::isFull(void) const
{
	//bool bFlag = false;
	for (int i=0;i<MAX_BUFFER_COUNT;i++)
	{
		int pos = (m_CurrPos+i)%MAX_BUFFER_COUNT;
		if ((!m_DataBuffer[pos].isEnd)&&(m_DataBuffer[pos].m_RtpSeqNum == 0))
		{
			return false;
		}
		if (m_DataBuffer[pos].isEnd)
		{
			return true;
		}
	}
	return false;
}

bool CrecvBuffer::GetFullFrame(BYTE* Framedata, int& frameLen, unsigned int & timestamp)
{
	timestamp = 0;
	int len = 0;
	for (int i=0;i<MAX_BUFFER_COUNT;i++)
	{
		int pos = (m_CurrPos+i)%MAX_BUFFER_COUNT;
		memcpy(Framedata+len,&m_DataBuffer[pos].m_Buffer,m_DataBuffer[pos].m_BufferSize);
		len += m_DataBuffer[pos].m_BufferSize;

		if (timestamp == 0)
			timestamp = m_DataBuffer[pos].m_timestamp;

		if (m_DataBuffer[pos].isEnd)
		{
			memset(&m_DataBuffer[pos], 0, sizeof(stuFragmentBuff));
			/*m_DataBuffer[pos].m_RtpSeqNum = 0;
			m_DataBuffer[pos].isStart = false;
			m_DataBuffer[pos].isEnd = false;
			memset(m_DataBuffer[pos].m_Buffer,0,MAX_BUFFER_SIZE);
			m_DataBuffer[pos].m_BufferSize = 0;*/
			m_CurrPos = (m_CurrPos+i+1)%MAX_BUFFER_COUNT;
			m_FirstSeqNum = m_DataBuffer[m_CurrPos].m_RtpSeqNum;
			break;
		}else
		{
			memset(&m_DataBuffer[pos], 0, sizeof(stuFragmentBuff));
			/*m_DataBuffer[pos].m_RtpSeqNum = 0;
			m_DataBuffer[pos].isStart = false;
			m_DataBuffer[pos].isEnd = false;
			memset(m_DataBuffer[pos].m_Buffer,0,MAX_BUFFER_SIZE);
			m_DataBuffer[pos].m_BufferSize = 0;*/
		}

	}
	frameLen = len;
	Framedata[frameLen] = '\0';
	return true;
}
