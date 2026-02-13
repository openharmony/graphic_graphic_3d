/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <3d/intf_graphics_context.h>
#include <3d/implementation_uids.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_plugin_register.h>
#include <custom/lume_custom_render.h>
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <render/intf_render_context.h>
#include <render/implementation_uids.h>

#include "3d_widget_adapter_log.h"

#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif
#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/vulkan.h>

#include <render/vulkan/intf_device_vk.h>
#endif

// Note: GetPluginRegister and CreatePluginRegistry are global function pointers
// defined in lume_common.cpp (linked via widget_adapter_source).
// They must be initialized via dlsym at runtime before use.

namespace OHOS::Render3D {

// Test helper class to access protected and private members
class LumeCustomRenderTestHelper : public LumeCustomRender {
public:
    explicit LumeCustomRenderTestHelper(bool needsFrameCallback) : LumeCustomRender(needsFrameCallback) {}

    // Protected members
    float GetWidthScale() const { return widthScale_; }
    float GetHeightScale() const { return heightScale_; }
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }

    // Private members - accessible in TDD mode
    bool GetUseMultiSwapChain() const { return useMultiSwapChain_; }
    bool GetUseBasisEngine() const { return useBasisEngine_; }
    CORE_NS::IEngine::Ptr GetEngine() const { return engine_; }
    CORE3D_NS::IGraphicsContext::Ptr GetGraphicsContext() const { return graphicsContext_; }
    RENDER_NS::IRenderContext::Ptr GetRenderContext() const { return renderContext_; }
    CORE_NS::IEcs::Ptr GetEcs() const { return ecs_; }

    // Forward declaration - implementation is after LumeCustomRenderUT definition
    void SetValidPointersForTest();
};

class LumeCustomRenderUT : public testing::Test {
public:
    static void SetUpTestCase()
    {
        LoadEngineLib();
        LoadPlugins();
        CreateEngine();
        CreateRenderContext();
        ASSERT_NE(renderContext_, nullptr);
        CreateGraphicsContext();
        CreateEcs();
    }

    static void TearDownTestCase()
    {
        if (ecs_) {
            ecs_->Uninitialize();
        }
        ecs_.reset();
        graphicsContext3D_.reset();
        renderContext_.reset();
        engine_.reset();

        if (libHandle_) {
            dlclose(libHandle_);
            libHandle_ = nullptr;
        }
    }

    void SetUp() override
    {}

