/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#ifndef CORE_IMAGE_LOADERS_IMAGE_LOADER_KTX_H
#define CORE_IMAGE_LOADERS_IMAGE_LOADER_KTX_H

#include <core/image/intf_image_loader_manager.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderKtx();
CORE_END_NAMESPACE()

#endif //  CORE_IMAGE_LOADERS_IMAGE_LOADER_KTX_H