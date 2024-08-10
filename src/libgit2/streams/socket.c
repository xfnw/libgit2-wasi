/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "streams/socket.h"

#include "posix.h"
#include "registry.h"
#include "runtime.h"
#include "stream.h"

#ifndef _WIN32
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#else
# include <winsock2.h>
# include <ws2tcpip.h>
# ifdef _MSC_VER
#  pragma comment(lib, "ws2_32")
# endif
#endif

int git_socket_stream__connect_timeout = 0;
int git_socket_stream__timeout = 0;

#ifdef GIT_WIN32
static void net_set_error(const char *str)
{
	int error = WSAGetLastError();
	char * win32_error = git_win32_get_error_message(error);

	if (win32_error) {
		git_error_set(GIT_ERROR_NET, "%s: %s", str, win32_error);
		git__free(win32_error);
	} else {
		git_error_set(GIT_ERROR_NET, "%s", str);
	}
}
#else
static void net_set_error(const char *str)
{
	git_error_set(GIT_ERROR_NET, "%s: %s", str, strerror(errno));
}
#endif

static int close_socket(GIT_SOCKET s)
{
	if (s == INVALID_SOCKET)
		return 0;

#ifdef GIT_WIN32
	if (closesocket(s) != 0) {
		net_set_error("could not close socket");
		return -1;
	}

	return 0;
#else
	return close(s);
#endif

}

static int set_nonblocking(GIT_SOCKET s)
{
#ifdef GIT_WIN32
	unsigned long nonblocking = 1;

	if (ioctlsocket(s, FIONBIO, &nonblocking) != 0) {
		net_set_error("could not set socket non-blocking");
		return -1;
	}
#else
	int flags;

	if ((flags = fcntl(s, F_GETFL, 0)) == -1) {
		net_set_error("could not query socket flags");
		return -1;
	}

	flags |= O_NONBLOCK;

	if (fcntl(s, F_SETFL, flags) != 0) {
		net_set_error("could not set socket non-blocking");
		return -1;
	}
#endif

	return 0;
}

/* Promote a sockerr to an errno for our error handling routines */
static int handle_sockerr(GIT_SOCKET socket)
{
	return -1;
}

GIT_INLINE(bool) connect_would_block(int error)
{
#ifdef GIT_WIN32
	if (error == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
		return true;
#endif

	if (error == -1 && errno == EINPROGRESS)
		return true;

	return false;
}

static int connect_with_timeout(
	GIT_SOCKET socket,
	const struct sockaddr *address,
	socklen_t address_len,
	int timeout)
{
	return -1;
}

static int socket_connect(git_stream *stream)
{
	return -1;
}

static ssize_t socket_write(
	git_stream *stream,
	const char *data,
	size_t len,
	int flags)
{
	return -1;
}

static int socket_close(git_stream *stream)
{
	return -1;
}

static void socket_free(git_stream *stream)
{
}

static int default_socket_stream_new(
	git_stream **out,
	const char *host,
	const char *port)
{
	return -1;
}

int git_socket_stream_new(
	git_stream **out,
	const char *host,
	const char *port)
{
	return -1;
}

#ifdef GIT_WIN32

static void socket_stream_global_shutdown(void)
{
	WSACleanup();
}

int git_socket_stream_global_init(void)
{
	WORD winsock_version;
	WSADATA wsa_data;

	winsock_version = MAKEWORD(2, 2);

	if (WSAStartup(winsock_version, &wsa_data) != 0) {
		git_error_set(GIT_ERROR_OS, "could not initialize Windows Socket Library");
		return -1;
	}

	if (LOBYTE(wsa_data.wVersion) != 2 ||
	    HIBYTE(wsa_data.wVersion) != 2) {
		git_error_set(GIT_ERROR_SSL, "Windows Socket Library does not support Winsock 2.2");
		return -1;
	}

	return git_runtime_shutdown_register(socket_stream_global_shutdown);
}

#else

#include "stream.h"

int git_socket_stream_global_init(void)
{
	return 0;
}

 #endif
