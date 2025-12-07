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

#include "text_system.h"

#include <ComponentTools/component_query.h>
#if defined(TEXT3D_ENABLE_EXTRUDING) && (TEXT3D_ENABLE_EXTRUDING)
#include <tesselator.h>
#endif

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <base/math/matrix_util.h>
#include <base/util/utf8_decode.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <core/property_tools/property_api_impl.inl>
#include <font/implementation_uids.h>
#include <font/intf_font_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <text_3d/ecs/components/text_component.h>

TEXT3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
namespace {
struct MeshData {
    BASE_NS::vector<BASE_NS::Math::Vec3> positions;
    BASE_NS::vector<BASE_NS::Math::Vec2> uvs;
    BASE_NS::vector<BASE_NS::Math::Vec3> normals;
    BASE_NS::vector<uint16_t> indices;
    RENDER_NS::RenderHandleReference atlas;
};

struct PathVertex {
    float x;
    float y;
};

MeshData GenerateMeshData(FONT_NS::IFont& font, BASE_NS::string_view text)
{
    // should really go through all glyphs and group based on atlas texture.
    // could also go through all the TextComponents and create one mesh for all the glyphs, submesh per atlas.
    MeshData data;

    float posX = 0.f;
    const char* str = text.data();
    while (const uint32_t codepoint = BASE_NS::GetCharUtf8(&str)) {
        const uint32_t glyphIndex = font.GetGlyphIndex(codepoint);
        const auto& metrics = font.GetGlyphMetrics(glyphIndex);
        const auto width = metrics.right - metrics.left;
        const auto bl = BASE_NS::Math::Vec3(posX + metrics.left - metrics.leftBearing, metrics.bottom, 0.f);
        const auto br = BASE_NS::Math::Vec3(bl.x + width, bl.y, 0.f);
        const auto tr = BASE_NS::Math::Vec3(br.x, metrics.top, 0.f);
        const auto tl = BASE_NS::Math::Vec3(bl.x, tr.y, 0.f);
        posX += metrics.advance;
        const auto i0 = static_cast<uint16_t>(data.positions.size());
        data.positions.push_back(bl);
        data.positions.push_back(br);
        data.positions.push_back(tr);
        data.positions.push_back(tl);

        const auto& info = font.GetGlyphInfo(glyphIndex);
        data.atlas = info.atlas;
        data.uvs.emplace_back(info.tl.x, info.br.y);
        data.uvs.push_back(info.br);
        data.uvs.emplace_back(info.br.x, info.tl.y);
        data.uvs.push_back(info.tl);

        data.indices.push_back(i0);
        data.indices.push_back(i0 + 2U);
        data.indices.push_back(i0 + 3U);

        data.indices.push_back(i0);
        data.indices.push_back(i0 + 1U);
        data.indices.push_back(i0 + 2U);
    }
    // +z facing normal for each vertex
    data.normals.append(data.positions.size(), { 0.f, 0.f, 1.f });
    return data;
}

#if defined(TEXT3D_ENABLE_EXTRUDING) && (TEXT3D_ENABLE_EXTRUDING)
void GenerateFrontBackFaces(const size_t vertexCount, const size_t elementCount, const size_t vertexOffset,
    const float depth, const TESSreal* vertices, const float posX, const Font::GlyphInfo& info,
    const TESSindex* elements, const Font::GlyphMetrics& metrics, int& elementOffset, MeshData& data)
{
    // front face vertices
    for (size_t ii = 0; ii < vertexCount; ii++) {
        const BASE_NS::Math::Vec3 pos(
            vertices[2 * ii] + (posX + metrics.left - metrics.leftBearing), vertices[2 * ii + 1], 0.f); // 2: index

        const BASE_NS::Math::Vec2 uv(
            info.tl.x + (vertices[2 * ii] / (metrics.right - metrics.left)) * (info.br.x - info.tl.x),      // 2: index
            info.br.y + (vertices[2 * ii + 1] / (metrics.top - metrics.bottom)) * (info.tl.y - info.br.y)); // 2: index

        data.positions.push_back(pos);
        data.uvs.push_back(uv);
        data.normals.push_back({ 0.f, 0.f, 1.f });
        elementOffset++;
    }

    // front face indices
    for (size_t ii = 0; ii < elementCount * 3; ii += 3) { // 3: index
        const uint16_t v0 = static_cast<uint16_t>(elements[ii] + vertexOffset);
        const uint16_t v1 = static_cast<uint16_t>(elements[ii + 1] + vertexOffset);
        const uint16_t v2 = static_cast<uint16_t>(elements[ii + 2] + vertexOffset);

        data.indices.push_back(v2);
        data.indices.push_back(v1);
        data.indices.push_back(v0);
    }

    // back face vertices
    for (size_t ii = 0; ii < vertexCount; ii++) {
        const BASE_NS::Math::Vec3 pos(
            vertices[2 * ii] + (posX + metrics.left - metrics.leftBearing), vertices[2 * ii + 1], -depth); // 2: index

        const BASE_NS::Math::Vec2 uv = data.uvs[ii];

        data.positions.push_back(pos);
        data.uvs.push_back(uv);
        data.normals.push_back({ 0.f, 0.f, -1.f });
        elementOffset++;
    }

    // back face indices
    for (size_t ii = 0; ii < elementCount * 3; ii += 3) { // 3: index
        const uint16_t v0 = static_cast<uint16_t>(elements[ii] + vertexOffset + vertexCount);
        const uint16_t v1 = static_cast<uint16_t>(elements[ii + 1] + vertexOffset + vertexCount);
        const uint16_t v2 = static_cast<uint16_t>(elements[ii + 2] + vertexOffset + vertexCount);

        data.indices.push_back(v0);
        data.indices.push_back(v1);
        data.indices.push_back(v2);
    }
}

void GenerateSideFaces(const float depth, const float posX, const Font::GlyphInfo& info,
    const BASE_NS::vector<Font::GlyphContour>& contours, const Font::GlyphMetrics& metrics, int& elementOffset,
    MeshData& data)
{
    // side faces
    for (const auto& contour : contours) {
        const size_t contourSize = contour.points.size();
        for (size_t ptIdx = 0; ptIdx < contourSize; ptIdx++) {
            const size_t nextPtIdx = (ptIdx + 1) % contourSize;
            const BASE_NS::Math::Vec2& pt = contour.points[ptIdx];
            const BASE_NS::Math::Vec2& nextPt = contour.points[nextPtIdx];

            // side face verticies
            const BASE_NS::Math::Vec3 frontFacePos1(pt.x + (posX + metrics.left - metrics.leftBearing), pt.y, 0.f);
            const BASE_NS::Math::Vec3 frontFacePos2(
                nextPt.x + (posX + metrics.left - metrics.leftBearing), nextPt.y, 0.f);
            const BASE_NS::Math::Vec3 backFacePos1(pt.x + (posX + metrics.left - metrics.leftBearing), pt.y, -depth);
            const BASE_NS::Math::Vec3 backFacePos2(
                nextPt.x + (posX + metrics.left - metrics.leftBearing), nextPt.y, -depth);

            data.positions.push_back(frontFacePos1);
            data.positions.push_back(backFacePos1);
            data.positions.push_back(frontFacePos2);
            data.positions.push_back(backFacePos2);

            const float u1 = info.tl.x + (pt.x / (metrics.right - metrics.left)) * (info.br.x - info.tl.x);
            const float u2 = info.tl.x + (nextPt.x / (metrics.right - metrics.left)) * (info.br.x - info.tl.x);

            data.uvs.emplace_back(u1, info.br.y);
            data.uvs.emplace_back(u1, info.tl.y);
            data.uvs.emplace_back(u2, info.br.y);
            data.uvs.emplace_back(u2, info.tl.y);

            elementOffset += 4; // 4: offset

            const BASE_NS::Math::Vec3 edge1 = backFacePos1 - frontFacePos1;
            const BASE_NS::Math::Vec3 edge2 = frontFacePos2 - frontFacePos1;
            const BASE_NS::Math::Vec3 normal = Math::Normalize(Math::Cross(edge1, edge2));

            data.normals.push_back(-normal);
            data.normals.push_back(-normal);
            data.normals.push_back(-normal);
            data.normals.push_back(-normal);

            const size_t sideVertOffset = data.positions.size() - 4;

            // side face indices
            data.indices.push_back(static_cast<uint16_t>(sideVertOffset + 2)); // 2: offset
            data.indices.push_back(static_cast<uint16_t>(sideVertOffset + 1));
            data.indices.push_back(static_cast<uint16_t>(sideVertOffset + 0));

            data.indices.push_back(static_cast<uint16_t>(sideVertOffset + 3)); // 3: offset
            data.indices.push_back(static_cast<uint16_t>(sideVertOffset + 1));
            data.indices.push_back(static_cast<uint16_t>(sideVertOffset + 2)); // 2: offset
        }
    }
}

MeshData GenerateMeshData3D(FONT_NS::IFont& font, BASE_NS::string_view text, const float depth)
{
    MeshData data;
    float posX = 0.f;
    size_t vertexOffset = 0;
    TESStesselator* tessellator = tessNewTess(nullptr);
    tessSetOption(tessellator, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);

    const char* str = text.data();
    while (const uint32_t codepoint = BASE_NS::GetCharUtf8(&str)) {
        const uint32_t glyphIndex = font.GetGlyphIndex(codepoint);
        const auto& metrics = font.GetGlyphMetrics(glyphIndex);
        const auto& info = font.GetGlyphInfo(glyphIndex);

        const auto contours = font.GetGlyphContours(glyphIndex);

        for (const auto& contour : contours) {
            tessAddContour(tessellator, 2, contour.points.data(), sizeof(BASE_NS::Math::Vec2), // 2: param
                static_cast<int>(contour.points.size()));
        }

        if (tessTesselate(tessellator, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, nullptr)) { // 3: param 2: param
            const size_t vertexCount = static_cast<size_t>(tessGetVertexCount(tessellator));
            const TESSreal* vertices = tessGetVertices(tessellator);
            const size_t elementCount = static_cast<size_t>(tessGetElementCount(tessellator));
            const TESSindex* elements = tessGetElements(tessellator);

            int elementOffset = 0;

            GenerateFrontBackFaces(vertexCount, elementCount, vertexOffset, depth, vertices, posX, info, elements,
                metrics, elementOffset, data);

            GenerateSideFaces(depth, posX, info, contours, metrics, elementOffset, data);

            vertexOffset += elementOffset;
        }
        posX += metrics.advance;
    }

    tessDeleteTess(tessellator);
    return data;
}
#endif

template<typename T>
constexpr inline IMeshBuilder::DataBuffer FillData(array_view<const T> c) noexcept
{
    Format format = BASE_FORMAT_UNDEFINED;
    if constexpr (is_same_v<T, Math::Vec2>) {
        format = BASE_FORMAT_R32G32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec3>) {
        format = BASE_FORMAT_R32G32B32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec4>) {
        format = BASE_FORMAT_R32G32B32A32_SFLOAT;
    } else if constexpr (is_same_v<T, uint16_t>) {
        format = BASE_FORMAT_R16_UINT;
    } else if constexpr (is_same_v<T, uint32_t>) {
        format = BASE_FORMAT_R32_UINT;
    }
    return IMeshBuilder::DataBuffer { format, sizeof(T),
        { reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(T) } };
}

template<typename T>
constexpr inline IMeshBuilder::DataBuffer FillData(const vector<T>& c) noexcept
{
    return FillData(array_view<const T> { c });
}

VertexInputDeclarationView GetVertexInputDeclaration(IRenderContext& renderContext)
{
    IShaderManager& shaderManager = renderContext.GetDevice().GetShaderManager();
    return shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
        DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));
}
} // namespace

