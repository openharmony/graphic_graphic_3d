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

#include "image_loader_manager.h"

#include <algorithm>
#include <cstring>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/image/intf_animated_image.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::string_view;
using BASE_NS::unique_ptr;
using BASE_NS::vector;

ImageLoaderManager::ImageLoaderManager(IFileManager& fileManager) : fileManager_(fileManager)
{
    for (const auto* typeInfo : GetPluginRegister().GetTypeInfos(IImageLoaderManager::ImageLoaderTypeInfo::UID)) {
        if (typeInfo && (typeInfo->typeUid == IImageLoaderManager::ImageLoaderTypeInfo::UID)) {
            const auto* imageLoaderInfo = static_cast<const IImageLoaderManager::ImageLoaderTypeInfo*>(typeInfo);
            if (imageLoaderInfo->createLoader &&
                std::none_of(imageLoaders_.cbegin(), imageLoaders_.cend(),
                    [&uid = imageLoaderInfo->uid](const RegisteredImageLoader& loader) { return loader.uid == uid; })) {
                imageLoaders_.push_back(
                    { imageLoaderInfo->uid, imageLoaderInfo->createLoader(imageLoaderInfo->token) });
            }
        }
    }
    GetPluginRegister().AddListener(*this);
}
ImageLoaderManager::~ImageLoaderManager()
{
    GetPluginRegister().RemoveListener(*this);
}
void ImageLoaderManager::RegisterImageLoader(IImageLoader::Ptr imageLoader)
{
    if (!imageLoader) {
        CORE_LOG_D("imageLoader is null, Not adding.");
        return;
    }

    // NOTE: We just add the registered unique pointers to a vector. The vector is not really used for anything else.
    // And the loaders cannot be currently unregistered.
    imageLoaders_.push_back({ {}, move(imageLoader) });
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
        if (loader.instance &&
            loader.instance->CanLoad(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)))) {
            return loader.instance->Load(file, loadFlags);
        }
    }
    return ResultFailure("Image loader not found for this format.");
}

ImageLoaderManager::LoadResult ImageLoaderManager::LoadImage(
    array_view<const uint8_t> imageFileBytes, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadImage(bytes)", "memory");
    if (imageFileBytes.empty()) {
        return ResultFailure("Image loader can't load from empty buffer.");
    }
    for (auto it=imageLoaders_.cbegin(); it!=imageLoaders_.cend(); ++it) { // this works fine!
        const auto& loader = *it;
        if (loader.instance) {
            if (loader.instance->CanLoad(imageFileBytes)) {
                return loader.instance->Load(imageFileBytes, loadFlags);
            }
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
        if (loader.instance->CanLoad(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)))) {
            return loader.instance->LoadAnimatedImage(file, loadFlags);
        }
    }
    return ResultFailureAnimated("Image loader not found for this format.");
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::LoadAnimatedImage(
    array_view<const uint8_t> imageFileBytes, uint32_t loadFlags)
{
    CORE_CPU_PERF_SCOPE("Image", "loadAnimatedImage(bytes)", "memory");

    for (auto& loader : imageLoaders_) {
        if (loader.instance->CanLoad(imageFileBytes)) {
            return loader.instance->LoadAnimatedImage(imageFileBytes, loadFlags);
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

vector<IImageLoaderManager::ImageType> ImageLoaderManager::GetSupportedTypes() const
{
    vector<IImageLoaderManager::ImageType> allTypes;
    for (const auto& loader : imageLoaders_) {
        const auto types = loader.instance->GetSupportedTypes();
        allTypes.append(types.cbegin(), types.cend());
    }
    return allTypes;
}
void ImageLoaderManager::OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos)
{
    for (const auto* typeInfo : typeInfos) {
        if (typeInfo && (typeInfo->typeUid == IImageLoaderManager::ImageLoaderTypeInfo::UID)) {
            const auto* imageLoaderInfo = static_cast<const IImageLoaderManager::ImageLoaderTypeInfo*>(typeInfo);
            if (type == EventType::ADDED) {
                if (imageLoaderInfo->createLoader &&
                    std::none_of(imageLoaders_.cbegin(), imageLoaders_.cend(),
                        [&uid = imageLoaderInfo->uid](
                            const RegisteredImageLoader& loader) { return loader.uid == uid; })) {
                    imageLoaders_.push_back(
                        { imageLoaderInfo->uid, imageLoaderInfo->createLoader(imageLoaderInfo->token) });
                }
            } else if (type == EventType::REMOVED) {
                imageLoaders_.erase(std::remove_if(imageLoaders_.begin(), imageLoaders_.end(),
                    [&uid = imageLoaderInfo->uid](const RegisteredImageLoader& loader) { return loader.uid == uid; }),
                    imageLoaders_.cend());
            }
        }
    }
}
CORE_END_NAMESPACE()
