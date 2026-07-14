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

#include "image_ktx_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>

#include "image/loaders/image_loader_ktx.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    auto loader = CORE_NS::CreateImageLoaderKtx(nullptr);
    if (!loader) {
        return 0;
    }

    const BASE_NS::array_view<const uint8_t> bytes(data, size);

    (void)loader->Load(bytes, CORE_NS::IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
    (void)loader->Load(bytes, 0U);
    (void)loader->Load(bytes,
        CORE_NS::IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS |
            CORE_NS::IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT);

    return 0;
}
