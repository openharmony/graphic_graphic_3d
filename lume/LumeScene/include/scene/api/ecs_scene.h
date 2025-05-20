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

#ifndef SCENE_API_ECS_SCENE_H
#define SCENE_API_ECS_SCENE_H

#include <scene/api/scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/interface/intf_render_context.h>

#include <core/ecs/intf_ecs.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Accessor class for underlying ECS of a Scene.
 * @note  No thread safety is guaranteed for EcsScene object, the user is responsible for running any operation in the
 *        correct thread.
 */
class EcsScene : public META_NS::InterfaceObject<IScene> {
public:
    META_INTERFACE_OBJECT(EcsScene, META_NS::InterfaceObject<IScene>, IScene)
    /**
     * @brief Returns the underlying ECS of a Scene.
     */
    CORE_NS::IEcs::Ptr GetEcs() const
    {
        auto is = META_INTERFACE_OBJECT_CALL_PTR(GetInternalScene());
        return is ? is->GetEcsContext().GetNativeEcs() : nullptr;
    }
    /**
     * @brief Add a task to be run in the ECS update thread. Usually any underlying ECS access should be done in the
     *        this thread.
     * @note  If the call is made from the ECS thread, the task is executed immediately.
     * @param fn The function to execute.
     * @return A future object that can be waited on for task completion and result.
     */
    template<typename Fn>
    auto AddEcsThreadTask(Fn&& fn)
    {
        auto is = META_INTERFACE_OBJECT_CALL_PTR(GetInternalScene());
        auto ctx = is ? is->GetContext() : IRenderContext::Ptr {};
        return ctx ? AddTask(is, BASE_NS::forward<Fn>(fn), ctx->GetRenderQueue())
                   : Future<META_NS::PlainType_t<decltype(fn())>> {};
    }
    /**
     * @brief Add a task to be run in application thread.
     * @note  If the call is made from the application thread, the task is executed immediately.
     * @param fn The function to execute.
     * @return A future object that can be waited on for task completion and result.
     */
    template<typename Fn>
    auto AddApplicationThreadTask(Fn&& fn)
    {
        auto is = META_INTERFACE_OBJECT_CALL_PTR(GetInternalScene());
        auto ctx = is ? is->GetContext() : IRenderContext::Ptr {};
        return ctx ? AddTask(is, BASE_NS::forward<Fn>(fn), ctx->GetApplicationQueue())
                   : Future<META_NS::PlainType_t<decltype(fn())>> {};
    }

private:
    template<typename Fn>
    auto AddTask(const IInternalScene::Ptr& is, Fn&& fn, const META_NS::ITaskQueue::Ptr& queue)
    {
        return is ? is->AddTask(BASE_NS::forward<Fn>(fn), queue) : Future<META_NS::PlainType_t<decltype(fn())>> {};
    }
};

/**
 * @brief Accessor class for underlying ECS Entity of a Scene object.
 * @note  No thread safety is guaranteed for EcsObject object, the user is responsible for running any operation in the
 *        correct thread.
 */
class EcsObject : public META_NS::InterfaceObject<IEcsObjectAccess> {
public:
    META_INTERFACE_OBJECT(EcsObject, META_NS::InterfaceObject<IEcsObjectAccess>, IEcsObjectAccess)
    /**
     * @brief Returns the underlying ECS entity of an object in a Scene.
     */
    CORE_NS::Entity GetEntity() const
    {
        auto ecso = META_INTERFACE_OBJECT_CALL_PTR(GetEcsObject());
        return ecso ? ecso->GetEntity() : CORE_NS::Entity {};
    }
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_ECS_SCENE_H
