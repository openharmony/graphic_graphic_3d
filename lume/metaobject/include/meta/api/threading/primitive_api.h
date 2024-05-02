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