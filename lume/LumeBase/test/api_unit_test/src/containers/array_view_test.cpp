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

#include <base/containers/array_view.h>
#include <base/containers/string.h>

using namespace BASE_NS;
using namespace testing::ext;

class ArrayViewTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ArrayViewTest, empty, TestSize.Level1)
{
    BASE_NS::array_view<int> intAv({});
    ASSERT_TRUE(intAv.size() == 0);
    ASSERT_TRUE(intAv.size_bytes() == 0);
    ASSERT_TRUE(intAv.empty());

    BASE_NS::array_view<bool> boolAv({});
    ASSERT_TRUE(boolAv.size() == 0);
    ASSERT_TRUE(boolAv.size_bytes() == 0);
    ASSERT_TRUE(boolAv.empty());

    BASE_NS::array_view<uint32_t> uintAv({});
    ASSERT_TRUE(uintAv.size() == 0);
    ASSERT_TRUE(uintAv.size_bytes() == 0);
    ASSERT_TRUE(uintAv.empty());

    BASE_NS::array_view<char> charAv({});
    ASSERT_TRUE(charAv.size() == 0);
    ASSERT_TRUE(charAv.size_bytes() == 0);
    ASSERT_TRUE(charAv.empty());

    BASE_NS::array_view<int*> pAv({});
    ASSERT_TRUE(pAv.size() == 0);
    ASSERT_TRUE(pAv.size_bytes() == 0);
    ASSERT_TRUE(pAv.empty());

    BASE_NS::array_view<array_view<int>> avAv({});
    ASSERT_TRUE(avAv.size() == 0);
    ASSERT_TRUE(avAv.size_bytes() == 0);
    ASSERT_TRUE(avAv.empty());
}

