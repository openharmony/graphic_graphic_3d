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

#include <algorithm>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

#include "test_framework.h"

namespace {
class CustomAllocator {
public:
    // simple linear allocator.
    uint8_t buf[42 * sizeof(uint32_t)];
    size_t pos { 0 };
    void reset()
    {
        pos = 0;
        memset(buf, 0, sizeof(buf));
    }
    void* alloc(BASE_NS::allocator::size_type size)
    {
        void* ret = &buf[pos];
        pos += size;
        return ret;
    };
    void free(void*)
    {
        return;
    };
    static void* alloc(void* instance, BASE_NS::allocator::size_type size)
    {
        return ((CustomAllocator*)instance)->alloc(size);
    }
    static void free(void* instance, void* ptr)
    {
        ((CustomAllocator*)instance)->free(ptr);
    };
} aca;

BASE_NS::allocator ca { &aca, CustomAllocator::alloc, CustomAllocator::free };

struct Thing {
    constexpr Thing(int v) : i(v) {}
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
class protectedVectorTest : public BASE_NS::vector<T> {
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
    void wrap_uninitialized_default_construct(pointer first, pointer last)
    {
        return BASE_NS::vector<T>::uninitialized_default_construct(first, last);
    }

    void wrap_uninitialized_value_construct(pointer first, pointer last)
    {
        return BASE_NS::vector<T>::uninitialized_value_construct(first, last);
    }

    void wrap_uninitialized_copy(const_pointer first, const_pointer last, pointer d_first)
    {
        return BASE_NS::vector<T>::uninitialized_copy(first, last, d_first);
    }

    void wrap_uninitialized_fill(pointer first, const_pointer last, const_reference value)
    {
        return BASE_NS::vector<T>::uninitialized_fill(first, last, value);
    }

    void wrap_uninitialized_move(pointer first, const_pointer last, pointer d_first)
    {
        return BASE_NS::vector<T>::uninitialized_move(first, last, d_first);
    }

    template<class InputIt>
    void wrap_copy(pointer pos, InputIt first, InputIt last)
    {
        return BASE_NS::vector<T>::copy(pos, first, last);
    }

    void wrap_move(pointer first, const_pointer last, pointer d_first)
    {
        return BASE_NS::vector<T>::move(first, last, d_first);
    }

    pointer wrap_init_value(pointer dst, size_t count)
    {
        return BASE_NS::vector<T>::init_value(dst, count);
    }

    pointer wrap_init_fill(pointer dst, size_t count, const_reference value)
    {
        return BASE_NS::vector<T>::init_fill(dst, count, value);
    }

    pointer wrap_init_copy(pointer dst, const_pointer src, size_type size)
    {
        return BASE_NS::vector<T>::init_copy(dst, src, size);
    }

