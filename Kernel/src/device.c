#include "Scorpio.h"
#include	<string.h>

scp_deviceTable_t dev_table={0,NULL,NULL};
scp_sem_t dev_table_sem={1,NULL};

extern scp_thread_t currentThread;

scp_device_t device_register(scp_device_t dev)
{
	scp_device_t dev_object,tmp_dev;
	int i;
	if(dev->device_class==KERNEL_MODE)
		if(0!=__cpu_mode_ensure())
			return NULL;
		
	if(strlen((const char *)(dev->name)) >= DEV_NAME_MAX_LENGTH - 1)
		return NULL;

	wait(dev_table_sem);
	if (dev_table.number >=DEV_MAX_NUM){
		signal(dev_table_sem);
		return NULL;
	}
	for(tmp_dev = dev_table.head;(0!=strcmp((const char *)(tmp_dev->name),(const char *)(dev->name)))&&(tmp_dev!=NULL);\
																	tmp_dev = tmp_dev->next);
	if(tmp_dev != NULL){
		signal(dev_table_sem);
		return NULL;
	}
	if(NULL==(dev_object=(scp_device_t)scp_malloc(sizeof(struct device)))){
		signal(dev_table_sem);
		return NULL;
	}
	
	*dev_object=*dev;
	dev_object->parent_thread = currentThread;
	if(currentThread!=NULL){
		if(currentThread->first_child_deivce == NULL){
			currentThread->first_child_deivce = dev_object;
		}else{
			for(tmp_dev=currentThread->first_child_deivce;tmp_dev->next_brother!=NULL;tmp_dev=tmp_dev->next_brother);
			tmp_dev->next_brother = dev_object;
		}
	}
	
	for(tmp_dev = dev_table.head,i = 1;(i == tmp_dev->id )&&(tmp_dev!=NULL);tmp_dev = tmp_dev->next,i++);
	dev_object->id = i ; 
	dev_object->next = tmp_dev;
	if(tmp_dev!=NULL){
		dev_object->prev = tmp_dev ->prev;
		tmp_dev->prev = dev_object;
			
		if(tmp_dev->prev == NULL)
				dev_table.head = dev_object;
		else
			dev_object->prev->next = dev_object;
		
	}else{
		if(dev_table.tail!=NULL){
			dev_table.tail->next =	dev_object;
			dev_object->prev = dev_table.tail;
			dev_table.tail = dev_object;
		}
		else{
			dev_table.tail = dev_object;
			dev_table.head = dev_object;
			dev_object->prev = dev_object->next = NULL;
		}
	}
	
	dev_table.number++;
	signal(dev_table_sem);
	return dev_object;
}

s32 device_cancel_withName(void *name)
{
	scp_device_t tmp_dev;
	wait(dev_table_sem);
	for(tmp_dev = dev_table.head;(0!=strcmp((const char *)(tmp_dev->name),(const char *)name))&&(tmp_dev!=NULL);\
																	tmp_dev = tmp_dev->next);
	if(tmp_dev==NULL){
		signal(dev_table_sem);
		return ERROR1;
	}else{
		signal(dev_table_sem);
		return device_cancel_withID(tmp_dev->id);
	}
}


s32 device_cancel_withID(u8 id)
{
	scp_device_t tmp_dev;
	wait(dev_table_sem);
	for(tmp_dev = dev_table.head;(tmp_dev->id != id)&&(tmp_dev!=NULL);tmp_dev = tmp_dev->next);
	if(tmp_dev==NULL){
		signal(dev_table_sem);
		return ERROR1;
	}else{
		
		if(tmp_dev->prev==NULL)
			dev_table.head=tmp_dev->next;
		else
			tmp_dev->prev->next = tmp_dev->next;

		if(tmp_dev->next==NULL)
			dev_table.tail=tmp_dev->prev;
		else
			tmp_dev->next->prev = tmp_dev->prev;
		
		while(SUCC!=scp_free(tmp_dev));
		signal(dev_table_sem);
		return SUCC;
	}
}

scp_device_t 	device_find_withID(u8 id)
{
	scp_device_t tmp_dev;
	wait(dev_table_sem);
	for(tmp_dev = dev_table.head;(tmp_dev->id != id)&&(tmp_dev!=NULL);tmp_dev = tmp_dev->next);
	signal(dev_table_sem);
	return tmp_dev;
	
}

scp_device_t device_find_withName(void *name)
{
	scp_device_t tmp_dev;
	wait(dev_table_sem);
	for(tmp_dev = dev_table.head;(0!=strcmp((const char *)(tmp_dev->name),(const char *)name))&&(tmp_dev!=NULL);\
																	tmp_dev = tmp_dev->next);
	signal(dev_table_sem);
	return tmp_dev;
	
}

