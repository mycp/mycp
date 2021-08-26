// librtphandler.h file here
#ifndef __librtphandler_h__
#define __librtphandler_h__

#include <boost/shared_ptr.hpp>
#include "dlldefine.h"
#include "RTPData.h"

class OnRtpHandler;
class LIBRTP_CLASS DoRtpHandler
{
public:
	typedef std::shared_ptr<DoRtpHandler> pointer;

	virtual void doSetRtpHandler(OnRtpHandler * handler, void * param=0) = 0;
	virtual const OnRtpHandler *  doGetRtpHandler(void) const = 0;
	virtual void doClearDest(void) = 0;
	virtual int doAddDest(const char * destip, unsigned int destport) = 0;
	virtual int doAddDest(unsigned long destip, unsigned int destport) = 0;
	virtual int doSendData(const unsigned char * pData, int nLen, unsigned int timestamp) = 0;
	virtual void doSetMediaType(unsigned short mediatype) = 0;
	virtual unsigned short doGetMediaType(void) const = 0;
	virtual int doGetRtpPort(void) const = 0;

};

const DoRtpHandler::pointer NullDoRtpHandlerPointer;

class LIBRTP_CLASS OnRtpHandler
{
public:
	virtual void onReceiveEvent(CRTPData::pointer receiveData, DoRtpHandler::pointer pDoRtpHandler, void * rtpParam) = 0;
	virtual void onRtpKilledEvent(DoRtpHandler::pointer pDoRtpHandler, void * rtpParam){}
};

#endif // __librtphandler_h__