    template<class InputIt>
    iterator wrap_insert_impl(const_iterator pos, InputIt first, InputIt last)
    {
        return BASE_NS::vector<T>::insert_impl(pos, first, last);
    }
};

/**
 * @tc.name: SetAllocator
 * @tc.desc: Tests for Set Allocator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, SetAllocator, testing::ext::TestSize.Level1)
{
    aca.reset();
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

/**
 * @tc.name: SetAllocator2
 * @tc.desc: Tests for Set Allocator2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, SetAllocator2, testing::ext::TestSize.Level1)
{
    aca.reset();
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

/**
 * @tc.name: CustomAllocator
 * @tc.desc: Tests for Custom Allocator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, CustomAllocator, testing::ext::TestSize.Level1)
{
    aca.reset();
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

/**
 * @tc.name: DefaultAllocator
 * @tc.desc: Tests for Default Allocator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, DefaultAllocator, testing::ext::TestSize.Level1)
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

/**
 * @tc.name: insert
 * @tc.desc: Tests for Insert. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ConteinersVector, insert, testing::ext::TestSize.Level1)
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

/**
 * @tc.name: insert_and_convert_vector_element_type
 * @tc.desc: Tests for Insertandconvertvectorelementtype. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, insert_and_convert_vector_element_type, testing::ext::TestSize.Level1)
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

    // the following is valid. due to class b providing the conversion operator.
    class a {
    public:
        int val { 42 };
    };
    class b {
    public:
        int val { 6 };
        operator a() const
        {
            a tmp;
            tmp.val = val;
            return tmp;
        }
    };
    BASE_NS::vector<a> as;
    BASE_NS::vector<b> bs;
    as.resize(3);
    bs.resize(3);
    as.insert(as.end(), bs.begin(), bs.end());

    // bs.insert(bs.end(),as.begin(),as.end()); //this is impossible, due to not having the conversion.
    ASSERT_TRUE(as[0].val == 42);
    ASSERT_TRUE(as[1].val == 42);
    ASSERT_TRUE(as[2].val == 42);
    ASSERT_TRUE(as[3].val == 6);
    ASSERT_TRUE(as[4].val == 6);
    ASSERT_TRUE(as[5].val == 6);
}

/**
 * @tc.name: insert_pointer_iterators
 * @tc.desc: Tests for Insertpointeriterators. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, insert_pointer_iterators, testing::ext::TestSize.Level1)
{
    int data[] = { 135, 32, 1 };
    BASE_NS::vector<int> int_vec;
    int_vec.insert(int_vec.end(), &data[0], &data[0] + BASE_NS::countof(data));

    ASSERT_TRUE(int_vec.size() == BASE_NS::countof(data));
    ASSERT_TRUE(int_vec[0] == data[0]);
    ASSERT_TRUE(int_vec[1] == data[1]);
    ASSERT_TRUE(int_vec[2] == data[2]);
}

/**
 * @tc.name: insert_array_view_iterators
 * @tc.desc: Tests for Insertarrayviewiterators. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, insert_array_view_iterators, testing::ext::TestSize.Level1)
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

/**
 * @tc.name: insert_vector_iterators
 * @tc.desc: Tests for Insertvectoriterators. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, insert_vector_iterators, testing::ext::TestSize.Level1)
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

/**
 * @tc.name: append
 * @tc.desc: Tests for Append. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, append, testing::ext::TestSize.Level1)
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

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, Default, testing::ext::TestSize.Level1)
{
    // Test some types
    vector<int> vecInt;
    vector<bool> vecBool;
    vector<float> vecFloat;
    vector<double> vecDouble;
    vector<string> vecString;
    vector<string_view> vecStringView;
    vector<vector<int>> vecVec;
}

template<typename T>
T* Allocate(BASE_NS::vector<T> vec, int size)
{
    // Allocate space for size(s) elements
    return static_cast<T*>(vec.getAllocator().alloc(vec.getAllocator().instance, size));
}

/**
 * @tc.name: Initialization
 * @tc.desc: Tests for Initialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, Initialization, testing::ext::TestSize.Level1)
{
    // Test some types
    vector<int> vecInt;
    vecInt.push_back(0);
    vecInt.push_back(5);
    ASSERT_EQ(vecInt[0], 0);
    ASSERT_EQ(vecInt[1], 5);

    vector<bool> vecBool;
    vecBool.push_back(true);
    vecBool.push_back(false);
    ASSERT_EQ(vecBool[0], true);
    ASSERT_EQ(vecBool[1], false);

    vector<float> vecFloat;
    vecFloat.push_back(0.000001f);
    vecFloat.push_back(9999999.9f);
    ASSERT_FLOAT_EQ(vecFloat[0], 0.000001f);
    ASSERT_FLOAT_EQ(vecFloat[1], 9999999.9f);

    vector<double> vecDouble;
    vecDouble.push_back(0.0000000001);
    vecDouble.push_back(99.999998);
    ASSERT_EQ(vecDouble[0], 0.0000000001);
    ASSERT_EQ(vecDouble[1], 99.999998);

    vector<string> vecString;
    vecString.push_back("Hello");
    vecString.push_back(" WoRlD");
    ASSERT_STREQ(vecString[0].c_str(), "Hello");
    ASSERT_STREQ(vecString[1].c_str(), " WoRlD");

    vector<string_view> vecStringView;
    vecStringView.push_back("Hello");
    vecStringView.push_back(" WoRlD");
    ASSERT_EQ(vecStringView[0], "Hello");
    ASSERT_EQ(vecStringView[1], " WoRlD");

    vector<vector<int>> vecVec;
    vector<int> innerVec;
    innerVec.push_back(0);
    innerVec.push_back(5);
    vecVec.push_back(innerVec);
    ASSERT_EQ(vecVec[0][0], 0);
    ASSERT_EQ(vecVec[0][1], 5);

    // Operator=
    vecInt[0] = 6;
    ASSERT_EQ(vecInt[0], 6);

    // Allocator (alternative) TODO
    // vector<double> array;
    // double* p;
    // int size = 8;
    // p = Allocate(array, size);
}

/**
 * @tc.name: InitialzationByPointers
 * @tc.desc: Tests for Initialzation By Pointers. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, InitialzationByPointers, testing::ext::TestSize.Level1)
{
    {
        vector<int> data = { 1, 2, 3 };
        vector<int> vecInt = vector(&(*data.begin()), &(*data.end()));
        ASSERT_TRUE(vecInt.size() == data.size());
        ASSERT_TRUE(vecInt[0] == data[0]);
        ASSERT_TRUE(vecInt[1] == data[1]);
        ASSERT_TRUE(vecInt[2] == data[2]);
    }
    {
        vector<int> data = { 1, 2, 3 };
        vector<int> vecInt = vector<int>(data.begin(), data.end());
        ASSERT_TRUE(vecInt.size() == data.size());
        ASSERT_TRUE(vecInt[0] == data[0]);
        ASSERT_TRUE(vecInt[1] == data[1]);
        ASSERT_TRUE(vecInt[2] == data[2]);
    }
}

/**
 * @tc.name: SetAllocators
 * @tc.desc: Tests for Set Allocators. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, SetAllocators, testing::ext::TestSize.Level1)
{
    allocator a1 = default_allocator();
    allocator a2 = default_allocator();
    vector<int> vecInt1;
    vector<int> vecInt2;
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

    const vector<int> vecInt3(ca);
    const auto a3_const = vecInt3.getAllocator();
    ASSERT_TRUE(ca.instance == a3_const.instance);
}

/**
 * @tc.name: InitialzationByList
 * @tc.desc: Tests for Initialzation By List. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, InitialzationByList, testing::ext::TestSize.Level1)
{
    vector<int> vecBase;
    vecBase = { 1, 2, 3, 4 };
    vector<int> vec = vecBase;
    vector<int> vec2;
    ASSERT_EQ(vecBase[0], vec[0]);
    ASSERT_EQ(vecBase[1], vec[1]);
    ASSERT_EQ(vecBase[2], vec[2]);
    ASSERT_EQ(vecBase[3], vec[3]);

    vec2 = static_cast<vector<int>&&>(vec); // without erase
    ASSERT_EQ(vecBase[0], vec2[0]);
    ASSERT_EQ(vecBase[1], vec2[1]);
    ASSERT_EQ(vecBase[2], vec2[2]);
    ASSERT_EQ(vecBase[3], vec2[3]);
    ASSERT_EQ(vec.size(), 0);

    vec2 = { 0, 0, 0 };
    ASSERT_NE(vecBase[0], vec2[0]);
    ASSERT_NE(vecBase[1], vec2[1]);
    ASSERT_NE(vecBase[2], vec2[2]);

    vec2 = static_cast<vector<int>&&>(vec); // with erase but from already empty
    ASSERT_EQ(vec2.size(), 0);

    vec = vecBase;
    vec2 = static_cast<vector<int>&&>(vec); // with erase but from already empty
    ASSERT_EQ(vecBase[0], vec2[0]);
    ASSERT_EQ(vecBase[1], vec2[1]);
    ASSERT_EQ(vecBase[2], vec2[2]);
    ASSERT_EQ(vecBase[3], vec2[3]);
}

/**
 * @tc.name: InitialzationByConst
 * @tc.desc: Tests for Initialzation By Const. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, InitialzationByConst, testing::ext::TestSize.Level1)
{
    const int cInt = 5;
    vector<int> vecInt = vector(2, cInt);
    ASSERT_EQ(vecInt[0], 5);
    ASSERT_EQ(vecInt[1], 5);

    vector<bool> vecBool(2, true);
    ASSERT_EQ(vecBool[0], true);
    ASSERT_EQ(vecBool[1], true);

    vector<float> vecFloat(2, 0.000001f);
    ASSERT_FLOAT_EQ(vecFloat[0], 0.000001f);
    ASSERT_FLOAT_EQ(vecFloat[1], 0.000001f);

    vector<double> vecDouble(2, 0.0000000001);
    ASSERT_EQ(vecDouble[0], 0.0000000001);
    ASSERT_EQ(vecDouble[1], 0.0000000001);

    vector<string> vecString(2, "Hello");
    ASSERT_STREQ(vecString[0].c_str(), "Hello");
    ASSERT_STREQ(vecString[1].c_str(), "Hello");

    vector<string_view> vecStringView(2, "Hello");
    ASSERT_EQ(vecStringView[0], "Hello");
    ASSERT_EQ(vecStringView[1], "Hello");

    vector<vector<int>> vecVec(1, vector(2, 3));
    ASSERT_EQ(vecVec[0][0], 3);
    ASSERT_EQ(vecVec[0][1], 3);

    vector<int> vecEmpty(0, 10000);
    ASSERT_EQ(vecEmpty.size(), 0);
}

/**
 * @tc.name: Poisoning
 * @tc.desc: Tests for Poisoning. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, Poisoning, testing::ext::TestSize.Level1)
{
    vector<int> vecInt;
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

/**
 * @tc.name: ElementAccess
 * @tc.desc: Tests for Element Access. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, ElementAccess, testing::ext::TestSize.Level1)
{
    {
        vector<int> vecInt;
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
        const vector<int> vecInt = { 2, 5 };

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

/**
 * @tc.name: ProtectedUninitializedMemory
 * @tc.desc: Tests for Protected Uninitialized Memory. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, ProtectedUninitializedMemory, testing::ext::TestSize.Level1)
{
    protectedVectorTest<int> classInstance;
    protectedVectorTest<uint32_t> classInstanceU32;
    {
        vector<int> vecInt;
        vecInt.resize(0); // allocate 0 memory
        classInstance.wrap_uninitialized_default_construct(vecInt.begin().ptr(), vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        vector<int> vecInt;
        vecInt.resize(2); // allocate some memory for int without value
        classInstance.wrap_uninitialized_default_construct(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr());
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
    }
    {
        vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.wrap_uninitialized_value_construct(vecInt.begin().ptr(), vecInt.begin().ptr());
        // ASSERT_EQ(vecInt.size(), 0);
    }
    {
        vector<int> vecInt; // allocate some memory for int with default value
        vecInt.resize(2);
        classInstance.wrap_uninitialized_value_construct(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr());
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
    }
    {
        vector<int> vecBase = { 1, 2, 3 };
        vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.wrap_uninitialized_copy(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        vector<int> vecBase = { 1, 2, 3 };
        vector<int> vecInt; // allocate some memory for int with values from vecBase
        vecInt.resize(2);
        classInstance.wrap_uninitialized_copy(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[1]);
    }
    {
        vector<uint32_t> vecBase = { 1, 2, 3 };
        vector<uint32_t> vecInt; // allocate some memory for int with values from vecBase
        vecInt.resize(2);
        classInstanceU32.wrap_uninitialized_copy(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[1]);
    }
    {
        vector<int> vecBase = { 1, 2, 3 };
        vector<int>::const_reference& a = vecBase.front();
        vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.wrap_uninitialized_fill(vecInt.begin().ptr(), vecInt.begin().ptr(), a);
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        vector<int> vecBase = { 1, 2, 3 };
        vector<int>::const_reference& a = vecBase.front();
        vector<int> vecInt; // unitialize some memory with vecBase[0]
        vecInt.resize(2);
        classInstance.wrap_uninitialized_fill(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), a);
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[0]);
    }
    {
        vector<uint32_t> vecBase = { 1, 2, 3 };
        vector<uint32_t>::const_reference& a = vecBase.front();
        vector<uint32_t> vecInt; // unitialize some memory with vecBase[0]
        vecInt.resize(2);
        classInstanceU32.wrap_uninitialized_fill(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), a);
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], vecBase[0]);
        ASSERT_EQ(vecInt[1], vecBase[0]);
    }
    {
        vector<int> vecBase = { 1, 2, 3 };
        vector<int> vecInt; // allocate 0 memory
        vecInt.resize(0);
        classInstance.wrap_uninitialized_move(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 0);
    }
    {
        vector<int> vecBase = { 1, 2, 3 };
        vector<int> vecInt; // allocate some memory for int with values from vecBase[2]
        vecInt.resize(2);
        classInstance.wrap_uninitialized_move(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
        ASSERT_EQ(vecBase[0], 0);
        ASSERT_EQ(vecBase[1], 0);
        ASSERT_EQ(vecBase[2], 3);
    }
    {
        vector<uint32_t> vecBase = { 1, 2, 3 };
        vector<uint32_t> vecInt; // allocate some memory for int with values from vecBase[2]
        vecInt.resize(2);
        classInstanceU32.wrap_uninitialized_move(vecInt.begin().ptr(), (vecInt.begin() + 2).ptr(), &(*vecBase.begin()));
        ASSERT_EQ(vecInt.size(), 2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
        ASSERT_EQ(vecBase[0], 0);
        ASSERT_EQ(vecBase[1], 0);
        ASSERT_EQ(vecBase[2], 3);
    }
}

/**
 * @tc.name: ProtectedInitializedMemory
 * @tc.desc: Tests for Protected Initialized Memory. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, ProtectedInitializedMemory, testing::ext::TestSize.Level1)
{
    protectedVectorTest<int> classInstance;
    protectedVectorTest<uint32_t> classInstanceU32;

    // move values, not references
    {
        vector<int> vecInt = { 1, 2, 3 };
        vector<int> vecInt2 = { 3, 2, 1 };
        vector<uint32_t> vecUint = { 99U, 88U, 77U };

        classInstance.wrap_copy(&(*vecInt2.begin()), vecInt.begin().ptr(), vecInt.begin().ptr());
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.wrap_copy<uint32_t*>(&(*vecInt2.begin()), &(*vecUint.begin()), &(*vecUint.begin()));
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.wrap_copy<iterator<vector<int>>>(&(*vecInt2.begin()), vecInt.begin(), vecInt.begin());
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.wrap_copy(&(*vecInt2.begin()), vecInt.begin().ptr(), &(*vecInt.end()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);

        classInstance.wrap_copy<uint32_t*>(&(*vecInt2.begin()), &(*vecUint.begin()), &(*vecUint.end()));
        ASSERT_EQ(vecInt2[0], 99U);
        ASSERT_EQ(vecInt2[1], 88U);
        ASSERT_EQ(vecInt2[2], 77U);
        ASSERT_EQ(vecUint[0], 99U);
        ASSERT_EQ(vecUint[1], 88U);
        ASSERT_EQ(vecUint[2], 77U);

        classInstance.wrap_copy<iterator<vector<int>>>(&(*vecInt2.begin()), vecInt.begin(), vecInt.end());
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
    }

    // move values, not references
    {
        vector<int> vecInt = { 1, 2, 3 };
        vector<int> vecInt2 = { 3, 2, 1 };

        classInstance.wrap_move(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstance.wrap_move(vecInt.begin().ptr(), &(*(vecInt.begin() + 2)), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);

        classInstance.wrap_move(vecInt.begin().ptr(), &(*(vecInt.end())), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
    }

    {
        vector<uint32_t> vecInt = { 1, 2, 3 };
        vector<uint32_t> vecInt2 = { 3, 2, 1 };

        classInstanceU32.wrap_move(vecInt.begin().ptr(), vecInt.begin().ptr(), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 3);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);

        classInstanceU32.wrap_move(vecInt.begin().ptr(), &(*(vecInt.begin() + 2)), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 1);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);

        classInstanceU32.wrap_move(vecInt.begin().ptr(), &(*(vecInt.end())), &(*vecInt2.begin()));
        ASSERT_EQ(vecInt2[0], 1);
        ASSERT_EQ(vecInt2[1], 2);
        ASSERT_EQ(vecInt2[2], 3);
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
    }
}
/**
 * @tc.name: ProtectedPointerMemoryActions
 * @tc.desc: Tests for Protected Pointer Memory Actions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, ProtectedPointerMemoryActions, testing::ext::TestSize.Level1)
{
    protectedVectorTest<int> classInstance;
    protectedVectorTest<uint32_t> classInstanceU32;
    {
        vector<int> vecInt;
        int* p;

        vecInt.resize(0);
        p = classInstance.wrap_init_value(vecInt.begin().ptr(), 0);
        ASSERT_EQ(p, vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);

        vecInt.resize(2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);

        p = classInstance.wrap_init_value(vecInt.begin().ptr(), 2);
        ASSERT_EQ(p, &(*(vecInt.begin() + 2)));
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
    }

    {
        vector<int> vecInt;
        int* p;

        vecInt.resize(0);
        p = classInstance.wrap_init_fill(vecInt.begin().ptr(), 0, 1);
        ASSERT_EQ(p, vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);

        vecInt.resize(2);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);

        p = classInstance.wrap_init_fill(vecInt.begin().ptr(), 2, 1);
        ASSERT_EQ(p, &(*(vecInt.begin() + 2)));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 1);

        // refresh
        p = classInstance.wrap_init_value(&(*(vecInt.begin() + 1)), 1);
        ASSERT_EQ(p, &(*(vecInt.begin() + 2)));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 0);
    }

    {
        vector<int> vecInt;
        vector<int> vecInt2 = { 1, 2, 3 };
        int* p;

        vecInt.resize(0);
        p = classInstance.wrap_init_copy(vecInt.begin().ptr(), &(*vecInt2.begin()), 0);
        ASSERT_EQ(p, vecInt.begin().ptr());
        ASSERT_EQ(vecInt.size(), 0);

        vecInt.resize(4);
        ASSERT_EQ(vecInt[0], 0);
        ASSERT_EQ(vecInt[1], 0);
        ASSERT_EQ(vecInt[2], 0);
        ASSERT_EQ(vecInt[3], 0);

        p = classInstance.wrap_init_copy(vecInt.begin().ptr(), &(*vecInt2.begin()), 3);
        ASSERT_EQ(p, &(*(vecInt.begin() + 3)));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 2);
        ASSERT_EQ(vecInt[2], 3);
        ASSERT_EQ(vecInt[3], 0);

        p = classInstance.wrap_init_copy(&(*(p - 2)), &(*vecInt2.begin()), 3);
        ASSERT_EQ(p, &(*(vecInt.end())));
        ASSERT_EQ(vecInt[0], 1);
        ASSERT_EQ(vecInt[1], 1);
        ASSERT_EQ(vecInt[2], 2);
        ASSERT_EQ(vecInt[3], 3);
    }

    {
        vector<int> vecInt = { 1, 2, 3 };
        vector<uint32_t> vecUint = { 99U, 88U, 77U };
        iterator<vector<int>> pInt; // here pointer to beginning, not end.

        classInstance.resize(0);
        pInt = classInstance.wrap_insert_impl<int*>(classInstance.begin(), vecInt.begin().ptr(), vecInt.begin().ptr());
        ASSERT_EQ(pInt, classInstance.begin());
        ASSERT_EQ(classInstance.size(), 0);

        classInstance.resize(4);
        ASSERT_EQ(classInstance[0], 0);
        ASSERT_EQ(classInstance[1], 0);
        ASSERT_EQ(classInstance[2], 0);
        ASSERT_EQ(classInstance[3], 0);

        pInt =
            classInstance.wrap_insert_impl<iterator<vector<int>>>(classInstance.begin(), vecInt.begin(), vecInt.end());
        ASSERT_EQ(pInt, classInstance.begin());
        ASSERT_EQ(classInstance[0], 1);
        ASSERT_EQ(classInstance[1], 2);
        ASSERT_EQ(classInstance[2], 3);
        ASSERT_EQ(classInstance[3], 0);

        pInt = classInstance.wrap_insert_impl<uint32_t*>(
            (classInstance.begin() + 1), vecUint.begin().ptr(), vecUint.end().ptr());
        ASSERT_EQ(pInt, classInstance.begin() + 1);
        ASSERT_EQ(classInstance[0], 1);
        ASSERT_EQ(classInstance[1], 99);
        ASSERT_EQ(classInstance[2], 88);
        ASSERT_EQ(classInstance[3], 77);
    }
}
/**
 * @tc.name: Iterators
 * @tc.desc: Tests for Iterators. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, Iterators, testing::ext::TestSize.Level1)
{
    int vec[5] = { 1, 2, 4, 8, 16 };
    vector<int> vecInt { 1, 2, 4, 8, 16 };

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

/**
 * @tc.name: Capacity
 * @tc.desc: Tests for Capacity. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, Capacity, testing::ext::TestSize.Level1)
{
    vector<int> vecInt;
    const int nEle = 1000;

    // Empty
    ASSERT_TRUE(vecInt.empty());

    // Max size
    ASSERT_GT(vecInt.max_size(), 0U);
    // printf("Max size of a vec: %lld \n", vecInt.max_size());

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

/**
 * @tc.name: Assign
 * @tc.desc: Tests for Assign. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, Assign, testing::ext::TestSize.Level1)
{
    {
        vector<int> vecA;
        vector<int> vecB;
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
    }
    // assign to smaller (will allocate and copy construct)
    {
        vector<int> vecA = { 1 };
        vector<int> vecB = { 2, 3, 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 3U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to smaller, but with extra capacity (will copy assign and copy construct)
    {
        vector<int> vecA = { 1, 2 };
        vecA.reserve(16);
        vector<int> vecB = { 3, 4, 5 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
        EXPECT_EQ(vecA.size(), 3U);
    }
    // assign to larger (will allocate and copy construct)
    {
        vector<int> vecA = { 1, 2, 3 };
        vector<int> vecB = { 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
        EXPECT_EQ(vecA.size(), 1U);
    }
    // assign to larger but with bigger capacity (will copy assign and destroy elements)
    {
        vector<int> vecA = { 1, 2, 3 };
        vecA.reserve(16);
        vector<int> vecB = { 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
        EXPECT_EQ(vecA.size(), 1U);
    }

    // do the same with a non-trivially destructable and copyable
    {
        vector<Thing> vecA;
        vector<Thing> vecB;
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
    }
    // assign to smaller (will allocate and copy construct)
    {
        vector<Thing> vecA = { 1 };
        vector<Thing> vecB = { 2, 3, 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 3U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to smaller, but with extra capacity (will copy assign and copy construct)
    {
        vector<Thing> vecA = { 1, 2 };
        vecA.reserve(16);
        vector<Thing> vecB = { 3, 4, 5 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 3U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to larger (will allocate and copy construct)
    {
        vector<Thing> vecA = { 1, 2, 3 };
        vector<Thing> vecB = { 4 };
        vecA = vecB;
        EXPECT_EQ(vecA.size(), vecB.size());
        EXPECT_EQ(vecA.size(), 1U);
        EXPECT_TRUE(std::equal(vecA.begin(), vecA.end(), vecB.begin()));
    }
    // assign to larger but with bigger capacity (will copy assign and destroy elements)
    {
        vector<Thing> vecA = { 1, 2, 3 };
        vecA.reserve(16);
        vector<Thing> vecB = { 4 };
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

/**
 * @tc.name: Modifiers
 * @tc.desc: Tests for Modifiers. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersVector, Modifiers, testing::ext::TestSize.Level1)
{
    vector<Foo<float>> vFoo;
    vector<int> vecInt { 1, 2, 4, 8, 16 };

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

    /*for (auto it = vecInt.begin(); it != vecInt.end(); ++it) {
        printf("Values: %d \n", *it);
    }*/
}