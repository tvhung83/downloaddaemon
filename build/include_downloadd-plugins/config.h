#ifndef CONFIG_H
#define CONFIG_H

/* #undef DD_CONF_DIR */
/* #undef DOWNLOADDAEMON */
/* #undef DOWNLOADDAEMON_VERSION */

#define HAVE_UINT64_T
/* #undef HAVE_SYSLOG_H */
/* #undef HAVE_INITGROUPS */
/* #undef HAVE_STAT64 */
#define USE_STD_THREAD

/* #undef BACKTRACE_ON_CRASH */

#ifdef __CYGWIN__
	#ifndef RTLD_LOCAL
		#define RTLD_LOCAL 0
	#endif
#endif

#if defined(HAVE_STAT64) && !defined(__APPLE__)
        #define pstat stat64
#else
        #define pstat stat
#endif

#ifdef HAVE_UINT64_T
	#include <stdint.h>
	typedef uint64_t filesize_t;
#else
	typedef double filesize_t;
#endif

#endif // CONFIG_H
