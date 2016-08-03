	AREA PROGRAM, CODE, READONLY, ALIGN=2
		THUMB
		REQUIRE8
		PRESERVE8
		
	IMPORT	currentThread
	IMPORT	thread_schedule
	IMPORT	sys_tick_it_enable
	IMPORT	sys_tick_start
		
		
__cpu_mode_ensure    PROC
		EXPORT  __cpu_mode_ensure
		MRS     r0, CONTROL
		TST		r0,#1
		MOVEQ	r0,#0
		MOVNE	r0,#1
		BX      LR
		ENDP

__spot_store	PROC
		EXPORT  __spot_store
		CPSID 	I
		PUSH	{r1}
		MRS		r1,PSP            ; get new task stack pointer
		SUB		r1,#32
		STR		r4,[r1,#0]
		STR		r5,[r1,#4]
		STR		r6,[r1,#8]
		STR		r7,[r1,#12]
		STR		r8,[r1,#16]
		STR		r9,[r1,#20]
		STR		r10,[r1,#24]
		STR		r11,[r1,#28]
		MSR		PSP,r1
		POP		{r1}
		CPSIE 	I
		BX		LR
		ENDP


;r0:the stackpoit of the thread to be switched to 	
__switch_to    PROC
		EXPORT  __switch_to
		PUSH	{r1}
		LDR		r1,[r0]            ; get new task stack pointer
		MSR		PSP,r1
		POP		{r1}
		BX		LR
		ENDP
		
		
		
		
;r0:the stackpoit of the thread to be switched from
;r1:the stackpoit of the thread to be switched to
__switch_between    PROC
		EXPORT  __switch_between
		PUSH	{r2}
		MRS 	r2, PSP            ; store sp in preempted tasks TCB
		STR		r2,[r0]
		LDR		r2,[r1]            ; get new task stack pointer
		MSR		PSP,r2
		POP 	{r2}
		BX 		LR
		ENDP
		
		
		
		
__psp_init	PROC
		EXPORT  __psp_init
		MSR     PSP,r0
		BX		LR
		ENDP
		
		
		
__interrupt_enable	PROC
		EXPORT 	__interrupt_enable
		MSR		PRIMASK,r0
		CPSIE 	I
		BX		LR
		ENDP
		
		
		
		
__interrupt_disable	PROC
		EXPORT 	__interrupt_disable
		MRS		r0,PRIMASK
		CPSID 	I
		BX		LR
		ENDP
		
		
		
		
		
__goto_user_mode    PROC
		EXPORT  __goto_user_mode
		MRS     r0,CONTROL
		ORR		r0,#1
		MSR		CONTROL,r0
		BX		LR
		ENDP
		
		
		
		
__goto_psp_mode    PROC
		EXPORT  __goto_psp_mode
		MRS     r0,CONTROL
		ORR		r0,#2
		MSR		CONTROL,r0
		BX		LR
		ENDP
		
		
__psp_return	PROC
		EXPORT  __psp_return
		PUSH	{r1}
		MRS		r1,PSP
		SUB		r1,#32
		MSR		PSP,r1
		POP		{r1}
		BX		LR
		ENDP
		


SysTick_Handler	PROC
		EXPORT	SysTick_Handler
		PUSH	{r0,r1,LR}
		LDR		r0,=__spot_store
		BLX		r0
		LDR		r0,=currentThread
		LDR		r1,[r0]
		CMP		r1,#0
		BNE		PEND
		LDR		r0,=__psp_return
		BLX		r0
PEND	MOV		r0,#0x0000ED04	
		ORR		r0,#0xE0000000
		LDR		r1,[r0]
		ORR		r1,#0x10000000
		STR		r1,[r0]
		POP		{r0,r1,LR}
		BX		LR
		ENDP


PendSV_Handler PROC
		EXPORT	PendSV_Handler
		PUSH	{r0,r1,r2,LR}
		LDR		r0,=currentThread
		LDR		r1,[r0]
		MOV		r0,#1
		CMP		r1,#0
		MOVEQ	r0,#0
		LDR		r1, =thread_schedule
		BLX		r1
		MOV		r2,r0
		MRS		r0,PSP
		LDR		r4,[r0,#0]
		LDR		r5,[r0,#4]
		LDR		r6,[r0,#8]
		LDR		r7,[r0,#12]
		LDR		r8,[r0,#16]
		LDR		r9,[r0,#20]
		LDR		r10,[r0,#24]
		LDR		r11,[r0,#28]
		ADD		r0,#32
		MSR		PSP,r0
		MRS		r0,CONTROL
		CMP		r2,#0
		ANDEQ	r0,#0xFFFFFFFE
		CMP		r2,#1
		ORREQ	r0,#1
		MSR		CONTROL,r0
		POP		{r0,r1,r2,LR}
		BX		LR
		ENDP
		
		
SVC_Handler	PROC
		EXPORT	SVC_Handler
		PUSH	{r0,r1,LR}
		CMP		r0,#0
		BEQ		SVC_0
		CMP		r0,#1
		BEQ		SVC_1
		B		SVC_R
		
SVC_0	LDR		r1,=sys_tick_it_enable
		BLX		r1
		LDR		r1,=sys_tick_start
		BLX		r1
		B		SVC_R
		
		
SVC_1	MOV		r0,#0x0000ED04	
		ORR		r0,#0xE0000000
		LDR		r1,[r0]
		ORR		r1,#0x10000000
		STR		r1,[r0]
		B		SVC_R
		
		
SVC_R	POP		{r0,r1,LR}
		BX		LR
		ENDP
		
		
		
		
__SVC_ASK	PROC
		EXPORT	__SVC_ASK
		SVC		0
		BX		LR
		ENDP
		
		NOP
		END