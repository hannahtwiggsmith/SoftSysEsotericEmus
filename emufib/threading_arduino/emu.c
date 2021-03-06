#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "emu.h"

volatile struct aos_sys _g_sys;
static void _aos_task_launcher(void);

__inline__
void _aos_common_sys_state_set(aos_sys_status_t a_state) {
	_g_sys.status &= 0xfc;
	_g_sys.status |= (a_state & 0x03);
}


/**
 * @brief idle task definition
 *
 * @param a_data private data for the task
 */
static void _aos_idle_task(void *a_pdata) {

	// busy loop
	while (1) {
		//printf("in idle task\n");
	}

}




void _aos_task_exit(void) {
  // call the task exit hook
  //_aos_common_hook_exec(AOS_HOOK_TASK_EXIT);

  printf("Inside task exit\n");

  // we will land up here if the task will exit suddenly
  while (1);
}

void init() {
	printf("Inside init.\n");

  printf("Initializing system data struct.\n");
	// initialize system data struct
	memset((void *)&_g_sys, 0x00, sizeof(struct aos_sys));

	printf("Creating and storing system stack pointer value...\n");
	// store system stack pointer value
	_g_sys.ctx.sp = (struct aos_machine_ctx *)SP;



	// create idle task
	_g_sys.current = aos_task_create(_aos_idle_task, NULL, 32);

	// setup as idle task
	_g_sys.rl.idle = _g_sys.current;


	// mark as running
	aos_task_state_set(_g_sys.current, AOS_TASK_RUNNING);

	// setup system status
	_aos_common_sys_state_set(AOS_SYS_READY);


	printf("%d\n", _g_sys.ctx.sp->pc.pc16);

	printf("Initialization success!\n");
}

struct task_cb* aos_task_create(task_proc_t a_task_proc, void *a_pdata, size_t a_stack) {

	printf("Inside task_create!\n");

	uint16_t blk_size = AOS_CALC_TASK_WA_SIZE(a_stack);

	struct task_cb *cb = (struct task_cb *)malloc(blk_size);

	memset(cb, 0x00, blk_size);
	cb->wa = (void *)((uint8_t *)cb + sizeof(struct task_cb));
	cb->wa_size = (blk_size - sizeof(struct task_cb));

	cb->proc = a_task_proc;
	cb->pdata = a_pdata;

	// initialize stack pointer
	cb->ctx.sp = (struct aos_machine_ctx *)((uint8_t *)cb +
			blk_size -
			AOS_MACHINE_CTX_SIZE() -
			AOS_IRQ_CTX_SIZE());

	// setup initial context
	// callback
	cb->ctx.sp->r[2]  = ((uint16_t)a_task_proc) & 0xff;
  	cb->ctx.sp->r[3]  = ((uint16_t)a_task_proc >> 8) & 0xff;

	// arguments
  	cb->ctx.sp->r[4]  = ((uint16_t)a_pdata) & 0xff;
  	cb->ctx.sp->r[5]  = ((uint16_t)a_pdata >> 8) & 0xff;

	// read status register
	cb->ctx.sp->sreg = SREG;

	// task launcher will be first called
	cb->ctx.sp->pc.pc8.lo = ((uint16_t)_aos_task_launcher >> 8) & 0xff;
	cb->ctx.sp->pc.pc8.hi = ((uint16_t)_aos_task_launcher) & 0xff;


	return cb;
}

void run() {
	printf("Inside run!\n");
	printf("_g_sys.current: \n");
	AOS_CTX_SAVE();

	// set sp to current task's context and restore it
	AOS_CTX_SP_SET(_g_sys.current);

	printf("About to save context\n");
  //printf("%d\n", _g_sys.ctx.sp->r);

	printf("Restore SP\n");

	AOS_CTX_RESTORE();

	printf("About to jump to first task.\n");

	// jump to first task
	AOS_CTX_POP_PC();

	printf("POP\n");
}

__inline__
void aos_task_state_set(struct task_cb *a_task, uint8_t a_state) {
	printf("Inside aos task state set.\n");
	a_task->prio_state &= 0xf0;
	a_task->prio_state |= (a_state & 0x0f);
}


__attribute__((naked))
static void _aos_task_launcher(void) {
	//printf("Inside aos task launcher.\n");

  __asm__ volatile ("movw  r24, r4");
  __asm__ volatile ("movw  r30, r2");

	//printf("r30: %s\n");
	//printf("right before icall.\n");
  __asm__ volatile ("icall");
	//__asm__ volatile ("call task1");
  // task is running now
	//printf("after icall.\n");
  __asm__ volatile ("call _aos_task_exit");
}



__inline__
aos_sys_status_t aos_common_sys_state_get() {
	printf("Inside aos common sys state get.\n");
	return (_g_sys.status & 0x03);
}


__inline__
uint8_t _aos_common_isr_interrupted_get() {
	return ((_g_sys.status & 0x04) ? 0x01: 0x00);
}


__inline__
struct task_cb* aos_common_current_get() {
	return _g_sys.current;
}


__inline__
void _aos_common_isr_interrupted_set(uint8_t a_isr_int_flag) {
	if (a_isr_int_flag)
		_g_sys.status |= 0x04;
	else
		_g_sys.status &= 0xfb;
}