struct TextSystem::TextData {
    uint32_t generation;
    BASE_NS::string text;
    BASE_NS::string fontFamily;
    BASE_NS::string fontStyle;
    float fontSize;
    float font3DThickness { 10.0f };
    TextComponent::FontMethod fontMethod { TextComponent::FontMethod::RASTER };
    FONT_NS::IFont::Ptr font;
    CORE_NS::EntityReference mesh;
};

void TextSystem::SetActive(bool state)
{
    active_ = (state) && (fontManager_);
}

bool TextSystem::IsActive() const
{
    return active_;
}

TextSystem::TextSystem(IEcs& ecs)
    : ecs_(ecs), meshManager_(GetManager<IMeshComponentManager>(ecs)),
      renderHandleManager_(GetManager<IRenderHandleComponentManager>(ecs)),
      renderMeshManager_(GetManager<IRenderMeshComponentManager>(ecs)),
      textManager_(GetManager<ITextComponentManager>(ecs))
{
    if (auto* engineClassRegister = ecs_.GetClassFactory().GetInterface<IClassRegister>()) {
        if (auto* renderClassRegister =
                CORE_NS::GetInstance<CORE_NS::IClassRegister>(*engineClassRegister, RENDER_NS::UID_RENDER_CONTEXT)) {
            fontManager_ = CORE_NS::GetInstance<FONT_NS::IFontManager>(*renderClassRegister, FONT_NS::UID_FONT_MANAGER);
            active_ = static_cast<bool>(fontManager_);
        }
    }
}

