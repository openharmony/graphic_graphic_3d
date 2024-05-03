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
#ifndef CORE_IMAGE_LOADERS_IMAGE_LOADER_STB_IMAGE_H
#define CORE_IMAGE_LOADERS_IMAGE_LOADER_STB_IMAGE_H

#include <core/image/intf_image_loader_manager.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
static const CORE_NS::IImageLoaderManager::ImageType STB_IMAGE_TYPES[] = {
    { "image/png", "png" },
    { "image/jpeg", "jpeg" },
    { "image/jpeg", "jpg" },
};

#if defined(USE_STB_IMAGE) && (USE_STB_IMAGE == 1)
IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderStbImage(PluginToken);
#else
static inline IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderStbImage(PluginToken)
{
    return {};
}
#endif
CORE_END_NAMESPACE()

#endif //  CORE_IMAGE_LOADERS_IMAGE_LOADER_STB_IMAGE_H