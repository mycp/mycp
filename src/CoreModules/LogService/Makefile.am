lib_LTLIBRARIES = libLogService.la
THESOURCES = LogService.cpp
libLogService_la_SOURCES = $(THESOURCES)
#libLogService_la_HEADERS = cgcLog.h
libLogService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a

AM_CFLAGS=-fPIC -shared -Wall -lrt
AM_CXXFLAGS=-fPIC -shared -Wall -lrt
#libLogService_ladir = $(includedir)/CGCServices/Log
