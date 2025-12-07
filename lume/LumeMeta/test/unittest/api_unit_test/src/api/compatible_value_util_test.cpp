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

#include <meta/api/compatible_value_util.h>
#include <meta/interface/detail/any.h>

#include "helpers/testing_objects.h"
#include "test_framework.h"

META_BEGIN_NAMESPACE()

namespace UTest {

using types = TypeList<bool, char, int16_t, int32_t>;

/**
 * @tc.name: SetGet
 * @tc.desc: Tests for Set Get. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CompatibleValueUtilTest, SetGet, testing::ext::TestSize.Level1)
{
    Any<int16_t> any { 2 };

    int64_t v = 0;
    EXPECT_TRUE(GetCompatibleValue(any, v, types {}));
    EXPECT_EQ(v, 2);

    size_t s = 3;
    EXPECT_TRUE(SetCompatibleValue(s, any, types {}));
    EXPECT_EQ(any.InternalGetValue(), 3);
}

} // namespace UTest

META_END_NAMESPACE()
