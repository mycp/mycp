lib_LTLIBRARIES = libParserSotp.la
THESOURCES = ParserSotp.cpp
libParserSotp_la_SOURCES = $(THESOURCES)
libParserSotp_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a \
	../../CGCClass/libCGCClass.a

AM_CFLAGS=-fPIC -shared -Wall -lrt
AM_CXXFLAGS=-fPIC -shared -Wall -lrt