TextSystem::~TextSystem() = default;

string_view TextSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid TextSystem::GetUid() const
{
    return UID;
}

const IPropertyHandle* TextSystem::GetProperties() const
{
    return nullptr;
}

IPropertyHandle* TextSystem::GetProperties()
{
    return nullptr;
}

void TextSystem::SetProperties(const IPropertyHandle&) {}

const IEcs& TextSystem::GetECS() const
{
    return ecs_;
}

void TextSystem::Initialize()
{
    if (auto* engineClassRegister = ecs_.GetClassFactory().GetInterface<IClassRegister>()) {
        if (auto* renderContext =
                CORE_NS::GetInstance<RENDER_NS::IRenderContext>(*engineClassRegister, RENDER_NS::UID_RENDER_CONTEXT)) {
            RENDER_NS::IShaderManager& shaderManager = renderContext->GetDevice().GetShaderManager();

            const auto renderSlot = DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT;
            const uint32_t renderSlotId = shaderManager.GetRenderSlotId(renderSlot);
            const RENDER_NS::IShaderManager::RenderSlotData rsd = shaderManager.GetRenderSlotData(renderSlotId);

            rasterShader_.shader = rsd.shader;
            rasterShader_.graphicsState = rsd.graphicsState;
            rasterShader_.depthShader =
                shaderManager.GetShaderHandle("3dshaders://shader/core3d_dm_depth_alpha.shader");

            sdfShader_.shader = shaderManager.GetShaderHandle("text3dshaders://shader/plugin_core3d_dm_fw_sdf.shader");
            sdfShader_.graphicsState = rsd.graphicsState;
            sdfShader_.depthShader =
                shaderManager.GetShaderHandle("text3dshaders://shader/plugin_core3d_dm_depth_alpha_sdf.shader");
        }
    }
}

