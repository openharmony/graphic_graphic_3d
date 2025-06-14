/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "render_preprocessor_system.h"

#include <algorithm>

#include <3d/ecs/components/graphics_state_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <3d/util/intf_picking.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/scoped_handle.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>
#include <render/implementation_uids.h>

#include "util/component_util_functions.h"
#include "util/log.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::IRenderDataStoreManager*);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace RENDER_NS;
using namespace BASE_NS;
using namespace CORE_NS;

PROPERTY_LIST(IRenderPreprocessorSystem::Properties, ComponentMetadata,
    MEMBER_PROPERTY(dataStoreScene, "dataStoreScene", 0), MEMBER_PROPERTY(dataStoreCamera, "dataStoreCamera", 0),
    MEMBER_PROPERTY(dataStoreLight, "dataStoreLight", 0), MEMBER_PROPERTY(dataStoreMaterial, "dataStoreMaterial", 0),
    MEMBER_PROPERTY(dataStoreMorph, "dataStoreMorph", 0), MEMBER_PROPERTY(dataStorePrefix, "", 0));

constexpr bool operator<(
    const RenderPreprocessorSystem::MaterialProperties& lhs, const RenderPreprocessorSystem::MaterialProperties& rhs)
{
    return lhs.material.id < rhs.material.id;
}

constexpr bool operator<(const RenderPreprocessorSystem::MaterialProperties& element, const Entity& value)
{
    return element.material.id < value.id;
}

constexpr bool operator<(const Entity& element, const RenderPreprocessorSystem::MaterialProperties& value)
{
    return element.id < value.material.id;
}

namespace {
static constexpr string_view DEFAULT_DS_SCENE_NAME { "RenderDataStoreDefaultScene" };
static constexpr string_view DEFAULT_DS_CAMERA_NAME { "RenderDataStoreDefaultCamera" };
static constexpr string_view DEFAULT_DS_LIGHT_NAME { "RenderDataStoreDefaultLight" };
static constexpr string_view DEFAULT_DS_MATERIAL_NAME { "RenderDataStoreDefaultMaterial" };
static constexpr string_view DEFAULT_DS_MORPH_NAME { "RenderDataStoreMorph" };

constexpr const auto RMC = 0U;
constexpr const auto NC = 1U;
constexpr const auto WMC = 2U;
constexpr const auto LC = 3U;
constexpr const auto JMC = 4U;
constexpr const auto SC = 5U;

static const MaterialComponent DEF_MATERIAL_COMPONENT {};
static constexpr RenderDataDefaultMaterial::InputMaterialUniforms DEF_INPUT_MATERIAL_UNIFORMS {};

struct SceneBoundingVolumeHelper {
    BASE_NS::Math::Vec3 sumOfSubmeshPoints { 0.0f, 0.0f, 0.0f };
    uint32_t submeshCount { 0 };

    BASE_NS::Math::Vec3 minAABB { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max() };
    BASE_NS::Math::Vec3 maxAABB { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max() };
};

template<typename DataStoreType>
inline auto CreateIfNeeded(
    IRenderDataStoreManager& manager, refcnt_ptr<DataStoreType>& dataStore, string_view dataStoreName)
{
    if (!dataStore || dataStore->GetName() != dataStoreName) {
        dataStore = refcnt_ptr<DataStoreType>(manager.Create(DataStoreType::UID, dataStoreName.data()));
    }
    return dataStore;
}

#if (CORE3D_VALIDATION_ENABLED == 1)
void ValidateInputColor(const Entity material, const MaterialComponent& matComp)
{
    if (matComp.type < MaterialComponent::Type::CUSTOM) {
        const auto& base = matComp.textures[MaterialComponent::TextureIndex::BASE_COLOR];
        if ((base.factor.x > 1.0f) || (base.factor.y > 1.0f) || (base.factor.z > 1.0f) || (base.factor.w > 1.0f)) {
            CORE_LOG_ONCE_I("ValidateInputColor_expect_base_colorfactor",
                "CORE3D_VALIDATION: Non custom material type expects base color factor to be <= 1.0f.");
        }
        const auto& mat = matComp.textures[MaterialComponent::TextureIndex::MATERIAL];
        if ((mat.factor.y > 1.0f) || (mat.factor.z > 1.0f)) {
            CORE_LOG_ONCE_I("ValidateInputColor_expect_roughness_metallic_factor",
                "CORE3D_VALIDATION: Non custom material type expects roughness and metallic to be <= 1.0f.");
        }
    }
}
#endif

RenderDataDefaultMaterial::InputMaterialUniforms InputMaterialUniformsFromMaterialComponent(
    const Entity material, const MaterialComponent& matDesc)
{
    RenderDataDefaultMaterial::InputMaterialUniforms mu = DEF_INPUT_MATERIAL_UNIFORMS;

#if (CORE3D_VALIDATION_ENABLED == 1)
    ValidateInputColor(material, matDesc);
#endif

    uint32_t transformBits = 0u;
    constexpr const uint32_t texCount = Math::min(static_cast<uint32_t>(MaterialComponent::TextureIndex::TEXTURE_COUNT),
        RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT);
    for (uint32_t idx = 0u; idx < texCount; ++idx) {
        const auto& tex = matDesc.textures[idx];
        auto& texRef = mu.textureData[idx];
        texRef.factor = tex.factor;
        texRef.translation = tex.transform.translation;
        texRef.rotation = tex.transform.rotation;
        texRef.scale = tex.transform.scale;
        const bool hasTransform = (texRef.translation.x != 0.0f) || (texRef.translation.y != 0.0f) ||
                                  (texRef.rotation != 0.0f) || (texRef.scale.x != 1.0f) || (texRef.scale.y != 1.0f);
        transformBits |= static_cast<uint32_t>(hasTransform) << idx;
    }
    {
        // NOTE: premultiplied alpha, applied here and therefore the baseColor factor is special
        const auto& tex = matDesc.textures[MaterialComponent::TextureIndex::BASE_COLOR];
        const float alpha = tex.factor.w;
        const Math::Vec4 baseColor = {
            tex.factor.x * alpha,
            tex.factor.y * alpha,
            tex.factor.z * alpha,
            alpha,
        };

        constexpr uint32_t index = 0u;
        mu.textureData[index].factor = baseColor;
    }
    mu.alphaCutoff = matDesc.alphaCutoff;
    mu.texCoordSetBits = matDesc.useTexcoordSetBit;
    mu.texTransformSetBits = transformBits;
    mu.id = material.id;
    return mu;
}

inline void GetRenderHandleReferences(const IRenderHandleComponentManager& renderHandleMgr,
    const array_view<const EntityReference> inputs, array_view<RenderHandleReference>& outputs)
{
    for (size_t idx = 0; idx < outputs.size(); ++idx) {
        outputs[idx] = renderHandleMgr.GetRenderHandleReference(inputs[idx]);
    }
}

constexpr uint32_t RenderMaterialLightingFlagsFromMaterialFlags(const MaterialComponent::LightingFlags materialFlags)
{
    uint32_t rmf = 0;
    if (materialFlags & MaterialComponent::LightingFlagBits::SHADOW_RECEIVER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_RECEIVER_BIT;
    }
    if (materialFlags & MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_CASTER_BIT;
    }
    if (materialFlags & MaterialComponent::LightingFlagBits::PUNCTUAL_LIGHT_RECEIVER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT;
    }
    if (materialFlags & MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT;
    }
    return rmf;
}

inline RenderDataDefaultMaterial::MaterialHandlesWithHandleReference GetMaterialHandles(
    const MaterialComponent& materialDesc, const IRenderHandleComponentManager& gpuManager)
{
    RenderDataDefaultMaterial::MaterialHandlesWithHandleReference materialHandles;
    auto imageIt = std::begin(materialHandles.images);
    auto samplerIt = std::begin(materialHandles.samplers);
    for (const MaterialComponent::TextureInfo& info : materialDesc.textures) {
        *imageIt++ = gpuManager.GetRenderHandleReference(info.image);
        *samplerIt++ = gpuManager.GetRenderHandleReference(info.sampler);
    }
    return materialHandles;
}

uint32_t RenderMaterialFlagsFromMaterialValues(const MaterialComponent& matComp,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& handles, const uint32_t hasTransformBit)
{
    uint32_t rmf = 0;
    // enable built-in specialization for default materials
    CORE_ASSERT(matComp.type <= MaterialComponent::Type::CUSTOM_COMPLEX);
    if (matComp.type < MaterialComponent::Type::CUSTOM) {
        if (handles.images[MaterialComponent::TextureIndex::NORMAL] ||
            handles.images[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL]) {
            // need to check for tangents as well with submesh
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_NORMAL_MAP_BIT;
        }
        if (matComp.textures[MaterialComponent::TextureIndex::CLEARCOAT].factor.x > 0.0f) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_CLEAR_COAT_BIT;
        }
        if ((matComp.textures[MaterialComponent::TextureIndex::SHEEN].factor.x > 0.0f) ||
            (matComp.textures[MaterialComponent::TextureIndex::SHEEN].factor.y > 0.0f) ||
            (matComp.textures[MaterialComponent::TextureIndex::SHEEN].factor.z > 0.0f)) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHEEN_BIT;
        }
        if (matComp.textures[MaterialComponent::TextureIndex::SPECULAR].factor != Math::Vec4(1.f, 1.f, 1.f, 1.f) ||
            handles.images[MaterialComponent::TextureIndex::SPECULAR]) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SPECULAR_BIT;
        }
        if (matComp.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x > 0.0f) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_TRANSMISSION_BIT;
        }
    }
    rmf |= (hasTransformBit > 0U) ? RenderMaterialFlagBits::RENDER_MATERIAL_TEXTURE_TRANSFORM_BIT : 0U;
    // NOTE: built-in shaders write 1.0 to alpha always when discard is enabled
    rmf |= (matComp.alphaCutoff < 1.0f) ? RenderMaterialFlagBits::RENDER_MATERIAL_SHADER_DISCARD_BIT : 0U;
    // NOTE: GPU instancing specialization needs to be enabled during rendering
    return rmf;
}

