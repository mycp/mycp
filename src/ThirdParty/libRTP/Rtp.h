// Rtp.h file here
#ifndef __Rtp_h__
#define __Rtp_h__

#include "dlldefine.h"
#include "librtphandler.h"

class LIBRTP_CLASS CRtp
{
public:
	DoRtpHandler::pointer startRtp(unsigned short rtpPort);
	void stopRtp(DoRtpHandler::pointer handler);
	void stopAll(void);

public:
	CRtp(void);
	~CRtp(void);
};

#endif // __Rtp_h__
