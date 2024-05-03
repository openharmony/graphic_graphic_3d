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

#include "gltf/gltf2_importer.h"

#include <chrono>
#include <cstring>
#include <functional>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/rsdz_model_id_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_skinning_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_class_register.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "gltf/gltf2_util.h"
#include "util/mesh_util.h"
#include "util/string_util.h"
#include "util/uri_lookup.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
// How many threads the GLTF2Importer will use to run tasks.
constexpr const uint32_t IMPORTER_THREADS = 2u;

template<class T>
size_t FindIndex(const vector<unique_ptr<T>>& container, T const* item)
{
    for (size_t i = 0; i < container.size(); ++i) {
        if (container[i].get() == item) {
            return i;
        }
    }
    return GLTF2::GLTF_INVALID_INDEX;
}

Format Convert(GLTF2::ComponentType componentType, size_t componentCount, bool normalized)
{
    switch (componentType) {
        case GLTF2::ComponentType::INVALID:
            break;

        case GLTF2::ComponentType::BYTE:
            if (normalized) {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R8_SNORM;
                    case 2:
                        return BASE_FORMAT_R8G8_SNORM;
                    case 3:
                        return BASE_FORMAT_R8G8B8_SNORM;
                    case 4:
                        return BASE_FORMAT_R8G8B8A8_SNORM;
                }
            } else {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R8_SINT;
                    case 2:
                        return BASE_FORMAT_R8G8_SINT;
                    case 3:
                        return BASE_FORMAT_R8G8B8_SINT;
                    case 4:
                        return BASE_FORMAT_R8G8B8A8_SINT;
                }
            }
            break;

        case GLTF2::ComponentType::UNSIGNED_BYTE:
            if (normalized) {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R8_UNORM;
                    case 2:
                        return BASE_FORMAT_R8G8_UNORM;
                    case 3:
                        return BASE_FORMAT_R8G8B8_UNORM;
                    case 4:
                        return BASE_FORMAT_R8G8B8A8_UNORM;
                }
            } else {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R8_UINT;
                    case 2:
                        return BASE_FORMAT_R8G8_UINT;
                    case 3:
                        return BASE_FORMAT_R8G8B8_UINT;
                    case 4:
                        return BASE_FORMAT_R8G8B8A8_UINT;
                }
            }
            break;

        case GLTF2::ComponentType::SHORT:
            if (normalized) {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R16_SNORM;
                    case 2:
                        return BASE_FORMAT_R16G16_SNORM;
                    case 3:
                        return BASE_FORMAT_R16G16B16_SNORM;
                    case 4:
                        return BASE_FORMAT_R16G16B16A16_SNORM;
                }
            } else {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R16_SINT;
                    case 2:
                        return BASE_FORMAT_R16G16_SINT;
                    case 3:
                        return BASE_FORMAT_R16G16B16_SINT;
                    case 4:
                        return BASE_FORMAT_R16G16B16A16_SINT;
                }
            }
            break;

        case GLTF2::ComponentType::UNSIGNED_SHORT:
            if (normalized) {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R16_UNORM;
                    case 2:
                        return BASE_FORMAT_R16G16_UNORM;
                    case 3:
                        return BASE_FORMAT_R16G16B16_UNORM;
                    case 4:
                        return BASE_FORMAT_R16G16B16A16_UNORM;
                }
            } else {
                switch (componentCount) {
                    case 1:
                        return BASE_FORMAT_R16_UINT;
                    case 2:
                        return BASE_FORMAT_R16G16_UINT;
                    case 3:
                        return BASE_FORMAT_R16G16B16_UINT;
                    case 4:
                        return BASE_FORMAT_R16G16B16A16_UINT;
                }
            }
            break;

        case GLTF2::ComponentType::INT:
            switch (componentCount) {
                case 1:
                    return BASE_FORMAT_R32_SINT;
                case 2:
                    return BASE_FORMAT_R32G32_SINT;
                case 3:
                    return BASE_FORMAT_R32G32B32_SINT;
                case 4:
                    return BASE_FORMAT_R32G32B32A32_SINT;
            }
            break;

        case GLTF2::ComponentType::UNSIGNED_INT:
            switch (componentCount) {
                case 1:
                    return BASE_FORMAT_R32_UINT;
                case 2: // 2 : type
                    return BASE_FORMAT_R32G32_UINT;
                case 3: // 3 :type
                    return BASE_FORMAT_R32G32B32_UINT;
                case 4: // 4 ： type
                    return BASE_FORMAT_R32G32B32A32_UINT;
            }
            break;

        case GLTF2::ComponentType::FLOAT:
            switch (componentCount) {
                case 1:
                    return BASE_FORMAT_R32_SFLOAT;
                case 2: // 2 ： type
                    return BASE_FORMAT_R32G32_SFLOAT;
                case 3: // 3 ： type
                    return BASE_FORMAT_R32G32B32_SFLOAT;
                case 4: // 4 : type
                    return BASE_FORMAT_R32G32B32A32_SFLOAT;
            }
            break;
        default :
            break;
    }
    return BASE_FORMAT_UNDEFINED;
}

void ConvertNormalizedDataToFloat(GLTF2::GLTFLoadDataResult const& result, float* destination,
    size_t dstComponentCount = 0, float paddingValue = 0.0f, float scale = 0.f)
{
    uint8_t const* source = reinterpret_cast<const uint8_t*>(result.data.data());

    if (dstComponentCount == 0) {
        // By default, use the source component count.
        dstComponentCount = result.componentCount;
    }

    CORE_ASSERT_MSG(dstComponentCount >= result.componentCount,
        "Padding count cannot be negative. Make sure expected component count is equal or greater than source "
        "component count.");

    // Amount of padding.
    const size_t paddingCount = dstComponentCount - result.componentCount;

    for (size_t i = 0; i < result.elementCount; ++i) {
        for (size_t j = 0; j < result.componentCount; ++j) {
            switch (result.componentType) {
                case GLTF2::ComponentType::BYTE:
                    *destination = std::max((reinterpret_cast<const int8_t*>(source))[j] / 127.0f, -1.0f);
                    break;

                case GLTF2::ComponentType::UNSIGNED_BYTE:
                    *destination = source[j] / 255.0f;
                    break;

                case GLTF2::ComponentType::SHORT:
                    *destination = std::max(
                        (reinterpret_cast<const int16_t*>(source))[j] / ((scale != 0.f) ? scale : 32767.0f), -1.0f);
                    break;

                case GLTF2::ComponentType::UNSIGNED_SHORT:
                    *destination = (reinterpret_cast<const uint16_t*>(source))[j] / 65535.0f;
                    break;

                case GLTF2::ComponentType::FLOAT: {
                    *destination = (reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(source)))[j];
                    break;
                }

                default:
                case GLTF2::ComponentType::UNSIGNED_INT:
                case GLTF2::ComponentType::INT:
                    CORE_ASSERT(false);
                    *destination = 0.0f;
                    break;
            }

            destination++;
        }

        // Apply padding.
        for (size_t padding = 0; padding < paddingCount; ++padding) {
            *destination = paddingValue;
            destination++;
        }

        source += result.elementSize;
    }
}

void ConvertDataToFloat(GLTF2::GLTFLoadDataResult const& result, float* destination, size_t dstComponentCount = 0,
    float paddingValue = 0.0f, float scale = 0.f)
{
    uint8_t const* source = reinterpret_cast<const uint8_t*>(result.data.data());

    if (dstComponentCount == 0) {
        // By default, use the source component count.
        dstComponentCount = result.componentCount;
    }

    CORE_ASSERT_MSG(dstComponentCount >= result.componentCount,
        "Padding count cannot be negative. Make sure expected component count is equal or greater than source "
        "component count.");

    // Amount of padding.
    const size_t paddingCount = dstComponentCount - result.componentCount;

    for (size_t i = 0; i < result.elementCount; ++i) {
        for (size_t j = 0; j < result.componentCount; ++j) {
            switch (result.componentType) {
                case GLTF2::ComponentType::BYTE:
                    *destination = reinterpret_cast<const int8_t*>(source)[j];
                    break;

                case GLTF2::ComponentType::UNSIGNED_BYTE:
                    *destination = source[j];
                    break;

                case GLTF2::ComponentType::SHORT:
                    *destination = reinterpret_cast<const int16_t*>(source)[j];
                    break;

                case GLTF2::ComponentType::UNSIGNED_SHORT:
                    *destination = reinterpret_cast<const uint16_t*>(source)[j];
                    break;

                case GLTF2::ComponentType::FLOAT: {
                    *destination = (reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(source)))[j];
                    break;
                }

                default:
                case GLTF2::ComponentType::UNSIGNED_INT:
                case GLTF2::ComponentType::INT:
                    CORE_ASSERT(false);
                    *destination = 0.0f;
                    break;
            }

            destination++;
        }

        // Apply padding.
        for (size_t padding = 0; padding < paddingCount; ++padding) {
            *destination = paddingValue;
            destination++;
        }

        source += result.elementSize;
    }
}

void ConvertDataToBool(GLTF2::GLTFLoadDataResult const& result, bool* destination)
{
    uint8_t const* source = reinterpret_cast<const uint8_t*>(result.data.data());

    for (size_t i = 0; i < result.elementCount; ++i) {
        for (size_t j = 0; j < result.componentCount; ++j) {
            switch (result.componentType) {
                case GLTF2::ComponentType::BYTE:
                    *destination = reinterpret_cast<const int8_t*>(source) != 0;
                    break;

                case GLTF2::ComponentType::UNSIGNED_BYTE:
                    *destination = source[j] != 0u;
                    break;

                case GLTF2::ComponentType::SHORT:
                    *destination = (reinterpret_cast<const int16_t*>(source))[j] != 0;
                    break;

                case GLTF2::ComponentType::UNSIGNED_SHORT:
                    *destination = (reinterpret_cast<const uint16_t*>(source))[j] != 0u;
                    break;

                case GLTF2::ComponentType::FLOAT: {
                    *destination = (reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(source)))[j] != 0.f;
                    break;
                }

                default:
                case GLTF2::ComponentType::UNSIGNED_INT:
                case GLTF2::ComponentType::INT:
                    CORE_ASSERT(false);
                    *destination = false;
                    break;
            }

            destination++;
        }

        source += result.elementSize;
    }
}

EntityReference GetImportedTextureHandle(const GLTFImportResult& importResult, uint32_t index)
{
    if (index != GLTF2::GLTF_INVALID_INDEX && index < importResult.data.textures.size()) {
        return importResult.data.textures[index];
    }

    return EntityReference();
}

string_view GetImageExtension(GLTF2::MimeType type)
{
    switch (type) {
        case GLTF2::MimeType::JPEG:
            return "jpg";
        case GLTF2::MimeType::PNG:
            return "png";
        case GLTF2::MimeType::KTX:
            return "ktx";
        case GLTF2::MimeType::DDS:
            return "dds";

        case GLTF2::MimeType::INVALID:
        default:
            return "";
    }
}

LightComponent::Type ConvertToCoreLightType(GLTF2::LightType lightType)
{
    switch (lightType) {
        case GLTF2::LightType::DIRECTIONAL:
            return LightComponent::Type::DIRECTIONAL;

        case GLTF2::LightType::POINT:
            return LightComponent::Type::POINT;

        case GLTF2::LightType::SPOT:
            return LightComponent::Type::SPOT;

        default:
        case GLTF2::LightType::INVALID:
        case GLTF2::LightType::AMBIENT:
            return LightComponent::Type::DIRECTIONAL;
    }
}

struct GatherMeshDataResult {
    GatherMeshDataResult() = default;
    ~GatherMeshDataResult() = default;
    GatherMeshDataResult(const GatherMeshDataResult& aOther) = delete;
    explicit GatherMeshDataResult(const string& error) : success(false), error(error) {}
    GatherMeshDataResult(GatherMeshDataResult&& other) noexcept
        : success(other.success), error(move(other.error)), meshBuilder(move(other.meshBuilder))
    {}

    GatherMeshDataResult& operator=(GatherMeshDataResult&& other) noexcept
    {
        success = other.success;
        error = move(other.error);
        meshBuilder = move(other.meshBuilder);
        return *this;
    }

    /** Indicates, whether the load operation is successful. */
    bool success { true };

    /** In case of import error, contains the description of the error. */
    string error;

    IMeshBuilder::Ptr meshBuilder;
};

void ConvertLoadResultToFloat(GLTF2::GLTFLoadDataResult& data, float scale = 0.f)
{
    vector<uint8_t> converted;
    auto const componentCount = data.elementCount * data.componentCount;
    converted.resize(componentCount * sizeof(float));
    if (data.normalized) {
        ConvertNormalizedDataToFloat(data, reinterpret_cast<float*>(converted.data()), 0u, 0.f, scale);
    } else {
        ConvertDataToFloat(data, reinterpret_cast<float*>(converted.data()), 0u, 0.f, scale);
    }

    data.componentType = GLTF2::ComponentType::FLOAT;
    data.componentByteSize = sizeof(float);
    data.elementSize = data.componentByteSize * data.componentCount;
    data.data = move(converted);
}

template<typename T>
void Validate(GLTF2::GLTFLoadDataResult& indices, uint32_t vertexCount)
{
    auto source = array_view((T const*)indices.data.data(), indices.elementCount);
    if (std::any_of(source.begin(), source.end(), [vertexCount](const auto& value) { return value >= vertexCount; })) {
        indices.success = false;
        indices.error += "Indices out-of-range.\n";
    }
}

void ValidateIndices(GLTF2::GLTFLoadDataResult& indices, uint32_t vertexCount)
{
    switch (indices.componentType) {
        case GLTF2::ComponentType::UNSIGNED_BYTE: {
            Validate<uint8_t>(indices, vertexCount);
            break;
        }
        case GLTF2::ComponentType::UNSIGNED_SHORT: {
            Validate<uint16_t>(indices, vertexCount);
            break;
        }
        case GLTF2::ComponentType::UNSIGNED_INT: {
            Validate<uint32_t>(indices, vertexCount);
            break;
        }

        default:
        case GLTF2::ComponentType::BYTE:
        case GLTF2::ComponentType::SHORT:
        case GLTF2::ComponentType::FLOAT:
        case GLTF2::ComponentType::INT:
            indices.success = false;
            indices.error += "Invalid componentType for indices.\n";
            CORE_ASSERT(false);
            break;
    }
}

bool LoadPrimitiveAttributeData(const GLTF2::MeshPrimitive& primitive, GLTF2::GLTFLoadDataResult& positions,
    GLTF2::GLTFLoadDataResult& normals, array_view<GLTF2::GLTFLoadDataResult> texcoords,
    GLTF2::GLTFLoadDataResult& tangents, GLTF2::GLTFLoadDataResult& joints, GLTF2::GLTFLoadDataResult& weights,
    GLTF2::GLTFLoadDataResult& colors, const uint32_t& flags)
{
    bool success = true;
    for (const auto& attribute : primitive.attributes) {
        if ((attribute.attribute.type != GLTF2::AttributeType::TEXCOORD && attribute.attribute.index > 0) ||
            (attribute.attribute.type == GLTF2::AttributeType::TEXCOORD &&
                attribute.attribute.index >= texcoords.size())) {
            continue;
        }

        GLTF2::GLTFLoadDataResult loadDataResult = GLTF2::LoadData(*attribute.accessor);
        success = success && loadDataResult.success;
        switch (attribute.attribute.type) {
            case GLTF2::AttributeType::POSITION:
                positions = move(loadDataResult);
                break;

            case GLTF2::AttributeType::NORMAL:
                normals = move(loadDataResult);
                break;

            case GLTF2::AttributeType::TEXCOORD:
                texcoords[attribute.attribute.index] = move(loadDataResult);
                break;

            case GLTF2::AttributeType::TANGENT:
                tangents = move(loadDataResult);
                break;

            case GLTF2::AttributeType::JOINTS:
                if (flags & CORE_GLTF_IMPORT_RESOURCE_SKIN) {
                    joints = move(loadDataResult);
                }
                break;

            case GLTF2::AttributeType::WEIGHTS:
                if (flags & CORE_GLTF_IMPORT_RESOURCE_SKIN) {
                    weights = move(loadDataResult);
                }
                break;

            case GLTF2::AttributeType::COLOR:
                colors = move(loadDataResult);
                break;

            case GLTF2::AttributeType::INVALID:
            default:
                break;
        }
    }
    return success;
}

void ProcessMorphTargetData(const IMeshBuilder::Submesh& importInfo, size_t targets,
    GLTF2::GLTFLoadDataResult& loadDataResult, GLTF2::GLTFLoadDataResult& finalDataResult)
{
#if !defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
    // Spec says POSITION,NORMAL and TANGENT must be FLOAT & VEC3
    // NOTE: ASSERT for now, if the types don't match, they need to be converted. (or we should fail
    // since out-of-spec)
    CORE_ASSERT(loadDataResult.componentType == GLTF2::ComponentType::FLOAT);
    CORE_ASSERT(loadDataResult.componentCount == 3U);
    CORE_ASSERT(loadDataResult.elementCount == importInfo.vertexCount);
#endif
    if (finalDataResult.componentCount > 0U) {
        finalDataResult.data.insert(finalDataResult.data.end(), loadDataResult.data.begin(), loadDataResult.data.end());
        for (size_t i = 0; i < finalDataResult.min.size(); i++) {
            finalDataResult.min[i] = std::min(finalDataResult.min[i], loadDataResult.min[i]);
        }
        for (size_t i = 0; i < finalDataResult.max.size(); i++) {
            finalDataResult.max[i] = std::max(finalDataResult.max[i], loadDataResult.max[i]);
        }
    } else {
        finalDataResult = move(loadDataResult);
        finalDataResult.data.reserve(finalDataResult.data.size() * targets);
    }
}

