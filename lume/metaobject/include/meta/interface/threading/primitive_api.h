/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef META_INTERFACE_THREADING_PRIMITIVE_API_H
#define META_INTERFACE_THREADING_PRIMITIVE_API_H

#include <stdint.h>

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()

enum class MutexType {
    NORMAL,
    // RECURSIVE,
    // SHARED
};

struct alignas(8) MutexHandle { // 8 alignment size
    union {
        void* ptr = nullptr;
        char storage[40];
    };
};

using CreateMutexType = void (*)(MutexType, MutexHandle&);
using DestroyMutexType = void (*)(MutexHandle&);
using LockMutexType = bool (*)(MutexHandle&);
using UnlockMutexType = bool (*)(MutexHandle&);

using NativeThreadHandle = uint64_t;
using GetThreadIdType = NativeThreadHandle (*)();

struct SyncApi {
    struct MutexApi {
        CreateMutexType create = nullptr;
        DestroyMutexType destroy = nullptr;
        LockMutexType lock = nullptr;
        UnlockMutexType unlock = nullptr;
    } mutex;

    GetThreadIdType getThreadId = nullptr;
};

CORE_END_NAMESPACE()

#endif