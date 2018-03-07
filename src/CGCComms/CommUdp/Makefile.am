lib_LTLIBRARIES = libCommUdpServer.la
THESOURCES = CommUdpServer.cpp
libCommUdpServer_la_SOURCES = $(THESOURCES)

libCommUdpServer_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIE -shared -Wall
AM_CXXFLAGS=-fPIE -shared -Wall
