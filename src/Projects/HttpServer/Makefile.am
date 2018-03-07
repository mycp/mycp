lib_LTLIBRARIES = libHttpServer.la
THESOURCES = HttpServer.cpp FileScripts.cpp ScriptItem.cpp MycpHttpServer.cpp
#THESOURCES = CgcTcpClient.cpp HttpServer.cpp FileScripts.cpp ScriptItem.cpp MycpHttpServer.cpp
libHttpServer_la_SOURCES = $(THESOURCES)

libHttpServer_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a
AM_CFLAGS=-fPIC -shared -Wall -lssl -lcrypto
AM_CXXFLAGS=-fPIC -shared -Wall -lssl -lcrypto
#AM_CFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem -lssl -lcrypto
#AM_CXXFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem -lssl -lcrypto
