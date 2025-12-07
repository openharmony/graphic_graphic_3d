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

#include <functional>

#include <meta/base/ids.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {

template<typename Id>
void IdTest()
{
    Id id1;
    Id id2("29495e67-14a6-40aa-a16f-1923630af506");
    Id id3(id2);
    Id id4("29495e67-14a6-40aa-a16f-1923630af507");
    EXPECT_FALSE(id1 == id2);
    EXPECT_TRUE(id2 == id3);
    EXPECT_TRUE(id1 != id2);
    EXPECT_TRUE(id2 != id4);
    EXPECT_FALSE(id2 != id3);
    EXPECT_TRUE(id1 < id2);
    EXPECT_TRUE(id2 < id4);
    EXPECT_FALSE(id2 < id3);
    EXPECT_EQ(id2.ToString(), "29495e67-14a6-40aa-a16f-1923630af506");
    EXPECT_FALSE(id1.IsValid());
    EXPECT_TRUE(id2.IsValid());
    EXPECT_EQ(id3.ToUid(), BASE_NS::Uid { "29495e67-14a6-40aa-a16f-1923630af506" });
    id1 = id2;
    EXPECT_TRUE(id1 == id2);
    EXPECT_TRUE(id1.Compare(id2) == 0);
    EXPECT_TRUE(id2.Compare(id4) < 0);
    EXPECT_TRUE(id4.Compare(id2) > 0);
}

/**
 * @tc.name: TypeId
 * @tc.desc: Tests for Type Id. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_IdTests, TypeId, testing::ext::TestSize.Level1)
{
    IdTest<TypeId>();
}
/**
 * @tc.name: ObjectId
 * @tc.desc: Tests for Object Id. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_IdTests, ObjectId, testing::ext::TestSize.Level1)
{
    IdTest<ObjectId>();
}
/**
 * @tc.name: InstanceId
 * @tc.desc: Tests for Instance Id. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_IdTests, InstanceId, testing::ext::TestSize.Level1)
{
    IdTest<InstanceId>();
}

} // namespace UTest
META_END_NAMESPACE()
