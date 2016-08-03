#include "stm32f10x.h"
#include "Scorpio.h"
#include <string.h>
#define USART1_INITIALED 0x01
#define USART1_OPENED		0x02
extern scp_device_t console_dev;
/*
 USART1初始化,波特率115200，单次8比特，无奇偶校验，1停止位
 */
s32 USART1_init(struct device * dev)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;	 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART1_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	wait(dev->dev_sem);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA| RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	  
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	dev->flag |= USART1_INITIALED;
	signal(dev->dev_sem);
	return SUCC;
}

s32 USART1_open(struct device * dev)
{
	wait(dev->dev_sem);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);
	dev->flag |= USART1_OPENED;
	signal(dev->dev_sem);
	return SUCC;
}
s32 USART1_close(struct device * dev)
{
	wait(dev->dev_sem);
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	USART_ClearFlag(USART1,USART_FLAG_TC); 
	USART_Cmd(USART1, DISABLE);
	dev->flag &= ~USART1_OPENED;
	signal(dev->dev_sem);
	return SUCC;
}
s32 USART1_read(struct device * dev,u16 length,void *bffer)
{
	int i,tmp;
	u8 *buffer;
	buffer = (u8 *)bffer;

	wait(dev->dev_sem);
	wait(dev->data.rx_buffer_out_sem);
	if(dev->data.rx_index_out <= dev->data.rx_index_in){
		if(dev->data.rx_index_in - dev->data.rx_index_out < length)
			tmp = dev->data.rx_index_in - dev->data.rx_index_out;
		else 
			tmp = length;
	}else if(dev->data.rx_index_out > dev->data.rx_index_in){
		if(DEV_BUFFER_SIZE - dev->data.rx_index_out + dev->data.rx_index_in < length)
			tmp = DEV_BUFFER_SIZE - dev->data.rx_index_out + dev->data.rx_index_in;
		else
			tmp = length;
	}
	for(i=0;i < tmp ;i++){
		*(buffer + i) = dev->data.rx_buffer[(dev->data.rx_index_out)++] ;
		if(dev->data.rx_index_out >= DEV_BUFFER_SIZE)
			dev->data.rx_index_out = 0;
	}
	signal(dev->data.rx_buffer_out_sem);
	signal(dev->dev_sem);
	return i;
		
}
s32 USART1_write(struct device *dev,u16 length,void *bffer)
{
	int i,tmp;
	u8 *buffer;

	buffer = (u8 *)bffer;
	wait(dev->dev_sem);
	wait(dev->data.tx_buffer_in_sem);
	if(dev->data.tx_index_out <= dev->data.tx_index_in){
		if(DEV_BUFFER_SIZE - dev->data.tx_index_in + dev->data.tx_index_out - 1< length)
			tmp = DEV_BUFFER_SIZE - dev->data.tx_index_in + dev->data.tx_index_out - 1;
		else 
			tmp = length;
	}else if(dev->data.tx_index_out > dev->data.tx_index_in){
		if(dev->data.tx_index_out - dev->data.tx_index_in  - 1< length )
			tmp = dev->data.tx_index_out - dev->data.tx_index_in - 1;
		else
			tmp = length;
	}
	for(i=0;i < tmp ;i++){
		dev->data.tx_buffer[dev->data.tx_index_in] = *(buffer + i);
		dev->data.tx_index_in++;
		if(dev->data.tx_index_in >= DEV_BUFFER_SIZE){
			dev->data.tx_index_in = 0;
		}
	}
	signal(dev->data.tx_buffer_in_sem);
	signal(dev->dev_sem);
	return tmp;
}
s32 USART1_control (struct device * dev,u8 flag)
{
	wait(dev->dev_sem);
	signal(dev->dev_sem);
	return 0;
}

int fputc(int ch, FILE *f)
{
	console_dev->device_write(console_dev,1,&ch);
	return (ch);
}
scp_device_t USART1_register(void)
{
	struct device dev;
	
	strcpy((char *)(dev.name),"USART1");
	dev.device_class = KERNEL_MODE;
	dev.device_init = USART1_init;
	dev.device_open = USART1_open;
	dev.device_close = USART1_close;
	dev.device_control = USART1_control;
	dev.device_read = USART1_read;
	dev.device_write = USART1_write;
	dev.dev_sem.value = 1;
	dev.dev_sem.sem_thread_head=NULL;
	dev.data.rx_buffer_in_sem.value = 1;
	dev.data.rx_buffer_in_sem.sem_thread_head=NULL;
	dev.data.rx_buffer_out_sem.value = 1;
	dev.data.rx_buffer_out_sem.sem_thread_head=NULL;
	dev.data.tx_buffer_in_sem.value = 1;
	dev.data.tx_buffer_in_sem.sem_thread_head=NULL;
	dev.data.tx_buffer_out_sem.value = 1;
	dev.data.tx_buffer_out_sem.sem_thread_head=NULL;
	dev.data.rx_index_in = 0;
	dev.data.rx_index_out = 0;
	dev.data.tx_index_in = 0;
	dev.data.tx_index_out = 0;
	dev.flag = 0;
	
	return device_register(&dev);
}



