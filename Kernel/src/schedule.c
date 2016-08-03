#include "Scorpio.h"
u8 thread_switch_lock = 0;
u8 interrupt_lock = 0;
extern scp_threadTable_t threadTable[THREAD_STATUS_NUM];
extern scp_thread_t currentThread;
/*
parameter
	0:		Directly switch to the very next RUNNING thread without current thread to be store into its stack.
				Normally,this option is used when the schedule start from hardware reset,which is the very beginning of the system,
			or switch from a already DEAD thread.
	1:		Directly switch to the very next RUNNING thread with current thread to be store into its stack.
				This is the most often used option,which is present as the switch between the current thread and the next coming 
			running thread.		
*/
u32 thread_schedule(u8 parameter)
{
	scp_thread_t thread;
	if(thread_switch_lock==0){
		if(parameter==0){
			currentThread = threadTable[RUNNING].head;
			__switch_to(&(threadTable[RUNNING].head->thread_sp));
		}else if(parameter==1){
			if(currentThread->thread_status == RUNNING){
				thread_table_roll(RUNNING);
			}
			thread = currentThread;
			currentThread = threadTable[RUNNING].head;
			__switch_between(&(thread->thread_sp),&(currentThread->thread_sp));
		}	
	}
	return (u32)(currentThread->thread_status);
}