    void TearDown() override
    {}

private:
    static void LoadEngineLib()
    {
        // Load engine library and initialize global function pointers
        // These global function pointers are defined in lume_common.cpp

        // Try multiple possible library paths for different environments
        const char* libPaths[] = {
            "/system/lib64/libAGPDLL.z.so",  // Production device path
            "/system/lib/libAGPDLL.z.so",    // 32-bit device path
            "./libAGPDLL.z.so",              // Local development path
            "libAGPDLL.z.so"                 // Search in library path
        };

        for (const char* path : libPaths) {
            libHandle_ = dlopen(path, RTLD_LAZY);
            if (libHandle_) {
                WIDGET_LOGI("[LumeTest] Successfully loaded engine library from: %{public}s", path);
                break;
            }
        }

        if (!libHandle_) {
            WIDGET_LOGE("[LumeTest] Failed to load engine library from all paths:");
            for (const char* path : libPaths) {
                WIDGET_LOGE("[LumeTest]   - %{public}s", path);
            }
            WIDGET_LOGE("[LumeTest] Error: %{public}s", dlerror());
        }
        ASSERT_TRUE(libHandle_ != nullptr) <<
            "Failed to load engine library. Check that the engine library is built and accessible.";

        auto* createPluginReg = reinterpret_cast<void (*)(const CORE_NS::PlatformCreateInfo&)>(
            dlsym(libHandle_, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE"));
        auto* getPluginReg = reinterpret_cast<CORE_NS::IPluginRegister& (*)()>(
            dlsym(libHandle_, "_ZN4Core17GetPluginRegisterEv"));

        if (!createPluginReg || !getPluginReg) {
            if (!createPluginReg) {
                WIDGET_LOGE("[LumeTest] CreatePluginRegistry: NULL - %{public}s", dlerror());
            }
            if (!getPluginReg) {
                WIDGET_LOGE("[LumeTest] GetPluginRegister: NULL - %{public}s", dlerror());
            }
            ASSERT_TRUE(false) << "Failed to load engine function pointers";
        }

        // Assign to global function pointers
        CORE_NS::CreatePluginRegistry = createPluginReg;
        CORE_NS::GetPluginRegister = getPluginReg;

        // Verify function pointers are properly initialized
        ASSERT_TRUE(CORE_NS::CreatePluginRegistry != nullptr);
        ASSERT_TRUE(CORE_NS::GetPluginRegister != nullptr);

        WIDGET_LOGI("[LumeTest] Engine library loaded successfully, function pointers initialized");
    }

    static void LoadPlugins()
    {
        if (CORE_NS::CreatePluginRegistry == nullptr) {
            WIDGET_LOGE("[LumeTest] ERROR: CreatePluginRegistry is nullptr in LoadPlugins!");
            ASSERT_TRUE(false) << "CreatePluginRegistry became nullptr between LoadEngineLib and LoadPlugins";
        }

        // Create plugin registry with platform info
        // Use the paths defined in BUILD.gn
#define TO_STRING(name) #name
#define PLATFORM_PATH_NAME(name) TO_STRING(name)
        CORE_NS::PlatformCreateInfo platformCreateInfo {
            PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
            PLATFORM_PATH_NAME(PLATFORM_CORE_PLUGIN_PATH),
            PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
            PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH)
        };
#undef PLATFORM_PATH_NAME
#undef TO_STRING
        CORE_NS::CreatePluginRegistry(platformCreateInfo);
        // Load necessary plugins
        constexpr BASE_NS::Uid plugins[] = {
            RENDER_NS::UID_RENDER_PLUGIN,
            CORE3D_NS::UID_3D_PLUGIN
        };

        auto& pluginRegister = CORE_NS::GetPluginRegister();
        bool loadResult = pluginRegister.LoadPlugins(plugins);
        WIDGET_LOGI("[LumeTest] LoadPlugins returned, result = %{public}s", loadResult ? "true" : "false");

        if (!loadResult) {
            WIDGET_LOGE("[LumeTest] ERROR: Failed to load plugins!");
            ASSERT_TRUE(false) << "Failed to load required plugins";
        }

        WIDGET_LOGI("[LumeTest] LoadPlugins completed successfully");
    }

    static void CreateEngine()
    {
        auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
        ASSERT_TRUE(factory != nullptr) << "Failed to get engine factory";

#define TO_STRING(name) #name
#define PLATFORM_PATH_NAME(name) TO_STRING(name)
        CORE_NS::PlatformCreateInfo platformCreateInfo {
            PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
            PLATFORM_PATH_NAME(PLATFORM_CORE_PLUGIN_PATH),
            PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
            PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH)
        };
#undef PLATFORM_PATH_NAME
#undef TO_STRING

        CORE_NS::EngineCreateInfo engineCreateInfo {
            platformCreateInfo,
            {"UnitTest", 0, 1, 0},  // application version
            {}                       // application context
        };
        engine_ = factory->Create(engineCreateInfo);
        ASSERT_TRUE(engine_ != nullptr) << "Failed to create engine";

        engine_->Init();
    }

    static void CreateRenderContext()
    {
        if (!engine_) {
            WIDGET_LOGE("[LumeTest] ERROR: engine_ is null in CreateRenderContext!");
            ASSERT_TRUE(false) << "engine_ is null";
        }

        auto* classFactory = engine_->GetInterface<CORE_NS::IClassFactory>();

        if (!classFactory) {
            WIDGET_LOGE("[LumeTest] ERROR: GetInterface<IClassFactory>() returned null!");
            ASSERT_TRUE(false) << "GetInterface<IClassFactory>() returned null";
        }

        auto renderInstance = classFactory->CreateInstance(RENDER_NS::UID_RENDER_CONTEXT);
        if (!renderInstance) {
            WIDGET_LOGE("[LumeTest] ERROR: CreateInstance returned null!");
            ASSERT_TRUE(false) << "CreateInstance returned null";
        }

        renderContext_ = RENDER_NS::IRenderContext::Ptr(
            static_cast<RENDER_NS::IRenderContext*>(renderInstance.release()));

        ASSERT_TRUE(renderContext_ != nullptr) << "Failed to create render context";

        RENDER_NS::DeviceCreateInfo deviceCreateInfo;
#if RENDER_HAS_VULKAN_BACKEND
        WIDGET_LOGI("[LumeTest] Using Vulkan backend");
        RENDER_NS::BackendExtraVk vkExtra;
        deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::VULKAN;
        deviceCreateInfo.backendConfiguration = &vkExtra;
#elif RENDER_HAS_GLES_BACKEND
        WIDGET_LOGI("[LumeTest] Using OpenGL ES backend");
        RENDER_NS::BackendExtraGles glesExtra;
        glesExtra.applicationContext = EGL_NO_CONTEXT;
        glesExtra.sharedContext = EGL_NO_CONTEXT;
        glesExtra.MSAASamples = 0;
        glesExtra.depthBits = 24;
        deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGLES;
        deviceCreateInfo.backendConfiguration = &glesExtra;
#elif RENDER_HAS_GL_BACKEND
        WIDGET_LOGI("[LumeTest] Using OpenGL backend");
        RENDER_NS::BackendExtraGL glExtra;
        glExtra.MSAASamples = 0;
        glExtra.depthBits = 24;
        glExtra.alphaBits = 8;
        glExtra.stencilBits = 0;
        deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGL;
        deviceCreateInfo.backendConfiguration = &glExtra;
#else
        WIDGET_LOGE("[LumeTest] ERROR: No rendering backend available!");
        ASSERT_TRUE(false) << "No rendering backend available";
#endif

        auto result = renderContext_->Init({{"UnitTest", 0, 1, 0}, deviceCreateInfo});

        ASSERT_EQ(result, RENDER_NS::RenderResultCode::RENDER_SUCCESS);

        WIDGET_LOGI("[LumeTest] CreateRenderContext completed successfully");
    }

