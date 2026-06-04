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

#ifndef SCENE_INTERFACE_SERIALIZATION_IMETADATA_IMPORTER_H
#define SCENE_INTERFACE_SERIALIZATION_IMETADATA_IMPORTER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/resource/intf_resource_context.h>
#include <scene/interface/scene_options.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IMetadataImporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMetadataImporter, "3e54f6f7-5821-4fe4-9207-2e0f15ad7b77")
public:
    virtual void RegisterResourceTypes(const IRenderContext::Ptr& context, const SceneOptions& opts) = 0;
    virtual void RegisterResourceTypes(
        const CORE_NS::IResourceManager::Ptr& res, const META_NS::IMetadata::Ptr& args) = 0;
};

META_REGISTER_CLASS(MetadataImporter, "b47973ff-a082-4618-863b-4dbf0c1f393b", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

#endif
