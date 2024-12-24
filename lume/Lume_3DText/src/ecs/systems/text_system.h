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

#ifndef TEXT_3D_ECS_TEXTSYSTEM_H
#define TEXT_3D_ECS_TEXTSYSTEM_H

#include <base/containers/unordered_map.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_system.h>
#include <core/namespace.h>
#include <font/namespace.h>
#include <render/resource_handle.h>
#include <text_3d/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
CORE_END_NAMESPACE()

FONT_BEGIN_NAMESPACE()
class IFontManager;
FONT_END_NAMESPACE()

TEXT3D_BEGIN_NAMESPACE()
class IMeshComponentManager;
class IRenderHandleComponentManager;
class IRenderMeshComponentManager;
class ITextComponentManager;
struct TextComponent;

class TextSystem final : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "4de00235-c9cd-44d0-94ef-2ef9bbffa088" };

    explicit TextSystem(CORE_NS::IEcs& ecs);
    ~TextSystem() override;

    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;

    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;
    void Uninitialize() override;

    const CORE_NS::IEcs& GetECS() const override;

private:
    struct TextData;

    void GenerateMesh(const TextComponent& textComponent, CORE_NS::Entity entity, struct TextData& cached);

    bool active_ { false };
    CORE_NS::IEcs& ecs_;
    FONT_NS::IFontManager* fontManager_ { nullptr };
    IMeshComponentManager* meshManager_ { nullptr };
    IRenderHandleComponentManager* renderHandleManager_ { nullptr };
    IRenderMeshComponentManager* renderMeshManager_ { nullptr };
    ITextComponentManager* textManager_ { nullptr };
    uint32_t textManagerGeneration_ {};
    struct ShaderData {
        RENDER_NS::RenderHandleReference graphicsState;
        RENDER_NS::RenderHandleReference shader;
        RENDER_NS::RenderHandleReference depthShader;
    };
    ShaderData rasterShader_;
    ShaderData sdfShader_;

    BASE_NS::unordered_map<CORE_NS::Entity, TextData> textCache_;
};

inline constexpr BASE_NS::string_view GetName(const TextSystem*)
{
    return "TextSystem";
}
TEXT3D_END_NAMESPACE()

#endif // TEXT_3D_ECS_TEXTSYSTEM_H
