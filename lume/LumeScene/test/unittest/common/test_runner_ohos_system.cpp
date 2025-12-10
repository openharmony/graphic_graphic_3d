/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "test_runner_ohos_system.h"

#include <3d/implementation_uids.h>
#include <core/engine_info.h>
#include <core/log.h>
#include <core/plugin/intf_class_factory.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>

#include <meta/api/future.h>
#include <meta/api/task_queue.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>

#if (RENDER_HAS_VULKAN_BACKEND)
#include <vulkan/vulkan.h>

#include <render/vulkan/intf_device_vk.h>
#endif
#if (RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND)
#include <render/gles/intf_device_gles.h>
#endif

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
CORE_NS::IPluginRegister& (*CORE_NS::GetPluginRegister)() { nullptr };
void (*CORE_NS::CreatePluginRegistry)(const struct CORE_NS::PlatformCreateInfo& platformCreateInfo) { nullptr };
#endif

SCENE_BEGIN_NAMESPACE()
namespace UTest {

TestEnvironment* GetTestEnv()
{
    CORE_ASSERT(g_testEnv.get());
    return g_testEnv.get();
}

void RegisterPaths(CORE_NS::IFileManager& fileManager, CORE_NS::IEngine& engine)
{
    // OHOS specific paths.
    const BASE_NS::string applicationDirectory = "file://" + GetTestEnv()->platformCreateInfo.appRootPath;
    const BASE_NS::string cacheDirectory = applicationDirectory + "cache/";

    if (fileManager.OpenDirectory(cacheDirectory) == nullptr) {
        const auto _ = fileManager.CreateDirectory(cacheDirectory);
    }
    CORE_ASSERT(fileManager.OpenDirectory(cacheDirectory) != nullptr);

    // Create app:// protocol that points to application root directory or application private data directory.
    fileManager.RegisterPath("app", applicationDirectory, true);
    // Create cache:// protocol that points to application cache directory.
    fileManager.RegisterPath("cache", cacheDirectory, true);
}

void RegisterTestFileProtocol(CORE_NS::IFileManager& fileManager)
{
    // OHOS specific paths
    const BASE_NS::string applicationTestAssetsDirectory = "file://" + GetTestEnv()->appAssetPath;

    // Create test:// protocol that points to application test data directory.
    fileManager.RegisterPath("test", applicationTestAssetsDirectory, true);

    // Register protocol requeired by LumeScene plugin assets
    fileManager.RegisterPath("test_assets", applicationTestAssetsDirectory, true);

    // TODO: Scene plugin also uses the "project:" protocol. The whole requirement should be removed.
    fileManager.RegisterPath("project", "test_assets://project/", false);
    fileManager.RegisterPath("assets", "project://", false);
    fileManager.RegisterPath("fonts", "project://fonts/", false);
    fileManager.RegisterPath("fonts", "project://assets/fonts/", false);
}

CORE_NS::IEngine::Ptr CreateEngine()
{
    // Show only errors because we don't want to clutter the test output.
    const ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_ERROR);

    const CORE_NS::EngineCreateInfo engineCreateInfo { GetTestEnv()->platformCreateInfo,
        // applicationVersion
        {
            "test", // name
            0,      // versionMajor
            1,      // versionMinor
            0,      // versionPatch
        },
        // applicationContext
        {} };

    auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
    auto engine = factory->Create(engineCreateInfo);

    // Register paths and protocols (using engine FileManager)
    RegisterPaths(engine->GetFileManager(), *engine);
    RegisterTestFileProtocol(engine->GetFileManager());

    engine->Init();

    return engine;
}

BASE_NS::shared_ptr<RENDER_NS::IRenderContext> CreateRenderContext(
    CORE_NS::IEngine& engine, const RENDER_NS::DeviceBackendType backend)
{
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext(static_cast<RENDER_NS::IRenderContext*>(
        engine.GetInterface<CORE_NS::IClassFactory>()->CreateInstance(RENDER_NS::UID_RENDER_CONTEXT).release()));

    RENDER_NS::DeviceCreateInfo deviceCreateInfo;
#if RENDER_HAS_VULKAN_BACKEND
    RENDER_NS::BackendExtraVk vkExtra;
    if (backend == RENDER_NS::DeviceBackendType::VULKAN) {
        deviceCreateInfo.backendType = backend;
        deviceCreateInfo.backendConfiguration = &vkExtra;
    }
#endif

#if RENDER_HAS_GL_BACKEND
    RENDER_NS::BackendExtraGL glExtra;
    if (backend == RENDER_NS::DeviceBackendType::OPENGL || backend == RENDER_NS::DeviceBackendType::OPENGLES) {
        glExtra.MSAASamples = 0;
        glExtra.depthBits = 24;
        glExtra.alphaBits = 8;
        glExtra.stencilBits = 0;
        deviceCreateInfo.backendType = backend;
        deviceCreateInfo.backendConfiguration = &glExtra;
    }
#endif

    const RENDER_NS::RenderCreateInfo info {
        {
            "test", // name
            1,      // versionMajor
            0,      // versionMinor
            0,      // versionPatch
        },
        deviceCreateInfo,
    };
    const RENDER_NS::RenderResultCode rrc = renderContext->Init(info);
    if (rrc == RENDER_NS::RenderResultCode::RENDER_SUCCESS) {
        return renderContext;
    } else {
        CORE_LOG_E("Render context init failed");
        CORE_ASSERT(false);
    }

    return nullptr;
}

CORE3D_NS::IGraphicsContext::Ptr CreateGraphicsContext(BASE_NS::shared_ptr<RENDER_NS::IRenderContext>& renderContext)
{
    auto c3d = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
        *renderContext->GetInterface<CORE_NS::IClassFactory>(), CORE3D_NS::UID_GRAPHICS_CONTEXT);

    c3d->Init();

    return c3d;
}

IApplicationContext::Ptr CreateAppContext(
    CORE_NS::IEngine& engine, BASE_NS::shared_ptr<RENDER_NS::IRenderContext>& renderContext)
{
    auto& taskQueueRegistry = META_NS::GetTaskQueueRegistry();

    auto engineTaskQueue =
        META_NS::GetObjectRegistry().Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
    auto appTaskQueue = engineTaskQueue;

    auto resources =
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);

    resources->SetFileManager(CORE_NS::IFileManager::Ptr(&engine.GetFileManager()));

    auto appc = META_NS::GetObjectRegistry().Create<IApplicationContext>(ClassId::ApplicationContext);

    if (appc) {
        IApplicationContext::ApplicationContextInfo info { engineTaskQueue, appTaskQueue, renderContext, resources,
            SceneOptions {} };

        if (appc->Initialize(info)) {
            return appc;

        } else {
            CORE_LOG_E("App context init failed");
            CORE_ASSERT(false);
        }
    }

    return nullptr;
}

} // namespace UTest
SCENE_END_NAMESPACE()

testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new SCENE_NS::UTest::TestRunnerEnv);
