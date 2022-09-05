/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#ifndef CORE_IMAGE_LOADERS_IMAGE_LOADER_STB_IMAGE_H
#define CORE_IMAGE_LOADERS_IMAGE_LOADER_STB_IMAGE_H

#include <core/image/intf_image_loader_manager.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
#ifdef USE_STB_IMAGE
#if USE_STB_IMAGE
IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderStbImage();
#else
static inline IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderStbImage()
{
    return {};
}
#endif
#endif
CORE_END_NAMESPACE()

#endif //  CORE_IMAGE_LOADERS_IMAGE_LOADER_STB_IMAGE_H
