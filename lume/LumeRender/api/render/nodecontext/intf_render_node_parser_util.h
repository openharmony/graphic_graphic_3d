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

#ifndef API_RENDER_IRENDER_NODE_PARSER_UTIL_H
#define API_RENDER_IRENDER_NODE_PARSER_UTIL_H

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/json/json.h>
#include <core/plugin/intf_interface.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()

/** @ingroup group_render_irendernodeparserutil */
/**
 * Provides parsering utilities for render node graph node jsons.
 */
class IRenderNodeParserUtil : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("cfba834a-63ea-4973-a3b5-337522cc51d2");

    /** Get uint64_t value from json. Returns max value if not found.
     * @param jsonValue Json value.
     * @param name Name of the value.
     */
    virtual uint64_t GetUintValue(const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get int64_t value from json. Returns max value if not found.
     * @param jsonValue Json value.
     */
    virtual int64_t GetIntValue(const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get uint32_t value from json. Returns max value if not found.
     * @param jsonValue Json value.
     */
    virtual float GetFloatValue(const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get string value from json.
     * @param jsonValue Json value.
     */
    virtual BASE_NS::string GetStringValue(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get input render pass from json.
     * @param jsonValue Json value.
     * @param name Name of the render pass in json. In core libraries "renderPass" is usually used.
     */
    virtual RenderNodeGraphInputs::InputRenderPass GetInputRenderPass(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get input render pass from json.
     * @param jsonValue Json value.
     * @param name Name of the resources pass in json. In core libraries "resources" is usually used.
     */
    virtual RenderNodeGraphInputs::InputResources GetInputResources(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get render data store from json.
     * @param jsonValue Json value.
     * @param name Name of the data store block json. In core libraries "renderDataStore" is usually used.
     */
    virtual RenderNodeGraphInputs::RenderDataStore GetRenderDataStore(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get GpuImageDescs from json.
     * @param jsonValue Json value.
     * @param name Name of the image desc block json. In core libraries "gpuImageDescs" is usually used.
     */
    virtual BASE_NS::vector<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> GetGpuImageDescs(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get GpuBufferDescs from json.
     * @param jsonValue Json value.
     * @param name Name of the buffer desc block json. In core libraries "gpuBuffersDescs" is usually used.
     */
    virtual BASE_NS::vector<RenderNodeGraphInputs::RenderNodeGraphGpuBufferDesc> GetGpuBufferDescs(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get RenderSlotSortType from json.
     * @param jsonValue Json value.
     * @param name Name of the sort type block json. In core libraries "renderSlotSortType" is usually used.
     */
    virtual RenderSlotSortType GetRenderSlotSortType(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

    /** Get RenderSlotCullType from json.
     * @param jsonValue Json value.
     * @param name Name of the cull type block json. In core libraries "renderSlotCullType" is usually used.
     */
    virtual RenderSlotCullType GetRenderSlotCullType(
        const CORE_NS::json::value& jsonValue, const BASE_NS::string_view name) const = 0;

protected:
    IRenderNodeParserUtil() = default;
    virtual ~IRenderNodeParserUtil() = default;

    IRenderNodeParserUtil(const IRenderNodeParserUtil&) = delete;
    IRenderNodeParserUtil& operator=(const IRenderNodeParserUtil&) = delete;
    IRenderNodeParserUtil(IRenderNodeParserUtil&&) = delete;
    IRenderNodeParserUtil& operator=(IRenderNodeParserUtil&&) = delete;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_PARSER_UTIL_H