void GenerateMorphTargets(const GLTF2::MeshPrimitive& primitive, const IMeshBuilder::Submesh& importInfo,
    GLTF2::GLTFLoadDataResult& targetPositions, GLTF2::GLTFLoadDataResult& targetNormals,
    GLTF2::GLTFLoadDataResult& targetTangents)
{
    // All targets collected to single buffer.
    for (const auto& target : primitive.targets) {
        for (const auto& targetAttribute : target.target) {
            GLTF2::GLTFLoadDataResult loadDataResult = GLTF2::LoadData(*targetAttribute.accessor);
            if (loadDataResult.success) {
                switch (targetAttribute.attribute.type) {
                    case GLTF2::AttributeType::POSITION:
#if defined(GLTF2_EXTENSION_IGFX_COMPRESSED)
                        // This is for the IGFX_compressed extension. Morph target offsets were multiplied by 10000(!)
                        // and cast to int16.
                        if (target.iGfxCompressed && loadDataResult.componentType == GLTF2::ComponentType::SHORT) {
                            loadDataResult.normalized = true;
                            ConvertLoadResultToFloat(loadDataResult, 10000.f);
                        }
#endif
                        ProcessMorphTargetData(importInfo, primitive.targets.size(), loadDataResult, targetPositions);
                        break;

                    case GLTF2::AttributeType::NORMAL:
                        ProcessMorphTargetData(importInfo, primitive.targets.size(), loadDataResult, targetNormals);
                        break;

                    case GLTF2::AttributeType::TANGENT:
                        ProcessMorphTargetData(importInfo, primitive.targets.size(), loadDataResult, targetTangents);
                        break;

                    case GLTF2::AttributeType::TEXCOORD:
                    case GLTF2::AttributeType::JOINTS:
                    case GLTF2::AttributeType::WEIGHTS:
                    case GLTF2::AttributeType::COLOR:
                    case GLTF2::AttributeType::INVALID:
                    default:
                        // NOTE: Technically there could be custom attributes, but those are not supported at all
                        // currently!
                        break;
                }
            }
        }
    }
}

IndexType GetPrimitiveIndexType(const GLTF2::MeshPrimitive& primitive)
{
    switch (primitive.indices->componentType) {
        case GLTF2::ComponentType::UNSIGNED_BYTE:
            return CORE_INDEX_TYPE_UINT16;

        case GLTF2::ComponentType::UNSIGNED_SHORT:
            return CORE_INDEX_TYPE_UINT16;

        case GLTF2::ComponentType::UNSIGNED_INT:
            return CORE_INDEX_TYPE_UINT32;

        case GLTF2::ComponentType::INVALID:
        case GLTF2::ComponentType::BYTE:
        case GLTF2::ComponentType::SHORT:
        case GLTF2::ComponentType::INT:
        case GLTF2::ComponentType::FLOAT:
            break;
        default:
            break;
    }

    CORE_ASSERT_MSG(false, "Not supported index type.");

    return CORE_INDEX_TYPE_UINT32;
}

bool ContainsAttribute(const GLTF2::MeshPrimitive& primitive, GLTF2::AttributeType type)
{
    return std::any_of(primitive.attributes.begin(), primitive.attributes.end(),
        [type](const GLTF2::Attribute& attribute) { return attribute.attribute.type == type; });
}

IMeshBuilder::Submesh CreatePrimitiveImportInfo(const GLTFImportResult& importResult,
    const IMaterialComponentManager& materialManager, const GLTF2::MeshPrimitive& primitive)
{
    IMeshBuilder::Submesh info;
    bool hasNormalMap = false;

    // Get material, if one assigned.
    if (primitive.materialIndex != GLTF2::GLTF_INVALID_INDEX &&
        primitive.materialIndex < importResult.data.materials.size()) {
        info.material = importResult.data.materials[primitive.materialIndex];
        hasNormalMap = (primitive.material->normalTexture.textureInfo.index != GLTF2::GLTF_INVALID_INDEX ||
                        primitive.material->clearcoat.normalTexture.textureInfo.index != GLTF2::GLTF_INVALID_INDEX);
    }

    info.colors = ContainsAttribute(primitive, GLTF2::AttributeType::COLOR);
    info.joints = ContainsAttribute(primitive, GLTF2::AttributeType::JOINTS);
    info.tangents = ContainsAttribute(primitive, GLTF2::AttributeType::TANGENT);
    if (!info.tangents) {
        // If material has normal map assigned, then always make sure that we have normals.
        info.tangents = hasNormalMap;
    }

    if (!info.tangents) {
        // NOTE. Currenty morph render node always writes tangent data to output buffer.
        // Therefore we must have tangents defined for this primitive.
        info.tangents = primitive.targets.size() > 0;
    }
    if (const auto pos = std::find_if(primitive.attributes.begin(), primitive.attributes.end(),
            [](const GLTF2::Attribute& attribute) {
                return attribute.attribute.type == GLTF2::AttributeType::POSITION;
            });
        pos != primitive.attributes.end()) {
        info.vertexCount = pos->accessor->count;
    }

    if (primitive.indices) {
        info.indexCount = primitive.indices->count;
        info.indexType = GetPrimitiveIndexType(primitive);
    }

    info.morphTargetCount = (uint32_t)primitive.targets.size();

    return info;
}

string GatherErrorStrings(size_t primitiveIndex, const GLTF2::GLTFLoadDataResult& position,
    const GLTF2::GLTFLoadDataResult& normal, array_view<const GLTF2::GLTFLoadDataResult> texcoords,
    const GLTF2::GLTFLoadDataResult& tangent, const GLTF2::GLTFLoadDataResult& color,
    const GLTF2::GLTFLoadDataResult& joint, const GLTF2::GLTFLoadDataResult& weight)
{
    string error = "Failed to load primitive " + to_string(primitiveIndex) + '\n' + position.error + normal.error;
    for (const auto& tc : texcoords) {
        error += tc.error;
    }
    error += tangent.error + color.error + joint.error + weight.error + '\n';
    return error;
}

GLTF2::GLTFLoadDataResult LoadIndices(GatherMeshDataResult& result, const GLTF2::MeshPrimitive& primitive,
    IndexType indexType, uint32_t loadedVertexCount)
{
    GLTF2::GLTFLoadDataResult loadDataResult;
    if (primitive.indices) {
        if (auto indicesLoadResult = LoadData(*primitive.indices); indicesLoadResult.success) {
            ValidateIndices(indicesLoadResult, loadedVertexCount);
            loadDataResult = move(indicesLoadResult);
        }
        if (!loadDataResult.success) {
            result.error += loadDataResult.error;
            result.success = false;
        }
    }
    return loadDataResult;
}

void ProcessPrimitives(GatherMeshDataResult& result, uint32_t flags, array_view<const GLTF2::MeshPrimitive> primitives)
{
    // Feed primitive data for builder.
    for (size_t primitiveIndex = 0, count = primitives.size(); primitiveIndex < count; ++primitiveIndex) {
        const auto& primitive = primitives[primitiveIndex];
        const auto& importInfo = result.meshBuilder->GetSubmesh(primitiveIndex);

        // Load data.
        GLTF2::GLTFLoadDataResult position, normal, tangent, color, joint, weight;
        GLTF2::GLTFLoadDataResult texcoords[2];
        if (!LoadPrimitiveAttributeData(primitive, position, normal, texcoords, tangent, joint, weight, color, flags)) {
            result.error +=
                GatherErrorStrings(primitiveIndex, position, normal, texcoords, tangent, color, joint, weight);
            result.success = false;
            break;
        }

        uint32_t const loadedVertexCount = static_cast<uint32_t>(position.elementCount);

        auto fillDataBuffer = [](GLTF2::GLTFLoadDataResult& attribute) {
            return IMeshBuilder::DataBuffer {
                Convert(attribute.componentType, attribute.componentCount, attribute.normalized),
                static_cast<uint32_t>(attribute.elementSize),
                attribute.data,
            };
        };
        const IMeshBuilder::DataBuffer positions = fillDataBuffer(position);
        const IMeshBuilder::DataBuffer normals = fillDataBuffer(normal);
        const IMeshBuilder::DataBuffer texcoords0 = fillDataBuffer(texcoords[0]);
        const IMeshBuilder::DataBuffer texcoords1 = fillDataBuffer(texcoords[1]);
        const IMeshBuilder::DataBuffer tangents = fillDataBuffer(tangent);
        const IMeshBuilder::DataBuffer colors = fillDataBuffer(color);

        result.meshBuilder->SetVertexData(primitiveIndex, positions, normals, texcoords0, texcoords1, tangents, colors);

        // Process indices.
        GLTF2::GLTFLoadDataResult indices = LoadIndices(result, primitive, importInfo.indexType, loadedVertexCount);
        if (!indices.data.empty()) {
            const IMeshBuilder::DataBuffer data {
                (indices.elementSize == sizeof(uint32_t))
                    ? BASE_FORMAT_R32_UINT
                    : ((indices.elementSize == sizeof(uint16_t)) ? BASE_FORMAT_R16_UINT : BASE_FORMAT_R8_UINT),
                static_cast<uint32_t>(indices.elementSize), { indices.data }
            };
            result.meshBuilder->SetIndexData(primitiveIndex, data);
        }

        // Set AABB.
        if (position.min.size() == 3 && position.max.size() == 3) {
            const Math::Vec3 min = { position.min[0], position.min[1], position.min[2] };
            const Math::Vec3 max = { position.max[0], position.max[1], position.max[2] };
            result.meshBuilder->SetAABB(primitiveIndex, min, max);
        } else {
            result.meshBuilder->CalculateAABB(primitiveIndex, positions);
        }

        // Process joints.
        if (!joint.data.empty() && (flags & CORE_GLTF_IMPORT_RESOURCE_SKIN)) {
            const IMeshBuilder::DataBuffer joints = fillDataBuffer(joint);
            const IMeshBuilder::DataBuffer weights = fillDataBuffer(weight);
            result.meshBuilder->SetJointData(primitiveIndex, joints, weights, positions);
        }
        // Process morphs.
        if (primitive.targets.size()) {
            GLTF2::GLTFLoadDataResult targetPosition, targetNormal, targetTangent;
            GenerateMorphTargets(primitive, importInfo, targetPosition, targetNormal, targetTangent);
            const IMeshBuilder::DataBuffer targetPositions = fillDataBuffer(targetPosition);
            const IMeshBuilder::DataBuffer targetNormals = fillDataBuffer(targetNormal);
            const IMeshBuilder::DataBuffer targetTangents = fillDataBuffer(targetTangent);

            result.meshBuilder->SetMorphTargetData(
                primitiveIndex, positions, normals, tangents, targetPositions, targetNormals, targetTangents);
        }
    }
}

GatherMeshDataResult GatherMeshData(const GLTF2::Mesh& mesh, const GLTFImportResult& importResult, uint32_t flags,
    const IMaterialComponentManager& materialManager, const IDevice& device, IEngine& engine)
{
    GatherMeshDataResult result;
    auto context = GetInstance<IRenderContext>(*engine.GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
    if (!context) {
        result.success = false;
        result.error = "RenderContext not found.";
        return result;
    }
    auto& shaderManager = device.GetShaderManager();
    const VertexInputDeclarationView vertexInputDeclaration =
        shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
            DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));

    result.meshBuilder = CreateInstance<IMeshBuilder>(*context, UID_MESH_BUILDER);
    result.meshBuilder->Initialize(vertexInputDeclaration, mesh.primitives.size());

    // Create primitive import info for mesh builder.
    for (const auto& primitive : mesh.primitives) {
        // Add to builder.
        result.meshBuilder->AddSubmesh(CreatePrimitiveImportInfo(importResult, materialManager, primitive));
    }

    // Allocate memory for builder.
    result.meshBuilder->Allocate();

    // Feed primitive data for builder.
    ProcessPrimitives(result, flags, mesh.primitives);

    if (result.meshBuilder->GetVertexCount()) {
        result.meshBuilder->CreateGpuResources();
    }

    return result;
}

Entity ImportMesh(IEcs& ecs, const GatherMeshDataResult& gatherResult)
{
    // No vertices, which means we can't import mesh.
    if (gatherResult.meshBuilder->GetVertexCount() == 0) {
        return {};
    }
    auto meshEntity = gatherResult.meshBuilder->CreateMesh(ecs);
    return meshEntity;
}

string ResolveNodePath(GLTF2::Node const& node)
{
    string path;

    auto length = node.name.size();
    GLTF2::Node* parent = node.parent;
    while (parent) {
        length += parent->name.size() + 1U;
        parent = parent->parent;
    }

    path.resize(length);
    length -= node.name.size();
    const auto begin = path.begin();
    path.replace(begin + static_cast<string::difference_type>(length),
        begin + static_cast<string::difference_type>(length + node.name.size()), node.name);

    parent = node.parent;
    while (parent) {
        length -= 1U;
        path[length] = '/';
        length -= parent->name.size();
        path.replace(begin + static_cast<string::difference_type>(length),
            begin + static_cast<string::difference_type>(length + parent->name.size()), parent->name);
        parent = parent->parent;
    }

    return path;
}

bool BuildSkinIbmComponent(GLTF2::Skin const& skin, SkinIbmComponent& skinIbm)
{
    skinIbm.matrices.reserve(skin.joints.size());
    bool failed = false;
    bool useIdentityMatrix = true;
    if (skin.inverseBindMatrices) {
        GLTF2::GLTFLoadDataResult loadDataResult = GLTF2::LoadData(*skin.inverseBindMatrices);
        if (loadDataResult.success) {
            useIdentityMatrix = false;
            auto ibls = array_view(
                reinterpret_cast<Math::Mat4X4 const*>(loadDataResult.data.data()), loadDataResult.elementCount);
            skinIbm.matrices.insert(skinIbm.matrices.end(), ibls.begin(), ibls.end());
        }
    }
    if (failed) {
        return false;
    }

    if (useIdentityMatrix) {
        skinIbm.matrices.insert(skinIbm.matrices.end(), skin.joints.size(), Math::IDENTITY_4X4);
    }

    return true;
}

enum ImporterImageUsageFlags : uint32_t {
    IMAGE_USAGE_BASE_COLOR_BIT = (1 << 1),
    IMAGE_USAGE_METALLIC_ROUGHNESS_BIT = (1 << 2),
    IMAGE_USAGE_NORMAL_BIT = (1 << 3),
    IMAGE_USAGE_EMISSIVE_BIT = (1 << 4),
    IMAGE_USAGE_OCCLUSION_BIT = (1 << 5),
    IMAGE_USAGE_SPECULAR_GLOSSINESS_BIT = (1 << 6),
    IMAGE_USAGE_CLEARCOAT_BIT = (1 << 7),
    IMAGE_USAGE_CLEARCOAT_ROUGHNESS_BIT = (1 << 8),
    IMAGE_USAGE_SHEEN_COLOR_BIT = (1 << 9),
    IMAGE_USAGE_SHEEN_ROUGHNESS_BIT = (1 << 10),
    IMAGE_USAGE_SPECULAR_BIT = (1 << 11),
    IMAGE_USAGE_TRANSMISSION_BIT = (1 << 12),
    IMAGE_USAGE_SINGLE_CHANNEL = IMAGE_USAGE_OCCLUSION_BIT | IMAGE_USAGE_TRANSMISSION_BIT
};

inline bool operator==(const GLTF2::TextureInfo& info, const GLTF2::Image& image) noexcept
{
    return info.texture && info.texture->image == &image;
}

inline void BaseColorFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.metallicRoughness.baseColorTexture == image) {
        result |= (IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_SRGB_BIT |
                   IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_PREMULTIPLY_ALPHA);
        usage |= IMAGE_USAGE_BASE_COLOR_BIT;
    }
}

inline void MetallicRoughnessFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.metallicRoughness.metallicRoughnessTexture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        usage |= IMAGE_USAGE_METALLIC_ROUGHNESS_BIT;
    }
}

inline void NormalFlags(const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.normalTexture.textureInfo == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        usage |= IMAGE_USAGE_NORMAL_BIT;
    }
}

inline void EmissiveFlags(const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.emissiveTexture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_SRGB_BIT;
        usage |= IMAGE_USAGE_EMISSIVE_BIT;
    }
}

inline void OcclusionFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.occlusionTexture.textureInfo == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        usage |= IMAGE_USAGE_OCCLUSION_BIT;
    }
}

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
inline void SpecularGlossinessFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.specularGlossiness.specularGlossinessTexture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_SRGB_BIT;
        usage |= IMAGE_USAGE_SPECULAR_GLOSSINESS_BIT;
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT)
inline void ClearcoatFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.clearcoat.texture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        usage |= IMAGE_USAGE_CLEARCOAT_BIT;
    }
}

inline void ClearcoatRoughnessFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.clearcoat.roughnessTexture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        usage |= IMAGE_USAGE_CLEARCOAT_ROUGHNESS_BIT;
    }
}

