#ifndef __KERNEL_SOCKET_H
#define __KERNEL_SOCKET_H


#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_cc.h"
#include "kernel_pipe.h"

enum socket_type {
	SOCKET_LISTENER,
	SOCKET_UNBOUND,
	SOCKET_PEER
};

struct listener_socket {

	rlnode queue;
	CondVar req_available;
};

struct unbound_socket {

	rlnode unbound_socket;
};

struct peer_socket {

	socket_cb* peer;
	pipe_cb* write_pipe;
	pipe_cb* read_pipe;
}


typedef struct socket_control_block {

	uint refcount;
	FCB* fcb;
	socket_type type;
	port_t port;

	union {
		listener_socket listener_s,
		unbound_socket unbound_s,
		peer_socket peer_s
	};



}socket_cb;


struct connection_request {

	int admitted; // flag to know if the request is accepted
	socket_cb* peer;

	CondVar connected_cv;
	rlnode queue_node;
}


#endif