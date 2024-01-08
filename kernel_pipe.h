#ifndef __KERNEL_PIPE_H
#define __KERNEL_PIPE_H

#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_cc.h"

#define PIPE_BUFFER_SIZE 4096



typedef struct pipe_control_block{

	FCB *reader,*writer;

	CondVar has_space; /* For blocking writer if no space is available */

  	CondVar has_data; /* For blocking reader until data are available */

  	char buffer[PIPE_BUFFER_SIZE];
  
  	int w_pos, r_pos, data_length;

	
} pipe_cb;

int sys_Pipe(pipe_t* pipe);

int pipe_read(void* input_pipe_cb, char *buf, unsigned int size);
int pipe_reader_write(void* pipe_cb, const char* buf, unsigned int size);
int pipe_reader_close(void* input_pipe_cb);
void* pipe_reader_open(uint minor);

int  pipe_write(void* input_pipe_cb, const char* buf, unsigned int size);
int  pipe_writer_read(void* pipe_cb, char* buf, unsigned int size);
int pipe_writer_close(void* input_pipe_cb);
void* pipe_writer_open(uint minor);


#endif