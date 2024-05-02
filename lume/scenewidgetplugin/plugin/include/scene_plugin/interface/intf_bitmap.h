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
#ifndef SCENEPLUGIN_INTF_BITMAP_H
#define SCENEPLUGIN_INTF_BITMAP_H

#include <base/containers/vector.h>
#include <render/resource_handle.h>

#include <core/plugin/intf_interface.h>
#include <meta/base/meta_types.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/property/property_events.h>

#include <scene_plugin/interface/compatibility.h>

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IBitmap, "1399fcd5-f338-43cb-8a78-0edb88470dfa")
class IBitmap : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBitmap)
public:
    enum BitmapStatus : uint32_t {
        /** No image has been loaded */
        NOT_INITIALIZED = 0,
        /** The image metadata is currently being loaded */
        LOADING_METADATA = 1,
        /** The image metadata loaded, size available */
        LOADED_METADATA = 2,
        /** The image is currently being loaded */
        LOADING_IMAGEDATA = 3,
        /** The image has been fully loaded */
        COMPLETED = 4,
        /** Loading has failed */
        FAILED = 5,
    };

    META_PROPERTY(BASE_NS::string, Uri)
    META_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)
    META_READONLY_PROPERTY(BitmapStatus, Status)
    virtual void SetRenderHandle(RENDER_NS::RenderHandleReference, const BASE_NS::Math::UVec2 size) = 0;
    virtual RENDER_NS::RenderHandleReference GetRenderHandle() const = 0;

    META_EVENT(META_NS::IOnChanged, ResourceChanged)
};


SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IBitmap::BitmapStatus)
META_TYPE(SCENE_NS::IBitmap::Ptr);

#endif // SCENEPLUGIN_INTF_MATERIAL_H
