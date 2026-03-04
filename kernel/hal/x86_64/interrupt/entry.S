#include <hal/linkage.h>
#include <cpu/define.h>
#include <hal/task/genasm.h>
#include <hal/interrupt/genasm.h>

.global task_sche_state
.global cpu_symbol_percpu
.global cpu_var(task_scheMsk)
.global cpu_var(softirq_flag)

ENTRY(hal_intr_setTr)
    movw %di, %ax
    shlw $3, %ax
    ltr %ax
    mfence
    xorw %ax, %ax
    retq

ENTRY(hal_intr_chkSwapgs)
    btq $63, %rdi
    jc hal_intr_chkSwapgs_end
    swapgs
hal_intr_chkSwapgs_end:
    retq

ENTRY(hal_intr_restoreAll)
	// conduct swapgs if we should back to user mode
    btq $63, hal_intr_PtReg_rip(%rsp)
    jc hal_intr_restoreAll_popReg
    swapgs
hal_intr_restoreAll_popReg:
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
	andq $-(task_krlStkSize), %rax
    movq task_TaskStruct_signal(%rax), %rax
    testq %rax, %rax
    jz hal_intr_trySchedule
    callq task_signal_scan
hal_intr_trySchedule:
    leaq cpu_var(task_scheMsk)(%rip), %rax
    leaq cpu_symbol_percpu(%rip), %rcx
    subq %rcx, %rax
    movq %gs:(%rax), %rax
    testq %rax, %rax
    jnz hal_intr_endSchedule
    movq %rsp, %rax
    andq $-(task_krlStkSize), %rax
	movq task_TaskStruct_state(%rax), %rax
    testq $1, %rax    // if %rax == task_state_Running
    jz hal_intr_endSchedule
    callq task_sche
hal_intr_endSchedule:
	// get CPU_ID of current task
    leaq cpu_var(softirq_flag)(%rip), %rax
    leaq cpu_symbol_percpu(%rip), %rcx
    subq %rcx, %rax
    movq %gs:(%rax), %rax
	testq %rax, %rax
    jz hal_intr_endSirq
    callq softirq_scan
hal_intr_endSirq:
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

    btq $63, hal_intr_PtReg_rip(%rsp)
    jc errorCode_noSwapgs
    swapgs
errorCode_noSwapgs:

    movq hal_intr_PtReg_errCode(%rsp), %rsi
    movq hal_intr_PtReg_func(%rsp), %rdx
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