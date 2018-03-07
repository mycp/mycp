lib_LTLIBRARIES = libMysqlService.la
THESOURCES = MysqlService.cpp MysqlPool.cpp
libMysqlService_la_SOURCES = $(THESOURCES)

#libMysqlService_la_LIBADD = /usr/lib/mysql/libmysqlclient.a
#libMysqlService_la_LIBADD = /mysql-connector-c-6.1.6-src/libmysql/libmysqlclient.a
#AM_CFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
#AM_CXXFLAGS=-fPIC -shared -Wall -lboost_system -lboost_thread -lboost_filesystem
libMysqlService_la_LIBADD = $(libdir)/libboost_thread.a $(libdir)/libboost_filesystem.a $(libdir)/libboost_system.a \
	/mysql-connector-c-6.1.6-src/libmysql/libmysqlclient.a
AM_CFLAGS=-fPIC -shared -Wall
AM_CXXFLAGS=-fPIC -shared -Wall


# **32 OK,need libmysqlclient.so.16
#AM_CFLAGS=-fPIC -shared -Wall -L/usr/lib/mysql -lmysqlclient
#AM_CXXFLAGS=-fPIC -shared -Wall -L/usr/lib/mysql -lmysqlclient

# **64 OK,need libmysql.so.16
#AM_CFLAGS=-fPIC -shared -Wall -L/usr/local/mysql/lib -lmysqlclient
#AM_CXXFLAGS=-fPIC -shared -Wall -L/usr/local/mysql/lib -lmysqlclient

#AM_CFLAGS=-fPIC -shared -Wall -Wl,-dn -L/usr/local/mysql/lib/ -lmysqlclient -Wl,-dy -lpthread -lm -ldl -lcrypt
#AM_CXXFLAGS=-fPIC -shared -Wall -Wl,-dn -L/usr/local/mysql/lib/ -lmysqlclient -Wl,-dy -lpthread -lm -ldl -lcrypt

#AM_CFLAGS=-fPIC -shared -Wall -Wl,-Bstatic -L/usr/local/mysql/lib/ -lmysqlclient -Wl,-Bdynamic 
#AM_CXXFLAGS=-fPIC -shared -Wall -Wl,-Bstatic -L/usr/local/mysql/lib/ -lmysqlclient -Wl,-Bdynamic 
