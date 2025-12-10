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

#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>

#include <meta/base/namespace.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/engine/internal_access.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()
namespace UTest {

struct EngineTestType {
    BASE_NS::Math::IVec2 vec;

    bool operator==(const EngineTestType& v) const
    {
        return vec == v.vec;
    }
    bool operator!=(const EngineTestType& v) const
    {
        return !(*this == v);
    }
};

enum class EngineTestEnum {
    MY_VALUE_1 = 1,
    MY_VALUE_2 = 3,
    MY_VALUE_3 = 5,
};

enum class EngineTestBitField {
    MY_VALUE_1 = 1,
    MY_VALUE_2 = 2,
    MY_VALUE_3 = 4,
    MY_VALUE_4 = 8,
};

} // namespace UTest

META_END_NAMESPACE()
CORE_BEGIN_NAMESPACE()

DECLARE_PROPERTY_TYPE(META_NS::UTest::EngineTestType);
DATA_TYPE_METADATA(META_NS::UTest::EngineTestType, MEMBER_PROPERTY(vec, "Vec", 0))
DECLARE_PROPERTY_TYPE(BASE_NS::vector<META_NS::UTest::EngineTestType>);

DECLARE_PROPERTY_TYPE(META_NS::UTest::EngineTestEnum);
DECLARE_PROPERTY_TYPE(META_NS::UTest::EngineTestBitField);
// clang-format off
ENUM_TYPE_METADATA(META_NS::UTest::EngineTestEnum
    , ENUM_VALUE(MY_VALUE_1, "Value 1")
    , ENUM_VALUE(MY_VALUE_2, "Value 2")
    , ENUM_VALUE(MY_VALUE_3, "Value 3"))

ENUM_TYPE_METADATA(META_NS::UTest::EngineTestBitField
    , ENUM_VALUE(MY_VALUE_1, "Value 1")
    , ENUM_VALUE(MY_VALUE_2, "Value 2")
    , ENUM_VALUE(MY_VALUE_3, "Value 3")
    , ENUM_VALUE(MY_VALUE_4, "Value 4"))

// clang-format on

CORE_END_NAMESPACE()
META_BEGIN_NAMESPACE()

META_TYPE(UTest::EngineTestType)
META_TYPE(UTest::EngineTestEnum)
META_TYPE(UTest::EngineTestBitField)

namespace UTest {
namespace prop1 {
struct EngineTestProp {
    int value {};
    BASE_NS::vector<float> floats;
    BASE_NS::Math::IVec2 vec2;
    EngineTestType type;
    uint32_t array[2] {};
    CORE_NS::IPropertyHandle* handle {};
};

constexpr size_t TEST_PROPERTY_COUNT = 6;
constexpr size_t TEST_RECURSIVE_PROPERTY_COUNT = 11;

PROPERTY_LIST(EngineTestProp, ENGINE_TESTPROP_METADATA, MEMBER_PROPERTY(value, "Value it is", 0), //
    MEMBER_PROPERTY(floats, "Floats", 0),                                                         //
    MEMBER_PROPERTY(vec2, "Vec2", 0),                                                             //
    MEMBER_PROPERTY(type, "Type", 0),                                                             //
    MEMBER_PROPERTY(array, "Array", 0),                                                           //
    MEMBER_PROPERTY(handle, "Handle", 0))

} // namespace prop1

namespace prop2 {
struct EngineTestProp {
    int value { 1 };
    uint32_t array[2] { 1, 1 };
    EngineTestType complex[3];
    BASE_NS::vector<EngineTestType> complexVec { EngineTestType { { 1, 2 } }, EngineTestType { { 8, 8 } } };
};

constexpr size_t TEST_PROPERTY_COUNT = 4;

PROPERTY_LIST(EngineTestProp, ENGINE_TESTPROP_METADATA, MEMBER_PROPERTY(value, "Value", 0), //
    MEMBER_PROPERTY(array, "Array", 0),                                                     //
    MEMBER_PROPERTY(complex, "Complex", 0),                                                 //
    MEMBER_PROPERTY(complexVec, "ComplexVec", 0))

} // namespace prop2

namespace prop3 {
struct EngineTestProp {
    EngineTestEnum enum1 = EngineTestEnum::MY_VALUE_1;
    EngineTestBitField enum2 = EngineTestBitField(0);
};

constexpr size_t TEST_PROPERTY_COUNT = 2;

PROPERTY_LIST(EngineTestProp, ENGINE_TESTPROP_METADATA, MEMBER_PROPERTY(enum1, "My Enum", 0),
    BITFIELD_MEMBER_PROPERTY(enum2, "My Bitfield", CORE_NS::PropertyFlags::IS_BITFIELD, EngineTestBitField))

} // namespace prop3

using EngineTestProp1Property = CORE_NS::PropertyApiImpl<prop1::EngineTestProp>;
using EngineTestProp2Property = CORE_NS::PropertyApiImpl<prop2::EngineTestProp>;
using EngineTestProp3Property = CORE_NS::PropertyApiImpl<prop3::EngineTestProp>;

class DummyRef : public CORE_NS::IEntityReferenceCounter {
public:
    int32_t GetRefCount() const noexcept override
    {
        return 1;
    }
    void Ref() noexcept override {}
    void Unref() noexcept override {}
};

template<typename Property>
class TestComponentManager final : public CORE_NS::IComponentManager {
public:
    CORE_NS::Entity ent { uint64_t(this) };
    DummyRef ref;
    CORE_NS::EntityReference entityRef { ent, CORE_NS::IEntityReferenceCounter::Ptr(&ref) };
    Property tprop;
    BASE_NS::unique_ptr<CORE_NS::PropertyApiImpl<Property>> property;

