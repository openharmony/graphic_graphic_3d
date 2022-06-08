/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
