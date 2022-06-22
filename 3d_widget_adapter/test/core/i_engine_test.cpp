/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#include "gtest/gtest.h"

#include "i_engine.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {
class IEngineTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void IEngineTest::SetUpTestCase() {}
void IEngineTest::TearDownTestCase() {}
void IEngineTest::SetUp() {}
void IEngineTest::TearDown() {}
}