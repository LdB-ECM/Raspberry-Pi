#include <stdint.h>
#include "rpi-SmartStart.h"
#include "emb-stdio.h"
#include "windows.h"
#include "mmu.h"

static uint32_t check_sem = 0;
static uint32_t check_hello = 0;
static volatile uint32_t hellocount = 0;

void xSchedule(void)
{

}

void xTickISR(void)
{

}

void Check_Semaphore (void) 
{
	printf("Core %u checked table semaphore\n", getCoreID());
	semaphore_give(&check_sem);
}

void Core_SayHello(void)
{
	semaphore_take(&check_hello);
	printf("Core %u says hello ... waiting 5 seconds\n", getCoreID());
                  timer_wait(5000000);
	hellocount++;
	semaphore_give(&check_hello);
}

static volatile unsigned int CoreWithMMUOn = 0;
void MMU_enable_count(void)
{
	MMU_enable();
	CoreWithMMUOn++;
}

static const char Spin[4] = { '|', '/', '-', '\\' };
void main(void)
{
	Init_EmbStdio(WriteText);										// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed

	/* Setup translation tables with Core 0 */
	MMU_setup_pagetable();

    /* setup mmu on core 0 */
	MMU_enable();

	/* setup mmu on core 1 */
	printf("Setting up MMU on core 1\n");
	CoreExecute(1, MMU_enable_count);
	while (CoreWithMMUOn != 1) {};

	/* setup mmu on core 2 */
	printf("Setting up MMU on core 2\n");
	CoreExecute(2, MMU_enable_count);
	while (CoreWithMMUOn != 2) {};

	/* setup mmu on core 3 */
	printf("Setting up MMU on core 3\n");
	CoreExecute(3, MMU_enable_count);
	while (CoreWithMMUOn != 3) {};

	// Dont print until table load done
	printf("The cores have all started their MMU\n");

	printf("MMU online check semaphore .. print order should be core 1,2,3\n");
	semaphore_take(&check_sem);
	CoreExecute(1, Check_Semaphore);
	semaphore_take(&check_sem);
	CoreExecute(2, Check_Semaphore);
	semaphore_take(&check_sem);
	CoreExecute(3, Check_Semaphore);
	semaphore_take(&check_sem);  // need to wait for check to finish writing to screen
	semaphore_give(&check_sem); // lets be pretty and release sem
	printf("Testing semaphore hold ability\n");
	semaphore_take(&check_hello); // lock hello semaphore
	CoreExecute(1, Core_SayHello);
	CoreExecute(2, Core_SayHello);
	CoreExecute(3, Core_SayHello);
	printf("Cores queued waiting 10 seconds to release\n");

	int i = 0;
	uint64_t endtime = timer_getTickCount64() + 10000000ul;
	while (timer_getTickCount64() < endtime) {
		printf("waiting %c\r", Spin[i]);
		timer_wait(50000);
		i++;
		i %= 4;
	}
	semaphore_give(&check_hello); // release hello semaphore
	while (hellocount != 3) {};
	printf("Core print above could be in any order\n");
	printf("\ntest all done ... now deadlooping stop\n");
	i = 0;
	while (1) {
		printf("Deadloop %c\r", Spin[i]);
		timer_wait(50000);
		i++;
		i %= 4;
	}

}
