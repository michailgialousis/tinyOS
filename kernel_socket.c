


#include "kernel_socket.h"

void* socket_open (uint minor){
	return NULL;
}

int socket_read (void* this, char *buf, unsigned int size){
	return -1;
}

 int socket_write (void* this, const char* buf, unsigned int size){
 	return -1;
 }

 int socket_close (void* this){

 	socket_cb* scb = (socket_cb*) this;
 	if(scb==NULL)
		return -1;
	free(scb);
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

	if(port < NOPORT || port > MAX_PORT || FCB_reserve(1, &fid, &fcb)==0 )
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
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

