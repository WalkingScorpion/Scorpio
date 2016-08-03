#include	<stdio.h>
#include	"Scorpio_conf.h"

#define DEBUG

/*系统*/
#define SUCC		0
#define ERROR1 -1
#define ERROR2 -2
#define ERROR3 -3
#define ERROR4 -4
#define ERROR5 -5

#define SYS_CLK 72000000
#define THREAD_SWITCH_PERIOD 10

#define SYS_TICK_PRIORITY 0
#define USART1_PRIORITY 1
#define SVC_PRIORITY 2
#define PendSV_PRIORITY 255

extern int Image$$RW_IRAM1$$ZI$$Limit;
unsigned long __cpu_mode_ensure(void);
void __goto_user_mode(void);
void __goto_psp_mode(void);
void psp_init(void);
void __switch_to(u32 *to_sp);
void __switch_between(u32 *from_sp,u32 *to_sp);
void __spot_store(void);
void __interrupt_enable(u32);
u32 __interrupt_disable(void);
void __SVC_ASK(u32);
void startup(void);

enum scp_class
{
	KERNEL_MODE,
	USER_MODE
};

/*DEBUG*/
#ifdef DEBUG
extern char debugCount;
void debug_init(void);
void test(char id);
void look(char *buffer,char length);
char asm_test(char);
void flow_led(u32);
#endif

/*板级控制*/
void board_init(void);
void sys_tick_start(void);
void sys_tick_stop(void);
void sys_tick_it_enable(void);
void sys_tick_it_disable(void);
void sys_tick_setreload(u32 index);
void sys_tick_clear(void);

/*信号量*/
struct sem
{
	volatile s32 value;
	volatile void *sem_thread_head;
};
typedef struct sem scp_sem_t;

void wait(struct sem sem);
void signal(struct sem sem);
void sem_table_insert(void *thread,volatile void *head);
void sem_table_remove(void *thread,volatile void *head);

/*设备管理*/
#define DEV_NAME_MAX_LENGTH 20
#define DEV_MAX_NUM 255
#define DEV_BUFFER_SIZE 1024

typedef enum scp_class scp_deviceClass_t;

struct data_scope
{
	struct sem tx_buffer_in_sem;
	struct sem tx_buffer_out_sem;
	struct sem rx_buffer_in_sem;
	struct sem rx_buffer_out_sem;
	volatile u16 rx_index_in;
	volatile u16 rx_index_out;
	volatile u16 tx_index_in;
	volatile u16 tx_index_out;
	u8 tx_buffer[DEV_BUFFER_SIZE];
	u8 rx_buffer[DEV_BUFFER_SIZE];
};
typedef struct data_scope scp_data_t;

struct device
{
	struct sem dev_sem;
	
	u8 id;
	u8 name[DEV_NAME_MAX_LENGTH];
	scp_deviceClass_t device_class;
	u8 flag;
	
	s32 (*device_init) (struct device * dev);
	s32 (*device_open) (struct device * dev);
	s32 (*device_close) (struct device * dev);
	s32 (*device_control) (struct device * dev,u8 flag);
	s32 (*device_read) (struct device * dev,u16 length,void *buffer);
	s32 (*device_write) (struct device * dev,u16 length,void *buffer);
	
	scp_data_t data;
	
	void *parent_thread;
	struct device *next_brother;

	struct device *prev;
	struct device *next;
};
typedef struct device *scp_device_t;

struct scp_device_table
{
	u8 number;
	scp_device_t tail;
	scp_device_t head;
};
typedef struct scp_device_table scp_deviceTable_t;


scp_device_t device_register(scp_device_t dev);
s32 device_cancel_withName(void *name);
s32 device_cancel_withID(u8 id);
struct device *device_find_withID(unsigned char id);
struct device *device_find_withName(void *name);
struct device *device_find_withID(unsigned char id);


/*内存管理*/
#define MIN_MEM_ALLOC 4
#define ALIGN_SIZE 4
#define SIZEOF_STRUCT_MEM sizeof(struct mem)
#define STM32_SRAM_SIZE 64
#define STM32_SRAM_END (0x20000000 + STM32_SRAM_SIZE * 1024)
//SCP_ALIGN(13, 4) would return 16.
#define SCP_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))
//SCP_ALIGN_DOWN(13, 4) would return 12.
#define SCP_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))
struct mem
{
	u32 size;
	u16 used;
	struct mem *prev_info_offset;
	struct mem *next_info_offset;
};
typedef struct mem *scp_mem_t;

void mem_init(void);
s32 scp_memory_heap_init(void *start_addr,void *end_addr);
unsigned char *scp_malloc(unsigned short size);
s32 scp_free(void *index);

/*进程管理*/
#define THREAD_NAME_MAX_LENGTH DEV_NAME_MAX_LENGTH
#define THREAD_SWAP_SIZE 1024
#define THREAD_MAX_NUM 256
#define THREAD_STATUS_NUM 4
typedef enum scp_class scp_threadClass_t;

enum thread_status
{
	RUNNING,
	SUSPENDING,
	STOPPED,
	DEAD
};
typedef enum thread_status scp_thread_status_t;

struct thread_communiation
{
	u32 signal_flag;
	scp_sem_t thread_communiation_sem;
	u16 size;
	u8 offset;
};
typedef struct thread_communiation thread_communiation_t;


struct scp_thread
{
	u8 name[THREAD_NAME_MAX_LENGTH];
	scp_threadClass_t thread_class;
	u32 flag;
	u16 thread_stack_size;
	u8 thread_priority;
	
	u8 id;
	u32 thread_sp;
	u32 thread_stack;
	
	scp_thread_status_t thread_status;
	
	u32 timer_info;
	
	thread_communiation_t swap_scope;
	
	
	//进程间关系
	struct scp_thread *parent;
	struct scp_thread *first_child;
	struct scp_thread *next_brother;
	//进程注册设备
	scp_device_t first_child_deivce;
	//进程调度链表指针
	struct scp_thread *next;
	//信号量链表指针
	struct scp_thread *sem_next;
};
typedef struct scp_thread *scp_thread_t;

struct scp_thread_table
{
	u8 number;
	scp_thread_t tail;
	scp_thread_t head;
};
typedef struct scp_thread_table scp_threadTable_t;

void sys_thread_init(void);
scp_thread_t thread_create(char *name, scp_threadClass_t thread_class, u32 flag, u32 thread_stack_size, 
														u8 thread_priority,void *tentry);

void texit(void);
void thread_table_insert(scp_thread_t thread,scp_thread_status_t status);
void thread_table_convert(scp_thread_t thread,scp_thread_status_t des_status);
void thread_table_remove(scp_thread_t thread);
void thread_table_roll(scp_thread_status_t status);

/*USART1串口驱动*/
scp_device_t USART1_register(void);
/*操作台*/
#define SCORPIO_LOGO ("\n\n\rScorpio v1.0\n\rNice to meet you,man!\n\rMake it easy.")
#define THINK_LOGO ("\n\rThink >")
s32 console_chose_withID(u8 id);
s32 console_chose_withName(void *name);
s32 console_init(void);
void console_thread_entry(void);
void Scorpio_logo(void);
/*命令行*/
void think(void);
void think_logo(void);
/*进程调度*/
u32 thread_schedule(u8 parameter);


