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

#include <3d/ecs/components/material_component.h>
#include <core/property/property_types.h>
CORE_BEGIN_NAMESPACE()
using CORE3D_NS::MaterialComponent;
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureInfo);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<MaterialComponent::TextureInfo>);
CORE_END_NAMESPACE()

#include <scene/ext/intf_ecs_context.h>

#include <3d/ecs/components/render_handle_component.h>
#include <core/property/property_handle_util.h>

#include "material_component.h"

SCENE_BEGIN_NAMESPACE()

BASE_NS::string MaterialComponent::GetName() const
{
    return "MaterialComponent";
}

IInternalMaterial::ActiveTextureSlotInfo MaterialComponent::GetActiveTextureSlotInfo()
{
    static constexpr auto textureInfoTypeDecl =
        CORE_NS::PropertySystem::PropertyTypeDeclFromType<CORE3D_NS::MaterialComponent::TextureInfo>();
    auto ecso = GetEcsObject();
    if (!ecso) {
        return {};
    }
    auto scene = ecso->GetScene();
    if (!scene) {
        return {};
    }
    ActiveTextureSlotInfo info;
    size_t maxIdx = 0;
    auto& ecsc = scene->GetEcsContext();
    if (auto mp = GetProperty("MaterialShader", META_NS::MetadataQuery::EXISTING)) {
        scene->SyncProperty(mp, META_NS::EngineSyncDirection::AUTO);
    }
    if (auto m = ecsc.FindComponent<CORE3D_NS::IMaterialComponentManager>()) {
        auto entity = ecso->GetEntity();
        if (m->HasComponent(entity)) {
            if (auto data = m->GetData(entity)) {
                const auto meta = data->Owner()->MetaData();
                uintptr_t texturesOffset {};
                for (auto& p : meta) {
                    if (p.name == "textures") {
                        texturesOffset = p.offset;
                        break;
                    }
                }
                // Add the default types.
                // Incorrect .shader declarations may make them active anyway, so we must keep them
                info.count = CORE3D_NS::MaterialComponent::TextureIndex::TEXTURE_COUNT;
                info.slots.resize(CORE3D_NS::MaterialComponent::TextureIndex::TEXTURE_COUNT);
                const char* const names[] = { "BASE_COLOR", "NORMAL", "MATERIAL", "EMISSIVE", "AO", "CLEARCOAT",
                    "CLEARCOAT_ROUGHNESS", "CLEARCOAT_NORMAL", "SHEEN", "TRANSMISSION", "SPECULAR" };
                for (int index = 0; index < CORE3D_NS::MaterialComponent::TextureIndex::TEXTURE_COUNT; index++) {
                    ActiveTextureSlotInfo::TextureSlot ts;
                    ts.name = names[index];
                    info.slots[index] = BASE_NS::move(ts);
                }
                // this updates the names of the inputs that .shader declares. (which hopefully are used)
                for (auto& p : meta) {
                    if (p.type == textureInfoTypeDecl) {
                        auto index = (p.offset - texturesOffset) / sizeof(CORE3D_NS::MaterialComponent::TextureInfo);
                        ActiveTextureSlotInfo::TextureSlot ts;
                        ts.name = p.name;
                        info.slots[index] = BASE_NS::move(ts);
                    }
                }
            }
        }
    }
    return info;
}

bool MaterialComponent::UpdateMetadata()
{
    auto ecso = GetEcsObject();
    auto scene = GetInternalScene();
    META_NS::Property<CORE3D_NS::MaterialComponent::Shader> p { GetProperty(
        "MaterialShader", META_NS::MetadataQuery::EXISTING) };

    if (!ecso || !scene || !p) {
        return false;
    }
    auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
        scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>());
    if (!rhman) {
        return false;
    }

    auto currentShader = rhman->GetRenderHandleReference(p->GetValue().shader);

    auto& sman = scene->GetRenderContext().GetDevice().GetShaderManager();
    auto frameIndex = currentShader ? sman.GetFrameIndex(currentShader) : uint64_t {};

    bool changed = cachedShader_.shader != currentShader.GetHandle() || cachedShader_.frameIndex != frameIndex;
    if (changed) {
        cachedShader_ = { currentShader.GetHandle(), frameIndex };

        auto& ecsc = scene->GetEcsContext();
        if (auto m = static_cast<CORE3D_NS::IMaterialComponentManager*>(
                ecsc.FindComponent<CORE3D_NS::MaterialComponent>())) {
            auto entity = ecso->GetEntity();
            // take and release the write lock to material component so that the metadata is updated
            auto lock = m->Write(entity);
        }
    }
    return changed;
}

SCENE_END_NAMESPACE()
