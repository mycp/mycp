lib_LTLIBRARIES = libSotpClientService.la
THESOURCES = SotpClientService.cpp
libSotpClientService_la_SOURCES = $(THESOURCES)
libSotpClientService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a \
	../../CGCLib/libCGCLib.a ../../CGCClass/libCGCClass.a

AM_CFLAGS=-fPIC -shared -Wall -lssl -lcrypto -lrt
AM_CXXFLAGS=-fPIC -shared -Wall -lssl -lcrypto -lrt
