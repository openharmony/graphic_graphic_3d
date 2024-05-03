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

#include "shader_state_loader_util.h"

#include <core/namespace.h>

#include "json_util.h"
#include "shader_state_loader.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
// clang-format off
CORE_JSON_SERIALIZE_ENUM(PrimitiveTopology,
    {
        { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, nullptr }, // default
        { CORE_PRIMITIVE_TOPOLOGY_POINT_LIST, "point_list" },
        { CORE_PRIMITIVE_TOPOLOGY_LINE_LIST, "line_list" },
        { CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP, "line_strip" },
        { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "triangle_list" },
        { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, "triangle_strip" },
        { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, "triangle_fan" },
        { CORE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY, "line_list_with_adjacency" },
        { CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY, "line_strip_with_adjacency" },
        { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, "triangle_list_with_adjacency" },
        { CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, "triangle_strip_with_adjacency" },
        { CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST, "patch_list" },
    })

CORE_JSON_SERIALIZE_ENUM(PolygonMode,
    {
        { CORE_POLYGON_MODE_FILL, "fill" },
        { CORE_POLYGON_MODE_LINE, "line" },
        { CORE_POLYGON_MODE_POINT, "point" },
    })

CORE_JSON_SERIALIZE_ENUM(CullModeFlagBits,
    {
        { (CullModeFlagBits)0, nullptr },
        { CORE_CULL_MODE_NONE, "none" },
        { CORE_CULL_MODE_FRONT_BIT, "front" },
        { CORE_CULL_MODE_BACK_BIT, "back" },
        { CORE_CULL_MODE_FRONT_AND_BACK, "front_and_back" },
    })

CORE_JSON_SERIALIZE_ENUM(FrontFace,
    {
        { CORE_FRONT_FACE_COUNTER_CLOCKWISE, "counter_clockwise" },
        { CORE_FRONT_FACE_CLOCKWISE, "clockwise" },
    })

CORE_JSON_SERIALIZE_ENUM(CompareOp,
    {
        { CORE_COMPARE_OP_NEVER, "never" },
        { CORE_COMPARE_OP_LESS, "less" },
        { CORE_COMPARE_OP_EQUAL, "equal" },
        { CORE_COMPARE_OP_LESS_OR_EQUAL, "less_or_equal" },
        { CORE_COMPARE_OP_GREATER, "greater" },
        { CORE_COMPARE_OP_NOT_EQUAL, "not_equal" },
        { CORE_COMPARE_OP_GREATER_OR_EQUAL, "greater_or_equal" },
        { CORE_COMPARE_OP_ALWAYS, "always" },
    })

CORE_JSON_SERIALIZE_ENUM(StencilOp,
    {
        { CORE_STENCIL_OP_KEEP, "keep" },
        { CORE_STENCIL_OP_ZERO, "zero" },
        { CORE_STENCIL_OP_REPLACE, "replace" },
        { CORE_STENCIL_OP_INCREMENT_AND_CLAMP, "increment_and_clamp" },
        { CORE_STENCIL_OP_DECREMENT_AND_CLAMP, "decrement_and_clamp" },
        { CORE_STENCIL_OP_INVERT, "invert" },
        { CORE_STENCIL_OP_INCREMENT_AND_WRAP, "increment_and_wrap" },
        { CORE_STENCIL_OP_DECREMENT_AND_WRAP, "decrement_and_wrap" },
    })

CORE_JSON_SERIALIZE_ENUM(BlendFactor,
    {
        { CORE_BLEND_FACTOR_ZERO, "zero" },
        { CORE_BLEND_FACTOR_ONE, "one" },
        { CORE_BLEND_FACTOR_SRC_COLOR, "src_color" },
        { CORE_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, "one_minus_src_color" },
        { CORE_BLEND_FACTOR_DST_COLOR, "dst_color" },
        { CORE_BLEND_FACTOR_ONE_MINUS_DST_COLOR, "one_minus_dst_color" },
        { CORE_BLEND_FACTOR_SRC_ALPHA, "src_alpha" },
        { CORE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, "one_minus_src_alpha" },
        { CORE_BLEND_FACTOR_DST_ALPHA, "dst_alpha" },
        { CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, "one_minus_dst_alpha" },
        { CORE_BLEND_FACTOR_CONSTANT_COLOR, "constant_color" },
        { CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, "one_minus_constant_color" },
        { CORE_BLEND_FACTOR_CONSTANT_ALPHA, "constant_alpha" },
        { CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, "one_minus_constant_alpha" },
        { CORE_BLEND_FACTOR_SRC_ALPHA_SATURATE, "src_alpha_saturate" },
        { CORE_BLEND_FACTOR_SRC1_COLOR, "src1_color" },
        { CORE_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR, "one_minus_src1_color" },
        { CORE_BLEND_FACTOR_SRC1_ALPHA, "src1_alpha" },
        { CORE_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, "one_minus_src1_alpha" },
    })

