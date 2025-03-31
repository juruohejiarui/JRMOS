#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <cpu/desc.h>

int main() {
	FILE *file = fopen("./includes/hal/cpu/genasm.h", "w");
	char buf[1024];
	sprintf(buf, "#ifndef __HAL_CPU_ASM_H__\n#define __HAL_CPU_ASM_H__\n\n");
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, 
		"#define cpu_DescSize   %#lx\n"
		"#define cpu_Desc_sirqFlag		%#lx\n"
		"#define hal_cpu_Desc_x2apic    %#lx\n"
		"#define hal_cpu_Desc_apic      %#lx\n"
        "#define hal_cpu_Desc_initStk   %#lx\n"
		"#define hal_cpu_Desc_curKrlStk %#lx\n"
        "#define hal_cpu_Desc_idtPtr    %#lx\n\n",
		sizeof(cpu_Desc),
		offsetof(cpu_Desc, sirqFlag),
		offsetof(cpu_Desc, hal) + offsetof(hal_cpu_Desc, x2apic),
		offsetof(cpu_Desc, hal) + offsetof(hal_cpu_Desc, apic),
        offsetof(cpu_Desc, hal) + offsetof(hal_cpu_Desc, initStk),
		offsetof(cpu_Desc, hal) + offsetof(hal_cpu_Desc, curKrlStk),
        offsetof(cpu_Desc, hal) + offsetof(hal_cpu_Desc, idtTblSz));
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, "#endif\n");
	fwrite(buf, strlen(buf), 1, file);
	fclose(file);
	return 0;
}