/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

#ifndef OS_SHIM_TASK_H
#define OS_SHIM_TASK_H

/**
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_TASK
 * @{
 * This section introduces the TASK module's enum, structure, functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup os_shim_task_typedef Typedef
 * @{
 */

/** @brief TASK define task handle. */
typedef void *os_task_h;

/**
 * @brief This function is used to define task routing function pointer type.
 * @param arg is the parameter is registered when creating a task.
 */
typedef void (*os_task_func_t)(void *arg);
/**
 * @}
 */

/**
 * @defgroup os_shim_task_enum Enum
 * @{
 */

/** @brief TASK state. */
typedef enum {
    TASK_STATE_RUN = 0, /* A task is querying the state of itself,
                    so must be running. */
    TASK_STATE_READY,   /* The task being queried is in a read or pending
                    ready list. */
    TASK_STATE_BLOCK,   /* The task being queried is in the Blocked state. */
    TASK_STATE_SUSPEND, /* The task being queried is in the Suspended state,
                    or is in the Blocked state with an infinite time out. */
    TASK_STATE_DELETE,  /* The task being queried has been deleted,
                    but its TCB has not yet been freed. */
    TASK_STATE_INVALID  /* Used as an 'invalid state' value. */
} task_state_e;
/**
 * @}
 */

/**
 * @defgroup os_shim_task_struct Struct
 * @{
 */
typedef struct _task_info_t {
    uint32_t id;
    uint32_t priority;
    task_state_e status;
    void *stack_base;
    uint32_t stack_size;
    uint32_t cpu_ts;
    const char *name;
} task_info_t;

typedef struct _task_priority_t {
    char *name;
    uint8_t priority;
} task_priority_t;

typedef struct _task_snapshot_t {
    void *tcb_ptr;    /*!< Address of task control block. */
    uint32_t task_id; /*!< Task id for snapshot */
    uint32_t *stack_start;    /*!< Points to the location of the last item placed on the tasks stack. */
    uint32_t *stack_end; /*!< Points to the end of the stack.*/
} task_snapshot_t;
/**
 * @}
 */

/**
 * Priority definition. 5 priorities have been defined.
 * Higer number has higer priority. OS specific code can map them
 * to OS defined priority accordingly.
 */
#define OS_TASK_PRIO_5 5
#define OS_TASK_PRIO_6 6
#define OS_TASK_PRIO_7 7
#define OS_TASK_PRIO_8 8
#define OS_TASK_PRIO_9 9

#define OS_TASK_PRIO_HIGHEST OS_TASK_PRIO_9
#define OS_TASK_PRIO_LOWEST  OS_TASK_PRIO_5

#define TASK_MAX_PRIORITY   15

#ifdef DEBUG_STACK_CHECK
/**
 * @brief This function-like macro is used to set up variable for checking a function's stack usage size.
 */
#define FUNCTION_STACK_CHECK_START()                           \
do {                                                           \
    uint32_t *_current_sp = (uint32_t *)cpu_get_sp_register(); \
    uint32_t *_p_watermark_prev = os_task_get_stack_watermark(NULL)
/**
 * @brief This function-like macro is used to check a function's stack usage size.
 * @param x is the stack usage threshold for a function that can use at maximum.(unit:word)
 */
#define FUNCTION_STACK_CHECK_END(x)                                         \
    uint32_t *_p_watermark_after = os_task_get_stack_watermark(NULL);       \
    if (_p_watermark_after < _p_watermark_prev) {                           \
        assert((uint32_t)(_current_sp - _p_watermark_after) < (uint32_t)x); \
    }                                                                       \
}while (0)
#else
#define FUNCTION_STACK_CHECK_START()
#define FUNCTION_STACK_CHECK_END(x)
#endif

/**
 * @brief This function is used to create a task.
 *
 * @param fn is the function pointer of the task.
 * @param arg is the parameter passed to the function call.
 * @param prio is task priority.
 * @param stack_size is the required stack size for this task. if set to 0,
 *                   default stack size will be used. unit is 4 bytes.
 * @param name is task name string.
 * @param core_id is core number.
 * @return os_task_h NULL(failure), otherwise(task handle).
 */
