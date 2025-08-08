#ifndef __SERVICE_API_H__
#define __SERVICE_API_H__

#include <service/desc.h>

void service_init();

int service_Server_init(service_Server *server, char *name, task_TaskStruct *tsk, u64 ringSz);

int service_Server_register(service_Server *server);

// unregister the host, return 0 if success, -1 if failed
// the host will not be removed if there are still clients connected to it
int service_Server_unregister(service_Server *server);

// find the registered host with specific name and connect to it
// return the pointer to the host if we have found the host
// return NULL if we have not found the host
service_Server *service_connect(char *name);

int service_disconnect(service_Server *server);

int service_send(service_Server *server, void *data);

service_Server *syscall_service_connect(char *name);

int syscall_service_disconnect(service_Server *server);

service_Request *syscall_service_send(service_Server *server, void *data);

#endif