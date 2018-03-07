#include <limits.h>
#include "RecvRtpSession.h"
#include "rtpsourcedata.h"
#include "rtppacket.h"
#include "rtpipv4address.h"
#include "rtpipv6address.h"

CRecvRtpSession::CRecvRtpSession(void)
: m_handlerReceiver(NULL)
, m_lasttimestamp(0)
, m_paramhandler(0)
, m_killed(false), m_proc_poll(0), m_proc_data(0)

{
	m_tempBuffer = new BYTE[MAX_WAITPACK*1400]; 
	m_proc_poll = new boost::thread(boost::bind(&CRecvRtpSession::do_proc_poll, this));
	m_proc_data = new boost::thread(boost::bind(&CRecvRtpSession::do_proc_data, this));
}

CRecvRtpSession::~CRecvRtpSession(void)
{
	Release();

	delete[] m_tempBuffer;
	m_rtpdatas.clear();
}

void CRecvRtpSession::Release(void)
{
	if (!m_killed)
	{
		m_killed = true;

		if (m_proc_poll)
		{
			m_proc_poll->join();
			delete m_proc_poll;
			m_proc_poll = NULL;
		}

		if (m_proc_data)
		{
			m_proc_data->join();
			delete m_proc_data;
			m_proc_data = NULL;
		}

		BYEDestroy(RTPTime(10,0),0,0);
	}
}

void CRecvRtpSession::do_proc_poll(CRecvRtpSession * owner)
{
	BOOST_ASSERT (owner != 0);

	owner->proc_Poll();
}

void CRecvRtpSession::do_proc_data(CRecvRtpSession * owner)
{
	BOOST_ASSERT (owner != 0);

	owner->proc_Data();
}

void CRecvRtpSession::proc_Poll(void)
{
	while (!m_killed)
	{
		if (IsActive())
		{
			this->Poll();
			OnPollThreadStep();
		}

#ifdef WIN32
		Sleep(5);
#else
		usleep(5000);
#endif
	}
}

void CRecvRtpSession::proc_Data(void)
{
	while (!m_killed)
	{
		CRTPData::pointer rtpData;
		if (!m_rtpdatas.front(rtpData))
		{
#ifdef WIN32
			Sleep(10);
#else
			usleep(10000);
#endif
			continue;
		}

		if (m_handlerReceiver)
			m_handlerReceiver->onReceiveEvent(rtpData, m_pDoRtpHandler, m_paramhandler);
	}

	if (m_handlerReceiver)
		m_handlerReceiver->onRtpKilledEvent(m_pDoRtpHandler, m_paramhandler);

}

