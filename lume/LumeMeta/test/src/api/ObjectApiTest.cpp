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
#include "TestRunner.h"
#include <src/testing_objects.h>

#include <gtest/gtest.h>

#include <meta/api/object.h>
#include <meta/interface/property/property.h>

#include "src/util.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class TestInterfaceAPI : public Internal::ObjectInterfaceAPI<TestInterfaceAPI, ClassId::TestType> {
    META_API(TestInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(ITestType)
    META_API_CACHE_INTERFACE(ITestType, Test)

public:
    META_API_INTERFACE_PROPERTY_CACHED(Test, First, int)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Test, Third, int)
    META_API_INTERFACE_ARRAY_PROPERTY_CACHED(Test, MyIntArray, int)
    META_API_INTERFACE_READONLY_ARRAY_PROPERTY_CACHED(Test, MyConstIntArray, int)
};

class ObjectApiTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: Properties
 * @tc.desc: test Properties
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(ObjectApiTest, Properties, TestSize.Level1)
{
    TestInterfaceAPI test;
    test.First(1);
    EXPECT_EQ(test.First()->GetValue(), 1);
    EXPECT_EQ(test.Third()->GetValue(), 0);
    test.MyIntArray({ 1, 2 });
    EXPECT_EQ(test.MyIntArray()->GetValue(), (BASE_NS::vector<int> { 1, 2 }));
    EXPECT_EQ(test.MyConstIntArray()->GetValue(), (BASE_NS::vector<int> { 1, 2, 3, 4, 5 }));
    test.First(GetValue(test.Third()));
    EXPECT_EQ(test.First()->GetValue(), 0);
    test.MyIntArray(test.MyConstIntArray()->GetValue());
    EXPECT_EQ(test.MyIntArray()->GetValue(), (BASE_NS::vector<int> { 1, 2, 3, 4, 5 }));
}

META_END_NAMESPACE()
