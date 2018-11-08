#include <stdbool.h>							// C standard needed for bool
#include <stdint.h>								// C standard for uint8_t, uint16_t, uint32_t etc
#include <stdio.h>
#include "sched.h"								// Include shecudler unit header

#ifndef _WIN32
#include "rpi-SmartStart.h"
#include "emb-stdio.h"
#endif

void Task1(void)
{
	printf("Task1 ran at time %lu msec\n", _schedulerMs);
}

void Task2(void)
{
	printf("Task2 ran at time %lu msec\n", _schedulerMs);
}

int main(void) {
	#ifndef _WIN32
	Init_EmbStdio(Embedded_Console_WriteChar);							// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed no message to screen
	#endif

	TaskInit();

	AddTask(&Task1, 5000, TASK_SCHEDULED);
	AddTask(&Task2, 9000, TASK_IMMEDIATESTART);

	RunTaskScheduler();

	return (0);
}