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

#include <base/containers/refcnt_ptr.h>
#include <base/containers/shared_ptr.h>
#include <base/containers/unique_ptr.h>

#include "test_framework.h"

using namespace BASE_NS;

struct TestPtr {
    void operator()(TestPtr* ptr) const
    {
        ptr->deleted = true;
    }

    void operator()(const TestPtr* ptr) const {}

    void Ref()
    {
        ++ref;
    }

    void Unref()
    {
        if (--ref == 0u) {
            deleted = true;
        }
    }

    bool deleted = false;
    uint32_t ref = 0u;
};

struct TestPtrD : TestPtr {
    constexpr TestPtrD operator+(TestPtrD v)
    {
        v.value += value;
        return v;
    }

public:
    uint32_t value = 1u;
};

// ToDo same tests for array

/**
 * @tc.name: destructor
 * @tc.desc: Tests for Destructor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, destructor, testing::ext::TestSize.Level1)
{
    // destructor calls deleter
    {
        TestPtr test;
        {
            auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        }
        EXPECT_TRUE(test.deleted);
    }
    // with other class destructor

    {
        TestPtr test;
        TestPtrD testD;
        {
            auto ptr = unique_ptr<TestPtr, TestPtrD>(&test, testD);
        }
        EXPECT_TRUE(test.deleted);
        {
            auto ptr = unique_ptr<TestPtrD, TestPtrD>(&testD, testD);
        }
        EXPECT_TRUE(testD.deleted);
    }
    {
        TestPtrD test1;
        TestPtrD test2;
        {
            auto ptr = unique_ptr<TestPtrD, TestPtrD>(&test1, test1 + test2);
        }
        EXPECT_TRUE(test1.deleted);
    }
    {
        TestPtrD test;
        {
            auto ptr = unique_ptr<TestPtrD&, TestPtrD&>(&test, test);
            EXPECT_TRUE(test.deleted == ptr->deleted); // destructor not called yet
        }
        EXPECT_TRUE(test.deleted);
    }
    {
        typedef unique_ptr<TestPtrD&, TestPtrD&> T;
        TestPtrD test1;
        TestPtrD test2;
        {
            auto ptrT1 = T(&test1, test1);
            auto ptrT2 = T(&test2, test2);
            {
                auto ptr = T(static_cast<T&&>(ptrT1));
                // ptr.operator=<TestPtrD&, TestPtrD&>(static_cast<unique_ptr<TestPtrD&, TestPtrD&>&&>(ptrT2));
            }
            EXPECT_TRUE(test1.deleted);  // not destroyed
            EXPECT_FALSE(test2.deleted); // destroyed
            {
                auto ptr = T(static_cast<T&&>(ptrT2));
            }
            EXPECT_TRUE(test1.deleted); // released at ptr.opretator=
            EXPECT_TRUE(test2.deleted); // released at ptr.opretator=
        }
    }

    {
        typedef unique_ptr<TestPtrD&, TestPtrD&> T;
        TestPtrD test1;
        TestPtrD test2;
        {
            auto ptrT1 = T(&test1, test1);
            auto ptrT2 = T(&test2, test2);
            {
                auto ptr = T(static_cast<T&&>(ptrT2));
                test1.value = 10;
                ptr.operator*().value = 20; // jsut reference, does not change value
                test2.value = 30;
                EXPECT_TRUE(ptr.operator*().value == test2.value);
                ptr.operator=<TestPtrD&, TestPtrD&>(static_cast<T&&>(ptrT1));
                EXPECT_TRUE(ptr.operator*().value == test1.value);
            }
            EXPECT_TRUE(test1.deleted);  // destroyed
            EXPECT_FALSE(test2.deleted); // set by operator *
        }
    }
    // const constructor which does nothing
    {
        TestPtrD test1;
        TestPtrD test2;
        {
            auto ptr = unique_ptr<const TestPtrD&, const TestPtrD&>(&test1, test2);
        }
        EXPECT_FALSE(test1.deleted);
        EXPECT_FALSE(test2.deleted);
    }
}

/**
 * @tc.name: makeUnique
 * @tc.desc: Tests for Make Unique. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, makeUnique, testing::ext::TestSize.Level1)
{
    // make_unique
    TestPtrD test;
    test.value = 3u;
    {
        auto ptr = make_unique<TestPtrD>(test);
        auto testPtr = ptr.get();
        EXPECT_EQ(test.value, testPtr->value);
        EXPECT_FALSE(&test == testPtr); // it is a copy
    }
    EXPECT_FALSE(test.deleted); // it is a copy
}

/**
 * @tc.name: get
 * @tc.desc: Tests for Get. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, get, testing::ext::TestSize.Level1)
{
    // get
    TestPtr test;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        auto testPtr = ptr.get();
        EXPECT_EQ(&test, testPtr);
    }
    EXPECT_TRUE(test.deleted);
}

/**
 * @tc.name: release
 * @tc.desc: Tests for Release. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, release, testing::ext::TestSize.Level1)
{
    // release
    TestPtr test;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        auto testPtr = ptr.release();
        EXPECT_EQ(&test, testPtr);
    }
    EXPECT_FALSE(test.deleted);
}

/**
 * @tc.name: resetNull
 * @tc.desc: Tests for Reset Null. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, resetNull, testing::ext::TestSize.Level1)
{
    // reset to null
    TestPtr test;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        EXPECT_TRUE(ptr);
        ptr.reset();
        EXPECT_FALSE(ptr);
    }
    EXPECT_TRUE(test.deleted);
}

/**
 * @tc.name: resetNonNull
 * @tc.desc: Tests for Reset Non Null. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, resetNonNull, testing::ext::TestSize.Level1)
{
    // reset to other value
    TestPtr test;
    TestPtr test2;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        ptr.reset(&test2);
    }
    EXPECT_TRUE(test.deleted);
    EXPECT_TRUE(test2.deleted);
}

/**
 * @tc.name: operatorBool
 * @tc.desc: Tests for Operator Bool. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, operatorBool, testing::ext::TestSize.Level1)
{
    // operator bool
    TestPtr test;
    auto ptr = unique_ptr<TestPtr, TestPtr>(nullptr, test);
    EXPECT_FALSE(ptr);
    ptr.reset(&test);
    EXPECT_TRUE(ptr);
}

/**
 * @tc.name: assignNull
 * @tc.desc: Tests for Assign Null. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, assignNull, testing::ext::TestSize.Level1)
{
    // operator =
    TestPtr test;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        ptr = nullptr;
        EXPECT_TRUE(test.deleted);
    }
}

/**
 * @tc.name: assignNonNull
 * @tc.desc: Tests for Assign Non Null. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, assignNonNull, testing::ext::TestSize.Level1)
{
    // operator =
    TestPtr test;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(nullptr, test);
        {
            auto ptr2 = unique_ptr<TestPtr, TestPtr>(&test, test);
            ptr = move(ptr2);
            EXPECT_FALSE(test.deleted);
        }
        EXPECT_FALSE(test.deleted);
    }
    EXPECT_TRUE(test.deleted);
}

/**
 * @tc.name: assignNonPointer
 * @tc.desc: Tests for Assign Non Pointer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, assignNonPointer, testing::ext::TestSize.Level1)
{
    typedef unique_ptr<TestPtrD&, TestPtrD&> T;
    {
        TestPtrD test1;
        TestPtrD test2;
        {
            auto ptrT1 = T(&test1, test1);
            auto ptrT2 = T(&test2, test2);
            {
                auto ptr = T(static_cast<T&&>(ptrT2));
                ptr.operator=<TestPtrD&, TestPtrD&>(static_cast<T&&>(ptrT1));
            }
            EXPECT_TRUE(test1.deleted);  // destroyed
            EXPECT_FALSE(test2.deleted); // released, not destroyed. It means, that it ptrT2 is NULL, and address of
                                         // test2 cannot be recieved anymore
            EXPECT_TRUE(ptrT1 == nullptr);
            EXPECT_TRUE(ptrT2 == nullptr);
            {
                auto ptr = T(static_cast<T&&>(ptrT2));
            }
            EXPECT_FALSE(test2.deleted); // NULL pointer delete instead of test2
        }
        EXPECT_FALSE(test2.deleted); // destruction of pointer to NULL does not help as well
    }
    {
        TestPtrD test1;
        TestPtrD test2;
        {
            auto ptrT1 = T(&test1, test1);
            auto ptrT2 = T(&test2, test2);
            {
                auto ptr = T(static_cast<T&&>(ptrT2));
                ptr.operator=<TestPtrD&, TestPtrD&>(static_cast<T&&>(ptrT1));
            }
            EXPECT_TRUE(test1.deleted); // destroyed
            EXPECT_FALSE(
                test2.deleted); // released, not destroyed. It means, that it points to NULL, and value cannot be read
            {
                auto ptr = T(static_cast<T&&>(ptrT2));
                ptr.operator=<TestPtrD&, TestPtrD&>(static_cast<T&&>(ptrT1));
                ptr.operator=<TestPtrD&, TestPtrD&>(static_cast<T&&>(ptrT2));
                ptr.operator=<TestPtrD&, TestPtrD&>(static_cast<T&&>(ptrT1));
            }
            EXPECT_TRUE(test2.deleted); // However multiple calls of operator will set value to deleted because poiter
                                        // ptrT2 has its deleter unchanged
        }
    }
}

/**
 * @tc.name: moveConstructor
 * @tc.desc: Tests for Move Constructor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, moveConstructor, testing::ext::TestSize.Level1)
{
    // move constructor
    TestPtr test;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        EXPECT_TRUE(ptr);
        auto ptr2 = unique_ptr(move(ptr));
        EXPECT_FALSE(ptr);
        EXPECT_TRUE(ptr2);
        EXPECT_FALSE(test.deleted);
    }
    EXPECT_TRUE(test.deleted);
}

/**
 * @tc.name: swap
 * @tc.desc: Tests for Swap. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, swap, testing::ext::TestSize.Level1)
{
    // swap
    TestPtr test;
    TestPtr test2;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        EXPECT_TRUE(ptr);
        EXPECT_EQ(&test, ptr.get());
        {
            auto ptr2 = unique_ptr<TestPtr, TestPtr>(&test2, test);
            ASSERT_TRUE(ptr2);
            EXPECT_EQ(&test2, ptr2.get());

            ptr.swap(ptr2);
            EXPECT_EQ(&test, ptr2.get());
            EXPECT_EQ(&test2, ptr.get());
        }
        EXPECT_TRUE(test.deleted);
        EXPECT_FALSE(test2.deleted);
    }
    EXPECT_TRUE(test2.deleted);
}

/**
 * @tc.name: destructorArray
 * @tc.desc: Tests for Destructor Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, destructorArray, testing::ext::TestSize.Level1)
{
    // destructor calls deleter
    {
        TestPtr* test(new TestPtr[3]);
        {
            auto ptr = unique_ptr<TestPtr[]>(test);
        }
        // default_deleter should have deallocated the memory so can't really verify.
    }
    {
        TestPtr* test(new TestPtr[3]);
        {
            auto ptr = unique_ptr<TestPtr[], TestPtr>(test, TestPtr());
        }
        // deleter from struct TestPtr
        EXPECT_TRUE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
        EXPECT_FALSE(test[2].deleted);
        delete[] test;
    }
    {
        TestPtr* test(new TestPtr[3]);
        {
            auto ptr = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
        }
        // seleter by lvalue type
        EXPECT_TRUE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
        EXPECT_FALSE(test[2].deleted);
        delete[] test;
    }
    {
        TestPtr* test(new TestPtr[3]);
        {
            auto ptr = unique_ptr<TestPtr[], TestPtr&>(test, test[1]);
        }

        EXPECT_TRUE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
        EXPECT_FALSE(test[2].deleted);
        delete[] test;
    }

    {
        TestPtr* test(new TestPtr[3]);
        {
            auto ptr = unique_ptr<TestPtr[], const TestPtr&>(test, test[1]);
        }

        EXPECT_TRUE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
        EXPECT_FALSE(test[2].deleted);

        delete[] test;
    }

    {
        TestPtr* test(new TestPtr[3]);
        auto p = unique_ptr<TestPtr[], TestPtr&>(test, test[1]);
        {
            auto ptr = unique_ptr<TestPtr[], TestPtr&>(static_cast<unique_ptr<TestPtr[], TestPtr&>&&>(p));
        }

        EXPECT_TRUE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
        EXPECT_FALSE(test[2].deleted);

        delete[] test;
    }

    {
        TestPtr* test(new TestPtr[3]);
        auto p = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
        {
            auto ptr = unique_ptr<TestPtr[], TestPtr>(static_cast<unique_ptr<TestPtr[], TestPtr>&&>(p));
        }

        EXPECT_TRUE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
        EXPECT_FALSE(test[2].deleted);

        delete[] test;
    }
}

/**
 * @tc.name: releaseArray
 * @tc.desc: Tests for Release Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, releaseArray, testing::ext::TestSize.Level1)
{
    // release
    TestPtr* test(new TestPtr[3]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
        auto testPtr = ptr.release();
        EXPECT_EQ(test, testPtr);
    }
    EXPECT_FALSE(test[0].deleted);

    delete[] test;
}

/**
 * @tc.name: resetNullArray
 * @tc.desc: Tests for Reset Null Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, resetNullArray, testing::ext::TestSize.Level1)
{
    // reset to null
    TestPtr* test(new TestPtr[3]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
        EXPECT_TRUE(ptr);
        ptr.reset();
        EXPECT_FALSE(ptr);
    }
    EXPECT_TRUE(test[0].deleted);

    delete[] test;
}

/**
 * @tc.name: resetNonNullArray
 * @tc.desc: Tests for Reset Non Null Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, resetNonNullArray, testing::ext::TestSize.Level1)
{
    // reset to other value
    TestPtr* test1(new TestPtr[3]);
    TestPtr* test2(new TestPtr[3]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(test1, test1[1]);
        ptr.reset(test2);
    }
    EXPECT_TRUE(test1[0].deleted);
    EXPECT_TRUE(test2[0].deleted);
    if (int(test1->ref) >= 0)
        delete[] test1;
    if (int(test2->ref) >= 0)
        delete[] test2;
}

/**
 * @tc.name: getArray
 * @tc.desc: Tests for Get Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, getArray, testing::ext::TestSize.Level1)
{
    // get
    TestPtr* test(new TestPtr[3]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
        auto testPtr = ptr.get();
        EXPECT_EQ(test, testPtr);
    }
    EXPECT_TRUE(test[0].deleted);

    delete[] test;
}

/**
 * @tc.name: operatorIndexArray
 * @tc.desc: Tests for Operator Index Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, operatorIndexArray, testing::ext::TestSize.Level1)
{
    // operator bool
    TestPtrD* test(new TestPtrD[3]);
    test[0].value = 0u;
    test[1].value = 1u;
    test[2].value = 2u;
    {
        auto ptr = unique_ptr<TestPtrD[], TestPtrD>(test, test[0]);
        EXPECT_EQ(ptr.operator[](0).value, test[0].value);
        EXPECT_EQ(ptr.operator[](1).value, test[1].value);
        EXPECT_EQ(ptr.operator[](2).value, test[2].value);
        EXPECT_EQ(ptr[2].value, test[2].value);
    }
    delete[] test;
}

/**
 * @tc.name: operatorBoolArray
 * @tc.desc: Tests for Operator Bool Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, operatorBoolArray, testing::ext::TestSize.Level1)
{
    // operator bool
    TestPtr* test(new TestPtr[3]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(nullptr, test[0]);
        EXPECT_FALSE(ptr);
        ptr.reset(test);
        EXPECT_TRUE(ptr);
    }
    delete[] test;
}

/**
 * @tc.name: assignNullArray
 * @tc.desc: Tests for Assign Null Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, assignNullArray, testing::ext::TestSize.Level1)
{
    // operator =
    // operatro == nullptr
    TestPtr* test(new TestPtr[3]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
        ptr = nullptr;
        EXPECT_TRUE(ptr == nullptr);
        EXPECT_TRUE(nullptr == ptr);
        EXPECT_TRUE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
    }

    delete[] test;
}

/**
 * @tc.name: assignNonNullArray
 * @tc.desc: Tests for Assign Non Null Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, assignNonNullArray, testing::ext::TestSize.Level1)
{
    // operator =
    // operator ==
    // operator !=
    TestPtr* test(new TestPtr[3]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(nullptr, test[1]);
        {
            auto ptr2 = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
            EXPECT_TRUE(ptr != ptr2);
            ptr = move(ptr2);
            EXPECT_TRUE(ptr != nullptr);
            EXPECT_TRUE(ptr != ptr2); // ptr2 NULL
            EXPECT_TRUE(nullptr != ptr);
            EXPECT_FALSE(test[0].deleted);
            EXPECT_FALSE(test[1].deleted);
        }
        EXPECT_FALSE(test[0].deleted);
        EXPECT_FALSE(test[1].deleted);
    }
    EXPECT_TRUE(test[0].deleted);
    EXPECT_FALSE(test[1].deleted);

    delete[] test;
}

/**
 * @tc.name: assignNonPointerArray
 * @tc.desc: Tests for Assign Non Pointer Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, assignNonPointerArray, testing::ext::TestSize.Level1)
{
    // operator ==
    TestPtr* test1(new TestPtr[2]);
    TestPtr* test2(new TestPtr[2]);
    typedef unique_ptr<TestPtr[], TestPtr&> T;
    {
        auto p1 = T(test1, test1[1]);
        auto p2 = T(test2, test2[1]);
        {
            auto ptr = T(static_cast<T&&>(p1));
            auto ptr2 = T(static_cast<T&&>(p2));
            auto ptr3 = T(static_cast<T&&>(p2));
            EXPECT_TRUE(ptr == ptr);
            EXPECT_FALSE(ptr2 == ptr3); // only 1 assigment at time
            ptr.operator=<TestPtr[], TestPtr&>(T(static_cast<T&&>(p2)));
            ptr2.operator=<TestPtr[], TestPtr&>(T(static_cast<T&&>(p1)));
            EXPECT_TRUE(ptr == ptr2); // both reseted
        }
        EXPECT_TRUE(test1[0].deleted); // Static array as for pointer. No tricks with pointers to NULL anymore
        EXPECT_FALSE(test1[1].deleted);

        EXPECT_TRUE(test2[0].deleted);
        EXPECT_FALSE(test2[1].deleted);
        {
            auto ptr = T(static_cast<T&&>(p2));
        }
        EXPECT_TRUE(test2[0].deleted);
        EXPECT_FALSE(test2[1].deleted);
    }
    EXPECT_TRUE(test2[0].deleted);
    EXPECT_FALSE(test2[1].deleted);

    delete[] test1;
    delete[] test2;
}

/**
 * @tc.name: moveConstructorArray
 * @tc.desc: Tests for Move Constructor Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, moveConstructorArray, testing::ext::TestSize.Level1)
{
    // move constructor
    TestPtr* test(new TestPtr[2]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(test, test[1]);
        EXPECT_TRUE(ptr);
        auto ptr2 = unique_ptr(move(ptr));
        EXPECT_FALSE(ptr);
        EXPECT_TRUE(ptr2);
        EXPECT_FALSE(test[0].deleted);
    }
    EXPECT_TRUE(test[0].deleted);

    delete[] test;
}

/**
 * @tc.name: swapArray
 * @tc.desc: Tests for Swap Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, swapArray, testing::ext::TestSize.Level1)
{
    // swap
    TestPtr* test1(new TestPtr[2]);
    TestPtr* test2(new TestPtr[2]);
    {
        auto ptr = unique_ptr<TestPtr[], TestPtr>(test1, test1[1]);
        EXPECT_TRUE(ptr);
        EXPECT_EQ(test1, ptr.get());
        {
            auto ptr2 = unique_ptr<TestPtr[], TestPtr>(test2, test1[1]);
            ASSERT_TRUE(ptr2);
            EXPECT_EQ(test2, ptr2.get());

            ptr.swap(ptr2);
            EXPECT_EQ(test1, ptr2.get());
            EXPECT_EQ(test2, ptr.get());
        }
        EXPECT_TRUE(test1[0].deleted);
        EXPECT_FALSE(test2[0].deleted);
    }
    EXPECT_TRUE(test2[0].deleted);

    delete[] test1;
    delete[] test2;
}
/**
 * @tc.name: staticPointerCastToDerived
 * @tc.desc: static_pointer_cast should work for unique_ptr if we cast to the correct derived type
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, staticPointerCastToDerived, testing::ext::TestSize.Level1)
{
    // GIVEN
    TestPtrD d;
    d.value = 42;
    BASE_NS::unique_ptr<TestPtr> base = BASE_NS::make_unique<TestPtrD>(d);

    // WHEN
    auto res = static_pointer_cast<TestPtrD>(BASE_NS::move(base));
    EXPECT_TRUE(res);
    static_assert(BASE_NS::is_same_v<decltype(res), BASE_NS::unique_ptr<TestPtrD>>, "wrong return type");
    EXPECT_EQ(res->value, 42);
}

/**
 * @tc.name: staticPointerCastToDerivedNull
 * @tc.desc: static_pointer_cast should work for unique_ptr if we cast to the correct derived type, but we supply null
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, staticPointerCastToDerivedNull, testing::ext::TestSize.Level1)
{
    // GIVEN
    BASE_NS::unique_ptr<TestPtr> base = BASE_NS::unique_ptr<TestPtrD>(nullptr);

    // WHEN
    auto res = static_pointer_cast<TestPtrD>(BASE_NS::move(base));
    static_assert(BASE_NS::is_same_v<decltype(res), BASE_NS::unique_ptr<TestPtrD>>, "wrong return type");

    // THEN
    EXPECT_EQ(res.get(), nullptr);
}

/**
 * @tc.name: staticPointerCastToDerivedNull
 * @tc.desc: static_pointer_cast should work for unique_ptr if we cast to the wrong derived type, but we supply null
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUniquePtr, staticPointerCastToNonDerivedNull, testing::ext::TestSize.Level1)
{
    // GIVEN
    BASE_NS::unique_ptr<TestPtr> base = BASE_NS::unique_ptr<TestPtrD>(nullptr);

    struct Sibling : TestPtr {
        int member {};
    };
    // WHEN casting to a wrong type (e.g. sibling)
    auto res = static_pointer_cast<Sibling>(BASE_NS::move(base));

    // THEN
    EXPECT_EQ(res.get(), nullptr);
    static_assert(BASE_NS::is_same_v<decltype(res), BASE_NS::unique_ptr<Sibling>>, "wrong return type");
}
/**
 * @tc.name: destructor
 * @tc.desc: Tests for Destructor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, destructor, testing::ext::TestSize.Level1)
{
    // destructor calls deleter
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        EXPECT_EQ(1u, test.ref);
    }
    EXPECT_EQ(0u, test.ref);
    {
        const auto ptr = refcnt_ptr<TestPtr&>(&test);
        EXPECT_EQ(1u, test.ref);
        auto ptr2 = refcnt_ptr<TestPtr>(ptr);
        EXPECT_EQ(2u, test.ref);
        auto ptr3 = refcnt_ptr<TestPtrD>(static_cast<refcnt_ptr<TestPtr>&&>(ptr));
        EXPECT_EQ(3u, test.ref);
    }
    EXPECT_EQ(0u, test.ref);
}

/**
 * @tc.name: get
 * @tc.desc: Tests for Get. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, get, testing::ext::TestSize.Level1)
{
    // get
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        auto testPtr = ptr.get();
        EXPECT_EQ(&test, testPtr);
    }
    EXPECT_EQ(0u, test.ref);
}

/**
 * @tc.name: release
 * @tc.desc: Tests for Release. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, release, testing::ext::TestSize.Level1)
{
    // release
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        auto testPtr = ptr.release();
        EXPECT_EQ(&test, testPtr);
    }
    EXPECT_EQ(1u, test.ref);
}

/**
 * @tc.name: resetNull
 * @tc.desc: Tests for Reset Null. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, resetNull, testing::ext::TestSize.Level1)
{
    // reset to null
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        EXPECT_TRUE(ptr);
        ptr.reset();
        EXPECT_FALSE(ptr);
    }
    EXPECT_EQ(0u, test.ref);
}

/**
 * @tc.name: resetNonNull
 * @tc.desc: Tests for Reset Non Null. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, resetNonNull, testing::ext::TestSize.Level1)
{
    // reset to other value
    TestPtr test;
    TestPtr test2;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        ptr.reset(&test2);
    }
    EXPECT_EQ(0u, test.ref);
    EXPECT_EQ(0u, test2.ref);
}

/**
 * @tc.name: operatorBool
 * @tc.desc: Tests for Operator Bool. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, operatorBool, testing::ext::TestSize.Level1)
{
    // operator bool
    TestPtr test;
    auto ptr = refcnt_ptr<TestPtr>(nullptr);
    EXPECT_FALSE(ptr);
    ptr.reset(&test);
    EXPECT_TRUE(ptr);
}

/**
 * @tc.name: assign
 * @tc.desc: Tests for Assign. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, assign, testing::ext::TestSize.Level1)
{
    // operator =
    // operator ==, !=
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        EXPECT_TRUE(ptr != nullptr);
        ptr = nullptr;
        EXPECT_EQ(0u, test.ref);
        EXPECT_TRUE(ptr == nullptr);
        ptr.operator=<TestPtr>(nullptr);
        EXPECT_TRUE(ptr == nullptr);
    }
}

/**
 * @tc.name: moveAssign
 * @tc.desc: Tests for Move Assign. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, moveAssign, testing::ext::TestSize.Level1)
{
    // move operator =
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        {
            auto ptr2 = refcnt_ptr<TestPtr>(&test);
            EXPECT_EQ(2u, test.ref);
            ptr = move(ptr2);
            EXPECT_EQ(1u, test.ref);
        }
        EXPECT_EQ(1u, test.ref);
    }
    EXPECT_EQ(0u, test.ref);
}

/**
 * @tc.name: moveConstructor
 * @tc.desc: Tests for Move Constructor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, moveConstructor, testing::ext::TestSize.Level1)
{
    // move constructor
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        EXPECT_TRUE(ptr);
        auto ptr2 = refcnt_ptr(move(ptr));
        EXPECT_FALSE(ptr);
        EXPECT_TRUE(ptr2);
        EXPECT_EQ(1u, test.ref);
    }
    EXPECT_EQ(0u, test.ref);
}

/**
 * @tc.name: copyConstructor
 * @tc.desc: Tests for Copy Constructor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, copyConstructor, testing::ext::TestSize.Level1)
{
    // copy constructor and operator =
    // operator ==, !=, ->, *
    TestPtr test;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        {
            auto ptr2 = refcnt_ptr(ptr);
            EXPECT_EQ(2u, test.ref);
            EXPECT_TRUE(ptr == ptr2);
            ptr = ptr2;
            EXPECT_EQ(2u, test.ref);
            EXPECT_TRUE(ptr == ptr2);
            EXPECT_TRUE(ptr->ref == ptr2->ref);
            EXPECT_TRUE(ptr.operator*().ref == ptr2.operator*().ref);
            ptr.operator=<TestPtr>(ptr2);
            EXPECT_TRUE(ptr == ptr2);
        }
        EXPECT_EQ(1u, test.ref);
    }
    EXPECT_EQ(0u, test.ref);
}

/**
 * @tc.name: swap
 * @tc.desc: Tests for Swap. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersRefCntPtr, swap, testing::ext::TestSize.Level1)
{
    // swap
    // operator !=

    TestPtr test;
    TestPtr test2;
    {
        auto ptr = refcnt_ptr<TestPtr>(&test);
        EXPECT_TRUE(ptr);
        EXPECT_EQ(&test, ptr.get());
        {
            auto ptr2 = refcnt_ptr<TestPtr>(&test2);
            ASSERT_TRUE(ptr2);
            EXPECT_TRUE(ptr != ptr2);
            EXPECT_EQ(&test2, ptr2.get());

            ptr.swap(ptr2);
            EXPECT_EQ(&test, ptr2.get());
            EXPECT_EQ(&test2, ptr.get());
            EXPECT_EQ(1u, test.ref);
            EXPECT_EQ(1u, test2.ref);
        }
        EXPECT_EQ(0u, test.ref);
        EXPECT_EQ(1u, test2.ref);
    }
    EXPECT_EQ(0u, test2.ref);
}

namespace {
struct PtrTestData {
    PtrTestData(int& constructCount, int& destructCount)
        : constructCount_(constructCount), destructCount_(destructCount)
    {
        constructCount_++;
    }
    ~PtrTestData()
    {
        destructCount_++;
    }

    int data_ { 0 };
    int& constructCount_;
    int& destructCount_;

    static constexpr int dataValue = 42;
};
} // namespace

/**
 * @tc.name: SharedPtr
 * @tc.desc: Tests for Shared Ptr. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, SharedPtr, testing::ext::TestSize.Level1)
{
    int constructCount = 0, destructCount = 0;

    BASE_NS::shared_ptr<PtrTestData> ptr(new PtrTestData(constructCount, destructCount));
    ASSERT_NE(ptr.get(), nullptr);

    EXPECT_EQ(ptr->data_, 0);
    EXPECT_EQ(constructCount, 1);

    BASE_NS::shared_ptr<PtrTestData> ptr2(new PtrTestData(constructCount, destructCount));
    EXPECT_EQ(constructCount, 2);
    ptr2->data_ = PtrTestData::dataValue;
    EXPECT_NE(ptr->data_, ptr2->data_);

    EXPECT_EQ(destructCount, 0);

    ptr = ptr2; // assign ptr2 to ptr, should cause original ptr to be destroyed
    EXPECT_EQ(destructCount, 1);
    EXPECT_EQ(ptr->data_, ptr2->data_);
    ptr.reset();
    ptr2.reset();
    EXPECT_EQ(destructCount, 2);
}

/**
 * @tc.name: WeakPtr
 * @tc.desc: Tests for Weak Ptr. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, WeakPtr, testing::ext::TestSize.Level1)
{
    int constructCount = 0, destructCount = 0;

    BASE_NS::shared_ptr<PtrTestData> ptr(new PtrTestData(constructCount, destructCount));
    ptr->data_ = PtrTestData::dataValue;
    EXPECT_EQ(ptr->data_, PtrTestData::dataValue);
    EXPECT_EQ(constructCount, 1);

    BASE_NS::weak_ptr<PtrTestData> wptr(ptr);
    EXPECT_EQ(constructCount, 1);
    EXPECT_EQ(ptr.use_count(), 1);
    EXPECT_EQ(ptr.weak_count(), 1);
    EXPECT_EQ(ptr.use_count(), wptr.use_count());

    {
        auto p = wptr.lock(); // Lock weak_ptr when ptr still exists, should get valid object

        ASSERT_NE(p.get(), nullptr);
        EXPECT_EQ(constructCount, 1);
        EXPECT_EQ(p->data_, PtrTestData::dataValue);
    }
    {
        // copy construct
        BASE_NS::weak_ptr<PtrTestData> wptr2(wptr);
        BASE_NS::weak_ptr<PtrTestData> wptr3;
        EXPECT_EQ(ptr.use_count(), 1);
        EXPECT_EQ(ptr.weak_count(), 2);

        // copy assign
        wptr3 = wptr2;
        EXPECT_EQ(ptr.use_count(), 1);
        EXPECT_EQ(ptr.weak_count(), 3);

        wptr3 = nullptr;
        EXPECT_EQ(ptr.weak_count(), 2);
    }
    {
        // move construct
        BASE_NS::weak_ptr<PtrTestData> wptr2(BASE_NS::move(wptr));
        EXPECT_EQ(ptr.use_count(), 1);
        EXPECT_EQ(ptr.weak_count(), 1);

        // move assign
        wptr = BASE_NS::move(wptr2);
        EXPECT_EQ(ptr.use_count(), 1);
        EXPECT_EQ(ptr.weak_count(), 1);
    }
    {
        BASE_NS::weak_ptr<PtrTestData> wptr2;
        EXPECT_EQ(ptr.use_count(), 1);
        EXPECT_EQ(ptr.weak_count(), 1);

        // assign shared_ptr
        wptr2 = ptr;
        EXPECT_EQ(ptr.use_count(), 1);
        EXPECT_EQ(ptr.weak_count(), 2);
    }

    ptr.reset(); // destroy ptr
    EXPECT_EQ(destructCount, 1);

    auto p = wptr.lock(); // wptr.lock() should still works but returns nullptr
    EXPECT_EQ(p.get(), nullptr);
}

/**
 * @tc.name: NullTests
 * @tc.desc: Tests for Null Tests. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, NullTests, testing::ext::TestSize.Level1)
{
    int constructCount = 0, destructCount = 0;

    BASE_NS::shared_ptr<PtrTestData> ptr(new PtrTestData(constructCount, destructCount));
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
    ASSERT_TRUE(ptr3 == ptr);

    {
        // Comparison of expired pointers
        BASE_NS::weak_ptr<PtrTestData> empty;
        BASE_NS::shared_ptr<PtrTestData> ptrA(new PtrTestData(constructCount, destructCount));
        BASE_NS::weak_ptr<PtrTestData> ptrAW = ptrA;

        BASE_NS::shared_ptr<PtrTestData> ptrB(new PtrTestData(constructCount, destructCount));
        BASE_NS::weak_ptr<PtrTestData> ptrBW = ptrB;
        ptrA.reset();
        ptrB.reset();
        ASSERT_TRUE(ptrAW.lock() == ptrBW.lock()); // this is the same though, since both are NULL.
        ASSERT_TRUE(empty.lock() == ptrBW.lock()); // this is the same though, since both are NULL.
    }
}

/**
 * @tc.name: SharedPtrCanBeConvertedToSharedPtrWithConstType
 * @tc.desc: Tests for Shared Ptr Can Be Converted To Shared Ptr With Const Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, SharedPtrCanBeConvertedToSharedPtrWithConstType, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    auto ptr = BASE_NS::shared_ptr<int>(new int(initialValue));

    // when
    BASE_NS::shared_ptr<const int> cptr = ptr;

    // expected
    EXPECT_EQ(*cptr, initialValue);
}

/**
 * @tc.name: SharedPtrCanBeReturnedAsSharedPtrWithConstType
 * @tc.desc: Tests for Shared Ptr Can Be Returned As Shared Ptr With Const Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, SharedPtrCanBeReturnedAsSharedPtrWithConstType, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    auto ptr = BASE_NS::shared_ptr<int>(new int(initialValue));

    auto functionReturningSharedConstPtr = [ptr]() -> BASE_NS::shared_ptr<const int> { return ptr; };

    // when
    auto cptr = functionReturningSharedConstPtr();

    // expected
    EXPECT_EQ(*cptr, initialValue);
}

/**
 * @tc.name: DefaultConstructedSharedPtrIsEmpty
 * @tc.desc: Tests for Default Constructed Shared Ptr Is Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, DefaultConstructedSharedPtrIsEmpty, testing::ext::TestSize.Level1)
{
    // given
    BASE_NS::shared_ptr<int> ptr;

    // when
    const auto rawPtr = ptr.get();
    const bool boolOperatorResult = ptr.operator bool();

    // expected
    EXPECT_EQ(rawPtr, nullptr);
    EXPECT_EQ(boolOperatorResult, false);
}

/**
 * @tc.name: CopiedSharedPtrExtendObjectLifetime
 * @tc.desc: Tests for Copied Shared Ptr Extend Object Lifetime. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, CopiedSharedPtrExtendObjectLifetime, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    BASE_NS::shared_ptr<const int> cptr(new int(1));

    // when
    {
        auto ptr = BASE_NS::shared_ptr<int>(new int(initialValue));
        EXPECT_NE(ptr, nullptr);

        cptr = ptr;
    }

    // expected
    EXPECT_EQ(*cptr, initialValue);
}

/**
 * @tc.name: CopyAssignedSharedPtrExtendObjectLifetime
 * @tc.desc: Tests for Copy Assigned Shared Ptr Extend Object Lifetime. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, CopyAssignedSharedPtrExtendObjectLifetime, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    BASE_NS::shared_ptr<int> ptr = CreateShared<int>(initialValue);
    BASE_NS::shared_ptr<int> copy = CreateShared<int>(12);

    // when
    copy = ptr;
    ptr.reset();

    // expected
    EXPECT_EQ(*copy, initialValue);
}

/**
 * @tc.name: CopyConstructedSharedPtrExtendObjectLifetime
 * @tc.desc: Tests for Copy Constructed Shared Ptr Extend Object Lifetime. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, CopyConstructedSharedPtrExtendObjectLifetime, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    BASE_NS::shared_ptr<int> ptr = CreateShared<int>(initialValue);

    // when
    auto copy = BASE_NS::shared_ptr<int>(ptr);
    ptr.reset();

    // expected
    EXPECT_EQ(*copy, initialValue);
}

/**
 * @tc.name: SharedPtrFromWeakPtr
 * @tc.desc: Tests for Shared Ptr From Weak Ptr. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, SharedPtrFromWeakPtr, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    BASE_NS::shared_ptr<int> ptr = CreateShared<int>(initialValue);

    // when
    auto weak = BASE_NS::weak_ptr(ptr);
    auto copy = BASE_NS::shared_ptr(weak);
    ptr = nullptr;

    // expected
    EXPECT_FALSE(weak.expired());
    EXPECT_EQ(weak.use_count(), copy.use_count());
    EXPECT_EQ(*copy, initialValue);
    EXPECT_EQ(*weak.lock(), initialValue);
    EXPECT_NE(weak.lock(), ptr);
}

/**
 * @tc.name: ResetSharedPtrFromPtr
 * @tc.desc: Tests for Reset Shared Ptr From Ptr. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, ResetSharedPtrFromPtr, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    constexpr auto initialValue2 = 12;
    BASE_NS::shared_ptr<int> ptr = CreateShared<int>(initialValue);
    int* ptr2 = new int(initialValue2);

    // when
    ptr.reset(ptr2);

    // expected
    EXPECT_EQ(*ptr, initialValue2);
}

/**
 * @tc.name: Swap
 * @tc.desc: Tests for Swap. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, Swap, testing::ext::TestSize.Level1)
{
    // given
    constexpr auto initialValue = 120;
    constexpr auto initialValue2 = 12;
    BASE_NS::shared_ptr<int> ptr = CreateShared<int>(initialValue);
    BASE_NS::shared_ptr<int> ptr2 = CreateShared<int>(initialValue2);

    // when
    ptr.swap(ptr2);

    // expected
    EXPECT_EQ(*ptr, initialValue2);
    EXPECT_EQ(*ptr2, initialValue);
}

struct CountType {
    int count { 1 };
};

void CountDeleter(void* p)
{
    --static_cast<CountType*>(p)->count;
}

/**
 * @tc.name: MoveMemoryLeakAndAssert
 * @tc.desc: Tests for Move Memory Leak And Assert. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, MoveMemoryLeakAndAssert, testing::ext::TestSize.Level1)
{
    CountType test;
    {
        BASE_NS::shared_ptr<CountType> p(&test, CountDeleter);
        auto copy = p;
        p = BASE_NS::move(copy);
    }

    EXPECT_EQ(test.count, 0);
}

/**
 * @tc.name: Expired
 * @tc.desc: Tests for Expired. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, Expired, testing::ext::TestSize.Level1)
{
    auto ptr = BASE_NS::shared_ptr<int>(new int(1));
    BASE_NS::weak_ptr<int> w = ptr;
    EXPECT_FALSE(w.expired());
    ptr.reset();
    EXPECT_TRUE(w.expired());
}

/**
 * @tc.name: Comparison
 * @tc.desc: Tests for Comparison. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, Comparison, testing::ext::TestSize.Level1)
{
    auto p1 = BASE_NS::shared_ptr<int>(new int(1));
    auto p2 = p1;
    auto p3 = BASE_NS::shared_ptr<int>(p1.get(), [](void*) {});

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1, p3);
    EXPECT_EQ(p2, p3);
}

/**
 * @tc.name: ExplicitDeleterBasicType
 * @tc.desc: Tests for Explicit Deleter Basic Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, ExplicitDeleterBasicType, testing::ext::TestSize.Level1)
{
    int count = 0;
    {
        auto p = BASE_NS::shared_ptr<int>(&count, [&](int*) { ++count; });
    }
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: ResetWithDeleter
 * @tc.desc: Tests for Reset With Deleter. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, ResetWithDeleter, testing::ext::TestSize.Level1)
{
    CountType test;
    CountType test2;

    BASE_NS::shared_ptr<CountType> p(&test, CountDeleter);
    EXPECT_EQ(test.count, 1);

    p.reset(&test2, CountDeleter);
    EXPECT_EQ(test.count, 0);
    EXPECT_EQ(test2.count, 1);

    p.reset(nullptr, CountDeleter);
    EXPECT_EQ(test2.count, 0);
}

/**
 * @tc.name: UniquePtrConversion
 * @tc.desc: Tests for Unique Ptr Conversion. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, UniquePtrConversion, testing::ext::TestSize.Level1)
{
    {
        BASE_NS::unique_ptr<int> unique(new int(2));
        BASE_NS::shared_ptr<int> shared(BASE_NS::move(unique));
        ASSERT_TRUE(!unique);
        ASSERT_TRUE(shared);
        EXPECT_EQ(*shared, 2);
    }
    {
        int count = 0;
        {
            auto deleter = [&](auto) { ++count; };
            int value = 2;
            BASE_NS::unique_ptr<int, decltype(deleter)> unique(&value, deleter);
            BASE_NS::shared_ptr<int> shared(BASE_NS::move(unique));
            ASSERT_TRUE(!unique);
            ASSERT_TRUE(shared);
            EXPECT_EQ(*shared, 2);
            EXPECT_EQ(count, 0);
        }
        EXPECT_EQ(count, 1);
    }
    {
        BASE_NS::unique_ptr<int> unique(new int(2));
        BASE_NS::shared_ptr<int> shared;
        shared = BASE_NS::move(unique);
        ASSERT_TRUE(!unique);
        ASSERT_TRUE(shared);
        EXPECT_EQ(*shared, 2);
    }
}

/**
 * @tc.name: UseCount
 * @tc.desc: Tests for Use Count. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, UseCount, testing::ext::TestSize.Level1)
{
    {
        BASE_NS::shared_ptr<int> p;
        EXPECT_EQ(p.use_count(), 0);
        EXPECT_EQ(p.weak_count(), 0);
    }
    {
        BASE_NS::shared_ptr<int> p(new int);
        EXPECT_EQ(p.use_count(), 1);
        EXPECT_EQ(p.weak_count(), 0);
    }
    {
        BASE_NS::shared_ptr<int> p(new int);
        BASE_NS::shared_ptr<int> p2(p);
        EXPECT_EQ(p.use_count(), 2);
        EXPECT_EQ(p.weak_count(), 0);

        BASE_NS::weak_ptr<int> w = p;
        EXPECT_EQ(p.use_count(), 2);
        EXPECT_EQ(p.weak_count(), 1);
        EXPECT_EQ(w.use_count(), 2);

        p2.reset();
        BASE_NS::weak_ptr<int> w2 = p;
        EXPECT_EQ(p.use_count(), 1);
        EXPECT_EQ(p.weak_count(), 2);
        EXPECT_EQ(w.use_count(), 1);

        p.reset();
        EXPECT_EQ(p.use_count(), 0);
        EXPECT_EQ(p.weak_count(), 0);
        EXPECT_EQ(w.use_count(), 0);
    }
}

/**
 * @tc.name: staticPointerCastToDerived
 * @tc.desc: static_pointer_cast should work for shared_ptr if we cast to the correct derived type
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, staticPointerCastToDerived, testing::ext::TestSize.Level1)
{
    // GIVEN
    TestPtrD d;
    d.value = 42;
    BASE_NS::shared_ptr<TestPtr> base = BASE_NS::make_shared<TestPtrD>(d);

    // WHEN
    auto res = static_pointer_cast<TestPtrD>(base);
    EXPECT_TRUE(res);
    static_assert(BASE_NS::is_same_v<decltype(res), BASE_NS::shared_ptr<TestPtrD>>, "wrong return type");
    EXPECT_EQ(res->value, 42);
}

/**
 * @tc.name: staticPointerCastToDerivedNull
 * @tc.desc: static_pointer_cast should work for shared_ptr if we cast to the correct derived type, but we supply null
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, staticPointerCastToDerivedNull, testing::ext::TestSize.Level1)
{
    // GIVEN
    BASE_NS::shared_ptr<TestPtr> base = BASE_NS::shared_ptr<TestPtrD>(nullptr);

    // WHEN
    auto res = static_pointer_cast<TestPtrD>(base);
    static_assert(BASE_NS::is_same_v<decltype(res), BASE_NS::shared_ptr<TestPtrD>>, "wrong return type");

    // THEN
    EXPECT_EQ(res.get(), nullptr);
}

/**
 * @tc.name: staticPointerCastToDerivedNull
 * @tc.desc: static_pointer_cast should work for shared_ptr if we cast to the wrong derived type, but we supply null
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersSharedPtr, staticPointerCastToNonDerivedNull, testing::ext::TestSize.Level1)
{
    // GIVEN
    BASE_NS::shared_ptr<TestPtr> base = BASE_NS::shared_ptr<TestPtrD>(nullptr);

    struct Sibling : TestPtr {
        int member {};
    };
    // WHEN casting to a wrong type (e.g. sibling)
    auto res = static_pointer_cast<Sibling>(BASE_NS::move(base));

    // THEN
    EXPECT_EQ(res.get(), nullptr);
    static_assert(BASE_NS::is_same_v<decltype(res), BASE_NS::shared_ptr<Sibling>>, "wrong return type");
}