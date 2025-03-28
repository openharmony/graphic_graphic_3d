/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <algorithm>

#include <gtest/gtest.h>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

using namespace testing::ext;

namespace {
class CustomAllocator {
public:
    // simple linear allocator.
    uint8_t buf[42 * sizeof(uint32_t)];
    size_t pos { 0 };
    void Reset()
    {
        pos = 0;
        memset_s(buf, sizeof(buf), 0, sizeof(buf));
    }
    void* alloc(BASE_NS::allocator::size_type size)
    {
        void* ret = &buf[pos];
        pos += size;
        return ret;
    };
    void Free(void*)
    {
        return;
    };
    static void* alloc(void* instance, BASE_NS::allocator::size_type size)
    {
        return ((CustomAllocator*)instance)->alloc(size);
    }
    static void Free(void* instance, void* ptr)
    {
        ((CustomAllocator*)instance)->Free(ptr);
    };
} g_aca;

BASE_NS::allocator ca { &g_aca, CustomAllocator::alloc, CustomAllocator::Free };

struct Thing {
    explicit constexpr Thing(int v) : i(v) {}
    ~Thing() {}
    constexpr Thing(const Thing& o) : i(o.i) {}
    constexpr Thing(Thing&& o) noexcept : i(o.i) {}
    constexpr Thing& operator=(const Thing& o)
    {
        i = o.i;
        return *this;
    }
    constexpr Thing& operator=(Thing&& o) noexcept
    {
        i = o.i;
        return *this;
    }
    constexpr bool operator==(const Thing& o) const
    {
        return i == o.i;
    }
    int i;
};
} // namespace

template<typename T>
class ProtectedVectorTest : public BASE_NS::vector<T> {
public:
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    using iterator = BASE_NS::iterator<BASE_NS::vector<T>>;
    using const_iterator = BASE_NS::const_iterator<BASE_NS::vector<T>>;
    using reverse_iterator = BASE_NS::reverse_iterator<BASE_NS::vector<T>>;
    // using const_reverse_iterator = BASE_NS::reverse_iterator<const_iterator>;

    using size_type = size_t;
    using const_reference = const value_type&;
    using const_pointer = const value_type*;

    // do not need instance of class. Just memory realocation
    void WrapUninitializedDefaultConstruct(pointer first, pointer last)
    {
        return BASE_NS::vector<T>::uninitialized_default_construct(first, last);
    }

    void WrapUninitializedValueConstruct(pointer first, pointer last)
    {
        return BASE_NS::vector<T>::uninitialized_value_construct(first, last);
    }

    void WrapUninitializedCopy(const_pointer first, const_pointer last, pointer dFirst)
    {
        return BASE_NS::vector<T>::uninitialized_copy(first, last, dFirst);
    }

    void WrapUninitializedFill(pointer first, const_pointer last, const_reference value)
    {
        return BASE_NS::vector<T>::uninitialized_fill(first, last, value);
    }

    void WrapUninitializedMove(pointer first, const_pointer last, pointer dFirst)
    {
        return BASE_NS::vector<T>::uninitialized_move(first, last, dFirst);
    }

    template<class InputIt>
    void WrapCopy(pointer pos, InputIt first, InputIt last)
    {
        return BASE_NS::vector<T>::copy(pos, first, last);
    }

    void WrapMove(pointer first, const_pointer last, pointer dFirst)
    {
        return BASE_NS::vector<T>::move(first, last, dFirst);
    }

    pointer WrapInitValue(pointer dst, size_t count)
    {
        return BASE_NS::vector<T>::init_value(dst, count);
    }

    pointer WrapInitFill(pointer dst, size_t count, const_reference value)
    {
        return BASE_NS::vector<T>::init_fill(dst, count, value);
    }

    pointer WrapInitCopy(pointer dst, const_pointer src, size_type size)
    {
        return BASE_NS::vector<T>::init_copy(dst, src, size);
    }

    template<class InputIt>
    iterator WrapInsertImpl(const_iterator pos, InputIt first, InputIt last)
    {
        return BASE_NS::vector<T>::insert_impl(pos, first, last);
    }
};

class VectorTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(VectorTest, SetAllocator, TestSize.Level1)
{
    g_aca.Reset();
    // create with default allocator
    BASE_NS::vector<uint32_t> duh;
    duh.push_back(1);
    // set custom allocator
    duh.setAllocator(ca);
    // add data
    duh.push_back(42);
    ASSERT_EQ(duh[0], 1);
    ASSERT_EQ(duh[1], 42);
    duh.setAllocator(BASE_NS::default_allocator());
    ASSERT_EQ(&duh.getAllocator(), &BASE_NS::default_allocator());
    ASSERT_EQ(duh[0], 1);
    ASSERT_EQ(duh[1], 42);
}

HWTEST_F(VectorTest, SetAllocator02, TestSize.Level1)
{
    g_aca.Reset();
    // create with default allocator
    BASE_NS::vector<uint32_t> duh;
    // add data
    duh.push_back(42);
    duh.setAllocator(ca);
    ASSERT_EQ(&duh.getAllocator(), &ca);

    // make copy, (with the same allocator)
    BASE_NS::vector<uint32_t> duh2(duh);
    ASSERT_EQ(&duh2.getAllocator(), &ca);

    // make copy, (with the default allocator)
    BASE_NS::vector<uint32_t> duh3(duh, BASE_NS::default_allocator());
    ASSERT_EQ(&duh3.getAllocator(), &BASE_NS::default_allocator());
}