bool TextSystem::Update(bool frameRenderingQueued, uint64_t, uint64_t)
{
    if (!active_) {
        return false;
    }

    const auto counter = textManager_->GetGenerationCounter();
    if (textManagerGeneration_ == counter) {
        return false;
    }
    textManagerGeneration_ = counter;

    const auto components = textManager_->GetComponentCount();
    for (IComponentManager::ComponentId id = 0U; id < components; ++id) {
        const auto entity = textManager_->GetEntity(id);
        auto pos = textCache_.find(entity);
        if (pos != textCache_.end()) {
            const auto generation = textManager_->GetComponentGeneration(id);
            if (pos->second.generation == generation) {
                continue;
            }
        } else {
            auto result = textCache_.insert({ entity, {} });
            pos = result.first;
        }
        pos->second.generation = textManager_->GetComponentGeneration(id);

        auto textHandle = textManager_->Read(id);
        if ((pos->second.fontFamily != textHandle->fontFamily) || (pos->second.fontStyle != textHandle->fontStyle)) {
            pos->second.fontFamily = textHandle->fontFamily;
            pos->second.fontStyle = textHandle->fontStyle;
            if (auto* typeface = fontManager_->GetTypeFace(textHandle->fontFamily, textHandle->fontStyle)) {
                pos->second.font = fontManager_->CreateFont(*typeface);
            }
        }
        if (!pos->second.font) {
            CORE_LOG_ONCE_E(to_hex(entity.id), "Failed to create font with '%s' '%s'", textHandle->fontFamily.data(),
                textHandle->fontStyle.data());
            continue;
        }
        if (pos->second.fontSize != textHandle->fontSize) {
            pos->second.fontSize = textHandle->fontSize;
            pos->second.font->SetSize(textHandle->fontSize);
        }
        if (pos->second.fontMethod != textHandle->fontMethod) {
            pos->second.fontMethod = textHandle->fontMethod;
            pos->second.font->SetMethod((textHandle->fontMethod == TextComponent::FontMethod::SDF)
                                            ? Font::FontMethod::SDF
                                            : Font::FontMethod::RASTER);
        }
        pos->second.text = textHandle->text;
        GenerateMesh(*textHandle, pos->first, pos->second);
    }
    fontManager_->UploadPending();

    return true;
}

