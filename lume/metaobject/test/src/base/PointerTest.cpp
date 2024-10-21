/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include <meta/interface/object_macros.h>

#include "src/test_runner.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class PointerTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }
    void SetUp() override {}
    void TearDown() override {}
};

namespace {
struct PtrTestData {
    PtrTestData(int& constructCount, int& destructCount)
        : constructCount(constructCount), destructCount(destructCount)
    {
        constructCount++;
    }
    ~PtrTestData()
    {
        destructCount++;
    }

    int data { 0 };
    int& constructCount;
    int& destructCount;

    static constexpr int dataValue = 42;
};
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test SharedPtr function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, SharedPtr, TestSize.Level1)
{
    int constructCount = 0, destructCount = 0;
    ASSERT_NE(ptr.get(), nullptr);

    EXPECT_EQ(ptr->data, 0);
    EXPECT_EQ(constructCount, 1);

    EXPECT_EQ(constructCount, 2);
    ptr2->data = PtrTestData::dataValue;
    EXPECT_NE(ptr->data, ptr2->data);

    EXPECT_EQ(destructCount, 0);

    ptr = ptr2;
    EXPECT_EQ(destructCount, 1);
    EXPECT_EQ(ptr->data, ptr2->data);
    ptr.reset();
    ptr2.reset();
    EXPECT_EQ(destructCount, 2);
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test WeakPtr function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, WeakPtr, TestSize.Level1)
{
    int constructCount = 0, destructCount = 0;
    ptr->data = PtrTestData::dataValue;
    EXPECT_EQ(ptr->data, PtrTestData::dataValue);
    EXPECT_EQ(constructCount, 1);

    BASE_NS::weak_ptr<PtrTestData> wptr(ptr);
    EXPECT_EQ(constructCount, 1);

    {
        auto p = wptr.lock();
        ASSERT_NE(p.get(), nullptr);
        EXPECT_EQ(constructCount, 1);
        EXPECT_EQ(p->data, PtrTestData::dataValue);
    }

    ptr.reset();
    EXPECT_EQ(destructCount, 1);

    auto p = wptr.lock();
    EXPECT_EQ(p.get(), nullptr);
}

class IBase : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "00000000-0000-0000-0000-000000000001" };
    virtual void AmBase() = 0;
    virtual void Identify() = 0;
};

class IBase2 : public IBase {
public:
    static constexpr BASE_NS::Uid UID { "00000000-0000-0000-0000-000000000002" };
    virtual uint32_t NonConstCount() const = 0;
    virtual uint32_t ConstCount() const = 0;
    virtual void B2() = 0;
    virtual void DoIt() const = 0;
    virtual void DoIt() = 0;

    virtual void NonConstOnlyDoIt() = 0;
    virtual void ConstOnlyDoIt() const = 0;
};

class IBase3 : public IBase {
public:
    static constexpr BASE_NS::Uid UID { "00000000-0000-0000-0000-000000000003" };
    virtual void B3() = 0;
    virtual void Identify() const = 0;
    virtual void Narf() const = 0;
    using IBase::Identify;
};

class DualBase : private IBase2, private IBase3 {
public:
    static BASE_NS::shared_ptr<CORE_NS::IInterface> Construct()
    {
        return interface_pointer_cast<CORE_NS::IInterface>(ptr);
    }
    mutable int ncc = 0;
    mutable int cc = 0;
    void Narf() const override
    {
        printf("Narf");
    }
    uint32_t NonConstCount() const override
    {
        return ncc;
    }
    uint32_t ConstCount() const override
    {
        return cc;
    }
    void NonConstOnlyDoIt() override
    {
        ncc++;
        printf("Non Const Only DoIt\n");
    }
    void ConstOnlyDoIt() const override
    {
        cc++;
        printf("Const Only DoIt\n");
    }
    void DoIt() const override
    {
        cc++;
        printf("Const DoIt\n");
    }
    void DoIt() override
    {
        ncc++;
        printf("Non const DoIt\n");
    }
    void AmBase() override
    {
        printf("DualBase %p\n", this);
    }
    void Identify() const override
    {
        printf("I am CONST DualBase %p\n", this);
    }
    void Identify() override
    {
        printf("I am DualBase %p\n", this);
    }
    void B2() override
    {
        printf("DualBase (B2) %p\n", this);
    }
    void B3() override
    {
        printf("DualBase (B3) %p\n", this);
    }

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        return const_cast<DualBase*>(this)->GetInterface(uid);
    }

    IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if (uid == IInterface::UID) {
            return static_cast<IBase2*>(this);
        }
        if (uid == IBase::UID) {
            return static_cast<IBase2*>(this);
        }
        if (uid == IBase2::UID) {
            return static_cast<IBase2*>(this);
        }
        if (uid == IBase3::UID) {
            return static_cast<IBase3*>(this);
        }
        return nullptr;
    }
    META_IMPLEMENT_REF_COUNT()
};

