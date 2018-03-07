lib_LTLIBRARIES = libSqliteService.la
THESOURCES = SqliteService.cpp
libSqliteService_la_SOURCES = $(THESOURCES)

#libSqliteService_la_LIBADD = /usr/local/lib/libsqlite3.a
#AM_CFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
#AM_CXXFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
libSqliteService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a \
	/usr/local/lib/libsqlite3.a
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
