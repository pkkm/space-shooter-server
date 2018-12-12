#include "cpsock.h"
#include <string.h>
#include <stdbool.h>
#include "detect-platform.h"

bool cpsock_initialize() {
#if defined(PLATFORM_WINDOWS)
	WSADATA wsa_data;
	WORD version = MAKEWORD(2, 2);
	return WSAStartup(version, &wsa_data) == NO_ERROR;
#else
	return true;
#endif
}

void cpsock_shutdown() {
#if defined(PLATFORM_WINDOWS)
	WSACleanup();
#endif
}

bool cpsock_set_nonblocking(int handle) {
#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	int non_blocking = 1;
	return fcntl(handle, F_SETFL, O_NONBLOCK, non_blocking) != -1;
#elif defined(PLATFORM_WINDOWS)
	DWORD non_blocking = 1;
	return ioctlsocket(handle, FIONBIO, &non_blocking ) == 0;
#endif
}

void cpsock_close(int handle) {
#if defined(PLATFORM_UNIX) || defined(PLATFORM_MAC)
	close(handle);
#elif defined(PLATFORM_WINDOWS)
	closesocket(handle);
#endif
}

#define CPSOCK_SIZEOF_MEMBER(type, member) \
	(sizeof(((type *) NULL)->member))

bool cpsock_ip_equal(const struct sockaddr *a,
					 const struct sockaddr *b) {
	if (a->sa_family != b->sa_family)
		return false;

	switch(a->sa_family) {
	case AF_INET: {
		struct sockaddr_in *a_in = (struct sockaddr_in *) a;
		struct sockaddr_in *b_in = (struct sockaddr_in *) b;
		return a_in->sin_port == b_in->sin_port
			&& a_in->sin_addr.s_addr == b_in->sin_addr.s_addr;
	}
	case AF_INET6: {
		struct sockaddr_in6 *a_in6 = (struct sockaddr_in6 *) a;
		struct sockaddr_in6 *b_in6 = (struct sockaddr_in6 *) b;
		return a_in6->sin6_port == b_in6->sin6_port
			&& a_in6->sin6_flowinfo == b_in6->sin6_flowinfo
			&& memcmp(a_in6->sin6_addr.s6_addr, b_in6->sin6_addr.s6_addr,
			          CPSOCK_SIZEOF_MEMBER(struct in6_addr, s6_addr)) == 0
			&& a_in6->sin6_scope_id == b_in6->sin6_scope_id;
	}
	default:
		return false;
	}
}

in_port_t cpsock_ip_port(const struct sockaddr *address) {
	// Return value: port number, 0 on failure.
	switch(address->sa_family) {
	case AF_INET:
		return ntohs(((struct sockaddr_in *) address)->sin_port);
	case AF_INET6:
		return ntohs(((struct sockaddr_in6 *) address)->sin6_port);
	default:
		return 0;
	}
}

const char *cpsock_ip_to_string(const struct sockaddr *address,
                                char *string, size_t string_size) {
	// Return value: string on success, NULL on failure.

	switch(address->sa_family) {
	case AF_INET:
		return inet_ntop(AF_INET,
		                 &((struct sockaddr_in *) address)->sin_addr,
		                 string, string_size);
		break;
	case AF_INET6:
		return inet_ntop(AF_INET6,
		                 &((struct sockaddr_in6 *) address)->sin6_addr,
		                 string, string_size);
		break;
	default:
		strncpy(string, "[Unknown AF!]", string_size);
		return NULL;
	}
}
