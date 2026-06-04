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

#include <meta/interface/intf_attach.h>
#include <scene_adapter/intf_surface_stream.h>

namespace OHOS::Render3D {
ImageETS::ImageETS(const std::string& name, const std::string& uri, const SCENE_NS::IBitmap::Ptr bitmap)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::IMAGE), bitmap_(bitmap)
{
    SetName(name);
    SetUri(uri);

    resources_ = RenderContextETS::GetInstance().GetResources();

    if (bitmap_ == nullptr) {
        WIDGET_LOGE("bitmap is nullptr");
        return;
    }
    auto iAttach = interface_cast<META_NS::IAttach>(bitmap_);
    if (iAttach == nullptr) {
        WIDGET_LOGE("Incorrect type, iAttach is null");
        return;
    }
    auto attachments = iAttach->GetAttachments();
    for (auto& attachment : attachments) {
        auto surfaceStream = interface_pointer_cast<OHOS::Render3D::ISurfaceStream>(attachment);
        if (surfaceStream == nullptr) {
            continue;
        }
        attachment_ = attachment;
        break;
    }
}

ImageETS::ImageETS(const SCENE_NS::IImage::Ptr& image)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::IMAGE), bitmap_(image)
{
    resources_ = RenderContextETS::GetInstance().GetResources();
}

ImageETS::~ImageETS()
{
    Cleanup();
}

void ImageETS::Destroy()
{
    if (!uri_.empty()) {
        ExecSyncTask([uri = uri_, resources = resources_]() -> META_NS::IAny::Ptr {
            if (resources) {
                resources->StoreBitmap(uri.c_str(), nullptr);
            }
            return {};
        });
    }
    Cleanup();
}

void ImageETS::Cleanup()
{
    bitmap_.reset();
    resources_.reset();
}

META_NS::IObject::Ptr ImageETS::GetNativeObj() const
{
    return interface_pointer_cast<META_NS::IObject>(bitmap_);
}

int32_t ImageETS::GetWidth() const
{
    if (auto surfaceStream = interface_pointer_cast<OHOS::Render3D::ISurfaceStream>(attachment_); surfaceStream) {
        return surfaceStream->GetWidth();
    }

    if (!bitmap_) {
        WIDGET_LOGE("ImageETS::GetWidth() bitmap_ is nullptr");
        return 0;
    }
    if (auto size = bitmap_->Size(); size) {
        return size->GetValue().x;
    }
    return 0;
}

int32_t ImageETS::GetHeight() const
{
    if (auto surfaceStream = interface_pointer_cast<OHOS::Render3D::ISurfaceStream>(attachment_); surfaceStream) {
        return surfaceStream->GetHeight();
    }

    if (!bitmap_) {
        WIDGET_LOGE("ImageETS::GetHeight() bitmap_ is nullptr");
        return 0;
    }
    if (auto size = bitmap_->Size(); size) {
        return size->GetValue().y;
    }
    return 0;
}

std::string ImageETS::GetSurfaceId() const
{
    if (auto surfaceStream = interface_pointer_cast<OHOS::Render3D::ISurfaceStream>(attachment_); surfaceStream) {
        return std::to_string(surfaceStream->GetSurfaceId());
    }

    return "";
}
}  // namespace OHOS::Render3D