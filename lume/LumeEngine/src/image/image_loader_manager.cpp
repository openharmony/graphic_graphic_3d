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

#include "image_loader_manager.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::string_view;
using BASE_NS::unique_ptr;

void ImageLoaderManager::RegisterImageLoader(IImageLoader::Ptr imageLoader)
{
    if (!imageLoader) {
        CORE_LOG_D("imageLoader is null, Not adding.");
        return;
    }

    // NOTE: We just add the registered unique pointers to a vector. The vector is not really used for anything else.
    // And the loaders cannot be currently unregistered.
    imageLoaders_.push_back(std::move(imageLoader));
}

ImageLoaderManager::LoadResult ImageLoaderManager::LoadImage(const string_view uri, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadImage()", uri);

    // Load 12 bytes (maximum header size of currently implemented file types)
    IFile::Ptr file = fileManager_.OpenFile(uri);
    if (!file) {
        return ResultFailure("Can not open image.");
    }

    return LoadImage(*file, loadFlags);
}

ImageLoaderManager::LoadResult ImageLoaderManager::LoadImage(IFile& file, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadImage()", "file");

    const uint64_t byteLength = 12u;

    // Read header of the file to a buffer.
    unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(byteLength));
    const uint64_t read = file.Read(buffer.get(), byteLength);
    if (read != byteLength) {
        return ResultFailure("Can not read file header.");
    }
    file.Seek(0);

    for (auto& loader : imageLoaders_) {
        if (loader->CanLoad(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)))) {
            return loader->Load(file, loadFlags);
        }
    }
    return ResultFailure("Image loader not found for this format.");
}

ImageLoaderManager::LoadResult ImageLoaderManager::LoadImage(
    array_view<const uint8_t> imageFileBytes, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadImage(bytes)", "memory");

    for (auto& loader : imageLoaders_) {
        if (loader->CanLoad(imageFileBytes)) {
            return loader->Load(imageFileBytes, loadFlags);
        }
    }

    return ResultFailure("Image loader not found for this format.");
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::LoadAnimatedImage(const string_view uri, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadAnimatedImage()", uri);

    // Load 12 bytes (maximum header size of currently implemented file types)
    IFile::Ptr file = fileManager_.OpenFile(uri);
    if (!file) {
        return ResultFailureAnimated("Can not open image.");
    }

    return LoadAnimatedImage(*file, loadFlags);
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::LoadAnimatedImage(IFile& file, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadAnimatedImage()", "file");

    const uint64_t byteLength = 12u;

    // Read header of the file to a buffer.
    unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(byteLength));
    const uint64_t read = file.Read(buffer.get(), byteLength);
    if (read != byteLength) {
        return ResultFailureAnimated("Can not read file header.");
    }
    file.Seek(0);

    for (auto& loader : imageLoaders_) {
        if (loader->CanLoad(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)))) {
            return loader->LoadAnimatedImage(file, loadFlags);
        }
    }
    return ResultFailureAnimated("Image loader not found for this format.");
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::LoadAnimatedImage(
    array_view<const uint8_t> imageFileBytes, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadAnimatedImage(bytes)", "memory");

    for (auto& loader : imageLoaders_) {
        if (loader->CanLoad(imageFileBytes)) {
            return loader->LoadAnimatedImage(imageFileBytes, loadFlags);
        }
    }

    return ResultFailureAnimated("Image loader not found for this format.");
}

ImageLoaderManager::LoadResult ImageLoaderManager::ResultFailure(const string_view error)
{
    LoadResult result {
        false,  // if success
        "",     // array error[128];
        nullptr // the image;
    };

    // Copy the error string
    const auto count = std::min(error.size(), sizeof(result.error) - 1);
    error.copy(result.error, count);
    result.error[count] = '\0';

    return result;
}

ImageLoaderManager::LoadResult ImageLoaderManager::ResultSuccess(IImageContainer::Ptr image)
{
    return LoadResult {
        true,       // if success
        "",         // array error[128];
        move(image) // the image;
    };
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::ResultFailureAnimated(const string_view error)
{
    LoadAnimatedResult result {
        false,  // if success
        "",     // array error[128];
        nullptr // the image;
    };

    // Copy the error string
    const auto count = std::min(error.size(), sizeof(result.error) - 1);
    error.copy(result.error, count);
    result.error[count] = '\0';

    return result;
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::ResultSuccessAnimated(IAnimatedImage::Ptr image)
{
    return LoadAnimatedResult {
        true,       // if success
        "",         // array error[128];
        move(image) // the image;
    };
}
CORE_END_NAMESPACE()
