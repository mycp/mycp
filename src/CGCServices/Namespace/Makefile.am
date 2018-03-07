lib_LTLIBRARIES = libNamespaceService.la
THESOURCES = NamespaceService.cpp
libNamespaceService_la_SOURCES = $(THESOURCES)

libNamespaceService_la_HEADERS = cgcNamespace.h

libNamespaceService_la_LIBADD = $(libdir)/libbodb.a $(libdir)/libsqlparser.a

AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall

libNamespaceService_ladir = $(includedir)/CGCServices/Namespace