inline void ClearcoatNormalFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.clearcoat.normalTexture.textureInfo == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        usage |= IMAGE_USAGE_NORMAL_BIT;
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
inline void SheenFlags(const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.sheen.texture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_SRGB_BIT;
        usage |= IMAGE_USAGE_SHEEN_COLOR_BIT;
    }
}

inline void SheenRoughnessFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.sheen.roughnessTexture == image) {
        if (!(usage & IMAGE_USAGE_SHEEN_COLOR_BIT)) {
            result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        }
        usage |= IMAGE_USAGE_SHEEN_ROUGHNESS_BIT;
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
inline void SpecularColorFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.specular.colorTexture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_SRGB_BIT;
        usage |= IMAGE_USAGE_SPECULAR_BIT;
    }
}

inline void SpecularStrengthFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.specular.texture == image) {
        if (!(usage & IMAGE_USAGE_SPECULAR_BIT)) {
            result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        }
        usage |= IMAGE_USAGE_SPECULAR_BIT;
    }
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
inline void TransmissionFlags(
    const GLTF2::Material& material, const GLTF2::Image& image, uint32_t& result, uint32_t& usage)
{
    if (material.transmission.texture == image) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
        usage |= IMAGE_USAGE_TRANSMISSION_BIT;
    }
}
#endif

uint32_t ResolveImageLoadFlags(GLTF2::Image const& image, GLTF2::Data const& data)
{
    // Resolve whether image should be imported as SRGB or LINEAR.
    uint32_t result = 0;
    // Resolve in which parts of material this texture has been used.
    uint32_t usage = 0;

    // Generating mipmaps for all textures (if not already contained in the image).
    result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_GENERATE_MIPS;

    for (const auto& material : data.materials) {
        BaseColorFlags(*material, image, result, usage);
        MetallicRoughnessFlags(*material, image, result, usage);
        NormalFlags(*material, image, result, usage);
        EmissiveFlags(*material, image, result, usage);
        OcclusionFlags(*material, image, result, usage);

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
        SpecularGlossinessFlags(*material, image, result, usage);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT)
        ClearcoatFlags(*material, image, result, usage);
        ClearcoatRoughnessFlags(*material, image, result, usage);
        ClearcoatNormalFlags(*material, image, result, usage);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
        SheenFlags(*material, image, result, usage);
        SheenRoughnessFlags(*material, image, result, usage);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
        SpecularColorFlags(*material, image, result, usage);
        SpecularStrengthFlags(*material, image, result, usage);
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
        TransmissionFlags(*material, image, result, usage);
#endif
    }

    // In case the texture is only used in occlusion channel, we can convert it to grayscale R8.
    if ((usage & (IMAGE_USAGE_SINGLE_CHANNEL)) && !(usage & ~(IMAGE_USAGE_SINGLE_CHANNEL))) {
        result |= IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_GRAYSCALE_BIT;
    }

    const bool isSRGB = result & IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_SRGB_BIT;
    const bool isLinear = result & IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    if (isSRGB && isLinear) {
        // In case the texture has both SRGB & LINEAR set, default to SRGB and print a warning.
        result &= ~IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;

        if (GLTF2::IsDataURI(image.uri)) {
            CORE_LOG_W("Unable to resolve color space for Image, defaulting to SRGB.");
        } else {
            CORE_LOG_W("Unable to resolve color space for Image %s, defaulting to SRGB.", image.uri.c_str());
        }
    }

    return result;
}

EntityReference ResolveSampler(
    GLTF2::Texture const& texture, GLTF2::Data const& data, GLTFImportResult const& importResult)
{
    if (texture.sampler) {
        const size_t index = FindIndex(data.samplers, texture.sampler);
        if (index != GLTF2::GLTF_INVALID_INDEX) {
            return importResult.data.samplers[index];
        }
    }

    return {};
}

inline EntityReference ResolveSampler(
    const GLTF2::TextureInfo& textureInfo, GLTF2::Data const& data, GLTFImportResult const& importResult)
{
    return textureInfo.texture ? ResolveSampler(*textureInfo.texture, data, importResult) : EntityReference {};
}

EntityReference ResolveDefaultSampler(IEcs& ecs, GLTF2::Material const& material,
    IGpuResourceManager const& gpuResourceManager, GLTF2::Data const& data, GLTFImportResult const& importResult)
{
    if (material.type == GLTF2::Material::Type::MetallicRoughness) {
        if (auto base = ResolveSampler(material.metallicRoughness.baseColorTexture, data, importResult); base) {
            return base;
        } else if (auto mat = ResolveSampler(material.metallicRoughness.metallicRoughnessTexture, data, importResult);
                   mat) {
            return mat;
        }
    } else if (material.type == GLTF2::Material::Type::SpecularGlossiness) {
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
        if (auto diff = ResolveSampler(material.specularGlossiness.diffuseTexture, data, importResult); diff) {
            return diff;
        } else if (auto spec =
                       ResolveSampler(material.specularGlossiness.specularGlossinessTexture, data, importResult);
                   spec) {
            return spec;
        }
#else
        return gpuResourceManager.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT");
#endif
    } else if (material.type == GLTF2::Material::Type::Unlit) {
        if (auto spl = ResolveSampler(material.metallicRoughness.baseColorTexture, data, importResult); spl) {
            return spl;
        }
    }
    if (auto nor = ResolveSampler(material.normalTexture.textureInfo, data, importResult); nor) {
        return nor;
    } else if (auto emis = ResolveSampler(material.emissiveTexture, data, importResult); emis) {
        return emis;
    } else if (auto ao = ResolveSampler(material.occlusionTexture.textureInfo, data, importResult); ao) {
        return ao;
    }

    // No sampler found, use default.
    auto uriManager = GetManager<IUriComponentManager>(ecs);
    auto gpuHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    constexpr string_view samplerUri = "engine://CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT";
    Entity samplerEntity = LookupResourceByUri(samplerUri, *uriManager, *gpuHandleManager);
    if (!EntityUtil::IsValid(samplerEntity)) {
        samplerEntity = ecs.GetEntityManager().Create();

        uriManager->Create(samplerEntity);
        uriManager->Write(samplerEntity)->uri = samplerUri;

        gpuHandleManager->Create(samplerEntity);
        constexpr auto start = samplerUri.rfind('/') + 1;
        constexpr auto samplerName = samplerUri.substr(start);
        // the imported resources don't own this resource
        gpuHandleManager->Write(samplerEntity)->reference = gpuResourceManager.GetSamplerHandle(samplerName);
    }

    return ecs.GetEntityManager().GetReferenceCounted(samplerEntity);
}

// Textures
IImageLoaderManager::LoadResult GatherImageData(GLTF2::Image& image, GLTF2::Data const& data, IFileManager& fileManager,
    IImageLoaderManager& imageManager, uint32_t loadFlags = 0)
{
    vector<uint8_t> raw;

    const GLTF2::BufferView* bufferView = image.bufferView;

    if (bufferView && image.type != GLTF2::MimeType::INVALID) {
        if (bufferView->data) {
            raw.reserve(image.bufferView->byteLength);
            raw.insert(raw.end(), image.bufferView->data, bufferView->data + bufferView->byteLength);
        }
    } else if (image.uri.size()) {
        auto extension = GetImageExtension(image.type);
        const auto result = GLTF2::LoadUri(image.uri, "image", data.filepath, fileManager, extension, raw);
        switch (result) {
            case GLTF2::URI_LOAD_FAILED_TO_DECODE_BASE64:
                return IImageLoaderManager::LoadResult { false, "Base64 decoding failed.", nullptr };

            case GLTF2::URI_LOAD_FAILED_TO_READ_FILE:
                return IImageLoaderManager::LoadResult { false, "Failed to read file.", nullptr };

            case GLTF2::URI_LOAD_FAILED_INVALID_MIME_TYPE:
                return IImageLoaderManager::LoadResult { false, "Image data is not image type.", nullptr };

            default:
            case GLTF2::URI_LOAD_SUCCESS:
                break;
        }
    }

    // Resolve image usage and determine flags.
    const uint32_t flags = ResolveImageLoadFlags(image, data) | loadFlags;

    array_view<const uint8_t> rawdata { raw };
    return imageManager.LoadImage(rawdata, flags);
}

void ResolveReferencedImages(GLTF2::Data const& data, vector<bool>& imageLoadingRequred)
{
    for (size_t i = 0; i < data.textures.size(); ++i) {
        const GLTF2::Texture& texture = *(data.textures[i]);
        const size_t index = FindIndex(data.images, texture.image);
        if (index != GLTF2::GLTF_INVALID_INDEX) {
            imageLoadingRequred[index] = true;
        }
    }

#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
    // Find image references from image based lights.
    for (size_t i = 0; i < data.imageBasedLights.size(); ++i) {
        const auto& light = *data.imageBasedLights[i];
        if (light.skymapImage != GLTF2::GLTF_INVALID_INDEX && light.skymapImage < data.images.size()) {
            imageLoadingRequred[light.skymapImage] = true;
        }
        if (light.specularCubeImage != GLTF2::GLTF_INVALID_INDEX && light.specularCubeImage < data.images.size()) {
            imageLoadingRequred[light.specularCubeImage] = true;
        }
    }
#endif
}

AnimationTrackComponent::Interpolation ConvertAnimationInterpolation(GLTF2::AnimationInterpolation mode)
{
    switch (mode) {
        case GLTF2::AnimationInterpolation::INVALID:
            CORE_ASSERT(false);
            break;
        case GLTF2::AnimationInterpolation::STEP:
            return AnimationTrackComponent::Interpolation::STEP;
        case GLTF2::AnimationInterpolation::LINEAR:
            return AnimationTrackComponent::Interpolation::LINEAR;
        case GLTF2::AnimationInterpolation::SPLINE:
            return AnimationTrackComponent::Interpolation::SPLINE;
        default:
            break;
    }

    return AnimationTrackComponent::Interpolation::LINEAR;
}

template<class T>
void CopyFrames(GLTF2::GLTFLoadDataResult const& animationFrameDataResult, vector<T>& destination)
{
    destination.resize(animationFrameDataResult.elementCount);
    if (animationFrameDataResult.componentType == GLTF2::ComponentType::FLOAT) {
        CORE_ASSERT(animationFrameDataResult.elementSize == sizeof(T));

        const size_t dataSizeInBytes = animationFrameDataResult.elementSize * animationFrameDataResult.elementCount;
        if (!CloneData(destination.data(), destination.size() * sizeof(T), animationFrameDataResult.data.data(),
                dataSizeInBytes)) {
            CORE_LOG_E("Copying of raw framedata failed.");
        }
    } else {
        // Convert data.
        if (animationFrameDataResult.normalized) {
            ConvertNormalizedDataToFloat(animationFrameDataResult, reinterpret_cast<float*>(destination.data()));
        } else {
            ConvertDataToFloat(animationFrameDataResult, reinterpret_cast<float*>(destination.data()));
        }
    }
}

template<>
void CopyFrames(GLTF2::GLTFLoadDataResult const& animationFrameDataResult, vector<bool>& destination)
{
    destination.resize(animationFrameDataResult.elementCount);
    if (animationFrameDataResult.componentType == GLTF2::ComponentType::BYTE ||
        animationFrameDataResult.componentType == GLTF2::ComponentType::UNSIGNED_BYTE) {
        CORE_ASSERT(animationFrameDataResult.elementSize == sizeof(bool));

        const size_t dataSizeInBytes = animationFrameDataResult.elementSize * animationFrameDataResult.elementCount;
        if (!CloneData(destination.data(), destination.size() * sizeof(bool), animationFrameDataResult.data.data(),
                dataSizeInBytes)) {
            CORE_LOG_E("Copying of raw framedata failed.");
        }
    } else {
        // Convert data.
        ConvertDataToBool(animationFrameDataResult, destination.data());
    }
}

bool BuildAnimationInput(GLTF2::Data const& data, IFileManager& fileManager, GLTFImportResult& result,
    GLTF2::Accessor& inputAccessor, AnimationInputComponent& inputComponent)
{
    const GLTF2::GLTFLoadDataResult animationInputDataResult = LoadData(inputAccessor);
    if (animationInputDataResult.success) {
        // Copy timestamps.
        inputComponent.timestamps.reserve(animationInputDataResult.elementCount);
        const float* timeStamps =
            reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(animationInputDataResult.data.data()));
        inputComponent.timestamps.insert(
            inputComponent.timestamps.end(), timeStamps, timeStamps + animationInputDataResult.elementCount);
    }

    return animationInputDataResult.success;
}

template<typename ReadType>
void AppendAnimationOutputData(uint64_t outputTypeHash, const GLTF2::GLTFLoadDataResult& animationOutputDataResult,
    AnimationOutputComponent& outputComponent)
{
    outputComponent.type = outputTypeHash;
    vector<ReadType> positions;
    CopyFrames(animationOutputDataResult, positions);
    const auto dataView = array_view(reinterpret_cast<const uint8_t*>(positions.data()), positions.size_in_bytes());
    outputComponent.data.insert(outputComponent.data.end(), dataView.begin(), dataView.end());
}

bool BuildAnimationOutput(GLTF2::Data const& data, IFileManager& fileManager, GLTFImportResult& result,
    GLTF2::Accessor& outputAccessor, GLTF2::AnimationPath path, AnimationOutputComponent& outputComponent)
{
    const GLTF2::GLTFLoadDataResult animationOutputDataResult = LoadData(outputAccessor);
    if (animationOutputDataResult.success) {
        switch (path) {
            case GLTF2::AnimationPath::TRANSLATION:
                AppendAnimationOutputData<Math::Vec3>(PropertyType::VEC3_T, animationOutputDataResult, outputComponent);
                break;

            case GLTF2::AnimationPath::ROTATION:
                AppendAnimationOutputData<Math::Vec4>(PropertyType::QUAT_T, animationOutputDataResult, outputComponent);
                break;

            case GLTF2::AnimationPath::SCALE:
                AppendAnimationOutputData<Math::Vec3>(PropertyType::VEC3_T, animationOutputDataResult, outputComponent);
                break;

            case GLTF2::AnimationPath::WEIGHTS:
                AppendAnimationOutputData<float>(
                    PropertyType::FLOAT_VECTOR_T, animationOutputDataResult, outputComponent);
                break;

            case GLTF2::AnimationPath::VISIBLE:
                AppendAnimationOutputData<bool>(PropertyType::BOOL_T, animationOutputDataResult, outputComponent);
                break;

            case GLTF2::AnimationPath::OPACITY:
                AppendAnimationOutputData<float>(PropertyType::FLOAT_T, animationOutputDataResult, outputComponent);
                break;

            case GLTF2::AnimationPath::INVALID:
            default:
                CORE_LOG_W("Animation.channel.path type %u not implemented", path);
                break;
        }
    }

    return animationOutputDataResult.success;
}

#if defined(GLTF2_EXTENSION_KHR_TEXTURE_TRANSFORM)
void FillTextureTransform(const GLTF2::TextureInfo& textureInfo, MaterialComponent::TextureTransform& desc)
{
    const auto& transform = textureInfo.transform;
    desc.translation = transform.offset;
    desc.rotation = transform.rotation;
    desc.scale = transform.scale;
}
#endif

void FillTextureParams(const GLTF2::TextureInfo& textureInfo, const GLTFImportResult& importResult,
    const GLTF2::Data& data, IEntityManager& entityManager, MaterialComponent& desc,
    MaterialComponent::TextureIndex index)
{
    desc.textures[index].image = GetImportedTextureHandle(importResult, textureInfo.index);
    if (textureInfo.texture) {
        desc.textures[index].sampler = ResolveSampler(*textureInfo.texture, data, importResult);
        desc.useTexcoordSetBit |= static_cast<uint32_t>((textureInfo.texCoordIndex == 1)) << index;
#if defined(GLTF2_EXTENSION_KHR_TEXTURE_TRANSFORM)
        FillTextureTransform(textureInfo, desc.textures[index].transform);
#endif
    }
}