os_task_h os_create_task_smp_ext(os_task_func_t fn, void *arg, uint8_t prio, uint32_t stack_size,
                                 const char *name, uint8_t core_id);

/**
 * @brief This function is used to create a task.
 *
 * @param fn is the function pointer of the task.
 * @param arg is the parameter passed to the function call.
 * @param prio is the task priority.
 * @param stack_size: is the required stack size for this task. if set to 0,
 *                    default stack size will be used. unit is 4 bytes.
 * @param name is task name string.
 * @return NULL(failure), otherwise(task handle).
 */
os_task_h os_create_task_ext(os_task_func_t fn, void *arg, uint8_t prio, uint32_t stack_size,
                             const char *name);

/**
 * @brief This function is used to create a task.
 *
 * @param fn is the function pointer of the task.
 * @param arg is the parameter passed to the function call.
 * @param prio is the  task priority.
 * @return NULL(failure), otherwise(task handle).
 */
#define os_create_task(fn, arg, prio) os_create_task_ext(fn, arg, prio, 0, __FUNCTION__)

/**
 * @brief This function is used to delete a task.
 *
 * @param handle is the handle of the task to be deleted. Passing NULL will
 *               cause the calling task to be deleted.
 */
void os_delete_task(os_task_h handle);

/**
 * @brief This function is used to set priority of a task.
 *
 * @param handle is the handle of the task to be set.
 * @param prio is the task priority.
 */
void os_set_task_prio(os_task_h handle, uint8_t prio);

/**
 * @brief This function is used to set task event.
 *
 * @param handle is the handle of the task to be set.
 */
void os_set_task_event(os_task_h handle);

/**
 * @brief This function is used to wait for task event.
 *        a task can call this funtion to wait for others to
 *        call os_set_task_event or os_set_task_event_with_v to wake it up.
 */
void os_wait_task_event(void);

/**
 * @brief This function is used to set task event with value transferred
 *        to the task. if the task haven't gotten the value through
 *        os_wait_task_event_with_v yet, then value of multiple calls will be OR-ed.
 *
 * @param handle is the handle of the task to be set.
 * @param v is the value to be transferred.
 */
void os_set_task_event_with_v(os_task_h handle, uint32_t v);

/**
 * @brief This function is used to set task event with value
 *        transferred to the task from ISR context. if the task haven't gotten the
 *        value through os_wait_task_event_with_v yet, then value of multiple calls
 *        will be OR-ed. Note that this function can only be called from ISR context.
 *
 * @param handle is the handle of the task to be set.
 * @param v is the value to be transferred.
 */
void os_set_task_event_with_v_from_isr(os_task_h handle, uint32_t v);

/**
 * @brief This function is used to wait task event and get the value.
 *        a task can call this function to wait for others to call os_set_task_event
 *        or os_set_task_event_with_v to wake it up. in addition, the value set by
 *        os_set_task_event_with_v will be returned.
 *
 * @param time_to_wait is time to wait before timeout.
 * @return uint32_t the received value from os_set_task_event_with_v.
 */
uint32_t os_wait_task_event_with_v(uint32_t time_to_wait);

/**
 * @brief This function is used to get the count of tasks.
 *
 * @return uint32_t the count of all tasks.
 */
uint32_t os_get_total_task_cnt(void);

/**
 * @brief This function is used to get task information.
 *
 * @param[out] tasks_to_get is the count of tasks we ready for get.
 * @param[out] task_time is the buf to store task info.
 * @return task_info_t count of tasks we got actually.
 */
task_info_t *os_get_task_info(uint32_t *tasks_to_get, uint32_t *task_time);

/**
 * @brief This function is used to get latest 5s' CPU utilization percentage.
 *
 * @return uint32_t get CPU utilization percentage * 100.
 */
uint32_t os_get_cpu_utilization(void);

/**
 * @brief This function is used to get memory utilization percentage.
 *
 * @return uint32_t get memory utilization percentage * 100.
 */
