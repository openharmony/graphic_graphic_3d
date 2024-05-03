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

#ifndef RENDER_RENDER__RENDER_NODE_PARSER_UTIL_H
#define RENDER_RENDER__RENDER_NODE_PARSER_UTIL_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_parser_util.h>

RENDER_BEGIN_NAMESPACE()
/**
 * class RenderNodeParserUtil.
 */
class RenderNodeParserUtil final : public IRenderNodeParserUtil {
public:
    struct CreateInfo {};

    explicit RenderNodeParserUtil(const CreateInfo& createInfo);
    ~RenderNodeParserUtil() override;

    uint64_t GetUintValue(const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;
    int64_t GetIntValue(const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;
    float GetFloatValue(const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;
    BASE_NS::string GetStringValue(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;

    RenderNodeGraphInputs::InputRenderPass GetInputRenderPass(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;
    RenderNodeGraphInputs::InputResources GetInputResources(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;
    RenderNodeGraphInputs::RenderDataStore GetRenderDataStore(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;

    BASE_NS::vector<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> GetGpuImageDescs(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;
    BASE_NS::vector<RenderNodeGraphInputs::RenderNodeGraphGpuBufferDesc> GetGpuBufferDescs(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;

    RenderSlotSortType GetRenderSlotSortType(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;
    RenderSlotCullType GetRenderSlotCullType(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const override;

    // IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
};
RENDER_END_NAMESPACE()

#endif // RENDER_RENDER__RENDER_NODE_PARSER_UTIL_H