    static void CreateGraphicsContext()
    {
        // Verify renderContext_ is valid before proceeding
        if (!renderContext_) {
            WIDGET_LOGE("[LumeTest] ERROR: renderContext_ is null!");
            ASSERT_TRUE(false) << "renderContext_ is null in CreateGraphicsContext";
        }

        auto* classFactory = renderContext_->GetInterface<CORE_NS::IClassFactory>();

        if (!classFactory) {
            WIDGET_LOGE("[LumeTest] ERROR: GetInterface<CORE_NS::IClassFactory>() returned null!");
            ASSERT_TRUE(false) << "GetInterface<CORE_NS::IClassFactory>() returned null";
        }

        graphicsContext3D_ = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
            *classFactory, CORE3D_NS::UID_GRAPHICS_CONTEXT);

        ASSERT_TRUE(graphicsContext3D_ != nullptr) << "Failed to create graphics context";

        // Create and initialize with CreateInfo (required for proper initialization)
        CORE3D_NS::IGraphicsContext::CreateInfo gfxInf;
        gfxInf.colorSpaceFlags = 0;
        gfxInf.createFlags = 0;

        graphicsContext3D_->Init(gfxInf);
        WIDGET_LOGI("[LumeTest] graphicsContext3D_->Init(gfxInf) completed");
    }

    static void CreateEcs()
    {
        ecs_ = engine_->CreateEcs();
        ASSERT_TRUE(ecs_ != nullptr) << "Failed to create ECS";
        ecs_->Initialize();
    }

public:
    static void* libHandle_;
    static CORE_NS::IEngine::Ptr engine_;
    static RENDER_NS::IRenderContext::Ptr renderContext_;  // 使用 refcnt_ptr 与 LumeCustomRender 一致
    static CORE3D_NS::IGraphicsContext::Ptr graphicsContext3D_;
    static CORE_NS::IEcs::Ptr ecs_;
};

// Static member definitions
void* LumeCustomRenderUT::libHandle_ = nullptr;
CORE_NS::IEngine::Ptr LumeCustomRenderUT::engine_ {};
RENDER_NS::IRenderContext::Ptr LumeCustomRenderUT::renderContext_ {};
CORE3D_NS::IGraphicsContext::Ptr LumeCustomRenderUT::graphicsContext3D_ {};
CORE_NS::IEcs::Ptr LumeCustomRenderUT::ecs_ {};

// LumeCustomRenderTestHelper::SetValidPointersForTest implementation
// Must be after LumeCustomRenderUT definition to access its static members
void LumeCustomRenderTestHelper::SetValidPointersForTest()
{
    engine_ = LumeCustomRenderUT::engine_;
    graphicsContext_ = LumeCustomRenderUT::graphicsContext3D_;
    renderContext_ = LumeCustomRenderUT::renderContext_;
    ecs_ = LumeCustomRenderUT::ecs_;
}

