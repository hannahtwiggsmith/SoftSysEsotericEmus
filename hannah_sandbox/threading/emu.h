
#include <stdlib.h>

/**
 * @brief initialize the system
 *
 * Initializes the internal system data structures to known conditions. Zeros _g_sys static variable,
 * creates an idle task and sets everything up, so the system is ready to accept new tasks being created
 * and to be started
 *
 * @param a_freq tick frequency
 */
void init();

typedef void (*task_proc_t)(void *a_data);

/**
 * @brief launch the scheduler and all tasks
 *
 * From now on, the system will start execution. This call is blocking and should never return.
 */
void run();


/**
 * @file aos_task.h
 *
 * @brief task control block interface
 *
 * Declares task control block and task related routines and interface
 */


// ================================================================================

/**
 * @brief this macro is used to determine the task "work area" size.
 *
 * @param __size stack size requested by the task
 *
 * It returns how much memory is needed for a task, which wants to have
 * a stack of size __size for it's private needs
 */
#define AOS_CALC_TASK_WA_SIZE(__size) \
	(__size + AOS_IRQ_CTX_SIZE() + AOS_MACHINE_CTX_SIZE() + sizeof(struct task_cb))

// ================================================================================

/**
 * @brief task procedure type declaration
 *
 * @param a_data private data for the task callback
 */
typedef void (*task_proc_t)(void *a_data);


/**
 * @brief task context block
 */
struct task_cb {

	/// task list pointers must be on top
	struct task_cb *prv, *nxt;

	/// holds current stack pointer, and reference to machine state

	/// task priority / task state combined field
	uint8_t prio_state;

	/// number of time quanta's task can consume
	uint8_t quanta;

	/// task execution handler
	task_proc_t proc;

	/// number of systicks consumed by the task

	/// work area size
	uint16_t wa_size;

	/// work area pointer
	void *wa;

	/// task private data
	void *pdata;
};


// ================================================================================

/**
 * @brief Creates a system task and ads it to system runlist
 *
 * Creates the system task, returns the freshly created task control block and adds
 * the task to system runlist.
 *
 * @param a_task_proc function callback for the task
 * @param a_pdata any private data which will be passed as tasks argument, NULL if none.
 * @param a_prio task priority
 * @param a_stack stack for the task, a value of 32 bytes is recommended
 *
 * @return task control block
 */
struct task_cb* aos_task_create(task_proc_t a_task_proc, size_t a_stack);


/**
 * @brief configures task priority to a given value
 *
 * @param a_task task which priority will be changed
 * @param a_prio priority
 */
void aos_task_priority_set(struct task_cb *a_task, uint8_t a_prio);


/**
 * @brief set task state
 *
 * This function shouldn't be used outside of the system.
 *
 * @param a_task task
 * @param a_state state
 */
void aos_task_state_set(struct task_cb *a_task, uint8_t a_state);


/**
 * @brief return the priority of a specified task
 *
 * @param a_task task to be examined
 *
 * @return priority
 */
uint8_t aos_task_priority_get(struct task_cb *a_task);


/**
 * @brief returns task state
 *
 * @param a_task task
 *
 * @return state
 */
uint8_t aos_task_state_get(struct task_cb *a_task);



/**
 * @brief full machine context.
 *
 * This structure will be mapped to the current stack pointer position
 *  and resemble the current machine state for the task.
 */
struct aos_machine_ctx {

	/**
	 * @brief since stack pointer is post incremented the machine context must by one byte padded
	 */
	uint8_t __sp_post_incr_padding;

	/// general purpose registers
	uint8_t r[32];

	/// status register
	uint8_t sreg;

	/// program counter is pushed first, thus last in the map
	union {
		/// as 8 bit register values
		struct {
			/// SPL
			uint8_t lo;

			/// SPH
			uint8_t hi;
		} pc8;

		/// as a 16 bit value
		uint16_t pc16;
	} pc;
};


/**
 * @brief wrapper over the machine context
 */
struct aos_ctx {

	/**
	 * @brief this pointer will be updated to current stack pointer value
	 */
	volatile struct aos_machine_ctx *sp;
};


/**
 * @brief return the size of the machine context
 */
#define AOS_MACHINE_CTX_SIZE()	(sizeof(struct aos_machine_ctx))


/**
 * @brief defines how much additional space in the process's work area for interrupt call stack
 *  is needed
 */
#define AOS_IRQ_CTX_SIZE() 8


/**
 * @brief switches stack pointer to the specified task context
 */
#define AOS_CTX_SP_SET(__task)	SP = (uint16_t)__task->ctx.sp


/**
 * @brief saves current stack pointer value to stack context of the current task
 */
#define AOS_CTX_SP_GET(__task) __task->ctx.sp = (struct aos_machine_ctx *)SP


/**
 * @brief update the program counter with a value from the stack
 */
#define AOS_CTX_POP_PC() __asm__ volatile ("ret" ::)


/**
 * @brief save machine context
 */
#define AOS_CTX_SAVE() \
	__asm__ volatile("push r31\n\t" \
		"in r31, 0x3f\n\t" \
		"push r31\n\t" \
		"push r30\n\t" \
		"push r29\n\t" \
		"push r28\n\t" \
		"push r27\n\t" \
		"push r26\n\t" \
		"push r25\n\t" \
		"push r24\n\t" \
		"push r23\n\t" \
		"push r22\n\t" \
		"push r21\n\t" \
		"push r20\n\t" \
		"push r19\n\t" \
		"push r18\n\t" \
		"push r17\n\t" \
		"push r16\n\t" \
		"push r15\n\t" \
		"push r14\n\t" \
		"push r13\n\t" \
		"push r12\n\t" \
		"push r11\n\t" \
		"push r10\n\t" \
		"push r9\n\t" \
		"push r8\n\t" \
		"push r7\n\t" \
		"push r6\n\t" \
		"push r5\n\t" \
		"push r4\n\t" \
		"push r3\n\t" \
		"push r2\n\t" \
		"push r1\n\t" \
		"clr r1\n\t" \
		"push r0\n\t" \
		::)


/**
 * @brief restore machine context
 */
#define AOS_CTX_RESTORE() \
	__asm__ volatile ("pop r0\n\t" \
		"pop r1\n\t" \
		"pop r2\n\t" \
		"pop r3\n\t" \
		"pop r4\n\t" \
		"pop r5\n\t" \
		"pop r6\n\t" \
		"pop r7\n\t" \
		"pop r8\n\t" \
		"pop r9\n\t" \
		"pop r10\n\t" \
		"pop r11\n\t" \
		"pop r12\n\t" \
		"pop r13\n\t" \
		"pop r14\n\t" \
		"pop r15\n\t" \
		"pop r16\n\t" \
		"pop r17\n\t" \
		"pop r18\n\t" \
		"pop r19\n\t" \
		"pop r20\n\t" \
		"pop r21\n\t" \
		"pop r22\n\t" \
		"pop r23\n\t" \
		"pop r24\n\t" \
		"pop r25\n\t" \
		"pop r26\n\t" \
		"pop r27\n\t" \
		"pop r28\n\t" \
		"pop r29\n\t" \
		"pop r30\n\t" \
		"pop r31\n\t" \
		"out 0x3f, r31\n\t" \
		"pop r31\n\t" \
		::)
