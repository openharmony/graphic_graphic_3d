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
#include <PropertyTools/core_metadata.inl>

#include <core/property/scoped_handle.h>

#include <meta/ext/engine/internal_access.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

// clang-format off
using SingleAndArrayTypes = TypeList<
    bool,
    int8_t,
    int16_t,
    int32_t,
    int64_t,
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    float,
    double,
#ifdef __APPLE__
    size_t,
#endif
    BASE_NS::Math::Vec2,
    BASE_NS::Math::Vec3,
    BASE_NS::Math::Vec4,
    BASE_NS::Math::UVec2,
    BASE_NS::Math::UVec3,
    BASE_NS::Math::UVec4,
    BASE_NS::Math::IVec2,
    BASE_NS::Math::IVec3,
    BASE_NS::Math::IVec4,
    BASE_NS::Math::Quat,
    BASE_NS::Math::Mat3X3,
    BASE_NS::Math::Mat4X4,
    BASE_NS::Uid,
    CORE_NS::Entity,
    CORE_NS::EntityReference,
    BASE_NS::string,
    CORE_NS::IPropertyHandle*
    >;
using VectorTypes = TypeList<
    BASE_NS::vector<float>,
    BASE_NS::vector<BASE_NS::Math::Mat4X4>,
    BASE_NS::vector<CORE_NS::EntityReference>
    >;
// clang-format on

namespace Internal {
namespace {

template<bool ArrayTypes, typename... List>
void RegisterBasicEngineTypes(IEngineData& d, TypeList<List...>)
{
    (d.RegisterInternalValueAccess(MetaType<List>::coreType, CreateShared<EngineInternalValueAccess<List>>()), ...);
    if constexpr (ArrayTypes) {
        (d.RegisterInternalValueAccess(
             MetaType<List[]>::coreType, CreateShared<EngineInternalArrayValueAccess<List>>()),
            ...);
    }
}

template<bool ArrayTypes, typename... List>
void UnRegisterBasicEngineTypes(IEngineData& d, TypeList<List...>)
{
    (d.UnregisterInternalValueAccess(MetaType<List>::coreType), ...);
    if constexpr (ArrayTypes) {
        (d.UnregisterInternalValueAccess(MetaType<List[]>::coreType), ...);
    }
}
} // namespace

void RegisterEngineTypes(IObjectRegistry& registry)
{
    auto& pr = registry.GetEngineData();
    RegisterBasicEngineTypes<true>(pr, SingleAndArrayTypes {});
    RegisterBasicEngineTypes<false>(pr, VectorTypes {});
}

void UnRegisterEngineTypes(IObjectRegistry& registry)
{
    auto& pr = registry.GetEngineData();
    UnRegisterBasicEngineTypes<false>(pr, VectorTypes {});
    UnRegisterBasicEngineTypes<true>(pr, SingleAndArrayTypes {});
}

} // namespace Internal
META_END_NAMESPACE()
