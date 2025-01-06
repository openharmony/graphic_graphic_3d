/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_MESH_MESHRESOURCE_H
#define SCENE_SRC_MESH_MESHRESOURCE_H

#include <scene/interface/intf_mesh_resource.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class MeshResource : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IMeshResource> {
    META_OBJECT(MeshResource, SCENE_NS::ClassId::MeshResource, IntroduceInterfaces)
public:
};

SCENE_END_NAMESPACE()

#endif
