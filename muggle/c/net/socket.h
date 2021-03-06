/******************************************************************************
 *  @file         socket.h
 *  @author       Muggle Wei
 *  @email        mugglewei@gmail.com
 *  @date         2021-06-17
 *  @copyright    Copyright 2021 Muggle Wei
 *  @license      MIT License
 *  @brief        mugglec socket
 *****************************************************************************/
 
#ifndef MUGGLE_C_SOCKET_H_
#define MUGGLE_C_SOCKET_H_

#include "muggle/c/base/macro.h"

EXTERN_C_BEGIN

#if MUGGLE_PLATFORM_WINDOWS

// NOTE: if without WIN32_LEAN_AND_MEAN defined, must
// include winsock2.h before windows.h
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define MUGGLE_INVALID_SOCKET INVALID_SOCKET
#define MUGGLE_SOCKET_ERROR SOCKET_ERROR
#define MUGGLE_SOCKET_LAST_ERRNO WSAGetLastError()
#define MUGGLE_SOCKET_ADDR_STRLEN (INET6_ADDRSTRLEN + 8)

#define MUGGLE_SOCKET_SHUT_RD     SD_RECEIVE
#define MUGGLE_SOCKET_SHUT_WR     SD_SEND
#define MUGGLE_SOCKET_SHUT_RDWR   SD_BOTH

#define MUGGLE_SYS_ERRNO_INTR       WSAEINTR
#define MUGGLE_SYS_ERRNO_WOULDBLOCK WSAEWOULDBLOCK

typedef SOCKET muggle_socket_t;
typedef int muggle_socklen_t;

#else // MUGGLE_PLATFORM_WINDOWS

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/epoll.h>
#include <net/if.h>
#include <sys/ioctl.h>

#define MUGGLE_INVALID_SOCKET     (-1)
#define MUGGLE_SOCKET_ERROR       (-1)
#define MUGGLE_SOCKET_LAST_ERRNO  errno
#define MUGGLE_SOCKET_ADDR_STRLEN (INET6_ADDRSTRLEN + 8)

#define MUGGLE_SOCKET_SHUT_RD     SHUT_RD
#define MUGGLE_SOCKET_SHUT_WR     SHUT_WR
#define MUGGLE_SOCKET_SHUT_RDWR   SHUT_RDWR

#define MUGGLE_SYS_ERRNO_INTR       EINTR
#define MUGGLE_SYS_ERRNO_WOULDBLOCK EWOULDBLOCK

typedef int muggle_socket_t;
typedef socklen_t muggle_socklen_t;

#endif // MUGGLE_PLATFORM_WINDOWS

/**
 * @brief initialize socket library
 *
 * @return return 0 if success, otherwise failed
 */
MUGGLE_C_EXPORT
int muggle_socket_lib_init();

/**
 * @brief create socket
 *
 * NOTE: arguments are the same as unix socket function
 *
 * @param family    protocol family, like AF_INET, AF_INET6
 * @param type      socket type
 * @param protocol  
 *
 * @return
 *     -on success, a socket descriptor is returned 
 *     - on error, MUGGLE_INVALID_SOCKET is returned, and MUGGLE_SOCKET_LAST_ERRNO is set
 */
MUGGLE_C_EXPORT
muggle_socket_t muggle_socket_create(int family, int type, int protocol);

/**
 * @brief close socket
 *
 * @param fd  socket file descriptor
 *
 * @return 
 *     - returns 0 on success
 *     - on error, -1 is returned and MUGGLE_SOCKET_LAST_ERRNO is set
 */
MUGGLE_C_EXPORT
int muggle_socket_close(muggle_socket_t fd);

/**
 * @brief shutdown socket
 *
 * @param fd   socket file descriptor
 * @param how  MUGGLE_SOCKET_SHUT_*
 *
 * @return 
 *     - returns 0 on success
 *     - on error, -1 is returned and MUGGLE_SOCKET_LAST_ERRNO is set
 */
MUGGLE_C_EXPORT
int muggle_socket_shutdown(muggle_socket_t fd, int how);

/**
 * @brief get string describing MUGGLE_SOCKET_LAST_ERRNO
 *
 * @param errnum   get from MUGGLE_SOCKET_LAST_ERRNO
 * @param buf      buffer that stores error string
 * @param bufsize  size of buffer
 *
 * @return returns 0 on success
 */
MUGGLE_C_EXPORT
int muggle_socket_strerror(int errnum, char *buf, size_t bufsize);

/**
 * @brief set socket block or non-block
 *
 * @param socket socket file descriptor
 * @param on     if 0, set block, otherwise set non block
 *
 * @return 
 *     - returns 0 on success
 *     - on error, -1 is returned and MUGGLE_SOCKET_LAST_ERRNO is set
 */
MUGGLE_C_EXPORT
int muggle_socket_set_nonblock(muggle_socket_t socket, int on);

/**
 * @brief socket send, the same as unix send
 *
 * @param fd    socket file descriptor
 * @param buf   pointer to the data need to send
 * @param len   length of bytes in buf
 * @param flags send flag
 *
 * @return 
 *     - on success, return the number of bytes sent
 *     - on error, -1 is returned and MUGGLE_SOCKET_LAST_ERRNO is set
 */
MUGGLE_C_EXPORT
int muggle_socket_send(muggle_socket_t fd, const void *buf, size_t len, int flags);

/**
 * @brief socket sendto, the same as unix sendto
 *
 * @param fd         socket file descriptor
 * @param buf        pointer to the data need to send
 * @param len        length of bytes in buf
 * @param flags      send flag
 * @param dest_addr  dest address
 * @param addrlen    dest address length
 *
 * @return 
 *     - on success, return the number of bytes sent
 *     - on error, -1 is returned and MUGGLE_SOCKET_LAST_ERRNO is set
 */
MUGGLE_C_EXPORT
int muggle_socket_sendto(muggle_socket_t fd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, muggle_socklen_t addrlen);

/**
 * @brief socket recv, the same as recv
 *
 * @param fd     socket file descriptor
 * @param buf    buffer used to store receive bytes
 * @param len    size of buffer
 * @param flags  flags
 *
 * @return
 * return the number of bytes received, or -1 if an error occurred.
 * if error occurred, MUGGLE_SOCKET_LAST_ERRNO is set.
 */
MUGGLE_C_EXPORT
int muggle_socket_recv(muggle_socket_t fd, void *buf, size_t len, int flags);

/**
 * @brief socket recvfrom ,the same as unix recvfrom
 *
 * @param fd       socket file descriptor
 * @param buf      buffer used to store receive bytes
 * @param len      size of buffer
 * @param flags    flags
 * @param addr     store socket address
 * @param addrlen  store socket address length
 *
 * @return 
 * return the number of bytes received, or -1 if an error occurred.
 * if error occurred, MUGGLE_SOCKET_LAST_ERRNO is set.
 */
MUGGLE_C_EXPORT
int muggle_socket_recvfrom(muggle_socket_t fd, void *buf, size_t len, int flags,
	struct sockaddr *addr, muggle_socklen_t *addrlen);

EXTERN_C_END

#endif
