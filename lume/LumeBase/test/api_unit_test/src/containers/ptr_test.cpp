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

#include <gtest/gtest.h>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/unique_ptr.h>

using namespace BASE_NS;
using namespace testing::ext;

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

class PtrTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

// ToDo same tests for array
HWTEST_F(PtrTest, UniquePtr_destructor, TestSize.Level1)
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
}

HWTEST_F(PtrTest, UniquePtr_destructor001, TestSize.Level1)
{
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
}

HWTEST_F(PtrTest, UniquePtr_destructor002, TestSize.Level1)
{
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

HWTEST_F(PtrTest, UniquePtr_makeUnique, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_get, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_release, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_resetNull, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_resetNonNull, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_operatorBool, TestSize.Level1)
{
    // operator bool
    TestPtr test;
    auto ptr = unique_ptr<TestPtr, TestPtr>(nullptr, test);
    EXPECT_FALSE(ptr);
    ptr.reset(&test);
    EXPECT_TRUE(ptr);
}

HWTEST_F(PtrTest, UniquePtr_assignNull, TestSize.Level1)
{
    // operator =
    TestPtr test;
    {
        auto ptr = unique_ptr<TestPtr, TestPtr>(&test, test);
        ptr = nullptr;
        EXPECT_TRUE(test.deleted);
    }
}

HWTEST_F(PtrTest, UniquePtr_assignNonNull, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_assignNonPointer, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_moveConstructor, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_swap, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_destructorArray, TestSize.Level1)
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
}

HWTEST_F(PtrTest, UniquePtr_destructorArray001, TestSize.Level1)
{
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

HWTEST_F(PtrTest, UniquePtr_releaseArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_resetNullArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_resetNonNullArray, TestSize.Level1)
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
    if (int(test1->ref) >= 0) {
        delete[] test1;
    }
    if (int(test2->ref) >= 0) {
        delete[] test2;
    }
}

HWTEST_F(PtrTest, UniquePtr_getArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_operatorIndexArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_operatorBoolArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_assignNullArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_assignNonNullArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_assignNonPointerArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_moveConstructorArray, TestSize.Level1)
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

HWTEST_F(PtrTest, UniquePtr_swapArray, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_destructor, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_get, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_release, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_resetNull, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_resetNonNull, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_operatorBool, TestSize.Level1)
{
    // operator bool
    TestPtr test;
    auto ptr = refcnt_ptr<TestPtr>(nullptr);
    EXPECT_FALSE(ptr);
    ptr.reset(&test);
    EXPECT_TRUE(ptr);
}

HWTEST_F(PtrTest, RefCntPtr_assign, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_moveAssign, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_moveConstructor, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_copyConstructor, TestSize.Level1)
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

HWTEST_F(PtrTest, RefCntPtr_swap, TestSize.Level1)
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
