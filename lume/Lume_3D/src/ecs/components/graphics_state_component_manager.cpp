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

#include <3d/ecs/components/graphics_state_component.h>
#include <core/property/property.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::StencilOp);
ENUM_TYPE_METADATA(RENDER_NS::StencilOp, ENUM_VALUE(CORE_STENCIL_OP_KEEP, "Keep"),
    ENUM_VALUE(CORE_STENCIL_OP_ZERO, "Zero"), ENUM_VALUE(CORE_STENCIL_OP_REPLACE, "Replace"),
    ENUM_VALUE(CORE_STENCIL_OP_INVERT, "Invert"),
    ENUM_VALUE(CORE_STENCIL_OP_INCREMENT_AND_CLAMP, "Increment And Clamp"),
    ENUM_VALUE(CORE_STENCIL_OP_DECREMENT_AND_CLAMP, "Decrement And Clamp"),
    ENUM_VALUE(CORE_STENCIL_OP_INCREMENT_AND_WRAP, "Increment And Wrap"),
    ENUM_VALUE(CORE_STENCIL_OP_DECREMENT_AND_WRAP, "Decrement And Wrap"))

DECLARE_PROPERTY_TYPE(RENDER_NS::CompareOp);
ENUM_TYPE_METADATA(RENDER_NS::CompareOp, ENUM_VALUE(CORE_COMPARE_OP_NEVER, "Never"),
    ENUM_VALUE(CORE_COMPARE_OP_ALWAYS, "Always"), ENUM_VALUE(CORE_COMPARE_OP_EQUAL, "Equal"),
    ENUM_VALUE(CORE_COMPARE_OP_NOT_EQUAL, "Not Equal"), ENUM_VALUE(CORE_COMPARE_OP_LESS, "Less"),
    ENUM_VALUE(CORE_COMPARE_OP_LESS_OR_EQUAL, "Less Or Equal"), ENUM_VALUE(CORE_COMPARE_OP_GREATER, "Greater"),
    ENUM_VALUE(CORE_COMPARE_OP_GREATER_OR_EQUAL, "Greater Or Equal"))

DECLARE_PROPERTY_TYPE(RENDER_NS::PolygonMode);
ENUM_TYPE_METADATA(RENDER_NS::PolygonMode, ENUM_VALUE(CORE_POLYGON_MODE_FILL, "Fill"),
    ENUM_VALUE(CORE_POLYGON_MODE_LINE, "Line"), ENUM_VALUE(CORE_POLYGON_MODE_POINT, "Point"))

DECLARE_PROPERTY_TYPE(RENDER_NS::CullModeFlagBits);
ENUM_TYPE_METADATA(RENDER_NS::CullModeFlagBits, ENUM_VALUE(CORE_CULL_MODE_FRONT_BIT, "Front"),
    ENUM_VALUE(CORE_CULL_MODE_BACK_BIT, "Back"))

DECLARE_PROPERTY_TYPE(RENDER_NS::FrontFace);
ENUM_TYPE_METADATA(RENDER_NS::FrontFace, ENUM_VALUE(CORE_FRONT_FACE_COUNTER_CLOCKWISE, "Counter Clockwise"),
    ENUM_VALUE(CORE_FRONT_FACE_CLOCKWISE, "Clockwise"))

DECLARE_PROPERTY_TYPE(RENDER_NS::LogicOp);
ENUM_TYPE_METADATA(RENDER_NS::LogicOp, ENUM_VALUE(CORE_LOGIC_OP_NO_OP, "No op"),
    ENUM_VALUE(CORE_LOGIC_OP_CLEAR, "Clear"), ENUM_VALUE(CORE_LOGIC_OP_COPY, "Copy"),
    ENUM_VALUE(CORE_LOGIC_OP_EQUIVALENT, "Equivalent"), ENUM_VALUE(CORE_LOGIC_OP_INVERT, "Invert"),
    ENUM_VALUE(CORE_LOGIC_OP_SET, "Set"), ENUM_VALUE(CORE_LOGIC_OP_COPY_INVERTED, "Copy Inverted"),
    ENUM_VALUE(CORE_LOGIC_OP_AND, "AND"), ENUM_VALUE(CORE_LOGIC_OP_OR, "OR"), ENUM_VALUE(CORE_LOGIC_OP_XOR, "XOR"),
    ENUM_VALUE(CORE_LOGIC_OP_NOR, "NOR"), ENUM_VALUE(CORE_LOGIC_OP_NAND, "NAND"),
    ENUM_VALUE(CORE_LOGIC_OP_AND_REVERSE, "AND Reverse"), ENUM_VALUE(CORE_LOGIC_OP_AND_INVERTED, "AND Inverted"),
    ENUM_VALUE(CORE_LOGIC_OP_OR_REVERSE, "OR Reverse"), ENUM_VALUE(CORE_LOGIC_OP_OR_INVERTED, "OR Inverted"))

