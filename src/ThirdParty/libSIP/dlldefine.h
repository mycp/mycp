
#define libSIP_STATIC 1	// 0,1

#ifndef LIBSIP_CLASS
#if libSIP_STATIC
#define LIBSIP_CLASS 
#else
#ifdef WIN32
#ifdef libSIP_EXPORTS
#define LIBSIP_CLASS __declspec(dllexport)
#else
#define LIBSIP_CLASS __declspec(dllimport)
#endif // libSIP_EXPORTS
#else
#define LIBSIP_CLASS 
#endif // WIN32
#endif // libSIP_STATIC
#endif // LIBSIP_CLASS
