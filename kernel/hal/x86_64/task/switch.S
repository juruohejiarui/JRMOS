#include <hal/task/genasm.h>
#include <hal/linkage.h>

ENTRY(hal_task_sche_switch)
	pushq %rbp
	pushq %rax
	movq %rsp, %rax
	andq $-32768, %rax
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