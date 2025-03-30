#include <hal/linkage.h>

.balign 0x1000

.text
.code16

ENTRY(hal_cpu_apBootEntry)

hal_cpu_apBootBase = .
_tmp:
	cli

	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs

	movl $(hal_cpu_apBootTmpStkEnd - hal_cpu_apBootBase), %esp
	
	mov 	%cs, %ax
	movzx 	%ax, %esi
	shll 	$4, %esi

	leal (hal_cpu_apCode32 - hal_cpu_apBootBase)(%esi), %eax
	movl %eax, hal_cpu_apCode32Vector - hal_cpu_apBootBase
	
	leal (hal_cpu_apCode64 - hal_cpu_apBootBase)(%esi), %eax
	movl %eax, hal_cpu_apCode64Vector - hal_cpu_apBootBase

	leal (hal_cpu_apTmpGDT - hal_cpu_apBootBase)(%esi), %eax
	movl %eax, (hal_cpu_apTmpGDT + 2 - hal_cpu_apBootBase)

	lidtl hal_cpu_apTmpIDT - hal_cpu_apBootBase
	lgdtl hal_cpu_apTmpGDT - hal_cpu_apBootBase
 
	smsw %ax
	bts $0, %ax
	lmsw %ax

	ljmpl *(hal_cpu_apCode32Vector - hal_cpu_apBootBase)

.code32
.balign 4

hal_cpu_apCode32:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs

	leal (hal_cpu_apBootTmpStkEnd - hal_cpu_apBootBase)(%esi), %eax
	movl %eax, %esp

	// enable PAE
	movl %cr4, %eax
	bts $5, %eax
	movl %eax, %cr4

	// set page table
	movl $0x101000, %eax
	movl %eax, %cr3


	// enable long mode
	movl $0xC0000080, %ecx
	rdmsr

	bts $8, %eax
	bts $0, %eax
	wrmsr

	// enable PE and paging
	movl %cr0, %eax
	bts $0, %eax
	bts $31, %eax
	movl %eax, %cr0

	ljmp *(hal_cpu_apCode64Vector - hal_cpu_apBootBase)(%esi)

.code64
.balign 4
hal_cpu_apCode64:
	movq $0x10, %rax
	movq %rax, %ds
	movq %rax, %es
	movq %rax, %fs
	movq %rax, %gs
	movq %rax, %ss

	// jmp to head.S
	movq $0x100000, %rax
	jmpq *%rax

	hlt

.balign 4
hal_cpu_apTmpIDT:
	.word 0
	.word 0, 0

.balign 4
hal_cpu_apTmpGDT:
	.short hal_cpu_apTmpGDTEnd - hal_cpu_apTmpGDT - 1
	.long hal_cpu_apTmpGDT - hal_cpu_apBootBase
	.short 0
	.quad 0x00cf9a000000ffff
	.quad 0x00cf92000000ffff
	.quad 0x0020980000000000
	.quad 0x0000920000000000
hal_cpu_apTmpGDTEnd:

.balign 4
hal_cpu_apCode32Vector:
	.long hal_cpu_apCode32 - hal_cpu_apBootBase
	.word 0x08, 0

.balign 4
hal_cpu_apCode64Vector:
	.long hal_cpu_apCode64 - hal_cpu_apBootBase
	.word 0x18, 0

.balign 4
hal_cpu_apBootTmpStkStart:
	.org 0x400
hal_cpu_apBootTmpStkEnd:

ENTRY(hal_cpu_apBootEnd)
	