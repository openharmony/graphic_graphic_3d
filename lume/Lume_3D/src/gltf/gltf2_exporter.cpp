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

#include "gltf2_exporter.h"

#include <charconv>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/util/compile_time_hashes.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>
#include <core/property/intf_property_handle.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "ecs/systems/animation_playback.h"
#include "gltf/data.h"
#include "gltf/gltf2_data_structures.h"
#include "gltf/gltf2_util.h"
#include "util/json_util.h"

template<>
uint64_t BASE_NS::hash(const bool& val)
{
    return static_cast<uint64_t>(val);
}

template<>
uint64_t BASE_NS::hash(const CORE3D_NS::GLTF2::ComponentType& val)
{
    return static_cast<uint64_t>(val);
}

template<>
uint64_t BASE_NS::hash(const CORE3D_NS::GLTF2::DataType& val)
{
    return static_cast<uint64_t>(val);
}

template<>
uint64_t BASE_NS::hash(const CORE3D_NS::GLTF2::BufferTarget& val)
{
    return static_cast<uint64_t>(val);
}

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

// internal method from
GLTFLoadResult LoadGLTF(IFileManager& fileManager, const string_view uri);

void AppendUnique(json::value& jsonData, const string_view value)
{
    if (jsonData.is_array()) {
        const auto cmpValue = [value](const json::value& it) { return it.string_ == value; };
        if (std::find_if(jsonData.array_.begin(), jsonData.array_.end(), cmpValue) == jsonData.array_.end()) {
            jsonData.array_.push_back(value);
        }
    }
}

// Some GLTF object names are not exported at the moment. Placeholder code has been added behind
// EXPORT_OTHER_OBJECT_NAMES macro to easily spot the missing names and add support if the need arises.
GLTF2::FilterMode GetFilterMode(Filter filter)
{
    if (filter == CORE_FILTER_NEAREST) {
        return GLTF2::FilterMode::NEAREST;
    } else if (filter == CORE_FILTER_LINEAR) {
        return GLTF2::FilterMode::LINEAR;
    }

    return GLTF2::FilterMode::NEAREST;
}

GLTF2::FilterMode GetFilterMode(Filter filter, Filter mipmapMode)
{
    /* NOTE: how to figure out should mips be used from SamplerDesc? */
    if (filter == CORE_FILTER_NEAREST && mipmapMode == CORE_FILTER_NEAREST) {
        return GLTF2::FilterMode::NEAREST_MIPMAP_NEAREST;
    } else if (filter == CORE_FILTER_LINEAR && mipmapMode == CORE_FILTER_NEAREST) {
        return GLTF2::FilterMode::LINEAR_MIPMAP_NEAREST;
    } else if (filter == CORE_FILTER_NEAREST && mipmapMode == CORE_FILTER_LINEAR) {
        return GLTF2::FilterMode::NEAREST_MIPMAP_LINEAR;
    } else if (filter == CORE_FILTER_LINEAR && mipmapMode == CORE_FILTER_LINEAR) {
        return GLTF2::FilterMode::LINEAR_MIPMAP_LINEAR;
    }

    return GLTF2::FilterMode::NEAREST;
}

GLTF2::WrapMode GetWrapMode(SamplerAddressMode mode)
{
    switch (mode) {
        default:
        case CORE_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
        case CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
        case CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
            return GLTF2::WrapMode::CLAMP_TO_EDGE;

        case CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
            return GLTF2::WrapMode::MIRRORED_REPEAT;

        case CORE_SAMPLER_ADDRESS_MODE_REPEAT:
            return GLTF2::WrapMode::REPEAT;
    }
}

GLTF2::AnimationPath GetAnimationPath(const AnimationTrackComponent& track)
{
    if (track.component == ITransformComponentManager::UID) {
        if (track.property == "position") {
            return GLTF2::AnimationPath::TRANSLATION;
        } else if (track.property == "rotation") {
            return GLTF2::AnimationPath::ROTATION;
        } else if (track.property == "scale") {
            return GLTF2::AnimationPath::SCALE;
        }
    } else if (track.component == IMorphComponentManager::UID) {
        if (track.property == "morphWeights") {
            return GLTF2::AnimationPath::WEIGHTS;
        }
    }
    return GLTF2::AnimationPath::INVALID;
}

GLTF2::AnimationInterpolation GetAnimationInterpolation(AnimationTrackComponent::Interpolation interpolation)
{
    switch (interpolation) {
        case AnimationTrackComponent::Interpolation::STEP:
            return GLTF2::AnimationInterpolation::STEP;
        case AnimationTrackComponent::Interpolation::LINEAR:
            return GLTF2::AnimationInterpolation::LINEAR;
        case AnimationTrackComponent::Interpolation::SPLINE:
            return GLTF2::AnimationInterpolation::SPLINE;
        default:
            return GLTF2::AnimationInterpolation::INVALID;
    }
}

namespace GLTF2 {
ExportResult::~ExportResult() = default;

namespace {
constexpr auto const DEFAULT_BASECOLOR_FACTOR = Math::Vec4(1.f, 1.f, 1.f, 1.f);
constexpr auto const DEFAULT_DIFFUSE_FACTOR = Math::Vec4(1.f, 1.f, 1.f, 1.f);
constexpr auto const DEFAULT_SPECULAR_FACTOR = Math::Vec3(1.f, 1.f, 1.f);
constexpr auto const DEFAULT_EMISSIVE_FACTOR = Math::Vec3(0.f, 0.f, 0.f);

constexpr auto const DEFAULT_TRANSLATION = Math::Vec3(0.f, 0.f, 0.f);
constexpr auto const DEFAULT_SCALE = Math::Vec3(1.f, 1.f, 1.f);
constexpr auto const DEFAULT_ROTATION = Math::Quat(0.f, 0.f, 0.f, 1.f);

constexpr auto const IDENTITY_MATRIX =
    Math::Mat4X4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

/* Entity which have skin, camera, and/or light attached are stored here for further procesing. */
struct Entities {
    vector<Entity> nodes;
    vector<Entity> withSkin;
    vector<Entity> withCamera;
    vector<Entity> withLight;
};

inline bool operator==(const RenderHandleReference& lhs, const RenderHandleReference& rhs) noexcept
{
    return lhs.GetHandle() == rhs.GetHandle();
}

inline bool operator<(const RenderHandleReference& lhs, const RenderHandleReference& rhs) noexcept
{
    return lhs.GetHandle().id < rhs.GetHandle().id;
}

/* Returns the index where the given object is in the vector. */
template<typename T>
int FindObjectIndex(vector<unique_ptr<T>> const& array, T const& object)
{
    auto const comparePointers = [ptr = &object](auto const& aUniqueObject) { return aUniqueObject.get() == ptr; };
    if (auto const pos = std::find_if(array.begin(), array.end(), comparePointers); pos != array.end()) {
        return static_cast<int>(std::distance(array.begin(), pos));
    }

    return -1;
}

/* Returns the index where the given handle is in the vector. */
template<typename T>
uint32_t FindHandleIndex(vector<T> const& handles, T handle)
{
    if (auto const handlePos = std::find(handles.begin(), handles.end(), handle); handlePos != handles.end()) {
        return static_cast<uint32_t>(std::distance(handles.begin(), handlePos));
    }
    return ~0u;
}

/* Returns the index where the given handle is in the vector. If the handle was not in the vector it is added. */
template<typename T>
uint32_t FindOrAddIndex(vector<T>& handles, const T& handle)
{
    if (auto const handlePos =
            std::find_if(handles.begin(), handles.end(), [handle](const auto& rhs) { return handle == rhs; });
        handlePos != handles.end()) {
        return static_cast<uint32_t>(std::distance(handles.begin(), handlePos));
    } else {
        handles.push_back(handle);
        return static_cast<uint32_t>(handles.size() - 1);
    }
}

class BufferHelper {
public:
    BufferHelper(
        Buffer& buffer, vector<unique_ptr<BufferView>>& usedBufferViews, vector<unique_ptr<Accessor>>& usedAccessors)
        : usedBufferViews_(usedBufferViews), usedAccessors_(usedAccessors), buffer_(buffer) {};

    ~BufferHelper() = default;

    /* StoreBufferView hashes bufferView and stores it if needed. If dataAlignment is non-zero, data from aBufferView is
     * written to the buffer. */
    BufferView* StoreBufferView(const BufferView& bufferView, uint32_t dataAlignment)
    {
        auto const bufferViewHash = BASE_NS::Hash(
            bufferView.buffer, bufferView.byteLength, bufferView.byteOffset, bufferView.byteStride, bufferView.target);
        auto const bufferViewIndex = FindOrAddIndex(bufferViewHashes_, bufferViewHash);
        if ((bufferViewIndex + 1) > usedBufferViews_.size()) {
            usedBufferViews_.resize(bufferViewIndex + 1);
            usedBufferViews_[bufferViewIndex] = make_unique<BufferView>(bufferView);
            usedBufferViews_[bufferViewIndex]->buffer = &buffer_;
            if (dataAlignment) {
                auto const pad = buffer_.data.size() % dataAlignment;
                buffer_.data.insert(buffer_.data.end(), pad, 0);
                usedBufferViews_[bufferViewIndex]->byteOffset = buffer_.data.size();
                buffer_.data.insert(buffer_.data.end(), bufferView.data, bufferView.data + bufferView.byteLength);
            }
        }
        return usedBufferViews_[bufferViewIndex].get();
    }

    /* StoreAccessor first stores the bufferView used by accessor, then hashes it and stores if needed. */
    Accessor* StoreAccessor(const Accessor& accessor)
    {
        BufferView* bufferView = nullptr;
        if (accessor.bufferView) {
            bufferView = StoreBufferView(*accessor.bufferView, GetComponentByteSize(accessor.componentType));
        }
        auto const accessorHash = BASE_NS::Hash(bufferView, accessor.componentType, accessor.count, accessor.type,
            accessor.byteOffset, accessor.normalized, accessor.sparse.count, accessor.sparse.indices.bufferView,
            accessor.sparse.indices.byteOffset, accessor.sparse.indices.componentType,
            accessor.sparse.values.bufferView, accessor.sparse.values.byteOffset);

        auto const accessorIndex = FindOrAddIndex(accessorHashes_, accessorHash);
        if ((accessorIndex + 1) > usedAccessors_.size()) {
            usedAccessors_.resize(accessorIndex + 1);
            usedAccessors_[accessorIndex] = make_unique<Accessor>(accessor);
            usedAccessors_[accessorIndex]->bufferView = bufferView;
        }
        return usedAccessors_[accessorIndex].get();
    };

    Buffer& GetBuffer() const
    {
        return buffer_;
    }

private:
    vector<uint64_t> bufferViewHashes_;
    vector<uint64_t> accessorHashes_;

    vector<unique_ptr<BufferView>>& usedBufferViews_;
    vector<unique_ptr<Accessor>>& usedAccessors_;
    Buffer& buffer_;
};

struct ResourceEntity {
    Entity entity;
    RenderHandleReference handle;
};

class TextureHelper {
public:
    auto GetSamplerIndex(const RenderHandleReference& sampler)
    {
        return FindOrAddIndex(usedSamplers_, sampler);
    }

    auto GetImageIndex(const RenderHandleReference& image)
    {
        return FindOrAddIndex(usedImages_, image);
    }

    // Helper which returns the index of the "texture", i.e. sampler and image index pair.
    auto GetTextureIndex(uint32_t samplerIndex, uint32_t imageIndex)
    {
        auto const textureHash = BASE_NS::Hash(samplerIndex, imageIndex);
        auto const textureIndex = FindOrAddIndex(textureHashes_, textureHash);
        if ((textureIndex + 1) >= usedTextures_.size()) {
            usedTextures_.resize(textureIndex + 1);
            usedTextures_[textureIndex] = { samplerIndex, imageIndex };
        }
        return textureIndex;
    }

    bool HasSamplers() const
    {
        return !usedSamplers_.empty();
    }

    bool HasImages() const
    {
        return !usedImages_.empty();
    }

    bool HasTextures() const
    {
        return !usedTextures_.empty();
    }

    /* Generate GLTF2::Samplers from gathered sampler resource handlers. */
    vector<unique_ptr<Sampler>> GenerateGltfSamplers(IGpuResourceManager const& gpuResourceManager) const
    {
        vector<unique_ptr<Sampler>> samplerArray;
        samplerArray.reserve(usedSamplers_.size());
        for (const auto& gpuSamplerHandle : usedSamplers_) {
            auto& exportSampler = samplerArray.emplace_back(make_unique<Sampler>());
            auto const& samplerDesc =
                gpuResourceManager.GetSamplerDescriptor(gpuResourceManager.Get(gpuSamplerHandle.GetHandle()));
            exportSampler->magFilter = GetFilterMode(samplerDesc.magFilter);
            exportSampler->minFilter = GetFilterMode(samplerDesc.minFilter, samplerDesc.mipMapMode);
            exportSampler->wrapS = GetWrapMode(samplerDesc.addressModeU);
            exportSampler->wrapT = GetWrapMode(samplerDesc.addressModeV);
        }
        return samplerArray;
    }

