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

#ifndef CORE_IMAGE_IMAGE_LOADER_MANAGER_H
#define CORE_IMAGE_IMAGE_LOADER_MANAGER_H

#include <cstdint>

#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/image/intf_animated_image.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin_register.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IFile;
class IFileManager;

class ImageLoaderManager final : public IImageLoaderManager, private IPluginRegister::ITypeInfoListener {
public:
    explicit ImageLoaderManager(IFileManager& fileManager);
    ~ImageLoaderManager() override;

    void RegisterImageLoader(IImageLoader::Ptr imageLoader) override;

    LoadResult LoadImage(const BASE_NS::string_view uri, uint32_t loadFlags) override;
    LoadResult LoadImage(IFile& file, uint32_t loadFlags) override;
    LoadResult LoadImage(BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) override;

    LoadAnimatedResult LoadAnimatedImage(const BASE_NS::string_view uri, uint32_t loadFlags) override;
    LoadAnimatedResult LoadAnimatedImage(IFile& file, uint32_t loadFlags) override;
    LoadAnimatedResult LoadAnimatedImage(
        BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) override;

    BASE_NS::vector<ImageType> GetSupportedTypes() const override;

    static LoadResult ResultFailure(const BASE_NS::string_view error);
    static LoadResult ResultSuccess(IImageContainer::Ptr image);
    static LoadAnimatedResult ResultFailureAnimated(const BASE_NS::string_view error);
    static LoadAnimatedResult ResultSuccessAnimated(IAnimatedImage::Ptr image);

private:
    void OnTypeInfoEvent(EventType type, BASE_NS::array_view<const ITypeInfo* const> typeInfos) override;
    IFileManager& fileManager_;
    struct RegisteredImageLoader {
        BASE_NS::Uid uid;
        IImageLoader::Ptr instance;
    };
    BASE_NS::vector<RegisteredImageLoader> imageLoaders_;
};
CORE_END_NAMESPACE()

#endif // CORE_IMAGE_IMAGE_LOADER_MANAGER_H
