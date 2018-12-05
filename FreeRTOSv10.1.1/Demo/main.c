#include <stdint.h>
#include <string.h>
#include "rpi-smartstart.h"
#include "emb-stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rpi-irq.h"
#include "semphr.h"


void DoProgress(char label[], int step, int total, int x, int y, COLORREF col)
{
	//progress width
	const int pwidth = 72;
	int len = strlen(label);

	//minus label len
	int width = (pwidth - len)*BitFontWth;
	int pos = (step * width) / total;

	int percent = (step * 100) / total;

	GotoXY(x, y);
	printf("%s[", label);

	//fill progress bar with =
	//COLORREF orgPen = SetDCPenColor(0, col);
	COLORREF orgBrush = SetDCBrushColor(0, col);
	Rectangle(0, (len + 1)*BitFontWth, y*BitFontHt + 2, pos + (len + 1)*BitFontWth, (y + 1)*BitFontHt - 4);

	//SetDCPenColor(0, 0);
	SetDCBrushColor(0, 0);
	Rectangle(0, pos+(len + 1)*BitFontWth, y*BitFontHt + 2, (pwidth + 1)*BitFontWth, (y + 1)*BitFontHt - 4);

	//SetDCPenColor(0, orgPen);
	SetDCBrushColor(0, orgBrush);

	GotoXY(pwidth + 1, y);
	printf("] %3i%%\n", percent);
}


static xSemaphoreHandle barSemaphore = { 0 };

void task1(void *pParam) {
	COLORREF col = 0xFFFF0000;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0)) dir = -dir;

		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{

			DoProgress("HARE  ", step, total, 0, 10, col);

			/* We have finished accessing the shared resource.  Release the
			semaphore. */
			xSemaphoreGive(barSemaphore);
		}
		vTaskDelay(10);
	}
}

void task2(void *pParam) {
	COLORREF col = 0xFF0000FF;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0)) dir = -dir;
		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{

			DoProgress("TURTLE", step, total, 0, 12, col);

			/* We have finished accessing the shared resource.  Release the
			semaphore. */
			xSemaphoreGive(barSemaphore);
		}
		vTaskDelay(35);

	}
}


static bool lit = false;
void task3(void *pvParameters)
{
	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;; )
	{
		/* Print out the name of this task AND the number of times ulIdleCycleCount has been incremented. */
		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{
			GotoXY(0, 14);
			printf("Cpu usage: %u%%    FreeRTOS: %s\n", xLoadPercentCPU(), tskKERNEL_VERSION_NUMBER);
			xSemaphoreGive(barSemaphore);
		}
		if (lit) lit = false; else lit = true;							// Flip lit flag
		set_Activity_LED(lit);											// Turn LED on/off as per new flag
		vTaskDelay(configTICK_RATE_HZ);
	}
}

void task4 (void *pParam) {
	char buf[256];
	while (1) 
	{
		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{
			GotoXY(0, 15);
			printf("[Task]       [State]  [Prio]  [Stack] [Num]\n");
			vTaskList(&buf[0]);
			printf(&buf[0]);
			printf("\n");


			/* We have finished accessing the shared resource.  Release the
			semaphore. */
			xSemaphoreGive(barSemaphore);
		}
		vTaskDelay(10000);
	}
}


void main (void)
{
	Init_EmbStdio(WriteText);										// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed
	printf("Task tick rate: %u\n", configTICK_RATE_HZ);
	/* Attempt to create a semaphore. */
	vSemaphoreCreateBinary(barSemaphore);

	if (barSemaphore != NULL)
	{
		/* The semaphore can now be used. Its handle is stored in the
		xSemahore variable.  Calling xSemaphoreTake() on the semaphore here
		will fail until the semaphore has first been given. */
		xSemaphoreGive(barSemaphore);
	}

	xTaskCreate(task1, "HARE  ", 2048, NULL, 4, NULL);
	xTaskCreate(task2, "TURTLE", 2048, NULL, 4, NULL);
	xTaskCreate(task3, "TIMER ", 2048, NULL, 3, NULL);
	xTaskCreate(task4, "DETAIL", 2048, NULL, 2, NULL);

	vTaskStartScheduler();
	/*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
	while (1) {
	}
}


