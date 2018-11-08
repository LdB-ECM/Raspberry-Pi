#ifndef _SCHED_									// If guard is not defined
#define _SCHED_									// Define guard to stop multiple inclusion

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

#include <stdbool.h>							// C standard needed for bool
#include <stdint.h>								// C standard for uint8_t, uint16_t, uint32_t etc

extern uint32_t _schedulerMs;

#define MAX_TASKS_NUMBER            255

typedef union {
	struct {
		unsigned RunTaskEachElapseTime : 1;							// @0	Run the task each elapsed time
		unsigned PauseAfterTaskRun : 1;								// @1	Pause after task is next run
		unsigned ImmediateStart : 1;								// @2	Immediately start the task
		unsigned _reserved3_31 : 29;								// @3	Reserved bits
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t						
} TASK_FLAGS;

#define TASK_SCHEDULED ((TASK_FLAGS) {.RunTaskEachElapseTime = 1})
#define TASK_IMMEDIATESTART ((TASK_FLAGS) {.RunTaskEachElapseTime = 1, .ImmediateStart = 1})
#define TASK_ONESHOT ((TASK_FLAGS) {.PauseAfterTaskRun = 1})
#define TASK_ONESHOT_IMMEDIATE ((TASK_FLAGS) {.PauseAfterTaskRun = 1, .ImmediateStart = 1})

typedef uint32_t TASKHANDLE;
#define INVALID_TASK_HANDLE 0xFFFFFFFF



void TaskInit (void);

TASKHANDLE AddTask (void (*userTask) (void),
					uint32_t taskInterval,
					TASK_FLAGS startFlags);

bool RemoveTask (TASKHANDLE taskhandle);

void RunTaskScheduler (void);



#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif
