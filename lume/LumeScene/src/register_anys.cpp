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
#include <scene/interface/intf_render_configuration.h>
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
    PolygonMode,
    ImageLoadInfo,
    SceneShadowType,
    SceneShadowQuality,
    SceneShadowSmoothness
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

// Compatibility with old IBitmap any
class BitmapAnyBuilder : public META_NS::AnyBuilder {
public:
    using AnyType = META_NS::AnyType<IImage::Ptr>;

    static META_NS::ObjectId StaticGetClassId()
    {
        return META_NS::ObjectId("61621edc-f7f4-a195-4275-696c74416e79");
    }
    META_NS::IAny::Ptr Construct() override
    {
        return META_NS::IAny::Ptr(new AnyType);
    }
    META_NS::ObjectId GetObjectId() const override
    {
        return StaticGetClassId();
    }
    BASE_NS::string GetTypeName() const override
    {
        return BASE_NS::string(AnyType::StaticGetTypeIdString());
    }
};

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

    // for compatibility
    pr.RegisterAny(CreateShared<BitmapAnyBuilder>());
}

void UnRegisterAnys()
{
    auto& pr = META_NS::GetObjectRegistry().GetPropertyRegister();
    UnregisterTypes(pr, ObjectTypes {});
    UnregisterTypes(pr, BasicTypes {});

    pr.UnregisterAny(BitmapAnyBuilder::StaticGetClassId());
}

} // namespace Internal
SCENE_END_NAMESPACE()
