/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: opendir, readdir, closedir implementation for windows.
 * Author: Jani Kattelus
 * Create: 2019-10-23
 */
#ifndef __LUME_DIR__
#define __LUME_DIR__
#if _WIN32
/*
 * File types
 */
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12

struct dirent {
    unsigned char d_type;
    char d_name[256];
};
struct DIR;
DIR* opendir(const char* path);
struct dirent* readdir(DIR* d);
void closedir(DIR* d);
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#endif
#endif