    /* Generate GLTF2::Images from gathered image resource handlers. */
    vector<unique_ptr<Image>> GenerateGltfImages(
        const vector<ResourceEntity>& resourceEnties, const IUriComponentManager& uriManager) const
    {
        vector<unique_ptr<Image>> imageArray;
        imageArray.reserve(usedImages_.size());

        for (const auto& gpuImageHandle : usedImages_) {
            auto& exportImage = imageArray.emplace_back(make_unique<Image>());
            // sorted for find performance
            if (auto const pos = std::lower_bound(resourceEnties.begin(), resourceEnties.end(), gpuImageHandle,
                    [](ResourceEntity const& info, const RenderHandleReference& gpuImageHandle) {
                        return info.handle < gpuImageHandle;
                    });
                pos != resourceEnties.end() && pos->handle == gpuImageHandle) {
                if (auto id = uriManager.GetComponentId(pos->entity); id != IComponentManager::INVALID_COMPONENT_ID) {
                    exportImage->uri = uriManager.Get(id).uri;
                }
            }
        }
        return imageArray;
    }

    /* Combines Sampler Image -pairs into GLTF2::Textures. */
    vector<unique_ptr<Texture>> GenerateGltfTextures(
        vector<unique_ptr<Sampler>> const& samplers, vector<unique_ptr<Image>> const& image) const
    {
        vector<unique_ptr<Texture>> textureArray;
        textureArray.reserve(usedTextures_.size());
        for (auto const& samplerImage : usedTextures_) {
            auto& exportTexture = textureArray.emplace_back(make_unique<Texture>());
            if (samplerImage.first < samplers.size()) {
                exportTexture->sampler = samplers[samplerImage.first].get();
            }
            exportTexture->image = image[samplerImage.second].get();
        }
        return textureArray;
    }

private:
    vector<RenderHandleReference> usedSamplers_;
    vector<RenderHandleReference> usedImages_;

    // Each "texture" is an index pair into usedSamplers and usedImages.
    vector<std::pair<size_t, size_t>> usedTextures_;
    vector<uint64_t> textureHashes_;
};

/* Tries to load the glTF resource pointed by URI (<file URI/resource type/resource index>).
 * If the file can be loaded data and index of the resource are returned. */
std::pair<Data*, size_t> ResolveGltfAndResourceIndex(
    string_view uri, IFileManager& fileManager, unordered_map<string, IGLTFData::Ptr>& originalGltfs)
{
    auto const resolveGltfAndResourceIndex =
        [](string_view originalUri, string_view extension, IFileManager& fileManager,
            unordered_map<string, IGLTFData::Ptr>& originalGltfs) -> std::pair<Data*, size_t> {
        const auto gltfPos = originalUri.find(extension);
        if (gltfPos == string_view::npos) {
            return { nullptr, GLTF_INVALID_INDEX };
        }
        auto uri = string_view(originalUri);
        const auto resourceIndexString = uri.substr(uri.rfind('/') + 1);
        uri.remove_suffix(uri.size() - gltfPos - extension.size());

        size_t resourceIndex = GLTF_INVALID_INDEX;
        if (auto const result = std::from_chars(
                resourceIndexString.data(), resourceIndexString.data() + resourceIndexString.size(), resourceIndex);
            result.ec != std::errc()) {
            return { nullptr, GLTF_INVALID_INDEX };
        }

        auto pos = originalGltfs.find(uri);
        if (pos == originalGltfs.end()) {
            auto loadResult = LoadGLTF(fileManager, uri);
            if (!loadResult.success || !loadResult.data->LoadBuffers()) {
                return { nullptr, GLTF_INVALID_INDEX };
            }
            auto inserted = originalGltfs.insert({ uri, move(loadResult.data) });
            pos = inserted.first;
        }
        if (pos != originalGltfs.end()) {
            return { static_cast<Data*>(pos->second.get()), resourceIndex };
        } else {
            return { nullptr, GLTF_INVALID_INDEX };
        }
    };
    if (auto result = resolveGltfAndResourceIndex(uri, ".gltf", fileManager, originalGltfs); result.first) {
        return result;
    }

    if (auto result = resolveGltfAndResourceIndex(uri, ".glb", fileManager, originalGltfs); result.first) {
        return result;
    }

    return { nullptr, GLTF_INVALID_INDEX };
}

void ExportGltfCameras(const IEcs& ecs, const Entities& entities, ExportResult& result)
{
    if (!entities.withCamera.empty() && entities.withCamera.size() == result.data->cameras.size()) {
        if (auto const cameraManager = GetManager<ICameraComponentManager>(ecs); cameraManager) {
            auto cameraIterator = result.data->cameras.begin();

            for (auto const cameraEntity : entities.withCamera) {
                auto& exportCamera = *cameraIterator++;

                auto const cameraComponent = cameraManager->Get(cameraEntity);
                switch (cameraComponent.projection) {
                    case CameraComponent::Projection::ORTHOGRAPHIC: {
                        exportCamera->type = CameraType::ORTHOGRAPHIC;
                        exportCamera->attributes.ortho.xmag = cameraComponent.xMag;
                        exportCamera->attributes.ortho.ymag = cameraComponent.yMag;
                        exportCamera->attributes.ortho.zfar = cameraComponent.zFar;
                        exportCamera->attributes.ortho.znear = cameraComponent.zNear;
                        break;
                    }
                    case CameraComponent::Projection::PERSPECTIVE: {
                        exportCamera->type = CameraType::PERSPECTIVE;
                        exportCamera->attributes.perspective.aspect = cameraComponent.aspect;
                        exportCamera->attributes.perspective.yfov = cameraComponent.yFov;
                        exportCamera->attributes.perspective.zfar = cameraComponent.zFar;
                        exportCamera->attributes.perspective.znear = cameraComponent.zNear;
                        break;
                    }
                    case CameraComponent::Projection::CUSTOM:
                    default: {
                        exportCamera->type = CameraType::INVALID;

                        CORE_LOG_E("cannot export camera %u", static_cast<uint32_t>(cameraComponent.projection));

                        result.error += "failed to export camera";
                        result.success = false;
                        break;
                    }
                }
            }
        }
    }
}

void ExportGltfLight(const IEcs& ecs, const Entities& entities, ExportResult& result)
{
#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)

    if (!entities.withLight.empty() && entities.withLight.size() == result.data->lights.size()) {
        if (auto const lightManager = GetManager<ILightComponentManager>(ecs); lightManager) {
            auto lightIterator = result.data->lights.begin();

            for (auto const lightEntity : entities.withLight) {
                auto& exportLight = *lightIterator++;

                auto const lightComponent = lightManager->Get(lightEntity);
                switch (lightComponent.type) {
                    default:
                        CORE_LOG_E("cannot export light %u", static_cast<uint32_t>(lightComponent.type));
                        exportLight->type = LightType::DIRECTIONAL;

                        result.error += "failed to export light";
                        result.success = false;
                        break;
                    case LightComponent::Type::DIRECTIONAL:
                        exportLight->type = LightType::DIRECTIONAL;
                        break;
                    case LightComponent::Type::POINT:
                        exportLight->type = LightType::POINT;
                        break;
                    case LightComponent::Type::SPOT:
                        exportLight->type = LightType::SPOT;
                        break;
                }
                exportLight->color = lightComponent.color;
                exportLight->intensity = lightComponent.intensity;
                exportLight->positional.range = lightComponent.range;

                exportLight->positional.spot.innerAngle = lightComponent.spotInnerAngle;
                exportLight->positional.spot.outerAngle = lightComponent.spotOuterAngle;

                exportLight->shadow.shadowCaster = lightComponent.shadowEnabled;
            }
        }
    }
#endif
}

Accessor* StoreInverseBindMatrices(array_view<const Math::Mat4X4> ibls, BufferHelper& bufferHelper)
{
    vector<Math::Mat4X4> inverseBindMatrices(ibls.cbegin(), ibls.cend());
    if (!inverseBindMatrices.empty()) {
        auto matrixData = array_view(
            reinterpret_cast<uint8_t*>(inverseBindMatrices.data()), inverseBindMatrices.size() * sizeof(Math::Mat4X4));
        BufferView bufferView;
        bufferView.buffer = &bufferHelper.GetBuffer();
        bufferView.byteLength = matrixData.size();
        bufferView.data = matrixData.data();

        Accessor accessor;
        accessor.bufferView = &bufferView;
        accessor.byteOffset = 0;
        accessor.componentType = ComponentType::FLOAT;
        accessor.count = static_cast<uint32_t>(inverseBindMatrices.size());
        accessor.type = DataType::MAT4;
        return bufferHelper.StoreAccessor(accessor);
    }
    return nullptr;
}

// helper for evaluating skeleton property for a skin
struct NodeDepth {
    uint32_t depth;
    GLTF2::Node* node;

    inline bool operator>(const NodeDepth& rhs) const noexcept
    {
        if (depth > rhs.depth) {
            return true;
        }
        if (depth < rhs.depth) {
            return false;
        } else if (node < rhs.node) {
            return true;
        }
        return false;
    }

