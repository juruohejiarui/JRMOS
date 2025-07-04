#include <hal/linkage.h>
#include <hal/cpu/genasm.h>
#include <hal/interrupt/genasm.h>

.globl cpu_desc
.globl hal_hw_apic_supportFlag
.globl hal_init_initAP
.globl hal_init_init
.globl hal_init_stk

.section .text

ENTRY(_start)
    mov %ss, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    mov $0x7E00, %esp

    // set cr3
    movq $0x101000, %rax
    movq %rax, %cr3

    lgdt gdtPointer(%rip)

    // set all the segment register to kernel data 64-bit
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    movq $0x1b, %rcx
    rdmsr
    bt $8, %rax
    jnc setSMPInfo

    movq stkPtr(%rip), %rsp
	lidt hal_init_idtPtr(%rip)
    jmp setSmpTmpStkEnd

setSMPInfo:
    movq $0xb, %rax
    xorq %rcx, %rcx
    cpuid
    leaq cpu_desc(%rip), %rax
setSMPInfo_LoopStart:
    cmpl hal_cpu_Desc_x2apic(%rax), %edx
    jz setSMPInfo_LoopEnd
    addq $cpu_DescSize, %rax
    jmp setSMPInfo_LoopStart
setSMPInfo_LoopEnd:
	// set stack pointer
    movq hal_cpu_Desc_initStk(%rax), %rsi
	// set idt pointer
	lidt hal_cpu_Desc_idtPtr(%rax)
    addq $32768, %rsi
    movq %rsi, %rsp
setSmpTmpStkEnd:

    movq $0x101000, %rax
    movq %rax, %cr3
    movq switchSeg(%rip), %rax
    pushq $0x08
    pushq %rax
    lretq

switchSeg:
    .quad entry64

entry64:

    lgdt gdtPointer(%rip)
    
    cli
    movq $0x10, %rax
    movq %rax, %ds
    movq %rax, %es
    movq %rax, %gs
    movq %rax, %ss

    movq $0x1b, %rcx
    rdmsr
    bt $8, %rax
    jnc SMPEntry

    movq stkPtr(%rip), %rsp

setupIDT:
    leaq ignoreInt(%rip), %rdx
    // the segment selector was set to 0x08 (kernel code 64-bit)
    movq $(0x08 << 16), %rax
    // set the 0...15th bits of offset
    movw %dx, %ax
    // set the gate type to be 0xE and DPL to be 3 and the 47-th bit to be 1
    movq $((0xE << 40) + (1 << 47)), %rcx
    orq %rcx, %rax
    // set the 16...31-th bits of offset to 48...63-th bits
    movl %edx, %ecx
    shrl $16, %ecx
    shlq $48, %rcx
    orq %rcx, %rax
    // set the 32...63-th bits of offset to 64...95-th bits
    shrq $32, %rdx
// set all the interrupt vector to ignoreInt()
    leaq hal_init_idtTbl(%rip), %rdi
    mov $256, %rcx
repeatSetIDT:
    movq %rax, (%rdi)
    movq %rdx, 8(%rdi)
    addq $0x10, %rdi
    dec %rcx
    jne repeatSetIDT

// set the GDT descriptor of TSS
setupTSS64:
    leaq hal_init_tss(%rip), %rdx
    xorq %rax, %rax
    xorq %rcx, %rcx
    // present(47th) = 1, DescriptorType(44th) = 0, E(32th) = 1, A(40th) = 1
    movq $(0x89 << 40), %rax
    // get the 0...31-th bits of address of hal_init_tss(represented by A)
    movl %edx, %ecx
    // set the 24-31-th bits of A to the 56...64-th bits
    shrl $24, %ecx
    shlq $56, %rcx
    orq %rcx, %rax
    // set the 0...23-th bits of A to 16...31-th bits
    movl %edx, %ecx
    andq $0xffffff, %rcx
    shlq $16, %rcx
    orq %rcx, %rax
    // set the limit of this segment to 0...15-th bits
    orq $103, %rax
    // set the 32...64-th bits of A to 64...95-th bits
    shrq $32, %rdx
    // set rax and rdx to the hal_init_gdtTbl + 0x50 
    leaq hal_init_gdtTbl(%rip), %rdi
    movq %rax, 0x50(%rdi)
    movq %rdx, 0x58(%rdi)

    // enable the tss
    // mov $0x50, %ax
    // ltr %ax

    movq goToKernel(%rip), %rax
    pushq $0x08
    pushq %rax
    lretq

SMPEntry:
    movq %rsi, %rsp
    leaq hal_init_initAP(%rip), %rax
    pushq $0x08
    pushq %rax
    lretq

