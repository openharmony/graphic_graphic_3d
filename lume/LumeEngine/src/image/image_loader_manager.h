/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_IMAGE_IMAGE_LOADER_MANAGER_H
#define CORE_IMAGE_IMAGE_LOADER_MANAGER_H

#include <base/containers/vector.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;

class ImageLoaderManager final : public IImageLoaderManager {
public:
    explicit ImageLoaderManager(IFileManager& fileManager) : fileManager_(fileManager) {};
    ~ImageLoaderManager() override = default;

    void RegisterImageLoader(IImageLoader::Ptr imageLoader) override;

    LoadResult LoadImage(const BASE_NS::string_view uri, uint32_t loadFlags) override;
    LoadResult LoadImage(IFile& file, uint32_t loadFlags) override;
    LoadResult LoadImage(BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) override;

    LoadAnimatedResult LoadAnimatedImage(const BASE_NS::string_view uri, uint32_t loadFlags) override;
    LoadAnimatedResult LoadAnimatedImage(IFile& file, uint32_t loadFlags) override;
    LoadAnimatedResult LoadAnimatedImage(
        BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) override;

    static LoadResult ResultFailure(const BASE_NS::string_view error);
    static LoadResult ResultSuccess(IImageContainer::Ptr image);
    static LoadAnimatedResult ResultFailureAnimated(const BASE_NS::string_view error);
    static LoadAnimatedResult ResultSuccessAnimated(IAnimatedImage::Ptr image);

private:
    IFileManager& fileManager_;
    BASE_NS::vector<IImageLoader::Ptr> imageLoaders_;
};
CORE_END_NAMESPACE()

#endif // CORE_IMAGE_IMAGE_LOADER_MANAGER_H