void RemoveMaterialProperties(IRenderDataStoreDefaultMaterial& dsMaterial,
    const IMaterialComponentManager& materialManager,
    vector<RenderPreprocessorSystem::MaterialProperties>& materialProperties, array_view<const Entity> removedMaterials)
{
    materialProperties.erase(std::set_difference(materialProperties.begin(), materialProperties.end(),
                                 removedMaterials.cbegin(), removedMaterials.cend(), materialProperties.begin()),
        materialProperties.cend());

    // destroy rendering side decoupled material data
    for (const auto& entRef : removedMaterials) {
        dsMaterial.DestroyMaterialData(entRef.id);
    }
}

constexpr uint32_t RenderSubmeshFlagsFromMeshFlags(const MeshComponent::Submesh::Flags flags)
{
    uint32_t rmf = 0;
    if (flags & MeshComponent::Submesh::FlagBits::TANGENTS_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_TANGENTS_BIT;
    }
    if (flags & MeshComponent::Submesh::FlagBits::VERTEX_COLORS_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_VERTEX_COLORS_BIT;
    }
    if (flags & MeshComponent::Submesh::FlagBits::SKIN_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT;
    }
    if (flags & MeshComponent::Submesh::FlagBits::SECOND_TEXCOORD_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_SECOND_TEXCOORD_BIT;
    }
    return rmf;
}

void SetupSubmeshBuffers(const IRenderHandleComponentManager& renderHandleManager,
    const MeshComponent::Submesh& submesh, RenderSubmeshDataWithHandleReference& renderSubmesh)
{
    CORE_STATIC_ASSERT(
        MeshComponent::Submesh::BUFFER_COUNT <= RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
    // calculate real vertex buffer count and fill "safety" handles for default material
    // no default shader variants without joints etc.
    // NOTE: optimize for minimal GetRenderHandleReference calls
    // often the same vertex buffer is used.
    Entity prevEntity = {};

    for (size_t idx = 0; idx < countof(submesh.bufferAccess); ++idx) {
        const auto& acc = submesh.bufferAccess[idx];
        auto& vb = renderSubmesh.buffers.vertexBuffers[idx];
        if (EntityUtil::IsValid(prevEntity) && (prevEntity == acc.buffer)) {
            vb.bufferHandle = renderSubmesh.buffers.vertexBuffers[idx - 1].bufferHandle;
            vb.bufferOffset = acc.offset;
            vb.byteSize = acc.byteSize;
        } else if (acc.buffer) {
            vb.bufferHandle = renderHandleManager.GetRenderHandleReference(acc.buffer);
            vb.bufferOffset = acc.offset;
            vb.byteSize = acc.byteSize;

            // store the previous entity
            prevEntity = acc.buffer;
        } else {
            vb.bufferHandle = renderSubmesh.buffers.vertexBuffers[0].bufferHandle; // expecting safety binding
            vb.bufferOffset = 0;
            vb.byteSize = 0;
        }
    }

    // NOTE: we will get max amount of vertex buffers if there is at least one
    renderSubmesh.buffers.vertexBufferCount =
        submesh.bufferAccess[0U].buffer ? static_cast<uint32_t>(countof(submesh.bufferAccess)) : 0U;

    if (submesh.indexBuffer.buffer) {
        renderSubmesh.buffers.indexBuffer.bufferHandle =
            renderHandleManager.GetRenderHandleReference(submesh.indexBuffer.buffer);
        renderSubmesh.buffers.indexBuffer.bufferOffset = submesh.indexBuffer.offset;
        renderSubmesh.buffers.indexBuffer.byteSize = submesh.indexBuffer.byteSize;
        renderSubmesh.buffers.indexBuffer.indexType = submesh.indexBuffer.indexType;
    }
    if (submesh.indirectArgsBuffer.buffer) {
        renderSubmesh.buffers.indirectArgsBuffer.bufferHandle =
            renderHandleManager.GetRenderHandleReference(submesh.indirectArgsBuffer.buffer);
        renderSubmesh.buffers.indirectArgsBuffer.bufferOffset = submesh.indirectArgsBuffer.offset;
        renderSubmesh.buffers.indirectArgsBuffer.byteSize = submesh.indirectArgsBuffer.byteSize;
    }
    renderSubmesh.buffers.inputAssembly = submesh.inputAssembly;
}
} // namespace

