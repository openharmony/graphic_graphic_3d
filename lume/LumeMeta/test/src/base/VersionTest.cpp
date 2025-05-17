/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <gtest/gtest.h>

#include <meta/base/version.h>

#include "TestRunner.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class VersionTest : public testing::Test {
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

HWTEST_F(VersionTest, Compare, TestSize.Level1)
{
    Version a;
    Version b(1, 0);
    Version c(1, 1);
    Version d(0, 9999);
    Version e(1, 9999);
    Version f(2366, 0);

    EXPECT_TRUE(a == a);
    EXPECT_TRUE(b == b);
    EXPECT_TRUE(a != b);

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b < c);
    EXPECT_TRUE(d < b);
    EXPECT_TRUE(e < f);

    EXPECT_FALSE(b > b);
    EXPECT_FALSE(b > c);
    EXPECT_FALSE(d > b);
    EXPECT_FALSE(e > f);

    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(b <= b);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b <= c);
    EXPECT_TRUE(d <= b);
    EXPECT_TRUE(e <= f);

    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(b >= b);
    EXPECT_FALSE(a >= b);
    EXPECT_FALSE(b >= c);
    EXPECT_FALSE(d >= b);
    EXPECT_FALSE(e >= f);
}

void TestFromToVersion(Version v)
{
    Version other(v.ToString());
    EXPECT_EQ(v, other);
}

HWTEST_F(VersionTest, ToString, TestSize.Level1)
{
    TestFromToVersion(Version());
    TestFromToVersion(Version(1, 0));
    TestFromToVersion(Version(1, 1));
    TestFromToVersion(Version(1, 9999));
    TestFromToVersion(Version(9999, 0));
    TestFromToVersion(Version(123, 2));
}

HWTEST_F(VersionTest, FromString, TestSize.Level1)
{
    EXPECT_EQ(Version("1"), Version(1, 0));
    EXPECT_EQ(Version("1125"), Version(1125, 0));
    EXPECT_EQ(Version("1."), Version(1, 0));
    EXPECT_EQ(Version("1.df"), Version(1, 0));
}

HWTEST_F(VersionTest, InvalidString, TestSize.Level1)
{
    EXPECT_EQ(Version(""), Version());
    EXPECT_EQ(Version(".1"), Version());
    EXPECT_EQ(Version(".."), Version());
    EXPECT_EQ(Version("asdb"), Version());
    EXPECT_EQ(Version("-1.24"), Version());
}

META_END_NAMESPACE()
