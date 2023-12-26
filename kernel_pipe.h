#ifndef __KERNEL_PIPE_H
#define __KERNEL_PIPE_H

#include "tinyos.h"
#include "kernel_dev.h"


#define PIPE_BUFFER_SIZE 4096



typedef struct pipe_control_block{

	FCB *reader,*writer;

	CondVar has_space; /* For blocking writer if no space is available */

  	CondVar has_data; /* For blocking reader until data are available */

  	char buffer[PIPE_BUFFER_SIZE];
  
  	int w_pos, r_pos, data_length;

	
} pipe_cb;



#endif