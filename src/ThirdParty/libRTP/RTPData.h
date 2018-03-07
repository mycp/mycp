// RTPData.h file here
#ifndef __RTPData_h__
#define __RTPData_h__

#include "dlldefine.h"
#include "boost/shared_ptr.hpp"
#include "string.h"

class LIBRTP_CLASS CRTPData
{
public:
	typedef boost::shared_ptr<CRTPData> pointer;

	static CRTPData::pointer create(void)
	{
		return CRTPData::pointer(new CRTPData(0, 0));
	}
	static CRTPData::pointer create(const unsigned char * data, long size)
	{
		return CRTPData::pointer(new CRTPData(data, size));
	}
	/*static CRTPData::pointer create(const unsigned char* data, long len, struct timeval presentationTime, bool isVideo)
	{
		return CRTPData::pointer(new CRTPData(data, len, presentationTime, isVideo));
	}*/
public:
	void data(unsigned char * data, long size) {clear(); m_data = data; m_size=size;}
	unsigned char * data(long & outSize)
	{
		outSize = m_size;
		m_size = 0;
		unsigned char * result = m_data;
		m_data = NULL;
		return result;
	}
	const unsigned char * data(void) const {return m_data;}
	long size(void) const {return m_size;}
	void dest(unsigned long destip, unsigned int destport) {m_destip = destip; m_destport = destport;}
	unsigned long destip(void) const {return m_destip;}
	unsigned int destport(void) const {return m_destport;}
	void timestamp(unsigned int newv, unsigned int newv2) {m_timestamp = newv; m_timestamp2 = newv2;}
	unsigned int timestamp(void) const {return m_timestamp;}
	unsigned int timestamp2(void) const {return m_timestamp2;}
	//void timestampunit(double newv) {m_timestampunit = newv;}
	//double timestampunit(void) const {return m_timestampunit;}
	void payloadtype(unsigned short newv) {m_plyloadtype = newv;}
	unsigned short payloadtype(void) const {return m_plyloadtype;}

protected:
	void clear(void)
	{
		m_size = 0;
		if (m_data != 0)
		{
			delete[] m_data;
			m_data = 0;
		}
	}
public:
	CRTPData(const unsigned char * data, long size)
		: m_size(size)
		, m_destip(0), m_destport(0)
		, m_timestamp(0), m_timestamp2(0)
		//, m_timestampunit(1.0)
		, m_plyloadtype(1)
	{
		if (m_size > 0 && data !=0)
		{
			m_data = new unsigned char[m_size+1];
			memcpy(m_data, data, m_size);
			m_data[m_size] = '\0';
		}else
		{
			m_data = 0;
		}
	}
	/*CRTPData(const unsigned char * data, long len, struct timeval presentationTime, bool isVideo)
		: m_len(len)
		, m_presentationTime(presentationTime)
		, m_bIsVideo(isVideo)
	{
		m_data = new unsigned char[m_len+1];
		memcpy(m_data, data, len);
		m_data[len] = '\0';
	}*/
	virtual ~CRTPData(void)
	{
		clear();
	}


private:
	unsigned char * m_data;
	long m_size;
	unsigned long m_destip;
	unsigned int m_destport;
	unsigned int m_timestamp;
	unsigned int m_timestamp2;
	//double m_timestampunit;		// default 1.0
	unsigned short m_plyloadtype;	// default 1
};


#endif // __RTPData_h__