RenderHandleReference ImportTexture(
    IImageContainer::Ptr image, IRenderDataStoreDefaultStaging& staging, IGpuResourceManager& gpuResourceManager)
{
    RenderHandleReference imageHandle;
    if (!image) {
        return imageHandle;
    }

    const auto& imageDesc = image->GetImageDesc();
    const auto& subImageDescs = image->GetBufferImageCopies();
    const auto data = image->GetData();

    // Create image according to the image container's description. (expects that conversion handles everything)
    const GpuImageDesc gpuImageDesc = gpuResourceManager.CreateGpuImageDesc(imageDesc);
    imageHandle = gpuResourceManager.Create(gpuImageDesc);

    if (imageHandle) {
        // Create buffer for uploading image data
        GpuBufferDesc gpuBufferDesc;
        gpuBufferDesc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        gpuBufferDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        gpuBufferDesc.engineCreationFlags =
            EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
            EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER |
            EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY;
        gpuBufferDesc.byteSize = static_cast<uint32_t>(data.size_bytes());
        auto bufferHandle = gpuResourceManager.Create(gpuBufferDesc);
        // Ideally ImageLoader would decode directly to the buffer to save one copy.
        if (auto buffer = static_cast<uint8_t*>(gpuResourceManager.MapBufferMemory(bufferHandle)); buffer) {
            const auto count = Math::min(static_cast<uint32_t>(data.size_bytes()), gpuBufferDesc.byteSize);
            std::copy(data.data(), data.data() + count, buffer);
        }
        gpuResourceManager.UnmapBuffer(bufferHandle);

        // Gather copy operations for all the sub images (mip levels, cube faces)
        vector<BufferImageCopy> copies;
        copies.reserve(subImageDescs.size());
        for (const auto& subImageDesc : subImageDescs) {
            const BufferImageCopy bufferImageCopy {
                subImageDesc.bufferOffset,      // bufferOffset
                subImageDesc.bufferRowLength,   // bufferRowLength
                subImageDesc.bufferImageHeight, // bufferImageHeight
                {
                    CORE_IMAGE_ASPECT_COLOR_BIT,                                // imageAspectFlags
                    subImageDesc.mipLevel,                                      // mipLevel
                    0,                                                          // baseArrayLayer
                    subImageDesc.layerCount,                                    // layerCount
                },                                                              // imageSubresource
                {},                                                             // imageOffset
                { subImageDesc.width, subImageDesc.height, subImageDesc.depth } // imageExtent
            };
            copies.push_back(bufferImageCopy);
        }
        staging.CopyBufferToImage(bufferHandle, imageHandle, copies);
    } else {
        CORE_LOG_W("Creating an import image failed (format:%u, width:%u, height:%u)",
            static_cast<uint32_t>(gpuImageDesc.format), gpuImageDesc.width, gpuImageDesc.height);
    }
    return imageHandle;
}

void FillMetallicRoughness(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
    desc.type = MaterialComponent::Type::METALLIC_ROUGHNESS;
    FillTextureParams(gltfMaterial.metallicRoughness.baseColorTexture, importResult, data, em, desc,
        MaterialComponent::TextureIndex::BASE_COLOR);
    desc.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = gltfMaterial.metallicRoughness.baseColorFactor;

    // Metallic-roughness.
    FillTextureParams(gltfMaterial.metallicRoughness.metallicRoughnessTexture, importResult, data, em, desc,
        MaterialComponent::TextureIndex::MATERIAL);
    desc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.y = gltfMaterial.metallicRoughness.roughnessFactor;
    desc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.z = gltfMaterial.metallicRoughness.metallicFactor;
}

void FillSpecularGlossiness(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
    desc.type = MaterialComponent::Type::SPECULAR_GLOSSINESS;
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
    FillTextureParams(gltfMaterial.specularGlossiness.diffuseTexture, importResult, data, em, desc,
        MaterialComponent::TextureIndex::BASE_COLOR);
    desc.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = gltfMaterial.specularGlossiness.diffuseFactor;

    // Glossiness.
    FillTextureParams(gltfMaterial.specularGlossiness.specularGlossinessTexture, importResult, data, em, desc,
        MaterialComponent::TextureIndex::MATERIAL);
    desc.textures[MaterialComponent::TextureIndex::MATERIAL].factor = { gltfMaterial.specularGlossiness.specularFactor,
        gltfMaterial.specularGlossiness.glossinessFactor };
#endif
}

void FillUnlit(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
    desc.type = MaterialComponent::Type::UNLIT;

    FillTextureParams(gltfMaterial.metallicRoughness.baseColorTexture, importResult, data, em, desc,
        MaterialComponent::TextureIndex::BASE_COLOR);
    desc.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = gltfMaterial.metallicRoughness.baseColorFactor;
}

void FillClearcoat(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT) || defined(GLTF2_EXTRAS_CLEAR_COAT_MATERIAL)
    // Clearcoat.
    desc.textures[MaterialComponent::TextureIndex::CLEARCOAT].factor.x = gltfMaterial.clearcoat.factor;
    FillTextureParams(
        gltfMaterial.clearcoat.texture, importResult, data, em, desc, MaterialComponent::TextureIndex::CLEARCOAT);
    desc.textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].factor.y = gltfMaterial.clearcoat.roughness;
    FillTextureParams(gltfMaterial.clearcoat.roughnessTexture, importResult, data, em, desc,
        MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS);
    FillTextureParams(gltfMaterial.clearcoat.normalTexture.textureInfo, importResult, data, em, desc,
        MaterialComponent::TextureIndex::CLEARCOAT_NORMAL);
    desc.textures[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL].factor.x =
        gltfMaterial.clearcoat.normalTexture.scale;
#endif
}

void FillIor(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
    // IOR.
    if (gltfMaterial.ior.ior != 1.5f) {
        auto reflectance = (gltfMaterial.ior.ior - 1.f) / (gltfMaterial.ior.ior + 1.f);
        desc.textures[MaterialComponent::TextureIndex::MATERIAL].factor.w = reflectance * reflectance;
    }
#endif
}

void FillSheen(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
    // Sheen.
    desc.textures[MaterialComponent::TextureIndex::SHEEN].factor = Math::Vec4(gltfMaterial.sheen.factor, 0.f);
    FillTextureParams(gltfMaterial.sheen.texture, importResult, data, em, desc, MaterialComponent::TextureIndex::SHEEN);
    if (gltfMaterial.sheen.roughnessTexture.texture) {
        CORE_LOG_W("Sheen roughness mapping not supported by gltf2 importer");
    }
    // to sheen alpha
    desc.textures[MaterialComponent::TextureIndex::SHEEN].factor.w = gltfMaterial.sheen.roughness;
#endif
}

void FillSpecular(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
    // Specular.
    desc.textures[MaterialComponent::TextureIndex::SPECULAR].factor =
        Math::Vec4(gltfMaterial.specular.color, gltfMaterial.specular.factor);
    if (gltfMaterial.specular.texture.index != GLTF2::GLTF_INVALID_INDEX &&
        gltfMaterial.specular.colorTexture.index == GLTF2::GLTF_INVALID_INDEX) {
        FillTextureParams(
            gltfMaterial.specular.texture, importResult, data, em, desc, MaterialComponent::TextureIndex::SPECULAR);
    } else if (gltfMaterial.specular.texture.index == GLTF2::GLTF_INVALID_INDEX &&
               gltfMaterial.specular.colorTexture.index != GLTF2::GLTF_INVALID_INDEX) {
        FillTextureParams(gltfMaterial.specular.colorTexture, importResult, data, em, desc,
            MaterialComponent::TextureIndex::SPECULAR);
    } else if (gltfMaterial.specular.texture.index != gltfMaterial.specular.colorTexture.index) {
        CORE_LOG_W("Separate specular strength and color textures are not supported!");
        FillTextureParams(gltfMaterial.specular.colorTexture, importResult, data, em, desc,
            MaterialComponent::TextureIndex::SPECULAR);
    } else {
        FillTextureParams(gltfMaterial.specular.colorTexture, importResult, data, em, desc,
            MaterialComponent::TextureIndex::SPECULAR);
    }
#endif
}

void FillTransmission(MaterialComponent& desc, const GLTFImportResult& importResult, const GLTF2::Data& data,
    const GLTF2::Material& gltfMaterial, IEntityManager& em)
{
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
    // Transmission.
    desc.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x = gltfMaterial.transmission.factor;
    FillTextureParams(
        gltfMaterial.transmission.texture, importResult, data, em, desc, MaterialComponent::TextureIndex::TRANSMISSION);
#endif
}

void SelectShaders(MaterialComponent& desc, const GLTF2::Material& gltfMaterial,
    const GLTF2::GLTF2Importer::DefaultMaterialShaderData& dmShaderData)
{
    // Shadow casting is removed from blend modes by default.
    // Transmission over writes the gltf material blend mode.
    if (desc.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x > 0.0f) {
        desc.materialShader.shader = dmShaderData.blend.shader;
        // no support for double-sideness with default material and transmission
        desc.materialShader.graphicsState = dmShaderData.blend.gfxState;
        if (gltfMaterial.alphaMode == GLTF2::AlphaMode::BLEND) { // blending -> no shadows
            desc.materialLightingFlags &= (~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT);
        }
    } else if (gltfMaterial.alphaMode == GLTF2::AlphaMode::BLEND) {
        desc.materialShader.shader = dmShaderData.blend.shader;
        desc.materialShader.graphicsState =
            gltfMaterial.doubleSided ? dmShaderData.blend.gfxStateDoubleSided : dmShaderData.blend.gfxState;
        desc.materialLightingFlags &= (~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT);
    } else {
        // opaque materials are expected to have alpha value of 1.0f based on the blend state
        desc.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor.w = 1.0f;
        desc.materialShader.shader = dmShaderData.opaque.shader;
        desc.materialShader.graphicsState =
            gltfMaterial.doubleSided ? dmShaderData.opaque.gfxStateDoubleSided : dmShaderData.opaque.gfxState;
        // default materials support instancing with opaque materials.
        desc.extraRenderingFlags |= MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT;
    }
    // shadow shader data
    if (desc.materialLightingFlags & MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT) {
        desc.depthShader.shader = dmShaderData.depth.shader;
        desc.depthShader.graphicsState =
            gltfMaterial.doubleSided ? dmShaderData.depth.gfxStateDoubleSided : dmShaderData.depth.gfxState;
    }
}

void ImportMaterial(GLTFImportResult const& importResult, GLTF2::Data const& data, GLTF2::Material const& gltfMaterial,
    const Entity materialEntity, IMaterialComponentManager& materialManager,
    IGpuResourceManager const& gpuResourceManager, const GLTF2::GLTF2Importer::DefaultMaterialShaderData& dmShaderData)
{
    auto materialHandle = materialManager.Write(materialEntity);
    auto& desc = *materialHandle;
    auto& ecs = materialManager.GetEcs();
    auto& em = ecs.GetEntityManager();
    if (gltfMaterial.type == GLTF2::Material::Type::MetallicRoughness) {
        FillMetallicRoughness(desc, importResult, data, gltfMaterial, em);
    } else if (gltfMaterial.type == GLTF2::Material::Type::SpecularGlossiness) {
        FillSpecularGlossiness(desc, importResult, data, gltfMaterial, em);
    } else if (gltfMaterial.type == GLTF2::Material::Type::Unlit) {
        FillUnlit(desc, importResult, data, gltfMaterial, em);
    }

    desc.textures[MaterialComponent::TextureIndex::BASE_COLOR].sampler =
        ResolveDefaultSampler(ecs, gltfMaterial, gpuResourceManager, data, importResult);

    // Normal texture.
    FillTextureParams(
        gltfMaterial.normalTexture.textureInfo, importResult, data, em, desc, MaterialComponent::TextureIndex::NORMAL);
    desc.textures[MaterialComponent::TextureIndex::NORMAL].factor.x = gltfMaterial.normalTexture.scale;

    // Occlusion texture.
    FillTextureParams(
        gltfMaterial.occlusionTexture.textureInfo, importResult, data, em, desc, MaterialComponent::TextureIndex::AO);
    desc.textures[MaterialComponent::TextureIndex::AO].factor.x = gltfMaterial.occlusionTexture.strength;

    // Emissive texture.
    FillTextureParams(
        gltfMaterial.emissiveTexture, importResult, data, em, desc, MaterialComponent::TextureIndex::EMISSIVE);
    desc.textures[MaterialComponent::TextureIndex::EMISSIVE].factor = gltfMaterial.emissiveFactor;

    // Handle material extension
    FillClearcoat(desc, importResult, data, gltfMaterial, em);
    FillIor(desc, importResult, data, gltfMaterial, em);
    FillSheen(desc, importResult, data, gltfMaterial, em);
    FillSpecular(desc, importResult, data, gltfMaterial, em);
    FillTransmission(desc, importResult, data, gltfMaterial, em);

    // Always receive and cast shadows. (Modified with blend modes below)
    desc.materialLightingFlags |= MaterialComponent::LightingFlagBits::SHADOW_RECEIVER_BIT;
    desc.materialLightingFlags |= MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;

    if (gltfMaterial.alphaMode == GLTF2::AlphaMode::MASK) {
        // we "enable" if it's set
        desc.alphaCutoff = gltfMaterial.alphaCutoff;
    }

    SelectShaders(desc, gltfMaterial, dmShaderData);
}

void ConvertToCoreFilter(GLTF2::FilterMode mode, Filter& outFilter, Filter& outMipmapMode)
{
    switch (mode) {
        case GLTF2::FilterMode::NEAREST:
            outFilter = CORE_FILTER_NEAREST;
            outMipmapMode = CORE_FILTER_NEAREST;
            break;

        default:
        case GLTF2::FilterMode::LINEAR:
            outFilter = CORE_FILTER_LINEAR;
            outMipmapMode = CORE_FILTER_LINEAR;
            break;

        case GLTF2::FilterMode::NEAREST_MIPMAP_NEAREST:
            outFilter = CORE_FILTER_NEAREST;
            outMipmapMode = CORE_FILTER_NEAREST;
            break;

        case GLTF2::FilterMode::LINEAR_MIPMAP_NEAREST:
            outFilter = CORE_FILTER_LINEAR;
            outMipmapMode = CORE_FILTER_NEAREST;
            break;

        case GLTF2::FilterMode::NEAREST_MIPMAP_LINEAR:
            outFilter = CORE_FILTER_NEAREST;
            outMipmapMode = CORE_FILTER_LINEAR;
            break;

        case GLTF2::FilterMode::LINEAR_MIPMAP_LINEAR:
            outFilter = CORE_FILTER_LINEAR;
            outMipmapMode = CORE_FILTER_LINEAR;
            break;
    }
}

void ConvertToCoreFilter(GLTF2::FilterMode mode, Filter& outFilter)
{
    Filter unused;
    ConvertToCoreFilter(mode, outFilter, unused);
}

