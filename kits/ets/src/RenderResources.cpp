/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "RenderResources.h"
#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

RenderResources::RenderResources()
{}

RenderResources::~RenderResources()
{
    for (auto b : bitmaps_) {
        b.second.reset();
    }
}

void RenderResources::StoreBitmap(BASE_NS::string_view uri, SCENE_NS::IBitmap::Ptr bitmap)
{
    CORE_NS::UniqueLock lock(mutex_);
    if (bitmap) {
        bitmaps_[uri] = bitmap;
    } else {
        // setting null. releases.
        bitmaps_.erase(uri);
    }
}

SCENE_NS::IBitmap::Ptr RenderResources::FetchBitmap(BASE_NS::string_view uri)
{
    CORE_NS::UniqueLock lock(mutex_);
    auto it = bitmaps_.find(uri);
    if (it != bitmaps_.end()) {
        return it->second;
    }
    return {};
}