void CRecvRtpSession::OnPollThreadStep()
{
	BeginDataAccess();

	// check incoming packets
	if (GotoFirstSourceWithData())
	{
		do
		{
			RTPPacket * rtpPacket = NULL;

			RTPSourceData * curRtpSourceData = GetCurrentSourceInfo();
			u_long destIp = 0;
			u_long destPort = 0;
			if (curRtpSourceData)	
			{
				while ((rtpPacket = GetNextPacket()) != NULL)
				{
					if (destIp == 0)
					{
						const RTPAddress * rtpAddress = curRtpSourceData->GetRTPDataAddress();
						if (rtpAddress)
						{
							if (rtpAddress->GetAddressType() == RTPAddress::IPv4Address)
							{
								const RTPIPv4Address * pRtpAddress = (const RTPIPv4Address *)rtpAddress;
								destIp = pRtpAddress->GetIP();
								destPort = pRtpAddress->GetPort();
							}else if (rtpAddress->GetAddressType() == RTPAddress::IPv6Address)
							{
#ifdef RTP_SUPPORT_IPV6
								const RTPIPv6Address * pRtpAddress = (const RTPIPv6Address *)rtpAddress;
								//??? destIp = pRtpAddress->GetIP();
								destPort = pRtpAddress->GetPort();
#endif // RTP_SUPPORT_IPV6
							}
						}
					}

					ProcessRTPPacket(*rtpPacket, destIp, destPort);
					DeletePacket(rtpPacket);
				}
			}

			/*
			*/
		} while (GotoNextSourceWithData());
	}
	EndDataAccess();
}
void CRecvRtpSession::ProcessRTPPacket(const RTPPacket &rtppack, u_long destIp, u_int destPort)
{
	unsigned short lnum = rtppack.GetSequenceNumber();
	bool isEnd = rtppack.HasMarker();
	bool isStart = false;
	uint8_t payloadtype = rtppack.GetPayloadType();
	int offset = 0;
	//if (rtppack.GetPayloadType()==PT_TYPE_MPEG4)
	//if (rtppack.GetPayloadType()==PT_TYPE_H263PLUS)
	switch (payloadtype)
	{
	case PT_TYPE_MPEG4:
		{
			if(rtppack.GetPayloadLength()<=4)
				isStart = false;
			else
			{
				BYTE Mpeg4Head[4];
				memcpy(Mpeg4Head,rtppack.GetPayloadData(),4);
				if ((Mpeg4Head[0] == 0x00)&&(Mpeg4Head[1] == 0x00)&&(Mpeg4Head[2] == 0x01)&&((Mpeg4Head[3]>>4)==0x0b))
				{
					isStart = true;
				}else
				{
					isStart = false;
				}
			}
		}break;
	case 98:   ///H.264
		{
			BYTE NALHead;
			BYTE FUAHead; 
			memcpy(&NALHead,rtppack.GetPayloadData(),1);
			if ((NALHead&0x1F)<24) ///单个包
			{
				offset = 1;
				isStart = true;
				isEnd = true;
			}else if ((NALHead&0x1F)==28) ///FUA单片包
			{
				memcpy(&FUAHead,rtppack.GetPayloadData()+1,1);
				offset = 2;
				if ((FUAHead&0xE0) == 0x80)
				{
					isStart = true;
				}else if ((FUAHead&0xE0) == 0x40)
				{
					isEnd = true;
				}else
				{
					isStart = false;
				}
			}else{return;}
		}break;
	case 8:		// G711A
	default:
		{
			isStart = true;
			isEnd = true;
		}break;
	}

	if (m_FrameFamily.GetFirstSeqNumber() == 0)
	{
		m_FrameFamily.InsertFrame(lnum,isStart,isEnd,rtppack.GetPayloadData()+offset,rtppack.GetPayloadLength()-offset, rtppack.GetTimestamp());
		if (m_FrameFamily.isFull())
		{
			int len = 0;
			unsigned int timestamp = 0;
			m_FrameFamily.GetFullFrame(m_tempBuffer, len, timestamp);
			CRTPData::pointer rtpData = CRTPData::create(m_tempBuffer, len);
			rtpData->dest(destIp, destPort);
			rtpData->payloadtype(payloadtype);
			//rtpData->timestampunit(timestampUnit);
			rtpData->timestamp(m_lasttimestamp == 0 ? 0 : timestamp - m_lasttimestamp, timestamp);
			m_lasttimestamp = timestamp;

			//if (m_handlerReceiver)
			//	m_handlerReceiver->onReceiveEvent(rtpData, m_pDoRtpHandler, m_paramhandler);
			m_rtpdatas.add(rtpData);
		}
	}else if (m_FrameFamily.GetFirstSeqNumber()>lnum) ///数据包接收已经迟到
	{
		if (m_FrameFamily.GetFirstSeqNumber() - lnum > 1000)
		{
			m_FrameFamily.releaseFirstFragment();
		}
	}else   
	{
		m_FrameFamily.InsertFrame(lnum,isStart,isEnd,rtppack.GetPayloadData()+offset,rtppack.GetPayloadLength()-offset, rtppack.GetTimestamp());
		if (m_FrameFamily.isFull())
		{
			int len = 0;
			unsigned int timestamp = 0;
			m_FrameFamily.GetFullFrame(m_tempBuffer, len, timestamp);
			CRTPData::pointer rtpData = CRTPData::create(m_tempBuffer, len);
			rtpData->dest(destIp, destPort);
			rtpData->payloadtype(payloadtype);
			//rtpData->timestampunit(timestampUnit);
			rtpData->timestamp(m_lasttimestamp == 0 ? 0 : timestamp - m_lasttimestamp, timestamp);
			m_lasttimestamp = timestamp;

			//if (m_handlerReceiver)
			//	m_handlerReceiver->onReceiveEvent(rtpData, m_pDoRtpHandler, m_paramhandler);
			m_rtpdatas.add(rtpData);
		}
	}
}
