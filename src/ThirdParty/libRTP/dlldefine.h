
#define libRTP_STATIC 1	// 0,1

#ifndef LIBRTP_CLASS
#if libRTP_STATIC
#define LIBRTP_CLASS 
#else
#ifdef WIN32
#ifdef libRTP_EXPORTS
#define LIBRTP_CLASS __declspec(dllexport)
#else
#define LIBRTP_CLASS __declspec(dllimport)
#endif // libRTP_EXPORTS
#else
#define LIBRTP_CLASS 
#endif // WIN32
#endif // libRTP_STATIC
#endif // LIBRTP_CLASS
