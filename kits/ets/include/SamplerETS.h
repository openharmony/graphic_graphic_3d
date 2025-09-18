/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_SAMPLER_ETS_H
#define OHOS_3D_SAMPLER_ETS_H

#include <meta/api/util.h>
#include <scene/interface/intf_texture.h>

namespace OHOS::Render3D {
constexpr auto DEFAULT_FILTER = SCENE_NS::SamplerFilter::NEAREST;
constexpr auto DEFAULT_ADDESS_MODE = SCENE_NS::SamplerAddressMode::REPEAT;

class SamplerETS {
public:
    SamplerETS();
    SamplerETS(const SCENE_NS::ISampler::Ptr sampler);
    ~SamplerETS();

    SCENE_NS::SamplerFilter GetMagFilter();
    void SetMagFilter(const SCENE_NS::SamplerFilter filter);

    SCENE_NS::SamplerFilter GetMinFilter();
    void SetMinFilter(const SCENE_NS::SamplerFilter filter);

    SCENE_NS::SamplerFilter GetMipMapMode();
    void SetMipMapMode(const SCENE_NS::SamplerFilter filter);

    SCENE_NS::SamplerAddressMode GetAddressModeU();
    void SetAddressModeU(const SCENE_NS::SamplerAddressMode mode);

    SCENE_NS::SamplerAddressMode GetAddressModeV();
    void SetAddressModeV(const SCENE_NS::SamplerAddressMode mode);

private:
    SCENE_NS::ISampler::Ptr sampler_;
    SCENE_NS::SamplerFilter magFilter_;
    SCENE_NS::SamplerFilter minFilter_;
    SCENE_NS::SamplerFilter mipMapMode_;
    SCENE_NS::SamplerAddressMode addressModeU_;
    SCENE_NS::SamplerAddressMode addressModeV_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_SAMPLER_ETS_H