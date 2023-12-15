/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: opendir, readdir, closedir implementation for windows.
 * Author: Jani Kattelus
 * Create: 2019-10-23
 */
#include "dir.h"

#if _WIN32
#include <windows.h>
#include <string>
struct DIR {
    HANDLE handle;
    WIN32_FIND_DATAA data;
    dirent cur;
};
DIR* opendir(const char* path)
{
    DIR* d = new DIR();
    std::string tmp = path;
    tmp.append("\\");
    tmp.append("*.*");
    d->handle = FindFirstFileA(tmp.c_str(), &d->data);
    if (d->handle == INVALID_HANDLE_VALUE) {
        delete d;
        return nullptr;
    }
    return d;
}
struct dirent* readdir(DIR* d)
{
    if (d->data.cFileName[0] == 0) {
        // end.
        return nullptr;
    }
    strcpy(d->cur.d_name, d->data.cFileName);
    if (d->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        d->cur.d_type = DT_DIR;
    } else {
        d->cur.d_type = DT_REG;
    }
    if (false == FindNextFileA(d->handle, &d->data)) {
        // clear;
        d->data = {};
    }
    return &d->cur;
}
void closedir(DIR* d)
{
    FindClose(d->handle);
}
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#endif
