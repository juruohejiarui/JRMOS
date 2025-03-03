#ifndef __HAL_INTERRUPT_DESC_H__
#define __HAL_INTERRUPT_DESC_H__

#include <lib/dtypes.h>

#define hal_irqName2(preffix, num) hal_##preffix##num##Interrupt(void)
#define hal_irqName(preffix, num) hal_irqName2(preffix, num)

#define hal_intr_buildIrq(preffix, num, dispatcher)   \
__noinline__ void irqName(preffix, num);      \
__asm__ ( \
    ".section .text     \n\t" \
    ".global "SYMBOL_NAME_STR(preffix)#num"Intr    \n\t" \
    SYMBOL_NAME_STR(preffix)#num"Intr: \n\t" \
    "cli       			\n\t" \
    "pushq $0   		\n\t" \
    saveAll \
    "movq %rsp, %rdi        \n\t" \
    "movq $"#num", %rsi     \n\t" \
    "leaq hal_intr_retFromIntr(%rip), %rax 	\n\t" \
	"pushq %rax			\n\t" \
    "jmp "#dispatcher"   \n\t" \
); \

#undef hal_irqName2
#undef hal_irqName

#endif