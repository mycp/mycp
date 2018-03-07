lib_LTLIBRARIES = libcspApp.la
THESOURCES = cspApp.cpp
libcspApp_la_SOURCES = $(THESOURCES)

libcspApp_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
