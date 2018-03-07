// CrecvBuffer.h file here
#ifndef _CrecvBuffer_h__
#define _CrecvBuffer_h__

#define MAX_BUFFER_COUNT 120
#define MAX_BUFFER_SIZE  1500
#define PT_TYPE_MPEG4 99
#define PT_TYPE_H263PLUS 98
#define PT_TYPE_H263 34
#define PT_TYPE_AAC  97

typedef unsigned char       BYTE;
//#ifdef WIN32
//typedef __int64 LONGLONG;
//#else
//typedef unsigned long LONGLONG;
//#endif

struct stuFragmentBuff
{
	unsigned short m_RtpSeqNum;
	//LONGLONG m_RtpSeqNum;
	bool isStart;  ////是否为帧起始头
	bool isEnd;    ////是否为帧结束
	BYTE m_Buffer[MAX_BUFFER_SIZE];
	int m_BufferSize;
	unsigned int m_timestamp;
};

class CrecvBuffer
{
public:
	CrecvBuffer(void);
	CrecvBuffer(int Type);
	~CrecvBuffer(void);
	//confirmStart(unsigned char* data,int datalen,int type); ///确定收到的数据包是否为帧的起始片段
	int InsertFrame(unsigned short rtpSeqNumber,bool isStart,bool isEnd,BYTE* data,int dataLen, unsigned int timestamp);
	bool isFull(void) const;

	int releaseFirstFragment();
	bool GetFullFrame(BYTE* Framedata,int& frameLen, unsigned int & timestamp);
	unsigned short GetFirstSeqNumber(void) const {return m_FirstSeqNum;}
	int GetCurrPos() {return m_CurrPos;}

	int releaseBuffer();	

private:
	stuFragmentBuff m_DataBuffer[MAX_BUFFER_COUNT];
	unsigned short m_FirstSeqNum;  ///buffer最小的seqNum值
	int m_CurrPos;           ///最小seqNum当前的数组下标值
	int m_BufferPtType;      ///buffer的pt类型
};

#endif // _CrecvBuffer_h__