HWTEST_F(VectorTest, CustomAllocator, TestSize.Level1)
{
    g_aca.Reset();
    // create with custom allocator
    BASE_NS::vector<uint32_t> duh(ca);
    duh.push_back(42);
    duh.push_back(12);
    ASSERT_EQ(duh[0], 42);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh.size(), 2U);
    ASSERT_GE(duh.capacity(), 2U);
    duh.reserve(5);
    ASSERT_EQ(duh.size(), 2U);
    ASSERT_GE(duh.capacity(), 5U);
    ASSERT_EQ(duh[0], 42);
    ASSERT_EQ(duh[1], 12);
    duh.resize(5);
    ASSERT_EQ(duh.size(), 5U);
    ASSERT_GE(duh.capacity(), 5U);
    ASSERT_EQ(duh[0], 42);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh[2], 0);

    duh.insert(duh.begin(), 4);
    ASSERT_EQ(duh.size(), 6U);
    ASSERT_GE(duh.capacity(), 6U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 42);
    ASSERT_EQ(duh[2], 12);
    ASSERT_EQ(duh[3], 0);

    duh.erase(duh.begin() + 1);
    ASSERT_EQ(duh.size(), 5U);
    ASSERT_GE(duh.capacity(), 6U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh[2], 0);

    // BASE_NS::vector<uint32_t> duh2(duh); Currently uses the same allocator.. which might not be correct? (in this
    // test it's an issue :)
    BASE_NS::vector<uint32_t> duh2; // default constructor uses default_allocator.
    duh2 = duh;
    ASSERT_EQ(duh2.size(), 5U);
    ASSERT_GE(duh2.capacity(), 5U);
    ASSERT_EQ(duh2[0], 4);
    ASSERT_EQ(duh2[1], 12);

    duh2.resize(2);
    ASSERT_EQ(duh2.size(), 2U);
    ASSERT_GE(duh2.capacity(), 5U);
    duh2.shrink_to_fit();
    ASSERT_EQ(duh2.size(), 2U);
    ASSERT_GE(duh2.capacity(), 2U);

    duh.resize(2, 1);
    ASSERT_EQ(duh.size(), 2U);
    ASSERT_GE(duh.capacity(), 2U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 12);

    duh.resize(3, 1);
    ASSERT_EQ(duh.size(), 3U);
    ASSERT_GE(duh.capacity(), 3U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh[2], 1);
}

HWTEST_F(VectorTest, DefaultAllocator, TestSize.Level1)
{
    // create with default allocator
    BASE_NS::vector<uint32_t> duh(BASE_NS::default_allocator());
    duh.push_back(42);
    duh.push_back(12);
    ASSERT_EQ(duh[0], 42);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh.size(), 2U);
    ASSERT_GE(duh.capacity(), 2U);
    duh.reserve(5);
    ASSERT_EQ(duh.size(), 2U);
    ASSERT_GE(duh.capacity(), 5U);
    ASSERT_EQ(duh[0], 42);
    ASSERT_EQ(duh[1], 12);
    duh.resize(5);
    ASSERT_EQ(duh.size(), 5U);
    ASSERT_GE(duh.capacity(), 5U);
    ASSERT_EQ(duh[0], 42);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh[2], 0);
    duh.insert(duh.begin(), 4);
    ASSERT_EQ(duh.size(), 6U);
    ASSERT_GE(duh.capacity(), 6U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 42);
    ASSERT_EQ(duh[2], 12);
    ASSERT_EQ(duh[3], 0);

    duh.erase(duh.begin() + 1);
    ASSERT_EQ(duh.size(), 5U);
    ASSERT_GE(duh.capacity(), 6U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh[2], 0);

    BASE_NS::vector<uint32_t> duh2(duh); // default constructor uses default_allocator.
    ASSERT_EQ(duh2.size(), 5U);
    ASSERT_GE(duh2.capacity(), 5U);
    ASSERT_EQ(duh2[0], 4);
    ASSERT_EQ(duh2[1], 12);

    duh2.resize(2);
    ASSERT_EQ(duh2.size(), 2U);
    ASSERT_GE(duh2.capacity(), 5U);
    duh2.shrink_to_fit();
    ASSERT_EQ(duh2.size(), 2U);
    ASSERT_GE(duh2.capacity(), 2U);

    duh.resize(2, 1);
    ASSERT_EQ(duh.size(), 2U);
    ASSERT_GE(duh.capacity(), 2U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 12);

    duh.resize(3, 1);
    ASSERT_EQ(duh.size(), 3U);
    ASSERT_GE(duh.capacity(), 3U);
    ASSERT_EQ(duh[0], 4);
    ASSERT_EQ(duh[1], 12);
    ASSERT_EQ(duh[2], 1);
}

HWTEST_F(VectorTest, Insert, TestSize.Level1)
{
    BASE_NS::vector<uint32_t> duh(BASE_NS::default_allocator());
    duh.insert(duh.begin(), 99U);
    ASSERT_EQ(duh.size(), 1U);
    ASSERT_EQ(duh[0], 99U);
    duh.insert(duh.end(), 99U);
    ASSERT_EQ(duh.size(), 2U);
    ASSERT_EQ(duh[1], 99U);
    const uint32_t VALUE1 = 1U;
    const uint32_t VALUE2 = 2U;
    duh.insert(duh.begin(), VALUE1);
    ASSERT_EQ(duh.size(), 3U);
    ASSERT_EQ(duh[0], VALUE1);
    duh.insert(duh.begin() + 2, VALUE1);
    ASSERT_EQ(duh.size(), 4U);
    ASSERT_EQ(duh[2], VALUE1);
    duh.insert(duh.end(), VALUE1);
    ASSERT_EQ(duh.size(), 5U);
    ASSERT_EQ(duh[4], VALUE1);

    duh.insert(duh.begin(), 0, VALUE2);
    ASSERT_EQ(duh.size(), 5U);
    duh.insert(duh.begin(), 2, VALUE2);
    ASSERT_EQ(duh.size(), 7U);
    ASSERT_EQ(duh[0], VALUE2);
    ASSERT_EQ(duh[1], VALUE2);
    duh.insert(duh.end(), 2, VALUE2); // 2 values at end are the same ?
    ASSERT_EQ(duh.size(), 9U);
    ASSERT_EQ(duh[7], VALUE2);
    ASSERT_EQ(duh[8], VALUE2);

    // Case when no alloctation is needed
    duh.insert(duh.end(), 99U);
    ASSERT_EQ(duh.size(), 10U);
    ASSERT_EQ(duh[9], 99U);
    duh.insert(duh.end(), VALUE1);
    ASSERT_EQ(duh.size(), 11U);
    ASSERT_EQ(duh[10], VALUE1);
}

