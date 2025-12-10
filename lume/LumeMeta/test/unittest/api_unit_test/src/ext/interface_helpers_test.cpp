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

#include <base/util/uid_util.h>

#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/object_macros.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {
namespace {

struct BaseA : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseA, "aefe1c98-1072-4867-8809-7fbea5887fa8")
};

struct BaseB : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseB, "916c2330-6096-41d8-b40a-2e04fe0b74cb")
};

struct BaseC : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseC, "c6dbe0a5-4201-454e-83d9-d944d20b52b6")
};

struct BaseD : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseD, "c891c511-9e80-4d31-8247-e74a2a2fc406")
};

struct BaseE3 : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseE3, "6fbdaf63-b0c6-45b3-acf4-5b15ac2059b1")
};

struct BaseE2 : public BaseE3 {
    META_INTERFACE(BaseE3, BaseE2, "d44b6a34-9a8a-4588-9265-415e3a8a6787")
};

struct BaseE1 : public BaseE2 {
    META_INTERFACE(BaseE2, BaseE1, "c1b8e289-878a-4fc5-b7f5-e5770f174244")
};

struct BaseE : public BaseE1 {
    META_INTERFACE(BaseE1, BaseE, "c99600c5-84d7-456e-8540-389804f7244c")
};

struct Class : IntroduceInterfaces<BaseA, BaseB, BaseC, BaseD, BaseE> {
    using MyBase = IntroduceInterfaces<BaseA, BaseB, BaseC, BaseD, BaseE>;
    using MyBase::GetInterface;

    template<typename T>
    T* Cast()
    {
        return static_cast<T*>(this);
    }
};

template<size_t S>
constexpr bool Contains(Internal::UIDArray<S> arr, const BASE_NS::Uid& uid)
{
    for (size_t i = 0; i != S; ++i) {
        if (arr.data[i].uid == uid) {
            return true;
        }
    }
    return false;
}

} // namespace