void TextSystem::GenerateMesh(const TextComponent& textComponent, CORE_NS::Entity entity, struct TextData& cached)
{
    if (!cached.font) {
        return;
    }

    auto* engineClassRegister = ecs_.GetClassFactory().GetInterface<IClassRegister>();
    if (!engineClassRegister) {
        return;
    }
    auto* renderContext = GetInstance<IRenderContext>(*engineClassRegister, UID_RENDER_CONTEXT);
    if (!renderContext) {
        return;
    }
    auto* renderClassFactory = renderContext->GetInterface<IClassFactory>();
    if (!renderClassFactory) {
        return;
    }

#if defined(TEXT3D_ENABLE_EXTRUDING) && (TEXT3D_ENABLE_EXTRUDING)
    const bool extrudedGeometry = (textComponent.fontMethod == TextComponent::FontMethod::TEXT3D);
    const auto data = extrudedGeometry ? GenerateMeshData3D(*cached.font, textComponent.text, cached.font3DThickness)
                                       : GenerateMeshData(*cached.font, textComponent.text);
#else
    const bool extrudedGeometry = false;
    const auto data = GenerateMeshData(*cached.font, textComponent.text);
#endif
    auto builder = CreateInstance<IMeshBuilder>(*renderClassFactory, UID_MESH_BUILDER);
    builder->Initialize(GetVertexInputDeclaration(*renderContext), 1U);
    builder->AddSubmesh(IMeshBuilder::Submesh { static_cast<uint32_t>(data.positions.size()),
        static_cast<uint32_t>(data.indices.size()), 1U, 0U, IndexType::CORE_INDEX_TYPE_UINT16, entity, false, false,
        false });
    builder->Allocate();

    static constexpr auto empty = IMeshBuilder::DataBuffer {};
    auto positionData = FillData(data.positions);
    builder->SetVertexData(0U, positionData, FillData(data.normals), FillData(data.uvs), empty, empty, empty);
    builder->CalculateAABB(0U, positionData);
    builder->SetIndexData(0U, FillData(data.indices));

    auto meshEntity = builder->CreateMesh(ecs_);
    renderMeshManager_->Create(entity);
    renderMeshManager_->Write(entity)->mesh = meshEntity;

    cached.mesh = ecs_.GetEntityManager().GetReferenceCounted(meshEntity);

    // NOTE: ATM the materials created in the system are not available current frame
    // The create-event happens after the system are processed
    if (data.atlas || extrudedGeometry) {
        auto* materialManager = GetManager<IMaterialComponentManager>(ecs_);
        auto id = materialManager->GetComponentId(entity);
        if (id == IComponentManager::INVALID_COMPONENT_ID) {
            materialManager->Create(entity);
            id = materialManager->GetComponentId(entity);
        }
        auto materialHandle = materialManager->Write(id);
        if (!materialHandle) {
            return;
        }
        materialHandle->textures[0U].image =
            GetOrCreateEntityReference(ecs_.GetEntityManager(), *renderHandleManager_, data.atlas);

        if (!extrudedGeometry) {
            const auto& shaderData =
                (textComponent.fontMethod == TextComponent::FontMethod::SDF) ? sdfShader_ : rasterShader_;
            materialHandle->materialShader.shader =
                GetOrCreateEntityReference(ecs_.GetEntityManager(), *renderHandleManager_, shaderData.shader);
            materialHandle->materialShader.graphicsState =
                GetOrCreateEntityReference(ecs_.GetEntityManager(), *renderHandleManager_, shaderData.graphicsState);
            materialHandle->depthShader.shader =
                GetOrCreateEntityReference(ecs_.GetEntityManager(), *renderHandleManager_, shaderData.depthShader);
        }
    }
}

void TextSystem::Uninitialize() {}

ISystem* TextSystemInstance(IEcs& ecs)
{
    return new TextSystem(ecs);
}

void TextSystemDestroy(ISystem* instance)
{
    delete static_cast<TextSystem*>(instance);
}
TEXT3D_END_NAMESPACE()