HWTEST_F(VectorTest, InsertAndConvertVectorElementType, TestSize.Level1)
{
    // this is valid for std so should be valid for us. (see LUME-1402)
    // ie. if the elements can be converted (basic copy conversion etc) then they can be inserted.
    BASE_NS::vector<short> short_vec;
    short_vec.push_back(1);
    short_vec.push_back(53);
    short_vec.push_back(469);
    BASE_NS::vector<int> int_vec;
    int_vec.insert(int_vec.end(), short_vec.begin(), short_vec.end());
    ASSERT_TRUE(int_vec.size() == short_vec.size());
    ASSERT_TRUE(int_vec[0] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[1] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[2] == (int)short_vec[2]);

    int_vec.insert(int_vec.begin() + 2, short_vec.begin(), short_vec.end());

    ASSERT_TRUE(int_vec.size() == 2 * short_vec.size());
    ASSERT_TRUE(int_vec[0] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[1] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[2] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[3] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[4] == (int)short_vec[2]);
    ASSERT_TRUE(int_vec[5] == (int)short_vec[2]);

    int_vec.reserve(4 * short_vec.size());
    int_vec.insert(int_vec.begin() + 2, short_vec.begin(), short_vec.end());

    ASSERT_TRUE(int_vec.size() == 3 * short_vec.size());
    ASSERT_TRUE(int_vec[0] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[1] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[2] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[3] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[4] == (int)short_vec[2]);
    ASSERT_TRUE(int_vec[5] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[6] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[7] == (int)short_vec[2]);
    ASSERT_TRUE(int_vec[8] == (int)short_vec[2]);

    int_vec.insert(int_vec.end(), short_vec.begin(), short_vec.end());

    ASSERT_TRUE(int_vec.size() == 4 * short_vec.size());
    ASSERT_TRUE(int_vec[0] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[1] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[2] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[3] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[4] == (int)short_vec[2]);
    ASSERT_TRUE(int_vec[5] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[6] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[7] == (int)short_vec[2]);
    ASSERT_TRUE(int_vec[8] == (int)short_vec[2]);
    ASSERT_TRUE(int_vec[9] == (int)short_vec[0]);
    ASSERT_TRUE(int_vec[10] == (int)short_vec[1]);
    ASSERT_TRUE(int_vec[11] == (int)short_vec[2]);
    /*
    // this will fail with "error C2338: Invalid input iterator type" (static_assert for types are not matching)
    CORE_NS::vector<CORE_NS::Math::Vec3> vec3_vec;
    int_vec.insert(int_vec.end(),vec3_vec.begin(),vec3_vec.end());
    */

    // the following is valid. due to class B providing the conversion operator.
    class A {
    public:
        int val { 42 };
    };
    class B {
    public:
        int val { 6 };
        operator A() const
        {
            A tmp;
            tmp.val = val;
            return tmp;
        }
    };
    BASE_NS::vector<A> as;
    BASE_NS::vector<B> bs;
    as.resize(3);
    bs.resize(3);
    as.insert(as.end(), bs.begin(), bs.end());

    ASSERT_TRUE(as[0].val == 42);
    ASSERT_TRUE(as[1].val == 42);
    ASSERT_TRUE(as[2].val == 42);
    ASSERT_TRUE(as[3].val == 6);
    ASSERT_TRUE(as[4].val == 6);
    ASSERT_TRUE(as[5].val == 6);
}

HWTEST_F(VectorTest, InsertPointerIterators, TestSize.Level1)
{
    int data[] = { 135, 32, 1 };
    BASE_NS::vector<int> int_vec;
    int_vec.insert(int_vec.end(), &data[0], &data[0] + BASE_NS::countof(data));

    ASSERT_TRUE(int_vec.size() == BASE_NS::countof(data));
    ASSERT_TRUE(int_vec[0] == data[0]);
    ASSERT_TRUE(int_vec[1] == data[1]);
    ASSERT_TRUE(int_vec[2] == data[2]);
}

HWTEST_F(VectorTest, InsertArrayViewIterators, TestSize.Level1)
{
    {
        int data[] = { 135, 32, 1 };
        BASE_NS::array_view<int> int_av(&data[0], BASE_NS::countof(data));
        BASE_NS::vector<int> int_vec;
        int_vec.insert(int_vec.end(), int_av.begin(), int_av.end());
        ASSERT_TRUE(int_vec.size() == BASE_NS::countof(data));
        ASSERT_TRUE(int_vec[0] == data[0]);
        ASSERT_TRUE(int_vec[1] == data[1]);
        ASSERT_TRUE(int_vec[2] == data[2]);
    }
    {
        int data[] = { 135, 32, 1 };
        BASE_NS::array_view<int> int_av(&data[0], BASE_NS::countof(data));
        BASE_NS::vector<int> int_vec;
        int_vec.insert(
            int_vec.end(), int_av.cbegin(), int_av.cend()); // no difference, InputIt array_view::const_iterator
        ASSERT_TRUE(int_vec.size() == BASE_NS::countof(data));
        ASSERT_TRUE(int_vec[0] == data[0]);
        ASSERT_TRUE(int_vec[1] == data[1]);
        ASSERT_TRUE(int_vec[2] == data[2]);
    }
}

HWTEST_F(VectorTest, InsertVectorIterators, TestSize.Level1)
{
    {
        BASE_NS::vector<int> data;
        data.push_back(146);
        data.push_back(15);
        data.push_back(26);
        BASE_NS::vector<int> int_vec;
        int_vec.insert(int_vec.end(), data.begin(), data.end());
        ASSERT_TRUE(int_vec.size() == data.size());
        ASSERT_TRUE(int_vec[0] == data[0]);
        ASSERT_TRUE(int_vec[1] == data[1]);
        ASSERT_TRUE(int_vec[2] == data[2]);
    }
    {
        BASE_NS::vector<int> data;
        data.push_back(146);
        data.push_back(15);
        data.push_back(26);
        BASE_NS::vector<int> int_vec;
        int_vec.insert(int_vec.end(), data.cbegin(), data.cend());
        ASSERT_TRUE(int_vec.size() == data.size());
        ASSERT_TRUE(int_vec[0] == data[0]);
        ASSERT_TRUE(int_vec[1] == data[1]);
        ASSERT_TRUE(int_vec[2] == data[2]);
    }
}

