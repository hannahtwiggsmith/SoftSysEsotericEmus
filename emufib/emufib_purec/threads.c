/*
* emufib Threading Library
* threads.c
*
* Handle basic threading functions: init, create, yield, destroy
*/



/* INCLUDES */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



/* THREAD DEFS */
#define MAX_THREADS 10
#define THREAD_STACK (1024*1024)
#define ASM_PREFIX ""

typedef struct
{
	void** stack;
	void* stack_bottom; /* malloc reference */
	int active;
} thread;


/* THREAD PARAMS */
/* Queue of threads to init */
static thread threadList[ MAX_THREADS ];
/* Current thread */
static int currentThread = -1;
/* True if in thread rather than main process */
static int inThread = 0;
/* Num active threads */
static int numThreads = 0;

/* Init a main thread */
static thread mainThread;

/* Define context switching functions */
extern int asm_switch(thread* next, thread* current, int return_value);
static void initStack(thread* thread, int stack_size, void (*fptr)(void));
extern void* asm_call_thread_exit;


/* THREAD FUNCTIONS */

/* 
* createStack 
* 
* Allocates a new stack in memory.  
* Takes in a pointer to the appropriate thread id, the size of the stack to be 
* allocated, and a pointer to the task function for the thread.
*/
static void createStack(thread* thread, int stack_size, void (*fptr)(void)) {
	int i;

		static const int NUM_REGISTERS = 32;

		assert(stack_size > 0);
		assert(fptr != NULL);

		/* Create a 16-byte aligned stack which will work on Mac OS X. */
		assert(stack_size % 16 == 0);
		thread->stack_bottom = malloc(stack_size);
		if (thread->stack_bottom == 0) return;
		thread->stack = (void**)((char*) thread->stack_bottom + stack_size);

	*(--thread->stack) = (void*) ((uintptr_t) &asm_call_thread_exit);
	*(--thread->stack) = (void*) ((uintptr_t) fptr);
	/* Init registers with NULL */
	for (i = 0; i < NUM_REGISTERS; ++i) {
		*(--thread->stack) = 0;
	}
}

/* 
* initThreads 
* 
* Initializes the thread list and the main thread.  Call this before creating any
* threads.
*/
void initThreads()
{
	memset(threadList, 0, sizeof(threadList));
	mainThread.stack = NULL;
	mainThread.stack_bottom = NULL;
	printf ("Initialized successfully.\n");
}

/*
* createThread
*
* Pass in a pointer to a task function to create a new thread.  Switches contexts and 
* creates the appropriate stack in memory, adding to the thread list.
*/

int createThread( void (*func)(void) )
{
	if ( numThreads == MAX_THREADS ) return 1; /* Max threads error */

	/* Set the context to a new stack */
	createStack( &threadList[numThreads], THREAD_STACK, func );
	if ( threadList[numThreads].stack_bottom == 0 )
	{
		printf( "Error: Could not allocate stack. \n" );
		return 2; /* Malloc error */
	}
	threadList[numThreads].active = 1;
	++ numThreads;

	return 0; /* No error */
}


/*
* threadYield
* 
* Provides context switching between threads.
* If currently in a thread, it will switch to the main context.  Otherwise, it will
* save the current state and switch in a new thread to the main context.
*/

void threadYield()
{
	/* If we are in a thread, switch to the main process */
	if ( inThread )
	{
		/* Switch to the main context */
		printf( "Thread %d yielding the processor. \n", currentThread );

		asm_switch( &mainThread, &threadList[currentThread], 0 );
	}
	/* Else, we are in the main process and we need to dispatch a new thread */
	else
	{
		if ( numThreads == 0 ) return;

		/* Saved the state so can call the next fiber */
		currentThread = (currentThread + 1) % numThreads;

		printf( "Switching to thread %d. \n", currentThread );
		inThread = 1;
		asm_switch( &threadList[ currentThread ], &mainThread, 0 );
		inThread = 0;
		printf( "Thread %d switched to main context. \n", currentThread );

		if ( threadList[currentThread].active == 0 )
		{
			printf( "Thread %d is finished. Cleaning up.\n", currentThread );
			/* Free the "current" fiber's stack pointer */
			free( threadList[currentThread].stack_bottom );

			/* Swap the last fiber with the current, now empty, entry */
			-- numThreads;
			if ( currentThread != numThreads )
			{
				threadList[ currentThread ] = threadList[ numThreads ];
			}
			threadList[ numThreads ].active = 0;
		}

	}
	return;
}

