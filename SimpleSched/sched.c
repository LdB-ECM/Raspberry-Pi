#include <stddef.h>								// Needed for NULL
#include <stdbool.h>							// C standard needed for bool
#include <stdint.h>								// C standard for uint8_t, uint16_t, uint32_t etc
#include "sched.h"								// Include this units header

#ifdef _WIN32
#include <Windows.h>							// Needed for GetTickCount
#define timer_getTickCount GetTickCount64		// Windows tickcount64 function
#else
#include "rpi-SmartStart.h"						// Needed for getTickCount on RaspberryPi
#endif


typedef struct TaskDescriptor_s {
	void(*taskPointer) (void);					// Task function
	uint32_t userTasksInterval;					// Interval between each run on this task
	uint32_t plannedTask;						// Next time this task is set to run
	TASK_FLAGS taskFlags;						// Task flags
	struct TaskDescriptor_s *pTaskNext;			// Pointer to the next task in the run list
} TASK_DESCRIPTOR;


static uint8_t _initialized = false;
uint32_t _schedulerMs = 0;
static uint8_t _numberTasks = 0;
static TASK_DESCRIPTOR *pTaskSchedule = NULL;
static TASK_DESCRIPTOR *pTaskFirst = NULL;
static TASK_DESCRIPTOR task_desc[MAX_TASKS_NUMBER] = { 0 };
static uint64_t _lastTimer = 0;


void TaskInit(void)
{
	_initialized = true;
	_schedulerMs = 0;
	_lastTimer = timer_getTickCount();
	_numberTasks = 0;
	pTaskSchedule = NULL;
	pTaskFirst = NULL;
	for (int i = 0; i < MAX_TASKS_NUMBER; i++)						// For each entry in task desc block		
		task_desc[i].taskPointer = NULL;							// NULL the task pointer which is a flag for unused
}

TASKHANDLE AddTask (void(*userTask) (void),
					uint32_t taskInterval,
					TASK_FLAGS startFlags)
{
	TASKHANDLE ret = INVALID_TASK_HANDLE;
	TASK_DESCRIPTOR *pTaskDescriptor = NULL;
	TASK_DESCRIPTOR *pTaskWork = NULL;
	if ((_initialized == false) || (_numberTasks == MAX_TASKS_NUMBER)
		|| (userTask == NULL))
	{
		return ret;
	}

	if (taskInterval == 0)
	{
		taskInterval = 50; //50 ms by default
	}

	for (int i = 0; i < MAX_TASKS_NUMBER; i++)						// For each entry in task desc block	
		if (task_desc[i].taskPointer == NULL) {						// The task desc entry is unused as pointer is null
			pTaskDescriptor = &task_desc[i];						// Set the task description pointer to it
			ret = i;												// Set the return handle as desc array number
			break;													// Now break out
		}


	if (pTaskDescriptor != NULL)
	{
		if (pTaskFirst != NULL)
		{
			// Initialize the work pointer with the scheduler pointer (never mind is current state)
			pTaskWork = pTaskSchedule;
			// Research the last task from the circular linked list : so research the adresse of the
			// first task in testing the pTaskNext fields of each structure
			while (pTaskWork->pTaskNext != pTaskFirst)
			{
				// Set the work pointer on the next task
				pTaskWork = pTaskWork->pTaskNext;
			}
			pTaskWork->pTaskNext = pTaskDescriptor; // Insert the new task at the end of the circular linked list

			pTaskDescriptor->pTaskNext = pTaskFirst; // The next task is feedback at the first task
		}
		else
		{
			// There is no task in the scheduler, this task become the First task in the circular linked list
			pTaskFirst = pTaskDescriptor; // The pTaskFirst pointer is initialize with the first task of the scheduler
			pTaskSchedule = pTaskFirst; // Initialize the scheduler pointer at the first task
			pTaskDescriptor->pTaskNext = pTaskDescriptor; // The next task is itself because there is just one task in the circular linked list
		}
		// Common initialization for all tasks
		// no wait if the user wants the task up and running once added...
		//...otherwise we wait for the interval before to run the task
		pTaskDescriptor->plannedTask =
			_schedulerMs + ((startFlags.ImmediateStart) ? 0 : taskInterval);

		// Set the periodicity of the task
		pTaskDescriptor->userTasksInterval = taskInterval;
		// Set the task pointer on the task body
		pTaskDescriptor->taskPointer = userTask;
		// Clear the IMMEDIATESTART bit
		startFlags.ImmediateStart = 0;
		pTaskDescriptor->taskFlags = startFlags;

		_numberTasks++;

		return ret;
	}
	return INVALID_TASK_HANDLE;
}


bool RemoveTask (TASKHANDLE taskhandle)
{
	TASK_DESCRIPTOR *pTaskDescriptor = NULL;
	TASK_DESCRIPTOR *pTaskCurr = NULL;

	if ((_initialized == false) || (_numberTasks == 0) ||
		(taskhandle == INVALID_TASK_HANDLE))
	{
		return false;
	}

	pTaskDescriptor = &task_desc[taskhandle];						// Fetch the task description pointer


																	// Initialize the work pointer with the scheduler pointer (never mind is current state)
	pTaskCurr = pTaskFirst;

	while (pTaskCurr->pTaskNext != pTaskDescriptor)
	{
		// Set the work pointer on the next task
		pTaskCurr = pTaskCurr->pTaskNext;
	}

	if (pTaskCurr->pTaskNext == pTaskFirst)
	{
		pTaskFirst = pTaskCurr->pTaskNext->pTaskNext;
	}
	else
	{
		pTaskCurr->pTaskNext = pTaskDescriptor->pTaskNext;
	}

	pTaskDescriptor->taskPointer = NULL;

	_numberTasks--;

	return true;
}


/**
* Scheduling. This runs the kernel itself.
*/
void RunTaskScheduler(void)
{
	while (1)
	{
		if (pTaskSchedule != NULL && _numberTasks != 0)
		{
			//the task is running
			if (pTaskSchedule->taskFlags.Raw32 != 0) // If n bits set task is paused
			{
				//this trick overrun the overflow of _counterMs
				if ((int32_t)(_schedulerMs - pTaskSchedule->plannedTask) >= 0)
				{
					if (pTaskSchedule->taskFlags.PauseAfterTaskRun)
					{
						pTaskSchedule->taskPointer(); //call the task
						pTaskSchedule->taskFlags.PauseAfterTaskRun = 0; // Clear pause flag
					}
					else
					{
						//let's schedule next start
						pTaskSchedule->plannedTask =
							_schedulerMs + pTaskSchedule->userTasksInterval;

						pTaskSchedule->taskPointer(); //call the task
					}
				}
			}
			// If a task has called the function DeleteAllTask() and if no
			// task are added, the pointer is null
			if (pTaskSchedule != NULL)
			{
				// Set the scheduler pointer on the next task
				pTaskSchedule = pTaskSchedule->pTaskNext;
			}
		}

		uint64_t temp = _lastTimer;
		_lastTimer = timer_getTickCount();
		_schedulerMs += (_lastTimer - temp);

	}
}


