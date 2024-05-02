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
#ifndef MESH_NODE_UID_H
#define MESH_NODE_UID_H

#include <scene_plugin/namespace.h>

#include <meta/base/types.h>
SCENE_BEGIN_NAMESPACE()
REGISTER_CLASS(SubMesh, "cbdb4719-a593-4ec9-9ed3-3476782c6925", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(Mesh, "07de757e-804a-4849-9d11-a54d7c88659d", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(MultiMeshProxy, "797a27c0-d233-499a-adbc-fa4bf7b944ff", META_NS::ObjectCategoryBits::NO_CATEGORY)
SCENE_END_NAMESPACE()

#endif