    TestComponentManager(BASE_NS::array_view<const CORE_NS::Property> prop)
        : property(new CORE_NS::PropertyApiImpl<Property>(&tprop, prop))
    {}

    BASE_NS::string_view GetName() const override
    {
        return "TestComponentManager";
    }
    BASE_NS::Uid GetUid() const override
    {
        return {};
    }
    size_t GetComponentCount() const override
    {
        return 1;
    }
    const CORE_NS::IPropertyApi& GetPropertyApi() const override
    {
        return *property;
    }
    CORE_NS::Entity GetEntity(IComponentManager::ComponentId index) const override
    {
        return index == 0 ? ent : CORE_NS::Entity {};
    }
    uint32_t GetComponentGeneration(IComponentManager::ComponentId index) const override
    {
        return 0;
    }
    bool HasComponent(CORE_NS::Entity entity) const override
    {
        return entity == ent;
    }
    IComponentManager::ComponentId GetComponentId(CORE_NS::Entity entity) const override
    {
        return entity == ent ? 0 : INVALID_COMPONENT_ID;
    }
    void Create(CORE_NS::Entity entity) override {}
    bool Destroy(CORE_NS::Entity entity) override
    {
        if (entity == ent) {
            property.reset();
        }
        return true;
    }
    void Gc() override {}
    void Destroy(BASE_NS::array_view<const CORE_NS::Entity> gcList) override {}
    BASE_NS::vector<CORE_NS::Entity> GetAddedComponents() override
    {
        return {};
    }
    BASE_NS::vector<CORE_NS::Entity> GetRemovedComponents() override
    {
        return {};
    }
    BASE_NS::vector<CORE_NS::Entity> GetUpdatedComponents() override
    {
        return {};
    }
    BASE_NS::vector<CORE_NS::Entity> GetMovedComponents() override
    {
        return {};
    }
    CORE_NS::ComponentManagerModifiedFlags GetModifiedFlags() const override
    {
        return 0;
    }
    void ClearModifiedFlags() override {}
    uint32_t GetGenerationCounter() const override
    {
        return 0;
    }
    void SetData(CORE_NS::Entity entity, const CORE_NS::IPropertyHandle& data) override {}
    const CORE_NS::IPropertyHandle* GetData(CORE_NS::Entity entity) const override
    {
        if (!property) {
            return nullptr;
        }
        return entity == ent ? property->GetData() : nullptr;
    }
    CORE_NS::IPropertyHandle* GetData(CORE_NS::Entity entity) override
    {
        if (!property) {
            return nullptr;
        }
        return entity == ent ? property->GetData() : nullptr;
    }
    void SetData(ComponentId index, const CORE_NS::IPropertyHandle& data) override {}
    const CORE_NS::IPropertyHandle* GetData(ComponentId index) const override
    {
        if (!property) {
            return nullptr;
        }
        return index == 0 ? property->GetData() : nullptr;
    }
    CORE_NS::IPropertyHandle* GetData(ComponentId index) override
    {
        if (!property) {
            return nullptr;
        }
        return index == 0 ? property->GetData() : nullptr;
    }
    CORE_NS::IEcs& GetEcs() const override
    {
        assert(false);
        abort();
        return *(CORE_NS::IEcs*)nullptr;
    }
};

inline void RegisterEngineTestType()
{
    auto& r = GetObjectRegistry();
    r.GetPropertyRegister().RegisterAny(CreateShared<DefaultAnyBuilder<Any<UTest::EngineTestType>>>());
    r.GetEngineData().RegisterInternalValueAccess(
        MetaType<UTest::EngineTestType>::coreType, CreateShared<EngineInternalValueAccess<UTest::EngineTestType>>());
    r.GetEngineData().RegisterInternalValueAccess(MetaType<UTest::EngineTestType[]>::coreType,
        CreateShared<EngineInternalArrayValueAccess<UTest::EngineTestType>>());

    r.GetEngineData().RegisterInternalValueAccess(MetaType<BASE_NS::vector<UTest::EngineTestType>>::coreType,
        CreateShared<EngineInternalArrayValueAccess<UTest::EngineTestType>>());

    r.GetEngineData().RegisterInternalValueAccess(
        MetaType<UTest::EngineTestEnum>::coreType, CreateShared<EngineInternalValueAccess<UTest::EngineTestEnum>>());
    r.GetEngineData().RegisterInternalValueAccess(MetaType<UTest::EngineTestBitField>::coreType,
        CreateShared<EngineInternalValueAccess<UTest::EngineTestBitField>>());
}

inline void UnregisterEngineTestType()
{
    auto& r = GetObjectRegistry();
    r.GetPropertyRegister().UnregisterAny(Any<UTest::EngineTestType>::StaticGetClassId());
    r.GetEngineData().UnregisterInternalValueAccess(MetaType<UTest::EngineTestType>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(MetaType<UTest::EngineTestType[]>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(MetaType<BASE_NS::vector<UTest::EngineTestType>>::coreType);

    r.GetEngineData().UnregisterInternalValueAccess(MetaType<UTest::EngineTestEnum>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(MetaType<UTest::EngineTestBitField>::coreType);
}

} // namespace UTest
META_END_NAMESPACE()
