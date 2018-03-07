#include "RtpProxy.h"
#include <time.h>

#define  RTP_SPLIT_PACKSIZE 1000
#define MILLION 1000000


CRtpProxy::CRtpProxy(void)
: m_bActive(false)
, m_rtpPort(0)
, m_pt(1)

{
	//m_TimeStamp = 0;//(LONGLONG)rand()%1000;
	//fLastPacketReceptionTime.tv_sec = 0;
	//fLastPacketReceptionTime.tv_usec = 0;
}

CRtpProxy::~CRtpProxy(void)
{
//	WSACleanup();
}

int CRtpProxy::InitComm(void)
{
#ifdef WIN32
	WSADATA dat;
	return WSAStartup(MAKEWORD(2,2),&dat);
#else
	return 0;
#endif
}

void CRtpProxy::ClearDest(void)
{
	boost::mutex::scoped_lock lock(m_mutexRtpSerssion);
	m_rtpSession.ClearDestinations();
}

int CRtpProxy::AddDest(const char* IP,uint16_t destport)
{
	unsigned long  destip = ntohl(inet_addr(IP));

	return AddDest(destip, destport);
}

int CRtpProxy::AddDest(unsigned long destip, uint16_t destport)
{
	RTPIPv4Address addr(destip,destport);

	boost::mutex::scoped_lock lock(m_mutexRtpSerssion);
	return m_rtpSession.AddDestination(addr);
}

int CRtpProxy::DelDest(unsigned long destip, uint16_t destport)
{
	RTPIPv4Address addr(destip,destport);

	boost::mutex::scoped_lock lock(m_mutexRtpSerssion);
	return m_rtpSession.DeleteDestination(addr);
}

int CRtpProxy::AddAccept(char* IP,uint16_t port)
{
	unsigned int  ip = ntohl(inet_addr(IP));
	RTPIPv4Address addr(ip,port);
	return m_rtpSession.AddToAcceptList(addr);
}

int CRtpProxy::InitSession(uint16_t rtpPort)
{
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	int status;
	//RTPSession sess1;
	sessparams.SetOwnTimestampUnit(1.0/90000.0);
	//m_pt = pt;
	//m_TimestampIncrement = TimestampIncrement;
	sessparams.SetUsePollThread(false);

	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(rtpPort);
	transparams.SetRTPSendBuffer(1024*1024);
	transparams.SetRTCPSendBuffer(0);

	//m_rtpSession.SetDefaultPayloadType(0);
	//m_rtpSession.SetDefaultMark(false);
	//m_rtpSession.SetDefaultTimestampIncrement(10);
	status = m_rtpSession.Create(sessparams,&transparams);	
	if (status == 0)
	{
		m_bActive = true;
		m_rtpPort = rtpPort;
	}
	return status;
}

void CRtpProxy::DestroySession()
{
	m_bActive = false;
	m_rtpPort = 0;
	//m_rtpSession.ReleaseBuffer();
	//fLastPacketReceptionTime.tv_sec = 0;
	//fLastPacketReceptionTime.tv_usec = 0;
	//m_rtpSession.BYEDestroy(RTPTime(10,0),0,0);

	m_rtpSession.Release();
}

