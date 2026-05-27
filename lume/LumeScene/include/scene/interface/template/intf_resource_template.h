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

#ifndef SCENE_INTERFACE_TEMPLATE_IRESOURCE_TEMPLATE_H
#define SCENE_INTERFACE_TEMPLATE_IRESOURCE_TEMPLATE_H

#include <scene/base/namespace.h>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_object.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Interface for applying template properties to a target object and
 *        copying properties from a source object into the template.
 */
class IResourceTemplate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceTemplate, "f6a234f8-a77a-4324-a667-0107681dd85b")
public:
    /// Copy set properties from this template onto target object.
    virtual bool ApplyTo(META_NS::IObject& target) const = 0;
    /// Copy properties from source object into this template. When
    /// onlySetValues is true (default) only properties that have an explicit
    /// value on source are copied; when false, all properties including
    /// defaults are copied.
    virtual bool CopyFrom(const META_NS::IObject& source, bool onlySetValues = true) = 0;
};

SCENE_END_NAMESPACE()

#endif
