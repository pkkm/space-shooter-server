// Crossplatform time functions.
// If you include <unistd.h> or time-related headers before this file, remember to define _POSIX_C_SOURCE (or something similar) first, so that clock_gettime and nanosleep are defined.

#pragma once
#include <stdbool.h>
#include "detect-platform.h"

#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	#if !defined(_POSIX_C_SOURCE)
		#define _POSIX_C_SOURCE 199309L
	#endif
	#include <sys/time.h>
	#include <time.h>
	#include <errno.h>

	typedef struct timespec Cptime;
#elif defined(PLATFORM_WINDOWS)
	#include <Windows.h>

	typedef LARGE_INTEGER Cptime;
#endif

Cptime cptime_time() {
	Cptime time;

#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	clock_gettime(CLOCK_MONOTONIC_RAW, &time);
#elif defined(PLATFORM_WINDOWS)
    QueryPerformanceCounter(&time);
#endif

    return time;
}

double cptime_elapsed(Cptime *start, Cptime *end) {
#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	return (double) end->tv_sec + (double) end->tv_nsec / 1000000000
		- (double) start->tv_sec - (double) start->tv_nsec / 1000000000;
#elif defined(PLATFORM_WINDOWS)
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (end->QuadPart - start->QuadPart) / (double) frequency.QuadPart;
#endif
}

void cptime_sleep(double seconds) {
#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	struct timespec time;
	time.tv_sec = (time_t) seconds;
	time.tv_nsec = (long) ((seconds - time.tv_sec) * 1e+9);

	int result;
	do {
		result = nanosleep(&time, &time);
	} while (result == -1 && errno == EINTR); // If interrupted by a signal, go back to sleep for the remaining time.
#elif defined(PLATFORM_WINDOWS)
	Sleep((DWORD) (seconds * 1000));
#endif
}
