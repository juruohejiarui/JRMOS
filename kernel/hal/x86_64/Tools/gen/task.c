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
		"#define task_Thread_state  %#lx\n"
		"#define task_Thread_signal	%#lx\n"
		"#define task_Thread_signalHandle %#lx\n"
		"#define task_Thread_hal    %#lx\n"
		"#define task_Thread_thread %#lx\n"
		"#define hal_task_Thread_rip    %#lx\n"
		"#define hal_task_Thread_rsp    %#lx\n"
		"#define hal_task_Thread_usrRsp %#lx\n"
		"#define hal_task_Thread_rflags %#lx\n\n",
		offsetof(task_Thread, state),
		offsetof(task_Thread, signal),
		offsetof(task_Thread, signalHandle),
		offsetof(task_Thread, hal),
		offsetof(task_Thread, proc),
		offsetof(task_Thread, hal) + offsetof(hal_task_Thread, rip),
		offsetof(task_Thread, hal) + offsetof(hal_task_Thread, rsp),
		offsetof(task_Thread, hal) + offsetof(hal_task_Thread, usrRsp),
		offsetof(task_Thread, hal) + offsetof(hal_task_Thread, rflags));
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, 
		"#define task_Process_hal %#lx\n"
		"#define hal_task_Process_pgd %#lx\n\n",
		offsetof(task_Process, hal),
		offsetof(task_Process, mem) + offsetof(task_MemStruct, hal) + offsetof(hal_task_MemStruct, pgd));
	fwrite(buf, strlen(buf), 1, file);

	sprintf(buf,
		"#define task_usrStkSize %#llx\n"
		"#define task_krlStkSize %#lx\n\n",
		task_usrStkSize,
		task_krlStkSize);
	fwrite(buf, strlen(buf), 1, file);

	sprintf(buf, 
		"#define task_state_Running			%#x\n"
		"#define task_state_NeedSchedule	%#x\n"
		"#define task_state_Idle			%#x\n\n",
		task_state_Running,
		task_state_NeedSchedule,
		task_state_Idle);
	fwrite(buf, strlen(buf), 1, file);
	sprintf(buf, "#endif\n");
	fwrite(buf, strlen(buf), 1, file);
	fclose(file);
	return 0;
}