/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_RESOURCE_IRESOURCE_CONTEXT_H
#define SCENE_INTERFACE_RESOURCE_IRESOURCE_CONTEXT_H

#include <scene/interface/intf_scene.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IResourceContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceContext, "7e42090a-86fd-4f07-9c90-d5f178476823")
public:
    virtual IScene::Ptr GetScene() const = 0;
    virtual bool IsNodeTemplateContext() const = 0;
};

SCENE_END_NAMESPACE()

#endif