    inline bool operator==(const NodeDepth& rhs) const noexcept
    {
        return (depth == rhs.depth) && (node == rhs.node);
    }
};

Node* FindSkeletonRoot(array_view<GLTF2::Node*> joints)
{
    // find the skeleton root node
    if (!joints.empty()) {
        // for each joint calculate distance to the root
        vector<NodeDepth> depths;
        depths.reserve(joints.size());
        for (Node* joint : joints) {
            uint32_t depth = 0;
            for (Node* parent = joint->parent; parent; parent = parent->parent) {
                ++depth;
            }
            depths.push_back({ depth, joint });
        }

        // sort by depth and node
        std::sort(depths.begin(), depths.end(), std::greater<NodeDepth>());

        // reduce the numer of nodes until one remains (or there are multiple roots)
        for (auto start = depths.begin(); depths.size() > 1 && start->depth; start = depths.begin()) {
            // select a range of nodes at an equal depth
            auto end = std::upper_bound(start, depths.end(), start->depth,
                [](uint32_t depth, const NodeDepth& current) { return current.depth < depth; });
            // replace each node with its parent
            for (auto& data : array_view(start.ptr(), end.ptr())) {
                data.node = data.node->parent;
                data.depth -= 1;
            }
            // sort again according to updated depths and nodes
            std::sort(depths.begin(), depths.end(), std::greater<NodeDepth>());

            // remove duplicates
            depths.erase(std::unique(start, depths.end()), depths.end());
        }

        return (depths.size() == 1) ? depths.front().node : nullptr;
    }
    return nullptr;
}

void ExportGltfSkins(const IEcs& ecs, const Entities& entities, const vector<unique_ptr<Node>>& nodeArray,
    ExportResult& result, BufferHelper& bufferHelper)
{
    if (!entities.withSkin.empty() && entities.withSkin.size() == result.data->skins.size()) {
        if (auto const skinManager = GetManager<ISkinComponentManager>(ecs); skinManager) {
            auto const skinIbmManager = GetManager<ISkinIbmComponentManager>(ecs);
            auto const skinJointsManager = GetManager<ISkinJointsComponentManager>(ecs);

            auto skinIterator = result.data->skins.begin();

            for (auto const skinnedEntity : entities.withSkin) {
                auto& exportSkin = *skinIterator++;
                auto const skinComponent = skinManager->Get(skinnedEntity);
                if (EntityUtil::IsValid(skinComponent.skin)) {
                    // store IBMs in the buffer handled by BufferHelper
                    if (const auto ibmHandle = skinIbmManager->Read(skinComponent.skin); ibmHandle) {
                        exportSkin->inverseBindMatrices = StoreInverseBindMatrices(ibmHandle->matrices, bufferHelper);
                    }

                    // gather all the joint nodes
                    if (auto const skinJointsHandle = skinJointsManager->Read(skinnedEntity); skinJointsHandle) {
                        exportSkin->joints.reserve(skinJointsHandle->count);
                        for (auto jointEntity : array_view(skinJointsHandle->jointEntities, skinJointsHandle->count)) {
                            if (auto const jointIndex = FindHandleIndex(entities.nodes, jointEntity);
                                jointIndex < nodeArray.size()) {
                                exportSkin->joints.push_back(nodeArray[jointIndex].get());
                            } else {
                                CORE_LOG_D("joint node not exported");
                            }
                        }
                    }

                    // find the skeleton root node
                    exportSkin->skeleton = FindSkeletonRoot(exportSkin->joints);
                    if (!exportSkin->skeleton) {
                        CORE_LOG_D("Couldn't find common root for skinned entity %s", to_hex(skinnedEntity.id).data());
                    }
                }
            }
        }
    }
}

Node* GetAnimationTarget(const INodeSystem& nodeSystem, const INameComponentManager& nameManager,
    const Entities& entities, array_view<const unique_ptr<Node>> nodes, const Entity trackEntity,
    const AnimationTrackComponent& trackComponent)
{
    Node* target = nullptr;
    if (auto animatedNode = nodeSystem.GetNode(trackComponent.target); animatedNode) {
        if (auto const nodeIndex = FindHandleIndex(entities.nodes, static_cast<Entity>(trackComponent.target));
            nodeIndex < nodes.size()) {
            target = nodes[nodeIndex].get();
        }
    }
    if (!target) {
        string_view nodePath;
        if (const auto nameHandle = nameManager.Read(trackEntity); nameHandle) {
            nodePath = nameHandle->name;
        }
        CORE_LOG_W("couldn't resolve node path: %s", nodePath.data());
    }
    return target;
}

Accessor* AnimationInput(
    const IAnimationInputComponentManager& inputManager, const Entity& animationInput, BufferHelper& bufferHelper)
{
    if (auto inputHandle = inputManager.Read(animationInput); inputHandle) {
        // BufferView data is not const although at this point it's not modified.
        auto inputData = array_view(reinterpret_cast<uint8_t*>(const_cast<float*>(inputHandle->timestamps.data())),
            inputHandle->timestamps.size_in_bytes());

        BufferView bufferView;
        bufferView.buffer = reinterpret_cast<Buffer*>(static_cast<uintptr_t>(animationInput.id));
        bufferView.byteLength = inputData.size();
        bufferView.data = inputData.data();

        Accessor accessor;
        accessor.bufferView = &bufferView;
        accessor.byteOffset = 0;
        accessor.componentType = ComponentType::FLOAT;
        accessor.count = static_cast<uint32_t>(inputHandle->timestamps.size());
        accessor.type = DataType::SCALAR;

        auto inputAccessor = bufferHelper.StoreAccessor(accessor);
        // The accessor used for animation.sampler.input requires min and max values.
        if (inputAccessor && (inputAccessor->min.empty() || inputAccessor->max.empty())) {
            auto input =
                array_view(reinterpret_cast<float const*>(inputAccessor->bufferView->data + inputAccessor->byteOffset),
                    inputAccessor->count);
            auto inputMin = std::numeric_limits<float>::max();
            auto inputMax = std::numeric_limits<float>::lowest();
            for (auto const value : input) {
                inputMin = std::min(value, inputMin);
                inputMax = std::max(value, inputMax);
            }
            inputAccessor->min.push_back(inputMin);
            inputAccessor->max.push_back(inputMax);
        }

        return inputAccessor;
    }
    return {};
}

Accessor* AnimationOutput(const IAnimationOutputComponentManager& outputManager, const Entity& animationOutput,
    AnimationPath type, BufferHelper& bufferHelper)
{
    Accessor accessor;
    // Setup the accessor to match the keyframe data for current animation type. Translation and scale are vec3s,
    // rotation is quaternions, and morph animation is floats.
    auto outputData = array_view<const uint8_t>();
    if (auto outputHandle = outputManager.Read(animationOutput); outputHandle) {
        outputData = outputHandle->data;
        switch (type) {
            case AnimationPath::TRANSLATION:
            case AnimationPath::SCALE: {
                accessor.count = static_cast<uint32_t>(outputData.size() / sizeof(Math::Vec3));
                accessor.type = DataType::VEC3;
            } break;
            case AnimationPath::ROTATION: {
                accessor.count = static_cast<uint32_t>(outputData.size() / sizeof(Math::Vec4));
                accessor.type = DataType::VEC4;
            } break;
            case AnimationPath::WEIGHTS: {
                accessor.count = static_cast<uint32_t>(outputData.size() / sizeof(float));
                accessor.type = DataType::SCALAR;
            } break;
            default:
                return nullptr;
        }
    }
    BufferView bufferView;
    bufferView.buffer = reinterpret_cast<Buffer*>(static_cast<uintptr_t>(animationOutput.id));
    bufferView.byteLength = outputData.size();
    bufferView.data = outputData.data();

    accessor.bufferView = &bufferView;
    accessor.byteOffset = 0;
    accessor.componentType = ComponentType::FLOAT;

    return bufferHelper.StoreAccessor(accessor);
}

unique_ptr<AnimationSampler> CreateAnimationSampler(const AnimationTrackComponent& trackComponent,
    const IAnimationInputComponentManager& animationInputManager,
    const IAnimationOutputComponentManager& animationOutputManager, BufferHelper& bufferHelper)
{
    auto exportSampler = make_unique<AnimationSampler>();
    exportSampler->interpolation = GetAnimationInterpolation(trackComponent.interpolationMode);
    exportSampler->input = AnimationInput(animationInputManager, trackComponent.timestamps, bufferHelper);
    exportSampler->output =
        AnimationOutput(animationOutputManager, trackComponent.data, GetAnimationPath(trackComponent), bufferHelper);
    return exportSampler;
}

void CleanupAnimation(Animation& exportAnimation)
{
    // Remove all tracks that don't have a node, sampler or sampler is missing input or output.
    exportAnimation.tracks.erase(std::find_if(exportAnimation.tracks.begin(), exportAnimation.tracks.end(),
                                    [](const AnimationTrack& track) {
                                        return !track.channel.node || !track.sampler || !track.sampler->input ||
                                               !track.sampler->output;
                                    }),
        exportAnimation.tracks.end());

    // Remove all samplers missing input or output.
    exportAnimation.samplers.erase(
        std::find_if(exportAnimation.samplers.begin(), exportAnimation.samplers.end(),
            [](const unique_ptr<AnimationSampler>& sampler) { return !sampler->input || !sampler->output; }),
        exportAnimation.samplers.end());
}

uint64_t Hash(const AnimationTrackComponent& trackComponent)
{
    return BASE_NS::Hash(static_cast<const Entity&>(trackComponent.timestamps).id,
        static_cast<const Entity&>(trackComponent.data).id, static_cast<uint32_t>(trackComponent.interpolationMode));
}

void ExportGltfAnimations(const IEcs& ecs, const Entities& entities, ExportResult& result, BufferHelper& bufferHelper)
{
    auto const nodeSystem = GetSystem<INodeSystem>(ecs);
    auto const animationSystem = GetSystem<IAnimationSystem>(ecs);
    auto const animationManager = GetManager<IAnimationComponentManager>(ecs);
    auto const animationInputManager = GetManager<IAnimationInputComponentManager>(ecs);
    auto const animationOutputManager = GetManager<IAnimationOutputComponentManager>(ecs);
    auto const animationTrackManager = GetManager<IAnimationTrackComponentManager>(ecs);
    auto const nameManager = GetManager<INameComponentManager>(ecs);
    if (nodeSystem && animationSystem && animationManager && animationInputManager && animationOutputManager &&
        animationTrackManager && nameManager) {
        auto const animationCount = animationManager->GetComponentCount();
        auto& animationArray = result.data->animations;
        animationArray.reserve(animationCount);

        for (IComponentManager::ComponentId i = 0U; i < animationCount; ++i) {
            auto& exportAnimation = animationArray.emplace_back(make_unique<Animation>());
            if (auto nameHandle = nameManager->Read(animationManager->GetEntity(i)); nameHandle) {
                exportAnimation->name = nameHandle->name;
            }

            // animation.samplers can be shared between channels. Identify samplers by hashing input, output and
            // interpolationMode.
            vector<uint64_t> samplerHashes;

            if (const auto animationHandle = animationManager->Read(i); animationHandle) {
                for (auto const& trackEntity : animationHandle->tracks) {
                    if (const auto trackHandle = animationTrackManager->Read(trackEntity); trackHandle) {
                        auto const samplerIndex = FindOrAddIndex(samplerHashes, Hash(*trackHandle));
                        if ((samplerIndex + 1) >= exportAnimation->samplers.size()) {
                            exportAnimation->samplers.resize(samplerIndex + 1);
                            exportAnimation->samplers[samplerIndex] = CreateAnimationSampler(
                                *trackHandle, *animationInputManager, *animationOutputManager, bufferHelper);
                        }
                        const auto target = GetAnimationTarget(
                            *nodeSystem, *nameManager, entities, result.data->nodes, trackEntity, *trackHandle);
                        exportAnimation->tracks.push_back(AnimationTrack { { target, GetAnimationPath(*trackHandle) },
                            exportAnimation->samplers[samplerIndex].get() });
                    }
                }
            }

            CleanupAnimation(*exportAnimation);

            // Pop the animation if it's empty.
            if (exportAnimation->samplers.empty() || exportAnimation->tracks.empty()) {
                animationArray.pop_back();
            }
        }
    }
}

struct MeshPrimitiveGenerator {
    const IMaterialComponentManager& materialManager;
    BufferHelper& buffer;
    vector<Entity>& usedMaterials;

