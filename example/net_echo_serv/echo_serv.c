#include "echo_serv_handle.h"

int get_event_loop_type(const char *str_loop_type)
{
	// get socket event loop type
	int event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_NULL;
	if (strcmp(str_loop_type, "thread") == 0)
	{
		event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_MULTHREAD;
	}
	else if (strcmp(str_loop_type, "select") == 0)
	{
		event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_SELECT;
	}
	else if (strcmp(str_loop_type, "poll") == 0)
	{
		event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_POLL;
	}
	else if (strcmp(str_loop_type, "epoll") == 0)
	{
		event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_EPOLL;
	}
	else if (strcmp(str_loop_type, "iocp") == 0)
	{
		event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_IOCP;
	}
	else if (strcmp(str_loop_type, "kqueue") == 0)
	{
		event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_KQUEUE;
	}
	else
	{
		MUGGLE_LOG_ERROR("invalid socket event loop type: %s", str_loop_type);
		exit(EXIT_FAILURE);
	}

	return event_loop_type;
}

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
		MUGGLE_LOG_ERROR("usage: %s <IP> <Port> [thread|select|poll|epoll|iocp|kqueue]", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *host = argv[1];
	const char *serv = argv[2];

	// peers
	int cnt_peer = 2;
	muggle_socket_peer_t peers[2];

	// create tcp listen socket
	if (muggle_tcp_listen(host, serv, 512, &peers[0]) == MUGGLE_INVALID_SOCKET)
	{
		MUGGLE_LOG_ERROR("failed create tcp listen for %s:%s", host, serv);
		exit(EXIT_FAILURE);
	}

	// create udp bind socket
	if (muggle_udp_bind(host, serv, &peers[1]) == MUGGLE_INVALID_SOCKET)
	{
		MUGGLE_LOG_ERROR("failed create udp bind for %s:%s", host, serv);
		exit(EXIT_FAILURE);
	}

	// get event loop type
	int event_loop_type = MUGGLE_SOCKET_EVENT_LOOP_TYPE_NULL;
	if (argc > 3)
	{
		event_loop_type = get_event_loop_type(argv[3]);
	}

	// fill up event loop input arguments
	const char *hello = "hello echo service";
	muggle_socket_event_init_arg_t ev_init_arg;
	memset(&ev_init_arg, 0, sizeof(ev_init_arg));
	ev_init_arg.ev_loop_type = event_loop_type;
	ev_init_arg.hints_max_peer = 1024;
	ev_init_arg.cnt_peer = cnt_peer;
	ev_init_arg.peers = peers;
	ev_init_arg.p_peers = NULL;
	ev_init_arg.timeout_ms = 5000;
	ev_init_arg.datas = (void*)hello;
	ev_init_arg.on_connect = on_connect;
	ev_init_arg.on_error = on_error;
	ev_init_arg.on_close = on_close;
	ev_init_arg.on_message = on_message;
	ev_init_arg.on_timer = on_timer;

	// event loop
	muggle_socket_event_t ev;
	if (muggle_socket_event_init(&ev_init_arg, &ev) != 0)
	{
		MUGGLE_LOG_ERROR("failed init socket event");
		exit(EXIT_FAILURE);
	}
	muggle_socket_event_loop(&ev);

	return 0;
}
