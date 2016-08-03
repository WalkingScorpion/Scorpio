#include "Scorpio.h"
extern u8 thread_switch_lock;
extern scp_thread_t currentThread;

void wait(scp_sem_t sem)
{
	u32 interrupt;
	interrupt=__interrupt_disable();
	thread_switch_lock = 1;
	sem.value--;
	if(sem.value >= 0){
		thread_switch_lock = 0;
		__interrupt_enable(interrupt);
		return;
	}else{
		thread_table_convert(currentThread,SUSPENDING);
		sem_table_insert(currentThread,sem.sem_thread_head);
		thread_switch_lock = 0;
		__interrupt_enable(interrupt);
		__SVC_ASK(1);//never come back
	}	
}
void signal(scp_sem_t sem)
{
	int i;
	u32 interrupt;
	interrupt=__interrupt_disable();
	thread_switch_lock = 1;
	sem.value++;
	if((sem.value>0) && (sem.sem_thread_head !=NULL)){
		for(i=0;i<sem.value;i++){
			thread_table_convert((scp_thread_t)(sem.sem_thread_head),RUNNING);
			sem_table_remove((scp_thread_t)(sem.sem_thread_head),sem.sem_thread_head);
		}
	}
	thread_switch_lock = 0;
	__interrupt_enable(interrupt);
}

void sem_table_insert(void *thread,volatile void *head)
{
	scp_thread_t tmp_thread,inserted_thread,table_head;
	inserted_thread = (scp_thread_t)thread;
	table_head = (scp_thread_t)head;
	if(table_head == NULL){
		table_head = inserted_thread;
		inserted_thread->sem_next = NULL;
	}else{
		for(tmp_thread = table_head;tmp_thread->sem_next!=NULL;tmp_thread = tmp_thread->sem_next);
		tmp_thread->sem_next = inserted_thread;
		inserted_thread->sem_next = NULL;
	}
}
void sem_table_remove(void *thread,volatile void *head)
{
	scp_thread_t tmp_thread,removed_thread,table_head;
	removed_thread = (scp_thread_t)thread;
	table_head = (scp_thread_t)head;
	if(table_head == NULL){
		return;
	}else if(table_head == removed_thread){
		table_head = removed_thread->sem_next;
	}else{
		for(tmp_thread = table_head;(tmp_thread ->sem_next!=NULL)&&(tmp_thread ->sem_next!=removed_thread);
				tmp_thread = tmp_thread->sem_next);
		if(tmp_thread->sem_next!=NULL){
			tmp_thread->sem_next = tmp_thread->sem_next->sem_next;
		}
	}
}
