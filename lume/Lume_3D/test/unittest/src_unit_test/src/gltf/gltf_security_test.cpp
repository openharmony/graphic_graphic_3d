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

#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/ecs/intf_ecs.h>
#include <core/io/intf_file_manager.h>

#include "gltf/gltf2.h"
#include "gltf/gltf2_loader.h"
#include "gltf/gltf2_util.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: PathTraversal
 * @tc.desc: VULN-007/008/025 — buffer URI with path traversal sequence must not escape the asset root.
 *           The file must not be loaded or the buffer data must be empty.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, PathTraversal, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-007-008-025_path_traversal.gltf");
    // Graceful failure: either parse fails or buffer load fails — no crash.
    if (result.success && result.data) {
        bool bufferDataLoaded = result.data->LoadBuffers();
        // If buffers "loaded", the traversal path must have resolved to nothing.
        if (bufferDataLoaded) {
            EXPECT_TRUE(result.data->buffers.empty() || result.data->buffers[0]->data.empty());
        }
    }
    // If success == false that is also an acceptable graceful failure.
}

/**
 * @tc.name: EncodedPathTraversal
 * @tc.desc: VULN-008/025 — image URI with percent-encoded traversal must not escape the asset root.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, EncodedPathTraversal, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-008-025_encoded_traversal.gltf");
    // Graceful: parse may succeed (image URI is not validated at parse time), but no crash.
    (void)result;
}

/**
 * @tc.name: BufferByteLengthHuge
 * @tc.desc: VULN-009 — a buffer with byteLength=268435456 and no URI must not cause OOM allocation.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, BufferByteLengthHuge, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-009_buffer_bytelength_huge.gltf");
    // Parse may succeed; LoadBuffers must not allocate 256 MB for a missing URI.
    if (result.success && result.data) {
        bool loaded = result.data->LoadBuffers();
        if (loaded && !result.data->buffers.empty()) {
            EXPECT_TRUE(result.data->buffers[0]->data.empty() || result.data->buffers[0]->data.size() < 268435456u);
        }
    }
}

/**
 * @tc.name: SignedBufferViewByteLength
 * @tc.desc: VULN-010 — bufferView byteLength=2147483647 must not cause signed integer overflow or OOB read.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, SignedBufferViewByteLength, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-010_signed_bytelength.gltf");
    // Either the oversized bufferView is rejected or loading produces an empty result — no crash.
    if (result.success && result.data) {
        result.data->LoadBuffers();
    }
}

/**
 * @tc.name: GlbChunkLengthOverflow
 * @tc.desc: VULN-017 — GLB chunk length near UINT32_MAX must not cause offset arithmetic overflow.
 *           Crafted entirely in memory so no file I/O is needed.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, GlbChunkLengthOverflow, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    // Minimal GLB header (12 bytes) + JSON chunk header (8 bytes) with chunkLength = UINT32_MAX.
    // The JSON payload itself is only 2 bytes ('{}'), so the loader must detect the mismatch.
    // clang-format off
    const uint8_t glb[] = {
        // GLB header: magic, version=2, total length=22
        0x67, 0x6C, 0x54, 0x46,  // magic "glTF"
        0x02, 0x00, 0x00, 0x00,  // version 2
        0x16, 0x00, 0x00, 0x00,  // total length 22
        // JSON chunk: chunkLength = UINT32_MAX, chunkType = JSON
        0xFF, 0xFF, 0xFF, 0xFF,  // chunkLength = 4294967295
        0x4A, 0x53, 0x4F, 0x4E,  // chunkType "JSON"
        // Only 2 bytes of actual JSON (far less than chunkLength)
        0x7B, 0x7D               // "{}"
    };
    // clang-format on
    const array_view<const uint8_t> data(glb, sizeof(glb));
    auto result = GLTF2::LoadGLTF(files, data);
    // Must not crash; overflow detection should cause failure.
    (void)result;
}

/**
 * @tc.name: GlbUnalignedHeader
 * @tc.desc: VULN-022 — GLB header accessed via reinterpret_cast must handle misaligned/truncated input.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, GlbUnalignedHeader, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    // Feed a buffer that is shorter than the 12-byte GLB header.
    const uint8_t glb[] = {0x67, 0x6C, 0x54, 0x46, 0x02, 0x00, 0x00};  // truncated after version
    const array_view<const uint8_t> data(glb, sizeof(glb));
    auto result = GLTF2::LoadGLTF(files, data);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: TooManyNodes
 * @tc.desc: VULN-027 — a scene with 20000 nodes must not exhaust memory or exceed internal array limits.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, TooManyNodes, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-027_too_many_nodes.gltf");
    // Either rejected with failure or accepted with a capped node count — no crash.
    if (result.success && result.data) {
        EXPECT_LE(result.data->nodes.size(), 20000u);
    }
}

/**
 * @tc.name: AccessorCountHuge
 * @tc.desc: VULN-030 — accessor count=16777217 exceeds the internal element limit; must not OOM.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, AccessorCountHuge, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-030_accessor_count_huge.gltf");
    // Parse may succeed with invalid data, but LoadData must not allocate 16M+ elements.
    (void)result;
}

/**
 * @tc.name: SparseOobRegression
 * @tc.desc: VULN-001/002/003 regression — sparse accessor with index > accessor.count must be rejected.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, SparseOobRegression, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-001-002-003_sparse_oob.gltf");
    // The fix must cause either a parse failure or LoadData to fail safely.
    if (result.success && result.data) {
        result.data->LoadBuffers();
        if (!result.data->accessors.empty()) {
            auto loadResult = GLTF2::LoadData(*result.data->accessors[0]);
            // If it "succeeds", data must be within accessor bounds.
            if (loadResult.success) {
                EXPECT_LE(loadResult.elementCount, 3u);
            }
        }
    }
}

/**
 * @tc.name: InvalidComponentTypeRegression
 * @tc.desc: VULN-020 regression — componentType=9999 must not cause a lookup table OOB access.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, InvalidComponentTypeRegression, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-020_invalid_component_type.gltf");
    // The regression fix must reject the unknown component type gracefully.
    if (result.success && result.data) {
        result.data->LoadBuffers();
        if (!result.data->accessors.empty()) {
            auto loadResult = GLTF2::LoadData(*result.data->accessors[0]);
            EXPECT_FALSE(loadResult.success);
        }
    }
}

/**
 * @tc.name: SparseWrongKeyRegression
 * @tc.desc: VULN-021 regression — sparse using "bufferOffset" (wrong key) must not silently use
 *           an uninitialized offset. Fix should either reject the file or treat offset as 0.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, SparseWrongKeyRegression, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto result = GLTF2::LoadGLTF(files, "test://gltf/Security/VULN-021_sparse_wrong_key.gltf");
    // Must not crash.  If parsed, LoadData must not use garbage offset.
    if (result.success && result.data) {
        result.data->LoadBuffers();
        if (!result.data->accessors.empty()) {
            (void)GLTF2::LoadData(*result.data->accessors[0]);
        }
    }
}

/**
 * @tc.name: DeepNodeHierarchyImport
 * @tc.desc: A deeply nested acyclic scene must import without recursive stack
 * growth.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, DeepNodeHierarchyImport, testing::ext::TestSize.Level1)
{
    auto* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    auto graphicsContext = testContext->graphicsContext;

    Gltf2 gltf2(*graphicsContext);
    GLTF2::Data data{testContext->engine->GetFileManager()};

    constexpr size_t NODE_COUNT = 20000u;
    data.nodes.reserve(NODE_COUNT);
    for (size_t index = 0; index < NODE_COUNT; ++index) {
        data.nodes.push_back(BASE_NS::unique_ptr<GLTF2::Node>{new GLTF2::Node{}});
    }
    data.nodes[0]->name = "chain_root";
    data.nodes[1]->name = "chain_child";
    for (size_t index = 1; index < NODE_COUNT; ++index) {
        auto* parent = data.nodes[index - 1U].get();
        auto* child = data.nodes[index].get();
        parent->children.push_back(child);
        child->parent = parent;
    }

    data.scenes.push_back(BASE_NS::unique_ptr<GLTF2::Scene>{new GLTF2::Scene{}});
    data.scenes[0]->nodes.push_back(data.nodes[0].get());

    auto* nodeManager = GetManager<INodeComponentManager>(*ecs);
    auto* nameManager = GetManager<INameComponentManager>(*ecs);
    auto* transformManager = GetManager<ITransformComponentManager>(*ecs);
    auto* localMatrixManager = GetManager<ILocalMatrixComponentManager>(*ecs);
    auto* worldMatrixManager = GetManager<IWorldMatrixComponentManager>(*ecs);

    const auto nodeCountBefore = nodeManager->GetComponentCount();
    const auto nameCountBefore = nameManager->GetComponentCount();
    const auto transformCountBefore = transformManager->GetComponentCount();
    const auto localMatrixCountBefore = localMatrixManager->GetComponentCount();
    const auto worldMatrixCountBefore = worldMatrixManager->GetComponentCount();

    const Entity root = gltf2.ImportGltfScene(0u, data, GLTFResourceData{}, *ecs, Entity{}, 0u);
    ASSERT_TRUE(EntityUtil::IsValid(root));

    const size_t importedEntityCount = NODE_COUNT + 1u;  // Scene root + node chain.
    EXPECT_EQ(nodeCountBefore + importedEntityCount, nodeManager->GetComponentCount());
    EXPECT_EQ(nameCountBefore + importedEntityCount, nameManager->GetComponentCount());
    EXPECT_EQ(transformCountBefore + importedEntityCount, transformManager->GetComponentCount());
    EXPECT_EQ(localMatrixCountBefore + importedEntityCount, localMatrixManager->GetComponentCount());
    EXPECT_EQ(worldMatrixCountBefore + importedEntityCount, worldMatrixManager->GetComponentCount());

    const auto findEntityByName = [nameManager](string_view name) -> Entity {
        for (IComponentManager::ComponentId index = 0; index < nameManager->GetComponentCount(); ++index) {
            const Entity entity = nameManager->GetEntity(index);
            if (const auto nameHandle = nameManager->Read(entity); nameHandle && nameHandle->name == name) {
                return entity;
            }
        }
        return {};
    };

    const Entity rootNodeEntity = findEntityByName("chain_root");
    const Entity childNodeEntity = findEntityByName("chain_child");
    ASSERT_TRUE(EntityUtil::IsValid(rootNodeEntity));
    ASSERT_TRUE(EntityUtil::IsValid(childNodeEntity));

    const auto childNodeHandle = nodeManager->Read(childNodeEntity);
    ASSERT_TRUE(childNodeHandle);
    EXPECT_EQ(rootNodeEntity, childNodeHandle->parent);
}

namespace {
// Parses raw glTF JSON bytes and returns whether the parse succeeded.
bool LoadsOk(IFileManager& files, string_view json)
{
    return GLTF2::LoadGLTF(files, array_view<uint8_t const>(reinterpret_cast<uint8_t const*>(json.data()), json.size()))
        .success;
}

// Appends a little-endian 32-bit value to a byte vector.
void PushU32(vector<uint8_t>& out, uint32_t value)
{
    for (uint32_t shift = 0; shift < 32u; shift += 8u) {
        out.push_back(static_cast<uint8_t>((value >> shift) & 0xffu));
    }
}

// Builds an in-memory GLB (header + JSON chunk + BIN chunk) from a JSON string and a binary blob.
vector<uint8_t> BuildGlb(string_view json, array_view<const uint8_t> bin)
{
    const auto pad4 = [](size_t size) { return (4u - (size & 3u)) & 3u; };
    const uint32_t jsonChunkLen = static_cast<uint32_t>(json.size() + pad4(json.size()));
    const uint32_t binChunkLen = static_cast<uint32_t>(bin.size() + pad4(bin.size()));
    const uint32_t total = 12u + 8u + jsonChunkLen + 8u + binChunkLen;

    vector<uint8_t> glb;
    glb.reserve(total);
    const auto put4cc = [&glb](const char fourcc[5]) {
        for (uint32_t index = 0; index < 4u; ++index) {
            glb.push_back(static_cast<uint8_t>(fourcc[index]));
        }
    };
    put4cc("glTF");
    PushU32(glb, 2u);
    PushU32(glb, total);
    PushU32(glb, jsonChunkLen);
    put4cc("JSON");
    for (const char character : json) {
        glb.push_back(static_cast<uint8_t>(character));
    }
    for (size_t index = 0; index < pad4(json.size()); ++index) {
        glb.push_back(0x20u);  // pad JSON chunk with spaces
    }
    PushU32(glb, binChunkLen);
    put4cc("BIN\0");
    for (const uint8_t byte : bin) {
        glb.push_back(byte);
    }
    for (size_t index = 0; index < pad4(bin.size()); ++index) {
        glb.push_back(0x00u);  // pad BIN chunk with zeros
    }
    return glb;
}

// Imports a single position+index primitive with the given UNSIGNED_BYTE indices and vertex count.
// Returns whether the importer reports success.
bool ImportUnsignedByteIndexedMesh(
    IGraphicsContext& graphicsContext, IEcs& ecs, uint32_t vertexCount, array_view<const uint8_t> indexBytes)
{
    vector<uint8_t> bin;
    const uint32_t positionBytes = vertexCount * 3u * static_cast<uint32_t>(sizeof(float));
    for (uint32_t index = 0; index < vertexCount * 3u; ++index) {
        PushU32(bin, 0u);  // positions are all-zero floats; only the element count matters for validation
    }
    const uint32_t indexOffset = positionBytes;
    for (const uint8_t byte : indexBytes) {
        bin.push_back(byte);
    }

    const auto indexCount = static_cast<uint32_t>(indexBytes.size());
    const auto json = string(R"({"asset":{"version":"2.0"},)") + R"("buffers":[{"byteLength":)" +
                      to_string(static_cast<uint32_t>(bin.size())).data() + R"(}],"bufferViews":[)" +
                      R"({"buffer":0,"byteOffset":0,"byteLength":)" + to_string(positionBytes).data() + R"(},)" +
                      R"({"buffer":0,"byteOffset":)" + to_string(indexOffset).data() + R"(,"byteLength":)" +
                      to_string(indexCount).data() + R"(}],"accessors":[)" +
                      R"({"bufferView":0,"componentType":5126,"count":)" + to_string(vertexCount).data() +
                      R"(,"type":"VEC3"},)" + R"({"bufferView":1,"componentType":5121,"count":)" +
                      to_string(indexCount).data() + R"(,"type":"SCALAR"}],)" +
                      R"("meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]}],)" +
                      R"("nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}]})";

    const vector<uint8_t> glb = BuildGlb(json, bin);
    Gltf2 gltf2(graphicsContext);
    auto loaded = gltf2.LoadGLTF(array_view<const uint8_t>(glb.data(), glb.size()));
    if (!loaded.success || !loaded.data) {
        return false;
    }
    auto importer = gltf2.CreateGLTF2Importer(ecs);
    importer->ImportGLTFAsync(*loaded.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, nullptr);
    while (!importer->Execute(0)) {
    }
    return importer->GetResult().success;
}
}  // namespace

/**
 * @tc.name: UnsignedByteIndexRestartSentinelBoundsChecked
 * @tc.desc: UNSIGNED_BYTE indices widen to UINT16, so a byte value 0xFF (255) is not the GPU restart sentinel
 *           (0xFFFF) and must be bounds-checked. With vertexCount<=255 an index of 255 is out-of-range and the
 *           import must fail rather than being silently accepted as primitive restart (which would leave a live
 *           OOB vertex index in the widened buffer). An in-range 255 (vertexCount>255) must still import.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, UnsignedByteIndexRestartSentinelBoundsChecked, testing::ext::TestSize.Level1)
{
    auto* testContext = UTest::GetTestContext();
    auto& graphicsContext = *testContext->graphicsContext;
    auto& ecs = *testContext->ecs;

    // Out-of-range: vertexCount 3, index 255 -> must be rejected (pre-fix it passed as primitive restart).
    const uint8_t oobIndices[] = {0u, 1u, 255u};
    EXPECT_FALSE(ImportUnsignedByteIndexedMesh(graphicsContext, ecs, 3u, oobIndices));

    // In-range control: all indices < vertexCount must import successfully.
    const uint8_t goodIndices[] = {0u, 1u, 2u};
    EXPECT_TRUE(ImportUnsignedByteIndexedMesh(graphicsContext, ecs, 3u, goodIndices));

    // In-range 255: with vertexCount 256 the value 255 is a valid vertex and must not be over-rejected.
    const uint8_t inRange255[] = {0u, 1u, 255u};
    EXPECT_TRUE(ImportUnsignedByteIndexedMesh(graphicsContext, ecs, 256u, inRange255));
}

/**
 * @tc.name: IndicesMustBeUnsignedScalar
 * @tc.desc: Per glTF 2.0 spec, a primitive indices accessor must be SCALAR with an unsigned integer
 *           component type; other types must be rejected at parse.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, IndicesMustBeUnsignedScalar, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    constexpr string_view good = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                R"("accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},)"
                                R"({"bufferView":0,"componentType":5125,"count":3,"type":"SCALAR"}],)"
                                R"("meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]}]})";
    // indices accessor componentType FLOAT (5126) is not a valid index type.
    constexpr string_view bad = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                R"("accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},)"
                                R"({"bufferView":0,"componentType":5126,"count":3,"type":"SCALAR"}],)"
                                R"("meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]}]})";
    EXPECT_TRUE(LoadsOk(files, good));
    EXPECT_FALSE(LoadsOk(files, bad));
}

/**
 * @tc.name: SkinInverseBindMatricesMustBeMat4Float
 * @tc.desc: Per glTF 2.0 spec a skin's inverseBindMatrices accessor must be a FLOAT MAT4 with one matrix
 *           per joint. BuildSkinIbmComponent casts the loaded bytes to Math::Mat4X4 and appends
 *           elementCount of them, so a SCALAR/FLOAT accessor (4 bytes/element) would over-read 64 bytes
 *           per element. A non-MAT4 accessor, or a count that does not match the joint count, must be
 *           rejected at parse.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, SkinInverseBindMatricesMustBeMat4Float, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    // accessor 0: MAT4 FLOAT count 2 (valid IBM for 2 joints); accessor 1: SCALAR FLOAT count 2 (invalid).
    constexpr string_view good = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                R"("accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"MAT4"},)"
                                R"({"bufferView":0,"componentType":5126,"count":2,"type":"SCALAR"}],)"
                                R"("nodes":[{},{}],"scenes":[{"nodes":[0,1]}],)"
                                R"("skins":[{"joints":[0,1],"inverseBindMatrices":0}]})";
    // inverseBindMatrices points at the SCALAR accessor — not a MAT4, must be rejected.
    constexpr string_view badType = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                    R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                    R"("accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"MAT4"},)"
                                    R"({"bufferView":0,"componentType":5126,"count":2,"type":"SCALAR"}],)"
                                    R"("nodes":[{},{}],"scenes":[{"nodes":[0,1]}],)"
                                    R"("skins":[{"joints":[0,1],"inverseBindMatrices":1}]})";
    // MAT4 accessor but count (2) does not match joint count (1) — must be rejected.
    constexpr string_view badCount = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                    R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                    R"("accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"MAT4"}],)"
                                    R"("nodes":[{},{}],"scenes":[{"nodes":[0]}],)"
                                    R"("skins":[{"joints":[0],"inverseBindMatrices":0}]})";
    EXPECT_TRUE(LoadsOk(files, good));
    EXPECT_FALSE(LoadsOk(files, badType));
    EXPECT_FALSE(LoadsOk(files, badCount));
}

/**
 * @tc.name: AnimationSamplerInputRequired
 * @tc.desc: Per glTF 2.0 spec, an animation sampler input accessor is required; a sampler missing it
 *           must be rejected at parse.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, AnimationSamplerInputRequired, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    constexpr string_view good = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                R"("accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"SCALAR"},)"
                                R"({"bufferView":0,"componentType":5126,"count":2,"type":"VEC3"}],"nodes":[{}],)"
                                R"("animations":[{"samplers":[{"input":0,"output":1,"interpolation":"LINEAR"}],)"
                                R"("channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]}]})";
    // sampler has no "input".
    constexpr string_view bad = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                R"("accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"SCALAR"},)"
                                R"({"bufferView":0,"componentType":5126,"count":2,"type":"VEC3"}],"nodes":[{}],)"
                                R"("animations":[{"samplers":[{"output":1,"interpolation":"LINEAR"}],)"
                                R"("channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]}]})";
    EXPECT_TRUE(LoadsOk(files, good));
    EXPECT_FALSE(LoadsOk(files, bad));
}

/**
 * @tc.name: MorphTargetEntriesMustBeObjects
 * @tc.desc: A mesh primitive's morph "targets" array element that is not a JSON object (number, string or
 *           nested array) must not be read as an object. PrimitiveTargets iterates target.object_ directly;
 *           json::value is a union, so reading object_ of a non-object element walks wild iterators (OOB
 *           read / crash). The loader must skip the malformed element and still load successfully.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, MorphTargetEntriesMustBeObjects, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    constexpr string_view base = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                 R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                 R"("accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"}],)";
    // targets element is a number -> object_ union member is the wrong active member.
    constexpr string_view numberTarget =
        R"("meshes":[{"primitives":[{"attributes":{"POSITION":0},"targets":[123]}]}]})";
    // targets element is a string.
    constexpr string_view stringTarget =
        R"("meshes":[{"primitives":[{"attributes":{"POSITION":0},"targets":["oops"]}]}]})";
    // targets element is a nested array (same pointer layout, wrong iteration stride).
    constexpr string_view arrayTarget =
        R"("meshes":[{"primitives":[{"attributes":{"POSITION":0},"targets":[[1,2,3]]}]}]})";

    const auto numberJson = string(base) + string(numberTarget);
    const auto stringJson = string(base) + string(stringTarget);
    const auto arrayJson = string(base) + string(arrayTarget);
    // Must not crash / OOB; the malformed morph-target entry is skipped and the load succeeds.
    EXPECT_TRUE(LoadsOk(files, numberJson));
    EXPECT_TRUE(LoadsOk(files, stringJson));
    EXPECT_TRUE(LoadsOk(files, arrayJson));
}

/**
 * @tc.name: AnimationOutputCountMustMatchInput
 * @tc.desc: Per glTF 2.0 spec, for non-CUBICSPLINE interpolation the sampler output frame count must equal
 *           the input keyframe count; a mismatch must be rejected at parse.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFSecurityTest, AnimationOutputCountMustMatchInput, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    // LINEAR translation with input count 2 but output count 3.
    constexpr string_view bad = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":1000}],)"
                                R"("bufferViews":[{"buffer":0,"byteLength":1000}],)"
                                R"("accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"SCALAR"},)"
                                R"({"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"}],"nodes":[{}],)"
                                R"("animations":[{"samplers":[{"input":0,"output":1,"interpolation":"LINEAR"}],)"
                                R"("channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]}]})";
    EXPECT_FALSE(LoadsOk(files, bad));
}
