lib_LTLIBRARIES = libOpenCommunication.la
THESOURCES = OpenCommunication.cpp
libOpenCommunication_la_SOURCES = $(THESOURCES)

libOpenCommunication_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
