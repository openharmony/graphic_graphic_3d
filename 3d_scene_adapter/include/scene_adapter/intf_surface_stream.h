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

#ifndef OHOS_RENDER_3D_INTF_SURFACE_STREAM_H
#define OHOS_RENDER_3D_INTF_SURFACE_STREAM_H

#include <base/math/matrix.h>
#include <base/util/color.h>
#include <base/util/formats.h>
#include <meta/base/interface_macros.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>
#include <scene/base/types.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_render_target.h>

namespace OHOS::Render3D {

class SurfaceStream;
class ISurfaceStream : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISurfaceStream, "af900811-a8df-4b32-b8f0-1f92e146e527")
public:
    virtual void SetWidth(uint32_t width) = 0;
    virtual uint32_t GetWidth() const = 0;
    virtual void SetHeight(uint32_t height) = 0;
    virtual uint32_t GetHeight() const = 0;

    virtual uint64_t GetSurfaceId() const = 0;
};

META_REGISTER_CLASS(SurfaceStream, "961433c5-ae97-423e-9a9d-d8ef3063618b", META_NS::ObjectCategoryBits::NO_CATEGORY)

}
#endif // OHOS_RENDER_3D_INTF_SURFACE_STREAM_H