HWTEST_F(VectorTest, Append, TestSize.Level1)
{
    // append(count, value)
    {
        BASE_NS::vector<uint32_t> duh(BASE_NS::default_allocator());

        duh.append(0U, 42U);
        ASSERT_TRUE(duh.empty());

        duh.append(1U, 42U);
        ASSERT_FALSE(duh.empty());
        ASSERT_EQ(duh.size(), 1U);
        ASSERT_EQ(duh[0U], 42U);

        duh.append(1U, 43U);
        ASSERT_EQ(duh.size(), 2U);
        ASSERT_EQ(duh[0U], 42U);
        ASSERT_EQ(duh[1U], 43U);

        duh.append(3U, 44U);
        ASSERT_EQ(duh.size(), 5U);
        ASSERT_EQ(duh[0U], 42U);
        ASSERT_EQ(duh[1U], 43U);
        ASSERT_EQ(duh[2U], 44U);
        ASSERT_EQ(duh[3U], 44U);
        ASSERT_EQ(duh[4U], 44U);
    }

    // append(begin, end)
    {
        static constexpr uint32_t VALUES[] = { 0U, 1U, 2U, 3U };

        BASE_NS::vector<uint32_t> duh(BASE_NS::default_allocator());
        duh.append(std::begin(VALUES), std::end(VALUES));
        ASSERT_EQ(duh.size(), 4U);
        ASSERT_EQ(duh[0U], 0U);
        ASSERT_EQ(duh[1U], 1U);
        ASSERT_EQ(duh[2U], 2U);
        ASSERT_EQ(duh[3U], 3U);

        static constexpr auto VIEW = BASE_NS::array_view(VALUES);
        duh.append(std::begin(VIEW), std::end(VIEW));
        ASSERT_EQ(duh.size(), 8U);
        ASSERT_EQ(duh[0U], 0U);
        ASSERT_EQ(duh[1U], 1U);
        ASSERT_EQ(duh[2U], 2U);
        ASSERT_EQ(duh[3U], 3U);
        ASSERT_EQ(duh[4U], 0U);
        ASSERT_EQ(duh[5U], 1U);
        ASSERT_EQ(duh[6U], 2U);
        ASSERT_EQ(duh[7U], 3U);
    }
}

using namespace BASE_NS;

HWTEST_F(VectorTest, Default, TestSize.Level1)
{
    // Test some types
    BASE_NS::vector<int> vecInt;
    BASE_NS::vector<bool> vecBool;
    BASE_NS::vector<float> vecFloat;
    BASE_NS::vector<double> vecDouble;
    BASE_NS::vector<BASE_NS::string> vecString;
    BASE_NS::vector<string_view> vecStringView;
    BASE_NS::vector<BASE_NS::vector<int>> vecVec;
}

template<typename T>
T* g_allocate(BASE_NS::vector<T> vec, int size)
{
    // Allocate space for size(s) elements
    return static_cast<T*>(vec.getAllocator().alloc(vec.getAllocator().instance, size));
}

HWTEST_F(VectorTest, Initialization, TestSize.Level1)
{
    // Test some types
    BASE_NS::vector<int> vecInt;
    vecInt.push_back(0);
    vecInt.push_back(5);
    ASSERT_EQ(vecInt[0], 0);
    ASSERT_EQ(vecInt[1], 5);

    BASE_NS::vector<bool> vecBool;
    vecBool.push_back(true);
    vecBool.push_back(false);
    ASSERT_EQ(vecBool[0], true);
    ASSERT_EQ(vecBool[1], false);

    BASE_NS::vector<float> vecFloat;
    vecFloat.push_back(0.000001f);
    vecFloat.push_back(9999999.9f);
    ASSERT_FLOAT_EQ(vecFloat[0], 0.000001f);
    ASSERT_FLOAT_EQ(vecFloat[1], 9999999.9f);

    BASE_NS::vector<double> vecDouble;
    vecDouble.push_back(0.0000000001);
    vecDouble.push_back(99.999998);
    ASSERT_EQ(vecDouble[0], 0.0000000001);
    ASSERT_EQ(vecDouble[1], 99.999998);

    BASE_NS::vector<BASE_NS::string> vecString;
    vecString.push_back("Hello");
    vecString.push_back(" WoRlD");
    ASSERT_STREQ(vecString[0].c_str(), "Hello");
    ASSERT_STREQ(vecString[1].c_str(), " WoRlD");

    BASE_NS::vector<string_view> vecStringView;
    vecStringView.push_back("Hello");
    vecStringView.push_back(" WoRlD");
    ASSERT_EQ(vecStringView[0], "Hello");
    ASSERT_EQ(vecStringView[1], " WoRlD");

    BASE_NS::vector<BASE_NS::vector<int>> vecVec;
    BASE_NS::vector<int> innerVec;
    innerVec.push_back(0);
    innerVec.push_back(5);
    vecVec.push_back(innerVec);
    ASSERT_EQ(vecVec[0][0], 0);
    ASSERT_EQ(vecVec[0][1], 5);

    // Operator=
    vecInt[0] = 6;
    ASSERT_EQ(vecInt[0], 6);
}

HWTEST_F(VectorTest, InitialzationByPointers, TestSize.Level1)
{
    {
        BASE_NS::vector<int> data = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt = BASE_NS::vector(&(*data.begin()), &(*data.end()));
        ASSERT_TRUE(vecInt.size() == data.size());
        ASSERT_TRUE(vecInt[0] == data[0]);
        ASSERT_TRUE(vecInt[1] == data[1]);
        ASSERT_TRUE(vecInt[2] == data[2]);
    }
    {
        BASE_NS::vector<int> data = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt = BASE_NS::vector<int>(data.begin(), data.end());
        ASSERT_TRUE(vecInt.size() == data.size());
        ASSERT_TRUE(vecInt[0] == data[0]);
        ASSERT_TRUE(vecInt[1] == data[1]);
        ASSERT_TRUE(vecInt[2] == data[2]);
    }
}

