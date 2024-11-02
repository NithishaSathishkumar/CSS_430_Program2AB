/*
  Nithisha Sathishkumar
  Program 2A
  Professor Peng

  Command: 
  g++ driver.cpp
  ./a.out
*/
#include <setjmp.h> // setjmp()
#include <signal.h> // signal()
#include <unistd.h> // sleep(), alarm()
#include <stdio.h>  // perror()
#include <stdlib.h> // exit()
#include <iostream> // cout
#include <string.h> // memcpy
#include <queue>    // queue

//Macro for initializing scheduler and entering scheduling loop
#define scheduler_init(){	\
  if(setjmp(main_env) == 0) \
    scheduler( );	\
}

//macro for starting scheduler by moving to its saved context
#define scheduler_start(){ \
  if(setjmp(main_env) == 0) \
    longjmp(scheduler_env, 1); \
}

// TODO: to be implemented in this assignment
//This macro saves current thread's context and stack state
//It preserves the stack contents for subsequent usage as well as current stack pointer
#define capture(){ \
  register void *bp asm("bp"); /* Declare bp register to capture current stack's base pointer */ \ 
  register void *sp asm("sp"); /* Declare sp register to capture current stack pointer */ \
  thr_queue.push(cur_tcb); /* Add current thread control block to scheduler queue */ \
  cur_tcb->size = (int)((long long int)bp - (long long int)sp); /* Calculate stack size based on bp & sp position */ \
  cur_tcb->stack = (void*) malloc(cur_tcb->size); /* Allocate memory for stack to preserve current state */ \
  cur_tcb->sp = sp; /* Save current stack pointer into TCB */ \
  memcpy(cur_tcb->stack, cur_tcb->sp, cur_tcb->size); /* Copy stack contents from sp to allocated memory in TCB */ \
}

// TODO: to be implemented in this assignment  
//Macro to give the scheduler control over the current thread
#define sthread_yield(){ \
  if(alarmed == true){ /* Check if alarm flag is set */ \
    alarmed = false; /* Reset alarm flag after detecting it */ \
    if(setjmp(cur_tcb->env) != 0){ \
      memcpy(cur_tcb->sp, cur_tcb->stack, cur_tcb->size); /* Restore saved stack contents to current stack pointer */ \
    }else{ \
      capture(); /* Save current thread's context and stack by calling capture */ \
      longjmp(scheduler_env, 1); /* Transfer control to scheduler by jumping to scheduler environment */ \
    } \
  } \
}

//Macro to initialize a thread by collecting its stack and returning to main
#define sthread_init() { \
  if(setjmp(cur_tcb->env) != 0){ \
    memcpy(cur_tcb->sp, cur_tcb->stack, cur_tcb->size);	\
  }else{ \
    capture(); \
    longjmp(main_env, 1); \
  }\
}

//Macro for creating a new thread, setting up functions and arguments
#define sthread_create(function, arguments){ \
  if(setjmp(main_env) == 0){ \
    func = &function;	\
    args = arguments;	\
    thread_created = true; \
    cur_tcb = new TCB(); \
    longjmp(scheduler_env, 1); \
  }	\
}

// Macro to exit the current thread, free its stack, and move to the next
#define sthread_exit(){	\
  if (cur_tcb->stack != NULL)	\
    free(cur_tcb->stack);	\
  longjmp( scheduler_env, 1);	\
}

using namespace std;

static jmp_buf main_env;
static jmp_buf scheduler_env;

// Thread control block
class TCB {
public:
  TCB() : sp(NULL), stack(NULL), size(0) {}
  jmp_buf env; // the execution environment captured by set_jmp( )
  void* sp; // the stack pointer 
  void* stack; // the temporary space to maintain the latest stack contents
  int size; // the size of the stack contents
};
static TCB* cur_tcb; // the TCB of the current thread in execution

// The queue of active threads
static queue<TCB*> thr_queue;

// Alarm caught to switch to the next thread
static bool alarmed = false;
static void sig_alarm( int signo ) {
  alarmed = true;
}

// A function to be executed by a thread
void (*func)(void *);
void *args = NULL;
static bool thread_created = false;

static void scheduler( ) {
  // invoke scheduler
  if(setjmp(scheduler_env) == 0){
    cerr << "scheduler: initialized" << endl;
    if (signal(SIGALRM, sig_alarm) == SIG_ERR) {
      perror("signal function");
      exit(-1);
    }
    longjmp(main_env, 1);
  }

  // check if it was called from sthread_create( )
  if(thread_created == true){
    thread_created = false;
    (*func)(args);
  }

  // restore the next thread's environment
  if((cur_tcb = thr_queue.front()) != NULL){
    thr_queue.pop();
    alarm(5); // allocate a time quontum of 5 seconds
    longjmp(cur_tcb->env, 1); // return to the next thread's execution
  }

  // no threads to schedule, simply return
  cerr << "scheduler: no more threads to schedule" << endl;
  longjmp(main_env, 2);
}


