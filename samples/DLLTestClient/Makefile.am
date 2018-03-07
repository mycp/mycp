bin_PROGRAMS=DLLTestClient
THESOURCES = DLLTestClient.cpp

DLLTestClient_SOURCES = $(THESOURCES)

#AM_CFLAGS=-Wall -lssl -lcrypto
#AM_CXXFLAGS=-Wall -lssl -lcrypto
AM_CFLAGS=-lssl -lcrypto -lrt
AM_CXXFLAGS=-lssl -lcrypto -lrt

DLLTestClient_LDADD = $(libdir)/libCGCLib.a $(libdir)/libCGCClass.a \
	$(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a

#DLLTestClient_LDADD = $(libdir)/libCGCLib.a	$(libdir)/libCGCClass.a -lboost_thread -lboost_filesystem
