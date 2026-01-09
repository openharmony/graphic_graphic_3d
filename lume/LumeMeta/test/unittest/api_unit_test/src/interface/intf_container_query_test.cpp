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

#include <test_framework.h>

#include <meta/api/container/find_containers.h>
#include <meta/api/object.h>
#include <meta/base/ref_uri.h>

#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()

namespace UTest {

/**
 * @tc.name: Self
 * @tc.desc: Tests for Self. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainerInfoTest, Self, testing::ext::TestSize.Level1)
{
    Object object(CreateInstance(ClassId::Object));
    EXPECT_THAT(FindAllContainers(object), testing::SizeIs(1)); // attachments container
}
} // namespace UTest
META_END_NAMESPACE()
