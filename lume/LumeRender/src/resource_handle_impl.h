/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
