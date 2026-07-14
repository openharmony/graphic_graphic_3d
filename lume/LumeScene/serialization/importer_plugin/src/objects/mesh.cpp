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

#include "mesh.h"

#include <cstring>
#include <scene/interface/intf_mesh_template.h>
#include <scene/interface/mesh_descriptor.h>
#include <scene/interface/resource/types.h>

#include <core/intf_engine.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>

#include "../import_helpers.h"
#include "flag_tables.h"
#include "format_table.h"
#include "mesh_bin_format.h"
#include "mesh_semantics.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {

using namespace BASE_NS;
using namespace CORE_NS;

IDiagnostics::Ptr ImportBufferRegion(ImportContext& context, SCENE_NS::BufferRegion& out)
{
    ErrorHandler h(context);
    if (auto v = GetOptUInt(context, "offset"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.offset = v.GetValue();
    }
    if (auto v = GetOptUInt(context, "size"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.size = v.GetValue();
    }
    return h;
}

IDiagnostics::Ptr ImportVertexBufferRegion(ImportContext& context, SCENE_NS::VertexBufferRegion& out)
{
    ErrorHandler h(context);
    if (auto err = ImportBufferRegion(context, out); h.Handle(err)) {
        return err;
    }
    if (auto v = GetOptString(context, "semantic"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        auto sem = v.GetValue();
        if (!SCENE_IMP_NS::IsKnownSemantic(sem)) {
            return context.CreateDiagnostics("Unknown vertex-buffer semantic '" + sem + "'");
        }
        out.semantic = BASE_NS::move(sem);
    }
    return h;
}

IDiagnostics::Ptr ImportVertexAttribute(ImportContext& context, SCENE_NS::VertexAttribute& out)
{
    ErrorHandler h(context);
    if (auto v = GetOptString(context, "semantic"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        auto sem = v.GetValue();
        if (!SCENE_IMP_NS::IsKnownSemantic(sem)) {
            return context.CreateDiagnostics(
                "Unknown vertex-attribute semantic '" + sem +
                "' (expected position, normal, tangent, joints, weights, color, uv0, or uv1)");
        }
        out.semantic = BASE_NS::move(sem);
    }
    if (auto v = GetOptString(context, "format"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        if (auto fmt = ParseFormat(v.GetValue())) {
            out.format = *fmt;
        } else {
            return context.CreateDiagnostics("Unknown vertex-attribute format '" + v.GetValue() + "'");
        }
    }
    if (auto v = GetOptUInt(context, "stride"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.stride = static_cast<uint32_t>(v.GetValue());
    }
    return h;
}

IDiagnostics::Ptr ImportSubmeshIndexType(ImportContext& context, SCENE_NS::SubmeshDesc& out)
{
    ErrorHandler h(context);
    if (auto v = GetOptString(context, "indexType"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        if (auto idx = LookupValue(INDEX_TYPE_TABLE, v.GetValue())) {
            out.indexType = *idx;
        } else {
            return context.CreateDiagnostics(
                "Unknown indexType '" + v.GetValue() + "' (expected 'uint16' or 'uint32')");
        }
    }
    if (auto v = GetOptString(context, "topology"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        if (auto t = LookupValue(TOPOLOGY_TABLE, v.GetValue())) {
            out.topology = *t;
        } else {
            return context.CreateDiagnostics("Unknown topology '" + v.GetValue() + "'");
        }
    }
    return h;
}

IDiagnostics::Ptr ImportSubmeshBuffers(ImportContext& context, SCENE_NS::SubmeshDesc& out)
{
    ErrorHandler h(context);
    auto buffersArr = context.GetOptArray("buffers");
    for (auto&& bufJson : buffersArr) {
        auto bufCtx = context.CreateContext(bufJson);
        SCENE_NS::VertexBufferRegion region;
        if (auto err = ImportVertexBufferRegion(bufCtx, region); h.Handle(err)) {
            return err;
        }
        out.buffers.push_back(BASE_NS::move(region));
    }
    auto idxObj = context.GetOptObject("indexBuffer");
    if (!idxObj.empty()) {
        auto idxCtx = context.CreateContext(BASE_NS::move(idxObj));
        if (auto err = ImportBufferRegion(idxCtx, out.indexBuffer); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

IDiagnostics::Ptr ImportSubmeshAabb(ImportContext& context, SCENE_NS::SubmeshDesc& out)
{
    auto aabbObj = context.GetOptObject("aabb");
    if (aabbObj.empty()) {
        return nullptr;
    }
    auto aabbCtx = context.CreateContext(BASE_NS::move(aabbObj));
    ErrorHandler h(aabbCtx);
    if (auto v = GetOptVec3(aabbCtx, "min"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.aabb.min = v.GetValue();
    }
    if (auto v = GetOptVec3(aabbCtx, "max"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.aabb.max = v.GetValue();
    }
    return h;
}

IDiagnostics::Ptr ImportSubmesh(ImportContext& context, SCENE_NS::SubmeshDesc& out)
{
    ErrorHandler h(context);
    if (auto v = GetOptUInt(context, "vertexCount"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.vertexCount = static_cast<uint32_t>(v.GetValue());
    }
    if (auto v = GetOptUInt(context, "indexCount"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.indexCount = static_cast<uint32_t>(v.GetValue());
    }
    if (auto err = ImportSubmeshIndexType(context, out); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportSubmeshBuffers(context, out); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportSubmeshAabb(context, out); h.Handle(err)) {
        return err;
    }
    if (auto v = GetOptResourceId(context, "material"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        out.materialId = v.GetValue();
    }
    auto flagsArr = context.GetOptArray("flags");
    if (auto err = ImportFlagsFromArray(context, flagsArr, SUBMESH_FLAG_TABLE, "flags", out.flags); h.Handle(err)) {
        return err;
    }
    if (auto sort = ImportOptRenderSort(context); h.HandleOptValue(sort)) {
        if (sort.error) {
            return sort.error;
        }
        out.renderSort = sort.GetValue();
    }
    return h;
}

IDiagnostics::Ptr ValidateBinarySize(ImportContext& context, uint64_t size, BASE_NS::string_view uri)
{
    if (size < MeshBin::HEADER_SIZE) {
        return context.CreateDiagnostics("Mesh binary buffer too small for header (" + BASE_NS::to_string(size) +
                                         " bytes, need at least " + BASE_NS::to_string(MeshBin::HEADER_SIZE) +
                                         "): " + uri);
    }
    if (size > MeshBin::MAX_FILE_SIZE) {
        return context.CreateDiagnostics("Mesh binary buffer too large (" + BASE_NS::to_string(size) +
                                         " bytes; limit is " + BASE_NS::to_string(MeshBin::MAX_FILE_SIZE) +
                                         "): " + uri);
    }
    return nullptr;
}

IDiagnostics::Ptr ValidateBinaryHeader(
    ImportContext& context, const BASE_NS::vector<uint8_t>& buf, BASE_NS::string_view uri)
{
    if (memcmp(buf.data() + MeshBin::MAGIC_OFFSET, MeshBin::MAGIC, sizeof(MeshBin::MAGIC)) != 0) {
        return context.CreateDiagnostics("Mesh binary buffer has invalid magic (expected 'LMSH'): " + uri);
    }
    const uint32_t version = MeshBin::ReadU32LE(buf.data() + MeshBin::VERSION_OFFSET);
    if (version > MeshBin::VERSION) {
        return context.CreateDiagnostics("Mesh binary buffer version " + BASE_NS::to_string(version) +
                                         " exceeds reader's supported version " + BASE_NS::to_string(MeshBin::VERSION) +
                                         ": " + uri);
    }
    return nullptr;
}

// Returns nullptr if `[offset, offset+size)` fits within a buffer of `binSize`
// bytes, or a diagnostic naming the offending region otherwise. The bounds
// test is phrased to be safe under uint64 overflow.
IDiagnostics::Ptr CheckRegionFits(
    ImportContext& context, uint64_t offset, uint64_t size, uint64_t binSize, BASE_NS::string_view label)
{
    if (offset > binSize || size > binSize - offset) {
        return context.CreateDiagnostics(
            BASE_NS::string(label) + " region exceeds binary size (offset=" + BASE_NS::to_string(offset) +
            ", size=" + BASE_NS::to_string(size) + ", binary=" + BASE_NS::to_string(binSize) + ")");
    }
    return nullptr;
}

// Walk every region declared by the parsed submeshes and verify it fits
// within `binSize` bytes. Run before allocating / reading the `.bin` so a
// malformed `.mesh` / `.bin` pair fails cheaply.
IDiagnostics::Ptr ValidateRegionsFitBinary(
    ImportContext& context, const BASE_NS::vector<SCENE_NS::SubmeshDesc>& submeshes, uint64_t binSize)
{
    for (size_t i = 0; i < submeshes.size(); ++i) {
        const auto& sm = submeshes[i];
        BASE_NS::string prefix = BASE_NS::string("submeshes[") + BASE_NS::to_string(i) + "]";
        for (size_t b = 0; b < sm.buffers.size(); ++b) {
            const auto& r = sm.buffers[b];
            auto label = prefix + ".buffers[" + BASE_NS::to_string(b) + " semantic=" + r.semantic + "]";
            if (auto err = CheckRegionFits(context, r.offset, r.size, binSize, label)) {
                return err;
            }
        }
        if (sm.indexBuffer.size > 0) {
            auto label = prefix + ".indexBuffer";
            if (auto err = CheckRegionFits(context, sm.indexBuffer.offset, sm.indexBuffer.size, binSize, label)) {
                return err;
            }
        }
    }
    return nullptr;
}

IDiagnostics::Ptr LoadBinaryBuffer(ImportContext& context, BASE_NS::string_view bufferUri,
    const BASE_NS::vector<SCENE_NS::SubmeshDesc>& submeshes, BASE_NS::vector<uint8_t>& out)
{
    auto renderCtx = context.GetRenderContext();
    if (!renderCtx) {
        return context.CreateDiagnostics("No render context available for mesh binary load");
    }
    auto& fileManager = renderCtx->GetRenderer()->GetEngine().GetFileManager();
    auto binUri = ResolveFilename(context, bufferUri);
    auto file = fileManager.OpenFile(binUri);
    if (!file) {
        return context.CreateDiagnostics("Failed to open binary mesh buffer: " + binUri);
    }
    const uint64_t size = file->GetLength();
    if (auto err = ValidateBinarySize(context, size, binUri)) {
        return err;
    }
    // Check every JSON-declared region against the file size *before* allocating
    // and reading the buffer. A malformed `.mesh` pointing at the wrong `.bin`
    // (or carrying inflated offsets) bails out cheaply.
    if (auto err = ValidateRegionsFitBinary(context, submeshes, size)) {
        return err;
    }
    out.resize(static_cast<size_t>(size));
    if (file->Read(out.data(), size) != size) {
        return context.CreateDiagnostics("Short read from binary mesh buffer: " + binUri);
    }
    return ValidateBinaryHeader(context, out, binUri);
}

}  // namespace

IDiagnostics::Ptr ImportVertexAttributes(ImportContext& context, BASE_NS::vector<SCENE_NS::VertexAttribute>& out)
{
    ErrorHandler h(context);
    auto attrsArr = context.GetOptArray("vertexAttributes");
    out.reserve(attrsArr.size());
    for (auto&& attrJson : attrsArr) {
        auto attrCtx = context.CreateContext(attrJson);
        SCENE_NS::VertexAttribute attr;
        if (auto err = ImportVertexAttribute(attrCtx, attr); h.Handle(err)) {
            return err;
        }
        out.push_back(BASE_NS::move(attr));
    }
    return h;
}

IDiagnostics::Ptr ImportSubmeshes(ImportContext& context, BASE_NS::vector<SCENE_NS::SubmeshDesc>& out)
{
    ErrorHandler h(context);
    auto subsArr = context.GetOptArray("submeshes");
    out.reserve(subsArr.size());
    for (auto&& subJson : subsArr) {
        auto subCtx = context.CreateContext(subJson);
        SCENE_NS::SubmeshDesc sm;
        if (auto err = ImportSubmesh(subCtx, sm); h.Handle(err)) {
            return err;
        }
        out.push_back(BASE_NS::move(sm));
    }
    return h;
}

// bufferUri is required when this entry is the source of mesh data, but
// index entries that only carry `derivedFrom` (chaining off a base
// meshTemplate) legitimately omit it — the base supplies the binary data.
// Detect "this entry has its own data" via the presence of bufferUri and
// only require the binary load when it's set; ImportSubType wires up
// derivedFrom centrally afterwards, so we can't peek at it here.
IDiagnostics::Ptr ImportBinaryBuffer(ImportContext& context, const BASE_NS::vector<SCENE_NS::SubmeshDesc>& submeshes,
    bool hasDescriptorData, BASE_NS::vector<uint8_t>& out)
{
    auto bufferUri = context.GetOptString("bufferUri");
    if (!bufferUri.empty()) {
        return LoadBinaryBuffer(context, bufferUri, submeshes, out);
    }
    if (hasDescriptorData) {
        return context.CreateDiagnostics(
            "Mesh has descriptor data but no 'bufferUri' — bufferUri is required when submeshes or "
            "vertexAttributes are present");
    }
    return nullptr;
}

ImportResult ImportMesh::Import(ImportContext& context)
{
    auto trace = context.Trace("Importing mesh");
    // The same handler serves both "mesh" (live) and "meshTemplate" (data only)
    // index entries — the dispatcher in index.cpp/importer.cpp routes both here.
    // The resulting MeshTemplate is consumed as IResourceOptions for live mesh
    // loads or as the loaded resource itself for template loads.
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::MeshTemplate);
    auto meshTemplate = interface_pointer_cast<SCENE_NS::IMeshTemplate>(obj);
    if (!meshTemplate) {
        return ImportResult{context.CreateDiagnostics("Failed to create MeshTemplate")};
    }
    ErrorHandler h(context);
    if (auto meta = interface_cast<META_NS::IMetadata>(obj)) {
        if (auto err = ImportResourceName(context, *meta); h.Handle(err)) {
            return ImportResult{err};
        }
    }
    BASE_NS::vector<SCENE_NS::VertexAttribute> attrs;
    if (auto err = ImportVertexAttributes(context, attrs); h.Handle(err)) {
        return ImportResult{err};
    }
    BASE_NS::vector<SCENE_NS::SubmeshDesc> submeshes;
    if (auto err = ImportSubmeshes(context, submeshes); h.Handle(err)) {
        return ImportResult{err};
    }
    BASE_NS::vector<uint8_t> binData;
    if (auto err = ImportBinaryBuffer(context, submeshes, !submeshes.empty() || !attrs.empty(), binData);
        h.Handle(err)) {
        return ImportResult{err};
    }
    meshTemplate->SetDescriptor(BASE_NS::move(submeshes), BASE_NS::move(attrs));
    meshTemplate->SetBinaryData(BASE_NS::move(binData));
    ImportResult result{interface_pointer_cast<META_NS::IObject>(obj)};
    result.error = h;
    return result;
}

SCENE_IMP_END_NAMESPACE()
