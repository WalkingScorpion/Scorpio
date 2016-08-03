#include "Scorpio.h"

void system_initial(void);
void user_mode_start(void);
void thread_start(void);
void system_thread_create(void);
void user_thread_create(void);


//在此处添加用户任务
void user_thread_create(void)
{
	
}

//在此处添加内核任务
void system_thread_create(void)
{
	thread_create("CONSOLE",KERNEL_MODE,0,1024,0,console_thread_entry);
	thread_create("LED",KERNEL_MODE,2000000,256,0,flow_led);
}

void startup(void)
{
#ifdef DEBUG
	debug_init();
#endif
	board_init();
	system_initial();
	__goto_psp_mode();
	system_thread_create();
	__goto_user_mode();
	user_thread_create();
	thread_start();
	
	while(1);
}

void system_initial(void)
{
	mem_init();
	sys_thread_init();
	psp_init();
}

void thread_start(void)
{
	__SVC_ASK(0);
}

int main(void)
{
	startup();
}