CORE_JSON_SERIALIZE_ENUM(BlendOp,
    {
        { CORE_BLEND_OP_ADD, "add" },
        { CORE_BLEND_OP_SUBTRACT, "subtract" },
        { CORE_BLEND_OP_REVERSE_SUBTRACT, "reverse_subtract" },
        { CORE_BLEND_OP_MIN, "min" },
        { CORE_BLEND_OP_MAX, "max" },
    })

CORE_JSON_SERIALIZE_ENUM(ColorComponentFlagBits,
    {
        { (ColorComponentFlagBits)0, nullptr },
        { CORE_COLOR_COMPONENT_R_BIT, "r_bit" },
        { CORE_COLOR_COMPONENT_G_BIT, "g_bit" },
        { CORE_COLOR_COMPONENT_B_BIT, "b_bit" },
        { CORE_COLOR_COMPONENT_A_BIT, "a_bit" },
    })

CORE_JSON_SERIALIZE_ENUM(GraphicsStateFlagBits,
    {
        { (GraphicsStateFlagBits)0, nullptr },
        { CORE_GRAPHICS_STATE_INPUT_ASSEMBLY_BIT, "input_assembly_bit" },
        { CORE_GRAPHICS_STATE_RASTERIZATION_STATE_BIT, "rasterization_state_bit" },
        { CORE_GRAPHICS_STATE_DEPTH_STENCIL_STATE_BIT, "depth_stencil_state_bit" },
        { CORE_GRAPHICS_STATE_COLOR_BLEND_STATE_BIT, "color_blend_state_bit" },
    })

CORE_JSON_SERIALIZE_ENUM(LogicOp,
    {
        { CORE_LOGIC_OP_CLEAR, "clear" },
        { CORE_LOGIC_OP_AND, "and" },
        { CORE_LOGIC_OP_AND_REVERSE, "and_reverse" },
        { CORE_LOGIC_OP_COPY, "copy" },
        { CORE_LOGIC_OP_AND_INVERTED, "and_inverted" },
        { CORE_LOGIC_OP_NO_OP, "no_op" },
        { CORE_LOGIC_OP_XOR, "xor" },
        { CORE_LOGIC_OP_OR, "or" },
        { CORE_LOGIC_OP_NOR, "nor" },
        { CORE_LOGIC_OP_EQUIVALENT, "equivalent" },
        { CORE_LOGIC_OP_INVERT, "invert" },
        { CORE_LOGIC_OP_OR_REVERSE, "or_reverse" },
        { CORE_LOGIC_OP_COPY_INVERTED, "copy_inverted" },
        { CORE_LOGIC_OP_OR_INVERTED, "or_inverted" },
        { CORE_LOGIC_OP_NAND, "nand" },
        { CORE_LOGIC_OP_SET, "set" },
    })
// clang-format on
void FromJson(const json::value& jsonData, JsonContext<GraphicsState::InputAssembly>& context)
{
    SafeGetJsonValue(jsonData, "enablePrimitiveRestart", context.error, context.data.enablePrimitiveRestart);
    SafeGetJsonEnum(jsonData, "primitiveTopology", context.error, context.data.primitiveTopology);
}

void FromJson(const json::value& jsonData, JsonContext<GraphicsState::RasterizationState>& context)
{
    SafeGetJsonValue(jsonData, "enableDepthClamp", context.error, context.data.enableDepthClamp);
    SafeGetJsonValue(jsonData, "enableDepthBias", context.error, context.data.enableDepthBias);
    SafeGetJsonValue(jsonData, "enableRasterizerDiscard", context.error, context.data.enableRasterizerDiscard);

    SafeGetJsonEnum(jsonData, "polygonMode", context.error, context.data.polygonMode);
    SafeGetJsonBitfield<CullModeFlagBits>(jsonData, "cullModeFlags", context.error, context.data.cullModeFlags);
    SafeGetJsonEnum(jsonData, "frontFace", context.error, context.data.frontFace);

    SafeGetJsonValue(jsonData, "depthBiasConstantFactor", context.error, context.data.depthBiasConstantFactor);
    SafeGetJsonValue(jsonData, "depthBiasClamp", context.error, context.data.depthBiasClamp);
    SafeGetJsonValue(jsonData, "depthBiasSlopeFactor", context.error, context.data.depthBiasSlopeFactor);

    SafeGetJsonValue(jsonData, "lineWidth", context.error, context.data.lineWidth);
}

