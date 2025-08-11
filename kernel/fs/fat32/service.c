#include "inner.h"
#include <fs/fat32/api.h>
#include <service/desc.h>

typedef struct fs_fat32_Server {
    service_Server server;
    #define fs_Service_RequestType_write      0x1
    #define fs_Service_RequestType_read       0x2
    #define fs_Service_RequestType_seek       0x3
    #define fs_Service_RequestType_open       0x4
    #define fs_Service_RequestType_close      0x5
    #define fs_Service_RequestType_openDir    0x6
    #define fs_Service_RequestType_nxtEnt     0x7
    #define fs_Service_RequestType_delEnt     0x8
    fs_fat32_Partition *par;
} fs_fat32_Server;

typedef struct fs_fat32_reqpak_RW {
    fs_fat32_File *file;
    void *data;
    u64 size;
} fs_fat32_reqpak_RW;

typedef struct fs_fat32_reqpak_Seek {
    fs_fat32_File *file;
    u64 ptr;
} fs_fat32_reqpak_Seek;

typedef struct fs_fat32_reqpak_Open {
    fs_fat32_File *file;
    u8 *path;
} fs_fat32_reqpak_Open;

typedef struct fs_fat32_reqpak_Close {
    fs_fat32_File *file;
} fs_fat32_reqpak_Close;

typedef struct fs_fat32_reqpak_OpenDir {
    fs_fat32_Dir *dir;
    u8 *path;
} fs_fat32_reqpak_OpenDir;