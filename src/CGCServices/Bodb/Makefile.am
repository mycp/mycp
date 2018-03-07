lib_LTLIBRARIES = libBodbService.la
THESOURCES = BodbService.cpp
libBodbService_la_SOURCES = $(THESOURCES)

#libBodbService_la_LIBADD = $(libdir)/libbodb.a $(libdir)/libsqlparser.a 
#AM_CFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
#AM_CXXFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
libBodbService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a \
	$(libdir)/libbodb.a $(libdir)/libsqlparser.a 
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
