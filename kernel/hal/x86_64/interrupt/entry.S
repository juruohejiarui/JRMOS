#include <hal/linkage.h>
#include <hal/task/genasm.h>

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

ENTRY(hal_intr_setTr)
    movw %di, %ax
    shlw $3, %ax
    ltr %ax
    mfence
    xorw %ax, %ax
    retq

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
    movq %rsp, %rax
	andq $-32768, %rax
	movq task_TaskStruct_state(%rax), %rax
    cmpq $task_state_NeedSchedule, %rax
    jne hal_intr_endSchedule
    callq task_schedule
hal_intr_endSchedule:
	// get CPU_ID of current task
    // leaq Intr_SoftIrq_state(%rip), %rbx
	// testq (%rbx, %rax, 8), %rcx
    // jnz _softirqHandler
    jmp hal_intr_restoreAll

ENTRY(hal_intr_divideError)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doDivideError(%rip), %rax
    xchgq %rax, (%rsp)

errorCode:
    pushq %rax
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
    pushq %rax
    xorq %rax, %rax
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq offsetErrorCode(%rsp), %rsi
    movq offsetFunc(%rsp), %rdx
    movq $0x10, %rdi
    movq %rdi, %ds
    movq %rdi, %es
    movq %rdi, %ss
    movq %rsp, %rdi

    call *%rdx
    jmp retFromException

ENTRY(hal_intr_debug)
    pushq $0
    pushq %rax
    leaq hal_intr_doDebug(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_nmi)
    cli
    /* no error code since it is not an exception */
    pushq %rax
    pushq %rax
    pushq %rax
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
    pushq %rax
    xorq %rax, %rax
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq $0x10, %rdi
    movq %rdi, %ds
    movq %rdi, %es
    movq %rdi, %ss
    movq $0, %rsi
    movq %rsp, %rdi

    callq hal_intr_doNMI
    jmp hal_intr_restoreAll

ENTRY(hal_intr_int3)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doInt3(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_overflow)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doOverflow(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_bounds)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doBounds(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_undefinedOpcode)
    cli 
    pushq	$0
	pushq	%rax
    leaq hal_intr_doUndefinedOpcode(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode
    
ENTRY(hal_intr_devNotAvailable)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doDevNotAvailable(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_doubleFault)
    cli 
    pushq %rax
    leaq hal_intr_doDoubleFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_coprocessorSegmentOverrun)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doCoprocessorSegmentOverrun(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_invalidTSS)
    cli 
    pushq %rax
    leaq hal_intr_doInvalidTSS(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_segmentNotPresent)
    cli 
    pushq %rax
    leaq hal_intr_doSegmentNotPresent(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_stackSegmentFault)
    cli 
    pushq %rax
    leaq hal_intr_doStackSegmentFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_generalProtection)
    cli 
    pushq %rax
    leaq hal_intr_doGeneralProtection(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_pageFault)
	cli
    pushq %rax
    leaq hal_intr_doPageFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_x87FPUError)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doX87FPUError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_alignmentCheck)
    cli 
    pushq %rax
    leaq hal_intr_doAlignmentCheck(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_machineCheck)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doMachineCheck(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_simdError)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doSIMDError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(hal_intr_virtualizationError)
    cli 
    pushq $0
    pushq %rax
    leaq hal_intr_doVirtualizationError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode