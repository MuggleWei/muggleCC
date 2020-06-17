#include "socket_peer.h"
#include "muggle/c/log/log.h"

int muggle_socket_peer_retain(muggle_socket_peer_t *peer)
{
	muggle_atomic_int ref_cnt = 0, desired = 0;
	do {
		MUGGLE_ASSERT(peer->ref_cnt >= 0);

		ref_cnt = peer->ref_cnt;
		if (ref_cnt == 0)
		{
			return ref_cnt;
		}
		desired = ref_cnt + 1;
	} while (!muggle_atomic_cmp_exch_weak(&peer->ref_cnt, &ref_cnt, desired, muggle_memory_order_relaxed));

	return desired;
}

int muggle_socket_peer_release(muggle_socket_peer_t *peer)
{
	muggle_atomic_int ref_cnt = 0, desired = 0;
	do {
		MUGGLE_ASSERT(peer->ref_cnt >= 0);

		ref_cnt = peer->ref_cnt;
		if (ref_cnt == 0)
		{
			return ref_cnt;
		}
		desired = ref_cnt - 1;
	} while (!muggle_atomic_cmp_exch_weak(&peer->ref_cnt, &ref_cnt, desired, muggle_memory_order_relaxed));

	if (desired == 0)
	{
		muggle_socket_close(peer->fd);
	}

	return desired;
}

int muggle_socket_peer_close(muggle_socket_peer_t *peer)
{
	return muggle_socket_peer_release(peer);
}

int muggle_socket_peer_recvfrom(
	muggle_socket_peer_t *peer, void *buf, size_t len, int flags,
	struct sockaddr *addr, muggle_socklen_t *addrlen)
{
	int n = 0;
	while (1)
	{
#if MUGGLE_PLATFORM_WINDOWS
		n = recvfrom(peer->fd, buf, (int)len, flags, addr, addrlen);
#else
		n = recvfrom(peer->fd, buf, len, flags, addr, addrlen);
#endif
		if (n > 0)
		{
			break;
		}
		else
		{
			if (n < 0)
			{
				if (MUGGLE_SOCKET_LAST_ERRNO == MUGGLE_SYS_ERRNO_INTR)
				{
					continue;
				}
				else if (MUGGLE_SOCKET_LAST_ERRNO == MUGGLE_SYS_ERRNO_WOULDBLOCK)
				{
					break;
				}
			}
			muggle_socket_peer_close(peer);
			break;
		}
	}

	return n;
}

int muggle_socket_peer_recv(muggle_socket_peer_t *peer, void *buf, size_t len, int flags)
{
	int n = 0;
	while (1)
	{
#if MUGGLE_PLATFORM_WINDOWS
		n = recv(peer->fd, buf, (int)len, flags);
#else
		n = recv(peer->fd, buf, len, flags);
#endif
		if (n > 0)
		{
			break;
		}
		else
		{
			if (n < 0)
			{
				if (MUGGLE_SOCKET_LAST_ERRNO == MUGGLE_SYS_ERRNO_INTR)
				{
					continue;
				}
				else if (MUGGLE_SOCKET_LAST_ERRNO == MUGGLE_SYS_ERRNO_WOULDBLOCK)
				{
					break;
				}
			}
			muggle_socket_peer_close(peer);
			break;
		}
	}

	return n;
}

int muggle_socket_peer_sendto(muggle_socket_peer_t *peer, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
#if MUGGLE_PLATFORM_WINDOWS
	int num_bytes = sendto(peer->fd, buf, (int)len, flags, dest_addr, addrlen);
#else
	int num_bytes = sendto(peer->fd, buf, len, flags, dest_addr, addrlen);
#endif
	if (num_bytes != len)
	{
		if (num_bytes == MUGGLE_SOCKET_ERROR)
		{
			char err_msg[1024] = { 0 };
			muggle_socket_strerror(MUGGLE_SOCKET_LAST_ERRNO, err_msg, sizeof(err_msg));
			MUGGLE_LOG_TRACE("failed send msg - %s", err_msg);
		}
		else
		{
			MUGGLE_LOG_TRACE("send buffer full");
		}

		muggle_socket_peer_close(peer);
	}
	return num_bytes;
}

int muggle_socket_peer_send(muggle_socket_peer_t *peer, const void *buf, size_t len, int flags)
{
#if MUGGLE_PLATFORM_WINDOWS
	int num_bytes = send(peer->fd, buf, (int)len, flags);
#else
	int num_bytes = send(peer->fd, buf, len, flags);
#endif
	if (num_bytes != len)
	{
		if (num_bytes == MUGGLE_SOCKET_ERROR)
		{
			char err_msg[1024] = { 0 };
			muggle_socket_strerror(MUGGLE_SOCKET_LAST_ERRNO, err_msg, sizeof(err_msg));
			MUGGLE_LOG_TRACE("failed send msg - %s", err_msg);
		}
		else
		{
			MUGGLE_LOG_TRACE("send buffer full");
		}

		muggle_socket_peer_close(peer);
	}
	return num_bytes;
}