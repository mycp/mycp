lib_LTLIBRARIES = libHttpService.la
THESOURCES = HttpService.cpp CgcTcpClient.cpp
libHttpService_la_SOURCES = $(THESOURCES)

#libHttpService_la_LIBADD = $(libdir)/libz.a $(libdir)/libCGCClass.a
#AM_CFLAGS=-fPIC -shared -Wall -lssl -lcrypto -lboost_system -lboost_thread -lboost_filesystem
#AM_CXXFLAGS=-fPIC -shared -Wall -lssl -lcrypto -lboost_system -lboost_thread -lboost_filesystem
libHttpService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a \
	$(libdir)/libz.a $(libdir)/libCGCClass.a
AM_CFLAGS=-fPIC -shared -Wall -lssl -lcrypto
AM_CXXFLAGS=-fPIC -shared -Wall -lssl -lcrypto
