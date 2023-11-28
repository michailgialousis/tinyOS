
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
#include "kernel_streams.h"

void create_process_thread()
{
  int exitval;

  Task call = cur_thread()->ptcb->task;
  int argl = cur_thread()->ptcb->argl;
  void* args = cur_thread()->ptcb->args;

  exitval = call(argl,args);

  sys_ThreadExit(exitval);
}


/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{

  TCB* new_tcb;
  PTCB* new_ptcb;

  /*Initialize a new TCB*/

  new_tcb = spawn_thread(cur_thread()->owner_pcb, create_process_thread);

  new_ptcb = (PTCB*)xmalloc(sizeof(PTCB));
  assert(new_ptcb!=NULL);

  initialize_PTCB(new_ptcb,new_tcb);
  new_ptcb->task = task;
  new_ptcb->argl = argl;
  new_ptcb->args = args;
  new_ptcb->exited = 0;
  new_ptcb->detached = 0;
  new_tcb->ptcb= new_ptcb;


  CURPROC->thread_count++;

  rlist_push_back(&CURPROC->ptcb_list,&new_ptcb->ptcb_list_node);
  
  wakeup(new_tcb);

  return (Tid_t) new_ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
  return (Tid_t) cur_thread()->ptcb;  
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
  if(rlist_find(&CURPROC->ptcb_list,joining_ptcb,NULL) == NULL){ 
    return -1;
  }

  if(joining_ptcb == NULL){
    return -1;
  }


  if(sys_ThreadSelf() == tid){
    return -1;
  }

  if(joining_ptcb->detached == 1){
    return -1;
  }

    joining_ptcb->refcount++;

    // Then we sleep...Zzzz
    while ((joining_ptcb->exited == 0) && (joining_ptcb->detached == 0))
        kernel_wait(&(joining_ptcb->exit_cv),SCHED_USER);

    // Check if thread is indeed exited
    //assert(joining_ptcb->exited == 0);

    joining_ptcb->refcount--;

    if (joining_ptcb->detached == 1){
      return -1;
    }

    if (exitval!=NULL)
      *exitval = joining_ptcb->exitval; // I may be doing it wrong maybe in need the address or smth

    if (joining_ptcb->refcount == 0) {
      rlist_remove(&joining_ptcb->ptcb_list_node);
      free(joining_ptcb);
    }

   return 0;
 }


/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
 PTCB* cur_ptcb = (PTCB*) tid;

 if(rlist_find(&CURPROC->ptcb_list,cur_ptcb,NULL) == NULL)
   return -1;

  if(cur_ptcb->exited == 1){
    return -1;
  }

   cur_ptcb->detached = 1;
   kernel_broadcast(&cur_ptcb->exit_cv);

   return 0;
 }


/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
  
  PCB *curproc = CURPROC;
  curproc->thread_count--;
  PTCB* ptcb = cur_thread()->ptcb;
  ptcb->exited = 1;
  ptcb->exitval = exitval;
  kernel_broadcast(&ptcb->exit_cv);


  if(curproc->thread_count==0){
    if(get_pid(curproc)!=1){

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
  }
  

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

   
   
  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);

}
