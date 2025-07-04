#ifndef __SERVICE_API_H__
#define __SERVICE_API_H__

#include <service/desc.h>

void service_init();

int service_Host_init(char *name, task_TaskStruct *hostTsk, u64 ringSz);

int service_Host_register(service_Host *host);

// unregister the host, return 0 if success, -1 if failed
// the host will not be removed if there are still clients connected to it
int service_Host_unregister(service_Host *host);

// find the registered host with specific name and connect to it
// return the pointer to the host if we have found the host
// return NULL if we have not found the host
service_Host *service_Host_connect(char *name);

int service_Host_disconnect(service_Host *host);

service_Host *syscall_service_Host_connect(char *name);

int syscall_service_Host_disconnect(service_Host *host);

#endif