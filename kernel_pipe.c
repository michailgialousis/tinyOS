
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"
#include "kernel_cc.h"



int pipe_read(void* input_pipe_cb, char *buf, unsigned int size)
{

/* can i read 
		- if writer is open !!!! how do i check that !!! -> sleep
		- if buffer empty-> sleep
	*/
	pipe_cb* pipe_cb = input_pipe_cb;

	if((pipe_cb == NULL) || (pipe_cb->reader == NULL))
	{
		kernel_broadcast(&(pipe_cb->has_space));
		return -1;
	}

	while(pipe_cb->data_length == 0 && pipe_cb->writer!=NULL)
	{
    kernel_broadcast(&(pipe_cb->has_space));
		kernel_wait(&(pipe_cb->has_data), SCHED_PIPE);
	}

	int data_read = 0;

	for(int i = 0 ; (i<size) && (i<=PIPE_BUFFER_SIZE);i++)
	{ 
		if(pipe_cb->reader==NULL) //reader end might close while we read
			return data_read;

		buf[i] = pipe_cb->buffer[pipe_cb->r_pos];	
		pipe_cb->r_pos = (pipe_cb->r_pos + 1) % PIPE_BUFFER_SIZE;
		pipe_cb->data_length--;
		data_read++;

		if(data_read==size && pipe_cb->writer==NULL)
			return 0;
	}


	kernel_broadcast(&(pipe_cb->has_space));


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
 	kernel_broadcast(&pipe_cb->has_space);

  if(pipe_cb->writer == NULL)
  	free(pipe_cb);

  return 0;
}

void* pipe_reader_open(uint minor)
{
  return NULL;
}



int  pipe_write(void* input_pipe_cb, const char* buf, unsigned int size)
{

	pipe_cb* pipe_cb = input_pipe_cb;

	if((pipe_cb == NULL ))
	{
		return -1;
	}

	/* i think the second check is wrong but xenia told me to write it, im not sure i understand it */

	while((pipe_cb->data_length == PIPE_BUFFER_SIZE) && (pipe_cb->reader != NULL))
	{
		kernel_wait(&(pipe_cb->has_space),SCHED_PIPE);
	}

	/* not sure if this check is necessary, my thought is that if reader gets null while we wait.. */
  if (pipe_cb->reader == NULL)
		return -1;



	int data_written = 0;

	for(int i=0 ;(i<size)&&(i<=PIPE_BUFFER_SIZE);i++)
	{
		pipe_cb->buffer[pipe_cb->w_pos] = buf[i];
		pipe_cb->w_pos = (pipe_cb->w_pos +1) %PIPE_BUFFER_SIZE;
		pipe_cb->data_length++;
		data_written++;
	}

	kernel_broadcast(&(pipe_cb->has_data));

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

 	kernel_broadcast(&pipe_cb->has_data);

  if(pipe_cb->reader == NULL)
  	free(pipe_cb);

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

	 
	R->streamobj = new_pipe_cb;
	W->streamobj= new_pipe_cb;

	R->streamfunc = &reader_fops;
	W->streamfunc = &writer_fops;


	return 0;
}


