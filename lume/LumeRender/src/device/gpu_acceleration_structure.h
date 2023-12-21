/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef DEVICE_GPU_ACCELERATION_STRUCTURE_H
#define DEVICE_GPU_ACCELERATION_STRUCTURE_H

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct GpuAccelerationStructureDesc;

class GpuAccelerationStructure {
public:
    GpuAccelerationStructure() = default;
    virtual ~GpuAccelerationStructure() = default;

    GpuAccelerationStructure(const GpuAccelerationStructure&) = delete;
    GpuAccelerationStructure& operator=(const GpuAccelerationStructure&) = delete;

    virtual const GpuAccelerationStructureDesc& GetDesc() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_ACCELERATION_STRUCTURE_H
