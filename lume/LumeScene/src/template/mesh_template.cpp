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

#include "mesh_template.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_utils.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/resource/intf_resource_context.h>

#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <core/log.h>
#include <core/plugin/intf_class_factory.h>

#include <meta/api/util.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace RENDER_NS;

SCENE_BEGIN_NAMESPACE()

namespace {

IMeshBuilder::DataBuffer GetDataBuffer(
    const array_view<const uint8_t>& binData, uint64_t offset, uint64_t size, Format format, uint32_t stride)
{
    // Bounds check phrased as `size > binData.size() - offset` so it survives
    // a maliciously large `offset + size` that would otherwise wrap around in
    // uint64 arithmetic. `offset > binData.size()` is the early-out for
    // offsets past the end.
    if (offset > binData.size() || size > binData.size() - offset) {
        CORE_LOG_W("MeshTemplate: buffer region [%llu, %llu) exceeds binary data size %zu",
            static_cast<unsigned long long>(offset),
            static_cast<unsigned long long>(offset + size),
            binData.size());
        return {};
    }
    if (size == 0) {
        return {};
    }
    return IMeshBuilder::DataBuffer{format, stride, {binData.data() + offset, size}};
}

const VertexBufferRegion* FindBufferBySemantic(const vector<VertexBufferRegion>& buffers, string_view semantic)
{
    for (const auto& buf : buffers) {
        if (buf.semantic == semantic) {
            return &buf;
        }
    }
    return nullptr;
}

IMeshBuilder::DataBuffer GetAttributeData(
    const array_view<const uint8_t>& binData, const SubmeshDesc& submesh, const VertexAttribute* attr)
{
    if (!attr) {
        return {};
    }
    auto* region = FindBufferBySemantic(submesh.buffers, attr->semantic);
    if (!region) {
        return {};
    }
    return GetDataBuffer(binData, region->offset, region->size, attr->format, attr->stride);
}

const VertexAttribute* FindAttr(const vector<VertexAttribute>& attrs, string_view semantic)
{
    for (const auto& a : attrs) {
        if (a.semantic == semantic) {
            return &a;
        }
    }
    return nullptr;
}

void ConfigureSubmesh(const SubmeshDesc& sm, IMeshBuilder::Submesh& out)
{
    out.vertexCount = sm.vertexCount;
    out.indexCount = sm.indexCount;
    out.indexType = sm.indexType;
    out.tangents = Any(sm.flags & SubmeshFlag::TANGENTS_BIT);
    out.colors = Any(sm.flags & SubmeshFlag::VERTEX_COLORS_BIT);
    out.joints = Any(sm.flags & SubmeshFlag::SKIN_BIT);
    out.inputAssembly.primitiveTopology = sm.topology;
}

void FillSubmeshData(IMeshBuilder& builder, size_t i, const SubmeshDesc& sm, const array_view<const uint8_t>& binView,
    const vector<VertexAttribute>& attrs)
{
    const auto* posAttr = FindAttr(attrs, MeshSemantic::POSITION);
    const auto* normAttr = FindAttr(attrs, MeshSemantic::NORMAL);
    const auto* uv0Attr = FindAttr(attrs, MeshSemantic::UV0);
    const auto* uv1Attr = FindAttr(attrs, MeshSemantic::UV1);
    const auto* tangentAttr = FindAttr(attrs, MeshSemantic::TANGENT);
    const auto* colorAttr = FindAttr(attrs, MeshSemantic::COLOR);

    auto positions = GetAttributeData(binView, sm, posAttr);
    auto normals = GetAttributeData(binView, sm, normAttr);
    auto uv0 = GetAttributeData(binView, sm, uv0Attr);
    auto uv1 = GetAttributeData(binView, sm, uv1Attr);
    auto tangents = GetAttributeData(binView, sm, tangentAttr);
    auto colors = GetAttributeData(binView, sm, colorAttr);

    builder.SetVertexData(i, positions, normals, uv0, uv1, tangents, colors);
    builder.SetAABB(i, sm.aabb.min, sm.aabb.max);

    if (sm.indexCount > 0 && sm.indexBuffer.size > 0) {
        Format idxFormat =
            (sm.indexType == IndexType::CORE_INDEX_TYPE_UINT16) ? BASE_FORMAT_R16_UINT : BASE_FORMAT_R32_UINT;
        uint32_t idxStride = (sm.indexType == IndexType::CORE_INDEX_TYPE_UINT16) ? 2u : 4u;
        auto indexData = GetDataBuffer(binView, sm.indexBuffer.offset, sm.indexBuffer.size, idxFormat, idxStride);
        builder.SetIndexData(i, indexData);
    }

    if (Any(sm.flags & SubmeshFlag::SKIN_BIT)) {
        const auto* jointsAttr = FindAttr(attrs, MeshSemantic::JOINTS);
        const auto* weightsAttr = FindAttr(attrs, MeshSemantic::WEIGHTS);
        auto jointData = GetAttributeData(binView, sm, jointsAttr);
        auto weightData = GetAttributeData(binView, sm, weightsAttr);
        if (jointData.buffer.size() > 0 && weightData.buffer.size() > 0) {
            builder.SetJointData(i, jointData, weightData, positions);
        } else {
            CORE_LOG_W("Submesh %zu has 'skin' flag but missing joint or weight buffer; skipped", i);
        }
    }
}

CORE_NS::Entity BuildEntity(const IInternalScene::Ptr& scene, const vector<SubmeshDesc>& submeshes,
    const vector<VertexAttribute>& attrs, const vector<uint8_t>& binData)
{
    auto& renderContext = scene->GetRenderContext();
    IShaderManager& shaderManager = renderContext.GetDevice().GetShaderManager();
    const VertexInputDeclarationView vertexInputDeclaration =
        shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
            DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));

    auto builder = CreateInstance<IMeshBuilder>(renderContext, UID_MESH_BUILDER);
    if (!builder) {
        CORE_LOG_E("MeshTemplate: failed to create IMeshBuilder");
        return {};
    }

    builder->Initialize(vertexInputDeclaration, submeshes.size());

    for (const auto& sm : submeshes) {
        IMeshBuilder::Submesh builderSubmesh;
        ConfigureSubmesh(sm, builderSubmesh);
        builder->AddSubmesh(builderSubmesh);
    }

    builder->Allocate();

    const array_view<const uint8_t> binView(binData.data(), binData.size());
    for (size_t i = 0; i < submeshes.size(); ++i) {
        FillSubmeshData(*builder, i, submeshes[i], binView, attrs);
    }
    return builder->CreateMesh(*scene->GetEcsContext().GetNativeEcs());
}

