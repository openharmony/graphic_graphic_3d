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
#ifndef MATERIAL_NODE_UID_H
#define MATERIAL_NODE_UID_H

#include <scene_plugin/namespace.h>

#include <meta/base/types.h>
SCENE_BEGIN_NAMESPACE()
REGISTER_CLASS(TextureInfo, "fb6e3268-d5da-45ec-95c7-ebc26cf58d4a", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(CustomPropertiesHolder, "ba1985b4-e4bb-4408-b448-feaeb5c47806", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Shader, "56d686b8-7a33-4608-b12a-1a170381bcfd", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(GraphicsState, "b49d2492-9bf3-467d-a214-de711a9e6e24", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Material, "ffcb25d5-18fd-42ad-8df5-ebd5197bc8a6", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Bitmap, "77b38a92-8182-4562-bd3f-deab7b40cedc", META_NS::ObjectCategoryBits::NO_CATEGORY)
SCENE_END_NAMESPACE()

#endif