void FromJson(const json::value& jsonData, JsonContext<GraphicsState::StencilOpState>& context)
{
    SafeGetJsonEnum(jsonData, "failOp", context.error, context.data.failOp);
    SafeGetJsonEnum(jsonData, "passOp", context.error, context.data.passOp);
    SafeGetJsonEnum(jsonData, "depthFailOp", context.error, context.data.depthFailOp);
    SafeGetJsonEnum(jsonData, "compareOp", context.error, context.data.compareOp);
    SafeGetJsonMask(jsonData, "compareMask", context.error, context.data.compareMask);
    SafeGetJsonMask(jsonData, "writeMask", context.error, context.data.writeMask);
    SafeGetJsonMask(jsonData, "reference", context.error, context.data.reference);
}

void FromJson(const json::value& jsonData, JsonContext<GraphicsState::DepthStencilState>& context)
{
    SafeGetJsonValue(jsonData, "enableDepthTest", context.error, context.data.enableDepthTest);
    SafeGetJsonValue(jsonData, "enableDepthWrite", context.error, context.data.enableDepthWrite);
    SafeGetJsonValue(jsonData, "enableDepthBoundsTest", context.error, context.data.enableDepthBoundsTest);
    SafeGetJsonValue(jsonData, "enableStencilTest", context.error, context.data.enableStencilTest);

    SafeGetJsonValue(jsonData, "minDepthBounds", context.error, context.data.minDepthBounds);
    SafeGetJsonValue(jsonData, "maxDepthBounds", context.error, context.data.maxDepthBounds);

    SafeGetJsonEnum(jsonData, "depthCompareOp", context.error, context.data.depthCompareOp);

    if (const json::value* frontStencilStateIt = jsonData.find("frontStencilOpState"); frontStencilStateIt) {
        JsonContext<GraphicsState::StencilOpState> stencilContext;
        FromJson(*frontStencilStateIt, stencilContext);
        context.data.frontStencilOpState = stencilContext.data;
        if (!stencilContext.error.empty()) {
            context.error = stencilContext.error;
        }
    }

    if (const json::value* backStencilStateIt = jsonData.find("backStencilOpState"); backStencilStateIt) {
        JsonContext<GraphicsState::StencilOpState> stencilContext;
        FromJson(*backStencilStateIt, stencilContext);
        context.data.backStencilOpState = stencilContext.data;
        if (!stencilContext.error.empty()) {
            context.error = stencilContext.error;
        }
    }
}

void FromJson(const json::value& jsonData, JsonContext<GraphicsState::ColorBlendState::Attachment>& context)
{
    SafeGetJsonValue(jsonData, "enableBlend", context.error, context.data.enableBlend);
    SafeGetJsonBitfield<ColorComponentFlagBits>(jsonData, "colorWriteMask", context.error, context.data.colorWriteMask);

    SafeGetJsonEnum(jsonData, "srcColorBlendFactor", context.error, context.data.srcColorBlendFactor);
    SafeGetJsonEnum(jsonData, "dstColorBlendFactor", context.error, context.data.dstColorBlendFactor);
    SafeGetJsonEnum(jsonData, "colorBlendOp", context.error, context.data.colorBlendOp);

    SafeGetJsonEnum(jsonData, "srcAlphaBlendFactor", context.error, context.data.srcAlphaBlendFactor);
    SafeGetJsonEnum(jsonData, "dstAlphaBlendFactor", context.error, context.data.dstAlphaBlendFactor);
    SafeGetJsonEnum(jsonData, "alphaBlendOp", context.error, context.data.alphaBlendOp);
}

void FromJson(const json::value& jsonData, JsonContext<GraphicsState::ColorBlendState>& context)
{
    SafeGetJsonValue(jsonData, "enableLogicOp", context.error, context.data.enableLogicOp);
    SafeGetJsonEnum(jsonData, "logicOp", context.error, context.data.logicOp);

    if (const json::value* colorBlendConstantsIt = jsonData.find("colorBlendConstants"); colorBlendConstantsIt) {
        FromJson(*colorBlendConstantsIt, context.data.colorBlendConstants);
    }

    if (const json::value* colorAttachmentsIt = jsonData.find("colorAttachments"); colorAttachmentsIt) {
        vector<JsonContext<GraphicsState::ColorBlendState::Attachment>> colorContexts;
        FromJson(*colorAttachmentsIt, colorContexts);
        context.data.colorAttachmentCount = 0;
        for (auto& colorContext : colorContexts) {
            context.data.colorAttachments[context.data.colorAttachmentCount] = colorContext.data;
            context.data.colorAttachmentCount++;

            if (!colorContext.error.empty()) {
                context.error = colorContext.error;
            }
        }
    }
}