void AssignSubmeshMaterial(const CORE_NS::ResourceId& matId, size_t i, IResourceManager& rm, ISubMesh& submesh,
    const ResourceContextPtr& context)
{
    auto matRes = rm.GetResource({matId, context});
    if (!matRes) {
        matRes = rm.GetResource(CORE_NS::ResourceIdContext{matId});
    }
    if (auto mat = interface_pointer_cast<IMaterial>(matRes)) {
        META_NS::SetValue(submesh.Material(), mat);
    } else {
        CORE_LOG_W("Submesh %zu material '%s' did not resolve to an IMaterial", i, matId.ToString().c_str());
    }
}

void AssignSubmeshMaterials(const vector<SubmeshDesc>& submeshes, IMesh& mesh, const ResourceContextPtr& context)
{
    auto rcontext = interface_cast<IResourceContext>(context);
    auto scene = rcontext ? rcontext->GetScene() : interface_pointer_cast<IScene>(context);
    auto rm = SCENE_NS::GetResourceManager(scene);
    if (!rm) {
        return;
    }
    auto liveSubmeshes = mesh.SubMeshes()->GetValue();
    if (submeshes.size() != liveSubmeshes.size()) {
        CORE_LOG_W("Mesh submesh count (%zu) differs from descriptor binding count (%zu)",
            liveSubmeshes.size(),
            submeshes.size());
    }
    const size_t count = submeshes.size() < liveSubmeshes.size() ? submeshes.size() : liveSubmeshes.size();
    for (size_t i = 0; i < count; ++i) {
        if (submeshes[i].materialId.IsValid()) {
            AssignSubmeshMaterial(submeshes[i].materialId, i, *rm, *liveSubmeshes[i], context);
        }
    }
}

}  // namespace

void MeshTemplate::SetDescriptor(vector<SubmeshDesc> submeshes, vector<VertexAttribute> vertexAttributes)
{
    submeshes_ = move(submeshes);
    vertexAttributes_ = move(vertexAttributes);
}

void MeshTemplate::SetBinaryData(vector<uint8_t> data)
{
    binData_ = move(data);
}