// goToSmpKernel:
    // .quad hal_Init_initAP

goToKernel:
    .quad hal_init_init

ignoreInt:
    cli
    pushq $0
    
    // save the register in the order of struct PtraceRegs
    pushq %rax
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
    pushq %rax
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

    // switch the segment register to kernel data 64-bit
    movq $0x10, %rax
    movq %rax, %ds
    movq %rax, %es

    movq hal_intr_PtReg_rip(%rsp), %rdi
    callq hal_intr_chkSwapgs

    movq $0xb, %rax
    xorq %rcx, %rcx
    cpuid
    leaq cpu_desc(%rip), %rax
    xorq %rcx, %rcx
ignoreInt_FindCpuId:
    cmpl hal_cpu_Desc_x2apic(%rax), %edx
    je ignoreInt_FindCpuId_End
    addq $cpu_DescSize, %rax
    incq %rcx
    jmp ignoreInt_FindCpuId
ignoreInt_FindCpuId_End:
    // printk(screen_err, "Unknown interrupt or fault at RIP %p cpuid=%d\n", rip, cpuid);
    movq hal_intr_PtReg_rip(%rsp), %rdx
    leaq intMsg(%rip), %rax
    pushq %rax
    movq %rax, %rsi
    movq $0xff0000, %rdi
    movq $0, %rax
    callq printk
    addq $0x8, %rsp

ignoreInt_loop:
    jmp ignoreInt_loop

    // restore the registers
    popq %rax
    movq %rax, %ds
    popq %rax
    movq %rax, %es

intMsg:
    .asciz "Unknown interrupt or fault at RIP %p cpuid=%d\n"

stkPtr:
	.quad hal_init_stk + 32768

// initial page table
.align 8

.org 0x1000

pml4e:
    .quad 0x102007
    .fill 255, 8, 0
    .quad 0x102007 /*0xffff800000000000 ~ 0xffff807FFFFFFFFF*/
    .fill 255, 8, 0

.org 0x2000

pdpte:
    .quad 0x103007  /* 0xffff800000000000 ~ 0xffff800040000000 */
    .quad 0x40000087      /* 0xffff800040000000 ~ 0xffff800080000000 */
    .quad 0x80000087      /* 0xffff800080000000 ~ 0xffff8000C0000000 */
    .fill 509, 8, 0

.org 0x3000
    // the 7-th bit is 1, so the pointer of the page table item is to the physical address
    .quad	0x000087	
	.quad	0x200087 /* 0x200000 ~ 0x200FFF */
	.quad	0x400087
	.quad	0x600087
	.quad	0x800087
	.quad	0xa00087		/*0x a00000*/
	.quad	0xc00087
	.quad	0xe00087
	.quad	0x1000087
	.quad	0x1200087		/*0x1000000*/
	.quad	0x1400087
	.quad	0x1600087
	.quad	0x1800087
	.quad 	0x1a00087
	.quad 	0x1c00087
	.quad 	0x1e00087
	.quad 	0x2000087
	.quad 	0x2200087
	.quad 	0x2400087
	.quad 	0x2600087
	.quad 	0x2800087
	.quad 	0x2a00087
	.quad 	0x2c00087
	.quad 	0x2e00087
    .fill 488, 8, 0
.section .data

ENTRY(hal_init_gdtTbl)
    .quad 0x0000000000000000    /* NULL */
    .quad 0x0020980000000000    /* kernel code 64-bit */
    .quad 0x0000920000000000    /* kernel data 64-bit */
    .quad 0x0020980000000000    /* kernel code 64-bit */
    .quad 0x0000000000000000 
    .quad 0x0000000000000000
    .quad 0x0000f30000000000    /* user data 64-bit */
    .quad 0x0020fb0000000000    /* user code 64-bit */
    .quad 0x00cf9a000000ffff    /* kernel code 32-bit */
    .quad 0x00cf92000000ffff    /* kernel data 32-bit */
    .fill 100, 8, 0				/* TSS (jmp one segment <9>) in long-mode 128-bit 50*/
gdtEnd:

gdtPointer:
    gdtLimit:   .word gdtEnd - hal_init_gdtTbl - 1
    gdtBase:    .quad hal_init_gdtTbl

ENTRY(hal_init_idtTbl)
    .fill 512, 8, 0
idtEnd:

hal_init_idtPtr:
    idtLimit:   .word idtEnd - hal_init_idtTbl - 1
    idtBase:    .quad hal_init_idtTbl

ENTRY(hal_init_tss)
    .fill 13, 8, 0
tss64End:

tss64Pointer:
    tss64Limit: .word tss64End - hal_init_tss - 1
    tss64Base:  .quad hal_init_tss