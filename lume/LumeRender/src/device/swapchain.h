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

#ifndef DEVICE_SWAPCHAIN_H
#define DEVICE_SWAPCHAIN_H

#include <cstdint>

#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct GpuImageDesc;

class Swapchain {
public:
    Swapchain() = default;
    virtual ~Swapchain() = default;

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    virtual const GpuImageDesc& GetDesc() const = 0;
    virtual const GpuImageDesc& GetDescDepthBuffer() const = 0;

    virtual uint32_t GetFlags() const = 0;
    virtual SurfaceTransformFlags GetSurfaceTransformFlags() const = 0;
    virtual uint64_t GetSurfaceHandle() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_SWAPCHAIN_H