/**
 * @tc.name: PointerTest
 * @tc.desc: test AliasTest function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, AliasTest, TestSize.Level1)
{
    {
        IBase2* ap = (IBase2*)dd->GetInterface(IBase2::UID);
        IBase3* bp = (IBase3*)dd->GetInterface(IBase3::UID);
        BASE_NS::shared_ptr<IBase> orig(ap);
        {
            BASE_NS::shared_ptr<IBase> alias(orig, bp);
        }
    }
    {
        BASE_NS::shared_ptr b3(interface_pointer_cast<IBase3>(t));
        BASE_NS::shared_ptr b2(interface_pointer_cast<IBase2>(t));

        EXPECT_TRUE(t == b3);
        EXPECT_TRUE((void*)b3.get() != (void*)b2.get());

        BASE_NS::shared_ptr ib3(interface_pointer_cast<CORE_NS::IInterface>(t));
        BASE_NS::shared_ptr ib2(interface_pointer_cast<CORE_NS::IInterface>(t));

        EXPECT_TRUE(ib2 == b2);
        EXPECT_TRUE(ib3 == b3);
        EXPECT_TRUE(ib3 == ib2);
        EXPECT_TRUE((void*)ib3.get() == (void*)ib2.get());
    }

    BASE_NS::weak_ptr<IBase> wp2;
    {
        BASE_NS::shared_ptr<IBase3> b3(interface_pointer_cast<IBase3>(t));
        wp2 = b3;
        ASSERT_TRUE(wp2.lock());
        ASSERT_FALSE(wp2.Compare(b3));
        ASSERT_TRUE(wp2.lock() == b3);
        ASSERT_TRUE(b3 == interface_pointer_cast<IBase3>(wp2.lock()));
        b3.reset();
        ASSERT_TRUE(wp2.lock());
        ASSERT_FALSE(wp2.Compare(b3));
        ASSERT_FALSE(b3 == interface_pointer_cast<IBase3>(wp2.lock()));
    }

    BASE_NS::weak_ptr wp(t);
    ASSERT_TRUE(wp.lock());
    ASSERT_TRUE(wp.Compare(t));
    ASSERT_TRUE(t == wp.lock());

    ASSERT_TRUE(wp2.lock());
    ASSERT_TRUE(wp2.lock() == t);
    ASSERT_TRUE(t == wp2.lock());

    t.reset();

    ASSERT_FALSE(wp.Compare(t));
    ASSERT_TRUE(t == wp.lock());

    ASSERT_FALSE(wp.lock());
    ASSERT_TRUE(wp.lock() == nullptr);
    ASSERT_TRUE(t == nullptr);

    ASSERT_TRUE(wp2.lock() == t);
    ASSERT_TRUE(t == wp2.lock());

    ASSERT_FALSE(wp2.lock());
    ASSERT_TRUE(wp2.lock() == nullptr);
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test NullTests function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, NullTests, TestSize.Level1)
{
    int constructCount = 0, destructCount = 0;
    BASE_NS::weak_ptr<PtrTestData> ptr2 = ptr;
    BASE_NS::weak_ptr<PtrTestData> ptr22 = ptr;
    ASSERT_FALSE(ptr.get() == nullptr);
    ASSERT_FALSE(ptr == nullptr);

    ASSERT_FALSE(ptr2.lock().get() == nullptr);
    ASSERT_FALSE(ptr2.lock() == nullptr);

    ASSERT_TRUE(ptr.get() != nullptr);
    ASSERT_TRUE(ptr != nullptr);

    ASSERT_TRUE(ptr2.lock().get() != nullptr);
    ASSERT_TRUE(ptr2.lock() != nullptr);

    ptr.reset();

    ASSERT_FALSE(ptr.get() != nullptr);
    ASSERT_FALSE(ptr != nullptr);

    ASSERT_FALSE(ptr2.lock().get() != nullptr);
    ASSERT_FALSE(ptr2.lock() != nullptr);

    ASSERT_TRUE(ptr.get() == nullptr);
    ASSERT_TRUE(ptr == nullptr);

    ASSERT_TRUE(ptr2.lock().get() == nullptr);
    ASSERT_TRUE(ptr2.lock() == nullptr);

    BASE_NS::shared_ptr<PtrTestData> ptr3;
    BASE_NS::weak_ptr<PtrTestData> ptr4;

    ASSERT_TRUE(ptr3 == ptr4.lock());
    ASSERT_TRUE(ptr4.Compare(ptr3));

    ASSERT_FALSE(ptr4.Compare(ptr2));
    ASSERT_TRUE(ptr3 == ptr);

    ASSERT_TRUE(ptr2.Compare(ptr22));

    {
        BASE_NS::weak_ptr<PtrTestData> empty;
        BASE_NS::weak_ptr<PtrTestData> ptrAW = ptrA;

        BASE_NS::weak_ptr<PtrTestData> ptrBW = ptrB;

        ptrA.reset();
        ptrB.reset();

        ASSERT_FALSE(ptrAW.Compare(empty));
        ASSERT_FALSE(ptrAW.Compare(ptrBW));
        ASSERT_TRUE(ptrAW.lock() == ptrBW.lock());
        ASSERT_TRUE(empty.lock() == ptrBW.lock());
    }
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test AliasTest2 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, AliasTest2, TestSize.Level1)
{
    BASE_NS::shared_ptr<CORE_NS::IInterface> t((IBase*)(db)->GetInterface(CORE_NS::IInterface::UID));
    BASE_NS::shared_ptr ib1(interface_pointer_cast<CORE_NS::IInterface>(t));
    BASE_NS::shared_ptr ib2(interface_pointer_cast<IBase>(t));
    BASE_NS::shared_ptr ib3(interface_pointer_cast<IBase2>(t));
    BASE_NS::shared_ptr ib4(interface_pointer_cast<IBase3>(t));

    ASSERT_TRUE(t == ib1);
    ASSERT_TRUE(t == ib2);
    ASSERT_TRUE(t == ib3);
    ASSERT_TRUE(t == ib4);
    ASSERT_TRUE(ib2 == ib3);
    ASSERT_TRUE(ib2 == ib4);

    ib4 = nullptr;
    t = nullptr;
    ib1 = nullptr;
    ib2 = nullptr;
    ib3 = nullptr;
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test WeakTest2 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, WeakTest2, TestSize.Level1)
{
    BASE_NS::shared_ptr<CORE_NS::IInterface> t((IBase*)(db)->GetInterface(CORE_NS::IInterface::UID));
    BASE_NS::shared_ptr ib1(interface_pointer_cast<CORE_NS::IInterface>(t));
    BASE_NS::shared_ptr ib2(interface_pointer_cast<IBase>(t));
    BASE_NS::shared_ptr ib3(interface_pointer_cast<IBase2>(t));
    BASE_NS::shared_ptr ib4(interface_pointer_cast<IBase3>(t));

    BASE_NS::weak_ptr wt(t);
    BASE_NS::weak_ptr wb1(ib1);
    BASE_NS::weak_ptr wb2(ib2);
    BASE_NS::weak_ptr wb3(ib3);
    BASE_NS::weak_ptr wb4(ib4);

    ASSERT_TRUE(wt.Compare(t));
    ASSERT_TRUE(wt.Compare(wb1));
    ASSERT_TRUE(wt.Compare(wb2));
    ASSERT_TRUE(wt.Compare(wb3));
    ASSERT_FALSE(wt.Compare(wb4));
    ASSERT_TRUE(wt.CompareOwner(wb4));
    ASSERT_TRUE(wb2.Compare(wb3));

    ASSERT_TRUE(wb2.CompareOwner(wb4));
    ASSERT_FALSE(wb2.Compare(wb4));

    ASSERT_FALSE(wt.Compare(wb4));
    ASSERT_TRUE(wt.CompareOwner(wb4));
    ASSERT_TRUE(wt.lock() == wb4.lock());

    t = nullptr;

    ASSERT_FALSE(wt.Compare(t));
    ASSERT_TRUE(wt.lock());
    ASSERT_TRUE(t == nullptr);

    ASSERT_TRUE(wt.Compare(wb1));
    ASSERT_TRUE(wt.Compare(wb2));
    ASSERT_TRUE(wt.Compare(wb3));

    ib1 = nullptr;
    ib2 = nullptr;
    ib3 = nullptr;

    ASSERT_TRUE(wt.Compare(wb1));
    ASSERT_TRUE(wt.Compare(wb2));
    ASSERT_TRUE(wt.Compare(wb3));
    ASSERT_FALSE(wt.Compare(wb4));
    ASSERT_TRUE(wt.CompareOwner(wb4));
    ASSERT_TRUE(wb2.Compare(wb3));
    ASSERT_TRUE(wb2.CompareOwner(wb4));
    ASSERT_FALSE(wb2.Compare(wb4));

    wt = nullptr;
    wb1 = nullptr;
    wb2 = nullptr;
    wb3 = nullptr;

    ASSERT_TRUE(wt.lock() == nullptr);
    ASSERT_TRUE(wb1.lock() == nullptr);
    ASSERT_TRUE(wb2.lock() == nullptr);
    ASSERT_TRUE(wb3.lock() == nullptr);
    ASSERT_TRUE(wb2.Compare(wb3));
    ASSERT_TRUE(!wb2.Compare(wb4));

    auto* ptr = ib4.get();
    auto* ptr2 = wb4.lock().get();

    ib4 = nullptr;

    ASSERT_TRUE(wb4.lock() == nullptr);
    ASSERT_FALSE(wt.Compare(wb4));
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test ImplicitConversions function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, ImplicitConversions, TestSize.Level1)
{
    auto db = DualBase::Construct();
    BASE_NS::shared_ptr<IBase3> ib4(interface_pointer_cast<IBase3>(db));
    BASE_NS::shared_ptr<CORE_NS::IInterface> a(ib4);

    ASSERT_TRUE(db.get() != ib4.get());
    ASSERT_TRUE(db.get() == a.get());

    BASE_NS::weak_ptr<CORE_NS::IInterface> aa(ib4);

    BASE_NS::weak_ptr<CORE_NS::IInterface> b;
    b = ib4;

    BASE_NS::weak_ptr<IBase3>  ba(ib4);

    BASE_NS::weak_ptr<CORE_NS::IInterface> d(ba);

    BASE_NS::weak_ptr<CORE_NS::IInterface> dd;
    dd = ba;

    BASE_NS::weak_ptr<CORE_NS::IInterface> c(BASE_NS::move(ba));
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test CusTomDeleter function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
int g_customDeleterCalled = 0;
HWTEST_F(PointerTest, CustomDeleter, TestSize.Level1)
{
    int value = 42;
    auto deleter = [](void* p) {
        g_customDeleterCalled++;
        printf("Last reference for %p gone\n", p);
    };
    {
        BASE_NS::shared_ptr<int> sb2;
        {
            BASE_NS::shared_ptr<int> custom(&value, deleter);
            BASE_NS::weak_ptr wb = custom;
            BASE_NS::shared_ptr sb = custom;
            sb2 = custom;
            custom = nullptr;
            sb = nullptr;
        }
        ASSERT_TRUE(g_customDeleterCalled == 0);
    }
    ASSERT_TRUE(g_customDeleterCalled == 1);
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test ConstConversion function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, ConstConversion, TestSize.Level1)
{
    auto db = interface_pointer_cast<IBase2>(DualBase::Construct());

    BASE_NS::shared_ptr<IBase2> a(db);
    BASE_NS::shared_ptr<const IBase2> ca(db);

    EXPECT_EQ(db->NonConstCount(), 0);
    EXPECT_EQ(db->ConstCount(), 0);

    a->DoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 0);

    ca->DoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 1);
    ca->DoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 2);

    a->ConstOnlyDoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 3);
    ca->ConstOnlyDoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 4);

    a->NonConstOnlyDoIt();
    EXPECT_EQ(db->NonConstCount(), 2);
    EXPECT_EQ(db->ConstCount(), 4);

    BASE_NS::shared_ptr<const IBase3> da;

    da = interface_pointer_cast<const IBase3>(db);
    da->Identify();

    auto* p = interface_cast<const IBase3>(a);

    auto* po = interface_cast<const IBase2>(a);
    auto* ppo = interface_cast<const IBase2>(ca);

    p->Narf();
}

/**
 * @tc.name: PointerTest
 * @tc.desc: test ConstConversion2 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(PointerTest, ConstConversion2, TestSize.Level1)
{
    auto db = interface_pointer_cast<IBase2>(DualBase::Construct());

    BASE_NS::weak_ptr<IBase2> a(db);
    BASE_NS::weak_ptr<const IBase2> ca(db);

    EXPECT_EQ(db->NonConstCount(), 0);
    EXPECT_EQ(db->ConstCount(), 0);

    a.lock()->DoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 0);

    ca.lock()->DoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 1);
    ca.lock()->DoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 2);

    a.lock()->ConstOnlyDoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 3);
    ca.lock()->ConstOnlyDoIt();
    EXPECT_EQ(db->NonConstCount(), 1);
    EXPECT_EQ(db->ConstCount(), 4);

    a.lock()->NonConstOnlyDoIt();
    EXPECT_EQ(db->NonConstCount(), 2);
    EXPECT_EQ(db->ConstCount(), 4);

    BASE_NS::weak_ptr<const IBase3> da;

    da = interface_pointer_cast<const IBase3>(db);
    da.lock()->Identify();
}
META_END_NAMESPACE()