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

#include <TestRunner.h>
#include <src/testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/property/array_property_event_handler.h>
#include <meta/interface/property/construct_array_property.h>

#include "src/util.h"

using namespace CORE_NS;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class ArrayPropertyEventHandlerTest : public testing::Test {
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

template<typename Type>
static void ExpectChanges(ArrayChanges<Type> a1, ArrayChanges<Type> a2)
{
    EXPECT_EQ(a1.indexesRemoved, a2.indexesRemoved);
    EXPECT_EQ(a1.valuesAdded, a2.valuesAdded);
    EXPECT_EQ(a1.positionChanged, a2.positionChanged);
}

/**
 * @tc.name: Diff
 * @tc.desc: test diff
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(ArrayPropertyEventHandlerTest, Diff, TestSize.Level1)
{
    auto p = ConstructArrayProperty<int>("test", BASE_NS::vector<int> { 1, 2, 3 });
    ArrayPropertyChangedEventHandler<int> h;
    ArrayChanges<int> change;
    h.Subscribe(p, [&](ArrayChanges<int> c) { change = c; });

    p->SetValue({ 1, 2 });
    ExpectChanges(change, { { 2 } });

    p->SetValue({ 1, 2, 3, 4 });
    ExpectChanges(change, { {}, { { 3, 2 }, { 4, 3 } } });

    p->SetValue({ 1, 5, 6, 4 });
    ExpectChanges(change, { { 1, 2 }, { { 5, 1 }, { 6, 2 } } });

    p->SetValue({ 1, 4, 5, 6 });
    ExpectChanges(change, { {}, {}, { { 3, 1 }, { 1, 2 }, { 2, 3 } } });

    p->SetValue({ 0, 1, 6, 5 });
    ExpectChanges(change, { { 1 }, { { 0, 0 } }, { { 0, 1 }, { 3, 2 }, { 2, 3 } } });

    p->SetValue({ 0, 1, 0 });
    p->SetValue({ 1, 0 });
    ExpectChanges(change, { { 2 }, {}, { { 1, 0 }, { 0, 1 } } });

    p->SetValue({ 0, 1, 0 });
    p->SetValue({ 2, 1, 0 });
    ExpectChanges(change, { { 2 }, { { 2, 0 } }, { { 0, 2 } } });

    p->SetValue({ 0 });
    p->SetValue(BASE_NS::vector<int> { 0, 0 });
    ExpectChanges(change, { {}, { { 0, 1 } } });

    p->SetValue({ 0, 1, 2, 2, 1, 0, 1, 0, 1, 0 });
    p->SetValue({ 0, 0, 2, 1, 0, 0, 1, 0, 1, 0 });
    ExpectChanges(
        change, { { 3, 8 }, { { 0, 7 }, { 0, 9 } }, { { 5, 1 }, { 1, 3 }, { 7, 4 }, { 9, 5 }, { 4, 6 }, { 6, 8 } } });

    change = {};
    h.Unsubscribe();
    p->SetValue({ -1, 0 });
    ExpectChanges(change, {});
}
META_END_NAMESPACE()