DECLARE_PROPERTY_TYPE(RENDER_NS::PrimitiveTopology);
ENUM_TYPE_METADATA(RENDER_NS::PrimitiveTopology, ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_POINT_LIST, "Point List"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_LIST, "Line List"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP, "Line Strip"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "Triangle List"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, "Triangle Strip"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, " Triangle Fan"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY, "Line Strip with Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY, "Line Strip with Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, "Triangle List with Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, "Triangle Strip with Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST, "Patch List"))

DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState::StencilOpState);
DATA_TYPE_METADATA(RENDER_NS::GraphicsState::StencilOpState, MEMBER_PROPERTY(failOp, "Fail Operation", 0),
    MEMBER_PROPERTY(passOp, "Pass Operation", 0), MEMBER_PROPERTY(depthFailOp, "Depth Fail Operation", 0),
    MEMBER_PROPERTY(compareOp, "Compare Operation", 0), MEMBER_PROPERTY(compareMask, "Compare Mask", 0),
    MEMBER_PROPERTY(writeMask, "Write Mask", 0), MEMBER_PROPERTY(reference, "Reference", 0))

DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState::RasterizationState);
DATA_TYPE_METADATA(RENDER_NS::GraphicsState::RasterizationState,
    MEMBER_PROPERTY(enableDepthClamp, "Enable Depth Clamp", 0),
    MEMBER_PROPERTY(enableDepthBias, "Enable Depth Bias", 0),
    MEMBER_PROPERTY(enableRasterizerDiscard, "Enable Rasterizer Discard", 0),
    MEMBER_PROPERTY(polygonMode, "Polygon Mode", 0),
    BITFIELD_MEMBER_PROPERTY(cullModeFlags, "Cull Mode Flags", PropertyFlags::IS_BITFIELD, RENDER_NS::CullModeFlagBits),
    MEMBER_PROPERTY(frontFace, "Front Face", 0),
    MEMBER_PROPERTY(depthBiasConstantFactor, "Depth Bias Constant Factor", 0),
    MEMBER_PROPERTY(depthBiasClamp, "Depth Bias Clamp", 0),
    MEMBER_PROPERTY(depthBiasSlopeFactor, "Depth Bias Slope Factor", 0), MEMBER_PROPERTY(lineWidth, "Line Width", 0))

DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState::DepthStencilState);
DATA_TYPE_METADATA(RENDER_NS::GraphicsState::DepthStencilState, MEMBER_PROPERTY(enableDepthTest, "Depth Test", 0),
    MEMBER_PROPERTY(enableDepthWrite, "Depth Write", 0), MEMBER_PROPERTY(enableDepthBoundsTest, "Depth Bounds Test", 0),
    MEMBER_PROPERTY(enableStencilTest, "Stencil Test", 0), MEMBER_PROPERTY(minDepthBounds, "Min Depth Bounds", 0),
    MEMBER_PROPERTY(maxDepthBounds, "Max Depth Bounds", 0), MEMBER_PROPERTY(depthCompareOp, "Depth CompareOp", 0),
    MEMBER_PROPERTY(frontStencilOpState, "Front StencilOp State", 0),
    MEMBER_PROPERTY(backStencilOpState, "Back StencilOp State", 0))

DECLARE_PROPERTY_TYPE(RENDER_NS::ColorComponentFlagBits);
ENUM_TYPE_METADATA(RENDER_NS::ColorComponentFlagBits, ENUM_VALUE(CORE_COLOR_COMPONENT_R_BIT, "Red"),
    ENUM_VALUE(CORE_COLOR_COMPONENT_G_BIT, "Green"), ENUM_VALUE(CORE_COLOR_COMPONENT_B_BIT, "Blue"),
    ENUM_VALUE(CORE_COLOR_COMPONENT_A_BIT, "Alpha"))

