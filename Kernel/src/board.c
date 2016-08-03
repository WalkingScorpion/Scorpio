#include "Scorpio.h"
#include "stm32f10x.h"
#define PSP_SIZE 1024

void sys_tick_init(void);
void pendSV_init(void);
void interrupt_init(void);
void __psp_init(u16 *tmp);
extern u16 __psp;


void board_init(void)
{
	interrupt_init();
	sys_tick_init();
	pendSV_init();
}
void interrupt_init(void)
{
	/* 4 bits for pre-emption priority
     0 bits for subpriority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}

void sys_tick_start(void)
{
	*((u32 *)(0xE000E010)) |= 0x00000001;
}
void sys_tick_stop(void)
{
	*((u32 *)(0xE000E010)) &= 0xFFFFFFFE;
}
void sys_tick_it_enable(void)
{
	*((u32 *)(0xE000E010)) |= 0x00000002;
}
void sys_tick_it_disable(void)
{
	*((u32 *)(0xE000E010)) &= 0xFFFFFFFD;
}
void sys_tick_setreload(u32 index)
{
	*((u32 *)(0xE000E014)) = index;
}
void sys_tick_clear(void)
{
	*((u32 *)(0xE000E018)) = 0;
}

void sys_tick_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = (u8)SysTick_IRQn;	 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = SYS_TICK_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//SYS_TICK时钟源72M
	*((u32 *)(0xE000E010)) |= 0x00000004;
	//重装值设定
	sys_tick_setreload(SYS_CLK/1000*THREAD_SWITCH_PERIOD);
}

void pendSV_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = (u8)PendSV_IRQn;	 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PendSV_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void psp_init(void)
{
	__psp_init(&__psp);
}


