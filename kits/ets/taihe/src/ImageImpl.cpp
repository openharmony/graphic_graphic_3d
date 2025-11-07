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

#include "ImageImpl.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <scene/interface/intf_render_context.h>
#include <meta/interface/property/construct_property.h>

namespace OHOS::Render3D::KITETS {
::SceneResources::Image ImageImpl::createImageFromTH(SceneTH::SceneResourceParameters const &params)
{
    const std::string name = ExtractResourceName(params);
    const std::string uri = ExtractUri(params.uri).c_str();
    auto imageETS = RenderContextETS::GetInstance().CreateImage(name, uri);
    if (uri.empty() || name.empty() || !imageETS) {
        ::taihe::set_error("Invalid scene resource Image parameters given");
        return SceneResources::Image({nullptr, nullptr});
    }
    return taihe::make_holder<ImageImpl, ::SceneResources::Image>(imageETS);
}

ImageImpl::ImageImpl(const std::shared_ptr<ImageETS> imageETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::IMAGE, imageETS), imageETS_(imageETS)
{}

ImageImpl::~ImageImpl()
{
    imageETS_.reset();
}

void ImageImpl::destroy()
{
    if (imageETS_) {
        imageETS_->Destroy();
        imageETS_.reset();
    }
}

int32_t ImageImpl::getWidth()
{
    return imageETS_ != nullptr ? imageETS_->GetWidth() : 0;
}

int32_t ImageImpl::getHeight()
{
    return imageETS_ != nullptr ? imageETS_->GetHeight() : 0;
}
}  // namespace OHOS::Render3D::KITETS