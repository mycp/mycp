lib_LTLIBRARIES = libCommTcpServer.la
THESOURCES = CommTcpServer.cpp
libCommTcpServer_la_SOURCES = $(THESOURCES)

libCommTcpServer_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIE -shared -Wall -lssl -lcrypto
AM_CXXFLAGS=-fPIE -shared -Wall -lssl -lcrypto
#AM_CFLAGS=-fPIC -shared -Wall -lssl -lcrypto
#AM_CXXFLAGS=-fPIC -shared -Wall -lssl -lcrypto