HWTEST_F(VectorTest, SetAllocators, TestSize.Level1)
{
    allocator a1 = default_allocator();
    allocator a2 = default_allocator();
    BASE_NS::vector<int> vecInt1;
    BASE_NS::vector<int> vecInt2;
    // poisoning allocator for different instance values
    ASSERT_TRUE(ClearToValue(&a2, sizeof(int), POISON, sizeof(int)));
    vecInt1.setAllocator(a1);
    vecInt2.setAllocator(a2);
    ASSERT_FALSE(vecInt1.getAllocator().instance == vecInt2.getAllocator().instance);
    const auto a2_old = vecInt2.getAllocator();
    a2 = a1;
    ASSERT_TRUE(vecInt1.getAllocator().instance == vecInt2.getAllocator().instance);
    ASSERT_FALSE(a2_old.instance == vecInt2.getAllocator().instance);
    vecInt2.setAllocator(a1);
    ASSERT_TRUE(vecInt1.getAllocator().instance == vecInt2.getAllocator().instance);

    const BASE_NS::vector<int> vecInt3(ca);
    const auto a3_const = vecInt3.getAllocator();
    ASSERT_TRUE(ca.instance == a3_const.instance);
}

HWTEST_F(VectorTest, InitialzationByList, TestSize.Level1)
{
    BASE_NS::vector<int> vecBase;
    vecBase = { 1, 2, 3, 4 };
    BASE_NS::vector<int> vec = vecBase;
    BASE_NS::vector<int> vec2;
    ASSERT_EQ(vecBase[0], vec[0]);
    ASSERT_EQ(vecBase[1], vec[1]);
    ASSERT_EQ(vecBase[2], vec[2]);
    ASSERT_EQ(vecBase[3], vec[3]);

    vec2 = static_cast<BASE_NS::vector<int>&&>(vec); // without erase
    ASSERT_EQ(vecBase[0], vec2[0]);
    ASSERT_EQ(vecBase[1], vec2[1]);
    ASSERT_EQ(vecBase[2], vec2[2]);
    ASSERT_EQ(vecBase[3], vec2[3]);
    ASSERT_EQ(vec.size(), 0);

    vec2 = { 0, 0, 0 };
    ASSERT_NE(vecBase[0], vec2[0]);
    ASSERT_NE(vecBase[1], vec2[1]);
    ASSERT_NE(vecBase[2], vec2[2]);

    vec2 = static_cast<BASE_NS::vector<int>&&>(vec); // with erase but from already empty
    ASSERT_EQ(vec2.size(), 0);

    vec = vecBase;
    vec2 = static_cast<BASE_NS::vector<int>&&>(vec); // with erase but from already empty
    ASSERT_EQ(vecBase[0], vec2[0]);
    ASSERT_EQ(vecBase[1], vec2[1]);
    ASSERT_EQ(vecBase[2], vec2[2]);
    ASSERT_EQ(vecBase[3], vec2[3]);
}

HWTEST_F(VectorTest, InitialzationByConst, TestSize.Level1)
{
    const int cInt = 5;
    BASE_NS::vector<int> vecInt = BASE_NS::vector(2, cInt);
    ASSERT_EQ(vecInt[0], 5);
    ASSERT_EQ(vecInt[1], 5);

    BASE_NS::vector<bool> vecBool(2, true);
    ASSERT_EQ(vecBool[0], true);
    ASSERT_EQ(vecBool[1], true);

    BASE_NS::vector<float> vecFloat(2, 0.000001f);
    ASSERT_FLOAT_EQ(vecFloat[0], 0.000001f);
    ASSERT_FLOAT_EQ(vecFloat[1], 0.000001f);

    BASE_NS::vector<double> vecDouble(2, 0.0000000001);
    ASSERT_EQ(vecDouble[0], 0.0000000001);
    ASSERT_EQ(vecDouble[1], 0.0000000001);

    BASE_NS::vector<BASE_NS::string> vecString(2, "Hello");
    ASSERT_STREQ(vecString[0].c_str(), "Hello");
    ASSERT_STREQ(vecString[1].c_str(), "Hello");

    BASE_NS::vector<string_view> vecStringView(2, "Hello");
    ASSERT_EQ(vecStringView[0], "Hello");
    ASSERT_EQ(vecStringView[1], "Hello");

    BASE_NS::vector<BASE_NS::vector<int>> vecVec(1, BASE_NS::vector(2, 3));
    ASSERT_EQ(vecVec[0][0], 3);
    ASSERT_EQ(vecVec[0][1], 3);

    BASE_NS::vector<int> vecEmpty(0, 10000);
    ASSERT_EQ(vecEmpty.size(), 0);
}

HWTEST_F(VectorTest, Poisoning, TestSize.Level1)
{
    BASE_NS::vector<int> vecInt;
    vecInt.push_back(1);
    vecInt.push_back(2);
    vecInt.push_back(3);
    vecInt.pop_back();
    // without poisoning, value is not changed.
    ASSERT_EQ(vecInt.data()[2], 3);
    ASSERT_TRUE(ClearToValue(&vecInt.data()[2], sizeof(int), POISON, sizeof(int)));
    // with poisoning, value is changed to huge constant.
    ASSERT_FALSE(vecInt.data()[2] == 3);
    // poisoning of nullpointer
    ASSERT_FALSE(ClearToValue(nullptr, sizeof(int), POISON, sizeof(int)));
    // poisoning 2 elements, cnt>dist -> bad cnt
    ASSERT_TRUE(ClearToValue(&vecInt[0], sizeof(int), POISON, sizeof(int) * 2));
    ASSERT_FALSE(vecInt[0] == 1);
    ASSERT_TRUE(vecInt[1] == 2);
    // poisoning 2 elements, cnt == dist
    ASSERT_TRUE(ClearToValue(&vecInt[0], sizeof(int) * 2, POISON, sizeof(int) * 2));
    ASSERT_FALSE(vecInt[0] == 1);
    ASSERT_FALSE(vecInt[1] == 2);
}