uint32_t os_get_mem_utilization(void);

/**
 * @brief This function is used to atomically check and set a memory location.
 *
 * @param tagert is the pointer to the memory location.
 * @param old_value is check if *tagert == old_value.
 *                    If does, set value to the *tagert.
 * @param value is the new value to set.
 * @return int 1(successfully), 0(failed).
 */
int os_atomic_check_set(int *tagert, int old_value, int value);

/**
 * @brief This function is used to atomically add a value onto a memory location.
 *
 * @param tagert : Pointer to the memory location.
 * @param value : The value to be added.
 * @return int 1(successfully), 0(failed).
 */
int os_atomic_add(int *tagert, int value);

/**
 * @brief This function is used to atomically get the value of a memory location.
 *
 * @param tagert is the pointer to the memory location.
 * @param value is the pointer to get the value.
 * @return int 1(successfully), 0(failed).
 */
int os_atomic_get(int *tagert, int *value);

/**
 * @brief This function is used to get current task handle.
 *
 * @return os_task_h task handle.
 */
os_task_h os_get_current_task_handle(void);

/**
 * @brief This function is used to get task's task number.
 *
 * @param handle is task's handle.
 * @return uint32_t task id.
 */
uint32_t os_get_task_id(os_task_h handle);

/**
 * @brief This function is used to current task yield and high priority task
 *        get scheduled.
 *
 */
void os_set_task_yield(void);

/**
 * @brief This function is used to start the FreeRtos.
 */
void os_start_kernel(void);

/**
 * @brief This function is used to start the scheduler.
 */
void os_start_scheduler(void);

/**
 * @brief This function is used to disable global interrupt.
 */
void os_disable_irq(void);

/**
 * @brief This function is used to enable global interrupt.
 */
void os_enable_irq(void);

/**
 * @brief This function is used to disable global interrupt.
 *
 * @return int is the mask that should pass into os_enable_irq_from_isr.
 */
int os_disable_irq_from_isr(void);

/**
 * @brief This function is used to enable global interrupt.
 *
 * @param mask is the mask got from os_disable_irq_from_isr.
 */
void os_enable_irq_from_isr(int mask);

/**
 * @brief This function is used to return OS scheduler's state.
 *
 * @return 0(not start), 1(started).
 */
int os_get_scheduler_state(void);

/**
 * @brief This function is used to OS's task context switch.
 */
void os_task_switch_context(void);

/**
 * @brief This function is used to init task list and defined task's priority.
 *
 * @param list is task list.
 * @param len is task element number.
 */
void os_task_init(const task_priority_t *list, uint32_t len);

/**
 * @brief This function is used to get all task's stack snap shot
 *
 * @param pxTaskSnapshotArray is the pointer to array of TaskSnapshot_t
*                            structures to store tasks snapshot data.
 * @param uxArraySize is the size of tasks snapshots array.
 * @param pxTcbSz is the pointer to store size of TCB.
 * @return uint32_t the number of elements stored in array.
 */
uint32_t os_task_snapshot_all(task_snapshot_t *const pxTaskSnapshotArray,
                              const uint32_t uxArraySize, uint32_t *pxTcbSz);

/**
 * @brief This function is used to get task's highest waternark of stack pointer.
 *
 * @param handle is task's handle.if current task,handle can be set to NULL.
 *
 * @return  Return the pointer point to the stack high water mark of a task given by the handle.
 */
uint32_t *os_task_get_stack_watermark(os_task_h handle);

/**
 * @brief This function is used to suspend all tasks, and this means disable task's scheduler.
 *
 */
void os_task_suspend_all(void);

/**
 * @brief This function is used to resume all tasks, and this means enable task's scheduler.
 *
 */
void os_task_resume_all(void);

/**
 * @brief This function is used to return kernel scheduler's state.
 *
 * @return bool_t is scheduler's state. If scheduler is running, return true, other wise return false.
 */
bool_t in_scheduling(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /* OS_SHIM_TASK_H */
