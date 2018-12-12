// Crossplatform time functions.
// If you include <unistd.h> or time-related headers before this file, remember to define _POSIX_C_SOURCE (or something similar) first, so that clock_gettime and nanosleep are defined.

#pragma once
#include "detect-platform.h"

#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	#include <sys/time.h>
	typedef struct timespec Cptime;
#elif defined(PLATFORM_WINDOWS)
	#include <Windows.h>
	typedef LARGE_INTEGER Cptime;
#endif

Cptime cptime_time();

double cptime_elapsed(Cptime *start, Cptime *end);

void cptime_sleep(double seconds);
