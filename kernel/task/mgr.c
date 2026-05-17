#include "inner.h"

void _mgrTask(void *arg) {
    while (1) {
        while (__unlikely__(!SafeList_isEmpty(&task_freeThds))) {

        }
        
    }
}

void _mgr_init() {
    _freeMgrThd = task_newThd(_mgrTask, NULL, task_attr_Builtin, task_rootProc);
}