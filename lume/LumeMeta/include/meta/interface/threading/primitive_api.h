/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Synchronisation primitive types
 * Author: Mikael Kilpeläinen
 * Create: 2023-06-09
 */

#ifndef META_INTERFACE_THREADING_PRIMITIVE_API_H
#define META_INTERFACE_THREADING_PRIMITIVE_API_H

#include <stdint.h>

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()

/// Mutex type
enum class MutexType {
    NORMAL,
    // RECURSIVE,
    // SHARED
};

#ifdef _WIN32
static constexpr size_t MUTEX_HANDLE_STORAGE_SIZE = 8;
#else
static constexpr size_t MUTEX_HANDLE_STORAGE_SIZE = 40;
#endif

struct alignas(8) MutexHandle {
    union {
        void* ptr = nullptr;
        char storage[MUTEX_HANDLE_STORAGE_SIZE];
    };
};

using CreateMutexType = void (*)(MutexType, MutexHandle&);
using DestroyMutexType = void (*)(MutexHandle&);
using LockMutexType = bool (*)(MutexHandle&);
using UnlockMutexType = bool (*)(MutexHandle&);

using NativeThreadHandle = uint64_t;
using GetThreadIdType = NativeThreadHandle (*)();

/// Synchronisation api which can be passed over dynamic library boundaries
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