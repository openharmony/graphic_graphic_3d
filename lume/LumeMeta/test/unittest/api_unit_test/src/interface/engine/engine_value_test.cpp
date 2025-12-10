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

// clang-format off
#include "engine_test_property.h"
// clang-format on

#include <test_framework.h>

#include <core/property/property_handle_util.h>

#include <meta/api/engine/util.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_object_registry.h>

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

struct InvalidType {};
META_TYPE(InvalidType);

META_BEGIN_NAMESPACE()
namespace UTest {

class API_EngineValueTest : public ::testing::Test {};

/**
 * @tc.name: Values
 * @tc.desc: Tests for Values. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineValueTest, Values, testing::ext::TestSize.Level1)
{
    // The purpose of this test case is to cover as many branches of CoreAny and ArrayCoreAny as possible

    auto& data = GetObjectRegistry().GetEngineData();

    auto accesses = data.GetAllRegisteredValueAccess();
    EXPECT_FALSE(accesses.empty());

    auto verifyClone = [](const IAny::Ptr& any) {
        ASSERT_TRUE(any);
        EXPECT_TRUE(any->Clone());
        EXPECT_TRUE(any->Clone(true));
        EXPECT_TRUE(any->Clone(false));
        EXPECT_TRUE(any->Clone(AnyCloneOptions {}));
        AnyCloneOptions opt;
        opt.value = CloneValueType::COPY_VALUE;
        opt.role = TypeIdRole::ITEM;
        EXPECT_TRUE(any->Clone(opt));
        opt.role = TypeIdRole::ARRAY;
        EXPECT_TRUE(any->Clone(opt));
        opt.value = CloneValueType::DEFAULT_VALUE;
        EXPECT_TRUE(any->Clone(opt));
    };

    // Go through all registered CoreAny/CoreArrayAny types and verify methods
    for (auto&& access : accesses) {
        auto type = BASE_NS::string(access.name);
        auto v = data.GetInternalValueAccess(access);
        ASSERT_TRUE(v);
        CORE_NS::Property p;
        p.type = access;
        auto any = v->CreateAny(p);
        auto serAny = v->CreateSerializableAny();
        ASSERT_TRUE(any);
        ASSERT_TRUE(serAny);
        if (auto et = interface_cast<IEngineType>(any)) {
            EXPECT_EQ(et->GetTypeDecl(), access);
        }
        AnyCloneOptions opt;
        opt.value = CloneValueType::COPY_VALUE;
        opt.role = TypeIdRole::ITEM;
        verifyClone(any->Clone(opt));
        opt.role = TypeIdRole::ARRAY;
        verifyClone(any->Clone(opt));
    }
}

class PropertyHandle;

namespace Internal {
static PropertyHandle* CurrentTestingProperty {};
}

class PropertyHandle : public CORE_NS::IPropertyHandle {
public:
    PropertyHandle(const CORE_NS::PropertyTypeDecl& decl) : decl_(decl)
    {
        data_.resize(decl_.byteSize, 0);
        api_.size = [](uintptr_t container) { return Internal::CurrentTestingProperty->size_; };
        api_.resize = [](uintptr_t container, size_t newSize) {
            auto& ph = *Internal::CurrentTestingProperty;
            ph.size_ = newSize;
        };
        api_.erase = [](uintptr_t container, size_t index) {
            // Doesn't do anything but update internal size value
            auto& ph = *Internal::CurrentTestingProperty;
            ph.size_--;
        };
        api_.insert = [](uintptr_t container, size_t index) {
            // Doesn't do anything, returns the first (and only) value
            auto& ph = *Internal::CurrentTestingProperty;
            ph.size_++;
            return reinterpret_cast<uintptr_t>(&ph.data_[0]);
        };
        api_.get = [](uintptr_t container, size_t index) {
            // Doesn't do anything but return the first (and only) value
            auto& ph = *Internal::CurrentTestingProperty;
            return reinterpret_cast<uintptr_t>(&ph.data_[0]);
        };
    }
    const CORE_NS::IPropertyApi* Owner() const override
    {
        return nullptr;
    }

    size_t Size() const override
    {
        return data_.size();
    }
    const void* RLock() const override
    {
        return &data_[0];
    };
    void RUnlock() const override {};
    void* WLock() override
    {
        return &data_[0];
    };
    void WUnlock() override {};

    CORE_NS::ContainerApi* GetContainerApi()
    {
        return &api_;
    }

    size_t GetDataIndex(size_t index)
    {
        return index * decl_.byteSize;
    }

    const CORE_NS::PropertyTypeDecl& GetPropertyTypeDecl() const
    {
        return decl_;
    }

private:
    CORE_NS::ContainerApi api_;
    size_t size_ { 1 };
    BASE_NS::vector<uint8_t> data_;
    CORE_NS::PropertyTypeDecl decl_;
};

class EngineValueTestComponentManager final : public CORE_NS::IComponentManager {
public:
    EngineValueTestComponentManager(const CORE_NS::PropertyTypeDecl& decl)
    {
        handle_ = BASE_NS::make_unique<PropertyHandle>(decl);
        Internal::CurrentTestingProperty = handle_.get();
    }
    ~EngineValueTestComponentManager() override
    {
        Internal::CurrentTestingProperty = nullptr;
        handle_.reset();
    }
    BASE_NS::string_view GetName() const override
    {
        return "EngineValueTestComponentManager";
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
        assert(false);
        abort();
        return *(CORE_NS::IPropertyApi*)nullptr;
    }
    CORE_NS::Entity GetEntity(IComponentManager::ComponentId index) const override
    {
        return CORE_NS::Entity {};
    }
    uint32_t GetComponentGeneration(IComponentManager::ComponentId index) const override
    {
        return 0;
    }
    bool HasComponent(CORE_NS::Entity entity) const override
    {
        return true;
    }
    IComponentManager::ComponentId GetComponentId(CORE_NS::Entity entity) const override
    {
        return 0;
    }
    void Create(CORE_NS::Entity entity) override {}
    bool Destroy(CORE_NS::Entity entity) override
    {
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
        returnValidCounter_--;
        return returnValidCounter_ ? handle_.get() : nullptr;
    }
    CORE_NS::IPropertyHandle* GetData(CORE_NS::Entity entity) override
    {
        returnValidCounter_--;
        return returnValidCounter_ ? handle_.get() : nullptr;
    }
    void SetData(ComponentId index, const CORE_NS::IPropertyHandle& data) override {}
    const CORE_NS::IPropertyHandle* GetData(ComponentId index) const override
    {
        returnValidCounter_--;
        return returnValidCounter_ ? handle_.get() : nullptr;
    }
    CORE_NS::IPropertyHandle* GetData(ComponentId index) override
    {
        returnValidCounter_--;
        return returnValidCounter_ ? handle_.get() : nullptr;
    }
    CORE_NS::IEcs& GetEcs() const override
    {
        assert(false);
        abort();
        return *(CORE_NS::IEcs*)nullptr;
    }

public:
    PropertyHandle* GetPropertyHandle()
    {
        return handle_.get();
    }
    void SetReturnInvalidCount(int count)
    {
        returnValidCounter_ = count;
    }
    void ResetReturnInvalidCount()
    {
        returnValidCounter_ = std::numeric_limits<int>::max();
    }

private:
    BASE_NS::unique_ptr<PropertyHandle> handle_;
    mutable int returnValidCounter_ { std::numeric_limits<int>::max() };
};

/**
 * @tc.name: Access
 * @tc.desc: Tests for Access. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineValueTest, Access, testing::ext::TestSize.Level1)
{
    // The purpose of this test case is to cover as many branches of EngineInternalValueAccess and
    // EngineInternalArrayValueAccess as possible

    auto& data = GetObjectRegistry().GetEngineData();

    auto accesses = data.GetAllRegisteredValueAccess();
    EXPECT_FALSE(accesses.empty());

    auto testAccess = [](const IEngineInternalValueAccess::Ptr& access, const EnginePropertyParams& params,
                          EngineValueTestComponentManager& manager, const IAny::Ptr& any) {
        ASSERT_TRUE(any);
        access->SyncFromEngine(params, *any);
        access->SyncToEngine(*any, params);

        auto invalid = Any<InvalidType> {};
        access->SyncFromEngine(params, invalid);
        access->SyncToEngine(invalid, params);

        auto sync = [&](int failCounter) {
            manager.SetReturnInvalidCount(failCounter);
            access->SyncToEngine(*any, params);
            manager.SetReturnInvalidCount(failCounter);
            access->SyncToEngine(invalid, params);
            manager.SetReturnInvalidCount(failCounter);
            access->SyncFromEngine(params, *any);
            manager.SetReturnInvalidCount(failCounter);
            access->SyncFromEngine(params, invalid);
        };

        // Make SyncToEngine fail after it has first internally run successful SyncFromEngine
        sync(3);
        sync(2);
        sync(1);
        sync(0);
        manager.ResetReturnInvalidCount();
    };

    auto verifyAny = [&](const IEngineInternalValueAccess::Ptr& access, EngineValueTestComponentManager& manager,
                         std::function<IAny::Ptr(CORE_NS::Property&)>&& createAny) {
        auto* ph = manager.GetPropertyHandle();
        ASSERT_TRUE(ph);
        auto* ca = ph->GetContainerApi();
        ASSERT_TRUE(ca);
        const auto& decl = ph->GetPropertyTypeDecl();
        CORE_NS::Property property;
        property.type = decl;
        property.metaData.containerMethods = ca;
        property.count = 0;
        property.size = decl.byteSize;
        property.offset = 0;
        property.flags = 0;
        ca->property = property;

        auto any = createAny(property);

        EnginePropertyParams params;
        params.property = property;
        // Invalid handle
        testAccess(access, params, manager, any);
        // Set handle
        EnginePropertyHandle handle;
        handle.manager = &manager;
        handle.entity = CORE_NS::Entity { 1 }; // Any valid value is ok here
        params.handle = handle;
        testAccess(access, params, manager, any);
        // Set container methods
        params.containerMethods = ca;
        testAccess(access, params, manager, any);
        // Set size to 0
        ca->resize(0, 0);
        testAccess(access, params, manager, any);
    };

    // Go through all registered InternalValueAccess types and verify methods
    for (auto&& decl : accesses) {
        auto type = BASE_NS::string(decl.name);
        auto access = data.GetInternalValueAccess(decl);
        ASSERT_TRUE(access);

        EXPECT_TRUE(access->IsCompatible(decl)) << type.c_str();
        EXPECT_FALSE(access->IsCompatible({})) << type.c_str();

        // Create a dummy property for calling IInternalValueAccess methods
        auto manager = EngineValueTestComponentManager(decl);
        AnyCloneOptions cloneItem { CloneValueType::COPY_VALUE, TypeIdRole::ITEM };
        AnyCloneOptions cloneArray { CloneValueType::COPY_VALUE, TypeIdRole::ARRAY };

        IAny::Ptr baseAny;
        verifyAny(access, manager, [&](CORE_NS::Property& p) {
            baseAny = access->CreateAny(p);
            return baseAny;
        });
        ASSERT_TRUE(baseAny);
        verifyAny(access, manager, [&](CORE_NS::Property&) { return baseAny->Clone(cloneItem); });
        verifyAny(access, manager, [&](CORE_NS::Property&) { return baseAny->Clone(cloneArray); });
    }
}

} // namespace UTest
META_END_NAMESPACE()
