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

#ifndef SCENE_INTERFACE_RESOURCE_TYPES_H
#define SCENE_INTERFACE_RESOURCE_TYPES_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

// clang-format off
META_REGISTER_CLASS(AnimationResource, "4cfb3f35-2044-4834-8238-297aaed08b4f", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ImageResource, "586f9a9d-4041-407e-bbdc-92a2a87f76b0", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(EnvironmentResourceTemplate,
    "17694ec9-f452-48d9-9660-ceb924b8e45a", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(EnvironmentResource,
    "35d42f80-82e3-4eba-a5de-b9c888821ab2", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(EnvironmentTemplateAccess,
    "95b61ec6-f855-4115-91f4-5eae39ff5fb7", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MaterialResourceTemplate,
    "f7fb6428-96f0-4df0-839d-099ae87317b7", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MaterialResource, "5003b616-910c-4ea9-960b-1b193265de63", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MaterialTemplateAccess,
    "d01af41d-8362-4d97-b892-dd1b3eac19b6", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(OcclusionMaterialResourceTemplate,
    "f5c66c97-581d-4516-9c44-1ce9a3e7edb4", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(OcclusionMaterialResource,
    "2faf350d-0a85-4785-a8c3-4cd4e150da22", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(OcclusionMaterialTemplateAccess,
    "03a18da9-706e-4f71-99c3-45c5b5f52ada", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(PostProcessResourceTemplate,
    "161b04c3-df31-4014-b338-c1e643aed0fd", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(PostProcessResource,
    "e898b36b-e823-4540-bd6d-71708f6bde83", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(PostProcessTemplateAccess, "f905d854-dcda-4e57-a095-e7e572d5d714",
    META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(SceneResource, "d5665843-e011-4d77-a036-e973d309071c", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(GltfSceneResource, "e5c20c22-e646-4b78-9458-39f7c8d32f29", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ShaderResource, "37e22cc6-30db-4d07-8e6d-a5145490a82f", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(NodeInstantiator, "4c5292fc-761d-4045-b578-f0abbea60b24", META_NS::ObjectCategoryBits::NO_CATEGORY)
// clang-format on

SCENE_END_NAMESPACE()

#endif