RenderPreprocessorSystem::RenderPreprocessorSystem(IEcs& ecs)
    : ecs_(ecs), graphicsStateManager_(GetManager<IGraphicsStateComponentManager>(ecs)),
      jointMatricesManager_(GetManager<IJointMatricesComponentManager>(ecs)),
      layerManager_(GetManager<ILayerComponentManager>(ecs)),
      materialManager_(GetManager<IMaterialComponentManager>(ecs)),
      renderHandleManager_(GetManager<IRenderHandleComponentManager>(ecs)),
      meshManager_(GetManager<IMeshComponentManager>(ecs)), nodeManager_(GetManager<INodeComponentManager>(ecs)),
      renderMeshManager_(GetManager<IRenderMeshComponentManager>(ecs)),
      skinManager_(GetManager<ISkinComponentManager>(ecs)),
      worldMatrixManager_(GetManager<IWorldMatrixComponentManager>(ecs)),
      RENDER_PREPROCESSOR_SYSTEM_PROPERTIES(&properties_, array_view(ComponentMetadata))
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
        renderContext_ = GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
        if (renderContext_) {
            picking_ = GetInstance<IPicking>(*renderContext_->GetInterface<IClassRegister>(), UID_PICKING);
            graphicsContext_ =
                GetInstance<IGraphicsContext>(*renderContext_->GetInterface<IClassRegister>(), UID_GRAPHICS_CONTEXT);
        }
    }
}

RenderPreprocessorSystem::~RenderPreprocessorSystem() = default;

void RenderPreprocessorSystem::SetActive(bool state)
{
    active_ = state;
}

bool RenderPreprocessorSystem::IsActive() const
{
    return active_;
}

string_view RenderPreprocessorSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid RenderPreprocessorSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* RenderPreprocessorSystem::GetProperties()
{
    return RENDER_PREPROCESSOR_SYSTEM_PROPERTIES.GetData();
}

const IPropertyHandle* RenderPreprocessorSystem::GetProperties() const
{
    return RENDER_PREPROCESSOR_SYSTEM_PROPERTIES.GetData();
}

void RenderPreprocessorSystem::SetProperties(const IPropertyHandle& data)
{
    if (data.Owner() != &RENDER_PREPROCESSOR_SYSTEM_PROPERTIES) {
        return;
    }
    if (const auto in = ScopedHandle<const IRenderPreprocessorSystem::Properties>(&data); in) {
        properties_.dataStorePrefix = in->dataStorePrefix;
        properties_.dataStoreScene = properties_.dataStorePrefix + in->dataStoreScene;
        properties_.dataStoreCamera = properties_.dataStorePrefix + in->dataStoreCamera;
        properties_.dataStoreLight = properties_.dataStorePrefix + in->dataStoreLight;
        properties_.dataStoreMaterial = properties_.dataStorePrefix + in->dataStoreMaterial;
        properties_.dataStoreMorph = properties_.dataStorePrefix + in->dataStoreMorph;
        if (renderContext_) {
            SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
        }
    }
}

void RenderPreprocessorSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, BASE_NS::array_view<const Entity> entities)
{
    if (componentManager.GetUid() == IMaterialComponentManager::UID) {
        if ((type == EventType::CREATED) || (type == EventType::MODIFIED)) {
            materialModifiedEvents_.append(entities.cbegin(), entities.cend());
        } else if (type == EventType::DESTROYED) {
            materialDestroyedEvents_.append(entities.cbegin(), entities.cend());
        }
    } else if (componentManager.GetUid() == IRenderMeshComponentManager::UID) {
        if (type == EventType::CREATED) {
            renderMeshAabbs_.reserve(entities.size());
            for (const auto& newRenderMesh : entities) {
                auto pos = std::lower_bound(renderMeshAabbs_.cbegin(), renderMeshAabbs_.cend(), newRenderMesh,
                    [](const RenderMeshAaabb& elem, const Entity& value) { return elem.entity < value; });
                if ((pos == renderMeshAabbs_.cend()) || (pos->entity != newRenderMesh)) {
                    renderMeshAabbs_.insert(pos, RenderMeshAaabb { newRenderMesh, {}, {} });
                }
            }
        } else if (type == EventType::DESTROYED) {
            for (const auto& removedRenderMesh : entities) {
                auto pos = std::lower_bound(renderMeshAabbs_.cbegin(), renderMeshAabbs_.cend(), removedRenderMesh,
                    [](const RenderMeshAaabb& elem, const Entity& value) { return elem.entity < value; });
                if ((pos != renderMeshAabbs_.cend()) && (pos->entity == removedRenderMesh)) {
                    renderMeshAabbs_.erase(pos);
                }
            }
        }
    } else if (componentManager.GetUid() == IMeshComponentManager::UID) {
        if ((type == EventType::CREATED) || (type == EventType::MODIFIED)) {
            meshModifiedEvents_.append(entities.cbegin(), entities.cend());
        } else if (type == EventType::DESTROYED) {
            meshDestroyedEvents_.append(entities.cbegin(), entities.cend());
        }
    } else if (componentManager.GetUid() == IGraphicsStateComponentManager::UID) {
        if ((type == EventType::CREATED) || (type == EventType::MODIFIED)) {
            graphicsStateModifiedEvents_.append(entities.cbegin(), entities.cend());
            graphicsStateGeneration_ = componentManager.GetGenerationCounter();
        }
    }
}

void RenderPreprocessorSystem::HandleMaterialEvents()
{
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "RenderPreprocessorSystem", "HandleMaterialEvents", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    if (!materialDestroyedEvents_.empty()) {
        std::sort(materialDestroyedEvents_.begin(), materialDestroyedEvents_.end());
    }

    if (!materialModifiedEvents_.empty()) {
        std::sort(materialModifiedEvents_.begin(), materialModifiedEvents_.end());
        // creating a component generates created and modified events. filter out materials which were created and
        // modified.
        materialModifiedEvents_.erase(std::unique(materialModifiedEvents_.begin(), materialModifiedEvents_.end()),
            materialModifiedEvents_.cend());
        if (!materialDestroyedEvents_.empty()) {
            // filter out materials which were created/modified, but also destroyed.
            materialModifiedEvents_.erase(std::set_difference(materialModifiedEvents_.cbegin(),
                                              materialModifiedEvents_.cend(), materialDestroyedEvents_.cbegin(),
                                              materialDestroyedEvents_.cend(), materialModifiedEvents_.begin()),
                materialModifiedEvents_.cend());
        }
        UpdateMaterialProperties();
        materialModifiedEvents_.clear();
    }
    if (!materialDestroyedEvents_.empty()) {
        RemoveMaterialProperties(*dsMaterial_, *materialManager_, materialProperties_, materialDestroyedEvents_);
        materialDestroyedEvents_.clear();
    }
}

