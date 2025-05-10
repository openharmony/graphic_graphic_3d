/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Synchronisation primitive types
 * Author: Mikael Kilpeläinen
 * Create: 2023-06-09
 */

#ifndef META_API_THREADING_PRIMITIVE_API_H
#define META_API_THREADING_PRIMITIVE_API_H

#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/threading/primitive_api.h>

CORE_BEGIN_NAMESPACE()

inline const SyncApi& GetSyncApi()
{
    return META_NS::GetMetaObjectLib().GetSyncApi();
}

struct ThreadId {
    ThreadId() = default;
    ThreadId(NativeThreadHandle handle) : handle_(handle) {}

    bool operator==(const ThreadId& tid) const
    {
        return handle_ == tid.handle_;
    }

    bool operator!=(const ThreadId& tid) const
    {
        return !(*this == tid);
    }

private:
    NativeThreadHandle handle_ {};
};

inline ThreadId CurrentThreadId()
{
    return ThreadId { GetSyncApi().getThreadId() };
}

CORE_END_NAMESPACE()

#endif