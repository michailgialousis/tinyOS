


#include "kernel_socket.h"

void* socket_open (uint minor){
	return NULL;
}

int socket_read (void* this, char *buf, unsigned int size){

	socket_cb* scb = (socket_cb*) this;

	if(scb==NULL || scb->type != SOCKET_PEER || scb->peer_s.read_pipe == NULL)
		return -1;

	return pipe_read(scb->peer_s.read_pipe, buf, size);
}

 int socket_write (void* this, const char* buf, unsigned int size){

 	socket_cb* scb = (socket_cb*) this;

 	if(scb==NULL || scb->type != SOCKET_PEER ||scb->peer_s.write_pipe == NULL)
 		return -1;

 	return pipe_write(scb->peer_s.write_pipe, buf, size);
 }

 int socket_close (void* this){

 	socket_cb* scb = (socket_cb*) this;

 	if(scb==NULL)
 			return -1;

 		if(scb->type==SOCKET_LISTENER){

 			PORT_MAP[scb->port] = NULL;
 			kernel_broadcast(&scb->listener_s.req_available);
 		}

 		if(scb->type==SOCKET_PEER){

 			if(scb->peer_s.read_pipe!=NULL){

 				pipe_reader_close(scb->peer_s.read_pipe);
 				scb->peer_s.read_pipe = NULL;
 			}

 			if(scb->peer_s.write_pipe!=NULL){

 				pipe_writer_close(scb->peer_s.write_pipe);
 				scb->peer_s.write_pipe = NULL;
 			}

 			scb->fcb = NULL;

 			if(scb->refcount==0)
 				free(scb);

 		}

	return 0;
 	
 }

static file_ops socket_fops = {
  .Open = socket_open,
  .Read = socket_read ,
  .Write = socket_write,
  .Close = socket_close
}; 

Fid_t sys_Socket(port_t port)
{
	Fid_t fid;
	FCB* fcb;

	if(port < NOPORT || port > MAX_PORT)
		return NOFILE;

	if ((FCB_reserve(1, &fid, &fcb) == 0))
		return NOFILE;


	/*Initialize socket_cb*/

	socket_cb* new_socket_cb;

	new_socket_cb = (socket_cb*)xmalloc(sizeof(socket_cb));

	new_socket_cb->refcount = 0;
	new_socket_cb->fcb = fcb ;

	new_socket_cb->type = SOCKET_UNBOUND;  
	new_socket_cb->port = port;

	fcb->streamobj = new_socket_cb;
	fcb->streamfunc = &socket_fops;

	rlnode_init(&new_socket_cb->unbound_s.unbound_socket,NULL);

	return fid;

}

int sys_Listen(Fid_t sock)
{
	FCB* fcb = get_fcb(sock);

	if (fcb==NULL )
	    return -1;

    socket_cb* scb = (socket_cb*) fcb->streamobj;

    if(scb == NULL || scb->port == NOPORT || PORT_MAP[scb->port] != NULL || scb->type != SOCKET_UNBOUND)
    	return -1;


    scb->type = SOCKET_LISTENER;

    rlnode_init(&scb->listener_s.queue, NULL);
    scb->listener_s.req_available = COND_INIT;

    PORT_MAP[scb->port]=scb;

    return 0;

}


