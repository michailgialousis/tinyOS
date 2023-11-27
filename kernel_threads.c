
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"


void start_main_thread_lookalike()
{
  int exitval;

  Task call = cur_thread()->ptcb->task;
  int argl = cur_thread()->ptcb->argl;
  void* args = cur_thread()->ptcb->args;

  exitval = call(argl,args);
  ThreadExit(exitval);
}

void initialize_PTCB(PTCB* ptcb,TCB* tcb)
{

  ptcb->tcb = tcb;
  
  ptcb->task = CURPROC->main_task;  // it should do the pcb task's, right?

  ptcb->argl = 0;
  ptcb->args = NULL;

 //for(int i=0;i<MAX_FILEID;i++)   /* I dont know how many ptcbs we can have*/
   // ptcb->FIDT[i] = NULL;

  ptcb->detached =0;
  ptcb->exited = 0;
  

  rlnode_init(& ptcb->ptcb_list_node, ptcb);
  ptcb->exit_cv = COND_INIT;

  ptcb->refcount = 0;
}


/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{


  // Initialize and return a new TCB with spawn
  TCB* new_thread ;
  new_thread= spawn_thread(cur_thread()->owner_pcb, start_main_thread_lookalike);
  CURPROC->thread_count++
  
  // Acquire a PTCB (allocate space, make connections with PCB and TCB)
  PTCB* new_ptcb = (PTCB*)xmalloc(sizeof(PTCB));
  assert(new_ptcb!=NULL);

  // Initialize PTCB
  initialize_PTCB(new_ptcb,new_thread); // i dont about the for-loop
  rlist_push_back(&CURPROC->ptcb_list,&new_ptcb->ptcb_list_node);

  new_thread->ptcb = new_ptcb;

  
  // Wake up TCB
  wakeup(new_thread);
  assert(new_thread->state == READY);

	return (Tid_t) new_ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread()->ptcb; /////EDO ENA BELAKI 
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{

  PTCB* joining_ptcb = (PTCB*)tid;

  // im not sure if i check the possibility 
  // that a thread from another procesess wants to join 
 
  // First, we check if we can indeed join the thread
  if(joining_ptcb->tcb == NULL){ // not surer if NULL will suffice
    return -1;
  }
  if(ThreadSelf() == tid){
    return -1;
  }
  if(joining_ptcb->detached == 1){
    return -1
  }
  else

    joining_ptcb->refcount ++;

    // Then we sleep...Zzzz
    kernel_wait(joining_ptcb->exit_cv,SCHED_USER);

    // Check if is indeed exited
    assert(joining_ptcb->exited == 0);

    joining_ptcb->refcount --;

    *exitval = joining_ptcb->exitval; // I may be doing it wrong maybe in need the address or smth

	 return 0;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PTCB* cur_ptcb = (Tid_t) tid;

  if(cur_ptcb->tcb == NULL){ 
    return -1;
  }
  if(cur_ptcb->exited == 1){
    return -1;
  }
  else

    cur_ptcb->detached = 1;

	 return 0;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

  PCB *curproc = CURPROC;

if(curproc->thread_count == 1){
    /* Reparent any children of the exiting process to the 
       initial task */
    PCB* initpcb = get_pcb(1);
    while(!is_rlist_empty(& curproc->children_list)) {
      rlnode* child = rlist_pop_front(& curproc->children_list);
      child->pcb->parent = initpcb;
      rlist_push_front(& initpcb->children_list, child);
    }

    /* Add exited children to the initial task's exited list 
       and signal the initial task */
    if(!is_rlist_empty(& curproc->exited_list)) {
      rlist_append(& initpcb->exited_list, &curproc->exited_list);
      kernel_broadcast(& initpcb->child_exit);
    }

    /* Put me into my parent's exited list */
    rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
    kernel_broadcast(& curproc->parent->child_exit);

  

  assert(is_rlist_empty(& curproc->children_list));
  assert(is_rlist_empty(& curproc->exited_list));


  /* 
    Do all the other cleanup we want here, close files etc. 
   */

  /* Release the args data */
  if(curproc->args) {
    free(curproc->args);
    curproc->args = NULL;
  }

  /* Clean up FIDT */
  for(int i=0;i<MAX_FILEID;i++) {
    if(curproc->FIDT[i] != NULL) {
      FCB_decref(curproc->FIDT[i]);
      curproc->FIDT[i] = NULL;
    }
  }

  /* Disconnect my main_thread */
  curproc->main_thread = NULL;

  /* Now, mark the process as exited. */
  curproc->pstate = ZOMBIE;
}

  curproc->thread_count--//KAlitera sthn arxi kato apo curproc

  kernel_broadcast(CURPROC->ptcb->exit_cv);

  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);

//kapou EDW prepei na kanoyme COND_BBRODADCASt

}




