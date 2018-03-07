// libRTP.h file here
#ifndef __libRTP_h__
#define __libRTP_h__

#include "Rtp.h"

#ifdef WIN32
#ifdef _DEBUG

#if (_MSC_VER == 1200)
#pragma comment(lib, "libRTP60d.lib")
#elif (_MSC_VER == 1300)
#pragma comment(lib, "libRTP70d.lib")
#elif (_MSC_VER == 1310)
#pragma comment(lib, "libRTP71d.lib")
#elif (_MSC_VER == 1400)
#pragma comment(lib, "libRTP80d.lib")
#elif (_MSC_VER == 1500)
#pragma comment(lib, "libRTP90d.lib")
#endif

#else // _DEBUG

#if (_MSC_VER == 1200)
#pragma comment(lib, "libRTP60.lib")
#elif (_MSC_VER == 1300)
#pragma comment(lib, "libRTP70.lib")
#elif (_MSC_VER == 1310)
#pragma comment(lib, "libRTP71.lib")
#elif (_MSC_VER == 1400)
#pragma comment(lib, "libRTP80.lib")
#elif (_MSC_VER == 1500)
#pragma comment(lib, "libRTP90.lib")
#endif

#endif // _DEBUG
#endif // WIN32

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#ifdef _DEBUG
//#pragma comment(linker,"/NODEFAULTLIB:uafxcwd.lib") 
//#pragma comment(lib, "uafxcwd.lib")
#pragma comment(lib, "jthreadd.lib")
#pragma comment(lib, "jrtplibd.lib")
#else
#pragma comment(linker,"/NODEFAULTLIB:libcmt.lib") 
#pragma comment(lib, "jthread.lib")
#pragma comment(lib, "jrtplib.lib")
#endif // _DEBUG
#endif // WIN32

#endif // __libRTP_h__