/**
 * @tc.name: IntroduceInterfaces
 * @tc.desc: Tests for Introduce Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_InterfaceHelpers, IntroduceInterfaces, testing::ext::TestSize.Level1)
{
    using CORE_NS::IInterface;
    Class c;
    EXPECT_TRUE(c.GetInterface(CORE_NS::IInterface::UID) == static_cast<IInterface*>(c.Cast<BaseA>()));
    EXPECT_TRUE(c.GetInterface(BaseA::UID) == static_cast<IInterface*>(c.Cast<BaseA>()));
    EXPECT_TRUE(c.GetInterface(BaseB::UID) == static_cast<IInterface*>(c.Cast<BaseB>()));
    EXPECT_TRUE(c.GetInterface(BaseC::UID) == static_cast<IInterface*>(c.Cast<BaseC>()));
    EXPECT_TRUE(c.GetInterface(BaseD::UID) == static_cast<IInterface*>(c.Cast<BaseD>()));
    EXPECT_TRUE(c.GetInterface(BaseE::UID) == static_cast<IInterface*>(c.Cast<BaseE>()));
    EXPECT_TRUE(c.GetInterface(BaseE1::UID) == static_cast<IInterface*>(c.Cast<BaseE1>()));
    EXPECT_TRUE(c.GetInterface(BaseE2::UID) == static_cast<IInterface*>(c.Cast<BaseE2>()));
    EXPECT_TRUE(c.GetInterface(BaseE3::UID) == static_cast<IInterface*>(c.Cast<BaseE3>()));
}

/**
 * @tc.name: GetInterfaces
 * @tc.desc: Tests for Get Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_InterfaceHelpers, GetInterfaces, testing::ext::TestSize.Level1)
{
    using GI = GetInterfacesImpl<void, BaseA, BaseB, BaseC, BaseD, BaseE>;
    static_assert(GI::SIZE == 8);

    constexpr auto arr = GetInterfaces<void, BaseA, BaseB, BaseC, BaseD, BaseE>();
    static_assert(sizeof(arr.data) / sizeof(arr.data[0]) == 8);

    static_assert(Contains(arr, BASE_NS::Uid { "aefe1c98-1072-4867-8809-7fbea5887fa8" }));
    static_assert(Contains(arr, BASE_NS::Uid { "916c2330-6096-41d8-b40a-2e04fe0b74cb" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c6dbe0a5-4201-454e-83d9-d944d20b52b6" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c891c511-9e80-4d31-8247-e74a2a2fc406" }));
    static_assert(Contains(arr, BASE_NS::Uid { "6fbdaf63-b0c6-45b3-acf4-5b15ac2059b1" }));
    static_assert(Contains(arr, BASE_NS::Uid { "d44b6a34-9a8a-4588-9265-415e3a8a6787" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c1b8e289-878a-4fc5-b7f5-e5770f174244" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c99600c5-84d7-456e-8540-389804f7244c" }));
}

struct IDervivedLifecycle : ILifecycle {
    META_INTERFACE(ILifecycle, IDervivedLifecycle, "c891f671-9e80-4d31-8247-e74a2a2fc406")

    bool Build(const IMetadata::Ptr& parameters) override
    {
        return true;
    }
    void SetInstanceId(InstanceId) override {}
    void Destroy() override {}
};

struct IOtherDervivedLifecycle : ILifecycle {
    META_INTERFACE(ILifecycle, IOtherDervivedLifecycle, "c981f671-9e80-4d31-8247-e74a2a2fc406")

    bool Build(const IMetadata::Ptr& parameters) override
    {
        return true;
    }
    void SetInstanceId(InstanceId) override {}
    void Destroy() override {}
};

struct LivelyClass : IntroduceInterfaces<BaseA, IDervivedLifecycle, IOtherDervivedLifecycle, BaseD, BaseE> {
    using MyBase = IntroduceInterfaces<BaseA, IDervivedLifecycle, IOtherDervivedLifecycle, BaseD, BaseE>;
    using MyBase::GetInterface;
};

/**
 * @tc.name: ILifecycleSupport
 * @tc.desc: Tests for Ilifecycle Support. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_InterfaceHelpers, ILifecycleSupport, testing::ext::TestSize.Level1)
{
    static_assert(HAS_ILIFECYCLE<BaseA, BaseB, ILifecycle, BaseC>);
    static_assert(HAS_ILIFECYCLE<BaseA, BaseB, IDervivedLifecycle, BaseC>);
    static_assert(HAS_ILIFECYCLE<BaseA, IDervivedLifecycle, IDervivedLifecycle, BaseC>);
    static_assert(HAS_ILIFECYCLE<BaseA, IOtherDervivedLifecycle, IDervivedLifecycle, BaseC>);

    LivelyClass c;
    EXPECT_TRUE(c.GetInterface(ILifecycle::UID));
    EXPECT_TRUE(c.GetInterface(IDervivedLifecycle::UID));
}

struct BaseF : CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseF, "23f2cf2e-7052-4037-b2ca-c0164918f1e1")
};

struct BaseG : CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseG, "41a6cfd4-55dc-4372-8ec1-7ca68fdbcafe")
};

struct BaseH : CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, BaseH, "920e72e2-abd5-4322-ba35-2a767db8ae89")
};

struct AnotherClass : IntroduceInterfaces<BaseF, BaseG> {};

struct TopClass : IntroduceInterfaces<Class, BaseH, AnotherClass> {
    using MyBase = IntroduceInterfaces<Class, BaseH, AnotherClass>;
    using MyBase::GetInterface;

    template<typename T>
    T* Cast()
    {
        return static_cast<T*>(this);
    }
};

/**
 * @tc.name: EmbeddedIntroduceInterfacesSupport
 * @tc.desc: Tests for Embedded Introduce Interfaces Support. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_InterfaceHelpers, EmbeddedIntroduceInterfacesSupport, testing::ext::TestSize.Level1)
{
    using GI = GetInterfacesImpl<TopClass, Class, BaseH, AnotherClass>;
    static_assert(GI::SIZE == 11);

    constexpr auto arr = GetInterfaces<TopClass, Class, BaseH, AnotherClass>();
    static_assert(sizeof(arr.data) / sizeof(arr.data[0]) == 11);

    static_assert(Contains(arr, BASE_NS::Uid { "aefe1c98-1072-4867-8809-7fbea5887fa8" }));
    static_assert(Contains(arr, BASE_NS::Uid { "916c2330-6096-41d8-b40a-2e04fe0b74cb" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c6dbe0a5-4201-454e-83d9-d944d20b52b6" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c891c511-9e80-4d31-8247-e74a2a2fc406" }));
    static_assert(Contains(arr, BASE_NS::Uid { "6fbdaf63-b0c6-45b3-acf4-5b15ac2059b1" }));
    static_assert(Contains(arr, BASE_NS::Uid { "d44b6a34-9a8a-4588-9265-415e3a8a6787" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c1b8e289-878a-4fc5-b7f5-e5770f174244" }));
    static_assert(Contains(arr, BASE_NS::Uid { "c99600c5-84d7-456e-8540-389804f7244c" }));
    static_assert(Contains(arr, BASE_NS::Uid { "23f2cf2e-7052-4037-b2ca-c0164918f1e1" }));
    static_assert(Contains(arr, BASE_NS::Uid { "41a6cfd4-55dc-4372-8ec1-7ca68fdbcafe" }));
    static_assert(Contains(arr, BASE_NS::Uid { "920e72e2-abd5-4322-ba35-2a767db8ae89" }));
}

/**
 * @tc.name: EmbeddedIntroduceInterfacesSupportGetInterface
 * @tc.desc: Tests for Embedded Introduce Interfaces Support Get Interface. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_InterfaceHelpers, EmbeddedIntroduceInterfacesSupportGetInterface, testing::ext::TestSize.Level1)
{
    using CORE_NS::IInterface;
    TopClass c;
    EXPECT_TRUE(c.GetInterface(CORE_NS::IInterface::UID) == static_cast<IInterface*>(c.Cast<BaseA>()));
    EXPECT_TRUE(c.GetInterface(BaseA::UID) == static_cast<IInterface*>(c.Cast<BaseA>()));
    EXPECT_TRUE(c.GetInterface(BaseB::UID) == static_cast<IInterface*>(c.Cast<BaseB>()));
    EXPECT_TRUE(c.GetInterface(BaseC::UID) == static_cast<IInterface*>(c.Cast<BaseC>()));
    EXPECT_TRUE(c.GetInterface(BaseD::UID) == static_cast<IInterface*>(c.Cast<BaseD>()));
    EXPECT_TRUE(c.GetInterface(BaseE::UID) == static_cast<IInterface*>(c.Cast<BaseE>()));
    EXPECT_TRUE(c.GetInterface(BaseE1::UID) == static_cast<IInterface*>(c.Cast<BaseE1>()));
    EXPECT_TRUE(c.GetInterface(BaseE2::UID) == static_cast<IInterface*>(c.Cast<BaseE2>()));
    EXPECT_TRUE(c.GetInterface(BaseE3::UID) == static_cast<IInterface*>(c.Cast<BaseE3>()));
    EXPECT_TRUE(c.GetInterface(BaseF::UID) == static_cast<IInterface*>(c.Cast<BaseF>()));
    EXPECT_TRUE(c.GetInterface(BaseG::UID) == static_cast<IInterface*>(c.Cast<BaseG>()));
    EXPECT_TRUE(c.GetInterface(BaseH::UID) == static_cast<IInterface*>(c.Cast<BaseH>()));
}

} // namespace UTest
META_END_NAMESPACE()