    MeshPrimitive operator()(const MeshComponent::Submesh& submesh, const MeshPrimitive& original)
    {
        MeshPrimitive copy;
        if (original.indices) {
            copy.indices = buffer.StoreAccessor(*original.indices);
        }
        for (auto const& attrib : original.attributes) {
            copy.attributes.push_back(Attribute { attrib.attribute, buffer.StoreAccessor(*attrib.accessor) });
        }
        if (materialManager.HasComponent(submesh.material)) {
            copy.materialIndex = FindOrAddIndex(usedMaterials, submesh.material);
        }
        for (auto const& target : original.targets) {
            MorphTarget morphTarget { target.name, {} };
            for (auto const& attribute : target.target) {
                morphTarget.target.push_back(
                    Attribute { attribute.attribute, buffer.StoreAccessor(*attribute.accessor) });
            }
            copy.targets.push_back(move(morphTarget));
        }

        return copy;
    }
};

vector<Entity> ExportGltfMeshes(const IMeshComponentManager& meshManager, const INameComponentManager& nameManager,
    const IUriComponentManager& uriManager, const IMaterialComponentManager& materialManager, IFileManager& fileManager,
    const vector<Entity>& usedMeshes, ExportResult& result, BufferHelper& buffer,
    unordered_map<string, IGLTFData::Ptr>& originalGltfs)
{
    vector<Entity> usedMaterials;
    CORE_ASSERT(usedMeshes.size() == result.data->meshes.size());
    if (!usedMeshes.empty() && usedMeshes.size() == result.data->meshes.size()) {
        auto meshIterator = result.data->meshes.begin();

        // Create GLTF2::Meshes from mesh entities
        for (auto const meshEntity : usedMeshes) {
            auto& exportMesh = *meshIterator++;

            if (auto const uriId = uriManager.GetComponentId(meshEntity);
                uriId != IComponentManager::INVALID_COMPONENT_ID) {
                auto const uc = uriManager.Get(uriId);
                // mesh data is copied from the original glTF pointer by UriComponent
                if (const auto [originalGltf, meshIndex] =
                        ResolveGltfAndResourceIndex(uc.uri, fileManager, originalGltfs);
                    originalGltf && meshIndex < originalGltf->meshes.size()) {
                    auto const meshData = meshManager.GetData(meshEntity);
                    auto const& mesh = *static_cast<const MeshComponent*>(meshData->RLock());

                    auto const& submeshes = mesh.submeshes;
                    auto const& originalPrimitives = originalGltf->meshes[meshIndex]->primitives;
                    std::transform(submeshes.begin(), submeshes.end(), originalPrimitives.begin(),
                        std::back_inserter(exportMesh->primitives),
                        MeshPrimitiveGenerator { materialManager, buffer, usedMaterials });

                    if (const auto nameId = nameManager.GetComponentId(meshEntity);
                        nameId != IComponentManager::INVALID_COMPONENT_ID) {
                        exportMesh->name = nameManager.Get(nameId).name;
                    }
                    // NOTE: exportMesh->weights
                } else {
                    // couldn't find original glTF.
                    for (const auto& node : result.data->nodes) {
                        if (node->mesh == exportMesh.get()) {
                            node->mesh = nullptr;
                        }
                    }
                    exportMesh.reset();
                }
            }
        }
        result.data->meshes.erase(
            std::remove(result.data->meshes.begin(), result.data->meshes.end(), nullptr), result.data->meshes.end());
    }
    return usedMaterials;
}

inline uint32_t GetTextureIndex(const MaterialComponent& materialDesc,
    const MaterialComponent::TextureIndex textureIndex, TextureHelper& textureHelper,
    const IRenderHandleComponentManager& gpuHandleManager)
{
    RenderHandleReference image;
    if (auto handle = gpuHandleManager.Read(materialDesc.textures[textureIndex].image); handle) {
        image = handle->reference;
    }
    if (image) {
        auto const imageIndex = textureHelper.GetImageIndex(image);
        RenderHandleReference sampler;
        if (auto handle = gpuHandleManager.Read(materialDesc.textures[textureIndex].sampler); handle) {
            sampler = handle->reference;
        }
        auto const samplerIndex = (sampler) ? textureHelper.GetSamplerIndex(sampler) : 0xFFFFffff;
        return textureHelper.GetTextureIndex(samplerIndex, imageIndex);
    }
    return GLTF_INVALID_INDEX;
}

void ExportGltfMaterialMetallicRoughness(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    exportMaterial.type = Material::Type::MetallicRoughness;
    exportMaterial.metallicRoughness.baseColorFactor =
        materialDesc.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor;
    exportMaterial.metallicRoughness.baseColorTexture.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::BASE_COLOR, textureHelper, gpuHandleManager);
    exportMaterial.metallicRoughness.metallicFactor =
        materialDesc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.z;
    exportMaterial.metallicRoughness.roughnessFactor =
        materialDesc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.y;
    exportMaterial.metallicRoughness.metallicRoughnessTexture.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::MATERIAL, textureHelper, gpuHandleManager);
}

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT) || defined(GLTF2_EXTRAS_CLEAR_COAT_MATERIAL)
void ExportGltfMaterialClearcoat(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    // Clearcoat (must not be used with pbrSpecularGlossiness or unlit).
    if (materialDesc.textures[MaterialComponent::TextureIndex::CLEARCOAT].factor.x > 0.0f) {
        exportMaterial.clearcoat.factor = materialDesc.textures[MaterialComponent::TextureIndex::CLEARCOAT].factor.x;
        exportMaterial.clearcoat.roughness =
            materialDesc.textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].factor.y;
        exportMaterial.clearcoat.texture.index =
            GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::CLEARCOAT, textureHelper, gpuHandleManager);
        exportMaterial.clearcoat.roughnessTexture.index = GetTextureIndex(
            materialDesc, MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS, textureHelper, gpuHandleManager);
        exportMaterial.clearcoat.normalTexture.textureInfo.index = GetTextureIndex(
            materialDesc, MaterialComponent::TextureIndex::CLEARCOAT_NORMAL, textureHelper, gpuHandleManager);
        exportMaterial.clearcoat.normalTexture.scale =
            materialDesc.textures[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL].factor.x;
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
void ExportGltfMaterialIor(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    // IOR (must not be used with pbrSpecularGlossiness or unlit).
    if (materialDesc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.w != 0.04f) {
        const auto refSqr = Math::sqrt(materialDesc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.w);
        exportMaterial.ior.ior = (1.f + refSqr) / (1.f - refSqr);
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
void ExportGltfMaterialSheen(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    // Sheen (must not be used with pbrSpecularGlossiness or unlit).
    if ((materialDesc.textures[MaterialComponent::TextureIndex::SHEEN].factor.x > 0.0f) ||
        (materialDesc.textures[MaterialComponent::TextureIndex::SHEEN].factor.y > 0.0f) ||
        (materialDesc.textures[MaterialComponent::TextureIndex::SHEEN].factor.z > 0.0f)) {
        exportMaterial.sheen.factor = materialDesc.textures[MaterialComponent::TextureIndex::SHEEN].factor;
        exportMaterial.sheen.texture.index =
            GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::SHEEN, textureHelper, gpuHandleManager);
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
void ExportGltfMaterialSpecular(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    // Specular (must not be used with pbrSpecularGlossiness or unlit).
    if (materialDesc.textures[MaterialComponent::TextureIndex::SPECULAR].factor != Math::Vec4(1.f, 1.f, 1.f, 1.f)) {
        exportMaterial.specular.factor = materialDesc.textures[MaterialComponent::TextureIndex::SPECULAR].factor.w;
        exportMaterial.specular.texture.index =
            GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::SPECULAR, textureHelper, gpuHandleManager);
        exportMaterial.specular.color = materialDesc.textures[MaterialComponent::TextureIndex::SPECULAR].factor;
        exportMaterial.specular.colorTexture.index =
            GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::SPECULAR, textureHelper, gpuHandleManager);
    }
}
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
void ExportGltfMaterialTransmission(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    // Transmission (must not be used with pbrSpecularGlossiness or unlit).
    if (materialDesc.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x > 0.0f) {
        exportMaterial.transmission.factor =
            materialDesc.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x;
        exportMaterial.transmission.texture.index = GetTextureIndex(
            materialDesc, MaterialComponent::TextureIndex::TRANSMISSION, textureHelper, gpuHandleManager);
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
void ExportGltfMaterialSpecularGlossiness(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    exportMaterial.type = Material::Type::SpecularGlossiness;
    exportMaterial.specularGlossiness.diffuseFactor =
        materialDesc.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor;
    exportMaterial.specularGlossiness.diffuseTexture.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::BASE_COLOR, textureHelper, gpuHandleManager);
    exportMaterial.specularGlossiness.specularFactor =
        materialDesc.textures[MaterialComponent::TextureIndex::MATERIAL].factor;
    exportMaterial.specularGlossiness.specularGlossinessTexture.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::MATERIAL, textureHelper, gpuHandleManager);
    exportMaterial.specularGlossiness.glossinessFactor =
        materialDesc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.w;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_UNLIT)
void ExportGltfMaterialUnlit(Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    exportMaterial.type = Material::Type::Unlit;
    exportMaterial.metallicRoughness.baseColorFactor =
        materialDesc.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor;
    exportMaterial.metallicRoughness.baseColorTexture.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::BASE_COLOR, textureHelper, gpuHandleManager);
}
#endif

void UpdateShaderStateToGltfMaterial(const IDevice* device, Material& exportMaterial,
    const MaterialComponent& materialDesc, const IRenderHandleComponentManager& gpuHandleManager)
{
    if ((materialDesc.materialShader.shader || materialDesc.materialShader.graphicsState) && device) {
        const IShaderManager& shaderMgr = device->GetShaderManager();
        if (materialDesc.materialShader.graphicsState) {
            const auto handle = gpuHandleManager.GetRenderHandleReference(materialDesc.materialShader.graphicsState);
            if (handle.GetHandleType() == RenderHandleType::GRAPHICS_STATE && handle) {
                const GraphicsState gfxState = shaderMgr.GetGraphicsState(handle);
                if (gfxState.rasterizationState.cullModeFlags == CullModeFlagBits::CORE_CULL_MODE_NONE) {
                    exportMaterial.doubleSided = true;
                }
                if (gfxState.colorBlendState.colorAttachmentCount > 0) {
                    if (gfxState.colorBlendState.colorAttachments[0].enableBlend) {
                        exportMaterial.alphaMode = AlphaMode::BLEND;
                    }
                }
            }
        } else if (materialDesc.materialShader.shader) {
            const auto handle = gpuHandleManager.GetRenderHandleReference(materialDesc.materialShader.shader);
            if (handle.GetHandleType() == RenderHandleType::SHADER_STATE_OBJECT && handle) {
                const RenderHandleReference gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(handle);
                const GraphicsState gfxState = shaderMgr.GetGraphicsState(gfxHandle);
                if (gfxState.rasterizationState.cullModeFlags == CullModeFlagBits::CORE_CULL_MODE_NONE) {
                    exportMaterial.doubleSided = true;
                }
                if (gfxState.colorBlendState.colorAttachmentCount > 0) {
                    if (gfxState.colorBlendState.colorAttachments[0].enableBlend) {
                        exportMaterial.alphaMode = AlphaMode::BLEND;
                    }
                }
            }
        }
    }
}

void ExportGltfMaterial(const IDevice* device, Material& exportMaterial, const MaterialComponent& materialDesc,
    TextureHelper& textureHelper, const IRenderHandleComponentManager& gpuHandleManager)
{
    if (materialDesc.type == MaterialComponent::Type::METALLIC_ROUGHNESS) {
        ExportGltfMaterialMetallicRoughness(exportMaterial, materialDesc, textureHelper, gpuHandleManager);

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT) || defined(GLTF2_EXTRAS_CLEAR_COAT_MATERIAL)
        ExportGltfMaterialClearcoat(exportMaterial, materialDesc, textureHelper, gpuHandleManager);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
        ExportGltfMaterialIor(exportMaterial, materialDesc, textureHelper, gpuHandleManager);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
        ExportGltfMaterialSheen(exportMaterial, materialDesc, textureHelper, gpuHandleManager);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
        ExportGltfMaterialSpecular(exportMaterial, materialDesc, textureHelper, gpuHandleManager);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
        ExportGltfMaterialTransmission(exportMaterial, materialDesc, textureHelper, gpuHandleManager);
#endif
    } else if (materialDesc.type == MaterialComponent::Type::SPECULAR_GLOSSINESS) {
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
        ExportGltfMaterialSpecularGlossiness(exportMaterial, materialDesc, textureHelper, gpuHandleManager);
#endif
    } else if (materialDesc.type == MaterialComponent::Type::UNLIT) {
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_UNLIT)
        ExportGltfMaterialUnlit(exportMaterial, materialDesc, textureHelper, gpuHandleManager);
#endif
    }

    // Normal texture.
    exportMaterial.normalTexture.textureInfo.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::NORMAL, textureHelper, gpuHandleManager);
    exportMaterial.normalTexture.scale = materialDesc.textures[MaterialComponent::TextureIndex::NORMAL].factor.x;

    // Occlusion texture.
    exportMaterial.occlusionTexture.textureInfo.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::AO, textureHelper, gpuHandleManager);
    exportMaterial.occlusionTexture.strength = materialDesc.textures[MaterialComponent::TextureIndex::AO].factor.x;

    // Emissive texture.
    exportMaterial.emissiveTexture.index =
        GetTextureIndex(materialDesc, MaterialComponent::TextureIndex::EMISSIVE, textureHelper, gpuHandleManager);

    exportMaterial.emissiveFactor = materialDesc.textures[MaterialComponent::TextureIndex::EMISSIVE].factor;
    exportMaterial.alphaCutoff = materialDesc.alphaCutoff;
    exportMaterial.alphaMode = AlphaMode::OPAQUE;

    UpdateShaderStateToGltfMaterial(device, exportMaterial, materialDesc, gpuHandleManager);

    // check if alpha is using cutoff / mask (Lume default is 1.0 -> no mask)
    if (materialDesc.alphaCutoff < 1.0f) {
        exportMaterial.alphaMode = AlphaMode::MASK;
    }
}

