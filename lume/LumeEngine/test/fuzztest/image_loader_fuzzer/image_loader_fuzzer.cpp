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

#include "image_loader_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <core/image/intf_image_loader_manager.h>

#include "image/loaders/image_loader_ktx.h"
#include "image/loaders/image_loader_astc.h"

using namespace CORE_NS;

namespace {

void FuzzLoader(IImageLoaderManager::IImageLoader& loader, const uint8_t* data, size_t size, uint32_t flags)
{
    BASE_NS::array_view<const uint8_t> bytes(data, size);
    auto result = loader.Load(bytes, flags);
    if (result.success && result.image) {
        // Exercise the loaded image to catch deferred parsing issues
        const auto& desc = result.image->GetImageDesc();
        (void)desc.width;
        (void)desc.height;
        (void)desc.depth;
        (void)desc.format;
        (void)desc.mipCount;
        (void)desc.layerCount;
        (void)result.image->GetData();
        (void)result.image->GetBufferImageCopies();
    }
}

void FuzzKtx(const uint8_t* data, size_t size)
{
    auto loader = CreateImageLoaderKtx(nullptr);
    if (!loader) {
        return;
    }
    FuzzLoader(*loader, data, size, 0);
    FuzzLoader(*loader, data, size, IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
    FuzzLoader(*loader, data, size, IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT);
    FuzzLoader(*loader, data, size, IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
}

void FuzzAstc(const uint8_t* data, size_t size)
{
    auto loader = CreateImageLoaderAstc(nullptr);
    if (!loader) {
        return;
    }
    FuzzLoader(*loader, data, size, 0);
    FuzzLoader(*loader, data, size, IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
    FuzzLoader(*loader, data, size, IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0) {
        return 0;
    }
    FuzzKtx(data, size);
    FuzzAstc(data, size);
    return 0;
}