namespace ShaderStateLoaderUtil {
void ParseStateFlags(const CORE_NS::json::value& jsonData, GraphicsStateFlags& stateFlags, ShaderStateResult& ssr)
{
    SafeGetJsonBitfield<GraphicsStateFlagBits>(jsonData, "stateFlags", ssr.res.error, stateFlags);
}

void ParseSingleState(const json::value& jsonData, ShaderStateResult& ssr)
{
    GraphicsState graphicsState;
    // Read input assembly.
    if (const json::value* inputAssemblyIt = jsonData.find("inputAssembly"); inputAssemblyIt) {
        JsonContext<GraphicsState::InputAssembly> context;
        FromJson(*inputAssemblyIt, context);
        graphicsState.inputAssembly = context.data;

        if (!context.error.empty()) {
            ssr.res.error += context.error;
        }
    }

    // Read rasterization state.
    if (const json::value* rasterizationStateIt = jsonData.find("rasterizationState"); rasterizationStateIt) {
        JsonContext<GraphicsState::RasterizationState> context;
        FromJson(*rasterizationStateIt, context);
        graphicsState.rasterizationState = context.data;

        if (!context.error.empty()) {
            ssr.res.error += context.error;
        }
    }

    // Read depth stencil state
    if (const json::value* depthStencilStateIt = jsonData.find("depthStencilState"); depthStencilStateIt) {
        JsonContext<GraphicsState::DepthStencilState> context;
        FromJson(*depthStencilStateIt, context);
        graphicsState.depthStencilState = context.data;

        if (!context.error.empty()) {
            ssr.res.error += context.error;
        }
    }

    // Read depth color blend state.
    if (const json::value* colorBlendStateIt = jsonData.find("colorBlendState"); colorBlendStateIt) {
        JsonContext<GraphicsState::ColorBlendState> context;
        FromJson(*colorBlendStateIt, context);
        graphicsState.colorBlendState = context.data;

        if (!context.error.empty()) {
            ssr.res.error += context.error;
        }
    }

    ssr.res.success = ssr.res.error.empty();
    if (ssr.res.success) {
        ssr.states.states.push_back(move(graphicsState));
    }
}

ShaderStateResult LoadStates(const json::value& jsonData)
{
    ShaderStateResult ssr;
    if (const json::value* iter = jsonData.find("shaderStates"); iter) {
        const auto& allStates = iter->array_;
        ssr.states.states.reserve(allStates.size());
        for (auto const& state : allStates) {
            ShaderStateLoaderVariantData variant;
            SafeGetJsonValue(state, "variantName", ssr.res.error, variant.variantName);
            if (variant.variantName.empty()) {
                ssr.res.error += "graphics state variant name needs to be given\n";
            }
            if (ssr.res.error.empty()) {
                if (const json::value* graphicsStateIt = state.find("state"); graphicsStateIt) {
                    ParseSingleState(*graphicsStateIt, ssr);
                    if (ssr.res.error.empty()) {
                        SafeGetJsonValue(state, "baseShaderState", ssr.res.error, variant.baseShaderState);
                        SafeGetJsonValue(state, "baseVariantName", ssr.res.error, variant.baseVariantName);
                        SafeGetJsonValue(state, "slot", ssr.res.error, variant.renderSlot);
                        SafeGetJsonValue(state, "renderSlot", ssr.res.error, variant.renderSlot);
                        SafeGetJsonValue(
                            state, "renderSlotDefaultShaderState", ssr.res.error, variant.renderSlotDefaultState);
                        SafeGetJsonBitfield<GraphicsStateFlagBits>(
                            state, "stateFlags", ssr.res.error, variant.stateFlags);
                        ssr.states.variantData.push_back(move(variant));
                    }
                }
            }
        }
    }

    ssr.res.success = ssr.res.error.empty();
    if (!ssr.res.success) {
        ssr.res.error += "error loading shader states\n";
        PLUGIN_LOG_E("error loading shader states: %s", ssr.res.error.c_str());
        ssr.states.states.clear();
    }
    return ssr;
}
} // namespace ShaderStateLoaderUtil
RENDER_END_NAMESPACE()