void ExportGltfMaterials(const IEngine& engine, const IMaterialComponentManager& materialManager,
    const INameComponentManager& nameManager, const IUriComponentManager& uriManager,
    const vector<Entity>& usedMaterials, ExportResult& result, BufferHelper& bufferHelper)
{
    if (!usedMaterials.empty()) {
        IDevice* device = nullptr;
        if (auto context = GetInstance<IRenderContext>(*engine.GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
            context) {
            device = &context->GetDevice();
        }
        IRenderHandleComponentManager& gpuHandleManager =
            *GetManager<IRenderHandleComponentManager>(materialManager.GetEcs());

        TextureHelper textureHash;

        auto& materialArray = result.data->materials;
        materialArray.reserve(usedMaterials.size());

        // Create Materials and gather used samplers and images.
        for (auto const materialEntity : usedMaterials) {
            string_view name;
            if (const auto nameHandle = nameManager.Read(materialEntity); nameHandle) {
                name = nameHandle->name;
            }
            if (const auto materialHandle = materialManager.Read(materialEntity); materialHandle) {
                auto& exportMaterial = materialArray.emplace_back(make_unique<Material>());
                const auto& material = *materialHandle;
                exportMaterial->name = name;
                ExportGltfMaterial(device, *exportMaterial, material, textureHash, gpuHandleManager);
            }
        }

        vector<ResourceEntity> resourceEnties;
        const auto gpuHandleComponents = gpuHandleManager.GetComponentCount();
        resourceEnties.reserve(gpuHandleComponents);
        // sorted for find performance
        for (auto i = 0u; i < gpuHandleComponents; ++i) {
            resourceEnties.push_back({ gpuHandleManager.GetEntity(i), gpuHandleManager.Get(i).reference });
        }
        std::sort(resourceEnties.begin(), resourceEnties.end(),
            [](const ResourceEntity& lhs, const ResourceEntity& rhs) { return lhs.handle < rhs.handle; });

        // Create Samplers
        if (device && textureHash.HasSamplers()) {
            result.data->samplers = textureHash.GenerateGltfSamplers(device->GetGpuResourceManager());
        }

        // Create Images
        if (textureHash.HasImages()) {
            result.data->images = textureHash.GenerateGltfImages(resourceEnties, uriManager);
        }

        // Create Textures
        if (textureHash.HasTextures()) {
            result.data->textures = textureHash.GenerateGltfTextures(result.data->samplers, result.data->images);
        }
    }
}

void ExportImageData(IFileManager& fileManager, ExportResult& result, BufferHelper& bufferHelper,
    unordered_map<string, IGLTFData::Ptr>& originalGltfs)
{
    for (auto& image : result.data->images) {
        if (!image->uri.empty()) {
            // First check if the URI is from ResourceManager in the form of <file URI/resource type/resource
            // index>. If that fails, try to open the URI as a file.
            if (const auto [originalGltf, imageIndex] =
                    ResolveGltfAndResourceIndex(image->uri, fileManager, originalGltfs);
                originalGltf && imageIndex < originalGltf->images.size()) {
                // We can store data from the loaded bufferView.
                auto& originalImage = originalGltf->images[imageIndex];
                image->bufferView = bufferHelper.StoreBufferView(
                    *originalImage->bufferView, GetComponentByteSize(ComponentType::UNSIGNED_INT));
                image->bufferView->target = BufferTarget::NOT_DEFINED;
                image->type = originalImage->type;
                image->uri.clear();
            } else if (auto imageFile = fileManager.OpenFile(image->uri); imageFile) {
                auto uri = string_view(image->uri);
                // Leave only the file extension.
                if (auto const ext = uri.rfind('.'); ext != string_view::npos) {
                    uri.remove_prefix(ext + 1);
                }

                // Resolve image type from the file extension
                if (uri == "png") {
                    image->type = MimeType::PNG;
                } else if (uri == "jpg") {
                    image->type = MimeType::JPEG;
                } else if (uri == "ktx") {
                    image->type = MimeType::KTX;
                } else if (uri == "ktx2") {
                    image->type = MimeType::KTX2;
                } else if (uri == "dds") {
                    image->type = MimeType::DDS;
                }

                if (image->type != MimeType::INVALID) {
                    // Read the image directly to the data buffer.
                    const size_t imageSize = static_cast<const size_t>(imageFile->GetLength());
                    auto& dataBuffer = bufferHelper.GetBuffer().data;
                    auto const imageOffset = dataBuffer.size();

                    dataBuffer.resize(imageOffset + imageSize);

                    imageFile->Read(dataBuffer.data() + imageOffset, imageSize);

                    BufferView bufferView;
                    bufferView.buffer = &bufferHelper.GetBuffer();
                    bufferView.byteLength = imageSize;
                    bufferView.byteOffset = imageOffset;
                    bufferView.data = dataBuffer.data() + imageOffset;

                    // The special zero data aligment skips copying the data again.
                    image->bufferView = bufferHelper.StoreBufferView(bufferView, 0);

                    image->uri.clear();
                }
            }
        }
    }
}

// The following Export* functions return a JSON object containing the related parts of the GLTF2::Data.
json::value ExportAccessorSparse(const Data& data, const Accessor& accessor)
{
    json::value jsonSparse = json::value::object {};
    {
        jsonSparse["count"] = accessor.sparse.count;
        {
            json::value jsonSparseIndices = json::value::array {};
            jsonSparseIndices["bufferView"] = FindObjectIndex(data.bufferViews, *accessor.sparse.indices.bufferView);
            jsonSparseIndices["byteOffset"] = accessor.sparse.indices.byteOffset;
            jsonSparseIndices["componentType"] = static_cast<int>(accessor.sparse.indices.componentType);
            jsonSparse["indices"] = move(jsonSparseIndices);
        }
        {
            json::value jsonSparseValues = json::value::array {};
            jsonSparseValues["bufferView"] = FindObjectIndex(data.bufferViews, *accessor.sparse.values.bufferView);
            jsonSparseValues["byteOffset"] = accessor.sparse.values.byteOffset;
            jsonSparse["values"] = move(jsonSparseValues);
        }
    }
    return jsonSparse;
}

json::value ExportAccessors(Data const& data)
{
    json::value jsonAccessors = json::value::array {};
    for (auto const& accessor : data.accessors) {
        json::value jsonAccessor = json::value::object {};
        if (accessor->bufferView) {
            jsonAccessor["bufferView"] = FindObjectIndex(data.bufferViews, *accessor->bufferView);
        }
        if (accessor->byteOffset) {
            jsonAccessor["byteOffset"] = accessor->byteOffset;
        }
        jsonAccessor["componentType"] = static_cast<int>(accessor->componentType);
        if (accessor->normalized) {
            jsonAccessor["normalized"] = accessor->normalized;
        }
        jsonAccessor["count"] = accessor->count;
        jsonAccessor["type"] = GetDataType(accessor->type);

        if (!accessor->max.empty()) {
            jsonAccessor["max"] = accessor->max;
        }
        if (!accessor->min.empty()) {
            jsonAccessor["min"] = accessor->min;
        }
        if (accessor->sparse.count) {
            jsonAccessor["sparse"] = ExportAccessorSparse(data, *accessor);
        }
#ifdef EXPORT_OTHER_OBJECT_NAMES
        if (accessor->Name) {
            jsonAccessor["name"] = accessor->normalized;
        }
#endif

        jsonAccessors.array_.push_back(move(jsonAccessor));
    }
    return jsonAccessors;
}

json::value ExportAnimations(Data const& data)
{
    json::value jsonAnimations = json::value::array {};
    for (auto const& animation : data.animations) {
        json::value jsonAnimation = json::value::object {};
        {
            json::value jsonSamplers = json::value::array {};
            for (auto const& sampler : animation->samplers) {
                json::value jsonSampler = json::value::object {};
                jsonSampler["input"] = FindObjectIndex(data.accessors, *sampler->input);
                if (sampler->interpolation != AnimationInterpolation::LINEAR) {
                    jsonSampler["interpolation"] = GetAnimationInterpolation(sampler->interpolation);
                }
                jsonSampler["output"] = FindObjectIndex(data.accessors, *sampler->output);
                jsonSamplers.array_.push_back(move(jsonSampler));
            }
            jsonAnimation["samplers"] = move(jsonSamplers);
        }
        {
            json::value jsonChannels = json::value::array {};
            for (auto const& track : animation->tracks) {
                json::value jsonChannel = json::value::object {};
                jsonChannel["sampler"] = FindObjectIndex(animation->samplers, *track.sampler);
                {
                    json::value jsonTarget = json::value::object {};
                    if (auto const nodeIndex = FindObjectIndex(data.nodes, *track.channel.node); 0 <= nodeIndex) {
                        jsonTarget["node"] = nodeIndex;
                    }
                    jsonTarget["path"] = GetAnimationPath(track.channel.path);
                    jsonChannel["target"] = move(jsonTarget);
                }
                jsonChannels.array_.push_back(move(jsonChannel));
            }
            jsonAnimation["channels"] = move(jsonChannels);
        }
        if (!animation->name.empty()) {
            jsonAnimation["name"] = string_view(animation->name);
        }
        jsonAnimations.array_.push_back(move(jsonAnimation));
    }
    return jsonAnimations;
}

json::value ExportBuffers(Data const& data, vector<string>& strings)
{
    json::value jsonBuffers = json::value::array {};
    for (auto const& buffer : data.buffers) {
        json::value jsonBuffer = json::value::object {};
        if (!buffer->uri.empty()) {
            jsonBuffer["uri"] = string_view(buffer->uri);
        }
        jsonBuffer["byteLength"] = buffer->byteLength;
#ifdef EXPORT_OTHER_OBJECT_NAMES
        if (!buffer->name.empty()) {
            jsonBuffer["name"] = buffer->name;
        }
#endif
        jsonBuffers.array_.push_back(move(jsonBuffer));
    }
    return jsonBuffers;
}

json::value ExportBufferViews(Data const& data)
{
    json::value jsonBufferViews = json::value::array {};
    for (auto const& bufferView : data.bufferViews) {
        json::value jsonBufferView = json::value::object {};
        jsonBufferView["buffer"] = FindObjectIndex(data.buffers, *bufferView->buffer);
        if (bufferView->byteOffset) {
            jsonBufferView["byteOffset"] = bufferView->byteOffset;
        }
        jsonBufferView["byteLength"] = bufferView->byteLength;
        if (bufferView->byteStride) {
            jsonBufferView["byteStride"] = bufferView->byteStride;
        }
        if (bufferView->target != BufferTarget::NOT_DEFINED) {
            jsonBufferView["target"] = static_cast<int>(bufferView->target);
        }
#ifdef EXPORT_OTHER_OBJECT_NAMES
        if (!bufferView->name.empty()) {
            jsonBufferView["name"] = bufferView->name;
        }
#endif
        jsonBufferViews.array_.push_back(move(jsonBufferView));
    }
    return jsonBufferViews;
}

json::value ExportCameras(Data const& data)
{
    json::value jsonCameras = json::value::array {};

    for (auto const& camera : data.cameras) {
        json::value jsonCamera = json::value::object {};
        jsonCamera["type"] = json::value { GetCameraType(camera->type) };

        if (!camera->name.empty()) {
            jsonCamera["name"] = json::value { camera->name };
        }
        if (camera->type == CameraType::PERSPECTIVE) {
            json::value jsonPerspective = json::value::object {};
            if (camera->attributes.perspective.aspect > 0.f) {
                jsonPerspective["aspectRatio"] = camera->attributes.perspective.aspect;
            }
            jsonPerspective["yfov"] = camera->attributes.perspective.yfov;
            if (camera->attributes.perspective.zfar > 0.f) {
                jsonPerspective["zfar"] = camera->attributes.perspective.zfar;
            }
            jsonPerspective["znear"] = camera->attributes.perspective.znear;
            jsonCamera["perspective"] = move(jsonPerspective);
        } else if (camera->type == CameraType::ORTHOGRAPHIC) {
            json::value jsonOrthographic = json::value::object {};
            jsonOrthographic["xmag"] = json::value { camera->attributes.ortho.xmag };
            jsonOrthographic["ymag"] = json::value { camera->attributes.ortho.ymag };
            jsonOrthographic["zfar"] = camera->attributes.ortho.zfar;
            jsonOrthographic["znear"] = camera->attributes.ortho.znear;
            jsonCamera["orthographic"] = move(jsonOrthographic);
        }

        jsonCameras.array_.push_back(move(jsonCamera));
    }

    return jsonCameras;
}

json::value ExportImages(Data const& data)
{
    json::value jsonImages = json::value::array {};
    for (auto const& image : data.images) {
        json::value jsonImage = json::value::object {};

        if (!image->uri.empty()) {
            jsonImage["uri"] = string_view(image->uri);
        } else if (image->bufferView) {
            jsonImage["mimeType"] = GetMimeType(image->type);
            jsonImage["bufferView"] = FindObjectIndex(data.bufferViews, *image->bufferView);
        }
#ifdef EXPORT_OTHER_OBJECT_NAMES
        if (!image->name.empty()) {
            jsonImage["name"] = image->name;
        }
#endif
        jsonImages.array_.push_back(move(jsonImage));
    }
    return jsonImages;
}

json::value ExportTextureInfo(TextureInfo const& textureInfo)
{
    json::value jsonTextureInfo = json::value::object {};
    jsonTextureInfo["index"] = textureInfo.index;
    if (textureInfo.texCoordIndex != 0 && textureInfo.texCoordIndex != GLTF_INVALID_INDEX) {
        jsonTextureInfo["texCoord"] = textureInfo.texCoordIndex;
    }
    return jsonTextureInfo;
}

json::value ExportMetallicRoughness(const Material& material)
{
    json::value jsonMetallicRoughness = json::value::object {};
    if (material.metallicRoughness.baseColorFactor != DEFAULT_BASECOLOR_FACTOR) {
        jsonMetallicRoughness["baseColorFactor"] = material.metallicRoughness.baseColorFactor.data;
    }
    if (material.metallicRoughness.baseColorTexture.index != GLTF_INVALID_INDEX) {
        jsonMetallicRoughness["baseColorTexture"] = ExportTextureInfo(material.metallicRoughness.baseColorTexture);
    }
    if (material.metallicRoughness.metallicFactor < 1.f) {
        jsonMetallicRoughness["metallicFactor"] = material.metallicRoughness.metallicFactor;
    }
    if (material.metallicRoughness.roughnessFactor < 1.f) {
        jsonMetallicRoughness["roughnessFactor"] = material.metallicRoughness.roughnessFactor;
    }
    if (material.metallicRoughness.metallicRoughnessTexture.index != GLTF_INVALID_INDEX) {
        jsonMetallicRoughness["metallicRoughnessTexture"] =
            ExportTextureInfo(material.metallicRoughness.metallicRoughnessTexture);
    }
    return jsonMetallicRoughness;
}

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
json::value ExportSpecularGlossiness(const Material& material)
{
    json::value jsonSpecularGlossiness = json::value::object {};
    if (material.specularGlossiness.diffuseFactor != DEFAULT_DIFFUSE_FACTOR) {
        jsonSpecularGlossiness["diffuseFactor"] = material.specularGlossiness.diffuseFactor.data;
    }
    if (material.specularGlossiness.diffuseTexture.index != GLTF_INVALID_INDEX) {
        jsonSpecularGlossiness["diffuseTexture"] = ExportTextureInfo(material.specularGlossiness.diffuseTexture);
    }
    if (material.specularGlossiness.specularFactor != DEFAULT_SPECULAR_FACTOR) {
        jsonSpecularGlossiness["specularFactor"] = material.specularGlossiness.specularFactor.data;
    }
    if (material.specularGlossiness.glossinessFactor != 1.f) {
        jsonSpecularGlossiness["glossinessFactor"] = material.specularGlossiness.glossinessFactor;
    }
    if (material.specularGlossiness.specularGlossinessTexture.index != GLTF_INVALID_INDEX) {
        jsonSpecularGlossiness["specularGlossinessTexture"] =
            ExportTextureInfo(material.specularGlossiness.specularGlossinessTexture);
    }
    return jsonSpecularGlossiness;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT)
json::value ExportClearcoat(const Material::Clearcoat& clearcoat)
{
    json::value jsonClearcoat = json::value::object {};
    if (clearcoat.factor != 0.f) {
        jsonClearcoat["clearcoatFactor"] = clearcoat.factor;
    }
    if (clearcoat.texture.index != GLTF_INVALID_INDEX) {
        jsonClearcoat["clearcoatTexture"] = ExportTextureInfo(clearcoat.texture);
    }
    if (clearcoat.roughness != 0.f) {
        jsonClearcoat["clearcoatRoughnessFactor"] = clearcoat.roughness;
    }
    if (clearcoat.roughnessTexture.index != GLTF_INVALID_INDEX) {
        jsonClearcoat["clearcoatRoughnessTexture"] = ExportTextureInfo(clearcoat.roughnessTexture);
    }
    if (clearcoat.normalTexture.textureInfo.index != GLTF_INVALID_INDEX) {
        auto& jsonNormalTexture = jsonClearcoat["clearcoatNormalTexture"] =
            ExportTextureInfo(clearcoat.normalTexture.textureInfo);
        if (clearcoat.normalTexture.scale != 1.f) {
            jsonNormalTexture["scale"] = clearcoat.normalTexture.scale;
        }
    }
    return jsonClearcoat;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_EMISSIVE_STRENGTH)
json::value ExportEmissiveStrength(const float strength)
{
    json::value jsonEmissiveStrength = json::value::object {};
    if (strength != 1.f) {
        jsonEmissiveStrength["emissiveStrength"] = strength;
    }
    return jsonEmissiveStrength;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
json::value ExportIor(const Material::Ior& ior)
{
    json::value jsonIor = json::value::object {};
    if (ior.ior != 1.5f) {
        jsonIor["ior"] = ior.ior;
    }
    return jsonIor;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
json::value ExportSheen(const Material::Sheen& sheen)
{
    json::value jsonSheen = json::value::object {};
    if (sheen.factor != Math::Vec3 {}) {
        jsonSheen["sheenColorFactor"] = sheen.factor.data;
    }
    if (sheen.texture.index != GLTF_INVALID_INDEX) {
        jsonSheen["sheenColorTexture"] = ExportTextureInfo(sheen.texture);
    }
    if (sheen.roughness != 0.f) {
        jsonSheen["sheenRoughnessFactor"] = sheen.roughness;
    }
    if (sheen.roughnessTexture.index != GLTF_INVALID_INDEX) {
        jsonSheen["sheenRoughnessTexture"] = ExportTextureInfo(sheen.roughnessTexture);
    }
    return jsonSheen;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
json::value ExportSpecular(const Material::Specular& specular)
{
    json::value jsonSpecular = json::value::object {};
    if (specular.factor != 1.f) {
        jsonSpecular["specularFactor"] = specular.factor;
    }
    if (specular.texture.index != GLTF_INVALID_INDEX) {
        jsonSpecular["specularTexture"] = ExportTextureInfo(specular.texture);
    }
    if (specular.color != Math::Vec3(1.f, 1.f, 1.f)) {
        jsonSpecular["specularColorFactor"] = specular.color.data;
    }
    if (specular.colorTexture.index != GLTF_INVALID_INDEX) {
        jsonSpecular["specularColorTexture"] = ExportTextureInfo(specular.colorTexture);
    }
    return jsonSpecular;
}
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
json::value ExportTransmission(const Material::Transmission& transmission)
{
    json::value jsonTransmission = json::value::object {};
    if (transmission.factor != 0.f) {
        jsonTransmission["transmissionFactor"] = transmission.factor;
    }
    if (transmission.texture.index != GLTF_INVALID_INDEX) {
        jsonTransmission["transmissionTexture"] = ExportTextureInfo(transmission.texture);
    }
    return jsonTransmission;
}
#endif

json::value ExportMaterialExtensions(
    const Material& material, json::value& jsonExtensionsUsed, json::value& jsonExtensionsRequired)
{
    json::value jsonExtensions = json::value::object {};
    if (material.type == Material::Type::SpecularGlossiness) {
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
        jsonExtensions["KHR_materials_pbrSpecularGlossiness"] = ExportSpecularGlossiness(material);
        AppendUnique(jsonExtensionsUsed, "KHR_materials_pbrSpecularGlossiness");
#endif
    } else if (material.type == Material::Type::Unlit) {
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_UNLIT)
        jsonExtensions["KHR_materials_unlit"] = json::value::object {};
        AppendUnique(jsonExtensionsUsed, "KHR_materials_unlit");
#endif
    } else if (material.type == Material::Type::TextureSheetAnimation) {
    }
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT)
    if (auto clearcoat = ExportClearcoat(material.clearcoat); !clearcoat.empty()) {
        jsonExtensions["KHR_materials_clearcoat"] = move(clearcoat);
        AppendUnique(jsonExtensionsUsed, "KHR_materials_clearcoat");
    }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_EMISSIVE_STRENGTH)
    if (auto emissiveStrength = ExportEmissiveStrength(material.emissiveFactor.w); !emissiveStrength.empty()) {
        jsonExtensions["KHR_materials_emissive_strength"] = move(emissiveStrength);
        AppendUnique(jsonExtensionsUsed, "KHR_materials_emissive_strength");
    }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
    if (auto ior = ExportIor(material.ior); !ior.empty()) {
        jsonExtensions["KHR_materials_ior"] = move(ior);
        AppendUnique(jsonExtensionsUsed, "KHR_materials_ior");
    }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
    if (auto sheen = ExportSheen(material.sheen); !sheen.empty()) {
        jsonExtensions["KHR_materials_sheen"] = move(sheen);
        AppendUnique(jsonExtensionsUsed, "KHR_materials_sheen");
    }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
    if (auto specular = ExportSpecular(material.specular); !specular.empty()) {
        jsonExtensions["KHR_materials_specular"] = move(specular);
        AppendUnique(jsonExtensionsUsed, "KHR_materials_specular");
    }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
    if (auto transmission = ExportTransmission(material.transmission); !transmission.empty()) {
        jsonExtensions["KHR_materials_transmission"] = move(transmission);
        AppendUnique(jsonExtensionsUsed, "KHR_materials_transmission");
    }
#endif
    return jsonExtensions;
}

json::value ExportMaterialExtras(const Material& material)
{
    auto jsonExtras = json::value::object {};
    return jsonExtras;
}

json::value ExportMaterials(Data const& data, json::value& jsonExtensionsUsed, json::value& jsonExtensionsRequired)
{
    json::value jsonMaterials = json::value::array {};
    for (auto const& material : data.materials) {
        json::value jsonMaterial = json::value::object {};
        if (!material->name.empty()) {
            jsonMaterial["name"] = string_view(material->name);
        }
        if (material->type == Material::Type::MetallicRoughness || material->type == Material::Type::Unlit) {
            jsonMaterial["pbrMetallicRoughness"] = ExportMetallicRoughness(*material);
        }
        if (auto jsonExtensions = ExportMaterialExtensions(*material, jsonExtensionsUsed, jsonExtensionsRequired);
            !jsonExtensions.empty()) {
            jsonMaterial["extensions"] = move(jsonExtensions);
        }

        if (material->normalTexture.textureInfo.index != GLTF_INVALID_INDEX) {
            auto& jsonNormalTexture = jsonMaterial["normalTexture"] =
                ExportTextureInfo(material->normalTexture.textureInfo);
            if (material->normalTexture.scale != 1.f) {
                jsonNormalTexture["scale"] = material->normalTexture.scale;
            }
        }
        if (material->occlusionTexture.textureInfo.index != GLTF_INVALID_INDEX) {
            auto& jsonOcclusionTexture = jsonMaterial["occlusionTexture"] =
                ExportTextureInfo(material->occlusionTexture.textureInfo);
            if (material->occlusionTexture.strength < 1.f) {
                jsonOcclusionTexture["strength"] = material->occlusionTexture.strength;
            }
        }
        if (material->emissiveTexture.index != GLTF_INVALID_INDEX) {
            jsonMaterial["emissiveTexture"] = ExportTextureInfo(material->emissiveTexture);
        }
        if (Math::Vec3 emissiveFactor(
                material->emissiveFactor.x, material->emissiveFactor.y, material->emissiveFactor.z);
            emissiveFactor != DEFAULT_EMISSIVE_FACTOR) {
            jsonMaterial["emissiveFactor"] = emissiveFactor.data;
        }
        if (material->alphaMode != AlphaMode::OPAQUE) {
            jsonMaterial["alphaMode"] = GetAlphaMode(material->alphaMode);
        }
        if (material->alphaCutoff != 0.5f) {
            jsonMaterial["alphaCutoff"] = material->alphaCutoff;
        }
        if (material->doubleSided) {
            jsonMaterial["doubleSided"] = material->doubleSided;
        }
        if (auto jsonExtras = ExportMaterialExtras(*material); !jsonExtras.empty()) {
            jsonMaterial["extras"] = move(jsonExtras);
        }
        jsonMaterials.array_.push_back(move(jsonMaterial));
    }
    return jsonMaterials;
}

json::value ExportMeshPrimitive(
    const MeshPrimitive& primitive, const vector<unique_ptr<Accessor>>& accessors, json::value& jsonTargetNames)
{
    json::value jsonPrimitive = json::value::object {};
    {
        json::value jsonAttributes = json::value::object {};
        for (auto const& attribute : primitive.attributes) {
            auto type = GetAttributeType(attribute.attribute);
            jsonAttributes[type] = FindObjectIndex(accessors, *attribute.accessor);
        }
        jsonPrimitive["attributes"] = move(jsonAttributes);
    }
    if (primitive.indices) {
        jsonPrimitive["indices"] = FindObjectIndex(accessors, *primitive.indices);
    }
    if (primitive.materialIndex != GLTF_INVALID_INDEX) {
        jsonPrimitive["material"] = primitive.materialIndex;
    }
    if (primitive.mode != RenderMode::TRIANGLES) {
        jsonPrimitive["mode"] = static_cast<int>(primitive.mode);
    }
    if (!primitive.targets.empty()) {
        json::value jsonTargets = json::value::array {};
        for (auto const& target : primitive.targets) {
            json::value jsonTarget = json::value::object {};
            for (auto const& attribute : target.target) {
                auto type = GetAttributeType(attribute.attribute);
                jsonTarget[type] = FindObjectIndex(accessors, *attribute.accessor);
            }
            jsonTargets.array_.push_back(move(jsonTarget));
            if (!target.name.empty()) {
                jsonTargetNames.array_.push_back(string_view(target.name));
            }
        }
        jsonPrimitive["targets"] = move(jsonTargets);
    }
    return jsonPrimitive;
}

json::value ExportMeshes(Data const& data)
{
    json::value jsonMeshes = json::value::array {};
    for (auto const& mesh : data.meshes) {
        json::value jsonMesh = json::value::object {};
        json::value jsonExtras = json::value::object {};
        {
            json::value jsonPrimitives = json::value::array {};
            json::value jsonTargetNames = json::value::array {};
            for (auto const& primitive : mesh->primitives) {
                jsonPrimitives.array_.push_back(ExportMeshPrimitive(primitive, data.accessors, jsonTargetNames));
            }
            jsonMesh["primitives"] = move(jsonPrimitives);
            if (!jsonTargetNames.empty()) {
                jsonExtras["targetNames"] = move(jsonTargetNames);
            }
        }
        if (!mesh->weights.empty()) {
            jsonMesh["weights"] = mesh->weights;
        }
        if (!mesh->name.empty()) {
            jsonMesh["name"] = string_view(mesh->name);
        }
        if (!jsonExtras.empty()) {
            jsonMesh["extras"] = move(jsonExtras);
        }
        jsonMeshes.array_.push_back(move(jsonMesh));
    }
    return jsonMeshes;
}

json::value ExportNodeExtensions(
    Data const& data, const Node& node, json::value& jsonExtensionsUsed, json::value& jsonExtensionsRequired)
{
    json::value jsonExtensions = json::value::object {};
#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    if (node.light) {
        json::value jsonKHRLights = json::value::object {};
        jsonKHRLights["light"] = FindObjectIndex(data.lights, *node.light);
        jsonExtensions["KHR_lights_punctual"] = move(jsonKHRLights);
        AppendUnique(jsonExtensionsUsed, "KHR_lights_punctual");
    }
#endif
    return jsonExtensions;
}

