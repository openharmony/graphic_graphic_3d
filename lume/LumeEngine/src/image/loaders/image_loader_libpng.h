/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */
 
#ifndef CORE_IMAGE_LOADERS_IMAGE_LOADER_LIB_PNG_H
#define CORE_IMAGE_LOADERS_IMAGE_LOADER_LIB_PNG_H

#include <core/image/intf_image_loader_manager.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderLibPNGImage();
CORE_END_NAMESPACE()

#endif  //  CORE_IMAGE_LOADERS_IMAGE_LOADER_LIB_PNG_H