
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"


int pipe_reader_read(pipe_cb* pipe_cb, char *buf, unsigned int size)
{
  
  return -1;
}

int  pipe_reader_write(pipe_cb* pipe_cb, const char* buf, unsigned int size)
{
    /* Reader can't write*/
    return -1;
}


int pipe_reader_close(pipe_cb* pipe_cb) 
{
  return -1;
}

void* pipe_reader_open(uint minor)
{
  return NULL;
}

static file_ops reader_fops = {
  .Open = pipe_reader_open,
  .Read = pipe_reader_read,
  .Write = pipe_reader_write,
  .Close = pipe_reader_close
};

int pipe_writer_read(pipe_cb* pipe_cb, char *buf, unsigned int size)
{
  /* Writer can't read */

  return -1;
}

int  pipe_writer_write(pipe_cb* pipe_cb, const char* buf, unsigned int size)
{
    
    
    return -1;
}


int pipe_writer_close(pipe_cb* pipe_cb) 
{
  return -1;
}

void* pipe_writer_open(uint minor)
{
  return NULL;
}

static file_ops writer_fops = {
  .Open = pipe_writer_open,
  .Read = pipe_writer_read,
  .Write = pipe_writer_write,
  .Close = pipe_writer_close
};


int sys_Pipe(pipe_t* pipe)
{

	Fid_t fid[2];
	FCB* fcb[2];

	
	if(FCB_reserve(2, fid, fcb)==0 )
	{
		return -1;
		
	}

	Fid_t r;
	Fid_t w;
	FCB* R;
	FCB* W;


	r = fid[0];
	w = fid[1];
	R = fcb[0];
	W = fcb[1];

	pipe->read = r;
	pipe->write = w;


	/* Initialize pipe_cb  */

	pipe_cb* new_pipe_cb;

	new_pipe_cb = (pipe_cb*)xmalloc(sizeof(pipe_cb));

	new_pipe_cb->w_pos = 0;
	new_pipe_cb->r_pos = 0;
	new_pipe_cb->data_length = 0;

	new_pipe_cb-> reader = R;
	new_pipe_cb-> writer = W;

	/* is this correct ? */  
	R->streamobj = new_pipe_cb;
	W->streamobj= new_pipe_cb;

	R->streamfunc = &reader_fops;
	W->streamfunc = &writer_fops;
	

	return 0;
}



