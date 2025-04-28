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

#ifndef SCENE_INTERFACE_ISHADER_H
#define SCENE_INTERFACE_ISHADER_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_scene.h>

#include <render/device/intf_shader_manager.h>
#include <render/resource_handle.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/** Cull mode flag bits */
enum class CullModeFlags : uint32_t {
    /** None */
    NONE = 0,
    /** Front bit */
    FRONT_BIT = 0x00000001,
    /** Back bit */
    BACK_BIT = 0x00000002,
    /** Front and back bit */
    FRONT_AND_BACK = 0x00000003,
};

inline CullModeFlags operator|(CullModeFlags l, CullModeFlags r)
{
    return CullModeFlags(static_cast<uint32_t>(l) | static_cast<uint32_t>(r));
}

class IShader : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IShader, "8a241b86-14c6-48ec-b450-6f9818f9f31a")
public:
    /**
     * @brief Uri of the shader definition
     */
    META_READONLY_PROPERTY(BASE_NS::string, Uri)

    /// Load shader from uri
    virtual Future<bool> LoadShader(const IScene::Ptr&, BASE_NS::string_view uri) = 0;
    /// Get cull mode for the shader
    virtual Future<CullModeFlags> GetCullMode() const = 0;
    /// Set cull mode for the shader
    virtual Future<bool> SetCullMode(CullModeFlags mode) = 0;
    /// Is blend enabled for the shader
    virtual Future<bool> IsBlendEnabled() const = 0;
    /// Enable blend mode for shader
    virtual Future<bool> EnableBlend(bool enabled) = 0;
};

class IGraphicsState : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IGraphicsState, "6788f05b-4a55-4396-8cdd-115dc2958656")
public:
    virtual bool SetGraphicsState(CORE_NS::EntityReference) = 0;
    virtual CORE_NS::EntityReference GetGraphicsState() const = 0;
};

class IShaderState : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IShaderState, "44b20afb-4e96-41de-a765-df9dd07799de")
public:
    virtual bool SetShaderState(const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference handle,
        CORE_NS::EntityReference ent, RENDER_NS::RenderHandleReference ghandle, CORE_NS::EntityReference gstate) = 0;
};
META_REGISTER_CLASS(Shader, "56d686b8-7a33-4608-b12a-1a170381bcfd", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::CullModeFlags)
META_INTERFACE_TYPE(SCENE_NS::IShader)
META_INTERFACE_TYPE(SCENE_NS::IGraphicsState)

#endif