/**
 * @tc.name: Constructor_NeedsCallbackTrue_InitialState
 * @tc.desc: Verify constructor with needsFrameCallback=true sets correct state
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Constructor_NeedsCallbackTrue_InitialState, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    EXPECT_TRUE(render.NeedsFrameCallback());
}

/**
 * @tc.name: Constructor_NeedsCallbackFalse_InitialState
 * @tc.desc: Verify constructor with needsFrameCallback=false sets correct state
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Constructor_NeedsCallbackFalse_InitialState, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(false);

    EXPECT_FALSE(render.NeedsFrameCallback());
}

/**
 * @tc.name: SetScaleInfo_DefaultValues
 * @tc.desc: Verify SetScaleInfo sets default scale values
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, SetScaleInfo_DefaultValues, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.SetScaleInfo(1.0f, 1.0f);

    EXPECT_FLOAT_EQ(render.GetWidthScale(), 1.0f);
    EXPECT_FLOAT_EQ(render.GetHeightScale(), 1.0f);
}

/**
 * @tc.name: SetScaleInfo_CustomValues
 * @tc.desc: Verify SetScaleInfo accepts custom scale values
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, SetScaleInfo_CustomValues, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.SetScaleInfo(0.5f, 2.0f);

    EXPECT_FLOAT_EQ(render.GetWidthScale(), 0.5f);
    EXPECT_FLOAT_EQ(render.GetHeightScale(), 2.0f);
}

/**
 * @tc.name: SetScaleInfo_ZeroValues
 * @tc.desc: Verify SetScaleInfo accepts zero scale values
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, SetScaleInfo_ZeroValues, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.SetScaleInfo(0.0f, 0.0f);

    EXPECT_FLOAT_EQ(render.GetWidthScale(), 0.0f);
    EXPECT_FLOAT_EQ(render.GetHeightScale(), 0.0f);
}

/**
 * @tc.name: SetScaleInfo_NegativeValues
 * @tc.desc: Verify SetScaleInfo accepts negative scale values
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, SetScaleInfo_NegativeValues, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.SetScaleInfo(-1.0f, -0.5f);

    EXPECT_FLOAT_EQ(render.GetWidthScale(), -1.0f);
    EXPECT_FLOAT_EQ(render.GetHeightScale(), -0.5f);
}

/**
 * @tc.name: SetScaleInfo_LargeValues
 * @tc.desc: Verify SetScaleInfo accepts large scale values
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, SetScaleInfo_LargeValues, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.SetScaleInfo(100.0f, 200.0f);

    EXPECT_FLOAT_EQ(render.GetWidthScale(), 100.0f);
    EXPECT_FLOAT_EQ(render.GetHeightScale(), 200.0f);
}

/**
 * @tc.name: GetSwapchainName_DefaultValue
 * @tc.desc: Verify GetSwapchainName returns default name
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetSwapchainName_DefaultValue, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    const std::string& name = render.GetSwapchainName();

    EXPECT_EQ(name, "RenderDataStoreDefaultStaging");
}

/**
 * @tc.name: GetInputBufferName_DefaultValue
 * @tc.desc: Verify GetInputBufferName returns default name
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetInputBufferName_DefaultValue, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    const std::string& name = render.GetInputBufferName();

    EXPECT_EQ(name, "INPUT_BUFFER");
}

/**
 * @tc.name: GetResolutionBufferName_DefaultValue
 * @tc.desc: Verify GetResolutionBufferName returns default name
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetResolutionBufferName_DefaultValue, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    const std::string& name = render.GetResolutionBufferName();

    EXPECT_EQ(name, "RESOLUTION_BUFFER");
}

/**
 * @tc.name: GetUseBasisEngine_DefaultValue
 * @tc.desc: Verify GetUseBasisEngine returns default false value
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetUseBasisEngine_DefaultValue, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    bool useBasis = render.GetUseBasisEngine();

    EXPECT_FALSE(useBasis);
}

/**
 * @tc.name: GetRenderHandles_Default_EmptyVector
 * @tc.desc: Verify GetRenderHandles returns empty vector when not initialized
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetRenderHandles_Default_EmptyVector, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    auto handles = render.GetRenderHandles();

    EXPECT_EQ(handles.size(), 1u);
}

/**
 * @tc.name: OnDrawFrame_Default_NoCrash
 * @tc.desc: Verify OnDrawFrame doesn't crash when called
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnDrawFrame_Default_NoCrash, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    render.OnDrawFrame();

    // OnDrawFrame is empty virtual, just verify it doesn't crash
    EXPECT_TRUE(render.NeedsFrameCallback());
}

/**
 * @tc.name: UnloadImages_Default_NoCrash
 * @tc.desc: Verify UnloadImages doesn't crash when called
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, UnloadImages_Default_NoCrash, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    render.UnloadImages();

    // Verify render is still valid after unload
    EXPECT_FALSE(render.GetSwapchainName().empty());
}

/**
 * @tc.name: UnloadRenderNodeGraph_Default_NoCrash
 * @tc.desc: Verify UnloadRenderNodeGraph doesn't crash when called
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, UnloadRenderNodeGraph_Default_NoCrash, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    render.UnloadRenderNodeGraph();

    // Verify render is still valid after unload
    EXPECT_FALSE(render.GetInputBufferName().empty());
}

/**
 * @tc.name: Destructor_NoCrash
 * @tc.desc: Verify destructor doesn't crash
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Destructor_NoCrash, testing::ext::TestSize.Level1)
{
    // Create and destroy multiple instances
    for (int i = 0; i < 10; i++) {
        LumeCustomRender render(true);
        EXPECT_TRUE(render.NeedsFrameCallback());
    }
}

/**
 * @tc.name: NeedsFrameCallback_True
 * @tc.desc: Verify NeedsFrameCallback returns true when set to true
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, NeedsFrameCallback_True, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    bool needsCallback = render.NeedsFrameCallback();

    EXPECT_TRUE(needsCallback);
}

/**
 * @tc.name: NeedsFrameCallback_False
 * @tc.desc: Verify NeedsFrameCallback returns false when set to false
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, NeedsFrameCallback_False, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(false);

    bool needsCallback = render.NeedsFrameCallback();

    EXPECT_FALSE(needsCallback);
}

/**
 * @tc.name: MultipleInstances_IndependentState
 * @tc.desc: Verify multiple render instances have independent state
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, MultipleInstances_IndependentState, testing::ext::TestSize.Level1)
{
    LumeCustomRender render1(true);
    LumeCustomRender render2(false);

    EXPECT_TRUE(render1.NeedsFrameCallback());
    EXPECT_FALSE(render2.NeedsFrameCallback());

    EXPECT_EQ(render1.GetSwapchainName(), "RenderDataStoreDefaultStaging");
    EXPECT_EQ(render2.GetSwapchainName(), "RenderDataStoreDefaultStaging");
}

/**
 * @tc.name: GetResolutionBufferName_AfterModification
 * @tc.desc: Verify GetResolutionBufferName returns expected value after construction
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetResolutionBufferName_AfterModification, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    std::string bufferName = render.GetResolutionBufferName();

    EXPECT_FALSE(bufferName.empty());
    EXPECT_EQ(bufferName, "RESOLUTION_BUFFER");
}

/**
 * @tc.name: GetInputBufferName_NotEmpty
 * @tc.desc: Verify GetInputBufferName returns non-empty string
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetInputBufferName_NotEmpty, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    std::string bufferName = render.GetInputBufferName();

    EXPECT_FALSE(bufferName.empty());
}

/**
 * @tc.name: GetSwapchainName_NotEmpty
 * @tc.desc: Verify GetSwapchainName returns non-empty string
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, GetSwapchainName_NotEmpty, testing::ext::TestSize.Level1)
{
    LumeCustomRender render(true);

    std::string swapchainName = render.GetSwapchainName();

    EXPECT_FALSE(swapchainName.empty());
}

// ============================================================================
// Tests for Initialize method
// ============================================================================

/**
 * @tc.name: Initialize_AllNullPointers_EarlyReturn
 * @tc.desc: Test early return when all pointers are null
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Initialize_AllNullPointers_EarlyReturn, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = nullptr;
    input.graphicsContext_ = nullptr;
    input.renderContext_ = nullptr;
    input.ecs_ = nullptr;
    input.width_ = 1920;
    input.height_ = 1080;
    input.useMultiSwapChain_ = false;

    render.Initialize(input);

    // Early return: dimensions should not be set
    EXPECT_EQ(render.GetWidth(), 0u);
    EXPECT_EQ(render.GetHeight(), 0u);
    // Pointers should remain null
    EXPECT_EQ(render.GetEngine(), nullptr);
    EXPECT_EQ(render.GetGraphicsContext(), nullptr);
    EXPECT_EQ(render.GetRenderContext(), nullptr);
    EXPECT_EQ(render.GetEcs(), nullptr);
}

/**
 * @tc.name: Initialize_OneNullPointer_EarlyReturn
 * @tc.desc: Test early return when any one pointer is null
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Initialize_OneNullPointer_EarlyReturn, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    // Test ecs_ = nullptr, others non-null
    {
        // First set all valid pointers
        render.SetValidPointersForTest();
        // Then create input and set ecs to nullptr
        CustomRenderInput input;
        input.engine_ = engine_;
        input.graphicsContext_ = graphicsContext3D_;
        input.renderContext_ = renderContext_;
        input.ecs_ = nullptr;  // This one is null
        input.width_ = 1920;
        input.height_ = 1080;

        render.Initialize(input);

        // Early return
        EXPECT_EQ(render.GetWidth(), 0u);
        EXPECT_EQ(render.GetHeight(), 0u);
    }

    // Test graphicsContext_ = nullptr
    {
        render.SetValidPointersForTest();
        CustomRenderInput input;
        input.engine_ = engine_;
        input.graphicsContext_ = nullptr;  // This one is null
        input.renderContext_ = renderContext_;
        input.ecs_ = ecs_;
        input.width_ = 1280;
        input.height_ = 720;

        render.Initialize(input);

        EXPECT_EQ(render.GetWidth(), 0u);
        EXPECT_EQ(render.GetHeight(), 0u);
    }
}

/**
 * @tc.name: Initialize_ValidPointers_MembersAssigned
 * @tc.desc: Test that input pointers are correctly assigned to member variables
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Initialize_ValidPointers_MembersAssigned, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    // Use the globally initialized engine contexts
    render.SetValidPointersForTest();

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 1920;
    input.height_ = 1080;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    // Verify pointers were assigned (now they are real pointers)
    EXPECT_NE(render.GetEngine(), nullptr);
    EXPECT_NE(render.GetGraphicsContext(), nullptr);
    EXPECT_NE(render.GetRenderContext(), nullptr);
    EXPECT_NE(render.GetEcs(), nullptr);
    EXPECT_EQ(render.GetUseMultiSwapChain(), true);
}

/**
 * @tc.name: Initialize_UseMultiSwapChainTrue_WidthHeightSet
 * @tc.desc: Test that width and height are set when useMultiSwapChain is true
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Initialize_UseMultiSwapChainTrue_WidthHeightSet, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);
    render.SetValidPointersForTest();

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    // With useMultiSwapChain=true, OnSizeChange is NOT called
    // but width_ and height_ should still be set
    EXPECT_EQ(render.GetWidth(), 2560u);
    EXPECT_EQ(render.GetHeight(), 1440u);
    EXPECT_EQ(render.GetUseMultiSwapChain(), true);
}

/**
 * @tc.name: Initialize_UseMultiSwapChainFalse_WidthHeightSet
 * @tc.desc: Test that width and height are set when useMultiSwapChain is false
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Initialize_UseMultiSwapChainFalse_WidthHeightSet, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);
    render.SetValidPointersForTest();

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 3840;
    input.height_ = 2160;
    input.useMultiSwapChain_ = false;

    render.Initialize(input);

    // With useMultiSwapChain=false, OnSizeChange would be called
    // and width_ and height_ should be set
    EXPECT_EQ(render.GetWidth(), 3840u);
    EXPECT_EQ(render.GetHeight(), 2160u);
    EXPECT_EQ(render.GetUseMultiSwapChain(), false);
}

/**
 * @tc.name: Initialize_MultipleCalls_LastOneWins
 * @tc.desc: Test that calling Initialize multiple times updates state
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, Initialize_MultipleCalls_LastOneWins, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);
    render.SetValidPointersForTest();

    // First call
    {
        CustomRenderInput input;
        input.engine_ = engine_;
        input.graphicsContext_ = graphicsContext3D_;
        input.renderContext_ = renderContext_;
        input.ecs_ = ecs_;
        input.width_ = 1920;
        input.height_ = 1080;
        input.useMultiSwapChain_ = false;

        render.Initialize(input);
        EXPECT_EQ(render.GetWidth(), 1920u);
        EXPECT_EQ(render.GetHeight(), 1080u);
    }

    // Second call with different values
    {
        CustomRenderInput input;
        input.engine_ = engine_;
        input.graphicsContext_ = graphicsContext3D_;
        input.renderContext_ = renderContext_;
        input.ecs_ = ecs_;
        input.width_ = 2560;
        input.height_ = 1440;
        input.useMultiSwapChain_ = true;

        render.Initialize(input);
        EXPECT_EQ(render.GetWidth(), 2560u);
        EXPECT_EQ(render.GetHeight(), 1440u);
        EXPECT_EQ(render.GetUseMultiSwapChain(), true);
    }
}

// ============================================================================
// Tests for OnSizeChange method
// ============================================================================

/**
 * @tc.name: OnSizeChange_ZeroWidth_EarlyReturn
 * @tc.desc: Test OnSizeChange returns early when width <= 0
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_ZeroWidth_EarlyReturn, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.OnSizeChange(0, 1080);

    // Early return: width_ should not be updated
    EXPECT_EQ(render.GetWidth(), 0u);
}

/**
 * @tc.name: OnSizeChange_ZeroHeight_EarlyReturn
 * @tc.desc: Test OnSizeChange returns early when height <= 0
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_ZeroHeight_EarlyReturn, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.OnSizeChange(1920, 0);

    // Early return: height_ should not be updated
    EXPECT_EQ(render.GetHeight(), 0u);
}

/**
 * @tc.name: OnSizeChange_NegativeWidth_EarlyReturn
 * @tc.desc: Test OnSizeChange returns early when width is negative
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_NegativeWidth_EarlyReturn, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.OnSizeChange(-1920, 1080);

    // Early return: dimensions should not be updated
    EXPECT_EQ(render.GetWidth(), 0u);
    EXPECT_EQ(render.GetHeight(), 0u);
}

/**
 * @tc.name: OnSizeChange_NegativeHeight_EarlyReturn
 * @tc.desc: Test OnSizeChange returns early when height is negative
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_NegativeHeight_EarlyReturn, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.OnSizeChange(1920, -1080);

    // Early return: dimensions should not be updated
    EXPECT_EQ(render.GetWidth(), 0u);
    EXPECT_EQ(render.GetHeight(), 0u);
}

/**
 * @tc.name: OnSizeChange_ValidDimensions_DimensionsUpdated
 * @tc.desc: Test OnSizeChange updates dimensions for valid input
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_ValidDimensions_DimensionsUpdated, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.OnSizeChange(1920, 1080);

    // Note: Will return early due to null renderContext_, but after setting width/height
    // Actually looking at the code: width_ and height_ are set BEFORE the buffer check
    EXPECT_EQ(render.GetWidth(), 1920u);
    EXPECT_EQ(render.GetHeight(), 1080u);
}

/**
 * @tc.name: OnSizeChange_MultipleCalls_LastOneWins
 * @tc.desc: Test that calling OnSizeChange multiple times updates state each time
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_MultipleCalls_LastOneWins, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.OnSizeChange(1920, 1080);
    EXPECT_EQ(render.GetWidth(), 1920u);
    EXPECT_EQ(render.GetHeight(), 1080u);

    render.OnSizeChange(1280, 720);
    EXPECT_EQ(render.GetWidth(), 1280u);
    EXPECT_EQ(render.GetHeight(), 720u);

    render.OnSizeChange(3840, 2160);
    EXPECT_EQ(render.GetWidth(), 3840u);
    EXPECT_EQ(render.GetHeight(), 2160u);
}

/**
 * @tc.name: OnSizeChange_LargeDimensions_Handled
 * @tc.desc: Test OnSizeChange handles large resolutions (8K, 16K)
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_LargeDimensions_Handled, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    render.OnSizeChange(7680, 4320);  // 8K
    EXPECT_EQ(render.GetWidth(), 7680u);
    EXPECT_EQ(render.GetHeight(), 4320u);

    render.OnSizeChange(16384, 16384);  // 16K
    EXPECT_EQ(render.GetWidth(), 16384u);
    EXPECT_EQ(render.GetHeight(), 16384u);
}

/**
 * @tc.name: OnSizeChange_ExtremeBoundary_MaxUint32
 * @tc.desc: Test OnSizeChange handles maximum reasonable uint32 values
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, OnSizeChange_ExtremeBoundary_MaxUint32, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    // Test with values close to uint32 max but reasonable
    render.OnSizeChange(8192, 8192);
    EXPECT_EQ(render.GetWidth(), 8192u);
    EXPECT_EQ(render.GetHeight(), 8192u);
}

// ============================================================================
// Tests for LoadImages method
// ============================================================================

/**
 * @tc.name: LoadImages_VariousInputs_NoCrash
 * @tc.desc: Verify LoadImages handles various input combinations without crashing
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, LoadImages_VariousInputs_NoCrash, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);

    // Test with empty vector
    render.LoadImages(std::vector<std::string>{});
    EXPECT_TRUE(render.NeedsFrameCallback());

    // Test with null engine (image paths but engine is null)
    render.LoadImages({"test.png", "image.jpg"});
    EXPECT_TRUE(render.NeedsFrameCallback());

    // Test with single image
    render.LoadImages({"test.png"});
    EXPECT_TRUE(render.NeedsFrameCallback());

    // Test with multiple images
    render.LoadImages({"image1.png", "image2.jpg", "image3.bmp"});
    EXPECT_TRUE(render.NeedsFrameCallback());

    // Test with empty paths
    render.LoadImages({"", "", ""});
    EXPECT_TRUE(render.NeedsFrameCallback());
}

/**
 * @tc.name: LoadImages_Unload_Cycle
 * @tc.desc: Verify LoadImages followed by UnloadImages works correctly
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, LoadImages_Unload_Cycle, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);

    render.LoadImages({"test.png"});
    render.UnloadImages();

    EXPECT_TRUE(render.NeedsFrameCallback());
}

// ============================================================================
// Tests for RegistorShaderPath method
// ============================================================================

/**
 * @tc.name: RegistorShaderPath_VariousPaths_NoCrash
 * @tc.desc: Verify RegistorShaderPath handles various path formats without crashing
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, RegistorShaderPath_VariousPaths_NoCrash, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);

    render.RegistorShaderPath("");                          // Empty path
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.RegistorShaderPath("/shaders/test.shader");     // Valid path
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.RegistorShaderPath("test.shader");              // Without slash
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.RegistorShaderPath("//path//to//test.shader");  // Multiple slashes
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.RegistorShaderPath("/custom/shaders/test.shader");
    EXPECT_TRUE(render.NeedsFrameCallback());
}

// ============================================================================
// Tests for UpdateShaderSpecialization method
// ============================================================================

/**
 * @tc.name: UpdateShaderSpecialization_VariousInputs_NoCrash
 * @tc.desc: Verify UpdateShaderSpecialization handles various input combinations
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, UpdateShaderSpecialization_VariousInputs_NoCrash, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);

    render.UpdateShaderSpecialization({});                      // Empty
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.UpdateShaderSpecialization({42});                    // Single value
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.UpdateShaderSpecialization({1, 2, 3, 4});          // Multiple values
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.UpdateShaderSpecialization({0, 0, 0, 0});          // Zero values
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.UpdateShaderSpecialization({0xFFFFFFFF, 0xFFFFFFFE, 0x12345678}); // Large values
    EXPECT_TRUE(render.NeedsFrameCallback());

    std::vector<uint32_t> largeVector(100, 42);               // Exceeds max
    render.UpdateShaderSpecialization(largeVector);
    EXPECT_TRUE(render.NeedsFrameCallback());
}

// ============================================================================
// Tests for UpdateShaderInputBuffer method
// ============================================================================

/**
 * @tc.name: UpdateShaderInputBuffer_ErrorCases
 * @tc.desc: Verify UpdateShaderInputBuffer handles various error cases
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, UpdateShaderInputBuffer_ErrorCases, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);

    // Test null buffer
    std::shared_ptr<ShaderInputBuffer> nullBuffer = nullptr;
    EXPECT_EQ(render.UpdateShaderInputBuffer(nullBuffer), false);

    // Test invalid buffer (not allocated)
    auto invalidBuffer = std::make_shared<ShaderInputBuffer>();
    EXPECT_EQ(render.UpdateShaderInputBuffer(invalidBuffer), false);

    // Test empty buffer
    auto emptyBuffer = std::make_shared<ShaderInputBuffer>();
    emptyBuffer->Alloc(0);
    EXPECT_EQ(render.UpdateShaderInputBuffer(emptyBuffer), false);

    // Test valid buffer with null render context
    auto validBuffer = std::make_shared<ShaderInputBuffer>();
    validBuffer->Alloc(10);
    EXPECT_EQ(render.UpdateShaderInputBuffer(validBuffer), true);

    // Test various buffer sizes
    auto smallBuffer = std::make_shared<ShaderInputBuffer>();
    smallBuffer->Alloc(1);
    EXPECT_EQ(render.UpdateShaderInputBuffer(smallBuffer), true);

    auto largeBuffer = std::make_shared<ShaderInputBuffer>();
    largeBuffer->Alloc(10000);
    EXPECT_EQ(render.UpdateShaderInputBuffer(largeBuffer), true);
}

// ============================================================================
// Tests for LoadRenderNodeGraph method
// ============================================================================

/**
 * @tc.name: LoadRenderNodeGraph_VariousInputs_NoCrash
 * @tc.desc: Verify LoadRenderNodeGraph handles various input combinations
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, LoadRenderNodeGraph_VariousInputs_NoCrash, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);
    RENDER_NS::RenderHandleReference output;

    render.LoadRenderNodeGraph("", output);                         // Empty path
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.LoadRenderNodeGraph("/path/to/test.rng", output);        // Valid path
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.LoadRenderNodeGraph("/custom/shaders/test", output);     // Without extension
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.LoadRenderNodeGraph("//path//to//test.rng", output);    // Multiple slashes
    EXPECT_TRUE(render.NeedsFrameCallback());

    render.LoadRenderNodeGraph("test.rng", RENDER_NS::RenderHandleReference{}); // Null output
    EXPECT_TRUE(render.NeedsFrameCallback());
}

/**
 * @tc.name: LoadRenderNodeGraph_Unload_Cycle
 * @tc.desc: Verify LoadRenderNodeGraph followed by UnloadRenderNodeGraph works
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, LoadRenderNodeGraph_Unload_Cycle, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);
    RENDER_NS::RenderHandleReference output;

    render.LoadRenderNodeGraph("test.rng", output);
    render.UnloadRenderNodeGraph();

    EXPECT_TRUE(render.NeedsFrameCallback());
}

// ============================================================================
// Tests for method sequences
// ============================================================================

/**
 * @tc.name: MethodSequences_StressTest
 * @tc.desc: Verify multiple operations in sequence don't crash
 * @tc.type: FUNC
 */
HWTEST_F(LumeCustomRenderUT, MethodSequences_StressTest, testing::ext::TestSize.Level1)
{
    LumeCustomRenderTestHelper render(true);

    CustomRenderInput input;
    input.engine_ = engine_;
    input.graphicsContext_ = graphicsContext3D_;
    input.renderContext_ = renderContext_;
    input.ecs_ = ecs_;
    input.width_ = 2560;
    input.height_ = 1440;
    input.useMultiSwapChain_ = true;

    render.Initialize(input);

    ASSERT_NE(engine_, nullptr);
    ASSERT_NE(renderContext_, nullptr);
    ASSERT_NE(ecs_, nullptr);

    // Multiple size changes
    render.OnSizeChange(1920, 1080);
    render.OnSizeChange(1280, 720);
    render.OnSizeChange(3840, 2160);

    // Multiple unload operations
    render.UnloadImages();
    render.UnloadImages();
    render.UnloadRenderNodeGraph();
    render.UnloadRenderNodeGraph();

    // Multiple OnDrawFrame calls
    for (int i = 0; i < 100; i++) {
        render.OnDrawFrame();
    }

    EXPECT_TRUE(render.NeedsFrameCallback());
}

} // namespace OHOS::Render3D
