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

#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_text.h>
#include <scene/interface/intf_texture.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/postprocess/intf_tonemap.h>
#include <scene/interface/resource/image_info.h>

#include <meta/base/meta_types.h>
#include <meta/ext/any_builder.h>
#include <meta/interface/detail/any.h>

SCENE_BEGIN_NAMESPACE()

// clang-format off
using BasicTypes = META_NS::TypeList<
    EnvBackgroundType,
    SamplerFilter,
    SamplerAddressMode,
    MaterialType,
    LightingFlags,
    RenderSort,
    ImageLoadFlags,
    CameraProjection,
    CameraCulling,
    CameraPipeline,
    CameraPipelineFlag,
    CameraSceneFlag,
    ColorFormat,
    EffectQualityType,
    FontMethod,
    LightType,
    BloomType,
    TonemapType,
    CullModeFlags,
    ImageLoadInfo
    >;
using ObjectTypes = META_NS::TypeList<
    ISampler::Ptr,
    IImage::Ptr,
    ITexture::Ptr,
    IMaterial::Ptr,
    IEnvironment::Ptr,
    IShader::Ptr,
    ISubMesh::Ptr,
    IMorpher::Ptr,
    IPostProcess::Ptr,
    IBloom::Ptr,
    ITonemap::Ptr
    >;
// clang-format on

namespace Internal {

template<typename... List>
static void RegisterTypes(META_NS::IPropertyRegister& pr, META_NS::TypeList<List...>)
{
    (META_NS::RegisterTypeForBuiltinAny<List>(), ...);
    (META_NS::RegisterTypeForBuiltinArrayAny<List>(), ...);
}

template<typename... List>
static void UnregisterTypes(META_NS::IPropertyRegister& pr, META_NS::TypeList<List...>)
{
    (META_NS::UnregisterTypeForBuiltinAny<List>(), ...);
    (META_NS::UnregisterTypeForBuiltinArrayAny<List>(), ...);
}

void RegisterAnys()
{
    auto& pr = META_NS::GetObjectRegistry().GetPropertyRegister();
    RegisterTypes(pr, BasicTypes {});
    RegisterTypes(pr, ObjectTypes {});
}

void UnRegisterAnys()
{
    auto& pr = META_NS::GetObjectRegistry().GetPropertyRegister();
    UnregisterTypes(pr, ObjectTypes {});
    UnregisterTypes(pr, BasicTypes {});
}

} // namespace Internal
SCENE_END_NAMESPACE()
