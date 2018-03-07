lib_LTLIBRARIES = libFileSystemService.la
THESOURCES = FileSystemService.cpp
libFileSystemService_la_SOURCES = $(THESOURCES)
#AM_CFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
#AM_CXXFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
libFileSystemService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall
