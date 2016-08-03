#include "Scorpio.h"

scp_mem_t heaphdr;
scp_mem_t heaplimit;
scp_sem_t heap_access_sem={1,NULL};

void mem_init(void)
{
	scp_memory_heap_init((void *)&Image$$RW_IRAM1$$ZI$$Limit,(void *)STM32_SRAM_END);
}

s32 scp_memory_heap_init(void *start_addr,void *end_addr)
{
		u32 start = SCP_ALIGN((u32)start_addr,ALIGN_SIZE);
		u32 end = SCP_ALIGN_DOWN((u32)end_addr,ALIGN_SIZE);
		if (end - start + 1<SIZEOF_STRUCT_MEM + MIN_MEM_ALLOC)
			return ERROR1;
		
		heaphdr=(scp_mem_t)start;
		heaplimit=(scp_mem_t)end - sizeof(struct mem) + 1;
		heaphdr->size = (u32)heaplimit - start - SIZEOF_STRUCT_MEM;
		heaphdr->used = 0;
		heaphdr->prev_info_offset=heaphdr;
		heaphdr->next_info_offset=heaplimit;
		
		return SUCC;
}

u8 *scp_malloc(u16 size)
{
		scp_mem_t mem,tmp_mem,new_mem;
		u8 sencond_plan_found=0;
		wait(heap_access_sem);
		if (size < MIN_MEM_ALLOC)
			size = MIN_MEM_ALLOC;
		size = SCP_ALIGN(size,ALIGN_SIZE);
		for(mem = heaphdr;mem != heaplimit;mem = mem -> next_info_offset){
			if(mem -> used == 0){
				if( (u32)mem->next_info_offset - (u32)mem >= size + 2*SIZEOF_STRUCT_MEM + MIN_MEM_ALLOC ){
						mem->size = size;
						mem->used = 1;
						new_mem = (scp_mem_t)((u32)mem + SIZEOF_STRUCT_MEM + size);
						new_mem->next_info_offset = mem->next_info_offset;
						new_mem->prev_info_offset = mem;
						mem->next_info_offset = new_mem;
						new_mem->next_info_offset->prev_info_offset = new_mem;
						new_mem->size = (u32)new_mem->next_info_offset - (u32)new_mem - SIZEOF_STRUCT_MEM ;
						new_mem->used = 0;
						
						signal(heap_access_sem);
						return ((u8 *)mem + SIZEOF_STRUCT_MEM);
					}else if((u32)mem->next_info_offset - (u32)mem >= size + SIZEOF_STRUCT_MEM){
						if(mem -> next_info_offset != heaplimit){
							tmp_mem = mem;
							sencond_plan_found = 1;
							continue;
						}else{
							mem->size = size;
							mem->used = 1;
							mem->next_info_offset = heaplimit;
						
							signal(heap_access_sem);
							return ((u8 *)mem + SIZEOF_STRUCT_MEM);
						}
					}
				}
			}
						
						
		if(sencond_plan_found){
			tmp_mem->size = size;
			tmp_mem->used = 1;
	
			signal(heap_access_sem);
			return ((u8 *)tmp_mem + SIZEOF_STRUCT_MEM);
		}
		
		signal(heap_access_sem);
		return NULL;
}

s32 scp_free(void *index)
{
	scp_mem_t mem,new_mem;
	s32 flag = ERROR1;
	wait(heap_access_sem);
	for (mem = heaphdr;mem != heaplimit;mem = mem -> next_info_offset){
		if((u8 *)mem + SIZEOF_STRUCT_MEM == index){
			flag = SUCC;
			if(mem->next_info_offset->used == 1 && mem->prev_info_offset->used == 1){
				new_mem = (scp_mem_t)((u32)mem->prev_info_offset + mem->prev_info_offset->size + SIZEOF_STRUCT_MEM);
				new_mem->used = 0;
				new_mem->prev_info_offset = mem->prev_info_offset;
				new_mem->next_info_offset = mem->next_info_offset;
				mem->prev_info_offset->next_info_offset = new_mem;
				mem->next_info_offset->prev_info_offset = new_mem;
				new_mem->size = (u32)new_mem->next_info_offset - (u32)new_mem - SIZEOF_STRUCT_MEM;
			}else{
				if(mem->prev_info_offset->used == 0){
					mem->prev_info_offset->next_info_offset=mem -> next_info_offset;
					mem->next_info_offset->prev_info_offset=mem -> prev_info_offset;
					mem->prev_info_offset->size = (u32)mem->next_info_offset - (u32)mem->prev_info_offset - SIZEOF_STRUCT_MEM;
					mem = mem->prev_info_offset;
				}
				if(mem->next_info_offset->used == 0){
					new_mem = (scp_mem_t)((u32)mem->prev_info_offset + mem->prev_info_offset->size + SIZEOF_STRUCT_MEM);
					new_mem->used = 0;
					new_mem->next_info_offset = mem->next_info_offset->next_info_offset;
					new_mem->prev_info_offset = mem->prev_info_offset;
					new_mem->prev_info_offset->next_info_offset = new_mem;
					new_mem->next_info_offset->prev_info_offset  = new_mem;
					new_mem->size = (u32)new_mem->next_info_offset - (u32)new_mem - SIZEOF_STRUCT_MEM;
				}
			}
		}
	}
	signal(heap_access_sem);
	return flag;
}

