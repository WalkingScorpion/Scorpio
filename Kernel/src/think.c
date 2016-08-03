#include "Scorpio.h"
#include "stm32f10x.h"
#include <string.h>
#define THINK_BUFFE_SIZE 64
#define CMD_BUFFER_CAP 10
#define CMD_BUFFER_SIZE (CMD_BUFFER_CAP+1)
extern scp_threadTable_t threadTable[THREAD_STATUS_NUM];

extern scp_deviceTable_t dev_table;
extern scp_sem_t dev_table_sem;
extern scp_device_t console_dev;
extern scp_sem_t console_sem;
extern scp_sem_t heap_access_sem;
extern scp_mem_t heaphdr;
extern scp_mem_t heaplimit;

u8 cmd_buffer[CMD_BUFFER_SIZE][THINK_BUFFE_SIZE];
u8 think_buffer[THINK_BUFFE_SIZE];
u16 think_index = 0;
u8 alt_flag=0;
s16 right_flag;
u8 up_flag;
s16 tail_flag;
s8 cmd_stack_sp=0;
s8 cmd_show_sp=-1;
s8 cmd_base_sp=-1;

void thinking(char);
void dev_list(void);
void heap_list(void);
void thread_list(void);
void debug(void);
void special(u8 buffer);
void think_refresh(void);
void think_to_tail(void);
void think_del_from_tail(u8 index);
void think_show(void);
void save_cmd(void);

void think(void)
{
	u8 buffer[THINK_BUFFE_SIZE];
	int tmp,i,j,just;
	tmp = console_dev->device_read(console_dev,DEV_BUFFER_SIZE,buffer);
	for (i = 0;i< tmp;i++){
		if(buffer[i] == '\0'){
			think_buffer[think_index++] = buffer[i];
			if(think_index!=0){
				save_cmd();
			}
			think_to_tail();
			thinking(0);
			think_index = 0;	
		}
		
		special(buffer[i]);
		
		if(alt_flag==0){
			if(buffer[i] == 127){
				if(up_flag){
					if (think_index && tail_flag){
						think_index--;
						think_to_tail();
						think_del_from_tail(1);
					}
				}else{
					if(right_flag&&tail_flag){
						for(j=right_flag;j<think_index;j++){
							think_buffer[j-1] = think_buffer[j];
						}
						just=tail_flag-right_flag;
						think_index--;
						think_refresh();
						for(j=0;j<just;j++,right_flag--){
							printf("%c%c%c",0x1b,0x5b,0x44);
						}
					}
				}
			
			}else if(buffer[i]>31&& buffer[i]<127){
				if(up_flag){
					think_to_tail();
					printf("%c",buffer[i]);
					right_flag++;
					tail_flag++;
					think_buffer[think_index++] = buffer[i];
				}else{
					for(j=think_index;j>right_flag;j--){
						think_buffer[j] = think_buffer[j-1];
					}
					think_buffer[right_flag]=buffer[i];
					just=tail_flag-right_flag;
					think_index++;
					think_refresh();
					for(j=0;j<just;j++,right_flag--){
						printf("%c%c%c",0x1b,0x5b,0x44);
					}
				}	
			}
		}
		if(alt_flag==0xff){
			alt_flag=0;
		}
		
	}//end of for
}

void special(u8 buffer)
{
	if(buffer == 0x1b){
		if(alt_flag==0)
			alt_flag++;
		else
			alt_flag=0;
		return;
	}
	
	if(alt_flag==1){
		if(buffer == 0x5b){
			alt_flag++;
		}else{
			alt_flag=0;
		}
		return;
	}
	
	if(alt_flag==2){
		if(buffer == 0x41){
			alt_flag=0xff;
			up_flag++;
			printf("%c%c%c",0x1b,0x5b,0x41);
		}else if(buffer == 0x42){
			alt_flag=0xff;
			if(up_flag){
				up_flag--;
				printf("%c%c%c",0x1b,0x5b,0x42);
				if(up_flag==0){
					think_to_tail();
				}
			}
		}else if(buffer == 0x43){
			alt_flag=0xff;
			if(!(up_flag==0 && right_flag == tail_flag)){
				printf("%c%c%c",0x1b,0x5b,0x43);
				right_flag++;
			}
		}else if(buffer == 0x44){
			alt_flag=0xff;
			if(up_flag  || ((up_flag==0) && (right_flag))){
				printf("%c%c%c",0x1b,0x5b,0x44);
				right_flag--;
			}
		}else if(buffer == 0x31){
			alt_flag=0xff;
		}else if(buffer == 0x32){
			alt_flag=0xff;
		}else if(buffer == 0x33){
			alt_flag+=1;
		}else if(buffer == 0x34){
			alt_flag=0xff;
		}else if(buffer == 0x35){
			alt_flag+=2;
		}else if(buffer == 0x36){
			alt_flag+=3;
		}else{
			alt_flag=0;
		}
		return;
	}
	
	if(alt_flag==3){
		if(buffer == 0x7e){
			alt_flag=0xff;
		}else{
			alt_flag=0;
		}
		return;
	}
	
	if(alt_flag==4){
		if(buffer == 0x7e){
			alt_flag=0xff;
			if((cmd_show_sp>=0 )&& (cmd_show_sp!=cmd_base_sp)){
				cmd_show_sp = (cmd_show_sp+CMD_BUFFER_SIZE -1)%CMD_BUFFER_SIZE;
				strcpy((char *)think_buffer,(const char *)(&cmd_buffer[cmd_show_sp][0]));
				think_index=strlen((const char *)think_buffer);
				think_refresh();
			}
		}else{
			alt_flag=0;
		}
		return;
	}
	
	if(alt_flag==5){
		if(buffer == 0x7e){
			alt_flag=0xff;
			if(cmd_show_sp>=0){
				if(((cmd_show_sp+1)%CMD_BUFFER_SIZE)!=cmd_stack_sp){
					cmd_show_sp = (cmd_show_sp+1)%CMD_BUFFER_SIZE;
					strcpy((char *)think_buffer,(const char *)(&cmd_buffer[cmd_show_sp][0]));
					think_index=strlen((const char *)think_buffer);
					think_refresh();
				}else{
					think_to_tail();
					think_del_from_tail(tail_flag);
				}
			}
		}else{
			alt_flag=0;
		}
		return;
	}
}

