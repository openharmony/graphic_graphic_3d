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
#ifndef NODE_UID_H
#define NODE_UID_H

#include <scene_plugin/namespace.h>

#include <meta/base/types.h>
SCENE_BEGIN_NAMESPACE()
REGISTER_CLASS(Node, "d9b484d0-8275-434d-89c5-0b14f2924aa0", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(
    NodeHierarchyController, "aa18952b-81f0-41fe-97cc-489d802ad7a1", META_NS::ObjectCategoryBits::NO_CATEGORY)
SCENE_END_NAMESPACE()
#endif
