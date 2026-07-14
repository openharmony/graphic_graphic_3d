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

#ifndef SCENE_API_TEMPLATE_IMAGE_TEMPLATE_H
#define SCENE_API_TEMPLATE_IMAGE_TEMPLATE_H

#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/types.h>

#include <core/resources/intf_resource.h>

#include <meta/api/interface_object.h>
#include <meta/api/object.h>

SCENE_BEGIN_NAMESPACE()

// Typed API wrapper for the minimal ImageTemplate holder. Exposes the two
// editable properties that control image loading. Changes made through
// this wrapper take effect on the next image load / reload because the
// importer's image loader reads ImageLoadInfo on every load call.
class ImageTemplate : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(ImageTemplate, META_NS::Object, CORE_NS::IResourceOptions)
    META_INTERFACE_OBJECT_INSTANTIATE(ImageTemplate, ::SCENE_NS::ClassId::ImageTemplate)

    template<typename Type>
    auto GetProperty(BASE_NS::string_view name) const
    {
        auto meta = META_NS::Metadata(*this);
        return meta.GetProperty<Type>(name);
    }

    auto Name() const
    {
        return GetProperty<BASE_NS::string>("Name");
    }
    auto ImageLoadInfo() const
    {
        return GetProperty<SCENE_NS::ImageLoadInfo>("ImageLoadInfo");
    }
};

SCENE_END_NAMESPACE()

#endif  // SCENE_API_TEMPLATE_IMAGE_TEMPLATE_H