SamplerAddressMode ConvertToCoreWrapMode(GLTF2::WrapMode mode)
{
    switch (mode) {
        default:
        case GLTF2::WrapMode::CLAMP_TO_EDGE:
            return CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        case GLTF2::WrapMode::MIRRORED_REPEAT:
            return CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

        case GLTF2::WrapMode::REPEAT:
            return CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

void RecursivelyCreateEntities(IEntityManager& entityManager, GLTF2::Data const& data, GLTF2::Node const& node,
    unordered_map<size_t, Entity>& sceneEntities)
{
    // Look up node index nodes array.
    if (size_t const nodeIndex = FindIndex(data.nodes, &node); nodeIndex != GLTF2::GLTF_INVALID_INDEX) {
        // Create entity for this node in this scene.
        sceneEntities[nodeIndex] = entityManager.Create();
    } else {
        // NOTE: Failed to find given node.
    }

    for (auto child : node.children) {
        RecursivelyCreateEntities(entityManager, data, *child, sceneEntities);
    }
}

Entity FindEntity(unordered_map<size_t, Entity> const& sceneEntities, size_t nodeIndex)
{
    if (auto const pos = sceneEntities.find(nodeIndex); pos != sceneEntities.end()) {
        return pos->second;
    } else {
        return {};
    }
}

void CreateNode(IEcs& ecs, const GLTF2::Node& node, const Entity entity, const GLTF2::Data& data,
    const unordered_map<size_t, Entity>& sceneEntities, const Entity sceneEntity)
{
    INodeComponentManager& nodeManager = *(GetManager<INodeComponentManager>(ecs));
    nodeManager.Create(entity);

    ScopedHandle<NodeComponent> component = nodeManager.Write(entity);
    if (const size_t parentIndex = FindIndex(data.nodes, node.parent); parentIndex != GLTF2::GLTF_INVALID_INDEX) {
        component->parent = FindEntity(sceneEntities, parentIndex);
    } else {
        // Set as child of scene.
        component->parent = sceneEntity;
    }
}

void CreateName(IEcs& ecs, const GLTF2::Node& node, const Entity entity)
{
    INameComponentManager& nameManager = *(GetManager<INameComponentManager>(ecs));
    nameManager.Create(entity);
    ScopedHandle<NameComponent> nameHandle = nameManager.Write(entity);
    nameHandle->name = node.name;
}

void CreateTransform(IEcs& ecs, const GLTF2::Node& node, const Entity entity)
{
    ITransformComponentManager& transformManager = *(GetManager<ITransformComponentManager>(ecs));

    transformManager.Create(entity);

    GetManager<ILocalMatrixComponentManager>(ecs)->Create(entity);
    GetManager<IWorldMatrixComponentManager>(ecs)->Create(entity);

    ScopedHandle<TransformComponent> component = transformManager.Write(entity);
    if (node.usesTRS) {
        component->position = node.translation;
        component->rotation = node.rotation;
        component->scale = node.scale;
    } else {
        Math::Vec3 skew;
        Math::Vec4 perspective;

        if (!Math::Decompose(
                node.matrix, component->scale, component->rotation, component->position, skew, perspective)) {
            component->position = { 0.f, 0.f, 0.f };
            component->rotation = { 0.f, 0.f, 0.f, 1.f };
            component->scale = { 1.f, 1.f, 1.f };
        }
    }
}

void CreateMesh(IEcs& ecs, const GLTF2::Node& node, const Entity entity, const GLTF2::Data& data,
    const GLTFResourceData& gltfResourceData)
{
    const size_t meshIndex = FindIndex(data.meshes, node.mesh);
    if (meshIndex != GLTF2::GLTF_INVALID_INDEX && meshIndex < gltfResourceData.meshes.size()) {
        IRenderMeshComponentManager& renderMeshManager = *(GetManager<IRenderMeshComponentManager>(ecs));

        renderMeshManager.Create(entity);
        ScopedHandle<RenderMeshComponent> component = renderMeshManager.Write(entity);
        component->mesh = gltfResourceData.meshes[meshIndex];
    }
}

void CreateCamera(IEcs& ecs, const GLTF2::Node& node, const Entity entity, const Entity environmentEntity)
{
    if (node.camera && node.camera->type != GLTF2::CameraType::INVALID) {
        ICameraComponentManager& cameraManager = *(GetManager<ICameraComponentManager>(ecs));
        cameraManager.Create(entity);
        ScopedHandle<CameraComponent> component = cameraManager.Write(entity);
        if (node.camera->type == GLTF2::CameraType::ORTHOGRAPHIC) {
            component->projection = CameraComponent::Projection::ORTHOGRAPHIC;
            component->xMag = node.camera->attributes.ortho.xmag;
            component->yMag = node.camera->attributes.ortho.ymag;
            component->zFar = node.camera->attributes.ortho.zfar;
            component->zNear = node.camera->attributes.ortho.znear;
        } else {
            // GLTF2::CameraType::PERSPECTIVE
            component->projection = CameraComponent::Projection::PERSPECTIVE;
            component->aspect = node.camera->attributes.perspective.aspect;
            component->yFov = node.camera->attributes.perspective.yfov;
            component->zFar = node.camera->attributes.perspective.zfar;
            component->zNear = node.camera->attributes.perspective.znear;
        }
        component->environment = environmentEntity;
    }
}

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
void CreateLight(IEcs& ecs, const GLTF2::Node& node, const Entity entity)
{
    if (node.light && node.light->type != GLTF2::LightType::INVALID) {
        ILightComponentManager& lightManager = *(GetManager<ILightComponentManager>(ecs));

        LightComponent component = CreateComponent(lightManager, entity);

        component.type = ConvertToCoreLightType(node.light->type);
        component.intensity = node.light->intensity;
        component.color = { node.light->color.x, node.light->color.y, node.light->color.z };

        if (component.type == LightComponent::Type::POINT || component.type == LightComponent::Type::SPOT) {
            // Positional parameters.
            component.range = node.light->positional.range;

            if (component.type == LightComponent::Type::SPOT) {
                // Spot parameters.
                component.spotInnerAngle = node.light->positional.spot.innerAngle;
                component.spotOuterAngle = node.light->positional.spot.outerAngle;
            }
        }

        lightManager.Set(entity, component);
    }
}
#endif

void RecursivelyCreateComponents(IEcs& ecs, GLTF2::Data const& data, GLTF2::Node const& node,
    unordered_map<size_t, Entity> const& sceneEntities, Entity sceneEntity, Entity environmentEntity,
    const GLTFResourceData& gltfResourceData, Entity& defaultCamera, uint32_t flags)
{
    size_t const nodeIndex = FindIndex(data.nodes, &node);
    CORE_ASSERT_MSG(nodeIndex != GLTF2::GLTF_INVALID_INDEX, "Cannot find node: %s", node.name.c_str());

    Entity const entity = FindEntity(sceneEntities, nodeIndex);

    // Apply to node hierarchy.
    CreateNode(ecs, node, entity, data, sceneEntities, sceneEntity);

    // Add name.
    CreateName(ecs, node, entity);

    // Apply transform.
    CreateTransform(ecs, node, entity);

    // Apply mesh.
    if (node.mesh && (flags & CORE_GLTF_IMPORT_COMPONENT_MESH)) {
        CreateMesh(ecs, node, entity, data, gltfResourceData);
    }

    // Apply camera.
    if (flags & CORE_GLTF_IMPORT_COMPONENT_CAMERA) {
        CreateCamera(ecs, node, entity, environmentEntity);

        if (!EntityUtil::IsValid(defaultCamera)) {
            defaultCamera = entity;
        }
    }

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    // Apply light.
    if (flags & CORE_GLTF_IMPORT_COMPONENT_LIGHT) {
        CreateLight(ecs, node, entity);
    }
#endif

#if defined(GLTF2_EXTRAS_RSDZ)
    if (!node.modelIdRSDZ.empty()) {
        IRSDZModelIdComponentManager& rsdzManager = *(GetManager<IRSDZModelIdComponentManager>(ecs));
        RSDZModelIdComponent component = CreateComponent(rsdzManager, entity);
        StringUtil::CopyStringToArray(
            node.modelIdRSDZ, component.modelId, StringUtil::MaxStringLengthFromArray(component.modelId));
        rsdzManager.Set(entity, component);
    }
#endif

    for (auto child : node.children) {
        RecursivelyCreateComponents(
            ecs, data, *child, sceneEntities, sceneEntity, environmentEntity, gltfResourceData, defaultCamera, flags);
    }
}

void AddSkinJointsComponents(const GLTF2::Data& data, const GLTFResourceData& gltfResourceData, ISkinningSystem& ss,
    const unordered_map<size_t, Entity>& sceneEntities)
{
    auto skinEntityIt = gltfResourceData.skins.begin();
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(ss.GetECS());
    // gltfResourceData.skins and data.skins should be the same size but take min just in case.
    auto skins = array_view(data.skins.data(), Math::min(gltfResourceData.skins.size(), data.skins.size()));
    for (auto const& skin : skins) {
        if (skin && (*skinEntityIt)) {
            skinJointsManager->Create(*skinEntityIt);
            auto jointsHandle = skinJointsManager->Write(*skinEntityIt);
            jointsHandle->count = Math::min(countof(jointsHandle->jointEntities), skin->joints.size());
            auto jointEntities = array_view(jointsHandle->jointEntities, jointsHandle->count);
            std::transform(skin->joints.begin(), skin->joints.begin() + static_cast<ptrdiff_t>(jointsHandle->count),
                jointEntities.begin(), [&data, &sceneEntities](const auto& jointNode) {
                    size_t const jointNodeIndex = FindIndex(data.nodes, jointNode);
                    return FindEntity(sceneEntities, jointNodeIndex);
                });
        }
        ++skinEntityIt;
    }
}

void CreateSkinComponents(const GLTF2::Data& data, const GLTFResourceData& gltfResourceData, ISkinningSystem& ss,
    const unordered_map<size_t, Entity>& sceneEntities)
{
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(ss.GetECS());
    for (auto const& node : data.nodes) {
        if (!node->skin) {
            continue;
        }

        if (size_t const skinIndex = FindIndex(data.skins, node->skin);
            skinIndex != GLTF2::GLTF_INVALID_INDEX && skinIndex < gltfResourceData.skins.size()) {
            size_t const nodeIndex = FindIndex(data.nodes, node.get());
            Entity const entity = FindEntity(sceneEntities, nodeIndex);
            if (!EntityUtil::IsValid(entity)) {
                // node was not part of current scene
                continue;
            }

            Entity skeleton {};
            if (node->skin->skeleton) {
                size_t const skeletonIndex = FindIndex(data.nodes, node->skin->skeleton);
                skeleton = FindEntity(sceneEntities, skeletonIndex);
            }

            vector<Entity> joints;
            if (auto jointsHandle = skinJointsManager->Read(gltfResourceData.skins[skinIndex]); jointsHandle) {
                joints.insert(
                    joints.end(), jointsHandle->jointEntities, jointsHandle->jointEntities + jointsHandle->count);
            } else {
                joints.reserve(node->skin->joints.size());
                std::transform(node->skin->joints.begin(), node->skin->joints.end(), std::back_inserter(joints),
                    [&data, &sceneEntities](const auto& jointNode) {
                        size_t const jointNodeIndex = FindIndex(data.nodes, jointNode);
                        return FindEntity(sceneEntities, jointNodeIndex);
                    });
            }
            ss.CreateInstance(gltfResourceData.skins[skinIndex], joints, entity, skeleton);
        }
    }
}

void CreateMorphComponents(const GLTF2::Data& data, const GLTFResourceData& gltfResourceData,
    IMorphComponentManager& mm, const unordered_map<size_t, Entity>& sceneEntities, const Entity sceneEntity)
{
    for (auto const& node : data.nodes) {
        size_t const meshIndex = FindIndex(data.meshes, node->mesh);
        if (meshIndex != GLTF2::GLTF_INVALID_INDEX && meshIndex < gltfResourceData.meshes.size()) {
            if (!node->mesh->primitives.empty()) {
                size_t const nodeIndex = FindIndex(data.nodes, node.get());
                Entity const entity = FindEntity(sceneEntities, nodeIndex);
                if (!EntityUtil::IsValid(entity)) {
                    // node was not part of current scene
                    continue;
                }
                const size_t weightCount = node->mesh->primitives[0].targets.size();
                // Assert that all primitives have the same targets. (the spec is a bit confusing here or i just
                // can't read.)
                for (const auto& primitive : node->mesh->primitives) {
                    CORE_UNUSED(primitive);
                    CORE_ASSERT(primitive.targets.size() == weightCount);
                }

                if (weightCount > 0) {
                    // We have targets, so prepare the morph system/component.
                    vector<string> names;
                    names.reserve(weightCount);
                    std::transform(node->mesh->primitives[0].targets.begin(), node->mesh->primitives[0].targets.end(),
                        std::back_inserter(names), [](const GLTF2::MorphTarget& target) { return target.name; });

                    // Apparently the node/mesh weight list is a concatenation of all primitives targets weights....
                    vector<float> weights;
                    weights.reserve(weightCount);
                    if (!node->weights.empty()) {
                        // Use instance weight (if there is one)
                        weights.insert(weights.end(), node->weights.begin(),
                            node->weights.begin() + static_cast<decltype(node->weights)::difference_type>(weightCount));
                    } else if (!node->mesh->weights.empty()) {
                        // Use mesh weight (if there is one)
                        weights.insert(weights.end(), node->mesh->weights.begin(),
                            node->mesh->weights.begin() +
                                static_cast<decltype(node->weights)::difference_type>(weightCount));
                    } else {
                        // Default to zero weight.
                        weights.insert(weights.end(), weightCount, 0.f);
                    }

                    if (weights.size() > 0) {
                        MorphComponent component;
                        component.morphWeights = move(weights);
                        component.morphNames = move(names);
                        mm.Set(entity, component);
                    }
                }
            }
        }
    }
}

EntityReference GetImageEntity(IEcs& ecs, const GLTFResourceData& gltfResourceData, size_t index)
{
    if ((index != CORE_GLTF_INVALID_INDEX) && (index < gltfResourceData.images.size()) &&
        GetManager<IRenderHandleComponentManager>(ecs)->HasComponent(gltfResourceData.images[index])) {
        return gltfResourceData.images[index];
    }
    return {};
}

Entity CreateEnvironmentComponent(IGpuResourceManager& gpuResourceManager, const GLTF2::Data& data,
    const GLTFResourceData& gltfResourceData, IEcs& ecs, Entity sceneEntity, const size_t lightIndex)
{
    Entity environmentEntity;
#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
    if (lightIndex != GLTF2::GLTF_INVALID_INDEX && lightIndex < data.imageBasedLights.size()) {
        // Create the component to the sceneEntity for convinience.
        environmentEntity = sceneEntity;
        IEnvironmentComponentManager& envManager = *(GetManager<IEnvironmentComponentManager>(ecs));
        envManager.Create(environmentEntity);
        if (auto envHandle = envManager.Write(environmentEntity); envHandle) {
            GLTF2::ImageBasedLight* light = data.imageBasedLights[lightIndex].get();
            envHandle->indirectDiffuseFactor.w = light->intensity;
            envHandle->indirectSpecularFactor.w = light->intensity;
            envHandle->envMapFactor.w = light->intensity;

            envHandle->environmentRotation = light->rotation;

            // Either we have full set of coefficients or there are no coefficients at all.
            CORE_ASSERT(light->irradianceCoefficients.size() == 0u || light->irradianceCoefficients.size() == 9u);

            std::transform(light->irradianceCoefficients.begin(), light->irradianceCoefficients.end(),
                envHandle->irradianceCoefficients, [](const GLTF2::ImageBasedLight::LightingCoeff& coeff) {
                    // Each coefficient needs to have three components.
                    CORE_ASSERT(coeff.size() == 3u);

                    // Values are expected to be prescaled with 1.0 / PI for Lambertian diffuse
                    return Math::Vec3(coeff[0u], coeff[1u], coeff[2u]);
                });

            if (lightIndex < gltfResourceData.specularRadianceCubemaps.size() &&
                EntityUtil::IsValid(gltfResourceData.specularRadianceCubemaps[lightIndex])) {
                envHandle->radianceCubemap = gltfResourceData.specularRadianceCubemaps[lightIndex];
                envHandle->radianceCubemapMipCount = (uint32_t)light->specularImages.size();
            }

            if (auto imageEntity = GetImageEntity(ecs, gltfResourceData, light->specularCubeImage); imageEntity) {
                envHandle->radianceCubemap = imageEntity;
            }

            envHandle->envMapLodLevel = light->skymapImageLodLevel;

            if (auto imageEntity = GetImageEntity(ecs, gltfResourceData, light->skymapImage); imageEntity) {
                envHandle->envMap = imageEntity;
                IRenderHandleComponentManager& gpuHandleManager = *(GetManager<IRenderHandleComponentManager>(ecs));
                if (const auto& envMapHandle = gpuHandleManager.Read(imageEntity)->reference; envMapHandle) {
                    const GpuImageDesc imageDesc = gpuResourceManager.GetImageDescriptor(envMapHandle);
                    if (imageDesc.imageViewType == CORE_IMAGE_VIEW_TYPE_CUBE) {
                        envHandle->background = EnvironmentComponent::Background::CUBEMAP;
                    } else {
                        envHandle->background = EnvironmentComponent::Background::EQUIRECTANGULAR;
                    }
                }
            }
        }
    }
#endif
    return environmentEntity;
}

GLTF2::ImportPhase operator+(GLTF2::ImportPhase lhs, int rhs)
{
    return static_cast<GLTF2::ImportPhase>(static_cast<int>(lhs) + rhs);
}

using ImageLoadResultVector = vector<IImageLoaderManager::LoadResult>;

struct ImageData {
    vector<uint8_t> data;
    vector<BufferImageCopy> copyInfo;
};

ImageData PrepareImageData(const ImageLoadResultVector& imageLoadResults, const GpuImageDesc& imageDesc)
{
    ImageData data;
    const size_t availableMipLayerCount = imageLoadResults.size() / imageDesc.layerCount;
    data.copyInfo.resize(availableMipLayerCount);

    // For all mip levels.
    size_t byteOffset = 0;
    for (size_t mipIndex = 0; mipIndex < availableMipLayerCount; ++mipIndex) {
        {
            const auto& imageLoadResult = imageLoadResults[(mipIndex * 6u)];
            const auto& loadedImageDesc = imageLoadResult.image->GetImageDesc();
            const auto& loadedBufferImageCopy = imageLoadResult.image->GetBufferImageCopies()[0];

            BufferImageCopy& bufferCopy = data.copyInfo[mipIndex];
            bufferCopy.bufferOffset = (uint32_t)byteOffset;
            bufferCopy.bufferRowLength = loadedBufferImageCopy.bufferRowLength;
            bufferCopy.bufferImageHeight = loadedBufferImageCopy.bufferImageHeight;
            bufferCopy.imageOffset.width = 0;
            bufferCopy.imageOffset.height = 0;
            bufferCopy.imageOffset.depth = 0;
            bufferCopy.imageExtent.width = loadedImageDesc.width;
            bufferCopy.imageExtent.height = loadedImageDesc.height;
            bufferCopy.imageExtent.depth = loadedImageDesc.depth;
            bufferCopy.imageSubresource.imageAspectFlags = CORE_IMAGE_ASPECT_COLOR_BIT;
            bufferCopy.imageSubresource.mipLevel = (uint32_t)mipIndex;
            bufferCopy.imageSubresource.baseArrayLayer = 0;
            bufferCopy.imageSubresource.layerCount = imageDesc.layerCount;
        }

        for (uint32_t face = 0; face < imageDesc.layerCount; ++face) {
            const auto& cubeFaceLoadResult = imageLoadResults[(mipIndex * imageDesc.layerCount) + face];
            const auto& cubeFaceBufferImageCopy = cubeFaceLoadResult.image->GetBufferImageCopies()[0];

            const auto& loadedImageData = cubeFaceLoadResult.image->GetData();
            data.data.insert(
                data.data.end(), loadedImageData.begin() + cubeFaceBufferImageCopy.bufferOffset, loadedImageData.end());

            byteOffset += loadedImageData.size() - cubeFaceBufferImageCopy.bufferOffset;
        }
    }
    return data;
}

RenderHandleReference CreateCubemapFromImages(
    uint32_t imageSize, const ImageLoadResultVector& imageLoadResults, IGpuResourceManager& gpuResourceManager)
{
    // Calculate number of mipmaps needed.
    uint32_t mipsize = imageSize;
    uint32_t totalMipCount = 0;
    while (mipsize > 0) {
        ++totalMipCount;
        mipsize >>= 1;
    }

    const ImageUsageFlags usageFlags =
        CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT | CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;

    const GpuImageDesc imageDesc = {
        ImageType::CORE_IMAGE_TYPE_2D,                                 // imageType
        CORE_IMAGE_VIEW_TYPE_CUBE,                                     // imageViewType
        imageLoadResults[0].image->GetImageDesc().format,              // format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,                        // imageTiling
        usageFlags,                                                    // usageFlags
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // memoryPropertyFlags
        ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,    // createFlags
        CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS,                      // engineCreationFlags
        imageSize,                                                     // width
        imageSize,                                                     // height
        1,                                                             // depth
        totalMipCount,                                                 // mipCount
        6,                                                             // layerCount
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,                  // sampleCountFlags
        {},                                                            // componentMapping
    };

    const auto data = PrepareImageData(imageLoadResults, imageDesc);

    auto handle = gpuResourceManager.Create("", imageDesc, data.data, data.copyInfo);
    if (!handle) {
        CORE_LOG_W("Creating an import cubemap image failed (format:%u, width:%u, height:%u)",
            static_cast<uint32_t>(imageDesc.format), imageDesc.width, imageDesc.height);
    }
    return handle;
}

auto FillShaderData(IEntityManager& em, IUriComponentManager& uriManager,
    IRenderHandleComponentManager& renderHandleMgr, const string_view renderSlot,
    GLTF2::GLTF2Importer::DefaultMaterialShaderData::SingleShaderData& shaderData)
{
    auto uri = "3dshaders://" + renderSlot;
    auto resourceEntity = LookupResourceByUri(uri, uriManager, renderHandleMgr);
    shaderData.shader = em.GetReferenceCounted(resourceEntity);

    uri = "3dshaderstates://";
    uri += renderSlot;
    resourceEntity = LookupResourceByUri(uri, uriManager, renderHandleMgr);
    shaderData.gfxState = em.GetReferenceCounted(resourceEntity);

    uri += "_DBL";
    resourceEntity = LookupResourceByUri(uri, uriManager, renderHandleMgr);
    shaderData.gfxStateDoubleSided = em.GetReferenceCounted(resourceEntity);

    if (!(EntityUtil::IsValid(shaderData.shader) && EntityUtil::IsValid(shaderData.shader) &&
            EntityUtil::IsValid(shaderData.shader))) {
        CORE_LOG_W("GLTF2 importer cannot find default shader data.");
    }
}
} // namespace

Entity ImportScene(IDevice& device, size_t sceneIndex, const GLTF2::Data& data,
    const GLTFResourceData& gltfResourceData, IEcs& ecs, Entity rootEntity, GltfSceneImportFlags flags)
{
    // Create entity for this scene.
    const Entity sceneEntity = ecs.GetEntityManager().Create();

    // Create default components for scene.
    ITransformComponentManager& transformManager = *(GetManager<ITransformComponentManager>(ecs));
    transformManager.Create(sceneEntity);
    ILocalMatrixComponentManager& localMatrixManager = *(GetManager<ILocalMatrixComponentManager>(ecs));
    localMatrixManager.Create(sceneEntity);
    IWorldMatrixComponentManager& worldMatrixManager = *(GetManager<IWorldMatrixComponentManager>(ecs));
    worldMatrixManager.Create(sceneEntity);

    auto& scene = data.scenes[sceneIndex];

    INodeComponentManager& nodeManager = *(GetManager<INodeComponentManager>(ecs));
    nodeManager.Create(sceneEntity);
    if (auto nodeHandle = nodeManager.Write(sceneEntity); nodeHandle) {
        nodeHandle->parent = rootEntity;
    }

    // Add name.
    {
        INameComponentManager& nameManager = *(GetManager<INameComponentManager>(ecs));
        nameManager.Create(sceneEntity);
        ScopedHandle<NameComponent> nameHandle = nameManager.Write(sceneEntity);
        nameHandle->name = scene->name;
    }

    // Default camera.
    Entity defaultCamera;

    IEntityManager& em = ecs.GetEntityManager();

    // Create environment.
    Entity environment;
    if (flags & CORE_GLTF_IMPORT_COMPONENT_ENVIRONMENT) {
#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
        const size_t lightIndex = scene->imageBasedLightIndex;
#else
        const size_t lightIndex = 0; // Not used.
#endif
        environment = CreateEnvironmentComponent(
            device.GetGpuResourceManager(), data, gltfResourceData, ecs, sceneEntity, lightIndex);
    }

    // Create entities for nodes in scene.
    unordered_map<size_t, Entity> sceneEntities;
    for (auto node : scene->nodes) {
        RecursivelyCreateEntities(em, data, *node, sceneEntities);
    }

    // Create components for all nodes in this scene.
    for (auto node : scene->nodes) {
        RecursivelyCreateComponents(
            ecs, data, *node, sceneEntities, sceneEntity, environment, gltfResourceData, defaultCamera, flags);
    }

    // Apply skins only after the node hiearachy is complete.
    if (flags & CORE_GLTF_IMPORT_COMPONENT_SKIN) {
        if (ISkinningSystem* ss = GetSystem<ISkinningSystem>(ecs); ss) {
            AddSkinJointsComponents(data, gltfResourceData, *ss, sceneEntities);
            CreateSkinComponents(data, gltfResourceData, *ss, sceneEntities);
        }
    }

    // Add the morphing system to entity if needed. (contains the target weights)
    if (flags & CORE_GLTF_IMPORT_COMPONENT_MORPH) {
        if (IMorphComponentManager* mm = GetManager<IMorphComponentManager>(ecs); mm) {
            CreateMorphComponents(data, gltfResourceData, *mm, sceneEntities, sceneEntity);
        }
    }

    return sceneEntity;
}

namespace GLTF2 {
struct GLTF2Importer::ImporterTask {
    virtual ~ImporterTask() = default;

    enum class State { Queued, Gather, Import, Finished };

    string name;
    uint64_t id;
    bool success { true };

    std::function<bool()> gather;
    std::function<bool()> import;
    std::function<void()> finished;

    State state { State::Queued };
    ImportPhase phase { ImportPhase::FINISHED };
};

class GLTF2Importer::GatherThreadTask final : public IThreadPool::ITask {
public:
    explicit GatherThreadTask(GLTF2Importer& importer, ImporterTask& task) : importer_(importer), task_(task) {};

    void operator()() override
    {
        importer_.Gather(task_);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    GLTF2Importer& importer_;
    ImporterTask& task_;
};

class GLTF2Importer::ImportThreadTask final : public IThreadPool::ITask {
public:
    explicit ImportThreadTask(GLTF2Importer& importer, ImporterTask& task) : importer_(importer), task_(task) {};

    void operator()() override
    {
        importer_.Import(task_);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    GLTF2Importer& importer_;
    ImporterTask& task_;
};

template<class T>
struct GLTF2Importer::GatheredDataTask : GLTF2Importer::ImporterTask {
    T data;
};

template<typename Component>
struct GLTF2Importer::ComponentTaskData {
    EntityReference entity;
    Component component;
};

struct GLTF2Importer::AnimationTaskData {
    GLTF2Importer* importer;
    size_t index;
    string uri;
    string_view name;
    IAnimationComponentManager* animationManager;
    IAnimationTrackComponentManager* trackManager;
    vector<GatheredDataTask<ComponentTaskData<AnimationInputComponent>>*> inputs;
    vector<GatheredDataTask<ComponentTaskData<AnimationOutputComponent>>*> outputs;

    bool operator()()
    {
        const auto& tracks = importer->data_->animations[index]->tracks;
        if (inputs.size() < tracks.size() || outputs.size() < tracks.size()) {
            return false;
        }
        auto& entityManager = importer->ecs_->GetEntityManager();

        auto animationEntity = entityManager.CreateReferenceCounted();
        animationManager->Create(animationEntity);
        const auto animationHandle = animationManager->Write(animationEntity);

        auto& animationTracks = animationHandle->tracks;
        animationTracks.reserve(tracks.size());
        auto inputIt = inputs.begin();
        auto outputIt = outputs.begin();
        float maxLength = 0.f;
        for (const auto& track : tracks) {
            if (track.sampler) {
                const auto animationTrackEntity = entityManager.CreateReferenceCounted();
                animationTracks.push_back(animationTrackEntity);

                trackManager->Create(animationTrackEntity);
                if (auto trackHandle = trackManager->Write(animationTrackEntity); trackHandle) {
                    if (track.channel.node) {
                        // Name the track with the path to the target node. Scene import can then find the
                        // target entity.
                        importer->nameManager_.Create(animationTrackEntity);
                        importer->nameManager_.Write(animationTrackEntity)->name = ResolveNodePath(*track.channel.node);
                    }

                    SelectComponentProperty(track.channel.path, *trackHandle);

                    trackHandle->interpolationMode = ConvertAnimationInterpolation(track.sampler->interpolation);
                    trackHandle->timestamps = (*inputIt)->data.entity;
                    trackHandle->data = (*outputIt)->data.entity;
                    if (!(*inputIt)->data.component.timestamps.empty()) {
                        maxLength = Math::max(maxLength, (*inputIt)->data.component.timestamps.back());
                    }
                }
                ++inputIt;
                ++outputIt;
            }
        }
        animationHandle->duration = maxLength;
        if (!uri.empty()) {
            importer->uriManager_.Create(animationEntity);
            importer->uriManager_.Write(animationEntity)->uri = move(uri);
        }
        if (!name.empty()) {
            importer->nameManager_.Create(animationEntity);
            importer->nameManager_.Write(animationEntity)->name = name;
        }
        importer->result_.data.animations[index] = move(animationEntity);
        return true;
    }

    static void SelectComponentProperty(AnimationPath pathType, AnimationTrackComponent& trackComponent)
    {
        switch (pathType) {
            case GLTF2::AnimationPath::INVALID:
                CORE_ASSERT(false);
                break;
            case GLTF2::AnimationPath::TRANSLATION:
                trackComponent.component = ITransformComponentManager::UID;
                trackComponent.property = "position";
                break;
            case GLTF2::AnimationPath::ROTATION:
                trackComponent.component = ITransformComponentManager::UID;
                trackComponent.property = "rotation";
                break;
            case GLTF2::AnimationPath::SCALE:
                trackComponent.component = ITransformComponentManager::UID;
                trackComponent.property = "scale";
                break;
            case GLTF2::AnimationPath::WEIGHTS:
                trackComponent.component = IMorphComponentManager::UID;
                trackComponent.property = "morphWeights";
                break;
            case GLTF2::AnimationPath::VISIBLE:
                trackComponent.component = INodeComponentManager::UID;
                trackComponent.property = "enabled";
                break;
            case GLTF2::AnimationPath::OPACITY:
                trackComponent.component = IMaterialComponentManager::UID;
                trackComponent.property = "enabled";
                break;
        }
    }
};

GLTF2Importer::GLTF2Importer(IEngine& engine, IRenderContext& renderContext, IEcs& ecs, IThreadPool& pool)
    : engine_(engine), renderContext_(renderContext), device_(renderContext.GetDevice()),
      gpuResourceManager_(device_.GetGpuResourceManager()), ecs_(&ecs),
      gpuHandleManager_(*GetManager<IRenderHandleComponentManager>(*ecs_)),
      materialManager_(*GetManager<IMaterialComponentManager>(*ecs_)),
      meshManager_(*GetManager<IMeshComponentManager>(*ecs_)), nameManager_(*GetManager<INameComponentManager>(*ecs_)),
      uriManager_(*GetManager<IUriComponentManager>(*ecs_)), threadPool_(&pool)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    mainThreadQueue_ = factory->CreateDispatcherTaskQueue({});

    // fetch default shaders and graphics states
    auto& entityMgr = ecs_->GetEntityManager();
    FillShaderData(entityMgr, uriManager_, gpuHandleManager_,
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE, dmShaderData_.opaque);
    FillShaderData(entityMgr, uriManager_, gpuHandleManager_,
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT, dmShaderData_.blend);
    FillShaderData(entityMgr, uriManager_, gpuHandleManager_, DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH,
        dmShaderData_.depth);
}

GLTF2Importer::GLTF2Importer(IEngine& engine, IRenderContext& renderContext, IEcs& ecs)
    : GLTF2Importer(engine, renderContext, ecs,
          *GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY)->CreateThreadPool(IMPORTER_THREADS))
{}

GLTF2Importer::~GLTF2Importer()
{
    Cancel();
}

void GLTF2Importer::ImportGLTF(const IGLTFData& data, GltfResourceImportFlags flags)
{
    Cancel();
    cancelled_ = false;

    data_ = (const GLTF2::Data*)&data;
    if (!data_) {
        CORE_LOG_E("invalid IGLTFData to ImportGLTF");
        return;
    }

    flags_ = flags;

    result_.data.samplers.clear();
    result_.data.images.clear();
    result_.data.textures.clear();
    result_.data.materials.clear();
    result_.data.meshes.clear();
    result_.data.skins.clear();
    result_.data.animations.clear();
    result_.data.specularRadianceCubemaps.clear();

    meshBuilders_.clear();

    // Build tasks.
    Prepare();

    // Fill in task queues for first tasks.
    StartPhase(ImportPhase::BUFFERS);

    // Run until completed.
    while (!Execute(0)) {
        if (pendingGatherTasks_) {
            std::unique_lock lock(gatherTasksLock_);
            condition_.wait(lock, [this]() { return pendingGatherTasks_ == finishedGatherTasks_.size(); });
        }
    }
}

void GLTF2Importer::ImportGLTFAsync(const IGLTFData& data, GltfResourceImportFlags flags, Listener* listener)
{
    Cancel();
    cancelled_ = false;

    data_ = (const GLTF2::Data*)&data;
    if (!data_) {
        CORE_LOG_E("invalid IGLTFData to ImportGLTFAsync");
        return;
    }

    flags_ = flags;

    result_.success = true;
    result_.error.clear();
    result_.data.samplers.clear();
    result_.data.images.clear();
    result_.data.textures.clear();
    result_.data.materials.clear();
    result_.data.meshes.clear();
    result_.data.skins.clear();
    result_.data.animations.clear();
    result_.data.specularRadianceCubemaps.clear();

    meshBuilders_.clear();

    listener_ = listener;

    // Build tasks.
    Prepare();

    // Fill in task queues for first tasks.
    StartPhase(ImportPhase::BUFFERS);
}

void GLTF2Importer::Cancel()
{
    if (!IsCompleted()) {
        cancelled_ = true;

        // Wait all ongoing threads to complete.
        for (size_t i = 0; i < gatherTaskResults_.size(); ++i) {
            gatherTaskResults_[i]->Wait();
        }

        StartPhase(ImportPhase::FINISHED);
    }
}

bool GLTF2Importer::IsCompleted() const
{
    return phase_ == ImportPhase::FINISHED;
}

const GLTFImportResult& GLTF2Importer::GetResult() const
{
    return result_;
}

const GltfMeshData& GLTF2Importer::GetMeshData() const
{
    return meshData_;
}

void GLTF2Importer::Prepare()
{
    // Build tasks.
    PrepareBufferTasks();

    if (flags_ & CORE_GLTF_IMPORT_RESOURCE_SAMPLER) {
        PrepareSamplerTasks();
    }

    if (flags_ & CORE_GLTF_IMPORT_RESOURCE_IMAGE) {
        PrepareImageTasks();
        PrepareImageBasedLightTasks();
    }

    if (flags_ & CORE_GLTF_IMPORT_RESOURCE_MATERIAL) {
        PrepareMaterialTasks();
    }

    if (flags_ & CORE_GLTF_IMPORT_RESOURCE_ANIMATION) {
        PrepareAnimationTasks();
    }

    if (flags_ & CORE_GLTF_IMPORT_RESOURCE_SKIN) {
        PrepareSkinTasks();
    }

    if (flags_ & CORE_GLTF_IMPORT_RESOURCE_MESH) {
        PrepareMeshTasks();
    }

    if (listener_) {
        listener_->OnImportStarted();
    }
}

void GLTF2Importer::QueueTask(unique_ptr<ImporterTask>&& task)
{
    size_t const taskIndex = tasks_.size();
    task->id = taskIndex;
    tasks_.push_back(move(task));
}

bool GLTF2Importer::ProgressTask(ImporterTask& task)
{
    if (task.state == ImporterTask::State::Queued) {
        // Task not started, proceed to gather phase.
        task.state = ImporterTask::State::Gather;
        if (task.gather) {
            pendingGatherTasks_++;
            gatherTaskResults_.push_back(
                threadPool_->Push(IThreadPool::ITask::Ptr { new GatherThreadTask(*this, task) }));
            return false;
        }
    }

    if (task.state == ImporterTask::State::Gather) {
        // Gather phase started or skipped, proceed to import.
        task.state = ImporterTask::State::Import;
        if (task.import) {
            pendingImportTasks_++;
            mainThreadQueue_->Submit(task.id, IThreadPool::ITask::Ptr { new ImportThreadTask(*this, task) });
            return false;
        }
    }

    if (task.state == ImporterTask::State::Import) {
        CompleteTask(task);
    }

    return task.state == ImporterTask::State::Finished;
}

void GLTF2Importer::Gather(ImporterTask& task)
{
    CORE_CPU_PERF_BEGIN(gather, "glTF", task.name, "Gather");

    if (cancelled_ || !task.gather()) {
        task.success = false;
    }

    CORE_CPU_PERF_END(gather);

    {
        // Mark task completed.
        std::lock_guard lock(gatherTasksLock_);
        finishedGatherTasks_.push_back(task.id);
        if (pendingGatherTasks_ == finishedGatherTasks_.size()) {
            condition_.notify_one();
        }
    }
}

void GLTF2Importer::Import(ImporterTask& task)
{
    CORE_CPU_PERF_BEGIN(import, "glTF", task.name, "Import");

    if (cancelled_ || !task.import()) {
        task.success = false;
    }

    CORE_CPU_PERF_END(import);
}

void GLTF2Importer::CompleteTask(ImporterTask& task)
{
    if (task.finished) {
        task.finished();
    }

    task.state = ImporterTask::State::Finished;

    completedTasks_++;

    if (listener_) {
        listener_->OnImportProgressed(completedTasks_, tasks_.size());
    }
}

void GLTF2Importer::StartPhase(ImportPhase phase)
{
    phase_ = phase;
    pendingGatherTasks_ = 0;
    pendingImportTasks_ = 0;
    completedTasks_ = 0;
    gatherTaskResults_.clear();

    if (phase == ImportPhase::FINISHED) {
        if ((flags_ & CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS) && !meshBuilders_.empty()) {
            {
                auto& shaderManager = renderContext_.GetDevice().GetShaderManager();
                const VertexInputDeclarationView vertexInputDeclaration =
                    shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
                        DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));
                meshData_.vertexInputDeclaration.bindingDescriptionCount =
                    static_cast<uint32_t>(vertexInputDeclaration.bindingDescriptions.size());
                meshData_.vertexInputDeclaration.attributeDescriptionCount =
                    static_cast<uint32_t>(vertexInputDeclaration.attributeDescriptions.size());
                std::copy(vertexInputDeclaration.bindingDescriptions.cbegin(),
                    vertexInputDeclaration.bindingDescriptions.cend(),
                    meshData_.vertexInputDeclaration.bindingDescriptions);
                std::copy(vertexInputDeclaration.attributeDescriptions.cbegin(),
                    vertexInputDeclaration.attributeDescriptions.cend(),
                    meshData_.vertexInputDeclaration.attributeDescriptions);
            }
            meshData_.meshes.resize(meshBuilders_.size());
            for (uint32_t mesh = 0U; mesh < meshBuilders_.size(); ++mesh) {
                if (!meshBuilders_[mesh]) {
                    continue;
                }
                auto& currentMesh = meshData_.meshes[mesh];
                auto vertexData = meshBuilders_[mesh]->GetVertexData();
                auto indexData = meshBuilders_[mesh]->GetIndexData();
                auto jointData = meshBuilders_[mesh]->GetJointData();
                auto submeshes = meshBuilders_[mesh]->GetSubmeshes();
                currentMesh.subMeshes.resize(submeshes.size());
                for (uint32_t subMesh = 0U; subMesh < submeshes.size(); ++subMesh) {
                    auto& currentSubMesh = currentMesh.subMeshes[subMesh];
                    currentSubMesh.indices = submeshes[subMesh].indexCount;
                    currentSubMesh.vertices = submeshes[subMesh].vertexCount;

                    currentSubMesh.indexBuffer = array_view(indexData.data() + submeshes[subMesh].indexBuffer.offset,
                        submeshes[subMesh].indexBuffer.byteSize);

                    auto& bufferAccess = submeshes[subMesh].bufferAccess;
                    auto fill = [](array_view<const uint8_t> data, const MeshComponent::Submesh::BufferAccess& buffer) {
                        if (buffer.offset > data.size() || buffer.offset + buffer.byteSize > data.size()) {
                            return array_view<const uint8_t> {};
                        }
                        return array_view(data.data() + buffer.offset, buffer.byteSize);
                    };
                    currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_POS] =
                        fill(vertexData, bufferAccess[MeshComponent::Submesh::DM_VB_POS]);
                    currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_NOR] =
                        fill(vertexData, bufferAccess[MeshComponent::Submesh::DM_VB_NOR]);
                    currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_UV0] =
                        fill(vertexData, bufferAccess[MeshComponent::Submesh::DM_VB_UV0]);
                    if (submeshes[subMesh].flags & MeshComponent::Submesh::SECOND_TEXCOORD_BIT) {
                        currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_UV1] =
                            fill(vertexData, bufferAccess[MeshComponent::Submesh::DM_VB_UV1]);
                    }
                    if (submeshes[subMesh].flags & MeshComponent::Submesh::TANGENTS_BIT) {
                        currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_TAN] =
                            fill(vertexData, bufferAccess[MeshComponent::Submesh::DM_VB_TAN]);
                    }
                    if (submeshes[subMesh].flags & MeshComponent::Submesh::VERTEX_COLORS_BIT) {
                        currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_COL] =
                            fill(vertexData, bufferAccess[MeshComponent::Submesh::DM_VB_COL]);
                    }
                    if (submeshes[subMesh].flags & MeshComponent::Submesh::SKIN_BIT) {
                        currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_JOI] =
                            fill(jointData, bufferAccess[MeshComponent::Submesh::DM_VB_JOI]);
                        currentSubMesh.attributeBuffers[MeshComponent::Submesh::DM_VB_JOW] =
                            fill(jointData, bufferAccess[MeshComponent::Submesh::DM_VB_JOW]);
                    }
                }
            }
        }
        // All tasks are done.
        if (listener_) {
            listener_->OnImportFinished();
        }
        tasks_.clear();
        return;
    }

    for (auto& task : tasks_) {
        if (task->state == ImporterTask::State::Finished) {
            continue;
        }

        if (task->phase != phase) {
            continue;
        }

        // This will turn queued tasks to running ones.
        ProgressTask(*task);
    }
}

