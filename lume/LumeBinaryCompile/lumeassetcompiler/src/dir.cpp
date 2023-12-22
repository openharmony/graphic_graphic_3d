/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