json::value ExportNodes(Data const& data, json::value& jsonExtensionsUsed, json::value& jsonExtensionsRequired)
{
    json::value jsonNodes = json::value::array {};

    for (auto const& node : data.nodes) {
        json::value jsonNodeObject = json::value::object {};

        if (node->camera) {
            jsonNodeObject["camera"] = FindObjectIndex(data.cameras, *node->camera);
        }

        if (!node->tmpChildren.empty()) {
            jsonNodeObject["children"] = node->tmpChildren;
        }

        if (!node->name.empty()) {
            jsonNodeObject["name"] = string_view(node->name);
        }

        if (node->skin) {
            jsonNodeObject["skin"] = FindObjectIndex(data.skins, *node->skin);
        }

        if (node->mesh) {
            jsonNodeObject["mesh"] = FindObjectIndex(data.meshes, *node->mesh);
        }

        if (node->usesTRS) {
            if (node->translation != DEFAULT_TRANSLATION) {
                jsonNodeObject["translation"] = node->translation.data;
            }
            if (node->rotation != DEFAULT_ROTATION) {
                jsonNodeObject["rotation"] = node->rotation.data;
            }
            if (node->scale != DEFAULT_SCALE) {
                jsonNodeObject["scale"] = node->scale.data;
            }
        } else {
            if (node->matrix != IDENTITY_MATRIX) {
                jsonNodeObject["matrix"] = node->matrix.data;
            }
        }

        if (!node->weights.empty()) {
            jsonNodeObject["weights"] = node->weights;
        }

        if (auto jsonExtensions = ExportNodeExtensions(data, *node, jsonExtensionsUsed, jsonExtensionsRequired);
            !jsonExtensions.empty()) {
            jsonNodeObject["extensions"] = move(jsonExtensions);
        }

        jsonNodes.array_.push_back(move(jsonNodeObject));
    }

    return jsonNodes;
}

