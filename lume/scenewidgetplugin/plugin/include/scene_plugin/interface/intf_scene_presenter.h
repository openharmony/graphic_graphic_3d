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
#ifndef SCENEPLUGIN_INTF_SCENE_PRESENTER_H
#define SCENEPLUGIN_INTF_SCENE_PRESENTER_H

#include <scene_plugin/interface/intf_nodes.h>
#include <scene_plugin/interface/intf_scene.h>

#include <meta/base/types.h>

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IScenePresenter, "3d8f3e84-83e3-4734-b855-9f83fb68f3cf")
class IScenePresenter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IScenePresenter, InterfaceId::IScenePresenter)
public:
    /**
     * @brief Defines a camera to use
     * @return Pointer to the camera property.
     */
    META_PROPERTY(ICamera::Ptr, Camera)

    /**
     * @brief Scene to present.
     * @return Pointer to the scene property.
     */
    META_PROPERTY(IScene::Ptr, Scene)

    enum SCENE_RENDERMODE {
        RENDERMODE_DEFAULT = 0,
        RENDERMODE_INVALIDATE_ALWAYS = 1,
    };

    META_PROPERTY(uint8_t, RenderMode)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IScenePresenter::WeakPtr);
META_TYPE(SCENE_NS::IScenePresenter::Ptr);

#endif // SCENEPLUGIN_INTF_SCENE_PRESENTER_H
