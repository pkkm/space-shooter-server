#include "cptime.h"
#include "detect-platform.h"

#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	#include <sys/time.h>
	#include <time.h>
	#include <errno.h>
#elif defined(PLATFORM_WINDOWS)
	#include <Windows.h>
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
