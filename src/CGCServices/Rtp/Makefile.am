lib_LTLIBRARIES = libRtpService.la
THESOURCES = RtpService.cpp
libRtpService_la_SOURCES = $(THESOURCES)

libRtpService_la_HEADERS = cgcRtp.h

libRtpService_la_LIBADD = $(libdir)/libRTP.a

AM_CFLAGS=-fPIC -shared -Wall -ljrtp -ljthread
AM_CXXFLAGS=-fPIC -shared -Wall -ljrtp -ljthread

libRtpService_ladir = $(includedir)/CGCServices/Rtp