HWTEST_F(VectorTest, ElementAccess, TestSize.Level1)
{
    {
        BASE_NS::vector<int> vecInt;
        vecInt.push_back(2);
        vecInt.push_back(5);

        // At
        ASSERT_EQ(vecInt.at(0), 2);
        ASSERT_EQ(vecInt.at(1), 5);

        // Operator[]
        ASSERT_EQ(vecInt[0], 2);
        ASSERT_EQ(vecInt[1], 5);

        // Front
        ASSERT_EQ(vecInt.front(), 2);

        // Back
        ASSERT_EQ(vecInt.back(), 5);

        // Data
        int* p = vecInt.data();
        ASSERT_EQ(p[0], 2);
        ASSERT_EQ(p[1], 5);
    }
    {
        const BASE_NS::vector<int> vecInt = { 2, 5 };

        // At
        ASSERT_EQ(vecInt.at(0), 2);
        ASSERT_EQ(vecInt.at(1), 5);

        // Operator[]
        ASSERT_EQ(vecInt[0], 2);
        ASSERT_EQ(vecInt[1], 5);

        // Front
        ASSERT_EQ(vecInt.front(), 2);

        // Back
        ASSERT_EQ(vecInt.back(), 5);

        // Data
        const int* p = vecInt.data();
        ASSERT_EQ(p[0], 2);
        ASSERT_EQ(p[1], 5);
    }
}