void RenderPreprocessorSystem::HandleMeshEvents()
{
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "RenderPreprocessorSystem", "HandleMeshEvents", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    if (!meshModifiedEvents_.empty()) {
        // creating a component generates created and modified events. filter out materials which were created and
        // modified.
        std::sort(meshModifiedEvents_.begin(), meshModifiedEvents_.end());
        meshModifiedEvents_.erase(
            std::unique(meshModifiedEvents_.begin(), meshModifiedEvents_.end()), meshModifiedEvents_.cend());
        for (const auto& destroyRef : meshDestroyedEvents_) {
            for (auto iter = meshModifiedEvents_.begin(); iter != meshModifiedEvents_.end();) {
                if (destroyRef.id == iter->id) {
                    meshModifiedEvents_.erase(iter);
                } else {
                    ++iter;
                }
            }
        }
        vector<uint64_t> additionalMaterials;
        for (const auto& entRef : meshModifiedEvents_) {
            if (auto meshHandle = meshManager_->Read(entRef); meshHandle) {
                MeshDataWithHandleReference md;
                md.aabbMin = meshHandle->aabbMin;
                md.aabbMax = meshHandle->aabbMax;
                md.meshId = entRef.id;
                const bool hasSkin = (!meshHandle->jointBounds.empty());
                // md.jointBounds
                md.submeshes.resize(meshHandle->submeshes.size());
                for (size_t smIdx = 0; smIdx < md.submeshes.size(); smIdx++) {
                    auto& writeRef = md.submeshes[smIdx];
                    const auto& readRef = meshHandle->submeshes[smIdx];
                    writeRef.materialId = readRef.material.id;
                    if (!readRef.additionalMaterials.empty()) {
                        additionalMaterials.clear();
                        additionalMaterials.resize(readRef.additionalMaterials.size());
                        for (size_t matIdx = 0; matIdx < additionalMaterials.size(); ++matIdx) {
                            additionalMaterials[matIdx] = readRef.additionalMaterials[matIdx].id;
                        }
                        // array view to additional materials
                        writeRef.additionalMaterials = additionalMaterials;
                    }

                    writeRef.meshRenderSortLayer = readRef.renderSortLayer;
                    writeRef.meshRenderSortLayerOrder = readRef.renderSortLayerOrder;
                    writeRef.aabbMin = readRef.aabbMin;
                    writeRef.aabbMax = readRef.aabbMax;

                    SetupSubmeshBuffers(*renderHandleManager_, readRef, writeRef);
                    writeRef.submeshFlags = RenderSubmeshFlagsFromMeshFlags(readRef.flags);

                    // Clear skinning bit if joint matrices were not given.
                    if (!hasSkin) {
                        writeRef.submeshFlags &= ~RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT;
                    }

                    writeRef.drawCommand.vertexCount = readRef.vertexCount;
                    writeRef.drawCommand.indexCount = readRef.indexCount;
                    writeRef.drawCommand.instanceCount = readRef.instanceCount;
                    writeRef.drawCommand.drawCountIndirect = readRef.drawCountIndirect;
                    writeRef.drawCommand.strideIndirect = readRef.strideIndirect;
                    writeRef.drawCommand.firstIndex = readRef.firstIndex;
                    writeRef.drawCommand.vertexOffset = readRef.vertexOffset;
                    writeRef.drawCommand.firstInstance = readRef.firstInstance;
                }
                dsMaterial_->UpdateMeshData(md.meshId, md);
            }
        }
        meshModifiedEvents_.clear();
    }
    if (!meshDestroyedEvents_.empty()) {
        // destroy rendering side decoupled material data
        for (const auto& entRef : meshDestroyedEvents_) {
            dsMaterial_->DestroyMeshData(entRef.id);
        }
    }
}

void RenderPreprocessorSystem::HandleGraphicsStateEvents() noexcept
{
    std::sort(graphicsStateModifiedEvents_.begin(), graphicsStateModifiedEvents_.end());
    graphicsStateModifiedEvents_.erase(
        std::unique(graphicsStateModifiedEvents_.begin(), graphicsStateModifiedEvents_.end()),
        graphicsStateModifiedEvents_.cend());
    auto& shaderManager = renderContext_->GetDevice().GetShaderManager();
    const auto materialCount = materialManager_->GetComponentCount();
    for (const auto& modifiedEntity : graphicsStateModifiedEvents_) {
        auto handle = graphicsStateManager_->Read(modifiedEntity);
        if (!handle) {
            continue;
        }
        const auto stateHash = shaderManager.HashGraphicsState(handle->graphicsState);
        auto gsRenderHandleRef = shaderManager.GetGraphicsStateHandleByHash(stateHash);
        // if the state doesn't match any existing states based on the hash create a new one
        if (!gsRenderHandleRef) {
            const auto path = "3dshaderstates://" + to_hex(stateHash);
            string_view renderSlot = handle->renderSlot;
            if (renderSlot.empty()) {
                // if no render slot is given select translucent or opaque based on blend state.
                renderSlot = std::any_of(handle->graphicsState.colorBlendState.colorAttachments,
                                 handle->graphicsState.colorBlendState.colorAttachments +
                                     handle->graphicsState.colorBlendState.colorAttachmentCount,
                                 [](const GraphicsState::ColorBlendState::Attachment& attachment) {
                                     return attachment.enableBlend;
                                 })
                                 ? DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
                                 : DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE;
            }
            IShaderManager::GraphicsStateCreateInfo createInfo { path, handle->graphicsState };
            IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo;
            variantCreateInfo.renderSlot = renderSlot;
            gsRenderHandleRef = shaderManager.CreateGraphicsState(createInfo, variantCreateInfo);
        }
        if (gsRenderHandleRef) {
            // when there's render handle for the state check that there's also a RenderHandleComponent which points to
            // the render handle.
            auto rhHandle = renderHandleManager_->Write(modifiedEntity);
            if (!rhHandle) {
                renderHandleManager_->Create(modifiedEntity);
                rhHandle = renderHandleManager_->Write(modifiedEntity);
            }
            if (rhHandle) {
                rhHandle->reference = gsRenderHandleRef;
            }
        }
        // add any material using the state to the list of modified materials, so that we update the material to render
        // data store.
        for (IComponentManager::ComponentId id = 0U; id < materialCount; ++id) {
            if (auto materialHandle = materialManager_->Read(id)) {
                if (materialHandle->materialShader.graphicsState == modifiedEntity ||
                    materialHandle->depthShader.graphicsState == modifiedEntity) {
                    materialModifiedEvents_.push_back(materialManager_->GetEntity(id));
                }
            }
        }
    }
    graphicsStateModifiedEvents_.clear();
}

