#include <stdbool.h>
#include "rpi-SmartStart.h"
#include "emb-stdio.h"


/*--------------------------------------------------------------------------}
{    LOCAL TIMER INTERRUPT ROUTING REGISTER - QA7_rev3.4.pdf page 18		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		enum {
			LOCALTIMER_TO_CORE0_IRQ = 0,
			LOCALTIMER_TO_CORE1_IRQ = 1,
			LOCALTIMER_TO_CORE2_IRQ = 2,
			LOCALTIMER_TO_CORE3_IRQ = 3,
			LOCALTIMER_TO_CORE0_FIQ = 4,
			LOCALTIMER_TO_CORE1_FIQ = 5,
			LOCALTIMER_TO_CORE2_FIQ = 6,
			LOCALTIMER_TO_CORE3_FIQ = 7,
		} Routing: 3;												// @0-2	  Local Timer routing 
		unsigned unused : 29;										// @3-31
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} local_timer_int_route_reg_t;

/*--------------------------------------------------------------------------}
{    LOCAL TIMER CONTROL AND STATUS REGISTER - QA7_rev3.4.pdf page 17		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned ReloadValue : 28;									// @0-27  Reload value 
		unsigned TimerEnable : 1;									// @28	  Timer enable (1 = enabled) 
		unsigned IntEnable : 1;										// @29	  Interrupt enable (1= enabled)
		unsigned unused : 1;										// @30	  Unused
		unsigned IntPending : 1;									// @31	  Timer Interrupt flag (Read-Only) 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} local_timer_ctrl_status_reg_t;

/*--------------------------------------------------------------------------}
{     LOCAL TIMER CLEAR AND RELOAD REGISTER - QA7_rev3.4.pdf page 18		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned unused : 30;										// @0-29  unused 
		unsigned Reload : 1;										// @30	  Local timer-reloaded when written as 1 
		unsigned IntClear : 1;										// @31	  Interrupt flag clear when written as 1  
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} local_timer_clr_reload_reg_t;

/*--------------------------------------------------------------------------}
{    GENERIC TIMER INTERRUPT CONTROL REGISTER - QA7_rev3.4.pdf page 13		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned nCNTPSIRQ_IRQ : 1;									// @0	Secure physical timer event IRQ enabled, This bit is only valid if bit 4 is clear otherwise it is ignored. 
		unsigned nCNTPNSIRQ_IRQ : 1;								// @1	Non-secure physical timer event IRQ enabled, This bit is only valid if bit 5 is clear otherwise it is ignored
		unsigned nCNTHPIRQ_IRQ : 1;									// @2	Hypervisor physical timer event IRQ enabled, This bit is only valid if bit 6 is clear otherwise it is ignored
		unsigned nCNTVIRQ_IRQ : 1;									// @3	Virtual physical timer event IRQ enabled, This bit is only valid if bit 7 is clear otherwise it is ignored
		unsigned nCNTPSIRQ_FIQ : 1;									// @4	Secure physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (0) 
		unsigned nCNTPNSIRQ_FIQ : 1;								// @5	Non-secure physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (1)
		unsigned nCNTHPIRQ_FIQ : 1;									// @6	Hypervisor physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (2)
		unsigned nCNTVIRQ_FIQ : 1;									// @7	Virtual physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (3)
		unsigned reserved : 24;										// @8-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} generic_timer_int_ctrl_reg_t;


/*--------------------------------------------------------------------------}
{       MAILBOX INTERRUPT CONTROL REGISTER - QA7_rev3.4.pdf page 14			}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned Mailbox0_IRQ  : 1;									// @0	Set IRQ enabled, This bit is only valid if bit 4 is clear otherwise it is ignored. 
		unsigned Mailbox1_IRQ : 1;									// @1	Set IRQ enabled, This bit is only valid if bit 5 is clear otherwise it is ignored
		unsigned Mailbox2_IRQ : 1;									// @2	Set IRQ enabled, This bit is only valid if bit 6 is clear otherwise it is ignored
		unsigned Mailbox3_IRQ : 1;									// @3	Set IRQ enabled, This bit is only valid if bit 7 is clear otherwise it is ignored
		unsigned Mailbox0_FIQ : 1;									// @4	Set FIQ enabled, If set, this bit overrides the IRQ bit (0) 
		unsigned Mailbox1_FIQ : 1;									// @5	Set FIQ enabled, If set, this bit overrides the IRQ bit (1)
		unsigned Mailbox2_FIQ : 1;									// @6	Set FIQ enabled, If set, this bit overrides the IRQ bit (2)
		unsigned Mailbox3_FIQ : 1;									// @7	Set FIQ enabled, If set, this bit overrides the IRQ bit (3)
		unsigned reserved : 24;										// @8-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mailbox_int_ctrl_reg_t;

/*--------------------------------------------------------------------------}
{		 CORE INTERRUPT SOURCE REGISTER - QA7_rev3.4.pdf page 16			}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned CNTPSIRQ : 1;										// @0	  CNTPSIRQ  interrupt 
		unsigned CNTPNSIRQ : 1;										// @1	  CNTPNSIRQ  interrupt 
		unsigned CNTHPIRQ : 1;										// @2	  CNTHPIRQ  interrupt 
		unsigned CNTVIRQ : 1;										// @3	  CNTVIRQ  interrupt 
		unsigned Mailbox0_Int : 1;									// @4	  Set FIQ enabled, If set, this bit overrides the IRQ bit (0) 
		unsigned Mailbox1_Int : 1;									// @5	  Set FIQ enabled, If set, this bit overrides the IRQ bit (1)
		unsigned Mailbox2_Int : 1;									// @6	  Set FIQ enabled, If set, this bit overrides the IRQ bit (2)
		unsigned Mailbox3_Int : 1;									// @7	  Set FIQ enabled, If set, this bit overrides the IRQ bit (3)
		unsigned GPU_Int : 1;										// @8	  GPU interrupt <Can be high in one core only> 
		unsigned PMU_Int : 1;										// @9	  PMU interrupt 
		unsigned AXI_Int : 1;										// @10	  AXI-outstanding interrupt <For core 0 only!> all others are 0 
		unsigned Timer_Int : 1;										// @11	  Local timer interrupt
		unsigned GPIO_Int : 16;										// @12-27 Peripheral 1..15 interrupt (Currently not used
		unsigned reserved : 4;										// @28-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} core_int_source_reg_t;


struct __attribute__((__packed__, aligned(4))) QA7Registers {
	local_timer_int_route_reg_t TimerRouting;						// 0x24
	uint32_t GPIORouting;											// 0x28
	uint32_t AXIOutstandingCounters;								// 0x2C
	uint32_t AXIOutstandingIrq;										// 0x30
	local_timer_ctrl_status_reg_t TimerControlStatus;				// 0x34
	local_timer_clr_reload_reg_t TimerClearReload;					// 0x38
	uint32_t unused;												// 0x3C
	generic_timer_int_ctrl_reg_t Core0TimerIntControl;				// 0x40
	generic_timer_int_ctrl_reg_t Core1TimerIntControl;				// 0x44
	generic_timer_int_ctrl_reg_t Core2TimerIntControl;				// 0x48
	generic_timer_int_ctrl_reg_t Core3TimerIntControl;				// 0x4C
	mailbox_int_ctrl_reg_t  Core0MailboxIntControl;					// 0x50
	mailbox_int_ctrl_reg_t  Core1MailboxIntControl;					// 0x54
	mailbox_int_ctrl_reg_t  Core2MailboxIntControl;					// 0x58
	mailbox_int_ctrl_reg_t  Core3MailboxIntControl;					// 0x5C
	core_int_source_reg_t Core0IRQSource;							// 0x60
	core_int_source_reg_t Core1IRQSource;							// 0x64
	core_int_source_reg_t Core2IRQSource;							// 0x68
	core_int_source_reg_t Core3IRQSource;							// 0x6C
	core_int_source_reg_t Core0FIQSource;							// 0x70
	core_int_source_reg_t Core1FIQSource;							// 0x74
	core_int_source_reg_t Core2FIQSource;							// 0x78
	core_int_source_reg_t Core3FIQSource;							// 0x7C
};

#define QA7 ((volatile __attribute__((aligned(4))) struct QA7Registers*)(uintptr_t)(0x40000024))


static bool lit = false;
void c_irq_handler (void)
{
	if (lit) lit = false; else lit = true;							// Flip lit flag
	set_Activity_LED(lit);											// Turn LED on/off as per new flag

	QA7->TimerClearReload.IntClear = 1;								// Clear interrupt
	QA7->TimerClearReload.Reload = 1;								// Reload now
}

void Core3_Deadloop(void) 
{
	EnableInterrupts();												// Start interrupts rolling on core3
	while(1) {}														// Hard deadloop
}


static const char Spin[4] = { '|', '/', '-', '\\' };

int main (void) {
	Init_EmbStdio(Embedded_Console_WriteChar);						// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed no message to screen

	setIrqFuncAddress(c_irq_handler);								// Set the address for our interrupt handler

	printf("Setting up Local Timer Irq to Core3\n");
	QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE3_IRQ;			// Route local timer IRQ to Core0

	QA7->TimerControlStatus.ReloadValue = 20000000;					// Timer period set
	QA7->TimerControlStatus.TimerEnable = 1;						// Timer enabled
	QA7->TimerControlStatus.IntEnable = 1;							// Timer IRQ enabled

	QA7->TimerClearReload.IntClear = 1;								// Clear interrupt
	QA7->TimerClearReload.Reload = 1;								// Reload now

	QA7->Core3TimerIntControl.nCNTPNSIRQ_IRQ = 1;					// We are in NS EL1 so enable IRQ to core0 that level
	QA7->Core3TimerIntControl.nCNTPNSIRQ_FIQ = 0;					// Make sure FIQ is zero

	CoreExecute(3, Core3_Deadloop);									// Tell core3 to execute the dealoop so it isn't asleep
	printf("Core3 is now simply deadlooping waiting for irq\n");

	printf("Activity LED should flash slowly now with core3 processing irq's\n");

	int i = 0;
	while (1) {
		printf("Core0 Deadloop %c\r", Spin[i]);
		timer_wait(50000);
		i++;
		i %= 4;
	}

	return(0);
}