bool MeshTemplate::ValidateResourceType(const CORE_NS::IResource& res) const
{
    return interface_cast<IMesh>(&res) != nullptr;
}

// MeshTemplate has no editable meta-property surface beyond Name; the structural
// data lives in submeshes_/vertexAttributes_/binData_ which are not properties.
// ApplyTo therefore does nothing — ApplyOptions drives the actual GPU build.
bool MeshTemplate::ApplyTo(META_NS::IObject& target) const
{
    return true;
}

namespace {

struct DescriptorSource {
    const vector<SubmeshDesc>* submeshes;
    const vector<VertexAttribute>* attrs;
    const vector<uint8_t>* binData;
};

// Pick the descriptor source: prefer this template's own data, fall back to
// a base template (resolved via derivedFrom). Avoids reading from `*this`
// when the entry is just an alias for a shared mesh template.
DescriptorSource PickDescriptorSource(const MeshTemplate& self, const CORE_NS::IResource::ConstPtr& base)
{
    DescriptorSource src{&self.GetSubmeshes(), &self.GetVertexAttributes(), &self.GetBinaryData()};
    auto baseTmpl = interface_cast<IMeshTemplate>(base);
    if (!baseTmpl) {
        return src;
    }
    if (src.submeshes->empty()) {
        src.submeshes = &baseTmpl->GetSubmeshes();
    }
    if (src.attrs->empty()) {
        src.attrs = &baseTmpl->GetVertexAttributes();
    }
    if (src.binData->empty()) {
        src.binData = &baseTmpl->GetBinaryData();
    }
    return src;
}

bool WireBuiltEntity(IResource& res, const IInternalScene::Ptr& internalScene, const DescriptorSource& src)
{
    auto entity = internalScene->RunDirectlyOrInTask(
        [&internalScene, &src] { return BuildEntity(internalScene, *src.submeshes, *src.attrs, *src.binData); });
    if (!EntityUtil::IsValid(entity)) {
        CORE_LOG_E("MeshTemplate: IMeshBuilder::CreateMesh failed for %s", res.GetResourceId().ToString().c_str());
        return false;
    }
    auto ecsObj = internalScene->GetEcsContext().GetEcsObject(entity);
    auto acc = interface_cast<IEcsObjectAccess>(&res);
    if (!ecsObj || !acc || !acc->SetEcsObject(ecsObj)) {
        CORE_LOG_E("MeshTemplate: failed to wire ECS object into live mesh %s", res.GetResourceId().ToString().c_str());
        return false;
    }
    return true;
}

}  // namespace

bool MeshTemplate::ApplyOptions(CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context) const
{
    if (!ValidateResourceType(res)) {
        return false;
    }
    auto base = baseResource_.IsValid() ? ResolveBaseResource(context) : nullptr;
    auto src = PickDescriptorSource(*this, base);
    if (src.submeshes->empty() || src.binData->empty()) {
        CORE_LOG_E("MeshTemplate: missing descriptor or binary data for %s", res.GetResourceId().ToString().c_str());
        return false;
    }
    auto rcontext = interface_cast<IResourceContext>(context);
    auto scene = rcontext ? rcontext->GetScene() : interface_pointer_cast<IScene>(context);
    auto internalScene = scene ? scene->GetInternalScene() : nullptr;
    if (!internalScene) {
        CORE_LOG_E("MeshTemplate: no internal scene when applying to %s", res.GetResourceId().ToString().c_str());
        return false;
    }
    if (!WireBuiltEntity(res, internalScene, src)) {
        return false;
    }
    if (auto mesh = interface_cast<IMesh>(&res)) {
        AssignSubmeshMaterials(*src.submeshes, *mesh, context);
    }
    // Stamp the originating template id. When this MeshTemplate is the inline
    // `options` on a 'mesh' entry, GetResourceId() is empty — fall back to the
    // configured baseResource_, which is the meshTemplate that actually carried
    // the descriptor.
    if (auto derived = interface_cast<META_NS::IDerivedFromTemplate>(&res)) {
        auto tid = GetResourceId();
        derived->SetTemplateId(tid.IsValid() ? tid : baseResource_);
    }
    // Mesh building is custom (no generic metadata copy), so the imported name
    // has to be transferred to the live mesh explicitly.
    if (auto name = GetName(); !name.empty()) {
        if (auto named = interface_cast<META_NS::INamed>(&res)) {
            META_NS::SetValue(named->Name(), name);
        }
    }
    return true;
}

SCENE_END_NAMESPACE()
