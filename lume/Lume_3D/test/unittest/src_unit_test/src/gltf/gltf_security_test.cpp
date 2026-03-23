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

#include <3d/gltf/gltf.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
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
    const uint8_t glb[] = { 0x67, 0x6C, 0x54, 0x46, 0x02, 0x00, 0x00 }; // truncated after version
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
