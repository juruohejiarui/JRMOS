#include <service/api.h>
#include <lib/string.h>
#include <lib/hash.h>
#include <mm/mm.h>
#include <screen/screen.h>

__always_inline__ int service_serverTr_cmp(RBNode *a, RBNode *b) {
    service_Server *ta = container(a, service_Server, rbNd),
        *tb = container(b, service_Server, rbNd);
    return ta->nameHash != tb->nameHash ? ta->nameHash < tb->nameHash : strcmp(ta->name, tb->name) < 0;
}
__always_inline__ int service_serverTr_match(RBNode *a, u32 hash, char *name) {
    service_Server *ta = container(a, service_Server, rbNd);
    if (ta->nameHash != hash) return ta->nameHash < hash ? -1 : 1;
    return strcmp(ta->name, name);
}

RBTree_insertDef(service_serverTr_ins, service_serverTr_cmp);

static RBTree service_serverTr;

void service_init() {
    RBTree_init(&service_serverTr, service_serverTr_ins, NULL);
}

int service_Server_init(service_Server *server, char *name, task_TaskStruct *tsk, u64 ringSz) {
    server->nameHash = crc32(name, strlen(name));
    memcpy(server->name, name, strlen(name) + 1);

    server->reqs = mm_kmalloc(sizeof(service_Request *) * ringSz, mm_Attr_Shared, NULL);
    if (server->reqs == NULL) {
        printk(screen_err, "server: failed to allocate request ring for server %p\n", server);
        return res_FAIL | res_DAMAGED;
    }
    memset(server->reqs, 0, sizeof(service_Request *) * ringSz);
    server->sz = ringSz;
    server->hdr = server->til = server->load = 0;
    SpinLock_init(&server->lck);

    server->tsk = tsk;

    server->connectCnt.value = 0;
    return res_SUCC;
}

int service_Server_register(service_Server *server) {
    RBTree_insLck(&service_serverTr, &server->rbNd);
    return res_SUCC;
}

int service_Server_unregister(service_Server *server) {
    RBTree_lck(&service_serverTr);
    if (server->connectCnt.value > 0) {
        RBTree_unlck(&service_serverTr);
        return res_FAIL | res_BUSY;
    }
    RBTree_del(&service_serverTr, &server->rbNd);
    RBTree_unlck(&service_serverTr);
    return res_SUCC;
}

service_Server *service_connect(char *name) {
    u32 hash = crc32(name, strlen(name));
    RBTree_lck(&service_serverTr);
    RBNode *nd = service_serverTr.root;
    service_Server *server = NULL;
    while (nd != NULL) {
        register int res = service_serverTr_match(nd, hash, name);
        if (!res) {
            server = container(nd, service_Server, rbNd);
            break;
        }
        if (res == -1) nd = nd->left;
        else nd = nd->right;
    }
    Atomic_inc(&server->connectCnt);
    RBTree_unlck(&service_serverTr);
    return server;
}

int service_disconnect(service_Server *server) {
    Atomic_dec(&server->connectCnt);
    return res_SUCC;
}