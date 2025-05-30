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

#ifndef DEVICE_GPU_SEMAPHORE_H
#define DEVICE_GPU_SEMAPHORE_H

#include <cstdint>

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** Gpu semaphore.
 */
class GpuSemaphore {
public:
    GpuSemaphore() = default;
    virtual ~GpuSemaphore() = default;

    GpuSemaphore(const GpuSemaphore&) = delete;
    GpuSemaphore& operator=(const GpuSemaphore&) = delete;

    virtual uint64_t GetHandle() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_SEMAPHORE_H
