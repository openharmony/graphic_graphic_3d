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

#ifndef SCENE_IMP_SRC_LOADERS_RESOURCE_TYPES_H
#define SCENE_IMP_SRC_LOADERS_RESOURCE_TYPES_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/scene_options.h>
#include <scene_importer/base/namespace.h>
#include <scene_importer/interface/intf_importer.h>

SCENE_IMP_BEGIN_NAMESPACE()

META_REGISTER_CLASS(
    GltfAnimationLoader, "02f1075b-f64c-40fc-b813-0fa3990a61dc", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    MetaAnimationLoader, "4a1b0f71-65e1-4dce-9929-ac45884daeb6", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(EnvironmentLoader, "0dde9454-30cf-4178-94e9-6208441eb433", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(GltfSceneLoader, "e635252f-5615-4abc-99b2-295f58028440", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ImageLoader, "822c4159-0352-44a6-884d-3bb8b32960e6", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MaterialLoader, "002b909b-b4fe-49b6-a0fb-5896c242ad77", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    OcclusionMaterialLoader, "4f5c77c8-dd0f-43bb-aec3-d64ce3fea30b", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    NodeTemplateLoader, "0936dac2-4555-4fc3-b221-906ab0331743", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(PostProcessLoader, "06b80fd4-34b0-4f9c-ac1b-bd56bda4d5ee", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(SceneLoader, "fbb393fc-a0f0-4a9e-853a-2b2b8aa6bc31", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ShaderLoader, "730334ee-bac4-4ea1-92e6-e0facd82a958", META_NS::ObjectCategoryBits::NO_CATEGORY)

META_REGISTER_CLASS(
    AnimationTemplateLoader, "f47c8a1e-2d69-4b35-8e7a-3c91d5f20b64", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    MaterialTemplateLoader, "c0f84d82-2b2c-40f5-91b4-ae55a07e211c", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    OcclusionMaterialTemplateLoader, "a757a07a-ab6a-409e-bb9e-2e6d5c2cf42d", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    PostProcessTemplateLoader, "2a008e31-d35d-45b1-8d1f-e19a38207e3f", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    EnvironmentTemplateLoader, "b177393c-8cb5-465c-8cd1-210e512d244a", META_NS::ObjectCategoryBits::NO_CATEGORY)

void RegisterResourceTypes(const IImporter::Ptr& importer);
void RegisterResourceTypes(const IImporter::Ptr& importer, const CORE_NS::IResourceManager::Ptr& res);

SCENE_IMP_END_NAMESPACE()

#endif