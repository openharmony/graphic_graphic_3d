/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_RESOURCE_HANDLE_IMPL_H
#define RENDER_RESOURCE_HANDLE_IMPL_H

#include <atomic>
#include <cstdint>

#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class RenderReferenceCounter final : public IRenderReferenceCounter {
public:
    RenderReferenceCounter() = default;
    ~RenderReferenceCounter() override = default;

    RenderReferenceCounter(const RenderReferenceCounter&) = delete;
    RenderReferenceCounter& operator=(const RenderReferenceCounter&) = delete;

    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() const override;

private:
    std::atomic<int32_t> refcnt_ { 0 };
};
RENDER_END_NAMESPACE()
#endif // RENDER_RESOURCE_HANDLE_IMPL_H
