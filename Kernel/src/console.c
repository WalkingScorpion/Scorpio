
#include "Scorpio.h"
#include "stm32f10x.h"
#include <string.h>

scp_device_t console_dev;
scp_sem_t console_sem={1,NULL};


void console_thread_entry(void)
{ 
	console_init();
	Scorpio_logo();
	think_logo();
	while(1){
		think();
		wait(console_dev->dev_sem);
		wait(console_dev->data.tx_buffer_out_sem);
		for(;console_dev->data.tx_index_in != console_dev->data.tx_index_out;){
			USART_SendData(USART1,console_dev->data.tx_buffer[console_dev->data.tx_index_out++]);
			while( USART_GetFlagStatus(USART1,USART_FLAG_TC)!= SET);
			if(console_dev->data.tx_index_out >= DEV_BUFFER_SIZE)
				console_dev->data.tx_index_out = 0;
		}
		signal(console_dev->data.tx_buffer_out_sem);
		signal(console_dev->dev_sem);
	}
}

s32 console_init(void)
{
	console_dev=USART1_register();
	if(NULL == console_dev)
		return ERROR1;
	
	if(SUCC!=console_chose_withName("USART1"))
		return ERROR2;
	
	wait(console_sem);
	console_dev->device_init(console_dev);
	console_dev->device_open(console_dev);
	signal(console_sem);
	
	return SUCC;
}

s32 console_chose_withID(u8 id)
{
	scp_device_t dev;
	wait(console_sem);
	dev = device_find_withID(id);
	if(dev != NULL){
		console_dev = dev;
		signal(console_sem);
		return SUCC;
	}
	signal(console_sem);
	return ERROR1;
}

s32 console_chose_withName(void *name)
{
	scp_device_t dev;
	dev = device_find_withName(name);
	if(dev != NULL){
		wait(console_sem);
		console_dev =  dev;
		signal(console_sem);
		return SUCC;
	}
	return ERROR1;
}

void Scorpio_logo(void)
{
	printf(SCORPIO_LOGO);
}




