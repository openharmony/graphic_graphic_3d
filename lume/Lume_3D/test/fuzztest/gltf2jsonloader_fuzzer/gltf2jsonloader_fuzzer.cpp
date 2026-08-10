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

/* Fuzz the glTF/GLB loader with raw input bytes fed straight to GLTF2::LoadGLTF. */

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>

#include "gltf_fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    // Whole input is the document: a leading glTF magic selects GLB, otherwise it parses as
    // text glTF/JSON. No selector byte, so every corpus seed exercises the parser as authored.
    GltfFuzz::RunLoadGltf(BASE_NS::array_view<uint8_t const>(data, size));
    return 0;
}
