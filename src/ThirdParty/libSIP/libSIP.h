// libSIP.h file here
#ifndef __libSIP_h__
#define __libSIP_h__

#include "Sip.h"

#ifdef WIN32
#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

#ifdef _DEBUG
#pragma comment(lib, "osipparser2d.lib")
#pragma comment(lib, "osip2d.lib")
#pragma comment(lib, "eXosipd.lib")
#pragma comment(lib, "libSIPd.lib")
#else
#pragma comment(lib, "osipparser2.lib")
#pragma comment(lib, "osip2.lib")
#pragma comment(lib, "eXosip.lib")
#pragma comment(lib, "libSIP.lib")
#endif // _DEBUG
#endif // WIN32

#endif // __libSIP_h__