void RenderPreprocessorSystem::SetDataStorePointers(IRenderDataStoreManager& manager)
{
    // creates own data stores based on names (the data store name will have the prefix)
    dsScene_ = CreateIfNeeded(manager, dsScene_, properties_.dataStoreScene);
    dsCamera_ = CreateIfNeeded(manager, dsCamera_, properties_.dataStoreCamera);
    dsLight_ = CreateIfNeeded(manager, dsLight_, properties_.dataStoreLight);
    dsMaterial_ = CreateIfNeeded(manager, dsMaterial_, properties_.dataStoreMaterial);
    dsMorph_ = CreateIfNeeded(manager, dsMorph_, properties_.dataStoreMorph);
}

void RenderPreprocessorSystem::CalculateSceneBounds()
{
    SceneBoundingVolumeHelper helper;

    for (const auto& i : renderMeshAabbs_) {
        // the mesh aabb will have default value if all the submeshes were skipped. in the default value min > max.
        // meshes which don't cast shadows are also skipped from the scene bounds.
        if ((i.meshAabb.min.x < i.meshAabb.max.x) && i.shadowCaster) {
            helper.sumOfSubmeshPoints += (i.meshAabb.min + i.meshAabb.max) / 2.f;
            helper.minAABB = Math::min(helper.minAABB, i.meshAabb.min);
            helper.maxAABB = Math::max(helper.maxAABB, i.meshAabb.max);
            ++helper.submeshCount;
        }
    }

    if (helper.submeshCount == 0) {
        boundingSphere_.radius = 0.0f;
        boundingSphere_.center = Math::Vec3();
    } else {
        const auto boundingSpherePosition = helper.sumOfSubmeshPoints / static_cast<float>(helper.submeshCount);

        const float radMin = Math::Magnitude(boundingSpherePosition - helper.minAABB);
        const float radMax = Math::Magnitude(helper.maxAABB - boundingSpherePosition);
        const float boundingSphereRadius = Math::max(radMin, radMax);

        // Compensate jitter and adjust scene bounding sphere only if change in bounds is meaningful.
        if (boundingSphere_.radius > 0.0f) {
            // Calculate distance to new bounding sphere origin from current sphere.
            const float pointDistance = Math::Magnitude(boundingSpherePosition - boundingSphere_.center);
            // Calculate distance to edge of new bounding sphere from current sphere origin.
            const float sphereEdgeDistance = pointDistance + boundingSphereRadius;

            // Calculate step size for adjustment, use 10% granularity from current bounds.
            constexpr float granularityPct = 0.10f;
            const float granularity = boundingSphere_.radius * granularityPct;

            // Calculate required change of size, in order to fit new sphere inside current bounds.
            const float radDifference = sphereEdgeDistance - boundingSphere_.radius;
            const float posDifference = Math::Magnitude(boundingSpherePosition - boundingSphere_.center);
            // We need to adjust only if the change is bigger than the step size.
            if ((Math::abs(radDifference) > granularity) || (posDifference > granularity)) {
                // Calculate how many steps we need to change and in to which direction.
                const float radAmount = ceil((boundingSphereRadius - boundingSphere_.radius) / granularity);
                const int32_t posAmount = (int32_t)ceil(posDifference / granularity);
                if ((radAmount != 0.f) || (posAmount != 0)) {
                    // Update size and position of the bounds.
                    boundingSphere_.center = boundingSpherePosition;
                    boundingSphere_.radius = boundingSphere_.radius + (radAmount * granularity);
                }
            }
        } else {
            // No existing bounds, start with new values.
            boundingSphere_.radius = boundingSphereRadius;
            boundingSphere_.center = boundingSpherePosition;
        }
    }
}
struct NodeData {
    bool effectivelyEnabled;
    uint32_t sceneId;
};
void RenderPreprocessorSystem::GatherSortData()
{
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "RenderPreprocessorSystem", "GatherSortData", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    const auto& results = renderableQuery_.GetResults();
    const auto renderMeshes = static_cast<IComponentManager::ComponentId>(results.size());
    meshComponents_.clear();
    meshComponents_.reserve(renderMeshes);
    renderMeshAabbs_.reserve(renderMeshes);

    vector<bool> disabled;
    vector<bool> shadowCaster;
    for (const auto& row : results) {
        // this list needs to update only when render mesh, node, or layer component have changed
        const auto nodeData = [](INodeComponentManager* nodeManager, IComponentManager::ComponentId id) {
            auto handle = nodeManager->Read(id);
            return NodeData { handle->effectivelyEnabled, handle->sceneId };
        }(nodeManager_, row.components[NC]);
        const uint64_t layerMask = !row.IsValidComponentId(LC) ? LayerConstants::DEFAULT_LAYER_MASK
                                                               : layerManager_->Get(row.components[LC]).layerMask;
        if (nodeData.effectivelyEnabled && (layerMask != LayerConstants::NONE_LAYER_MASK)) {
            auto renderMeshHandle = renderMeshManager_->Read(row.components[RMC]);
            // gather the submesh world aabbs
            if (const auto meshData = meshManager_->Read(renderMeshHandle->mesh); meshData) {
                // render system doesn't necessarily have to read the render mesh components. preprocessor
                // could offer two lists of mesh+world, one containing the render mesh batch style meshes and second
                // containing regular meshes
                const auto renderMeshEntity = renderMeshManager_->GetEntity(row.components[RMC]);

                auto pos = std::lower_bound(renderMeshAabbs_.begin(), renderMeshAabbs_.end(), renderMeshEntity,
                    [](const RenderMeshAaabb& elem, const Entity& value) { return elem.entity < value; });
                if ((pos == renderMeshAabbs_.end()) || (pos->entity != renderMeshEntity)) {
                    pos = renderMeshAabbs_.insert(pos, RenderMeshAaabb { renderMeshEntity, {}, {} });
                }
                auto& data = *pos;
                auto& meshAabb = data.meshAabb;
                meshAabb = {};
                auto& aabbs = data.submeshAabbs;
                aabbs.clear();

                // check MaterialComponent::DISABLE_BIT and discard those, check
                // MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT and don't include them in scene bounding.
                disabled.clear();
                disabled.resize(meshData->submeshes.size());
                shadowCaster.clear();
                shadowCaster.resize(meshData->submeshes.size());
                auto submeshIdx = 0U;
                bool allowInstancing = true;
                for (const auto& submesh : meshData->submeshes) {
                    if (EntityUtil::IsValid(submesh.material)) {
                        if (auto matPos = GetMaterialProperties(submesh.material)) {
                            disabled[submeshIdx] = matPos->disabled;
                            allowInstancing = allowInstancing && matPos->allowInstancing;
                            shadowCaster[submeshIdx] = matPos->shadowCaster;
                        }
                    } else {
                        // assuming MaterialComponent::materialLightingFlags default value includes
                        // SHADOW_CASTER_BIT
                        shadowCaster[submeshIdx] = true;
                    }
                    for (const auto additionalMaterial : submesh.additionalMaterials) {
                        if (EntityUtil::IsValid(additionalMaterial)) {
                            if (auto matPos = GetMaterialProperties(additionalMaterial)) {
                                disabled[submeshIdx] = matPos->disabled;
                                allowInstancing = allowInstancing && matPos->allowInstancing;
                                shadowCaster[submeshIdx] = matPos->shadowCaster;
                            }
                        } else {
                            // assuming MaterialComponent::materialLightingFlags default value includes
                            // SHADOW_CASTER_BIT
                            shadowCaster[submeshIdx] = true;
                        }
                    }
                    ++submeshIdx;
                }
                data.shadowCaster = std::any_of(
                    shadowCaster.cbegin(), shadowCaster.cend(), [](const bool shadowCaster) { return shadowCaster; });

                if (std::any_of(disabled.cbegin(), disabled.cend(), [](const bool disabled) { return !disabled; })) {
                    bool hasJoints = row.IsValidComponentId(JMC);
                    if (hasJoints) {
                        // this needs to happen only when joint matrices have changed
                        auto jointMatricesHandle = jointMatricesManager_->Read(row.components[JMC]);
                        hasJoints = (jointMatricesHandle->count > 0U);
                        if (hasJoints) {
                            aabbs.push_back(
                                Aabb { jointMatricesHandle->jointsAabbMin, jointMatricesHandle->jointsAabbMax });
                            meshAabb.min = Math::min(meshAabb.min, jointMatricesHandle->jointsAabbMin);
                            meshAabb.max = Math::max(meshAabb.max, jointMatricesHandle->jointsAabbMax);
                        }
                    }
                    if (!hasJoints) {
                        submeshIdx = 0U;
                        const auto& world = worldMatrixManager_->Read(row.components[WMC])->matrix;
                        if (allowInstancing) {
                            // negative scale requires a different graphics state and assuming most of the content
                            // doesn't have negative scaling we'll just use separate draws for inverted meshes instead
                            // of instanced draws. negative scaling factor can be determined by checking is the
                            // determinant of the 3x3 sub-matrix negative.
                            const float determinant = world.x.x * (world.y.y * world.z.z - world.z.y * world.y.z) -
                                                      world.x.y * (world.y.x * world.z.z - world.y.z * world.z.x) +
                                                      world.x.z * (world.y.x * world.z.y - world.y.y * world.z.x);
                            if (determinant < 0.f) {
                                allowInstancing = false;
                            }
                        }
                        for (const auto& submesh : meshData->submeshes) {
                            // this needs to happen only when world matrix, or mesh component have changed
                            if (disabled[submeshIdx]) {
                                aabbs.push_back({});
                            } else {
                                const MinAndMax mam = picking_->GetWorldAABB(world, submesh.aabbMin, submesh.aabbMax);
                                aabbs.push_back({ mam.minAABB, mam.maxAABB });
                                meshAabb.min = Math::min(meshAabb.min, mam.minAABB);
                                meshAabb.max = Math::max(meshAabb.max, mam.maxAABB);
                            }
                            ++submeshIdx;
                        }
                    }

                    auto skin = (row.IsValidComponentId(SC)) ? skinManager_->Read(row.components[SC])->skin : Entity {};
                    meshComponents_.push_back({ nodeData.sceneId, row.components[RMC], renderMeshHandle->mesh,
                        renderMeshHandle->renderMeshBatch, skin, allowInstancing });
                }
            }
        }
    }
}

