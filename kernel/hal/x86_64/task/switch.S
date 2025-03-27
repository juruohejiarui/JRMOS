#include <hal/task/genasm.h>
#include <hal/interrupt/genasm.h>
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
	movq hal_task_TaskStruct_rsp(%rdi), %rsp
	movq hal_task_TaskStruct_rip(%rdi), %rax
	jmp *%rax
hal_task_sche_switch_back:

	popq %rax
	popq %rbp

	retq

ENTRY(hal_task_syscall_api0) 
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq %rdi, %rax

	syscall

	popq %rbp
	retq

// u64 hal_task_syscall1(u64 index, u64 arg0)
ENTRY(hal_task_syscall_api1) 
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq %rdi, %rax
	movq %rsi, %rdi

	syscall

	popq %rbp
	retq

// u64 hal_task_syscall2(u64 index, u64 arg0, u64 arg1)
ENTRY(hal_task_syscall_api2) 
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi

	syscall

	popq %rbp
	retq

// u64 hal_task_syscall3(u64 index, u64 arg0, u64 arg1, u64 arg2)
ENTRY(hal_task_syscall_api3) 
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %r10

	syscall

	popq %rbp
	retq

// u64 hal_task_syscall4(u64 index, u64 arg0, u64 arg1, u64 arg2, u64 arg3)
ENTRY(hal_task_syscall_api4) 
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %r10
	movq %r8, %r12

	syscall

	popq %rbp
	retq

// u64 hal_task_syscall5(u64 index, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4)
ENTRY(hal_task_syscall_api5) 
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %r10
	movq %r8, %r12
	movq %r9, %r8

	syscall

	popq %rbp
	retq

// u64 hal_task_syscall6(u64 index, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
ENTRY(hal_task_syscall_api6) 
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %r10
	movq %r8, %r12
	movq %r9, %r8
	movq 16(%rsp), %r9

	syscall

	popq %rbp
	retq

// before enter this function,
// rax : syscall index
// rdi, rsi, r10, r12, r8, r9: paramters
ENTRY(hal_task_syscall_entryKrl)
	pushq %rbp
	pushq %rax
	movq %rsp, %rbp
	movq %rsp, %rdx
	andq $-(task_usrStkSize), %rdx
	// switch fs & gs
	movq hal_task_UsrStruct_krlFs, %rax
	movq %rax, %fs
	movq hal_task_UsrStruct_krlGs, %rax
	movq %rax, %gs
	
	// save user stack pointer
	movq task_UsrStruct_tsk(%rax), %rdx
	movq %rsp, hal_task_TaskStruct_rsp2(%rdx)

	// switch stack
	leaq task_krlStkSize(%rdx), %rsp

	// save rip and rflags
	pushq %rcx
	pushq %r11
	
	// load the pointer of syscall function
	movq (%rbp), %rax
	leaq task_syscall_tbl(%rip), %rbx
	movq (%rbx, %rax, 8), %rax
	// keep rdi and rsi as 1-st and 2-nd parameters
	// load 3-rd and 4-th parameters to rdx, rcx
	movq %r10, %rdx
	movq %r12, %rcx
	// keep r8, r9 as 5-th and 6-th paramters

	// move to the syscall function
	pushq hal_task_syscall_backUsr(%rip)
	sti
	jmpq *%rax
ENTRY(hal_task_syscall_backUsr)
	cli
	// recover user stack pointer, rip and rflags
	popq %r11
	popq %rcx
	movq %rsp, %rbx
	andq $-(task_krlStkSize), %rbx
	movq hal_task_TaskStruct_rsp2(%rbx), %rsp

	// recover fs, gs
	movq %rsp, %rbx
	andq $-(task_usrStkSize), %rbx
	hlt
	// switch fs & gs
	movq hal_task_UsrStruct_usrFs(%rbx), %rax
	movq %rax, %fs
	movq hal_task_UsrStruct_usrGs(%rbx), %rax
	movq %rax, %gs


	popq %rbx
	popq %rbp
	
	sysretq