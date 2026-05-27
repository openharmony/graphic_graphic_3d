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

#ifndef SCENE_SRC_TEMPLATE_TEMPLATE_COPY_HELPERS_H
#define SCENE_SRC_TEMPLATE_TEMPLATE_COPY_HELPERS_H

#include <initializer_list>
#include <scene/base/namespace.h>

#include <meta/interface/intf_metadata.h>
#include <meta/interface/property/intf_property.h>

SCENE_BEGIN_NAMESPACE()

enum class CopyMode {
    OVERRIDES_ONLY,  // Propagate only properties with IsValueSet (the overrides). If dst lacks the
                     // property, clone it into dst so the override lands.
    WITH_DEFAULTS    // Copy default slot to dst default; also copy set values if present.
};

void CopyMetadata(const META_NS::IMetadata& src, META_NS::IMetadata& dst, CopyMode mode);
void CopyMetadataExcept(const META_NS::IMetadata& src, META_NS::IMetadata& dst, CopyMode mode,
    std::initializer_list<BASE_NS::string_view> skipNames);
// Copy a single property from src onto dst's *default* slot (resetting any
// override). Sub-object / array-of-sub-object properties are deep-copied with
// CopyMode::WITH_DEFAULTS. Used by base-resource loading.
void CopyToDefaultMaybeDeep(META_NS::IMetadata& dst, const META_NS::IProperty::ConstPtr& srcProp);

// Type-agnostic accessor for an element's IMetadata aspect from an IArrayAny.
// Works whether the array's value type is META_NS::IObject::Ptr (templates) or
// a more specific interface (e.g. ITexture::Ptr on a live IMaterial).
META_NS::IMetadata* GetArrayElementMetadata(const META_NS::IArrayAny& arr, size_t index);
void CopyArrayElements(
    const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp, CopyMode mode);
void CopyReadonlyProperty(
    const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp, CopyMode mode);
void CopyProperty(const META_NS::IProperty::ConstPtr& srcProp, META_NS::IProperty& dstProp, CopyMode mode);
void CopyPropertyIfSet(const META_NS::IProperty::ConstPtr& src, const META_NS::IProperty::Ptr& dst);
void ClonePropertyWithDefaults(const META_NS::IProperty::ConstPtr& src, META_NS::IMetadata& dst);

SCENE_END_NAMESPACE()

#endif
