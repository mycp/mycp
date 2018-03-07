lib_LTLIBRARIES = libParserHttp.la
THESOURCES = ParserHttp.cpp
libParserHttp_la_SOURCES = $(THESOURCES)
libParserHttp_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a \
	$(libdir)/libz.a $(libdir)/libCGCClass.a

AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
