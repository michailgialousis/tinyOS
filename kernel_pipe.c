
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"
#include "kernel_cc.h"



int pipe_read(void* input_pipe_cb, char *buf, unsigned int size)
{

	pipe_cb* pipe_cb = input_pipe_cb;

	if(pipe_cb == NULL)
		return -1;

	if(pipe_cb->reader==NULL){
		kernel_broadcast(&pipe_cb->has_space);
		return -1;
	}
	
	int data_read = 0;

	for(int i = 0 ; i<size  ; i++)
	{ 
		if(pipe_cb->reader == NULL) //reader end might close while we read
			return data_read;

		while((pipe_cb->data_length == 0) && pipe_cb->writer!=NULL){
     kernel_broadcast(&pipe_cb->has_space);
		 kernel_wait(&pipe_cb->has_data, SCHED_PIPE);
	}

    pipe_cb->data_length--;
		data_read++;

    if(pipe_cb->r_pos == pipe_cb->w_pos && pipe_cb->writer==NULL) 
    	return i;

		buf[i] = pipe_cb->buffer[pipe_cb->r_pos];	
		pipe_cb->r_pos = (pipe_cb->r_pos + 1) % PIPE_BUFFER_SIZE;

	}

	kernel_broadcast(&pipe_cb->has_space);

  return data_read;
}

int  pipe_reader_write(void* pipe_cb, const char* buf, unsigned int size)
{
    /* Reader can't write*/
    return -1;
}


int pipe_reader_close(void* input_pipe_cb) 
{
	pipe_cb* pipe_cb = input_pipe_cb;

   	if(pipe_cb==NULL)
		return -1;

    pipe_cb->reader = NULL;
 	

    if(pipe_cb->writer == NULL)
  	  free(pipe_cb);
    else
  	  kernel_broadcast(&pipe_cb->has_space);

  return 0;
}

void* pipe_reader_open(uint minor)
{
  return NULL;
}



int  pipe_write(void* input_pipe_cb, const char* buf, unsigned int size)
{

	pipe_cb* pipe_cb = input_pipe_cb;

	if((pipe_cb == NULL || pipe_cb->writer == NULL || pipe_cb->reader == NULL))
	{
		kernel_broadcast(&pipe_cb->has_data);
		return -1;
	}

	int data_written = 0;

	for(int i=0 ;i<size ;i++)
	{

	
	while((pipe_cb->data_length==PIPE_BUFFER_SIZE) && (pipe_cb->reader != NULL) && (pipe_cb->writer != NULL))
	{ 
		kernel_broadcast(&pipe_cb->has_data);
		kernel_wait(&pipe_cb->has_space,SCHED_PIPE);
	}

		if((pipe_cb->writer == NULL || pipe_cb->reader == NULL))
	{
		kernel_broadcast(&pipe_cb->has_data);
		return data_written;
	}
    pipe_cb->data_length++;
		data_written++;

		pipe_cb->buffer[pipe_cb->w_pos] = buf[i];
		pipe_cb->w_pos = (pipe_cb->w_pos +1) %PIPE_BUFFER_SIZE;
		
	}

	kernel_broadcast(&pipe_cb->has_data);

  return data_written;
}

int  pipe_writer_read(void* pipe_cb, char* buf, unsigned int size)
{
    /* Writer can't read*/
    return -1;
}



int pipe_writer_close(void* input_pipe_cb) 
{
	pipe_cb* pipe_cb = input_pipe_cb;

	if(pipe_cb==NULL)
		return -1;

  pipe_cb->writer = NULL;


  if(pipe_cb->reader == NULL)
  	free(pipe_cb);
  else
  	kernel_broadcast(&pipe_cb->has_data);

  return 0;
}

void* pipe_writer_open(uint minor)
{
  return NULL;
}

static file_ops writer_fops = {
  .Open = pipe_writer_open,
  .Read = pipe_writer_read ,
  .Write = pipe_write,
  .Close = pipe_writer_close
}; 

static file_ops reader_fops = {
  .Open = pipe_reader_open,
  .Read = pipe_read,
  .Write = pipe_reader_write,
  .Close = pipe_reader_close
};


int sys_Pipe(pipe_t* pipe)
{

	Fid_t fid[2];
	FCB* fcb[2];


	if(FCB_reserve(2, fid, fcb)==0 )
		return -1;


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


	new_pipe_cb->reader = R;
	new_pipe_cb->writer = W;

	 
	new_pipe_cb->reader->streamobj  = new_pipe_cb;
	new_pipe_cb->writer->streamobj= new_pipe_cb;

	new_pipe_cb->reader->streamfunc = &reader_fops;
	new_pipe_cb->writer->streamfunc = &writer_fops;

	new_pipe_cb->has_data = COND_INIT;
	new_pipe_cb->has_space = COND_INIT;




	return 0;
}


