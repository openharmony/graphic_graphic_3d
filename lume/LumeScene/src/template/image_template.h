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

#ifndef SCENE_SRC_TEMPLATE_IMAGE_TEMPLATE_H
#define SCENE_SRC_TEMPLATE_IMAGE_TEMPLATE_H

#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/types.h>

#include <core/resources/intf_resource.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/property/property.h>
#include <meta/interface/resource/intf_resource.h>

SCENE_BEGIN_NAMESPACE()

// Interface exposing the static ImageLoadInfo property on ImageTemplate.
class IImageTemplate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImageTemplate, "66341605-5733-485a-8397-4f7eac4b650a")
public:
    META_PROPERTY(SCENE_NS::ImageLoadInfo, ImageLoadInfo)
};

// Minimal template holder for image load-time options.

// Deliberately NOT derived from IResourceTemplate / ResourceTemplateBase:
// images are immutable once loaded, so there is no "apply template to
// resource" step. Instead, ImageTemplate is a simple metadata bag that
// implements IResourceOptions so it can sit in ResourceData::options and
// be handed to the image loader on every LoadResource / ReloadResource.
// API consumers can edit ImageLoadInfo after import; edits take effect on
// the next load.
class ImageTemplate : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::INamed, IImageTemplate,
                          META_NS::IDerivedResourceOptions> {
    META_OBJECT(ImageTemplate, ClassId::ImageTemplate, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_STATIC_PROPERTY_DATA(IImageTemplate, SCENE_NS::ImageLoadInfo, ImageLoadInfo, SCENE_NS::DEFAULT_IMAGE_LOAD_INFO)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_PROPERTY(SCENE_NS::ImageLoadInfo, ImageLoadInfo)

    // IResourceOptions — all no-ops: options are read at image load time
    // and never applied to a post-load resource.
    bool ApplyOptions(CORE_NS::IResource&, const CORE_NS::ResourceContextPtr&) const override
    {
        return true;
    }
    bool UpdateOptions(const CORE_NS::IResource&, const CORE_NS::ResourceContextPtr&) override
    {
        return true;
    }
    bool Merge(const CORE_NS::IResourceOptions&) override
    {
        return false;
    }
    CORE_NS::IResourceOptions::Ptr Clone() const override;

    // IDerivedResourceOptions — retained only so JSON index entries that
    // specify `derivedFrom` for an image don't silently lose that link.
    // The image loader itself does not dereference baseResource_.
    void SetBaseResource(CORE_NS::ResourceId id) override
    {
        baseResource_ = BASE_NS::move(id);
    }
    CORE_NS::ResourceId GetBaseResource() const override
    {
        return baseResource_;
    }

private:
    CORE_NS::ResourceId baseResource_{};
};

SCENE_END_NAMESPACE()

#endif  // SCENE_SRC_TEMPLATE_IMAGE_TEMPLATE_H