//int CRtpProxy::SendData(const CData & receiveData,LONGLONG nSeqNumber)
int CRtpProxy::SendData(const unsigned char * pData, int nLen, unsigned int timestamp)
{
	if (pData == 0 || nLen <= 0) return 0;

	int datalen = nLen;
	const unsigned char * dataSend = pData;
	int pos = 0;
	
	boost::mutex::scoped_lock lock(m_mutexRtpSerssion);
	switch (m_pt)
	{
	case 98:
		{
			// H.264 Ö§³Ö NAL 1-23  FU-A 28
			int RawDataLen = RTP_SPLIT_PACKSIZE -2;
			BYTE NALHead;
			BYTE FUAHead;
			BYTE TempSendBuf[RTP_SPLIT_PACKSIZE];
			if (datalen>RawDataLen)
			{ 
				NALHead = 28;
				FUAHead = 0x8c;	
				TempSendBuf[0] = NALHead;
				TempSendBuf[1] = FUAHead;
				memcpy(TempSendBuf+2,dataSend+pos,RawDataLen);
				m_rtpSession.SendPacket(TempSendBuf,RTP_SPLIT_PACKSIZE,98,false, timestamp);
				datalen -= RawDataLen;
				pos += RawDataLen;
#ifdef WIN32
				Sleep(3);
#else
				usleep(3000);
#endif
				while (m_bActive && datalen> RawDataLen)
				{
					NALHead = 28;
					FUAHead = 0x0c;	
					TempSendBuf[0] = NALHead;
					TempSendBuf[1] = FUAHead;
					memcpy(TempSendBuf+2,dataSend+pos,RawDataLen);
					m_rtpSession.SendPacket(TempSendBuf,RTP_SPLIT_PACKSIZE,98,false,0);
					pos += RawDataLen;
					datalen -= RawDataLen;
#ifdef WIN32
					Sleep(1);
#else
					usleep(1000);
#endif
				}
				if (m_bActive && datalen != 0)
				{
					NALHead = 28;
					FUAHead = 0x4c;	
					TempSendBuf[0] = NALHead;
					TempSendBuf[1] = FUAHead;
					memcpy(TempSendBuf+2,dataSend+pos,datalen);
					m_rtpSession.SendPacket(TempSendBuf,datalen+2,98,true,0);
				}

			}else
			{	
				NALHead = 12;
				TempSendBuf[0] = NALHead;
				memcpy(TempSendBuf+1,dataSend+pos,datalen);
				m_rtpSession.SendPacket(TempSendBuf,datalen+1,98,true,timestamp);
			}	
		}break;
	default:
		{
			if (datalen>RTP_SPLIT_PACKSIZE)
			{

				//		head.m_bLastFragment = 0;
				//m_rtpSession.SendPacketEx(dataSend+pos,RTP_SPLIT_PACKSIZE,0,false,10,1,(void*)&head,sizeof(head));
				m_rtpSession.SendPacket(dataSend+pos,RTP_SPLIT_PACKSIZE,m_pt,false, timestamp);
				datalen -= RTP_SPLIT_PACKSIZE;
				pos += RTP_SPLIT_PACKSIZE;
#ifdef WIN32
				Sleep(3);
#else
				usleep(3000);
#endif
				while (m_bActive && datalen> RTP_SPLIT_PACKSIZE)
				{
					//		head.m_bLastFragment = 0;
					//		head.m_nFragmentIndex++;
					//	m_rtpSession.SendPacketEx(dataSend+pos,RTP_SPLIT_PACKSIZE,0,false,0,1,(void*)&head,sizeof(head));
					m_rtpSession.SendPacket(dataSend+pos,RTP_SPLIT_PACKSIZE,m_pt,false,0);
					pos += RTP_SPLIT_PACKSIZE;
					datalen -= RTP_SPLIT_PACKSIZE;
#ifdef WIN32
					Sleep(1);
#else
					usleep(1000);
#endif
				}
				if (m_bActive && datalen != 0)
				{
					//		head.m_bLastFragment = 1;
					//		head.m_nFragmentIndex++;
					//	m_rtpSession.SendPacketEx(dataSend+pos,datalen,0,true,0,1,(void*)&head,sizeof(head));
					m_rtpSession.SendPacket(dataSend+pos,datalen,m_pt,true,0);
				}

			}else
			{	
				//	head.m_bLastFragment = 1;
				//	m_rtpSession.SendPacketEx(dataSend,datalen,0,true,10,1,(void*)&head,sizeof(head));
				m_rtpSession.SendPacket(dataSend+pos,datalen,m_pt,true, timestamp);
			}
		}break;
	}

	return 1;
}

