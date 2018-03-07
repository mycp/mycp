lib_LTLIBRARIES = libcspServlet.la
THESOURCES = cspServlet.cpp
libcspServlet_la_SOURCES = $(THESOURCES)

libcspServlet_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
