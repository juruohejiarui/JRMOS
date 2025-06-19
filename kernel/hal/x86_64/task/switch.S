#include <hal/task/genasm.h>
#include <hal/interrupt/genasm.h>
#include <hal/cpu/genasm.h>
#include <hal/linkage.h>

.global task_syscall_tbl

ENTRY(hal_task_sche_switch)
	pushq %rbp
	pushq %rax
	movq %rsp, %rax
	andq $-(task_krlStkSize), %rax
	movq %rsp, hal_task_TaskStruct_rsp(%rax)
	leaq hal_task_sche_switch_back(%rip), %rcx
	movq %rcx, hal_task_TaskStruct_rip(%rax)

	pushq %rdi
	movq %rdi, %rsi
	movq %rax, %rdi
	callq hal_task_sche_switchTss
	popq %rdi

	movq task_TaskStruct_thread(%rdi), %rbx
	movq hal_task_ThreadStruct_pgd(%rbx), %rax
	movq %cr3, %rbx
	cmpq %rbx, %rax
	
	je hal_task_sche_switchCR3_end
	movq %rax, %cr3
hal_task_sche_switchCR3_end:
	movq %rdi, %gs:hal_cpu_Desc_curKrlStk
	movq hal_task_TaskStruct_rsp(%rdi), %rsp
	movq hal_task_TaskStruct_rip(%rdi), %rax
	jmp *%rax
hal_task_sche_switch_back:
	popq %rax
	popq %rbp

	retq

// before enter this function,
// rdi: syscall idx
// rsi: syscall args
ENTRY(hal_task_syscall_entryKrl)
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	
	pushq %rbx

	// switch es and ds
	movq $0x10, %rbx
	movq %rbx, %ds
	movq %rbx, %es

	swapgs

	
	// save user stack and get kernel stack from hal_task_curKrlStk
	movq %gs:hal_cpu_Desc_curKrlStk, %rax
	movq %rsp, hal_task_TaskStruct_usrRsp(%rax)
	movq %rax, %rsp
	addq $task_krlStkSize, %rsp

	// save rip and rflags
	pushq %rcx
	pushq %r11
	
	// load the pointer of syscall function
	leaq task_syscall_tbl(%rip), %rax
	movq (%rax, %rdi, 8), %rax

	pushq %rdx
	pushq %rcx
	pushq %r8
	pushq %r9

	movq hal_intr_PtReg_rdi(%rsi), %rdi
	movq hal_intr_PtReg_rdx(%rsi), %rdx
	movq hal_intr_PtReg_rcx(%rsi ), %rcx
	movq hal_intr_PtReg_r8(%rsi), %r8
	movq hal_intr_PtReg_r9(%rsi), %r9
	movq hal_intr_PtReg_rsi(%rsi), %rsi

	// move to the syscall function
	sti
	callq *%rax

	popq %r9
	popq %r8
	popq %rcx
	popq %rdx

ENTRY(hal_task_syscall_backUsr)
	cli

	swapgs

	// recover user stack pointer, rip and rflags
	movq %rsp, %rbx
	andq $-(task_krlStkSize), %rbx

	popq %r11
	popq %rcx
	movq hal_task_TaskStruct_usrRsp(%rbx), %rsp

	// switch es and ds
	movq $0x33, %rbx
	movq %rbx, %ds
	movq %rbx, %es

	popq %rbx

	popq %rbp
	sysretq