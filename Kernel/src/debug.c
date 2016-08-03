#include "stm32f10x.h"
#include <stdio.h>
#include <math.h>
char debugCount=1;

void debug_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOF, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOF,GPIO_Pin_8);
	GPIO_SetBits(GPIOF,GPIO_Pin_7);
	GPIO_SetBits(GPIOB,GPIO_Pin_0);
}
void test(char id)
{
	
	if ((id&0x01) == 0x01)
		GPIO_ResetBits(GPIOF,GPIO_Pin_8);
	if ((id&0x02) == 0x02)
		GPIO_ResetBits(GPIOF,GPIO_Pin_7);
	if ((id&0x04) == 0x04)
		GPIO_ResetBits(GPIOB,GPIO_Pin_0);
	
}
void look(char *buffer,char length)
{
	int i;
	for(i = 0 ;i<length;i++){
		USART_SendData(USART1, *(buffer+i));
		while( USART_GetFlagStatus(USART1,USART_FLAG_TC)!= SET);
	}
}

void flow_led(u32 flag)
{
	u32 i;
	u32 tmp;
	u8 n;
	tmp=flag;
	n=0;
	while(1){
		if(tmp<=150000){
			tmp=flag;
			n=0;
			i=tmp;
			while(i--);
		}
		GPIO_ResetBits(GPIOB,GPIO_Pin_0);
		tmp=flag*(sqrt(n+1)-sqrt(n));
		n++;
		i=tmp;
		while(i--);	
		GPIO_SetBits(GPIOB,GPIO_Pin_0);
		GPIO_ResetBits(GPIOF,GPIO_Pin_7);
		tmp=flag*(sqrt(n+1)-sqrt(n));
		n++;
		i= tmp;
		while(i--);
		GPIO_SetBits(GPIOF,GPIO_Pin_7);
		GPIO_ResetBits(GPIOF,GPIO_Pin_8);
		tmp=flag*(sqrt(n+1)-sqrt(n));
		n++;
		i= tmp;
		while(i--);
		GPIO_SetBits(GPIOF,GPIO_Pin_8);
	}
	
}