Fid_t sys_Accept(Fid_t lsock)
{
	FCB* fcb = get_fcb(lsock);

	if (fcb==NULL )
	    return NOFILE;

	socket_cb* listener = (socket_cb*) fcb->streamobj;

	if(listener == NULL || listener->type != SOCKET_LISTENER || listener->port == NOPORT)
		return NOFILE;

	listener->refcount++;

	while(is_rlist_empty(&listener->listener_s.queue) && PORT_MAP[listener->port] != NULL)
		kernel_wait(&listener->listener_s.req_available,SCHED_PIPE);

	if(PORT_MAP[listener->port]==NULL)
		return NOFILE;

	connection_req* req =rlist_pop_front(&listener->listener_s.queue)->req;
    

	/*Create server peer socket*/

	Fid_t server_peer_fid = sys_Socket(listener->port);

	if(server_peer_fid == NOFILE)
		return NOFILE;

	req->admitted = 1;

   /*Create client peer socket for efficiency*/

    socket_cb* client_peer = req->peer;
    client_peer->type=SOCKET_PEER;

	FCB* server_peer_fcb = get_fcb(server_peer_fid);

	if(server_peer_fcb == NULL)
		return NOFILE;

	socket_cb* server_peer = (socket_cb*) server_peer_fcb->streamobj;
	server_peer->type = SOCKET_PEER;

	 /*Initialize the connection between the 2 peer sockets*/

    server_peer->peer_s.peer = client_peer;
    client_peer->peer_s.peer = server_peer;

	/*Initialize pipes*/

    pipe_cb* write_pipe = (pipe_cb*)xmalloc(sizeof(pipe_cb));
    pipe_cb* read_pipe = (pipe_cb*)xmalloc(sizeof(pipe_cb));

    write_pipe->writer=server_peer_fcb;
    write_pipe->reader=client_peer->fcb;
    write_pipe->w_pos = 0;
	write_pipe->r_pos = 0;
	write_pipe->data_length = 0;
	write_pipe->has_data = COND_INIT;
	write_pipe->has_space = COND_INIT;


    read_pipe->reader=server_peer_fcb;
    read_pipe->writer=client_peer->fcb;
    read_pipe->w_pos = 0;
	read_pipe->r_pos = 0;
	read_pipe->data_length = 0;
	read_pipe->has_data = COND_INIT;
	read_pipe->has_space = COND_INIT;

   /*Connect the pipes with the peer sockets*/

	server_peer->peer_s.write_pipe = write_pipe;
	server_peer->peer_s.read_pipe = read_pipe;

	client_peer->peer_s.write_pipe = read_pipe;
	client_peer->peer_s.read_pipe = write_pipe;

	listener->refcount--;

	/*Signal the Connect side*/
	 kernel_signal(&req->connected_cv);


     return server_peer_fid;

}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{

	FCB* fcb = get_fcb(sock);

	if (fcb==NULL )
	    return NOFILE;

	socket_cb* client_peer = (socket_cb*) fcb->streamobj;

	if(client_peer == NULL || client_peer->type != SOCKET_UNBOUND)
		return NOFILE;


	if(port <= NOPORT || port > MAX_PORT)
		return NOFILE;

	
	socket_cb* listener = PORT_MAP[port];

	if(listener == NULL || listener->type != SOCKET_LISTENER)
		return NOFILE;

	/*Increase refcount*/

	client_peer->refcount++;

	/*Initialize the connection request*/

	connection_req* req = (connection_req*)xmalloc(sizeof(connection_req));

	req->admitted = 0;
	req->peer = client_peer;
	req->connected_cv = COND_INIT;

	rlnode_init(&req->queue_node,req);

	/*Add request to the list*/
	rlist_push_back(&listener->listener_s.queue,&req->queue_node);

	/*Signal the listener socket*/

	kernel_signal(&listener->listener_s.req_available);

	int status = 1;

	while(req->admitted==0 && status == 1 && listener != NULL){
		status=kernel_timedwait(&req->connected_cv, SCHED_PIPE, timeout);
	}

	if (status==0)
		return NOFILE;

	client_peer->refcount--;

	rlist_remove(&req->queue_node);
	free(req);

  return 0;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	FCB* fcb = get_fcb(sock);

	if(fcb==NULL)
		return -1;

	socket_cb* socket = (socket_cb*)fcb->streamobj;

	if(socket->type != SOCKET_PEER)
		return -1;

	switch(how){

	case SHUTDOWN_READ:

		pipe_reader_close(socket->peer_s.read_pipe);

		socket->peer_s.read_pipe = NULL;

		break;

	case SHUTDOWN_WRITE:

		pipe_writer_close(socket->peer_s.write_pipe);
		socket->peer_s.write_pipe = NULL;

		break;

	case SHUTDOWN_BOTH:

		pipe_reader_close(socket->peer_s.read_pipe);
		pipe_writer_close(socket->peer_s.write_pipe);

		socket->peer_s.read_pipe = NULL;
		socket->peer_s.write_pipe = NULL;

		break;

	default:
		return -1;

	}

	return 0;
}

