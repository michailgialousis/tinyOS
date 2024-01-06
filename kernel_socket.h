#ifndef __KERNEL_SOCKET_H
#define __KERNEL_SOCKET_H


#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_cc.h"
#include "kernel_pipe.h"
#include "util.h"



enum socket_type {
	SOCKET_LISTENER,
	SOCKET_UNBOUND,
	SOCKET_PEER
};

typedef struct listener_socket {

	rlnode queue;
	CondVar req_available;
}listener_socket;

typedef struct unbound_socket {

	rlnode unbound_socket;
}unbound_socket;

typedef struct peer_socket {

	struct peer_socket* peer;
	pipe_cb* write_pipe;
	pipe_cb* read_pipe;
}peer_socket;


typedef struct socket_control_block {

	uint refcount;
	FCB* fcb;
	enum socket_type type;
	port_t port;

	union{
		listener_socket listener_s;
		unbound_socket unbound_s;
		peer_socket peer_s;
	};



}socket_cb;

socket_cb* PORT_MAP[MAX_PORT];

struct connection_request {

	int admitted; // flag to know if the request is accepted
	socket_cb* peer;

	CondVar connected_cv;
	rlnode queue_node;
};

Fid_t sys_Socket(port_t port);

#endif