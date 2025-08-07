#ifndef __SERVICE_DESC_H__
#define __SERVICE_DESC_H__

#include <task/structs.h>
#include <task/syscall.h>

typedef struct service_Request {
	task_TaskStruct *clientTsk;
	// the request id, used to identify the request
	u64 reqId;
	Atomic flags;
	#define service_Request_flags_Pending 	0x1
	#define service_Request_flags_Handled 	0x2
} service_Request;

typedef struct service_Host {
	char name[512];
	// crc32 of name
	u32 nameHash;

	u64 serviceId;

	u64 attr;
	// this host is a user host, which means the request is handled by user space
	#define service_Request_flags_UserHost 0x1

	task_TaskStruct *hostTsk;
	
	Atomic openCnt;

	RBNode rbNd;
	
	// use ring to store request pointer, size of ring is specific for each service
	SpinLock lck;
	u64 hdr, til, sz, load;
	service_Request *reqs;
} service_Host;

#endif