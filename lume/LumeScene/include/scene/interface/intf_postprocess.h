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

#ifndef SCENE_INTERFACE_IPOSTPROCESS_H
#define SCENE_INTERFACE_IPOSTPROCESS_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/postprocess/intf_blur.h>
#include <scene/interface/postprocess/intf_color_conversion.h>
#include <scene/interface/postprocess/intf_color_fringe.h>
#include <scene/interface/postprocess/intf_dof.h>
#include <scene/interface/postprocess/intf_fxaa.h>
#include <scene/interface/postprocess/intf_lens_flare.h>
#include <scene/interface/postprocess/intf_motion_blur.h>
#include <scene/interface/postprocess/intf_taa.h>
#include <scene/interface/postprocess/intf_tonemap.h>
#include <scene/interface/postprocess/intf_upscale.h>
#include <scene/interface/postprocess/intf_vignette.h>

SCENE_BEGIN_NAMESPACE()

class IPostProcess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPostProcess, "a0b91095-0383-40f8-a211-5735ca5440f9")
public:
    /**
     * @brief Camera postprocessing settings, tonemap
     */
    META_READONLY_PROPERTY(ITonemap::Ptr, Tonemap)
    /**
     * @brief Camera postprocessing settings, bloom
     */
    META_READONLY_PROPERTY(IBloom::Ptr, Bloom)
    /**
     * @brief Camera postprocessing settings, blur
     */
    META_READONLY_PROPERTY(IBlur::Ptr, Blur)
    /**
     * @brief Camera postprocessing settings, motion blur
     */
    META_READONLY_PROPERTY(IMotionBlur::Ptr, MotionBlur)
    /**
     * @brief Camera postprocessing settings, color conversion
     */
    META_READONLY_PROPERTY(IColorConversion::Ptr, ColorConversion)
    /**
     * @brief Camera postprocessing settings, color fringe
     */
    META_READONLY_PROPERTY(IColorFringe::Ptr, ColorFringe)
    /**
     * @brief Camera postprocessing settings, depth of field
     */
    META_READONLY_PROPERTY(IDepthOfField::Ptr, DepthOfField)
    /**
     * @brief Camera postprocessing settings, fxaa
     */
    META_READONLY_PROPERTY(IFxaa::Ptr, Fxaa)
    /**
     * @brief Camera postprocessing settings, taa
     */
    META_READONLY_PROPERTY(ITaa::Ptr, Taa)
    /**
     * @brief Camera postprocessing settings, vignette
     */
    META_READONLY_PROPERTY(IVignette::Ptr, Vignette)
    /**
     * @brief Camera postprocessing settings, lens flare
     */
    META_READONLY_PROPERTY(ILensFlare::Ptr, LensFlare)
    /**
     * @brief Camera postprocessing settings, upscale
     */
    META_READONLY_PROPERTY(IUpscale::Ptr, Upscale)
};

META_REGISTER_CLASS(PostProcess, "2df6c964-5a32-4270-86e6-755e3f5697ff", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IPostProcess)

#endif
