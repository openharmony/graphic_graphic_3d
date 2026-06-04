/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MALEOON_GPU_IMAGE_MLN_H
#define MALEOON_GPU_IMAGE_MLN_H

#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_device.h>
#include <render/namespace.h>
#include <render/maleoon/intf_device_mln.h>

#include "device/gpu_image.h"

RENDER_BEGIN_NAMESPACE()

class Device;

struct GpuImagePlatformDataViewsMln final {
    BASE_NS::vector<MlnResourceView> mipImageViews;
    BASE_NS::vector<MlnResourceView> mipImageAllLayerViews;
    BASE_NS::vector<MlnResourceView> layerImageViews;
};

class GpuImageMln final : public GpuImage {
public:
    GpuImageMln(Device& device, const GpuImageDesc& desc);
    GpuImageMln(Device& device, const GpuImageDesc& desc, const GpuImagePlatformData& platformData);
    ~GpuImageMln() override;

    const GpuImageDesc& GetDesc() const override;
    const GpuImagePlatformData& GetBasePlatformData() const override;
    AdditionalFlags GetAdditionalFlags() const override;

    const GpuImagePlatformDataMln& GetPlatformData() const;
    const GpuImagePlatformDataViewsMln& GetPlatformDataViews() const;

private:
    void CreateImage();
    void CreateImageViews();
    void AllocateAndBindMemory();
    uint32_t FindMemoryType(uint32_t memoryTypeBits, MlnMemoryPropertyFlags requiredFlags) const;

    Device& device_;
    GpuImageDesc desc_;
    GpuImagePlatformDataMln plat_;
    GpuImagePlatformDataViewsMln platViews_;

    bool ownsResources_ { true };
    bool ownsImage_ { true };
    bool ownsImageViews_ { true };
};

RENDER_END_NAMESPACE()

#endif // MALEOON_GPU_IMAGE_MLN_H
