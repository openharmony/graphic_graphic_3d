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

#ifndef SCENE_INTERFACE_IAPPLICATION_CONTEXT_H
#define SCENE_INTERFACE_IAPPLICATION_CONTEXT_H

#include <scene/base/types.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/scene_options.h>

#include <meta/interface/intf_task_queue.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief The IApplicationContext interface can be used to access application specific parameters.
 */
class IApplicationContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IApplicationContext, "52d1686d-8214-49ea-98f7-83c02a45f985")
public:
    struct ApplicationContextInfo {
        META_NS::ITaskQueue::Ptr engineTaskQueue;
        META_NS::ITaskQueue::Ptr applicationTaskQueue;
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext;
        CORE_NS::IResourceManager::Ptr resourceManager;
        SceneOptions defaultSceneOptions;
    };
    virtual bool Initialize(const ApplicationContextInfo& info) = 0;

    virtual IRenderContext::Ptr GetRenderContext() const = 0;
    virtual ISceneManager::Ptr GetSceneManager() const = 0;
    virtual SceneOptions GetDefaultSceneOptions() const = 0;
};

/**
 * @brief The IApplicationContextProvider interface can be implemented by classes which can be queried for an
 *        IApplicationContext instance they belong to.
 * @note  The default RenderContext implementation (return by calling
 *        GetDefaultApplicationContext()->GetRenderContext()) implements IApplicationContextProvider, enabling to query
 *        the application context a render context is associated with.
 */
class IApplicationContextProvider : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IApplicationContextProvider, "71c07cc6-36b5-4b71-a0e8-be4731a9753d")
public:
    /// Returns the associated application context.
    virtual IApplicationContext::ConstPtr GetApplicationContext() const = 0;
};

class IApplicationContextSetter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IApplicationContextSetter, "586601d7-ded9-43f9-aacf-f7ccd7fa33cf")
public:
    virtual void SetApplicationContext(const IApplicationContext::ConstPtr& context) = 0;
};

META_REGISTER_SINGLETON_CLASS(
    ApplicationContext, "424b9f97-6447-41c0-8225-1fb0e1bbd7e8", META_NS::ObjectCategoryBits::NO_CATEGORY)

/// Helper function for retrieving the application context associated with a render context
inline IApplicationContext::ConstPtr GetApplicationContext(const IRenderContext::Ptr& renderContext)
{
    if (auto provider = interface_cast<IApplicationContextProvider>(renderContext)) {
        return provider->GetApplicationContext();
    }
    return nullptr;
}

/// Helper function for retrieving the scene manager associated with the application context of a render context
inline ISceneManager::Ptr GetSceneManager(const IRenderContext::Ptr& renderContext)
{
    if (auto ctx = GetApplicationContext(renderContext)) {
        return ctx->GetSceneManager();
    }
    return nullptr;
}

inline IApplicationContext::ConstPtr GetDefaultApplicationContext()
{
    if (auto obj = META_NS::GetObjectRegistry().Create<IApplicationContext>(ClassId::ApplicationContext)) {
        return obj;
    }
    return nullptr;
}

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IApplicationContext)

#endif
