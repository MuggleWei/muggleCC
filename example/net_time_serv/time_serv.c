#include "time_serv_handle.h"

int main(int argc, char *argv[])
{
	if (muggle_log_simple_init(MUGGLE_LOG_LEVEL_INFO, MUGGLE_LOG_LEVEL_INFO) != 0)
	{
		MUGGLE_LOG_ERROR("failed initalize log");
		exit(EXIT_FAILURE);
	}

	if (muggle_socket_lib_init() != 0)
	{
		MUGGLE_LOG_ERROR("failed initalize socket library");
		exit(EXIT_FAILURE);
	}

	if (argc < 3)
	{
		MUGGLE_LOG_ERROR("usage: %s <IP> <Port> [udp-ip] [udp-port]", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *host = argv[1];
	const char *serv = argv[2];
	const char *multicast_host = NULL;
	const char *multicast_serv = NULL;

	// udp peer
	muggle_socket_peer_t udp_peer, *p_udp_peer = NULL;
	if (argc >= 5)
	{
		multicast_host = argv[3];
		multicast_serv = argv[4];
		udp_peer.fd = muggle_udp_connect(multicast_host, multicast_serv, &udp_peer);
		if (udp_peer.fd == MUGGLE_INVALID_SOCKET)
		{
			MUGGLE_LOG_ERROR("failed connect multicast target");
			exit(EXIT_FAILURE);
		}
		p_udp_peer = &udp_peer;
	}

	// listen peer
	muggle_socket_peer_t listen_peer;
	listen_peer.fd = muggle_tcp_listen(host, serv, 512, &listen_peer);
	if (listen_peer.fd == MUGGLE_INVALID_SOCKET)
	{
		MUGGLE_LOG_ERROR("failed create tcp listen for %s:%s", host, serv);
		exit(EXIT_FAILURE);
	}

	// ev datas
	int max_peer = 1024;
	struct peer_container container;
	memset(&container, 0, sizeof(container));
	container.udp_peer = p_udp_peer;
	container.peers = (muggle_socket_peer_t**)malloc(max_peer * sizeof(muggle_socket_peer_t*));
	container.max_peer = max_peer;
	container.cnt_peer = 0;

	// event init arguments
	muggle_socket_event_init_arg_t ev_init_arg;
	memset(&ev_init_arg, 0, sizeof(ev_init_arg));
	ev_init_arg.ev_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_NULL;
	ev_init_arg.hints_max_peer = max_peer;
	ev_init_arg.cnt_peer = 1;
	ev_init_arg.peers = &listen_peer;
	ev_init_arg.p_peers = NULL;
	ev_init_arg.timeout_ms = 1000;
	ev_init_arg.datas = (void*)&container;
	ev_init_arg.on_connect = on_connect;
	ev_init_arg.on_error = on_error;
	ev_init_arg.on_message = NULL;
	ev_init_arg.on_timer = on_timer;

	// event loop
	muggle_socket_event_t ev;
	if (muggle_socket_event_init(&ev_init_arg, &ev) != 0)
	{
		MUGGLE_LOG_ERROR("failed init socket event");
		exit(EXIT_FAILURE);
	}
	muggle_socket_event_loop(&ev);

	// clear
	if (p_udp_peer)
	{
		muggle_socket_close(p_udp_peer->fd);
	}
	free(container.peers);

	return 0;
}