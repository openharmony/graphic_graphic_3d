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

#ifndef OHOS_3D_RENDER_RESOURCE_FACTORY_IMPL_H
#define OHOS_3D_RENDER_RESOURCE_FACTORY_IMPL_H

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "SceneResources.user.hpp"
#include "SceneTH.user.hpp"

#include "ANIUtils.h"
#include "ImageImpl.h"
#include "MeshResourceImpl.h"
#include "SamplerImpl.h"
#include "ShaderImpl.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "RenderContextETS.h"
#include "SamplerETS.h"

namespace OHOS::Render3D::KITETS {
class RenderResourceFactoryImpl {
public:
    RenderResourceFactoryImpl()
    {
        // Don't forget to implement the constructor.
    }

    ::SceneResources::Shader createShaderSync(::SceneTH::SceneResourceParameters const &params);

    ::SceneResources::Image createImageSync(::SceneTH::SceneResourceParameters const &params)
    {
        return ImageImpl::createImageFromTH(params);
    }

    ::SceneResources::MeshResource createMeshSync(
        ::SceneTH::SceneResourceParameters const &params, ::SceneTypes::GeometryDefinitionType const &geometry)
    {
        WIDGET_LOGI("RenderResourceFactoryImpl::createMeshSync, tag: %{public}d", static_cast<int>(geometry.get_tag()));
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return MeshResourceImpl::Create(params, geometry);
    }

    ::SceneResources::Sampler createSamplerSync(::SceneTH::SceneResourceParameters const &params);

    ::SceneTH::Scene createSceneSync(::taihe::optional_view<uintptr_t> uri)
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return SceneTH::loadScene(uri);
    }
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_RENDER_RESOURCE_FACTORY_IMPL_H