void RenderPreprocessorSystem::UpdateMaterialProperties()
{
    // assuming modifiedMaterials was sorted we can assume the next entity is between pos and end.
    auto pos = materialProperties_.begin();
    for (const auto& entity : materialModifiedEvents_) {
        auto materialHandle = materialManager_->Read(entity);
        if (!materialHandle) {
            continue;
        }
        pos = std::lower_bound(pos, materialProperties_.end(), entity);
        if ((pos == materialProperties_.end()) || (pos->material != entity)) {
            pos = materialProperties_.insert(pos, {});
            pos->material = entity;
        }
        pos->disabled =
            (materialHandle->extraRenderingFlags & MaterialComponent::ExtraRenderingFlagBits::DISABLE_BIT) ==
            MaterialComponent::ExtraRenderingFlagBits::DISABLE_BIT;
        pos->allowInstancing = ((materialHandle->extraRenderingFlags &
                                    MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT) ==
                                   MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT) ||
                               !EntityUtil::IsValid(materialHandle->materialShader.shader);
        pos->shadowCaster =
            (materialHandle->materialLightingFlags & MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT) ==
            MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;

        // create/update rendering side decoupled material data
        UpdateSingleMaterial(entity, &(*materialHandle));
    }
}

void RenderPreprocessorSystem::UpdateSingleMaterial(const Entity matEntity, const MaterialComponent* materialHandle)
{
    const MaterialComponent& materialComp = (materialHandle) ? *materialHandle : DEF_MATERIAL_COMPONENT;
    RenderDataDefaultMaterial::InputMaterialUniforms materialUniforms =
        InputMaterialUniformsFromMaterialComponent(matEntity, materialComp);

    // NOTE: we force material updates, no early outs

    array_view<const uint8_t> customData;
    if (materialComp.customProperties) {
        const auto buffer = static_cast<const uint8_t*>(materialComp.customProperties->RLock());
        // NOTE: set and binding are currently not supported, we only support built-in mapping
        // the data goes to a predefined set and binding
        customData = array_view(buffer, materialComp.customProperties->Size());
        materialComp.customProperties->RUnlock();
    }
    // material extensions
    RenderHandleReference handleReferences[RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT];
    array_view<RenderHandleReference> extHandles;
    // extension valid only with non-default material
    if (EntityUtil::IsValid(matEntity)) {
        // first check the preferred vector version
        if (!materialComp.customResources.empty()) {
            const size_t maxCount = Math::min(static_cast<size_t>(materialComp.customResources.size()),
                static_cast<size_t>(RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT));
            extHandles = { handleReferences, maxCount };
            GetRenderHandleReferences(*renderHandleManager_, materialComp.customResources, extHandles);
        }
    }
    const uint32_t transformBits = materialUniforms.texTransformSetBits;
    const RenderMaterialFlags rmfFromBits =
        RenderMaterialLightingFlagsFromMaterialFlags(materialComp.materialLightingFlags);
    {
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference materialHandles =
            GetMaterialHandles(materialComp, *renderHandleManager_);
        const RenderMaterialFlags rmfFromValues =
            RenderMaterialFlagsFromMaterialValues(materialComp, materialHandles, transformBits);
        const RenderMaterialFlags rmf = rmfFromBits | rmfFromValues;
        const uint32_t customCameraId = materialComp.cameraEntity.id & 0xFFFFffffU;
        const RenderDataDefaultMaterial::MaterialData data {
            { renderHandleManager_->GetRenderHandleReference(materialComp.materialShader.shader),
                renderHandleManager_->GetRenderHandleReference(materialComp.materialShader.graphicsState) },
            { renderHandleManager_->GetRenderHandleReference(materialComp.depthShader.shader),
                renderHandleManager_->GetRenderHandleReference(materialComp.depthShader.graphicsState) },
            materialComp.extraRenderingFlags, rmf, materialComp.customRenderSlotId, customCameraId,
            RenderMaterialType(materialComp.type), materialComp.renderSort.renderSortLayer,
            materialComp.renderSort.renderSortLayerOrder
        };

        dsMaterial_->UpdateMaterialData(matEntity.id, materialUniforms, materialHandles, data, customData, extHandles);
    }
}

