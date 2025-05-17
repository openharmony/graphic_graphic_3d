/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <functional>

#include <gtest/gtest.h>

#include <meta/base/ids.h>

#include "TestRunner.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class IdTests : public testing::Test {
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

HWTEST_F(IdTests, TypeId, TestSize.Level1)
{
    IdTest<TypeId>();
}
HWTEST_F(IdTests, ObjectId, TestSize.Level1)
{
    IdTest<ObjectId>();
}
HWTEST_F(IdTests, InstanceId, TestSize.Level1)
{
    IdTest<InstanceId>();
}

META_END_NAMESPACE()