GLTF2Importer::ImporterTask* GLTF2Importer::FindTaskById(uint64_t id)
{
    for (size_t i = 0; i < tasks_.size(); ++i) {
        if (tasks_[i]->id == id) {
            return tasks_[i].get();
        }
    }

    return nullptr;
}

bool GLTF2Importer::Execute(uint32_t timeBudget)
{
    if (IsCompleted()) {
        return true;
    }

    // Handle data gathering.
    HandleGatherTasks();

    // 'timeBudget' is given in microseconds
    const auto budget = std::chrono::microseconds(timeBudget);

    // Use steady_clock for measuring
    using Clock = std::chrono::steady_clock;

    // Start time.
    const auto s0 = Clock::now();

    // Handle resource / scene importing.
    while (pendingImportTasks_ > 0) {
        // Execute and handle one task.
        mainThreadQueue_->Execute();
        HandleImportTasks();

        // Check how much time has elapsed.
        if (timeBudget > 0) {
            const auto s1 = Clock::now();
            if (((s1 - s0) >= budget)) {
                // Break if whole time budget consumed.
                break;
            }
        }
    }

    // All tasks done for this phase?
    if (pendingGatherTasks_ == 0 && pendingImportTasks_ == 0) {
        // Proceed to next phase.
        StartPhase((ImportPhase)(phase_ + 1));
    }

    return IsCompleted();
}