RenderPreprocessorSystem::MaterialProperties* RenderPreprocessorSystem::GetMaterialProperties(CORE_NS::Entity matEntity)
{
    auto matPos = std::lower_bound(materialProperties_.begin(), materialProperties_.end(), matEntity,
        [](const MaterialProperties& element, const Entity& value) { return element.material.id < value.id; });
    if ((matPos != materialProperties_.end()) && (matPos->material == matEntity)) {
        // material's properties were cached
        return &(*matPos);
    }

    // material is unknown (created by a system after ProcessEvents), try to cache the properties
    auto materialHandle = materialManager_->Read(matEntity);
    if (!materialHandle) {
        // entity didn't have a material component
        return nullptr;
    }

    matPos = materialProperties_.insert(matPos, {});
    matPos->material = matEntity;
    matPos->disabled = (materialHandle->extraRenderingFlags & MaterialComponent::ExtraRenderingFlagBits::DISABLE_BIT) ==
                       MaterialComponent::ExtraRenderingFlagBits::DISABLE_BIT;
    matPos->allowInstancing =
        ((materialHandle->extraRenderingFlags & MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT) ==
            MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT) ||
        !EntityUtil::IsValid(materialHandle->materialShader.shader);
    matPos->shadowCaster =
        (materialHandle->materialLightingFlags & MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT) ==
        MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;

    // create/update rendering side decoupled material data
    UpdateSingleMaterial(matEntity, &(*materialHandle));

    return &(*matPos);
}

const IEcs& RenderPreprocessorSystem::GetECS() const
{
    return ecs_;
}

array_view<const RenderPreprocessorSystem::SceneData> RenderPreprocessorSystem::GetSceneData() const
{
    return renderMeshComponentsPerScene_;
}

array_view<const Entity> RenderPreprocessorSystem::GetRenderBatchMeshEntities(const uint32_t sceneId) const
{
    if (auto pos = std::find_if(renderMeshComponentsPerScene_.cbegin(), renderMeshComponentsPerScene_.cend(),
            [sceneId](const SceneData& data) { return data.sceneId == sceneId; });
        pos != renderMeshComponentsPerScene_.cend()) {
        return pos->renderBatchComponents;
    }
    return {};
}

array_view<const Entity> RenderPreprocessorSystem::GetInstancingAllowedEntities(const uint32_t sceneId) const
{
    if (auto pos = std::find_if(renderMeshComponentsPerScene_.cbegin(), renderMeshComponentsPerScene_.cend(),
            [sceneId](const SceneData& data) { return data.sceneId == sceneId; });
        pos != renderMeshComponentsPerScene_.cend()) {
        return pos->instancingAllowed;
    }
    return {};
}

array_view<const Entity> RenderPreprocessorSystem::GetInstancingDisabledEntities(const uint32_t sceneId) const
{
    if (auto pos = std::find_if(renderMeshComponentsPerScene_.cbegin(), renderMeshComponentsPerScene_.cend(),
            [sceneId](const SceneData& data) { return data.sceneId == sceneId; });
        pos != renderMeshComponentsPerScene_.cend()) {
        return pos->rest;
    }
    return {};
}

RenderPreprocessorSystem::Aabb RenderPreprocessorSystem::GetRenderMeshAabb(Entity renderMesh) const
{
    const auto pos = std::lower_bound(renderMeshAabbs_.cbegin(), renderMeshAabbs_.cend(), renderMesh,
        [](const RenderMeshAaabb& elem, const Entity& value) { return elem.entity < value; });
    if ((pos != renderMeshAabbs_.cend()) && (pos->entity == renderMesh)) {
        return pos->meshAabb;
    }
    return {};
}

array_view<const RenderPreprocessorSystem::Aabb> RenderPreprocessorSystem::GetRenderMeshAabbs(Entity renderMesh) const
{
    const auto pos = std::lower_bound(renderMeshAabbs_.cbegin(), renderMeshAabbs_.cend(), renderMesh,
        [](const RenderMeshAaabb& elem, const Entity& value) { return elem.entity < value; });
    if ((pos != renderMeshAabbs_.cend()) && (pos->entity == renderMesh)) {
        return pos->submeshAabbs;
    }
    return {};
}

RenderPreprocessorSystem::Sphere RenderPreprocessorSystem::GetBoundingSphere() const
{
    return boundingSphere_;
}

bool RenderPreprocessorSystem::CheckIfDefaultDataStoreNames() const
{
    if ((properties_.dataStoreScene == DEFAULT_DS_SCENE_NAME) &&
        (properties_.dataStoreCamera == DEFAULT_DS_CAMERA_NAME) &&
        (properties_.dataStoreLight == DEFAULT_DS_LIGHT_NAME) &&
        (properties_.dataStoreMaterial == DEFAULT_DS_MATERIAL_NAME) &&
        (properties_.dataStoreMorph == DEFAULT_DS_MORPH_NAME)) {
        return true;
    } else {
        return false;
    }
}

void RenderPreprocessorSystem::Initialize()
{
    if (graphicsContext_ && renderContext_) {
        const bool hasDefaultNames = CheckIfDefaultDataStoreNames();
        if ((properties_.dataStorePrefix.empty()) && (ecs_.GetId() != 0) && hasDefaultNames) {
            properties_.dataStorePrefix = to_string(ecs_.GetId());
        }
        if (hasDefaultNames) {
            properties_.dataStoreScene = properties_.dataStorePrefix + properties_.dataStoreScene;
            properties_.dataStoreCamera = properties_.dataStorePrefix + properties_.dataStoreCamera;
            properties_.dataStoreLight = properties_.dataStorePrefix + properties_.dataStoreLight;
            properties_.dataStoreMaterial = properties_.dataStorePrefix + properties_.dataStoreMaterial;
            properties_.dataStoreMorph = properties_.dataStorePrefix + properties_.dataStoreMorph;
        }
        SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
    }
    if (renderMeshManager_ && nodeManager_ && layerManager_) {
        const ComponentQuery::Operation operations[] = {
            { *nodeManager_, ComponentQuery::Operation::REQUIRE },
            { *worldMatrixManager_, ComponentQuery::Operation::REQUIRE },
            { *layerManager_, ComponentQuery::Operation::OPTIONAL },
            { *jointMatricesManager_, ComponentQuery::Operation::OPTIONAL },
            { *skinManager_, ComponentQuery::Operation::OPTIONAL },
        };
        renderableQuery_.SetEcsListenersEnabled(true);
        renderableQuery_.SetupQuery(*renderMeshManager_, operations, false);
    }
    ecs_.AddListener(*renderMeshManager_, *this);
    ecs_.AddListener(*materialManager_, *this);
    ecs_.AddListener(*meshManager_, *this);
    ecs_.AddListener(*graphicsStateManager_, *this);
}