json::value ExportSamplers(Data const& data)
{
    json::value jsonSamplers = json::value::array {};
    for (auto const& sampler : data.samplers) {
        json::value jsonSampler = json::value::object {};
        if (sampler->magFilter != FilterMode::LINEAR) {
            jsonSampler["magFilter"] = static_cast<int>(sampler->magFilter);
        }
        if (sampler->minFilter != FilterMode::LINEAR) {
            jsonSampler["minFilter"] = static_cast<int>(sampler->minFilter);
        }
        if (sampler->wrapS != WrapMode::REPEAT) {
            jsonSampler["wrapS"] = static_cast<int>(sampler->wrapS);
        }
        if (sampler->wrapT != WrapMode::REPEAT) {
            jsonSampler["wrapT"] = static_cast<int>(sampler->wrapT);
        }
#ifdef EXPORT_OTHER_OBJECT_NAMES
        if (!sampler->name.empty()) {
            jsonSampler["name"] = sampler->name;
        }
#endif
        jsonSamplers.array_.push_back(move(jsonSampler));
    }
    return jsonSamplers;
}

json::value ExportScenes(Data const& data)
{
    json::value jsonScenes = json::value::array {};
    for (auto const& scene : data.scenes) {
        json::value jsonScene = json::value::object {};

        if (!scene->name.empty()) {
            jsonScene["name"] = string_view(scene->name);
        }

        json::value jsonNodes = json::value::array {};
        for (auto const node : scene->nodes) {
            jsonNodes.array_.push_back(FindObjectIndex(data.nodes, *node));
        }
        jsonScene["nodes"] = move(jsonNodes);

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
        if (scene->light) {
            json::value jsonExtensions = json::value::object {};

            json::value jsonKHRLights = json::value::object {};
            jsonKHRLights["light"] = FindObjectIndex(data.lights, *scene->light);
            jsonExtensions["KHR_lights_punctual"] = move(jsonKHRLights);

            jsonScene["extensions"] = move(jsonExtensions);
        }
#endif
        jsonScenes.array_.push_back(move(jsonScene));
    }
    return jsonScenes;
}

json::value ExportSkins(Data const& data)
{
    json::value jsonSkins = json::value::array {};
    for (auto const& skin : data.skins) {
        json::value jsonSkin = json::value::object {};
        if (skin->inverseBindMatrices) {
            jsonSkin["inverseBindMatrices"] = FindObjectIndex(data.accessors, *skin->inverseBindMatrices);
        }
        if (skin->skeleton) {
            jsonSkin["skeleton"] = FindObjectIndex(data.nodes, *skin->skeleton);
        }
        json::value jsonJoints = json::value::array {};
        for (auto const joint : skin->joints) {
            jsonJoints.array_.push_back(FindObjectIndex(data.nodes, *joint));
        }
        jsonSkin["joints"] = move(jsonJoints);
        if (!skin->name.empty()) {
            jsonSkin["name"] = skin->name.empty();
        }
        jsonSkins.array_.push_back(move(jsonSkin));
    }
    return jsonSkins;
}

json::value ExportTextures(Data const& data, json::value& jsonExtensionsUsed, json::value& jsonExtensionsRequired)
{
    json::value jsonTextures = json::value::array {};
    for (auto const& texture : data.textures) {
        json::value jsonTexture = json::value::object {};
        if (texture->sampler) {
            jsonTexture["sampler"] = FindObjectIndex(data.samplers, *texture->sampler);
        }
        if (texture->image) {
            switch (texture->image->type) {
                default:
                case MimeType::INVALID:
                case MimeType::JPEG:
                case MimeType::PNG:
                case MimeType::KTX: // NOTE: this is incorrect, but there's no extension for .ktx
                    jsonTexture["source"] = FindObjectIndex(data.images, *texture->image);
                    break;
                case MimeType::DDS: {
                    json::value jsonMsftTextureDds = json::value::object {};
                    jsonMsftTextureDds["source"] = FindObjectIndex(data.images, *texture->image);

                    json::value jsonExtensions = json::value::object {};
                    jsonExtensions["MSFT_texture_dds"] = move(jsonMsftTextureDds);

                    jsonTexture["extensions"] = move(jsonExtensions);

                    AppendUnique(jsonExtensionsUsed, "MSFT_texture_dds");
                    AppendUnique(jsonExtensionsRequired, "MSFT_texture_dds");
                    break;
                }
                case MimeType::KTX2: {
                    json::value jsonKHRtextureBasisU = json::value::object {};
                    jsonKHRtextureBasisU["source"] = FindObjectIndex(data.images, *texture->image);

                    json::value jsonExtensions = json::value::object {};
                    jsonExtensions["KHR_texture_basisu"] = move(jsonKHRtextureBasisU);

                    jsonTexture["extensions"] = move(jsonExtensions);

                    AppendUnique(jsonExtensionsUsed, "KHR_texture_basisu");
                    AppendUnique(jsonExtensionsRequired, "KHR_texture_basisu");
                    break;
                }
            }
        }
#ifdef EXPORT_OTHER_OBJECT_NAMES
        if (!texture->name.empty()) {
            jsonTexture["name"] = texture->name;
        }
#endif
        jsonTextures.array_.push_back(move(jsonTexture));
    }
    return jsonTextures;
}

json::value ExportKHRLights(Data const& data)
{
    json::value jsonLightArray = json::value::array {};
#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)

    for (auto const& light : data.lights) {
        if (light->type == LightType::AMBIENT || light->type == LightType::INVALID) {
            continue;
        }

        json::value jsonLightObject = json::value::object {};
        if (!light->name.empty()) {
            jsonLightObject["name"] = string_view(light->name);
        }
        if (light->color != Math::Vec3(1.f, 1.f, 1.f)) {
            jsonLightObject["color"] = light->color.data;
        }
        if (light->intensity != 1.f) {
            jsonLightObject["intensity"] = light->intensity;
        }
        jsonLightObject["type"] = GetLightType(light->type);
        if ((light->type == LightType::POINT || light->type == LightType::SPOT) && light->positional.range > 0.f) {
            jsonLightObject["range"] = light->positional.range;
        }
        if (light->type == LightType::SPOT) {
            json::value jsonSpotObject = json::value::object {};
            if (light->positional.spot.innerAngle != 0.f) {
                jsonSpotObject["innerConeAngle"] = light->positional.spot.innerAngle;
            }
            if (light->positional.spot.outerAngle != 0.785398163397448f) {
                jsonSpotObject["outerConeAngle"] = light->positional.spot.outerAngle;
            }
            if (!jsonSpotObject.empty()) {
                jsonLightObject["spot"] = move(jsonSpotObject);
            }
        }
        jsonLightArray.array_.push_back(move(jsonLightObject));
    }
#endif
    json::value jsonLights = json::value::object {};
    jsonLights["lights"] = move(jsonLightArray);
    return jsonLights;
}

json::value ExportExtensions(Data const& data, json::value& jsonExtensionsUsed, json::value& jsonExtensionsRequired)
{
    json::value jsonExtensions = json::value::object {};

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    if (!data.lights.empty()) {
        if (auto jsonKHRLights = ExportKHRLights(data); !jsonKHRLights.empty()) {
            jsonExtensions["KHR_lights_punctual"] = move(jsonKHRLights);
            AppendUnique(jsonExtensionsUsed, "KHR_lights_punctual");
        }
    }
#endif
    return jsonExtensions;
}

json::value ExportAsset(string_view versionString, vector<string>& strings)
{
    auto jsonAsset = json::value { json::value::object {} };
    jsonAsset["version"] = string_view("2.0");
    strings.push_back("CoreEngine " + versionString);
    jsonAsset["generator"] = string_view(strings.back());
    return jsonAsset;
}

/* Returns a JSON string generated from given GLTF2::Data. The above Export* helpers are used to convert different
 * parts of the data into JSON objects. */
auto ExportGLTFData(Data const& data, string_view versionString)
{
    vector<string> strings;
    auto jsonGltf = json::value { json::value::object {} };

    auto jsonExtensionsUsed = json::value { json::value::array {} };
    auto jsonExtensionsRequired = json::value { json::value::array {} };
    jsonGltf["asset"] = ExportAsset(versionString, strings);

    if (!data.animations.empty()) {
        jsonGltf["animations"] = ExportAnimations(data);
    }

    if (!data.cameras.empty()) {
        jsonGltf["cameras"] = ExportCameras(data);
    }

    if (!data.images.empty()) {
        jsonGltf["images"] = ExportImages(data);
    }

    if (!data.materials.empty()) {
        jsonGltf["materials"] = ExportMaterials(data, jsonExtensionsUsed, jsonExtensionsRequired);
    }

    if (!data.meshes.empty()) {
        jsonGltf["meshes"] = ExportMeshes(data);
    }

    if (!data.nodes.empty()) {
        jsonGltf["nodes"] = ExportNodes(data, jsonExtensionsUsed, jsonExtensionsRequired);
    }

    if (!data.samplers.empty()) {
        jsonGltf["samplers"] = ExportSamplers(data);
    }

    if (!data.scenes.empty()) {
        jsonGltf["scenes"] = ExportScenes(data);
        jsonGltf["scene"] = 0;
    }

    if (!data.skins.empty()) {
        jsonGltf["skins"] = ExportSkins(data);
    }

    if (!data.textures.empty()) {
        jsonGltf["textures"] = ExportTextures(data, jsonExtensionsUsed, jsonExtensionsRequired);
    }

    if (auto jsonExtensions = ExportExtensions(data, jsonExtensionsUsed, jsonExtensionsRequired);
        !jsonExtensions.empty()) {
        jsonGltf["extensions"] = move(jsonExtensions);
    }

    if (!data.accessors.empty()) {
        jsonGltf["accessors"] = ExportAccessors(data);
    }

    if (!data.bufferViews.empty()) {
        jsonGltf["bufferViews"] = ExportBufferViews(data);
    }

    if (!data.buffers.empty()) {
        jsonGltf["buffers"] = ExportBuffers(data, strings);
    }

    if (!jsonExtensionsUsed.empty()) {
        jsonGltf["extensionsUsed"] = move(jsonExtensionsUsed);
    }
    if (!jsonExtensionsRequired.empty()) {
        jsonGltf["extensionsRequired"] = move(jsonExtensionsRequired);
    }

    return to_string(jsonGltf);
}
} // namespace

