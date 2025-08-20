/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "ImageETS.h"

#include "RenderContextETS.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D {
ImageETS::ImageETS(const std::string &name, const std::string &uri, const SCENE_NS::IBitmap::Ptr bitmap)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::IMAGE), bitmap_(bitmap)
{
    WIDGET_LOGI("ImageETS ++, name: %{public}s, uri:%{public}s", name.c_str(), uri.c_str());
    SetName(name);
    SetUri(uri);

    resources_ = RenderContextETS::GetInstance().GetResources();
}

ImageETS::ImageETS(const SCENE_NS::IImage::Ptr &image)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::IMAGE), bitmap_(image)
{
}

ImageETS::~ImageETS()
{
    WIDGET_LOGI("ImageETS --");
    if (!uri_.empty()) {
        ExecSyncTask([uri = uri_, resources = resources_]() -> META_NS::IAny::Ptr {
            if (resources) {
                resources->StoreBitmap(uri.c_str(), nullptr);
            }
            return {};
        });
    }
    bitmap_.reset();
    resources_.reset();
}

META_NS::IObject::Ptr ImageETS::GetNativeObj() const
{
    return interface_pointer_cast<META_NS::IObject>(bitmap_);
}

int32_t ImageETS::GetWidth() const
{
    if (!bitmap_) {
        WIDGET_LOGE("ImageETS::GetWidth() bitmap_ is nullptr");
        return 0;
    }
    return bitmap_->Size()->GetValue().x;
}

int32_t ImageETS::GetHeight() const
{
    if (!bitmap_) {
        WIDGET_LOGE("ImageETS::GetHeight() bitmap_ is nullptr");
        return 0;
    }
    return bitmap_->Size()->GetValue().y;
}
}  // namespace OHOS::Render3D