HWTEST_F(ArrayViewTest, dataAccess, TestSize.Level1)
{
    int intData[] = { 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv(intData);
    ASSERT_TRUE(intAv.size() == 4);
    ASSERT_TRUE(intAv.size_bytes() == 4 * sizeof(int));
    ASSERT_FALSE(intAv.empty());

    ASSERT_TRUE(intAv.data() == intData);
    ASSERT_TRUE(*intAv.begin() == intData[0]);
    ASSERT_TRUE(*intAv.cbegin() == intData[0]);
    ASSERT_TRUE(intAv[1] == intData[1]);
    ASSERT_TRUE(intAv.at(2) == intData[2]);
    ASSERT_TRUE(*(intAv.end() - 1) == intData[3]);
    ASSERT_TRUE(*(intAv.cend() - 1) == intData[3]);

    const BASE_NS::array_view<int> cintAv(intData);

    ASSERT_TRUE(cintAv.size() == 4);
    ASSERT_TRUE(cintAv.size_bytes() == 4 * sizeof(int));
    ASSERT_FALSE(cintAv.empty());

    ASSERT_TRUE(cintAv.data() == intData);
    ASSERT_TRUE(*cintAv.begin() == intData[0]);
    ASSERT_TRUE(*cintAv.cbegin() == intData[0]);
    ASSERT_TRUE(cintAv[1] == intData[1]);
    ASSERT_TRUE(cintAv.at(2) == intData[2]);
    ASSERT_TRUE(*(cintAv.end() - 1) == intData[3]);
    ASSERT_TRUE(*(cintAv.cend() - 1) == intData[3]);

    bool boolData[] = { true, false };
    BASE_NS::array_view<bool> boolAv(boolData);
    ASSERT_TRUE(boolAv.size() == 2);
    ASSERT_TRUE(boolAv.size_bytes() == 2 * sizeof(bool));
    ASSERT_FALSE(boolAv.empty());

    ASSERT_TRUE(boolAv[0] == boolData[0]);
    ASSERT_TRUE(boolAv[1] == boolData[1]);
}

HWTEST_F(ArrayViewTest, constructors001, TestSize.Level1)
{
    int intData[] = { 3, 2, 1, 0 };
    {
        BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>();
        ASSERT_TRUE(intAv.size() == 0);
        ASSERT_TRUE(intAv.empty());
    }
    {
        BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
        ASSERT_TRUE(intAv.size() == 4);
        ASSERT_TRUE(intAv[0] == intData[0]);
        ASSERT_TRUE(intAv[1] == intData[1]);
        ASSERT_TRUE(intAv[2] == intData[2]);
        ASSERT_TRUE(intAv[3] == intData[3]);
    }
    {
        BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(&intData[0], &intData[3]);
        ASSERT_TRUE(intAv.size() == 3);
        ASSERT_TRUE(intAv[0] == intData[0]);
        ASSERT_TRUE(intAv[1] == intData[1]);
        ASSERT_TRUE(intAv[2] == intData[2]);
    }
    {
        BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(&intData[0], 4);
        ASSERT_TRUE(intAv.size() == 4);
        ASSERT_TRUE(intAv[0] == intData[0]);
        ASSERT_TRUE(intAv[1] == intData[1]);
        ASSERT_TRUE(intAv[2] == intData[2]);
        ASSERT_TRUE(intAv[3] == intData[3]);
    }
}

HWTEST_F(ArrayViewTest, constructors002, TestSize.Level1)
{
    int intData[] = { 3, 2, 1, 0 };
    {
        BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
        BASE_NS::array_view<int> intAv2 = BASE_NS::array_view<int>(intAv);
        ASSERT_TRUE(intAv2.size() == 4);
        ASSERT_TRUE(intAv2[0] == intData[0]);
        ASSERT_TRUE(intAv2[1] == intData[1]);
        ASSERT_TRUE(intAv2[2] == intData[2]);
        ASSERT_TRUE(intAv2[3] == intData[3]);
    }
    {
        const BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
        BASE_NS::array_view<const int> intAv2 = BASE_NS::array_view<const int>(intAv);
        ASSERT_TRUE(intAv2.size() == 4);
        ASSERT_TRUE(intAv2[0] == intData[0]);
        ASSERT_TRUE(intAv2[1] == intData[1]);
        ASSERT_TRUE(intAv2[2] == intData[2]);
        ASSERT_TRUE(intAv2[3] == intData[3]);
    }
    {
        BASE_NS::array_view<int> intAv = BASE_NS::arrayview<int, 4>(intData);
        ASSERT_TRUE(intAv.size() == 4);
        ASSERT_TRUE(intAv[0] == intData[0]);
        ASSERT_TRUE(intAv[1] == intData[1]);
        ASSERT_TRUE(intAv[2] == intData[2]);
        ASSERT_TRUE(intAv[3] == intData[3]);
    }
}

HWTEST_F(ArrayViewTest, constructors003, TestSize.Level1)
{
    {
        uint8_t* uint8Data = new uint8_t[4]();
        uint8Data[0] = 3;
        uint8Data[1] = 2;
        uint8Data[2] = 1;
        uint8Data[3] = 0;
        BASE_NS::array_view<const uint8_t> uintAv = BASE_NS::arrayviewU8<uint8_t>(*uint8Data);
        ASSERT_TRUE(uintAv.size() == 1); // pointer size is 1 -> data is the same but size is always 1
        ASSERT_TRUE(uintAv[0] == uint8Data[0]);
        delete[] uint8Data;
    }
    {
        uint8_t uint8Data[4] = { 3, 2, 1, 0 };
        BASE_NS::array_view<const uint8_t> uintAv = BASE_NS::arrayviewU8<uint8_t>(*uint8Data);
        ASSERT_TRUE(uintAv.size() == 1); // pointer size is 1 -> data is the same but size is always 1
        ASSERT_TRUE(uintAv[0] == uint8Data[0]);
    }
    {
        uint8_t uint8Data[4] = { 3, 2, 1, 0 };
        BASE_NS::array_view<const uint8_t> uintAv = BASE_NS::arrayviewU8(uint8Data);
        ASSERT_TRUE(uintAv.size() == 4); // array as input
        ASSERT_TRUE(uintAv[0] == uint8Data[0]);
        ASSERT_TRUE(uintAv[1] == uint8Data[1]);
        ASSERT_TRUE(uintAv[2] == uint8Data[2]);
        ASSERT_TRUE(uintAv[3] == uint8Data[3]);
    }
    {
        auto fun = [](BASE_NS::array_view<const int> intAv) {
            ASSERT_TRUE(intAv.size() == 4);
            ASSERT_TRUE(intAv[0] == 3);
            ASSERT_TRUE(intAv[1] == 2);
            ASSERT_TRUE(intAv[2] == 1);
            ASSERT_TRUE(intAv[3] == 0);
        };
        fun((BASE_NS::array_view<const int>){ 3, 2, 1, 0 });
    }
    {
        auto fun = [](BASE_NS::array_view<const BASE_NS::string> stringAv) {
            ASSERT_TRUE(stringAv.size() == 2);
            ASSERT_TRUE(stringAv[0] == "foo");
            ASSERT_TRUE(stringAv[1] == "A really, really, really long string.");
        };
        fun((BASE_NS::array_view<const BASE_NS::string>) {
            BASE_NS::string("foo"), BASE_NS::string("A really, really, really long string.")});
    }
}

HWTEST_F(ArrayViewTest, hash, TestSize.Level1)
{
    static constexpr const int data[] = { 1, 2, 3, 4 };
    auto h1 = FNV1aHash(data);
    auto h2 = hash(array_view(data));
    EXPECT_EQ(h1, h2);
}