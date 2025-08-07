#include "inner.h"
#include <fs/fat32/api.h>
#include <service/desc.h>

typedef struct fs_fat32_ServiceHost {
    service_Host host;
    fs_fat32_Partition *par;
} fs_fat32_ServiceHost;
