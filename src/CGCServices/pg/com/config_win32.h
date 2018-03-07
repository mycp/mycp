/*
 *  config-win32.h
 *
 *  Windows specific definitions and includes.
 *  
 *  $Revision: 1.1 $
 *  $Date: 2005/08/28 11:01:54 $
 *
 * Copyright (c) 1995-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef WIN32
#ifndef __CONFIG_WIN32_H__
#define __CONFIG_WIN32_H__

#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>   /* abs() */
#include <string.h>
#include <winsock2.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_IPv6
#ifdef MUSICA_IPV6
#include <winsock6.h>
#else
#ifdef WINXP_IPV6
#include <ws2tcpip.h>
#else
#ifdef WIN2K_IPV6
#include <ws2tcpip.h>
#include <tpipv6.h>
#else
#include <ws2ip6.h>
#include <ws2tcpip.h>
#endif
#endif
#endif
#endif

#ifndef MUSICA_IPV6
#include <ws2tcpip.h>
#endif

//#include <mmreg.h>
//#include <msacm.h>
//#include <mmsystem.h>
//#include <windows.h>
//#include <io.h>
//#include <process.h>
//#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>


/* Just a hack 'til we fix it properly */
typedef int		        ttl_t;
typedef unsigned int	fd_t;
/*
* the definitions below are valid for 32-bit architectures and will have to
* be adjusted for 16- or 64-bit architectures
*/
typedef unsigned __int8		uint8_t;
typedef unsigned __int16	uint16_t;
typedef unsigned __int32	uint32_t;
typedef __int8		        int8_t;
typedef __int16		        int16_t;
typedef __int32		        int32_t;
typedef unsigned long	    in_addr_t;


#ifndef TRUE
#define FALSE	0
#define	TRUE	1
#endif /* TRUE */

#define USERNAMELEN	8

#define WORDS_SMALLENDIAN
#define NEED_INET_ATON
#define NEED_DRAND48
#define NEED_GETTIMEOFDAY

#ifdef NDEBUG
#define assert(x) if ((x) == 0) fprintf(stderr, "%s:%u: failed assertion\n", __FILE__, __LINE__)
#else
#include <assert.h>
#endif

#include <time.h>		/* For clock_t */

#define inline
#define __inline     

#define AUDIO_MICROPHONE	1
#define AUDIO_LINE_IN		2
#define AUDIO_CD            4
#define AUDIO_SPEAKER		0
#define AUDIO_HEADPHONE		1
#define AUDIO_LINE_OUT		4

#define srand48	srand
#define lrand48 rand() * rand
#define snprintf _snprintf

#define IN_CLASSD(i)	(((long)(i) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(i)	IN_CLASSD(i)

typedef char	*caddr_t;
typedef long	ssize_t;

/*
typedef struct iovec {
	caddr_t	iov_base;
	ssize_t	iov_len;
} iovec_t;

struct msghdr {
	caddr_t		msg_name;
	int		msg_namelen;
	struct iovec	*msg_iov;
	int		msg_iovlen;
	caddr_t		msg_accrights;
	int		msg_accrightslen;
};
*/

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256
#endif

/*
#define _SYS_NMLN	9
struct utsname {
	char sysname[_SYS_NMLN];
	char nodename[_SYS_NMLN];
	char release[_SYS_NMLN];
	char version[_SYS_NMLN];
	char machine[_SYS_NMLN];
};
*/

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

/*
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
*/
    
#if defined(__cplusplus)
extern "C" {
#endif

int uname(struct utsname *);
int getopt(int, char * const *, const char *);
int srandom(int);
int random(void);
int gethostid(void);
int getuid(void);
int getgid(void);
#define getpid _getpid
int nice(int);
int usleep(unsigned int);


const char * w32_make_version_info(char * rat_verion);

#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

//int  RegGetValue(HKEY *, char *, char*, char*, int);
void ShowMessage(int level, char *msg);

#define bcopy(from,to,len) memcpy(to,from,len)

#if defined(__cplusplus)
}
#endif

#define ECONNREFUSED	WSAECONNREFUSED
#define ENETUNREACH		WSAENETUNREACH
#define EHOSTUNREACH	WSAEHOSTUNREACH
#define EWOULDBLOCK		WSAEWOULDBLOCK

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT	WSAEAFNOSUPPORT
#endif

#define M_PI		3.14159265358979323846

#endif 
#endif /*__CONFIG_WIN32_H__ */



