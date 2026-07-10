/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "custom_property_defaults.h"

#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/json/json.h>

#include <meta/interface/property/property.h>

SCENE_BEGIN_NAMESPACE()
namespace {

template<typename T>
void SetPropertyDefault(const META_NS::IProperty::Ptr& prop, T value)
{
    META_NS::Property<T> typed{prop};
    if (typed) {
        bool wasDefault = typed->IsDefaultValue();
        typed->SetDefaultValue(BASE_NS::move(value), wasDefault);
    }
}

template<typename VecType, typename ElementType, size_t N>
VecType VecFromJson(const CORE_NS::json::value& value)
{
    VecType result{};
    if (value.is_array()) {
        for (size_t i = 0; i < N && i < value.array_.size(); ++i) {
            result.data[i] = value.array_[i].template as_number<ElementType>();
        }
    }
    return result;
}

void SetCustomPropertyDefaultFromJson(
    const META_NS::IProperty::Ptr& prop, BASE_NS::string_view type, const CORE_NS::json::value& value)
{
    using namespace BASE_NS::Math;
    if (type == "vec4") {
        SetPropertyDefault(prop, VecFromJson<Vec4, float, 4>(value));
    } else if (type == "uvec4") {
        SetPropertyDefault(prop, VecFromJson<UVec4, uint32_t, 4>(value));
    } else if (type == "ivec4") {
        SetPropertyDefault(prop, VecFromJson<IVec4, int32_t, 4>(value));
    } else if (type == "vec3") {
        SetPropertyDefault(prop, VecFromJson<Vec3, float, 3>(value));
    } else if (type == "uvec3") {
        SetPropertyDefault(prop, VecFromJson<UVec3, uint32_t, 3>(value));
    } else if (type == "ivec3") {
        SetPropertyDefault(prop, VecFromJson<IVec3, int32_t, 3>(value));
    } else if (type == "vec2") {
        SetPropertyDefault(prop, VecFromJson<Vec2, float, 2>(value));
    } else if (type == "uvec2") {
        SetPropertyDefault(prop, VecFromJson<UVec2, uint32_t, 2>(value));
    } else if (type == "ivec2") {
        SetPropertyDefault(prop, VecFromJson<IVec2, int32_t, 2>(value));
    } else if (type == "float") {
        SetPropertyDefault<float>(prop, value.as_number<float>());
    } else if (type == "uint") {
        SetPropertyDefault<uint32_t>(prop, value.as_number<uint32_t>());
    } else if (type == "int") {
        SetPropertyDefault<int32_t>(prop, value.as_number<int32_t>());
    } else if (type == "bool") {
        SetPropertyDefault<bool>(prop, value.is_boolean() ? value.boolean_ : (value.as_number<uint32_t>() != 0));
    } else if (type == "mat3x3") {
        SetPropertyDefault(prop, VecFromJson<Mat3X3, float, 9>(value));
    } else if (type == "mat4x4") {
        SetPropertyDefault(prop, VecFromJson<Mat4X4, float, 16>(value));
    }
}

struct DataEntry {
    BASE_NS::string_view name;
    BASE_NS::string_view type;
    const CORE_NS::json::value* value{};
};

DataEntry ParseDataEntry(const CORE_NS::json::value& entry)
{
    DataEntry result;
    if (!entry.is_object()) {
        return result;
    }
    for (const auto& field : entry.object_) {
        if (field.key == "name" && field.value.is_string()) {
            result.name = field.value.string_;
        } else if (field.key == "type" && field.value.is_string()) {
            result.type = field.value.string_;
        } else if (field.key == "value") {
            result.value = &field.value;
        }
    }
    return result;
}

}  // namespace

const CORE_NS::json::value* FindMaterialComponentJson(const CORE_NS::json::value& metaJson)
{
    for (const auto& entry : metaJson.array_) {
        auto* nameVal = entry.find("name");
        if (nameVal && nameVal->is_string() && nameVal->string_ == "MaterialComponent") {
            return &entry;
        }
    }
    return nullptr;
}

const CORE_NS::json::value* FindCustomPropertiesJson(
    const RENDER_NS::IShaderManager& sman, const RENDER_NS::RenderHandleReference& shader)
{
    const auto* metaJson = sman.GetMaterialMetadata(shader);
    if (!metaJson || !metaJson->is_array()) {
        return nullptr;
    }
    const auto* material = FindMaterialComponentJson(*metaJson);
    if (!material) {
        return nullptr;
    }
    auto* propsJson = material->find("customProperties");
    if (propsJson && propsJson->is_array()) {
        return propsJson;
    }
    return nullptr;
}

void ApplyDataEntryDefaults(const META_NS::IMetadata::Ptr& meta, const CORE_NS::json::value& dataJson)
{
    for (const auto& dataEntry : dataJson.array_) {
        auto entry = ParseDataEntry(dataEntry);
        if (entry.name.empty() || entry.type.empty() || !entry.value) {
            break;
        }
        if (auto prop = meta->GetProperty(entry.name, META_NS::MetadataQuery::EXISTING)) {
            SetCustomPropertyDefaultFromJson(prop, entry.type, *entry.value);
        }
    }
}

void UpdateCustomPropertyDefaults(const META_NS::IMetadata::Ptr& meta, const CORE_NS::json::value& propsJson)
{
    for (const auto& propGroup : propsJson.array_) {
        if (!propGroup.is_object()) {
            break;
        }
        auto* dataJson = propGroup.find("data");
        if (!dataJson || !dataJson->is_array()) {
            break;
        }
        ApplyDataEntryDefaults(meta, *dataJson);
    }
}

SCENE_END_NAMESPACE()
