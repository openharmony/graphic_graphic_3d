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
#ifndef POSTPROCESS_UID_H
#define POSTPROCESS_UID_H

#include <scene_plugin/namespace.h>

#include <meta/base/types.h>
SCENE_BEGIN_NAMESPACE()
REGISTER_CLASS(PostProcess, "294596ca-1409-4c30-a73a-eb03c5216033", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Bloom, "75ebe528-8924-4c6a-9d53-a6d68f6f436d", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Blur, "9069981e-a568-4502-acea-ae541c229950", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(ColorConversion, "db42f775-5eb6-4493-9245-732e14d846d2", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(ColorFringe, "3d051a01-4921-4058-ab54-aa3160bd3952", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(DepthOfField, "b804c877-f59d-4516-ad2e-0a9dee099a64", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Dither, "a4c4b56e-2fa9-4341-8540-6c56a5bd2da0", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Fxaa, "741b33e8-b6cd-4dbb-affb-04eaa2254b1a", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(MotionBlur, "dcfea024-a61e-45ea-8f6b-cd0a1ae6de12", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Taa, "15418c52-1aa5-4518-9c52-fb8429322eb2", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Tonemap, "7da7b828-3827-47de-869b-1fd3b46ebba6", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Vignette, "2a3fa46e-e6c0-4c83-9b0c-7be1330f5914", META_NS::ObjectCategoryBits::NO_CATEGORY)
SCENE_END_NAMESPACE()

#endif
