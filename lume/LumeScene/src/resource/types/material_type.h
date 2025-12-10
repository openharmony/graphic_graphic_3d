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

#ifndef SCENE_SRC_RESOURCE_TYPES_MATERIAL_TYPE_H
#define SCENE_SRC_RESOURCE_TYPES_MATERIAL_TYPE_H

#include <scene/interface/intf_material.h>

#include "resource_type_base.h"

SCENE_BEGIN_NAMESPACE()

class MaterialResourceType : public META_NS::IntroduceInterfaces<ResourceTypeBase> {
    META_OBJECT(MaterialResourceType, ClassId::MaterialResource, IntroduceInterfaces)
public:
    MaterialResourceType() : Super(ClassId::MaterialResource) {}

    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;
};

class OcclusionMaterialResourceType : public META_NS::IntroduceInterfaces<ResourceTypeBase> {
    META_OBJECT(OcclusionMaterialResourceType, ClassId::OcclusionMaterialResource, IntroduceInterfaces)
public:
    OcclusionMaterialResourceType() : Super(ClassId::OcclusionMaterialResource) {}

    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;
};

SCENE_END_NAMESPACE()

#endif