DECLARE_PROPERTY_TYPE(RENDER_NS::BlendFactor);
ENUM_TYPE_METADATA(RENDER_NS::BlendFactor, ENUM_VALUE(CORE_BLEND_FACTOR_ZERO, "Zero"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE, "One"), ENUM_VALUE(CORE_BLEND_FACTOR_SRC_COLOR, "Source Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, "One Minus Source Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_DST_COLOR, "Destination Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_DST_COLOR, "One Minus Destination Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_SRC_ALPHA, "Source Alpha"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, "One Minus Source Alpha"),
    ENUM_VALUE(CORE_BLEND_FACTOR_DST_ALPHA, "Destination Alpha"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, "One Minus Destination Alpha"),
    ENUM_VALUE(CORE_BLEND_FACTOR_CONSTANT_COLOR, "Constant Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, "One Minus Constant Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_CONSTANT_ALPHA, "Constant Alpha"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, "One Minus Constant Alpha"),
    ENUM_VALUE(CORE_BLEND_FACTOR_SRC_ALPHA_SATURATE, "Source Alpha Saturate"),
    ENUM_VALUE(CORE_BLEND_FACTOR_SRC1_COLOR, "Source One Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR, "One Minus Source One Color"),
    ENUM_VALUE(CORE_BLEND_FACTOR_SRC1_ALPHA, "Source One Alpha"),
    ENUM_VALUE(CORE_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, "One Minus Source One Alpha"))

DECLARE_PROPERTY_TYPE(RENDER_NS::BlendOp);
ENUM_TYPE_METADATA(RENDER_NS::BlendOp, ENUM_VALUE(CORE_BLEND_OP_ADD, "Add"),
    ENUM_VALUE(CORE_BLEND_OP_SUBTRACT, "Subtract"), ENUM_VALUE(CORE_BLEND_OP_REVERSE_SUBTRACT, "Reverse Subtract"),
    ENUM_VALUE(CORE_BLEND_OP_MIN, "Min"), ENUM_VALUE(CORE_BLEND_OP_MAX, "Max"))

DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState::ColorBlendState::Attachment);
DATA_TYPE_METADATA(RENDER_NS::GraphicsState::ColorBlendState::Attachment,
    MEMBER_PROPERTY(enableBlend, "Enable Blend", 0),
    BITFIELD_MEMBER_PROPERTY(
        colorWriteMask, "Color Write Mask", PropertyFlags::IS_BITFIELD, RENDER_NS::ColorComponentFlagBits),
    MEMBER_PROPERTY(srcColorBlendFactor, "Source Color Blend Factor", 0),
    MEMBER_PROPERTY(dstColorBlendFactor, "Destination Color Blend Factor", 0),
    MEMBER_PROPERTY(colorBlendOp, "Color Blend Operation", 0),
    MEMBER_PROPERTY(srcAlphaBlendFactor, "Source Alpha Blend Factor", 0),
    MEMBER_PROPERTY(dstAlphaBlendFactor, "Destination Alpha Blend Factor", 0),
    MEMBER_PROPERTY(alphaBlendOp, "Alpha Blend Operation", 0))

DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState::ColorBlendState);
DATA_TYPE_METADATA(RENDER_NS::GraphicsState::ColorBlendState,
    MEMBER_PROPERTY(enableLogicOp, "Enable Logic Operation", 0), MEMBER_PROPERTY(logicOp, "Logic Operatio", 0),
    MEMBER_PROPERTY(colorBlendConstants, "Color Blend Constants (R, G, B, A)", 0),
    MEMBER_PROPERTY(colorAttachmentCount, "Color Attachment Count", 0),
    MEMBER_PROPERTY(colorAttachments, "Color Attachments", 0))

DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState::InputAssembly);
DATA_TYPE_METADATA(RENDER_NS::GraphicsState::InputAssembly,
    MEMBER_PROPERTY(enablePrimitiveRestart, "Enable Primitive Restart", 0),
    MEMBER_PROPERTY(primitiveTopology, "Primitive Topology", 0))

DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState);
DATA_TYPE_METADATA(RENDER_NS::GraphicsState, MEMBER_PROPERTY(inputAssembly, "", 0),
    MEMBER_PROPERTY(rasterizationState, "", 0), MEMBER_PROPERTY(depthStencilState, "", 0),
    MEMBER_PROPERTY(colorBlendState, "", 0))

DECLARE_PROPERTY_TYPE(CORE3D_NS::GraphicsStateComponent);
DATA_TYPE_METADATA(
    CORE3D_NS::GraphicsStateComponent, MEMBER_PROPERTY(graphicsState, "", 0), MEMBER_PROPERTY(renderSlot, "", 0))

CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class GraphicsStateComponentManager final
    : public CORE_NS::BaseManager<GraphicsStateComponent, IGraphicsStateComponentManager> {
public:
    explicit GraphicsStateComponentManager(CORE_NS::IEcs& ecs)
        : CORE_NS::BaseManager<GraphicsStateComponent, IGraphicsStateComponentManager>(
              ecs, CORE_NS::GetName<IGraphicsStateComponentManager>(), 0U)
    {}

    ~GraphicsStateComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(CORE_NS::PropertyType::DataType<GraphicsStateComponent>::properties);
    }

    const CORE_NS::Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(CORE_NS::PropertyType::DataType<GraphicsStateComponent>::properties)) {
            return &CORE_NS::PropertyType::DataType<GraphicsStateComponent>::properties[index];
        }
        return nullptr;
    }

    BASE_NS::array_view<const CORE_NS::Property> MetaData() const override
    {
        return CORE_NS::PropertyType::DataType<GraphicsStateComponent>::MetaDataFromType();
    }
};

CORE_NS::IComponentManager* IGraphicsStateComponentManagerInstance(CORE_NS::IEcs& ecs)
{
    return new GraphicsStateComponentManager(ecs);
}

void IGraphicsStateComponentManagerDestroy(CORE_NS::IComponentManager* instance)
{
    delete static_cast<GraphicsStateComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