void think_refresh(void)
{
	think_to_tail();
	think_del_from_tail(tail_flag);
	think_show();
}
void think_to_tail(void)
{
	for(;up_flag;up_flag--){
		printf("%c%c%c",0x1b,0x5b,0x42);
	}
	if(tail_flag<right_flag){
		for(;right_flag != tail_flag;right_flag--){
			printf("%c%c%c",0x1b,0x5b,0x44);
		}
	}else{
		for(;tail_flag != right_flag;right_flag++){
			printf("%c%c%c",0x1b,0x5b,0x43);
		}
	}
}

void think_show(void)
{
	int i;
	for(i=0;i<think_index;i++,right_flag++,tail_flag++){
		printf("%c",think_buffer[i]);
	}
}

void think_del_from_tail(u8 index)
{
	int i;
	for(i=0;i<index;i++,right_flag--,tail_flag--){
		printf("%c",127);
	}
}


void thinking(char tmp)
{	
	if(think_index == 1){
		think_logo();
		return;
	}
	
	if (0 == strcmp((const char *)think_buffer,"dev"))
		dev_list();
	else if(0 == strcmp((const char *)think_buffer,"heap"))
		heap_list();
	else if(0 == strcmp((const char *)think_buffer,"thread"))
		thread_list();
#ifdef	DEBUG
	else if(0 == strcmp((const char *)think_buffer,"debug"))
		debug();
#endif
	else
		printf("\n\rNo such command.");
	
	think_logo();
}
void think_logo(void)
{
	printf(THINK_LOGO);
	right_flag=up_flag=tail_flag=0;
}
void dev_list(void)
{
	int i;
	scp_device_t dev;
	wait(dev_table_sem);
	printf("\n\r\tNo.\tID\tNAME\t\tLIMIT\tSTATUS");
	for(i=1,dev = dev_table.head;dev != NULL;dev = dev->next,i++){
		printf("\n\r\t%d\t%d\t%s\t\t",i,dev->id,dev->name);
		if(dev->device_class == 0)
			printf("Kernel\t");
		else if(dev->device_class == 1)
			printf("User\t");
		printf("%02xH",dev->flag);
	}			
	signal(dev_table_sem);
}
void heap_list(void)
{
	u32 i,sum=0;
	scp_mem_t mem;
	wait(heap_access_sem);
	printf("\n\r\tNo.\tSTART\t\tEND\t\tSIZE\tPERCENTAGE\tSTATUS");
	for(mem = heaphdr,sum = 0;mem != heaplimit;sum+=mem->size,mem = mem -> next_info_offset);
	for(mem = heaphdr,i=1;mem != heaplimit;mem = mem -> next_info_offset,i++){
		printf("\n\r\t%u\t0x%x\t0x%x\t%u\t%.1lf%%\t\t",i,(u32)mem+SIZEOF_STRUCT_MEM,(u32)mem \
		+ SIZEOF_STRUCT_MEM + mem->size - 1, mem->size,1.0*mem->size*100/sum);
		if(mem->used == 0)
			printf("Available");
		else
			printf("In Use");
		
	}
	signal(heap_access_sem);
}

void thread_list(void)
{
	scp_thread_t tmp_thread;
	int i,j;
	u8 *k;
	printf("\n\r\tNo.\tID\tNAME\t\tSTACK\tUSED\tSTATUS");
	for(i=0,j=1;i<THREAD_STATUS_NUM;i++){
		for(tmp_thread=threadTable[i].head;tmp_thread!=NULL;tmp_thread= tmp_thread->next,j++){
			for(k=(u8 *)(tmp_thread->thread_stack-tmp_thread->thread_stack_size);*k=='#';k++);
			k=(u8 *)(tmp_thread->thread_stack-(u32)k);
			printf("\n\r\t%u\t%u\t%s\t\t%u\t%lu\t",j,tmp_thread->id,tmp_thread->name,tmp_thread->thread_stack_size,(u32)k);
			switch (tmp_thread->thread_status){
				case 0:
					printf("RUNNING");
					break;
				case 1:
					printf("SUSPENDING");
					break;
				case 2:
					printf("STOPPED");
					break;
				case 3:
					printf("DEAD");
					break;
				default:
					;
			}
		}
	}
}

void save_cmd(void)
{
	if(cmd_base_sp==-1)
		cmd_base_sp=0;
		
	strcpy((char *)(&cmd_buffer[cmd_stack_sp][0]),(const char *)think_buffer);

	cmd_stack_sp=(cmd_stack_sp+1)%CMD_BUFFER_SIZE;	
	cmd_show_sp = cmd_stack_sp;
	if(cmd_stack_sp==cmd_base_sp)
		cmd_base_sp=(cmd_stack_sp+1)%CMD_BUFFER_SIZE;

}
#ifdef	DEBUG
void debug(void)
{
	
}
#endif
