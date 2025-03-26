#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <task/structs.h>

int main() {
	FILE *file = fopen("./includes/hal/task/genasm.h", "w");
	char buf[1024];
	sprintf(buf, "#ifndef __HAL_TASK_ASM_H__\n#define __HAL_TASK_ASM_H__\n\n");
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, 
		"#define task_TaskStruct_state  %#lx\n"
		"#define task_TaskStruct_hal    %#lx\n"
		"#define task_TaskStruct_thread %#lx\n"
		"#define hal_task_TaskStruct_rip    %#lx\n"
		"#define hal_task_TaskStruct_rsp    %#lx\n"
		"#define hal_task_TaskStruct_rsp2    %#lx\n"
		"#define hal_task_TaskStruct_rflags %#lx\n\n",
		offsetof(task_TaskStruct, state),
		offsetof(task_TaskStruct, hal),
		offsetof(task_TaskStruct, thread),
		offsetof(task_TaskStruct, hal) + offsetof(hal_task_TaskStruct, rip),
		offsetof(task_TaskStruct, hal) + offsetof(hal_task_TaskStruct, rsp),
		offsetof(task_TaskStruct, hal) + offsetof(hal_task_TaskStruct, rsp2),
		offsetof(task_TaskStruct, hal) + offsetof(hal_task_TaskStruct, rflags));
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, 
		"#define task_ThreadStruct_hal %#lx\n"
		"#define hal_task_ThreadStruct_pgd %#lx\n\n",
		offsetof(task_ThreadStruct, hal),
		offsetof(task_ThreadStruct, hal) + offsetof(hal_task_ThreadStruct, pgd));
	fwrite(buf, strlen(buf), 1, file);

	sprintf(buf, 
		"#define task_UsrStruct_tsk	%#lx\n"
		"#define hal_task_UsrStruct_usrFs %#lx\n"
		"#define hal_task_UsrStruct_usrGs %#lx\n"
		"#define hal_task_UsrStruct_krlFs %#lx\n"
		"#define hal_task_UsrStruct_krlGs %#lx\n\n",
		offsetof(task_UsrStruct, tsk),
		offsetof(task_UsrStruct, hal) + offsetof(hal_task_UsrStruct, usrFs),
		offsetof(task_UsrStruct, hal) + offsetof(hal_task_UsrStruct, usrGs),
		offsetof(task_UsrStruct, hal) + offsetof(hal_task_UsrStruct, krlFs),
		offsetof(task_UsrStruct, hal) + offsetof(hal_task_UsrStruct, krlGs));
	fwrite(buf, strlen(buf), 1, file);

	sprintf(buf,
		"#define task_usrStkSize %#lx\n"
		"#define task_krlStkSize %#lx\n\n",
		task_usrStkSize,
		task_krlStkSize);
	fwrite(buf, strlen(buf), 1, file);

	sprintf(buf, 
		"#define task_state_Running			%#lx\n"
		"#define task_state_NeedSchedule	%#lx\n"
		"#define task_state_Idle			%#lx\n\n",
		task_state_Running,
		task_state_NeedSchedule,
		task_state_Idle);
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, "#endif\n");
	fwrite(buf, strlen(buf), 1, file);
	fclose(file);
	return 0;
}