HWTEST_F(VectorTest, ProtectedUninitializedMemory, TestSize.Level1)
{
    ProtectedVectorTest<int> classInstance;
    ProtectedVectorTest<uint32_t> classInstanceU32;
    {
        BASE_NS::vector<int> vecInt;
        vecInt.resize(0); // allocate 0 memory
        classInstance.WrapUninitializedDefaultConstruct(vecInt.begin().ptr(), vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        BASE_NS::vector<int> vecInt;
        vecInt.resize(2); // allocate some memory for int without value
        classInstance.WrapUninitializedDefaultConstruct(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr());
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
    }
    {
        BASE_NS::vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.WrapUninitializedValueConstruct(vecInt.begin().ptr(), vecInt.begin().ptr());
    }
    {
        BASE_NS::vector<int> vecInt; // allocate some memory for int with default value
        vecInt.resize(2);
        classInstance.WrapUninitializedValueConstruct(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr());
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
    }
    {
        BASE_NS::vector<int> vecBase = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.WrapUninitializedCopy(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        BASE_NS::vector<int> vecBase = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt; // allocate some memory for int with values from vecBase
        vecInt.resize(2);
        classInstance.WrapUninitializedCopy(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[1]);
    }
    {
        BASE_NS::vector<uint32_t> vecBase = { 1, 2, 3 };
        BASE_NS::vector<uint32_t> vecInt; // allocate some memory for int with values from vecBase
        vecInt.resize(2);
        classInstanceU32.WrapUninitializedCopy(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[1]);
    }
    {
        BASE_NS::vector<int> vecBase = { 1, 2, 3 };
        BASE_NS::vector<int>::const_reference& A = vecBase.front();
        BASE_NS::vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.WrapUninitializedFill(vecInt.begin().ptr(), vecInt.begin().ptr(), A);
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        BASE_NS::vector<int> vecBase = { 1, 2, 3 };
        BASE_NS::vector<int>::const_reference& A = vecBase.front();
        BASE_NS::vector<int> vecInt; // unitialize some memory with vecBase[0]
        vecInt.resize(2);
        classInstance.WrapUninitializedFill(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), A);
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[0]);
    }
    {
        BASE_NS::vector<uint32_t> vecBase = { 1, 2, 3 };
        BASE_NS::vector<uint32_t>::const_reference& A = vecBase.front();
        BASE_NS::vector<uint32_t> vecInt; // unitialize some memory with vecBase[0]
        vecInt.resize(2);
        classInstanceU32.WrapUninitializedFill(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), A);
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[0]);
    }
    {
        BASE_NS::vector<int> vecBase = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.WrapUninitializedMove(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        BASE_NS::vector<int> vecBase = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt; // allocate some memory for int with values from vecBase[2]
        vecInt.resize(2);
        classInstance.WrapUninitializedMove(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
        ASSERT_EQ(vecBase[0], 0);
        ASSERT_EQ(vecBase[1], 0);
        ASSERT_EQ(vecBase[2], 3);
    }
    {
        BASE_NS::vector<uint32_t> vecBase = { 1, 2, 3 };
        BASE_NS::vector<uint32_t> vecInt; // allocate some memory for int with values from vecBase[2]
        vecInt.resize(2);
        classInstanceU32.WrapUninitializedMove(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
        ASSERT_EQ(vecBase[0], 0);
        ASSERT_EQ(vecBase[1], 0);
        ASSERT_EQ(vecBase[2], 3);
    }
}

HWTEST_F(VectorTest, ProtectedInitializedMemory, TestSize.Level1)
{
    ProtectedVectorTest<int> classInstance;
    ProtectedVectorTest<uint32_t> classInstanceU32;

    // move values, not references
    {
        BASE_NS::vector<int> vecInt = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt2 = { 3, 2, 1 };
        BASE_NS::vector<uint32_t> vecUint = { 99U, 88U, 77U };

        classInstance.WrapCopy(&(*vecInt2.begin()), vecInt.begin().ptr(), vecInt.begin().ptr());
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.WrapCopy<uint32_t*>(&(*vecInt2.begin()), &(*vecUint.begin()), &(*vecUint.begin()));
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.WrapCopy<iterator<BASE_NS::vector<int>>>(&(*vecInt2.begin()), vecInt.begin(), vecInt.begin());
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.WrapCopy(&(*vecInt2.begin()), vecInt.begin().ptr(), &(*vecInt.end()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);

        classInstance.WrapCopy<uint32_t*>(&(*vecInt2.begin()), &(*vecUint.begin()), &(*vecUint.end()));
        ASSERT_EQ(vecInt2[0], 99U);
        ASSERT_EQ(vecInt2[1], 88U);
        ASSERT_EQ(vecInt2[2], 77U);
        ASSERT_EQ(vecUint[0], 99U);
        ASSERT_EQ(vecUint[1], 88U);
        ASSERT_EQ(vecUint[2], 77U);

        classInstance.WrapCopy<iterator<BASE_NS::vector<int>>>(&(*vecInt2.begin()), vecInt.begin(), vecInt.end());
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
    }

    // move values, not references
    {
        BASE_NS::vector<int> vecInt = { 1, 2, 3 };
        BASE_NS::vector<int> vecInt2 = { 3, 2, 1 };

        classInstance.WrapMove(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.WrapMove(vecInt.begin().ptr(), &(*(vecInt.begin() + 2)), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);

        classInstance.WrapMove(vecInt.begin().ptr(), &(*(vecInt.end())), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
    }

    {
        BASE_NS::vector<uint32_t> vecInt = { 1, 2, 3 };
        BASE_NS::vector<uint32_t> vecInt2 = { 3, 2, 1 };

        classInstanceU32.WrapMove(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstanceU32.WrapMove(vecInt.begin().ptr(), &(*(vecInt.begin() + 2)), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);

        classInstanceU32.WrapMove(vecInt.begin().ptr(), &(*(vecInt.end())), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
    }
}
HWTEST_F(VectorTest, ProtectedPointerMemoryActions, TestSize.Level1)
{
    ProtectedVectorTest<int> classInstance;
    ProtectedVectorTest<uint32_t> classInstanceU32;
    {
        BASE_NS::vector<int> vecInt;
        int* p;

        vecInt.resize(0);
        p = classInstance.WrapInitValue(vecInt.begin().ptr(), 0);
        ASSERT_EQ(p, vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);

        vecInt.resize(2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);

        p = classInstance.WrapInitValue(vecInt.begin().ptr(), 2);
        ASSERT_EQ(p, &(*(vecInt.begin() + 2)));
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
    }

    {
        BASE_NS::vector<int> vecInt;
        int* p;

        vecInt.resize(0);
        p = classInstance.WrapInitFill(vecInt.begin().ptr(), 0, 1);
        ASSERT_EQ(p, vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);

        vecInt.resize(2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);

        p = classInstance.WrapInitFill(vecInt.begin().ptr(), 2, 1);
        ASSERT_EQ(p, &(*(vecInt.begin() + 2)));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 1);

        // refresh
        p = classInstance.WrapInitValue(&(*(vecInt.begin() + 1)), 1);
        ASSERT_EQ(p, &(*(vecInt.begin() + 2)));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 0);
    }

    {
        BASE_NS::vector<int> vecInt;
        BASE_NS::vector<int> vecInt2 = { 1, 2, 3 };
        int* p;

        vecInt.resize(0);
        p = classInstance.WrapInitCopy(vecInt.begin().ptr(), &(*vecInt2.begin()), 0);
        ASSERT_EQ(p, vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);

        vecInt.resize(4);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
        ASSERT_EQ(vecInt[2], 0);
        ASSERT_EQ(vecInt[3], 0);

        p = classInstance.WrapInitCopy(vecInt.begin().ptr(), &(*vecInt2.begin()), 3);
        ASSERT_EQ(p, &(*(vecInt.begin() + 3)));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
        ASSERT_EQ(vecInt[3], 0);

        p = classInstance.WrapInitCopy(&(*(p - 2)), &(*vecInt2.begin()), 3);
        ASSERT_EQ(p, &(*(vecInt.end())));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 1);
        ASSERT_EQ(vecInt[2], 2);
        ASSERT_EQ(vecInt[3], 3);
    }

    {
        BASE_NS::vector<int> vecInt = { 1, 2, 3 };
        BASE_NS::vector<uint32_t> vecUint = { 99U, 88U, 77U };
        iterator<BASE_NS::vector<int>> pInt; // here pointer to beginning, not end.

        classInstance.resize(0);
        pInt = classInstance.WrapInsertImpl<int*>(classInstance.begin(), vecInt.begin().ptr(), vecInt.begin().ptr());
        ASSERT_EQ(pInt, classInstance.begin());
        ASSERT_EQ(classInstance.size(), 0);

        classInstance.resize(4);
        ASSERT_EQ(classInstance[0], 0);
        ASSERT_EQ(classInstance[1], 0);
        ASSERT_EQ(classInstance[2], 0);
        ASSERT_EQ(classInstance[3], 0);

        pInt = classInstance.WrapInsertImpl<iterator<BASE_NS::vector<int>>>(
            classInstance.begin(), vecInt.begin(), vecInt.end());
        ASSERT_EQ(pInt, classInstance.begin());
        ASSERT_EQ(classInstance[0], 1);
        ASSERT_EQ(classInstance[1], 2);
        ASSERT_EQ(classInstance[2], 3);
        ASSERT_EQ(classInstance[3], 0);

        pInt = classInstance.WrapInsertImpl<uint32_t*>(
            (classInstance.begin() + 1), vecUint.begin().ptr(), vecUint.end().ptr());
        ASSERT_EQ(pInt, classInstance.begin() + 1);
        ASSERT_EQ(classInstance[0], 1);
        ASSERT_EQ(classInstance[1], 99);
        ASSERT_EQ(classInstance[2], 88);
        ASSERT_EQ(classInstance[3], 77);
    }
}
HWTEST_F(VectorTest, Iterators, TestSize.Level1)
{
    int vec[5] = { 1, 2, 4, 8, 16 };
    BASE_NS::vector<int> vecInt { 1, 2, 4, 8, 16 };

    // Iterator begin, end
    int index = 0;
    for (auto it = vecInt.begin(); it != vecInt.end(); ++it) {
        ASSERT_EQ(*it, vec[index]);
        index++;
    };

    index = 0;
    for (auto it = vecInt.cbegin(); it != vecInt.cend(); ++it) {
        ASSERT_EQ(*it, vec[index]);
        index++;
    };

    index = 4;
    for (auto it = vecInt.rbegin(); it != vecInt.rend(); ++it) {
        ASSERT_EQ(*it, vec[index]);
        index--;
    };

    index = 4;
    for (auto it = vecInt.crbegin(); it != vecInt.crend(); ++it) {
        ASSERT_EQ(*it, vec[index]);
        index--;
    };
}

HWTEST_F(VectorTest, Capacity, TestSize.Level1)
{
    BASE_NS::vector<int> vecInt;
    const int nEle = 1000;

    // Empty
    ASSERT_TRUE(vecInt.empty());

    // Max size
    ASSERT_GT(vecInt.max_size(), 0U);

    // Reserve
    vecInt.reserve(nEle);

    // Size & Capacity
    ASSERT_EQ(vecInt.capacity(), nEle);
    ASSERT_EQ(vecInt.capacity_in_bytes(), nEle * sizeof(int));
    ASSERT_EQ(vecInt.size(), 0U);
    ASSERT_EQ(vecInt.size_in_bytes(), 0U * sizeof(int));
    ASSERT_TRUE(vecInt.empty());

    const int nEleHalf = nEle >> 1;
    for (int i = 0; i < nEleHalf; ++i)
        vecInt.push_back(i % 2);
    ASSERT_EQ(vecInt.capacity(), nEle);
    ASSERT_EQ(vecInt.capacity_in_bytes(), nEle * sizeof(int));
    ASSERT_EQ(vecInt.size(), nEleHalf);
    ASSERT_EQ(vecInt.size_in_bytes(), nEleHalf * sizeof(int));
    ASSERT_FALSE(vecInt.empty());

    // Shrink to fit
    vecInt.shrink_to_fit();
    ASSERT_EQ(vecInt.capacity(), nEleHalf);
    ASSERT_EQ(vecInt.capacity_in_bytes(), nEleHalf * sizeof(int));
    ASSERT_EQ(vecInt.size(), nEleHalf);
    ASSERT_EQ(vecInt.size_in_bytes(), nEleHalf * sizeof(int));
}

HWTEST_F(VectorTest, Assign, TestSize.Level1)
{
    {
        BASE_NS::vector<int> vecA;
        BASE_NS::vector<int> vecB;
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
    }
    // assign to smaller (will allocate and copy construct)
    {
        BASE_NS::vector<int> vecA = { 1 };
        BASE_NS::vector<int> vecB = { 2, 3, 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 3U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to smaller, but with extra capacity (will copy assign and copy construct)
    {
        BASE_NS::vector<int> vecA = { 1, 2 };
        vecA.reserve(16);
        BASE_NS::vector<int> vecB = { 3, 4, 5 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
        EXPECT_EQ(vecA.size(), 3U);
    }
    // assign to larger (will allocate and copy construct)
    {
        BASE_NS::vector<int> vecA = { 1, 2, 3 };
        BASE_NS::vector<int> vecB = { 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
        EXPECT_EQ(vecA.size(), 1U);
    }
    // assign to larger but with bigger capacity (will copy assign and destroy elements)
    {
        BASE_NS::vector<int> vecA = { 1, 2, 3 };
        vecA.reserve(16);
        BASE_NS::vector<int> vecB = { 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
        EXPECT_EQ(vecA.size(), 1U);
    }

    // do the same with A non-trivially destructable and copyable
    {
        BASE_NS::vector<Thing> vecA;
        BASE_NS::vector<Thing> vecB;
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
    }
    // assign to smaller (will allocate and copy construct)
    {
        BASE_NS::vector<Thing> vecA = { Thing(1) };
        BASE_NS::vector<Thing> vecB = { Thing(2), Thing(3), Thing(4) };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 3U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to smaller, but with extra capacity (will copy assign and copy construct)
    {
        BASE_NS::vector<Thing> vecA = { Thing(1), Thing(2) };
        vecA.reserve(16);
        BASE_NS::vector<Thing> vecB = { Thing(3), Thing(4), Thing(5) };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 3U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to larger (will allocate and copy construct)
    {
        BASE_NS::vector<Thing> vecA = { Thing(1), Thing(2), Thing(3) };
        BASE_NS::vector<Thing> vecB = { Thing(4) };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 1U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to larger but with bigger capacity (will copy assign and destroy elements)
    {
        BASE_NS::vector<Thing> vecA = { Thing(1), Thing(2), Thing(3) };
        vecA.reserve(16);
        BASE_NS::vector<Thing> vecB = { Thing(4) };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 1U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
}

template<typename T>
struct Foo {
    int n;
    T x;
    Foo(int val1, T val2) : n(val1), x(val2) {}
};

HWTEST_F(VectorTest, Modifiers, TestSize.Level1)
{
    BASE_NS::vector<Foo<float>> vFoo;
    BASE_NS::vector<int> vecInt { 1, 2, 4, 8, 16 };

    // Push back
    vecInt.push_back(32);
    ASSERT_EQ(*(vecInt.end() - 1), 32);

    // Erase
    vecInt.erase(vecInt.begin() + 2);

    ASSERT_EQ(vecInt[2], 8);
    ASSERT_EQ(vecInt[1], 2);
    vecInt.erase(vecInt.end());
    ASSERT_EQ(*(vecInt.end() - 1), 32);
    vecInt.erase(vecInt.end() - 1);
    ASSERT_EQ(*(vecInt.end() - 1), 16);

    // Emplace
    vFoo.emplace(vFoo.begin(), 2, 3.1416f);
    ASSERT_EQ(vFoo[0].n, 2);
    ASSERT_FLOAT_EQ(vFoo[0].x, 3.1416f);

    vecInt.emplace(vecInt.begin() + 2, 4);
    ASSERT_EQ(vecInt[2], 4);

    // Emplace back
    vecInt.emplace_back(32);
    ASSERT_EQ(*(vecInt.end() - 1), 32);

    // Insert
    vFoo.insert(vFoo.end(), Foo(0, 0.2f));
    ASSERT_EQ(vFoo[1].n, 0);
    ASSERT_FLOAT_EQ(vFoo[1].x, 0.2f);

    vecInt.insert(vecInt.begin(), 0);

    // Clear
    ASSERT_GT(vecInt.size(), 0U);
    vecInt.clear();
    ASSERT_EQ(vecInt.size(), 0U);
}