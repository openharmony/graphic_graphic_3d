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

#include <nodecontext/render_node_parser_util.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

/**
 * @tc.name: ParseUintTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse a uint.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseUintTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    ASSERT_TRUE(renderNodeParserUtil.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(renderNodeParserUtil.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderNodeParserUtil>());
    ASSERT_TRUE(renderNodeParserUtil.GetInterface(IRenderNodeParserUtil::UID));
    ASSERT_FALSE(renderNodeParserUtil.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    const auto& constRenderNodeParserUtil = renderNodeParserUtil;
    ASSERT_TRUE(constRenderNodeParserUtil.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(
        constRenderNodeParserUtil.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderNodeParserUtil>());
    ASSERT_TRUE(constRenderNodeParserUtil.GetInterface(IRenderNodeParserUtil::UID));
    ASSERT_FALSE(constRenderNodeParserUtil.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));

    // Uint
    {
        const string_view str = "{\"number\": 123456}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        uint64_t number = renderNodeParserUtil.GetUintValue(jsonValue, "number");
        ASSERT_EQ(123456u, number);
    }
    {
        const string_view str = "{\"number\": \"123456\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        uint64_t number = renderNodeParserUtil.GetUintValue(jsonValue, "number");
        ASSERT_NE(123456u, number);
    }
}

/**
 * @tc.name: ParseIntTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse an int.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseIntTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    // Int
    {
        const string_view str = "{\"number\": -123456}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        int64_t number = renderNodeParserUtil.GetIntValue(jsonValue, "number");
        ASSERT_EQ(-123456, number);
    }
    {
        const string_view str = "{\"number\": \"-123456\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        int64_t number = renderNodeParserUtil.GetIntValue(jsonValue, "number");
        ASSERT_NE(-123456, number);
    }
}

/**
 * @tc.name: ParseFloatTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse a float.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseFloatTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    // Float
    {
        const string_view str = "{\"number\": -123456.5}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        float number = renderNodeParserUtil.GetFloatValue(jsonValue, "number");
        ASSERT_EQ(-123456.5, number);
    }
    {
        const string_view str = "{\"number\": \"-123456.5\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        float number = renderNodeParserUtil.GetFloatValue(jsonValue, "number");
        ASSERT_NE(-123456.5, number);
    }
}

/**
 * @tc.name: ParseStringTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse a string.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseStringTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    // String
    {
        const string_view str = "{\"string\": \"string data\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        string result = renderNodeParserUtil.GetStringValue(jsonValue, "string");
        ASSERT_EQ("string data", result);
    }
    {
        const string_view str = "{\"string\": -1}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        string result = renderNodeParserUtil.GetStringValue(jsonValue, "string");
        ASSERT_NE("string data", result);
    }
    {
        const string_view str = "{\"string\": string}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        string result = renderNodeParserUtil.GetStringValue(jsonValue, "string");
        ASSERT_NE("string", result);
    }
}

/**
 * @tc.name: ParseRenderSlotSortTypeTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse an enum called RenderSlotSortType.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseRenderSlotSortTypeTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    // Render slot sort type
    {
        const string_view str = "{\"renderSlotSortType\": \"none\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotSortType(jsonValue, "renderSlotSortType");
        ASSERT_EQ(RenderSlotSortType::NONE, result);
    }
    {
        const string_view str = "{\"renderSlotSortType\": \"front_to_back\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotSortType(jsonValue, "renderSlotSortType");
        ASSERT_EQ(RenderSlotSortType::FRONT_TO_BACK, result);
    }
    {
        const string_view str = "{\"renderSlotSortType\": \"back_to_front\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotSortType(jsonValue, "renderSlotSortType");
        ASSERT_EQ(RenderSlotSortType::BACK_TO_FRONT, result);
    }
    {
        const string_view str = "{\"renderSlotSortType\": \"by_material\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotSortType(jsonValue, "renderSlotSortType");
        ASSERT_EQ(RenderSlotSortType::BY_MATERIAL, result);
    }
    {
        const string_view str = "{\"renderSlotSortType\": \"invalid_string\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotSortType(jsonValue, "renderSlotSortType");
        ASSERT_EQ(RenderSlotSortType::NONE, result);
    }
    {
        const string_view str = "{\"renderSlotSortType\": -4}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotSortType(jsonValue, "renderSlotSortType");
        ASSERT_EQ(RenderSlotSortType::NONE, result);
    }
}

/**
 * @tc.name: ParseRenderSlotCullTypeTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse an enum called RenderSlotCullType.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseRenderSlotCullTypeTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    // Render slot cull type
    {
        const string_view str = "{\"renderSlotCullType\": \"none\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotCullType(jsonValue, "renderSlotCullType");
        ASSERT_EQ(RenderSlotCullType::NONE, result);
    }
    {
        const string_view str = "{\"renderSlotCullType\": \"view_frustum_cull\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotCullType(jsonValue, "renderSlotCullType");
        ASSERT_EQ(RenderSlotCullType::VIEW_FRUSTUM_CULL, result);
    }
    {
        const string_view str = "{\"renderSlotCullType\": \"invalid\"}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotCullType(jsonValue, "renderSlotCullType");
        ASSERT_EQ(RenderSlotCullType::NONE, result);
    }
    {
        const string_view str = "{\"renderSlotCullType\": -6.43}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderSlotCullType(jsonValue, "renderSlotCullType");
        ASSERT_EQ(RenderSlotCullType::NONE, result);
    }
}

/**
 * @tc.name: ParseInputResourcesTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse input resources.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseInputResourcesTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    {
        const string_view str = "{\"inputResources\": {\"images\": [{\"set\": \"none\"}]}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetInputResources(jsonValue, "inputResources");
        ASSERT_EQ(1, result.images.size());
    }
}

/**
 * @tc.name: ParseRenderDataStoreTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse a render data store.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseRenderDataStoreTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    {
        const string_view str = "{\"dataStore\": {\"dataStoreName\": -1}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetRenderDataStore(jsonValue, "dataStore");
        ASSERT_TRUE(result.dataStoreName.empty());
    }
}

/**
 * @tc.name: ParseGpuBufferDescsTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse a GPUBufferDesc structure.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseGpuBufferDescsTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    {
        const string_view str = "{\"descs\": -1}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetGpuBufferDescs(jsonValue, "descs");
        ASSERT_TRUE(result.empty());
    }
}

/**
 * @tc.name: ParseGpuImageDescsTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse a GPUImageDesc structure.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseGpuImageDescsTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    {
        const string_view str = "{\"descs\": -1}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetGpuImageDescs(jsonValue, "descs");
        ASSERT_TRUE(result.empty());
    }
    {
        const string_view str = "{\"descs\": [{\"format\": \"undefined\"}]}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetGpuImageDescs(jsonValue, "descs");
        ASSERT_EQ(1, result.size());
    }
    {
        const string_view str = "{\"descs\": [{\"shadingRateTexelSize\": [1, 1]}]}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetGpuImageDescs(jsonValue, "descs");
        ASSERT_EQ(1, result.size());
    }
}

/**
 * @tc.name: ParseInputRenderPassTest
 * @tc.desc: Tests for RenderNodeParserUtil to parse a renderpass description.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeParserUtil, ParseInputRenderPassTest, testing::ext::TestSize.Level1)
{
    RenderNodeParserUtil::CreateInfo createInfo;
    RenderNodeParserUtil renderNodeParserUtil(createInfo);
    {
        const string_view str = "{\"renderPass\": {\"attachments\": [{\"clearColor\":[0.1]}]}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetInputRenderPass(jsonValue, "renderPass");
        ASSERT_EQ(1, result.attachments.size());
    }
    {
        const string_view str = "{\"renderPass\": {\"attachments\": [{\"clearDepth\":[0.1]}]}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetInputRenderPass(jsonValue, "renderPass");
        ASSERT_EQ(1, result.attachments.size());
    }
    {
        const string_view str = "{\"renderPass\": {\"attachments\": [{\"clearDepth\":[\"x\", \"y\"]}]}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetInputRenderPass(jsonValue, "renderPass");
        ASSERT_EQ(1, result.attachments.size());
    }
    {
        const string_view str = "{\"renderPass\": {\"subpassIndex\": 5, \"subpassCount\": 6}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetInputRenderPass(jsonValue, "renderPass");
        ASSERT_EQ(0, result.attachments.size());
        ASSERT_EQ(6, result.subpassCount);
        ASSERT_EQ(5, result.subpassIndex);
    }
    {
        const string_view str = "{\"renderPass\": {\"subpass\": {\"depthResolveAttachmentIndex\": 1}}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetInputRenderPass(jsonValue, "renderPass");
        ASSERT_EQ(0, result.attachments.size());
    }
    {
        const string_view str = "{\"renderPass\": {\"subpass\": {\"shadingRateTexelSize\": [1, 1]}}}";
        auto jsonValue = CORE_NS::json::parse(str.data());
        auto result = renderNodeParserUtil.GetInputRenderPass(jsonValue, "renderPass");
        ASSERT_EQ(1u, result.shadingRateTexelSize.width);
        ASSERT_EQ(1u, result.shadingRateTexelSize.height);
    }
}
