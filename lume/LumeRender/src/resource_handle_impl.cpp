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

#include "resource_handle_impl.h"

#include "device/gpu_resource_handle_util.h"

RENDER_BEGIN_NAMESPACE()
void RenderReferenceCounter::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderReferenceCounter::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderReferenceCounter::GetRefCount() const
{
    return refcnt_.load();
}
RENDER_END_NAMESPACE()
