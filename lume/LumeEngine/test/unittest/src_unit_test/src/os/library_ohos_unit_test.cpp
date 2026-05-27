/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <core/namespace.h>

#include "os/intf_library.h"
#include "test_framework.h"

using namespace CORE_NS;
using namespace BASE_NS;

/**
 * @tc.name: GetFileExtension001
 * @tc.desc: test ILibrary::GetFileExtension returns ".so"
 * @tc.type: FUNC
 */
UNIT_TEST(LibraryOhosTest, GetFileExtension001, testing::ext::TestSize.Level1)
{
    auto ext = ILibrary::GetFileExtension();
    EXPECT_EQ(ext, ".so");
}

/**
 * @tc.name: GetFileExtensionConsistency001
 * @tc.desc: test GetFileExtension returns same result on multiple calls
 * @tc.type: FUNC
 */
UNIT_TEST(LibraryOhosTest, GetFileExtensionConsistency001, testing::ext::TestSize.Level1)
{
    auto ext1 = ILibrary::GetFileExtension();
    auto ext2 = ILibrary::GetFileExtension();
    EXPECT_EQ(ext1, ext2);
    EXPECT_EQ(ext1, ".so");
}