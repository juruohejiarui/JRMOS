#ifndef __HAL_INTERRUPT_H__
#define __HAL_INTERRUPT_H__

#include <hal/interrupt/desc.h>
#include <interrupt/desc.h>
#include <hal/linkage.h>
#include <lib/string.h>

#define hal_intr_irqName(preffix, num) preffix##num(void)

#define hal_intr_saveAll \
    "pushq %rax     \n\t" \
    "pushq %rax     \n\t" \
    "movq %es, %rax \n\t" \
    "pushq %rax     \n\t" \
    "movq %ds, %rax \n\t" \
    "pushq %rax     \n\t" \
    "xorq %rax, %rax\n\t" \
    "pushq %rbp     \n\t" \
    "pushq %rdi     \n\t" \
    "pushq %rsi     \n\t" \
    "pushq %rdx     \n\t" \
    "pushq %rcx     \n\t" \
    "pushq %rbx     \n\t" \
    "pushq %r8      \n\t" \
    "pushq %r9      \n\t" \
    "pushq %r10     \n\t" \
    "pushq %r11     \n\t" \
    "pushq %r12     \n\t" \
    "pushq %r13     \n\t" \
    "pushq %r14     \n\t" \
    "pushq %r15     \n\t" \
    "movq $0x10, %rdx\n\t" \
    "movq %rdx, %ss \n\t" \
    "movq %rdx, %ds \n\t" \
    "movq %rdx, %es \n\t" \

#define hal_intr_buildIrq(preffix, num, dispatcher)   \
__noinline__ void hal_intr_irqName(preffix, num);      \
__asm__ ( \
    ".section .text     \n\t" \
    ".global "SYMBOL_NAME_STR(preffix)#num"    \n\t" \
    SYMBOL_NAME_STR(preffix)#num": \n\t" \
    "cli       			\n\t" \
    "pushq $0   		\n\t" \
    hal_intr_saveAll \
    "movq %rsp, %rdi        \n\t" \
    "movq $"#num", %rsi     \n\t" \
    "leaq hal_intr_retFromIntr(%rip), %rax 	\n\t" \
	"pushq %rax			\n\t" \
    "jmp "#dispatcher"   \n\t" \
); \


#define HAL_INTR_MASK
#define HAL_INTR_UNMASK
#define HAL_INTR_PAUSE
#define HAL_INTR_STATE

#define hal_intr_mask()     do { __asm__ volatile ("cli \n\t" : : : "memory"); } while(0)
#define hal_intr_unmask()   do { __asm__ volatile ("sti \n\t" : : : "memory"); } while(0)
#define hal_intr_pause()    do { __asm__ volatile ("hlt \n\t" : : : "memory"); } while(0)

#define hal_intr_state() ({ \
    u64 rflags; \
    __asm__ volatile (\
        "pushfq     \n\t" \
        "popq %0    \n\t" \
        : "=r"(rflags) \
        : \
        : "memory" \
    ); \
    (rflags >> 9) & 1; \
})

#define hal_intr_setGate(idtAddr, attr, istIndex, codeAddr) \
    do { \
        u64 d0, d1; \
        __asm__ volatile ( \
            /* set the 0...15-th bits of code addr */ \
            "movw %%dx, %%ax        \n\t" \
            /* set the ist index and attr */ \
            "andq $0x7, %%rcx       \n\t" \
            "addq %4, %%rcx         \n\t" \
            "shlq $32, %%rcx         \n\t" \
            "orq %%rcx, %%rax       \n\t" \
            /* set the 15...31-th bits of code addr to 48...63-th bits */ \
            "xorq %%rcx, %%rcx      \n\t" \
            "movl %%edx, %%ecx      \n\t" \
            "shrq $16, %%rcx        \n\t" \
            "shlq $48, %%rcx        \n\t" \
            "orq %%rcx, %%rax       \n\t" \
            "movq %%rax, %0         \n\t" \
            /* set the 32...63-th bits of code addr to 64...95-th bits */ \
            "shrq $32, %%rdx        \n\t" \
            "movq %%rdx, %1         \n\t" \
            :   "=m"(*(u64 *)(idtAddr)), "=m"(*(1 + (u64 *)(idtAddr))), \
                "=&a"(d0), "=&d"(d1) \
            :   "i"(attr << 8), \
                "3"((u64 *)(codeAddr)),  /*set rdx to be the code addr*/ \
                "2"(0x8 << 16), /*set the segment selector to be 0x8 (kernel code 64-bit)*/ \
                "c"(istIndex) \
            : "memory" \
        ); \
    } while(0)

static __always_inline__ void hal_intr_setIntrGate(hal_intr_IdtItem *idtTbl, u64 idtIdx, u8 istIdx, void *codeAddr) {
    hal_intr_setGate(idtTbl + idtIdx, 0x8E, istIdx, codeAddr);
}

static __always_inline__ void hal_intr_setTrapGate(hal_intr_IdtItem *idtTbl, u64 idtIdx, u8 istIdx, void *codeAddr) {
    hal_intr_setGate(idtTbl + idtIdx, 0x8F, istIdx, codeAddr);
}

static __always_inline__ void hal_intr_setSystemGate(hal_intr_IdtItem *idtTbl, u64 idtIdx, u8 istIdx, void *codeAddr) {
    hal_intr_setGate(idtTbl + idtIdx, 0xEF, istIdx, codeAddr);
}

static __always_inline__ void hal_intr_setSysIntrGate(hal_intr_IdtItem *idtTbl, u64 idtIdx, u8 istIdx, void *codeAddr) {
    hal_intr_setGate(idtTbl + idtIdx, 0xEE, istIdx, codeAddr);
}

#undef hal_intr_setGate

// vectors from 0x20 to 0x37 are reserved for apic or 8259A
extern void (*hal_intr_entryList[24])(void);
extern intr_Desc hal_intr_desc[24];

void hal_intr_setTss(hal_intr_TSS *tss, 
    u64 rsp0, u64 rsp1, u64 rsp2, u64 ist1, u64 ist2, u64 ist3, u64 ist4, u64 ist5, u64 ist6, u64 ist7);

// set tss item in the gdtTable
void hal_intr_setTssItem(u64 idx, hal_intr_TSS *tss);

void hal_intr_setTr(u16 idx);

static __always_inline__ void hal_intr_cpyTss(hal_intr_TSS *src, u32 *dst) { memcpy(src, dst, sizeof(hal_intr_TSS)); }

int hal_intr_init();
#endif