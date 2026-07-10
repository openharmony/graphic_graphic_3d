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

// Just the ImageLoaderManager::Result* statics the loaders reference; compiling the full
// image_loader_manager.cpp would pull in the plugin register via its ctor/dtor.

#include <algorithm>

#include <base/containers/string_view.h>

#include "image/image_loader_manager.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::move;
using BASE_NS::string_view;

ImageLoaderManager::LoadResult ImageLoaderManager::ResultFailure(const string_view error)
{
    LoadResult result{false, "", nullptr};

    const auto count = std::min(error.size(), sizeof(result.error) - 1);
    error.copy(result.error, count);
    result.error[count] = '\0';

    return result;
}

ImageLoaderManager::LoadResult ImageLoaderManager::ResultSuccess(IImageContainer::Ptr image)
{
    return LoadResult{true, "", move(image)};
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::ResultFailureAnimated(const string_view error)
{
    LoadAnimatedResult result{false, "", nullptr};

    const auto count = std::min(error.size(), sizeof(result.error) - 1);
    error.copy(result.error, count);
    result.error[count] = '\0';

    return result;
}

ImageLoaderManager::LoadAnimatedResult ImageLoaderManager::ResultSuccessAnimated(IAnimatedImage::Ptr image)
{
    return LoadAnimatedResult{true, "", move(image)};
}
CORE_END_NAMESPACE()
