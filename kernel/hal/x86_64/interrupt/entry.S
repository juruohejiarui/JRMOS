#include <hal/linkage.h>

offsetR15		= 0x00
offsetR14		= 0x08
offsetR13		= 0x10
offsetR12		= 0x18
offsetR11		= 0x20
offsetR10		= 0x28
offsetR9		= 0x30
offsetR8		= 0x38
offsetRbx		= 0x40
offsetRcx		= 0x48
offsetRdx		= 0x50
offsetRsi		= 0x58
offsetRdi		= 0x60
offsetRbp		= 0x68
offsetDs		= 0x70
offsetEs		= 0x78
offsetRax		= 0x80
offsetFunc		= 0x88
offsetErrorCode	= 0x90
offsetRip		= 0x98
offsetCs		= 0xa0
offsetRflags	= 0xa8
offsetRsp		= 0xb0
offsetSs		= 0xb8

ENTRY(hal_intr_restoreAll)
	// movq %rsp, %rdi
	// call Task_checkPtRegInStack
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rbp
    popq %rax
    movq %rax, %ds
    popq %rax
    movq %rax, %es
    popq %rax
    addq $0x10, %rsp
    iretq

retFromException:
ENTRY(hal_intr_retFromIntr)
    // call Task_schedule
	// // get CPU_ID of current task
	// movq %rsp, %rax
	// andq $-32768, %rax
	// movq 0x50(%rax), %rax
    // movq $-1, %rcx
    // leaq Intr_SoftIrq_state(%rip), %rbx
	// testq (%rbx, %rax, 8), %rcx
    // jnz _softirqHandler
    jmp hal_intr_restoreAll