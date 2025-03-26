#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <hal/interrupt/desc.h>

int main() {
	FILE *file = fopen("./includes/hal/interrupt/genasm.h", "w");
	char buf[1024];
	sprintf(buf, "#ifndef __HAL_INTERRUPT_ASM_H__\n#define __HAL_INTERRUPT_ASM_H__\n\n");
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf,
		"#define hal_intr_PtReg_r15 %#lx\n"
		"#define hal_intr_PtReg_r14 %#lx\n"
		"#define hal_intr_PtReg_r13 %#lx\n"
		"#define hal_intr_PtReg_r12 %#lx\n"
		"#define hal_intr_PtReg_r11 %#lx\n"
		"#define hal_intr_PtReg_r10 %#lx\n"
		"#define hal_intr_PtReg_r9  %#lx\n"
		"#define hal_intr_PtReg_r8  %#lx\n"
		"#define hal_intr_PtReg_rbx %#lx\n"
		"#define hal_intr_PtReg_rcx %#lx\n"
		"#define hal_intr_PtReg_rdx %#lx\n"
		"#define hal_intr_PtReg_rsi %#lx\n"
		"#define hal_intr_PtReg_rdi %#lx\n"
		"#define hal_intr_PtReg_rbp %#lx\n"
		"#define hal_intr_PtReg_ds  %#lx\n"
		"#define hal_intr_PtReg_es  %#lx\n"
		"#define hal_intr_PtReg_rax %#lx\n"
		"#define hal_intr_PtReg_func %#lx\n"
		"#define hal_intr_PtReg_errCode %#lx\n"
		"#define hal_intr_PtReg_rip %#lx\n"
		"#define hal_intr_PtReg_cs  %#lx\n"
		"#define hal_intr_PtReg_rflags %#lx\n"
		"#define hal_intr_PtReg_rsp %#lx\n"
		"#define hal_intr_PtReg_ss  %#lx\n\n",
		offsetof(hal_intr_PtReg, r15),
		offsetof(hal_intr_PtReg, r14),
		offsetof(hal_intr_PtReg, r13),
		offsetof(hal_intr_PtReg, r12),
		offsetof(hal_intr_PtReg, r11),
		offsetof(hal_intr_PtReg, r10),
		offsetof(hal_intr_PtReg, r9),
		offsetof(hal_intr_PtReg, r8),
		offsetof(hal_intr_PtReg, rbx),
		offsetof(hal_intr_PtReg, rcx),
		offsetof(hal_intr_PtReg, rdx),
		offsetof(hal_intr_PtReg, rsi),
		offsetof(hal_intr_PtReg, rdi),
		offsetof(hal_intr_PtReg, rbp),
		offsetof(hal_intr_PtReg, ds),
		offsetof(hal_intr_PtReg, es),
		offsetof(hal_intr_PtReg, rax),
		offsetof(hal_intr_PtReg, func),
		offsetof(hal_intr_PtReg, errCode),
		offsetof(hal_intr_PtReg, rip),
		offsetof(hal_intr_PtReg, cs),
		offsetof(hal_intr_PtReg, rflags),
		offsetof(hal_intr_PtReg, rsp),
		offsetof(hal_intr_PtReg, ss));
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, "#endif\n");
	fwrite(buf, strlen(buf), 1, file);
	fclose(file);
	return 0;
}