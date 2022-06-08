/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
