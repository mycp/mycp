lib_LIBRARIES = libRTP.a

THESOURCES = \
	CrecvBuffer.cpp \
	RecvRtpSession.cpp \
	Rtp.cpp \
	RtpProxy.cpp
        
libRTP_a_SOURCES = $(THESOURCES)

libRTP_a_HEADERS = CrecvBuffer.h dlldefine.h librtphandler.h RecvRtpSession.h \
		Rtp.h RTPData.h RtpProxy.h libRTP.h

AM_CFLAGS=-fPIC -Wall
AM_CXXFLAGS=-fPIC -Wall

libRTP_adir = $(includedir)/ThirdParty/libRTP