/* 
* waitForAllThreads 
*
* Run and yield all threads until the processess finish. 
*/
int waitForAllThreads()
{
	int threadsRemaining = 0;

	/* If we are in a fiber, wait for all the *other* fibers to quit */
	if ( inThread ) threadsRemaining = 1;

	printf( "Waiting until there are only %d threads remaining. \n", threadsRemaining );

	/* Execute the fibers until they quit */
	while ( numThreads > threadsRemaining )
	{
		threadYield();
	}

	return 0;
}


/*
* destroyThread
*
* Switch to the main context and destroy threads when the thread list reaches 0.
*/
void destroyThread() {
	assert( inThread );
	assert( 0 <= currentThread && currentThread < numThreads );
	threadList[currentThread].active = 0;
	asm_switch( &mainThread, &threadList[currentThread], 0 );

	/* asm_switch should never return for an exiting thread. */
	abort();
}

/* DEFINE ARCHITECTURE PARAMS */
/* THIS IS NOT THE ARDUINO VERSION */
asm(".globl " ASM_PREFIX "asm_call_thread_exit\n"
ASM_PREFIX "asm_call_thread_exit:\n"
/*"\t.type asm_call_thread_exit, @function\n"*/
"\tcall " ASM_PREFIX "destroyThread\n");


#ifdef __x86_64
/* arguments in rdi, rsi, rdx */
asm(".globl " ASM_PREFIX "asm_switch\n"
ASM_PREFIX "asm_switch:\n"
#ifndef __APPLE__
"\t.type asm_switch, @function\n"
#endif
/* Move return value into rax */
"\tmovq %rdx, %rax\n"

/* save registers: rbx rbp r12 r13 r14 r15 (rsp into structure) */
"\tpushq %rbx\n"
"\tpushq %rbp\n"
"\tpushq %r12\n"
"\tpushq %r13\n"
"\tpushq %r14\n"
"\tpushq %r15\n"
"\tmovq %rsp, (%rsi)\n"

/* restore registers */
"\tmovq (%rdi), %rsp\n"
"\tpopq %r15\n"
"\tpopq %r14\n"
"\tpopq %r13\n"
"\tpopq %r12\n"
"\tpopq %rbp\n"
"\tpopq %rbx\n"

/* return to the "next" thread */
"\tret\n");
#else
/* static int asm_switch(thread* next, thread* current, int return_value); */
asm(".globl " ASM_PREFIX "asm_switch\n"
ASM_PREFIX "asm_switch:\n"
#ifndef __APPLE__
"\t.type asm_switch, @function\n"
#endif
/* Move return value into eax, current pointer into ecx, next pointer into edx */
"\tmov 12(%esp), %eax\n"
"\tmov 8(%esp), %ecx\n"
"\tmov 4(%esp), %edx\n"

/* save registers: ebx ebp esi edi (esp into structure) */
"\tpush %ebx\n"
"\tpush %ebp\n"
"\tpush %esi\n"
"\tpush %edi\n"
"\tmov %esp, (%ecx)\n"

/* restore registers */
"\tmov (%edx), %esp\n"
"\tpop %edi\n"
"\tpop %esi\n"
"\tpop %ebp\n"
"\tpop %ebx\n"

/* return to the "next" thread with eax set to return_value */
"\tret\n");
#endif