/* Writes the GLTF2::Data as a GLB file. */
void SaveGLB(Data const& data, IFile& file, string_view versionString)
{
    auto jsonString = ExportGLTFData(data, versionString);
    if (jsonString.empty()) {
        return;
    }
    if (auto const pad = (jsonString.size() % 4); pad) {
        jsonString.append(4 - pad, ' ');
    }

    auto const jsonSize = static_cast<uint32_t>(jsonString.size());
    auto const binarySize = [](auto const& aBuffers) {
        size_t totalSize = 0;
        for (auto const& buffer : aBuffers) {
            totalSize += buffer->data.size();
        }
        return static_cast<uint32_t>(totalSize);
    }(data.buffers);

    auto const header = GLBHeader { GLTF_MAGIC, 2,
        static_cast<uint32_t>(sizeof(GLBHeader) + sizeof(GLBChunk) + jsonSize + sizeof(GLBChunk) + binarySize) };
    file.Write(&header, sizeof(header));

    auto const jsonChunk = GLBChunk { jsonSize, static_cast<uint32_t>(ChunkType::JSON) };
    file.Write(&jsonChunk, sizeof(jsonChunk));

    file.Write(jsonString.data(), jsonSize);

    auto const binaryChunk = GLBChunk { binarySize, static_cast<uint32_t>(ChunkType::BIN) };
    file.Write(&binaryChunk, sizeof(binaryChunk));

    file.Write(data.buffers.front()->data.data(), binarySize);
}

/* Writes the GLTF2::Data as a glTF file. */
void SaveGLTF(Data const& data, IFile& file, string_view versionString)
{
    auto const jsonString = ExportGLTFData(data, versionString);
    file.Write(jsonString.data(), jsonString.size());
}

/* Returns true if the scene node has a node component and it hasn't been excluded from export. */
bool IsExportable(ISceneNode const& node, IEcs const& ecs)
{
    auto const nodeEntity = node.GetEntity();
    if (auto const nodeManager = GetManager<INodeComponentManager>(ecs); nodeManager) {
        return nodeManager->HasComponent(nodeEntity) && nodeManager->Get(nodeEntity).exported;
    }
    return false;
}

namespace {
/* Returns the first parent which can be exported or null if no such parent exists. */
ISceneNode* FindExportedParent(ISceneNode const& node, IEcs const& ecs)
{
    auto parent = node.GetParent();
    while (parent) {
        if (IsExportable(*parent, ecs)) {
            return parent;
        }
        parent = parent->GetParent();
    }
    return parent;
}

/* Gathers transformations of nodes which haven't been exported and applies them to the exported child. */
void CombineSkippedParentTransformations(
    TransformComponent& transformComponent, ISceneNode const& node, IEcs const& ecs, vector<Entity> const& nodeEntities)
{
    auto parent = node.GetParent();
    auto const transformManager = GetManager<ITransformComponentManager>(ecs);
    while (parent) {
        if (auto const parentIndex = FindHandleIndex(nodeEntities, parent->GetEntity());
            parentIndex < nodeEntities.size()) {
            // found an exported node and no need to continue.
            parent = nullptr;
        } else {
            // apply the transformation of a node which wasn't exported.
            if (transformManager->HasComponent(parent->GetEntity())) {
                auto const parentTransformComponent = transformManager->Get(parent->GetEntity());

                auto const transformation =
                    Math::Trs(parentTransformComponent.position, parentTransformComponent.rotation,
                        parentTransformComponent.scale) *
                    Math::Trs(transformComponent.position, transformComponent.rotation, transformComponent.scale);

                Math::Vec3 skew;
                Math::Vec4 perspective;
                Math::Decompose(transformation, transformComponent.scale, transformComponent.rotation,
                    transformComponent.position, skew, perspective);
            }
            parent = parent->GetParent();
        }
    }
}

Node& GetNode(vector<unique_ptr<Node>>& nodeArray, size_t index)
{
    if (index < nodeArray.size()) {
        return *nodeArray[index];
    } else {
        return *nodeArray.emplace_back(make_unique<Node>());
    }
}

void AttachMesh(IEcs const& ecs, const Entity nodeEntity, Node& exportNode, decltype(Data::meshes)& meshArray,
    vector<Entity>& usedMeshes)
{
    if (auto const meshManager = GetManager<IRenderMeshComponentManager>(ecs);
        meshManager && meshManager->HasComponent(nodeEntity)) {
        auto const meshHandle = meshManager->Get(nodeEntity).mesh;
        if (auto const meshIndex = FindOrAddIndex(usedMeshes, meshHandle); meshIndex < meshArray.size()) {
            exportNode.mesh = meshArray[meshIndex].get();
        } else {
            exportNode.mesh = meshArray.emplace_back(make_unique<Mesh>()).get();
        }
    }
}

void AttachCamera(IEcs const& ecs, const Entity nodeEntity, Node& exportNode, decltype(Data::cameras)& cameraArray,
    Entities& entities)
{
    if (auto const cameraManager = GetManager<ICameraComponentManager>(ecs);
        cameraManager && cameraManager->HasComponent(nodeEntity)) {
        if (auto const cameraIndex = FindOrAddIndex(entities.withCamera, nodeEntity);
            cameraIndex < cameraArray.size()) {
            exportNode.camera = cameraArray[cameraIndex].get();
        } else {
            exportNode.camera = cameraArray.emplace_back(make_unique<Camera>()).get();
        }
    }
}

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
void AttachLight(
    IEcs const& ecs, const Entity nodeEntity, Node& exportNode, decltype(Data::lights)& lightArray, Entities& entities)
{
    if (auto const lightManager = GetManager<ILightComponentManager>(ecs);
        lightManager && lightManager->HasComponent(nodeEntity)) {
        if (auto const lightIndex = FindOrAddIndex(entities.withLight, nodeEntity); lightIndex < lightArray.size()) {
            exportNode.light = lightArray[lightIndex].get();
        } else {
            exportNode.light = lightArray.emplace_back(make_unique<KHRLight>()).get();
        }
    }
}
#endif

void AttachParent(const ISceneNode& node, const IEcs& ecs, Scene& scene, Node& exportNode, uint32_t nodeIndex,
    const vector<Entity>& nodeEntities, decltype(Data::nodes)& nodeArray)
{
    if (const auto* parent = FindExportedParent(node, ecs); parent) {
        if (auto const parentIndex = FindHandleIndex(nodeEntities, parent->GetEntity());
            parentIndex < nodeArray.size()) {
            // Parent has been exported -> node has a parent and will be added to the parents list of children.
            exportNode.parent = nodeArray[parentIndex].get();
            if (std::none_of(exportNode.parent->children.begin(), exportNode.parent->children.end(),
                [&exportNode](const auto childNode) { return childNode == &exportNode; })) {
                exportNode.parent->children.push_back(&exportNode);
                exportNode.parent->tmpChildren.push_back(nodeIndex);
            }
        } else {
            // Parent hasn't been exported i.e. it's outside this scene hierarchy -> add node as a scene root.
            scene.nodes.push_back(&exportNode);
        }
    } else {
        // Parent marked to be excluded from exporting -> add node as a scene root.
        scene.nodes.push_back(&exportNode);
    }
}

void AttachSkin(
    IEcs const& ecs, const Entity nodeEntity, Node& exportNode, decltype(Data::skins)& skinArray, Entities& entities)
{
    if (auto const skinManager = GetManager<ISkinComponentManager>(ecs);
        skinManager && skinManager->HasComponent(nodeEntity)) {
        if (auto const entityIndex = FindOrAddIndex(entities.withSkin, nodeEntity); entityIndex < skinArray.size()) {
            exportNode.skin = skinArray[entityIndex].get();
        } else {
            exportNode.skin = skinArray.emplace_back(make_unique<Skin>()).get();
        }
    }
}

/* Export scene node hierarcy as a glTF scene. Only nodes and indices to other resources are written to GLTF2::Data.
 * Mesh, Image etc. have to be written to GLTF2::Data separately based on the usedMeshes, and entities output
 * parameters.
 * @param node Scene node to consider to be exported.
 * @param ecs ECS instance where node and related data lives.
 * @param scene Scene where nodes will be included.
 * @param data Exported nodes and placeholders for meshes, cameras, lights etc. will be stored here.
 * @param nodeEntities Entities with NodeComponents which were exported.
 * @param usedMeshes Handles to meshes used by exported entities.
 * @param entities Collections of entities having special components attached such as cameras or lights.
 */
void RecursivelyExportNode(ISceneNode const& node, IEcs const& ecs, Scene& scene, Data& data,
    vector<Entity>& nodeEntities, vector<Entity>& usedMeshes, Entities& entities)
{
    if (!IsExportable(node, ecs)) {
        // if this node shouldn't be exported, try exporting the child nodes.
        for (const auto* child : node.GetChildren()) {
            RecursivelyExportNode(*child, ecs, scene, data, nodeEntities, usedMeshes, entities);
        }
        return;
    }

    auto const nodeEntity = node.GetEntity();
    auto const nodeIndex = FindOrAddIndex(nodeEntities, nodeEntity);
    auto& exportNode = GetNode(data.nodes, nodeIndex);

    // name
    exportNode.name = node.GetName();

    // mesh
    AttachMesh(ecs, nodeEntity, exportNode, data.meshes, usedMeshes);

    // camera
    AttachCamera(ecs, nodeEntity, exportNode, data.cameras, entities);

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    // light
    AttachLight(ecs, nodeEntity, exportNode, data.lights, entities);
#endif

    // parent
    AttachParent(node, ecs, scene, exportNode, nodeIndex, nodeEntities, data.nodes);

    // isJoint
    // children, tmpChildren, the child will actually add itself to the parents list of children
    for (const auto* child : node.GetChildren()) {
        RecursivelyExportNode(*child, ecs, scene, data, nodeEntities, usedMeshes, entities);
    }

    // skin
    // tmpSkin
    AttachSkin(ecs, nodeEntity, exportNode, data.skins, entities);

    // usesTRS, translation, rotation, scale
    if (const auto* transformManager = GetManager<ITransformComponentManager>(ecs);
        transformManager && transformManager->HasComponent(nodeEntity)) {
        exportNode.usesTRS = true;

        auto transformComponent = transformManager->Get(nodeEntity);
        CombineSkippedParentTransformations(transformComponent, node, ecs, nodeEntities);
        transformComponent.rotation = Math::Normalize(transformComponent.rotation);
        exportNode.translation = transformComponent.position;
        exportNode.rotation = transformComponent.rotation;
        exportNode.scale = transformComponent.scale;
    }

    // NOTE: weights, defaults are not exported
}
} // namespace

// Internal exporting function.
ExportResult ExportGLTF(IEngine& engine, const IEcs& ecs)
{
    auto result = ExportResult(make_unique<Data>(engine.GetFileManager()));

    // We write all the binary data to a single buffer.
    auto& exportBuffer = result.data->buffers.emplace_back(make_unique<Buffer>());

    // Helper for gathering bufferViews and accessors, and packing data in the buffer.
    BufferHelper bufferHelper(*exportBuffer, result.data->bufferViews, result.data->accessors);

    Entities entities;
    vector<Entity> usedMeshes;

    // Create Nodes and Scenes.
    auto const nameManager = GetManager<INameComponentManager>(ecs);
    auto const nodeManager = GetManager<INodeComponentManager>(ecs);
    auto const nodeSystem = GetSystem<INodeSystem>(ecs);
    if (nodeManager && nodeSystem) {
        auto& sceneArray = result.data->scenes;

        auto const nodeCount = nodeManager->GetComponentCount();
        auto& nodeArray = result.data->nodes;
        nodeArray.reserve(nodeCount);
        entities.nodes.reserve(nodeCount);

        auto& exportScene = *sceneArray.emplace_back(make_unique<Scene>());

        for (const auto* child : nodeSystem->GetRootNode().GetChildren()) {
            RecursivelyExportNode(*child, ecs, exportScene, *result.data, entities.nodes, usedMeshes, entities);
        }
        if (exportScene.nodes.empty()) {
            sceneArray.pop_back();
        }

        // Create Skins.
        ExportGltfSkins(ecs, entities, nodeArray, result, bufferHelper);

        // Create Cameras.
        ExportGltfCameras(ecs, entities, result);

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
        // Create KHRLights.
        ExportGltfLight(ecs, entities, result);
#endif
        // Create Animations.
        ExportGltfAnimations(ecs, entities, result, bufferHelper);
    }
    unordered_map<string, IGLTFData::Ptr> originalGltfs;
    // Create Meshes for the mesh handles referenced by exported nodes. Materials referenced by the meshes will
    // be gathered for exporting.
    auto meshManager = GetManager<IMeshComponentManager>(ecs);
    auto materialManager = GetManager<IMaterialComponentManager>(ecs);
    auto uriManager = GetManager<IUriComponentManager>(ecs);
    auto usedMaterials = ExportGltfMeshes(*meshManager, *nameManager, *uriManager, *materialManager,
        engine.GetFileManager(), usedMeshes, result, bufferHelper, originalGltfs);

    ExportGltfMaterials(engine, *materialManager, *nameManager, *uriManager, usedMaterials, result, bufferHelper);

    // Write image data to the buffer
    ExportImageData(engine.GetFileManager(), result, bufferHelper, originalGltfs);

    exportBuffer->byteLength = exportBuffer->data.size();

    return result;
}
} // namespace GLTF2

CORE3D_END_NAMESPACE()
