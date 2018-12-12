// Crossplatform utilities for BSD sockets.

#pragma once
#include <stdbool.h>
#include "detect-platform.h"

#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <arpa/inet.h>
#elif defined(PLATFORM_WINDOWS)
	#pragma comment(lib, "wsock32.lib")
	#pragma comment(lib, "ws2_32.lib")
	#include <winsock2.h>
	#include <ws2tcpip.h>

	typedef int socklen_t;
	typedef unsigned short in_port_t;
	#include <BaseTsd.h>
	typedef SSIZE_T ssize_t;
#endif

bool cpsock_initialize();

void cpsock_shutdown();

bool cpsock_set_nonblocking(int handle);

void cpsock_close(int handle);

bool cpsock_ip_equal(
	const struct sockaddr *a, const struct sockaddr *b);

in_port_t cpsock_ip_port(const struct sockaddr *address);

enum { CPSOCK_IP_TO_STRING_LEN = INET6_ADDRSTRLEN };
const char *cpsock_ip_to_string(
	const struct sockaddr *address, char *string, size_t string_size);
