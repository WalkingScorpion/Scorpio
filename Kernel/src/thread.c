#include "Scorpio.h"
#include	<string.h>

u8 thread_swap[THREAD_SWAP_SIZE];
scp_threadTable_t threadTable[THREAD_STATUS_NUM];
scp_thread_t currentThread = NULL;
u8 thread_id_bitmap[THREAD_MAX_NUM];
scp_sem_t thread_id_bitmap_sem={1,NULL};
void sys_thread_init(void)
{
	u8 i=0;
	for(;i<5;i++){
		threadTable[i].number=0;
		threadTable[i].head=NULL;
		threadTable[i].tail=NULL;
	}
}

scp_thread_t thread_create(char *name, scp_threadClass_t thread_class, u32 flag, u32 thread_stack_size, 
														u8 thread_priority,void *tentry)
{
	scp_thread_t thread,tmp_thread;
	u16 i;
	if((0!=__cpu_mode_ensure())&&(thread_class == KERNEL_MODE))
		return NULL;
	if(strlen((const char *)name) >= THREAD_NAME_MAX_LENGTH - 1)
		return NULL;
	if( NULL == (thread=(scp_thread_t)scp_malloc(sizeof(struct scp_thread))))
		return NULL;
	if( NULL == (thread->thread_stack = (u32)scp_malloc(thread_stack_size)))
		return NULL;
	
	memset((u8 *)thread->thread_stack,'#',thread_stack_size);
	//id
	wait(thread_id_bitmap_sem);
	for(i=0;i < THREAD_MAX_NUM;i++){
		if(thread_id_bitmap[i]==0)
			break;
	}
	if(i < THREAD_MAX_NUM){
		thread->id=i;
		thread_id_bitmap[i]=1;
		signal(thread_id_bitmap_sem);
	}else{
		signal(thread_id_bitmap_sem);
		return NULL;
	}
	

	strcpy((char *)(thread->name),(const char *)name);
	thread->thread_class = thread_class;
	thread->flag=flag;
	thread->thread_stack_size=thread_stack_size;
	thread->thread_priority=thread_priority;
	thread->thread_stack=(u32)((u8 *)(thread->thread_stack) + thread_stack_size);
	thread->thread_sp = thread->thread_stack;
	thread->thread_status = RUNNING;
	thread->timer_info=0xFFFFFFFF;
	thread->swap_scope.signal_flag = 0;
	thread->swap_scope.thread_communiation_sem.value=1;
	thread->swap_scope.size=0;
	thread->swap_scope.offset=0;
	thread->parent = currentThread;
	thread->first_child = NULL;
	if(thread->parent != NULL){
		if(thread->parent->first_child == NULL){
			thread->parent->first_child =thread;
		}else{
			for(tmp_thread=thread->parent->first_child;tmp_thread->next_brother!=NULL;tmp_thread=tmp_thread->next_brother);
			tmp_thread->next_brother = thread;
		}
	}
	thread->next_brother = NULL;		
	thread->first_child_deivce=NULL;
	thread->sem_next=NULL;
	//stack init
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0x01000000L;	//PSR
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = (u32)tentry;	//pc
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = (u32)texit;	//lr
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r12
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r3
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r2
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r1
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = flag;						//r0
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r11
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r10
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r9
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;					 	//r8
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;					 	//r7
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r6
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;						//r5
	thread->thread_sp-=4;
	*((u32 *)(thread->thread_sp)) = 0;				//r4
	
	//table insert
	thread_table_insert(thread,RUNNING);
	return thread;
}

void texit(void)
{
	thread_table_convert(currentThread,DEAD);
	__SVC_ASK(1);//never come back
}

scp_thread_t get_thread_by_id(u8 id)
{
	u16 i;
	scp_thread_t tmp_thread = NULL;
	for(i=0;i<THREAD_STATUS_NUM;i++){
		for(tmp_thread=threadTable[i].head;tmp_thread!=NULL;tmp_thread=tmp_thread->next){
			if(tmp_thread->id == id)
				break;
		}
	}
	return tmp_thread;
}

void thread_table_insert(scp_thread_t thread,scp_thread_status_t status)
{
	if(threadTable[status].number == 0){
		threadTable[status].head = thread;
		threadTable[status].tail = thread;
		thread->next=NULL;
	}else{
		threadTable[status].tail->next=thread;
		thread->next = NULL;
		threadTable[status].tail = thread;
	}
	thread->thread_status = status;
	threadTable[status].number++;
}

void thread_table_remove(scp_thread_t thread)
{
	scp_thread_t tmp_thread ;
	if(thread == threadTable[thread->thread_status].head){
		threadTable[thread->thread_status].head = thread->next;
		threadTable[thread->thread_status].number--;
	}else{
		for(tmp_thread = threadTable[thread->thread_status].head;(tmp_thread->next!=thread)&&(tmp_thread->next!=NULL);
				tmp_thread=tmp_thread->next);
		if(tmp_thread==threadTable[thread->thread_status].tail){
			return;
		}else{
			tmp_thread->next = tmp_thread->next->next;
			threadTable[thread->thread_status].number--;
		}
	}
}
void thread_table_convert(scp_thread_t thread,scp_thread_status_t des_status)
{
	thread_table_remove(thread);
	thread_table_insert(thread,des_status);
}
void thread_table_roll(scp_thread_status_t status)
{
	scp_thread_t thread;
	if(threadTable[status].number!=0){
		thread=threadTable[RUNNING].head;
		thread_table_remove(threadTable[RUNNING].head);
		thread_table_insert(thread,RUNNING);
		
	}
}