void GLTF2Importer::HandleImportTasks()
{
    const vector<uint64_t> finishedImportTasks = mainThreadQueue_->CollectFinishedTasks();
    if (!finishedImportTasks.empty()) {
        pendingImportTasks_ -= finishedImportTasks.size();

        for (auto& finishedId : finishedImportTasks) {
            ImporterTask* task = FindTaskById(finishedId);
            if (task) {
                if (task->success) {
                    ProgressTask(*task);
                } else {
                    // Import phase failed.
                    const string error = "Import data failed: " + task->name;
                    CORE_LOG_W("%s", error.c_str());
                    result_.success = false;
                    result_.error += error + '\n';
                }
            }
        }
    }
}

void GLTF2Importer::HandleGatherTasks()
{
    vector<uint64_t> finishedGatherTasks;

    {
        std::lock_guard lock(gatherTasksLock_);
        if (pendingGatherTasks_ == finishedGatherTasks_.size()) {
            std::swap(finishedGatherTasks, finishedGatherTasks_);
        }
    }

    if (finishedGatherTasks.size() > 0) {
        for (auto& finishedId : finishedGatherTasks) {
            ImporterTask* task = FindTaskById(finishedId);
            if (task) {
                if (task->success) {
                    ProgressTask(*task);
                } else {
                    // Gather phase failed.
                    const string error = "Gather data failed: " + task->name;
                    CORE_LOG_W("%s", error.c_str());
                    result_.success = false;
                    result_.error += error + '\n';
                }
            }
        }

        pendingGatherTasks_ -= finishedGatherTasks.size();
    }
}

void GLTF2Importer::PrepareBufferTasks()
{
    // Buffers task.
    auto task = make_unique<ImporterTask>();
    task->name = "Load buffers";
    task->phase = ImportPhase::BUFFERS;
    task->gather = [this]() -> bool {
        BufferLoadResult result = LoadBuffers(*data_, engine_.GetFileManager());
        result_.success = result_.success && result.success;
        result_.error += result.error;
        return result_.success;
    };
    QueueTask(move(task));
}

void GLTF2Importer::PrepareSamplerTasks()
{
    for (size_t i = 0; i < data_->samplers.size(); ++i) {
        auto task = make_unique<ImporterTask>();
        task->name = "Import sampler";
        task->phase = ImportPhase::SAMPLERS;
        task->import = [this, i]() -> bool {
            auto const& sampler = data_->samplers[i];
            string const name = data_->defaultResources + "_sampler_" + to_string(i);

            GpuSamplerDesc desc;
            ConvertToCoreFilter(sampler->magFilter, desc.magFilter);
            ConvertToCoreFilter(sampler->minFilter, desc.minFilter, desc.mipMapMode);

            desc.minLod = 0.0f;
            desc.maxLod = 32.0f;

            desc.addressModeU = ConvertToCoreWrapMode(sampler->wrapS);
            desc.addressModeV = ConvertToCoreWrapMode(sampler->wrapT);
            desc.addressModeW = desc.addressModeU;

            EntityReference entity = ecs_->GetEntityManager().CreateReferenceCounted();
            gpuHandleManager_.Create(entity);
            gpuHandleManager_.Write(entity)->reference = gpuResourceManager_.Create(name, desc);
            result_.data.samplers.push_back(move(entity));

            return true;
        };

        QueueTask(move(task));
    }
}

void GLTF2Importer::QueueImage(size_t i, string&& uri, string&& name)
{
    struct ImageTaskData {
        GLTF2Importer* importer;
        size_t index;
        string uri;
        string name;
        IRenderDataStoreDefaultStaging* staging;
        RenderHandleReference imageHandle;
    };

    constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";

    auto staging = static_cast<IRenderDataStoreDefaultStaging*>(
        renderContext_.GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING.data()));
    // Does not exist, which means it needs to be imported.
    auto task = make_unique<GatheredDataTask<ImageTaskData>>();
    task->data = ImageTaskData { this, i, move(uri), move(name), staging, {} };
    task->name = "Import image";
    task->phase = ImportPhase::IMAGES;
    task->gather = [t = task.get()]() -> bool {
        GLTF2Importer* importer = t->data.importer;
        auto const& image = importer->data_->images[t->data.index];
        IImageLoaderManager::LoadResult result = GatherImageData(
            *image, *importer->data_, importer->engine_.GetFileManager(), importer->engine_.GetImageLoaderManager());
        if (result.success) {
            t->data.imageHandle = ImportTexture(move(result.image), *t->data.staging, importer->gpuResourceManager_);
        } else {
            CORE_LOG_W("Loading image '%s' failed: %s", image->uri.c_str(), result.error);
            importer->result_.error += result.error;
            importer->result_.error += '\n';
        }

        return true;
    };

    task->import = [t = task.get()]() mutable -> bool {
        if (t->data.imageHandle) {
            GLTF2Importer* importer = t->data.importer;
            auto imageEntity = importer->ecs_->GetEntityManager().CreateReferenceCounted();
            importer->gpuHandleManager_.Create(imageEntity);
            importer->gpuHandleManager_.Write(imageEntity)->reference = move(t->data.imageHandle);
            if (!t->data.uri.empty()) {
                importer->uriManager_.Create(imageEntity);
                importer->uriManager_.Write(imageEntity)->uri = move(t->data.uri);
            }
            if (!t->data.name.empty()) {
                importer->nameManager_.Create(imageEntity);
                importer->nameManager_.Write(imageEntity)->name = move(t->data.name);
            }

            importer->result_.data.images[t->data.index] = move(imageEntity);
        }

        return true;
    };

    task->finished = [t = task.get()]() {};

    QueueTask(move(task));
}

void GLTF2Importer::PrepareImageTasks()
{
    result_.data.images.resize(data_->images.size(), {});
    result_.data.textures.resize(data_->textures.size(), {});

    const bool skipUnusedResources = (flags_ & CORE_GLTF_IMPORT_RESOURCE_SKIP_UNUSED) != 0;

    vector<bool> imageLoadingRequred;
    imageLoadingRequred.resize(data_->images.size(), skipUnusedResources ? false : true);

    if (skipUnusedResources) {
        // Find image references from textures.
        ResolveReferencedImages(*data_, imageLoadingRequred);
    }

    // Tasks for image loading.
    for (size_t i = 0; i < data_->images.size(); ++i) {
        // Skip loading of this image if it is not referenced by textures.
        if (!imageLoadingRequred[i]) {
            continue;
        }

        string uri;
        string name = data_->defaultResources + "/images/" + to_string(i);

        auto const& image = data_->images[i];
        if (image->uri.empty() || GLTF2::IsDataURI(image->uri)) {
            // NOTE: Might need to figure out better way to reference embedded resources.
            uri = data_->filepath + "/" + name;
        } else {
            uri = data_->filepath + "/" + image->uri;
        }

        // See if this resource already exists.
        Entity imageEntity = LookupResourceByUri(uri, uriManager_, gpuHandleManager_);
        if (EntityUtil::IsValid(imageEntity)) {
            // Already exists.
            result_.data.images[i] = ecs_->GetEntityManager().GetReferenceCounted(imageEntity);
            CORE_LOG_D("Resource already exists, skipping ('%s')", uri.c_str());
            continue;
        }

        QueueImage(i, move(uri), move(name));
    }

    // Tasks assigning textures to images.
    for (size_t i = 0; i < data_->textures.size(); ++i) {
        auto task = make_unique<ImporterTask>();
        task->name = "Import texture";
        task->phase = ImportPhase::TEXTURES;
        task->import = [this, i]() -> bool {
            const GLTF2::Texture& texture = *(data_->textures[i]);

            const size_t index = FindIndex(data_->images, texture.image);
            if (index != GLTF_INVALID_INDEX) {
                result_.data.textures[i] = result_.data.images[index];
            }

            return true;
        };
        QueueTask(move(task));
    }
}

void GLTF2Importer::PrepareImageBasedLightTasks()
{
#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
    result_.data.specularRadianceCubemaps.resize(data_->imageBasedLights.size(), {});

    // Do nothing in case there are no image based lights.
    if (data_->imageBasedLights.empty()) {
        return;
    }

    for (size_t lightIndex = 0; lightIndex < data_->imageBasedLights.size(); ++lightIndex) {
        const auto& light = data_->imageBasedLights[lightIndex];

        // Create gather task that loads all cubemap faces for this light.
        auto task = make_unique<GatheredDataTask<RenderHandleReference>>();
        task->name = "Import specular radiance cubemaps";
        task->phase = ImportPhase::IMAGES;

        task->gather = [this, &light, t = task.get()]() -> bool {
            bool success = true;
            vector<IImageLoaderManager::LoadResult> mipLevels;
            // For all mip levels.
            for (const auto& mipLevel : light->specularImages) {
                // For all cube faces.
                for (const auto& cubeFace : mipLevel) {
                    // Get image for this cube face.
                    auto& image = data_->images[cubeFace];

                    // Load image.
                    auto loadResult = GatherImageData(*image, *data_, engine_.GetFileManager(),
                        engine_.GetImageLoaderManager(), IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT);
                    if (!loadResult.success) {
                        success = false;
                        CORE_LOG_W("Loading image '%s' failed: %s", image->uri.c_str(), loadResult.error);
                    }

                    mipLevels.push_back(move(loadResult));
                }
            }
            if (!mipLevels.empty()) {
                t->data = CreateCubemapFromImages(light->specularImageSize, mipLevels, gpuResourceManager_);
            }
            return success;
        };

        task->import = [this, lightIndex, t = task.get()]() -> bool {
            // Import specular cubemap image if needed.
            if (t->data) {
                EntityReference imageEntity = ecs_->GetEntityManager().CreateReferenceCounted();
                gpuHandleManager_.Create(imageEntity);
                gpuHandleManager_.Write(imageEntity)->reference = move(t->data);
                result_.data.specularRadianceCubemaps[lightIndex] = move(imageEntity);
            }

            return true;
        };

        QueueTask(move(task));
    }
#endif
}

