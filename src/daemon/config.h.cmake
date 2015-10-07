#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine DD_CONF_DIR @DD_CONF_DIR_OPT@
#cmakedefine DOWNLOADDAEMON
#cmakedefine DOWNLOADDAEMON_VERSION @VERSION@

#cmakedefine HAVE_UINT64_T
#cmakedefine HAVE_SYSLOG_H
#cmakedefine HAVE_INITGROUPS
#cmakedefine HAVE_STAT64
#cmakedefine USE_STD_THREAD

#cmakedefine BACKTRACE_ON_CRASH

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
