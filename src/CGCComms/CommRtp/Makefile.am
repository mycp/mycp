lib_LTLIBRARIES = libCommRtpServer.la
THESOURCES = CommRtpServer.cpp
libCommRtpServer_la_SOURCES = $(THESOURCES)

libCommRtpServer_la_LIBADD = ../../CGCClass/libCGCClass.a ../../ThirdParty/libRTP/libRTP.a $(libdir)/libjrtp.a $(libdir)/libjthread.a

AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