void GLTF2Importer::PrepareMaterialTasks()
{
    result_.data.materials.resize(data_->materials.size(), {});

    auto task = make_unique<ImporterTask>();
    task->name = "Import material";
    task->phase = ImportPhase::MATERIALS;
    task->import = [this]() -> bool {
        for (size_t i = 0; i < data_->materials.size(); ++i) {
            const string uri = data_->filepath + "/" + data_->defaultResources + "/materials/" + to_string(i);
            const string_view name = data_->materials[i]->name;

            auto materialEntity = LookupResourceByUri(uri, uriManager_, materialManager_);
            if (EntityUtil::IsValid(materialEntity)) {
                CORE_LOG_D("Resource already exists, skipping ('%s')", uri.c_str());
            } else {
                // Does not exist, so needs to be imported.
                const auto& gltfMaterial = *data_->materials[i];
                if (gltfMaterial.type != GLTF2::Material::Type::TextureSheetAnimation) {
                    materialEntity = ecs_->GetEntityManager().Create();
                    materialManager_.Create(materialEntity);
                    if (!uri.empty()) {
                        uriManager_.Create(materialEntity);
                        uriManager_.Write(materialEntity)->uri = uri;
                    }
                    if (!name.empty()) {
                        nameManager_.Create(materialEntity);
                        nameManager_.Write(materialEntity)->name = name;
                    }

                    ImportMaterial(result_, *data_, gltfMaterial, materialEntity, materialManager_, gpuResourceManager_,
                        dmShaderData_);
                }
            }
            result_.data.materials[i] = ecs_->GetEntityManager().GetReferenceCounted(materialEntity);
        }

        return true;
    };

    QueueTask(move(task));
}

void GLTF2Importer::PrepareMeshTasks()
{
    result_.data.meshes.resize(data_->meshes.size(), {});
    if (flags_ & CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS) {
        meshBuilders_.resize(data_->meshes.size());
    }

    for (size_t i = 0; i < data_->meshes.size(); ++i) {
        string uri = data_->filepath + '/' + data_->defaultResources + "/meshes/" + to_string(i);
        const string_view name = data_->meshes[i]->name;

        // See if this resource already exists.
        const auto meshEntity = LookupResourceByUri(uri, uriManager_, meshManager_);
        if (EntityUtil::IsValid(meshEntity)) {
            // Already exists.
            result_.data.meshes[i] = ecs_->GetEntityManager().GetReferenceCounted(meshEntity);
            CORE_LOG_D("Resource already exists, skipping ('%s')", uri.c_str());
            continue;
        }

        auto task = make_unique<GatheredDataTask<GatherMeshDataResult>>();
        task->name = "Import mesh";
        task->phase = ImportPhase::MESHES;
        task->gather = [this, i, t = task.get()]() -> bool {
            const GLTF2::Mesh& mesh = *(data_->meshes[i]);

            // Gather mesh data.
            t->data = GatherMeshData(mesh, result_, flags_, materialManager_, device_, engine_);
            return t->data.success;
        };

        task->import = [this, i, uri = move(uri), name, t = task.get()]() mutable -> bool {
            if (t->data.success) {
                // Import mesh.
                auto meshEntity = ImportMesh(*ecs_, t->data);
                if (EntityUtil::IsValid(meshEntity)) {
                    if (!uri.empty()) {
                        uriManager_.Create(meshEntity);
                        uriManager_.Write(meshEntity)->uri = move(uri);
                    }
                    if (!name.empty()) {
                        nameManager_.Create(meshEntity);
                        nameManager_.Write(meshEntity)->name = name;
                    }

                    result_.data.meshes[i] = ecs_->GetEntityManager().GetReferenceCounted(meshEntity);
                    return true;
                }
            }

            return false;
        };

        task->finished = [this, i, t = task.get()]() {
            if (flags_ & CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS) {
                meshBuilders_[i] = BASE_NS::move(t->data.meshBuilder);
            } else {
                t->data.meshBuilder.reset();
            }
        };

        QueueTask(move(task));
    }
}

template<>
GLTF2Importer::GatheredDataTask<GLTF2Importer::ComponentTaskData<AnimationInputComponent>>*
GLTF2Importer::PrepareAnimationInputTask(
    unordered_map<Accessor*, GatheredDataTask<ComponentTaskData<AnimationInputComponent>>*>& inputs,
    const AnimationTrack& track, IAnimationInputComponentManager* animationInputManager)
{
    GLTF2Importer::GatheredDataTask<GLTF2Importer::ComponentTaskData<AnimationInputComponent>>* result = nullptr;
    if (auto pos = inputs.find(track.sampler->input); pos == inputs.end()) {
        auto task = make_unique<GatheredDataTask<ComponentTaskData<AnimationInputComponent>>>();
        task->name = "Import animation input";
        task->phase = ImportPhase::ANIMATION_SAMPLERS;
        task->gather = [this, accessor = track.sampler->input, t = task.get()]() -> bool {
            return BuildAnimationInput(*data_, engine_.GetFileManager(), result_, *accessor, t->data.component);
        };
        task->import = [em = &ecs_->GetEntityManager(), animationInputManager, t = task.get()]() -> bool {
            t->data.entity = em->CreateReferenceCounted();
            animationInputManager->Set(t->data.entity, t->data.component);
            return true;
        };
        inputs.insert({ track.sampler->input, task.get() });
        result = task.get();
        QueueTask(move(task));
    } else {
        result = pos->second;
    }
    return result;
}

template<>
GLTF2Importer::GatheredDataTask<GLTF2Importer::ComponentTaskData<AnimationOutputComponent>>*
GLTF2Importer::PrepareAnimationOutputTask(
    unordered_map<Accessor*, GatheredDataTask<ComponentTaskData<AnimationOutputComponent>>*>& outputs,
    const AnimationTrack& track, IAnimationOutputComponentManager* animationOutputManager)
{
    GatheredDataTask<ComponentTaskData<AnimationOutputComponent>>* result = nullptr;
    if (auto pos = outputs.find(track.sampler->output); pos == outputs.end()) {
        auto task = make_unique<GatheredDataTask<ComponentTaskData<AnimationOutputComponent>>>();
        task->name = "Import animation output";
        task->phase = ImportPhase::ANIMATION_SAMPLERS;
        task->gather = [this, accessor = track.sampler->output, path = track.channel.path, t = task.get()]() -> bool {
            return BuildAnimationOutput(*data_, engine_.GetFileManager(), result_, *accessor, path, t->data.component);
        };
        task->import = [em = &ecs_->GetEntityManager(), animationOutputManager, t = task.get()]() -> bool {
            t->data.entity = em->CreateReferenceCounted();
            animationOutputManager->Set(t->data.entity, t->data.component);
            return true;
        };
        outputs.insert({ track.sampler->output, task.get() });
        result = task.get();
        QueueTask(move(task));
    } else {
        result = pos->second;
    }
    return result;
}

void GLTF2Importer::PrepareAnimationTasks()
{
    auto animationManager = GetManager<IAnimationComponentManager>(*ecs_);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs_);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs_);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs_);

    result_.data.animations.resize(data_->animations.size(), {});

    unordered_map<Accessor*, GatheredDataTask<ComponentTaskData<AnimationInputComponent>>*> inputs;
    unordered_map<Accessor*, GatheredDataTask<ComponentTaskData<AnimationOutputComponent>>*> outputs;
    for (size_t i = 0; i < data_->animations.size(); i++) {
        const string uri = data_->filepath + '/' + data_->defaultResources + "/animations/" + to_string(i);
        const string_view name = data_->animations[i]->name;

        // See if this resource already exists.
        const auto animationEntity = LookupResourceByUri(uri, uriManager_, *animationManager);
        if (EntityUtil::IsValid(animationEntity)) {
            result_.data.animations[i] = ecs_->GetEntityManager().GetReferenceCounted(animationEntity);
            CORE_LOG_D("Resource already exists, skipping ('%s')", uri.c_str());
            continue;
        }
        vector<GatheredDataTask<ComponentTaskData<AnimationInputComponent>>*> inputResults;
        vector<GatheredDataTask<ComponentTaskData<AnimationOutputComponent>>*> outputResults;
        for (const auto& track : data_->animations[i]->tracks) {
            if (track.sampler && track.sampler->input && track.sampler->output) {
                inputResults.push_back(PrepareAnimationInputTask(inputs, track, animationInputManager));
                outputResults.push_back(PrepareAnimationOutputTask(outputs, track, animationOutputManager));
            }
        }

        auto task = make_unique<ImporterTask>();
        task->name = "Import animation";
        task->phase = ImportPhase::ANIMATIONS;
        task->import = AnimationTaskData { this, i, uri, name, animationManager, animationTrackManager,
            move(inputResults), move(outputResults) };
        task->finished = [t = task.get()]() { t->import = {}; };

        QueueTask(move(task));
    }
}

void GLTF2Importer::PrepareSkinTasks()
{
    result_.data.skins.resize(data_->skins.size(), {});
    if (auto* skinIbmManager = GetManager<ISkinIbmComponentManager>(*ecs_); skinIbmManager) {
        for (size_t i = 0; i < data_->skins.size(); i++) {
            string name = data_->defaultResources + "/skins/" + to_string(i);
            string uri = data_->filepath + '/' + name;

            // See if this resource already exists.
            const Entity skinIbmEntity = LookupResourceByUri(uri, uriManager_, *skinIbmManager);
            if (EntityUtil::IsValid(skinIbmEntity)) {
                // Already exists.
                result_.data.skins[i] = ecs_->GetEntityManager().GetReferenceCounted(skinIbmEntity);
                CORE_LOG_D("Resource already exists, skipping ('%s')", uri.c_str());
                continue;
            }

            auto task = make_unique<GatheredDataTask<SkinIbmComponent>>();
            task->name = "Import skin";
            task->phase = ImportPhase::SKINS;
            task->gather = [this, i, t = task.get()]() -> bool {
                return BuildSkinIbmComponent(*(data_->skins[i]), t->data);
            };

            task->import = [this, i, uri = move(uri), name = move(name), t = task.get(),
                               skinIbmManager]() mutable -> bool {
                if (!t->data.matrices.empty()) {
                    auto skinIbmEntity = ecs_->GetEntityManager().CreateReferenceCounted();

                    skinIbmManager->Create(skinIbmEntity);
                    {
                        auto skinIbmHandle = skinIbmManager->Write(skinIbmEntity);
                        *skinIbmHandle = move(t->data);
                    }

                    uriManager_.Create(skinIbmEntity);
                    uriManager_.Write(skinIbmEntity)->uri = move(uri);
                    nameManager_.Create(skinIbmEntity);
                    nameManager_.Write(skinIbmEntity)->name = move(name);
                    result_.data.skins[i] = move(skinIbmEntity);
                    return true;
                }

                return false;
            };

            task->finished = [t = task.get()]() { t->data = {}; };

            QueueTask(move(task));
        }
    }
}

void GLTF2Importer::Destroy()
{
    delete this;
}

Gltf2SceneImporter::Gltf2SceneImporter(IEngine& engine, IRenderContext& renderContext, IEcs& ecs)
    : ecs_(ecs), graphicsContext_(GetInstance<IGraphicsContext>(
                     *renderContext.GetInterface<IClassRegister>(), UID_GRAPHICS_CONTEXT)),
      importer_(new GLTF2::GLTF2Importer(engine, renderContext, ecs))
{}

Gltf2SceneImporter::Gltf2SceneImporter(IEngine& engine, IRenderContext& renderContext, IEcs& ecs, IThreadPool& pool)
    : ecs_(ecs), graphicsContext_(GetInstance<IGraphicsContext>(
                     *renderContext.GetInterface<IClassRegister>(), UID_GRAPHICS_CONTEXT)),
      importer_(new GLTF2::GLTF2Importer(engine, renderContext, ecs, pool))
{}

void Gltf2SceneImporter::ImportResources(const ISceneData::Ptr& data, ResourceImportFlags flags)
{
    data_ = {};
    listener_ = nullptr;
    result_.error = 1;
    if (auto sceneData = data->GetInterface<SceneData>()) {
        if (auto* gltfData = sceneData->GetData()) {
            importer_->ImportGLTF(*gltfData, flags);
            const auto& result = importer_->GetResult();
            result_.error = result.success ? 0 : 1;
            result_.message = result.error;
            result_.data.samplers = result.data.samplers;
            result_.data.images = result.data.images;
            result_.data.textures = result.data.textures;
            result_.data.materials = result.data.materials;
            result_.data.meshes = result.data.meshes;
            result_.data.skins = result.data.skins;
            result_.data.animations = result.data.animations;
            result_.data.specularRadianceCubemaps = result.data.specularRadianceCubemaps;
            if (!result_.error) {
                data_ = data;
            }
        }
    }
}

void Gltf2SceneImporter::ImportResources(
    const ISceneData::Ptr& data, ResourceImportFlags flags, ISceneImporter::Listener* listener)
{
    data_ = {};
    listener_ = nullptr;

    if (auto sceneData = data->GetInterface<SceneData>()) {
        if (auto* gltfData = sceneData->GetData()) {
            listener_ = listener;
            data_ = data;
            importer_->ImportGLTFAsync(*gltfData, flags, this);
        }
    }
}

bool Gltf2SceneImporter::Execute(uint32_t timeBudget)
{
    if (importer_->Execute(timeBudget)) {
        const auto& result = importer_->GetResult();
        result_.error = result.success ? 0 : 1;
        result_.message = result.error;
        result_.data.samplers = result.data.samplers;
        result_.data.images = result.data.images;
        result_.data.textures = result.data.textures;
        result_.data.materials = result.data.materials;
        result_.data.meshes = result.data.meshes;
        result_.data.skins = result.data.skins;
        result_.data.animations = result.data.animations;
        result_.data.specularRadianceCubemaps = result.data.specularRadianceCubemaps;
        const auto& meshData = importer_->GetMeshData();
        meshData_.meshes.resize(meshData.meshes.size());
        for (size_t i = 0U; i < meshData.meshes.size(); ++i) {
            auto& dstMesh = meshData_.meshes[i];
            const auto& srcMesh = meshData.meshes[i];
            dstMesh.subMeshes.resize(srcMesh.subMeshes.size());
            std::transform(srcMesh.subMeshes.cbegin(), srcMesh.subMeshes.cend(), dstMesh.subMeshes.begin(),
                [](const GltfMeshData::SubMesh& gltfSubmesh) {
                    MeshData::SubMesh submesh;
                    submesh.indices = gltfSubmesh.indices;
                    submesh.vertices = gltfSubmesh.vertices;
                    submesh.indexBuffer = gltfSubmesh.indexBuffer;
                    std::copy(std::begin(gltfSubmesh.attributeBuffers), std::end(gltfSubmesh.attributeBuffers),
                        std::begin(submesh.attributeBuffers));
                    return submesh;
                });
        }
        meshData_.vertexInputDeclaration = meshData.vertexInputDeclaration;
        return true;
    }
    return false;
}

void Gltf2SceneImporter::Cancel()
{
    importer_->Cancel();
}

bool Gltf2SceneImporter::IsCompleted() const
{
    return importer_->IsCompleted();
}

const ISceneImporter::Result& Gltf2SceneImporter::GetResult() const
{
    return result_;
}

const MeshData& Gltf2SceneImporter::GetMeshData() const
{
    return meshData_;
}

Entity Gltf2SceneImporter::ImportScene(size_t sceneIndex)
{
    return ImportScene(sceneIndex, {}, SceneImportFlagBits::CORE_IMPORT_COMPONENT_FLAG_BITS_ALL);
}

Entity Gltf2SceneImporter::ImportScene(size_t sceneIndex, SceneImportFlags flags)
{
    return ImportScene(sceneIndex, {}, flags);
}

Entity Gltf2SceneImporter::ImportScene(size_t sceneIndex, Entity parentEntity)
{
    return ImportScene(sceneIndex, parentEntity, SceneImportFlagBits::CORE_IMPORT_COMPONENT_FLAG_BITS_ALL);
}

Entity Gltf2SceneImporter::ImportScene(size_t sceneIndex, Entity parentEntity, SceneImportFlags flags)
{
    if (importer_ && data_) {
        if (auto sceneData = data_->GetInterface<SceneData>()) {
            if (auto* gltfData = sceneData->GetData()) {
                auto& gltf = graphicsContext_->GetGltf();
                return gltf.ImportGltfScene(
                    sceneIndex, *gltfData, importer_->GetResult().data, ecs_, parentEntity, flags);
            }
        }
    }
    return {};
}

// IInterface
const IInterface* Gltf2SceneImporter::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == ISceneImporter::UID) {
        return static_cast<const ISceneImporter*>(this);
    }
    if (uid == IInterface::UID) {
        return static_cast<const IInterface*>(this);
    }
    return nullptr;
}

IInterface* Gltf2SceneImporter::GetInterface(const BASE_NS::Uid& uid)
{
    if (uid == ISceneImporter::UID) {
        return static_cast<ISceneImporter*>(this);
    }
    if (uid == IInterface::UID) {
        return static_cast<IInterface*>(this);
    }
    return nullptr;
}

void Gltf2SceneImporter::Ref()
{
    ++refcnt_;
}

void Gltf2SceneImporter::Unref()
{
    if (--refcnt_ == 0) {
        delete this;
    }
}

void Gltf2SceneImporter::OnImportStarted()
{
    if (listener_) {
        listener_->OnImportStarted();
    }
}

void Gltf2SceneImporter::OnImportFinished()
{
    if (listener_) {
        listener_->OnImportFinished();
    }
}

void Gltf2SceneImporter::OnImportProgressed(size_t taskIndex, size_t taskCount)
{
    if (listener_) {
        listener_->OnImportProgressed(taskIndex, taskCount);
    }
}
} // namespace GLTF2

CORE3D_END_NAMESPACE()
