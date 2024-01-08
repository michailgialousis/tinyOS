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

	struct socket_control_block* peer;
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

typedef struct connection_request {

	int admitted; // flag to know if the request is accepted
	socket_cb* peer;

	CondVar connected_cv;
	rlnode queue_node;
}connection_req;


Fid_t sys_Socket(port_t port);

void* socket_open (uint minor);
int socket_read (void* this, char *buf, unsigned int size);
int socket_write (void* this, const char* buf, unsigned int size);
int socket_close (void* this);

int sys_Listen(Fid_t sock);
Fid_t sys_Accept(Fid_t lsock);
int sys_Connect(Fid_t sock, port_t port, timeout_t timeout);
int sys_ShutDown(Fid_t sock, shutdown_mode how);

#endif