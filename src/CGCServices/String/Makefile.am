lib_LTLIBRARIES = libStringService.la
THESOURCES = StringService.cpp Base64.cpp d3des.c
libStringService_la_SOURCES = $(THESOURCES)

libStringService_la_HEADERS = cgcString.h
#AM_CFLAGS=-fPIC -shared -Wall -lssl -lcrypto -lboost_system -lboost_thread -lboost_filesystem
#AM_CXXFLAGS=-fPIC -shared -Wall -lssl -lcrypto -lboost_system -lboost_thread -lboost_filesystem
libStringService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a $(libdir)/libboost_regex.a
AM_CFLAGS=-fPIC -shared -Wall -lssl -lcrypto
AM_CXXFLAGS=-fPIC -shared -Wall -lssl -lcrypto

libStringService_ladir = $(includedir)/CGCServices/String
