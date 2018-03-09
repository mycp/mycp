lib_LTLIBRARIES = libRestfulTest.la
THESOURCES = RestfulTest.cpp
libRestfulTest_la_SOURCES = $(THESOURCES)

libRestfulTest_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