bool RenderPreprocessorSystem::Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime)
{
    if (!active_) {
        return false;
    }

    const auto gsGen = graphicsStateManager_->GetGenerationCounter();
    if (gsGen != graphicsStateGeneration_) {
        graphicsStateGeneration_ = gsGen;
    }
    if (!graphicsStateModifiedEvents_.empty()) {
        HandleGraphicsStateEvents();
    }

    const bool queryChanged = renderableQuery_.Execute();
    const auto materialChanged = (!materialModifiedEvents_.empty()) || (!materialDestroyedEvents_.empty());
    const auto meshChanged = (!meshModifiedEvents_.empty()) || (!meshDestroyedEvents_.empty());
    const auto jointGen = jointMatricesManager_->GetGenerationCounter();
    const auto layerGen = layerManager_->GetGenerationCounter();
    const auto nodeGen = nodeManager_->GetGenerationCounter();
    const auto renderMeshGen = renderMeshManager_->GetGenerationCounter();
    const auto worldMatrixGen = worldMatrixManager_->GetGenerationCounter();
    if (!queryChanged && !materialChanged && !meshChanged && (jointGeneration_ == jointGen) &&
        (layerGeneration_ == layerGen) && (nodeGeneration_ == nodeGen) && (renderMeshGeneration_ == renderMeshGen) &&
        (worldMatrixGeneration_ == worldMatrixGen)) {
        return false;
    }
    if (materialChanged) {
        HandleMaterialEvents();
    }
    if (meshChanged) {
        HandleMeshEvents();
    }

    // gather which mesh is used by each render mesh component.
    GatherSortData();

    jointGeneration_ = jointGen;
    layerGeneration_ = layerGen;
    nodeGeneration_ = nodeGen;
    renderMeshGeneration_ = renderMeshGen;
    worldMatrixGeneration_ = worldMatrixGen;
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_BEGIN(
        sortMeshComponents, "CORE3D", "RenderPreprocessorSystem", "SortMeshComponents", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    std::sort(meshComponents_.begin(), meshComponents_.end(), [](const SortData& lhs, const SortData& rhs) {
        // first sort by scene ID
        if (lhs.sceneId < rhs.sceneId) {
            return true;
        }
        if (lhs.sceneId > rhs.sceneId) {
            return false;
        }
        // entities with render mesh batch are grouped at the begining of each scene sorted by entity id
        if (EntityUtil::IsValid(lhs.batch)) {
            if (!EntityUtil::IsValid(rhs.batch)) {
                return true;
            }
            return lhs.renderMeshId < rhs.renderMeshId;
        }
        if (EntityUtil::IsValid(rhs.batch)) {
            return false;
        }
        // render mesh batch entities are followed by in
        if (lhs.allowInstancing && !rhs.allowInstancing) {
            return true;
        }
        if (!lhs.allowInstancing && rhs.allowInstancing) {
            return false;
        }
        // next are meshes that can be instanced sorted by mesh id, skin id and entity id
        if (lhs.mesh < rhs.mesh) {
            return true;
        }
        if (lhs.mesh > rhs.mesh) {
            return false;
        }
        if (lhs.skin < rhs.skin) {
            return true;
        }
        if (lhs.skin > rhs.skin) {
            return false;
        }
        return lhs.renderMeshId < rhs.renderMeshId;
    });
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_END(sortMeshComponents);
#endif
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_BEGIN(renderMeshComponentsPerScene, "CORE3D", "RenderPreprocessorSystem", "BuildLists",
        CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    // list the entities with render mesh components in sorted order.
    const auto meshCount = meshComponents_.size();
    renderMeshComponents_.clear();
    renderMeshComponents_.reserve(meshCount);
    renderMeshComponentsPerScene_.clear();
    uint32_t currentScene = meshCount ? meshComponents_.front().sceneId : 0U;
    size_t startIndex = 0U;
    size_t lastBatchIndex = 0U;
    size_t lastAllowInstancingIndex = 0U;
    for (const auto& data : meshComponents_) {
        if (currentScene != data.sceneId) {
            const auto endIndex = renderMeshComponents_.size();
            renderMeshComponentsPerScene_.push_back({ currentScene,
                array_view(renderMeshComponents_.data() + startIndex, lastBatchIndex - startIndex),
                array_view(renderMeshComponents_.data() + lastBatchIndex, lastAllowInstancingIndex - lastBatchIndex),
                array_view(
                    renderMeshComponents_.data() + lastAllowInstancingIndex, endIndex - lastAllowInstancingIndex) });
            startIndex = endIndex;
            lastBatchIndex = endIndex;
            lastAllowInstancingIndex = endIndex;
            currentScene = data.sceneId;
        }
        renderMeshComponents_.push_back(renderMeshManager_->GetEntity(data.renderMeshId));
        const auto endIndex = renderMeshComponents_.size();
        if (EntityUtil::IsValid(data.batch)) {
            lastBatchIndex = endIndex;
            lastAllowInstancingIndex = endIndex;
        } else if (data.allowInstancing) {
            lastAllowInstancingIndex = endIndex;
        }
    }
    {
        const auto endIndex = renderMeshComponents_.size();
        renderMeshComponentsPerScene_.push_back({ currentScene,
            array_view(renderMeshComponents_.data() + startIndex, lastBatchIndex - startIndex),
            array_view(renderMeshComponents_.data() + lastBatchIndex, lastAllowInstancingIndex - lastBatchIndex),
            array_view(renderMeshComponents_.data() + lastAllowInstancingIndex, endIndex - lastAllowInstancingIndex) });
    }
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_END(renderMeshComponentsPerScene);
#endif

    CalculateSceneBounds();

    return true;
}

void RenderPreprocessorSystem::Uninitialize()
{
    ecs_.RemoveListener(*graphicsStateManager_, *this);
    ecs_.RemoveListener(*meshManager_, *this);
    ecs_.RemoveListener(*materialManager_, *this);
    ecs_.RemoveListener(*renderMeshManager_, *this);
    renderableQuery_.SetEcsListenersEnabled(false);
}

ISystem* IRenderPreprocessorSystemInstance(IEcs& ecs)
{
    return new RenderPreprocessorSystem(ecs);
}

void IRenderPreprocessorSystemDestroy(ISystem* instance)
{
    delete static_cast<RenderPreprocessorSystem*>(instance);
}
CORE3D_END_NAMESPACE()
