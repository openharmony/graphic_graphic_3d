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

#ifndef SCENE_INTERFACE_IMESHRESOURCE_H
#define SCENE_INTERFACE_IMESHRESOURCE_H

#include <core/plugin/intf_interface.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Hold GPU resources for meshes-to-be-created.
 * @note Nothing implement yet. This is a placeholder interface.
 */
class IMeshResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMeshResource, "dfaaac5a-337c-4bbe-8aa6-3f09827419c2")
public:
};

META_REGISTER_CLASS(MeshResource, "82a96082-5089-4778-be70-dfb8b36fbb75", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IMeshResource)

#endif
