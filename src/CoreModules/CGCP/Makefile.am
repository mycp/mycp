bin_PROGRAMS = CGCP

THESOURCES = \
	CGCP.cpp \
	CGC/TimerInfo.cpp \
	CGC/SessionMgr.cpp \
	CGC/SotpResponseImpl.cpp \
	CGC/HttpResponseImpl.cpp \
	CGC/ModuleMgr.cpp \
	CGC/CGCApp.cpp

CGCP_SOURCES = $(THESOURCES)
AM_CFLAGS = -ldl -lssl -lcrypto -lrt
AM_CXXFLAGS = -ldl -lssl -lcrypto -lrt
CGCP_LDADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a $(libdir)/libCGCClass.a

#AM_CFLAGS = -lboost_system -lboost_thread -lboost_filesystem -ldl -lssl -lcrypto
#AM_CXXFLAGS = -lboost_system -lboost_thread -lboost_filesystem -ldl -lssl -lcrypto
#CGCP_LDADD = $(libdir)/libCGCLib.a $(libdir)/libCGCClass.a
CGCP_adir = /usr/include
