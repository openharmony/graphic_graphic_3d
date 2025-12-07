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

#include <device/device.h>
#include <device/shader_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

void LoadShaderDataTest(const UTest::EngineResources& engine)
{
    auto device = (Device*)engine.device;
    ASSERT_TRUE(device != nullptr);

    auto* fileManager = &engine.engine->GetFileManager();
    ASSERT_TRUE(fileManager != nullptr);

#if defined(__ANDROID__)

    constexpr string_view applicationTestAssetsDirectory = "apk://test_data/";

#elif defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
    constexpr string_view applicationTestAssetsDirectory = "hap://test_data/";

#elif defined(_WIN32) || defined(__linux__) || defined(__OHOS_PLATFORM__)
    const string applicationTestAssetsDirectory = "file://" + UTest::GetTestEnv()->appAssetPath;
#endif

    unique_ptr<ShaderManager> shaderMgr = make_unique<ShaderManager>(*device);
    ASSERT_TRUE(shaderMgr != nullptr);
    shaderMgr->SetFileManager(*fileManager);

    // Cast to API
    IShaderManager& iShaderMgr = *shaderMgr;

    // Loading specific shader json.
    {
        const RenderHandleReference testShader =
            iShaderMgr.GetShaderHandle(string_view("test_shaders://shader_manager_test.shader"));
        ASSERT_FALSE(iShaderMgr.IsShader(testShader));
        const RenderHandleReference testPl =
            iShaderMgr.GetPipelineLayoutHandle(string_view("test_pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_FALSE(testPl);
        const RenderHandleReference testShaderState = iShaderMgr.GetGraphicsStateHandle(
            string_view("test_shaderstates://shader_manager_test.shadergs"), string_view("variant"));
        ASSERT_FALSE(testShaderState);
        const RenderHandleReference testVid = iShaderMgr.GetVertexInputDeclarationHandle(
            string_view("test_vertexinputdeclarations://shader_manager_test.shadervid"));
        ASSERT_FALSE(testVid);
    }

    // Register
    {
        fileManager->RegisterPath("test_shaders", applicationTestAssetsDirectory + "separateshaders/shaders", true);
        fileManager->RegisterPath(
            "test_pipelinelayouts", applicationTestAssetsDirectory + "separateshaders/pipelinelayouts", true);
        fileManager->RegisterPath("test_vertexinputdeclarations",
            applicationTestAssetsDirectory + "separateshaders/vertexinputdeclarations", true);
        fileManager->RegisterPath(
            "test_shaderstates", applicationTestAssetsDirectory + "separateshaders/shaderstates", true);
    }
    {
        IShaderManager::ShaderFilePathDesc desc;
        desc.shaderPath = "test_shaders://";
        desc.shaderStatePath = "test_shaderstates://";
        desc.pipelineLayoutPath = "test_pipelinelayouts://";
        desc.vertexInputDeclarationPath = "test_vertexinputdeclarations://";
        iShaderMgr.LoadShaderFiles(desc);
    }
    // check that there are shader data loaded
    {
        const auto res = iShaderMgr.GetShaders();
        ASSERT_TRUE(!res.empty());
    }
    {
        const auto res = iShaderMgr.GetGraphicsStates();
        ASSERT_TRUE(!res.empty());
    }
    {
        const auto res = iShaderMgr.GetPipelineLayouts();
        ASSERT_TRUE(!res.empty());
    }
    {
        const auto res = iShaderMgr.GetVertexInputDeclarations();
        ASSERT_TRUE(!res.empty());
    }

    {
        const RenderHandleReference testShader =
            iShaderMgr.GetShaderHandle(string_view("test_shaders://shader_manager_test.shader"));
        ASSERT_TRUE(iShaderMgr.IsShader(testShader));
        const RenderHandleReference testPl =
            iShaderMgr.GetPipelineLayoutHandle(string_view("test_pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_TRUE(testPl);
        const RenderHandleReference testShaderState = iShaderMgr.GetGraphicsStateHandle(
            string_view("test_shaderstates://shader_manager_test.shadergs"), string_view("variant"));
        ASSERT_TRUE(testShaderState);
        const RenderHandleReference testVid = iShaderMgr.GetVertexInputDeclarationHandle(
            string_view("test_vertexinputdeclarations://shader_manager_test.shadervid"));
        ASSERT_TRUE(testVid);
    }

    shaderMgr->HandlePendingAllocations();

    {
        const RenderHandleReference testShader =
            iShaderMgr.GetShaderHandle(string_view("test_shaders://shader_manager_test.shader"));
        ASSERT_TRUE(iShaderMgr.IsShader(testShader));
        const RenderHandleReference testPl =
            iShaderMgr.GetPipelineLayoutHandle(string_view("test_pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_TRUE(testPl);
        const RenderHandleReference testShaderState = iShaderMgr.GetGraphicsStateHandle(
            string_view("test_shaderstates://shader_manager_test.shadergs"), string_view("variant"));
        ASSERT_TRUE(testShaderState);
        const RenderHandleReference testVid = iShaderMgr.GetVertexInputDeclarationHandle(
            string_view("test_vertexinputdeclarations://shader_manager_test.shadervid"));
        ASSERT_TRUE(testVid);

        // test graphics state flags
        {
            constexpr GraphicsStateFlags compareFlags {
                GraphicsStateFlagBits::CORE_GRAPHICS_STATE_INPUT_ASSEMBLY_BIT |
                GraphicsStateFlagBits::CORE_GRAPHICS_STATE_DEPTH_STENCIL_STATE_BIT
            };

            // get with shader
            {
                const GraphicsStateFlags graphicsStateFlags = iShaderMgr.GetForcedGraphicsStateFlags(testShader);
                ASSERT_TRUE(graphicsStateFlags == compareFlags);
            }
            // get with shader graphics state
            {
                const GraphicsStateFlags graphicsStateFlags = iShaderMgr.GetForcedGraphicsStateFlags(testShaderState);
                ASSERT_TRUE(graphicsStateFlags == compareFlags);
            }
            // get with shader render slot
            {
                const uint32_t renderSlotId = iShaderMgr.GetRenderSlotId(testShaderState);
                const GraphicsStateFlags graphicsStateFlags = iShaderMgr.GetForcedGraphicsStateFlags(renderSlotId);
                ASSERT_TRUE(graphicsStateFlags == compareFlags);
            }
        }

        // Cleanup.
        iShaderMgr.Destroy(testShader);
        iShaderMgr.Destroy(testPl);
        iShaderMgr.Destroy(testShaderState);
        iShaderMgr.Destroy(testVid);
        shaderMgr->HandlePendingAllocations();
    }

    {
        const RenderHandleReference testShader =
            iShaderMgr.GetShaderHandle(string_view("test_shaders://shader_manager_test.shader"));
        ASSERT_FALSE(iShaderMgr.IsShader(testShader));
        const RenderHandleReference testPl =
            iShaderMgr.GetPipelineLayoutHandle(string_view("test_pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_FALSE(testPl);
        const RenderHandleReference testShaderState = iShaderMgr.GetGraphicsStateHandle(
            string_view("test_shaderstates://shader_manager_test.shadergs"), string_view("variant"));
        ASSERT_FALSE(testShaderState);
        const RenderHandleReference testVid = iShaderMgr.GetVertexInputDeclarationHandle(
            string_view("test_vertexinputdeclarations://shader_manager_test.shadervid"));
        ASSERT_FALSE(testVid);
    }
}

void LoadShaderJsonTest(const UTest::EngineResources& engine)
{
    auto device = (Device*)engine.device;
    ASSERT_TRUE(device != nullptr);

    auto* fileManager = &engine.engine->GetFileManager();
    ASSERT_TRUE(fileManager != nullptr);

#if defined(__ANDROID__)

    constexpr string_view applicationTestAssetsDirectory = "apk://test_data/";

#elif defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
    constexpr string_view applicationTestAssetsDirectory = "hap://test_data/";

#elif defined(_WIN32) || defined(__linux__) || defined(__OHOS_PLATFORM__)
    const string applicationTestAssetsDirectory = "file://" + UTest::GetTestEnv()->appAssetPath;
#endif

    unique_ptr<ShaderManager> shaderMgr = make_unique<ShaderManager>(*device);
    ASSERT_TRUE(shaderMgr != nullptr);
    shaderMgr->SetFileManager(*fileManager);

    // Cast to API
    IShaderManager& iShaderMgr = *shaderMgr;

    // Loading specific shader json.
    {
        const RenderHandleReference testShader = iShaderMgr.GetShaderHandle("shaders://shader_manager_test.shader");
        ASSERT_FALSE(iShaderMgr.IsShader(testShader));
    }

    // Register
    {
        fileManager->RegisterPath("shaders", applicationTestAssetsDirectory + "separateshaders/shaders", true);
    }
    iShaderMgr.LoadShaderFile("shaders://shader_manager_test.shader");

    {
        const RenderHandleReference testShader = iShaderMgr.GetShaderHandle("shaders://shader_manager_test.shader");
        ASSERT_TRUE(iShaderMgr.IsShader(testShader));
    }

    shaderMgr->HandlePendingAllocations();

    {
        const RenderHandleReference testShader = iShaderMgr.GetShaderHandle("shaders://shader_manager_test.shader");
        ASSERT_TRUE(iShaderMgr.IsShader(testShader));

        // Cleanup.
        iShaderMgr.Destroy(testShader);
        shaderMgr->HandlePendingAllocations();
    }

    {
        const RenderHandleReference testShader = iShaderMgr.GetShaderHandle("shaders://shader_manager_test.shader");
        ASSERT_FALSE(iShaderMgr.IsShader(testShader));
    }
}

void LoadShaderStateJsonTest(const UTest::EngineResources& engine)
{
    auto device = (Device*)engine.device;
    ASSERT_TRUE(device != nullptr);

    auto* fileManager = &engine.engine->GetFileManager();
    ASSERT_TRUE(fileManager != nullptr);

#if defined(__ANDROID__)

    constexpr string_view applicationTestAssetsDirectory = "apk://test_data/";

#elif defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
    constexpr string_view applicationTestAssetsDirectory = "hap://test_data/";

#elif defined(_WIN32) || defined(__linux__) || defined(__OHOS_PLATFORM__)
    const string applicationTestAssetsDirectory = "file://" + UTest::GetTestEnv()->appAssetPath;
#endif

    unique_ptr<ShaderManager> shaderMgr = make_unique<ShaderManager>(*device);
    ASSERT_TRUE(shaderMgr != nullptr);
    shaderMgr->SetFileManager(*fileManager);

    // Cast to API
    IShaderManager& iShaderMgr = *shaderMgr;

    // Loading specific shader json.
    {
        const RenderHandleReference handle =
            iShaderMgr.GetGraphicsStateHandle("shaderstates://shader_manager_test.shadergs", string_view("variant"));
        ASSERT_FALSE(handle);
    }

    // Register
    {
        fileManager->RegisterPath(
            "shaderstates", applicationTestAssetsDirectory + "separateshaders/shaderstates", true);
    }
    iShaderMgr.LoadShaderFile("shaderstates://shader_manager_test.shadergs");

    {
        const RenderHandleReference handle =
            iShaderMgr.GetGraphicsStateHandle("shaderstates://shader_manager_test.shadergs", string_view("variant"));
        ASSERT_TRUE(handle);
    }

    shaderMgr->HandlePendingAllocations();

    {
        const RenderHandleReference handle =
            iShaderMgr.GetGraphicsStateHandle("shaderstates://shader_manager_test.shadergs", string_view("variant"));
        ASSERT_TRUE(handle);

        // Cleanup.
        iShaderMgr.Destroy(handle);
        shaderMgr->HandlePendingAllocations();
    }

    {
        const RenderHandleReference handle =
            iShaderMgr.GetGraphicsStateHandle("shaderstates://shader_manager_test.shadergs", string_view("variant"));
        ASSERT_FALSE(handle);
    }
}

void LoadPipelineLayoutJsonTest(const UTest::EngineResources& engine)
{
    auto device = (Device*)engine.device;
    ASSERT_TRUE(device != nullptr);

    auto* fileManager = &engine.engine->GetFileManager();
    ASSERT_TRUE(fileManager != nullptr);

#if defined(__ANDROID__)

    constexpr string_view applicationTestAssetsDirectory = "apk://test_data/";

#elif defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
    constexpr string_view applicationTestAssetsDirectory = "hap://test_data/";

#elif defined(_WIN32) || defined(__linux__) || defined(__OHOS_PLATFORM__)
    const string applicationTestAssetsDirectory = "file://" + UTest::GetTestEnv()->appAssetPath;
#endif

    unique_ptr<ShaderManager> shaderMgr = make_unique<ShaderManager>(*device);
    ASSERT_TRUE(shaderMgr != nullptr);
    shaderMgr->SetFileManager(*fileManager);

    // Cast to API
    IShaderManager& iShaderMgr = *shaderMgr;

    // Loading specific shader json.
    {
        const RenderHandleReference handle =
            iShaderMgr.GetPipelineLayoutHandle(string_view("pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_FALSE(handle);
    }

    // Register
    {
        fileManager->RegisterPath(
            "pipelinelayouts", applicationTestAssetsDirectory + "separateshaders/pipelinelayouts", true);
    }
    iShaderMgr.LoadShaderFile("pipelinelayouts://shader_manager_test.shaderpl");

    {
        const RenderHandleReference handle =
            iShaderMgr.GetPipelineLayoutHandle(string_view("pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_TRUE(handle);
    }

    shaderMgr->HandlePendingAllocations();

    {
        const RenderHandleReference handle =
            iShaderMgr.GetPipelineLayoutHandle(string_view("pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_TRUE(handle);

        // Cleanup.
        iShaderMgr.Destroy(handle);
        shaderMgr->HandlePendingAllocations();
    }

    {
        const RenderHandleReference handle =
            iShaderMgr.GetPipelineLayoutHandle(string_view("pipelinelayouts://shader_manager_test.shaderpl"));
        ASSERT_FALSE(handle);
    }
}

void LoadVertexInputDeclarationJsonTest(const UTest::EngineResources& engine)
{
    auto device = (Device*)engine.device;
    ASSERT_TRUE(device != nullptr);

    auto* fileManager = &engine.engine->GetFileManager();
    ASSERT_TRUE(fileManager != nullptr);

#if defined(__ANDROID__)

    constexpr string_view applicationTestAssetsDirectory = "apk://test_data/";

#elif defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
    constexpr string_view applicationTestAssetsDirectory = "hap://test_data/";

#elif defined(_WIN32) || defined(__linux__) || defined(__OHOS_PLATFORM__)
    const string applicationTestAssetsDirectory = "file://" + UTest::GetTestEnv()->appAssetPath;
#endif

    unique_ptr<ShaderManager> shaderMgr = make_unique<ShaderManager>(*device);
    ASSERT_TRUE(shaderMgr != nullptr);
    shaderMgr->SetFileManager(*fileManager);

    // Cast to API
    IShaderManager& iShaderMgr = *shaderMgr;

    // Loading specific shader json.
    {
        const RenderHandleReference handle =
            iShaderMgr.GetVertexInputDeclarationHandle(string_view("test_vids://shader_manager_test.shadervid"));
        ASSERT_FALSE(handle);
    }

    // Register
    {
        fileManager->RegisterPath("vertexinputdeclarations",
            applicationTestAssetsDirectory + "separateshaders/vertexinputdeclarations", true);
    }
    iShaderMgr.LoadShaderFile("vertexinputdeclarations://shader_manager_test.shadervid");

    {
        const RenderHandleReference handle = iShaderMgr.GetVertexInputDeclarationHandle(
            string_view("vertexinputdeclarations://shader_manager_test.shadervid"));
        ASSERT_TRUE(handle);
    }

    shaderMgr->HandlePendingAllocations();

    {
        const RenderHandleReference handle = iShaderMgr.GetVertexInputDeclarationHandle(
            string_view("vertexinputdeclarations://shader_manager_test.shadervid"));
        ASSERT_TRUE(handle);

        // Cleanup.
        iShaderMgr.Destroy(handle);
        shaderMgr->HandlePendingAllocations();
    }

    {
        const RenderHandleReference handle = iShaderMgr.GetVertexInputDeclarationHandle(
            string_view("vertexinputdeclarations://shader_manager_test.shadervid"));
        ASSERT_FALSE(handle);
    }
}

void ShaderPipelineBinderTest(const UTest::EngineResources& engine)
{
    auto device = (Device*)engine.device;
    ASSERT_TRUE(device != nullptr);

    auto* fileManager = &engine.engine->GetFileManager();
    ASSERT_TRUE(fileManager != nullptr);

#if defined(__ANDROID__)

    constexpr string_view applicationTestAssetsDirectory = "apk://test_data/";

#elif defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
    constexpr string_view applicationTestAssetsDirectory = "hap://test_data/";

#elif defined(_WIN32) || defined(__linux__) || defined(__OHOS_PLATFORM__)
    const string applicationTestAssetsDirectory = "file://" + UTest::GetTestEnv()->appAssetPath;
#endif

    unique_ptr<ShaderManager> shaderMgr = make_unique<ShaderManager>(*device);
    ASSERT_TRUE(shaderMgr != nullptr);
    shaderMgr->SetFileManager(*fileManager);

    // Cast to API
    IShaderManager& iShaderMgr = *shaderMgr;

    // Register
    {
        fileManager->RegisterPath("test_shaders", applicationTestAssetsDirectory + "separateshaders/shaders", true);
        fileManager->RegisterPath(
            "test_pipelinelayouts", applicationTestAssetsDirectory + "separateshaders/pipelinelayouts", true);
        fileManager->RegisterPath("test_vertexinputdeclarations",
            applicationTestAssetsDirectory + "separateshaders/vertexinputdeclarations", true);
        fileManager->RegisterPath(
            "test_shaderstates", applicationTestAssetsDirectory + "separateshaders/shaderstates", true);
    }
    {
        IShaderManager::ShaderFilePathDesc desc;
        desc.shaderPath = "test_shaders://";
        desc.shaderStatePath = "test_shaderstates://";
        desc.pipelineLayoutPath = "test_pipelinelayouts://";
        desc.vertexInputDeclarationPath = "test_vertexinputdeclarations://";
        iShaderMgr.LoadShaderFiles(desc);
    }

    // TODO: we should actually bind something
    // TODO: we should have shader that has bindings
    {
        const RenderHandleReference testShader =
            iShaderMgr.GetShaderHandle(string_view("test_shaders://shader_manager_test.shader"));

        auto binderPtr = iShaderMgr.CreateShaderPipelineBinder(testShader);
        ASSERT_TRUE(binderPtr);
        ASSERT_TRUE(binderPtr->GetBindingValidity());
    }

    shaderMgr->HandlePendingAllocations();
}

void Verify(const CORE_NS::json::value& json, const bool value)
{
    EXPECT_TRUE(json.is_boolean());
    EXPECT_EQ(json.boolean_, value);
}

void Verify(const CORE_NS::json::value& json, const char* value)
{
    EXPECT_TRUE(json.is_string());
    EXPECT_EQ(json.string_, value);
}

void Verify(const CORE_NS::json::value& json, const float value)
{
    EXPECT_TRUE(json.is_number());
    EXPECT_FLOAT_EQ(json.as_number<float>(), value);
}

void Verify(const CORE_NS::json::value& json, const uint32_t value)
{
    EXPECT_TRUE(json.is_number());
    EXPECT_EQ(json.as_number<uint32_t>(), value);
}

void SaveShaderJsonTest(const UTest::EngineResources& engine)
{
    auto device = (Device*)engine.device;
    ASSERT_TRUE(device != nullptr);

    auto* fileManager = &engine.engine->GetFileManager();
    ASSERT_TRUE(fileManager != nullptr);

    unique_ptr<ShaderManager> shaderMgr = make_unique<ShaderManager>(*device);
    ASSERT_TRUE(shaderMgr != nullptr);

    shaderMgr->SetFileManager(*fileManager);
    IShaderManager& iShaderMgr = *shaderMgr;

    // lazy mask value (val)
    char* end;
    auto str = "FF00FFFF";
    uint32_t val = strtoul(str, &end, 16);

    // the following values in here are not correct configuration, only something to compare against.
    {
        IShaderManager::ShaderGraphicsStateSaveInfo sgsdsi {};
        GraphicsState state {};
        state.colorBlendState = {};
        uint32_t multiBit =
            ColorComponentFlagBits::CORE_COLOR_COMPONENT_A_BIT | ColorComponentFlagBits::CORE_COLOR_COMPONENT_G_BIT;
        state.colorBlendState.colorAttachmentCount = 1;
        state.colorBlendState.colorAttachments[0] = GraphicsState::ColorBlendState::Attachment { true, multiBit };

        GraphicsState::DepthStencilState& dss = state.depthStencilState;
        dss.backStencilOpState.compareMask = val;
        dss.backStencilOpState.compareOp = CORE_COMPARE_OP_LESS;
        dss.backStencilOpState.depthFailOp = CORE_STENCIL_OP_DECREMENT_AND_CLAMP;
        dss.backStencilOpState.failOp = CORE_STENCIL_OP_ZERO;
        dss.backStencilOpState.passOp = CORE_STENCIL_OP_KEEP;
        dss.backStencilOpState.reference = val;
        dss.backStencilOpState.writeMask = CORE_COLOR_COMPONENT_A_BIT | CORE_COLOR_COMPONENT_R_BIT;
        dss.depthCompareOp = CORE_COMPARE_OP_EQUAL;
        dss.enableDepthBoundsTest = true;
        dss.enableDepthTest = true;
        dss.enableDepthWrite = true;
        dss.enableStencilTest = true;
        dss.frontStencilOpState.compareMask = val;
        dss.frontStencilOpState.compareOp = CORE_COMPARE_OP_ALWAYS;
        dss.frontStencilOpState.depthFailOp = CORE_STENCIL_OP_INVERT;
        dss.frontStencilOpState.failOp = CORE_STENCIL_OP_ZERO;
        dss.frontStencilOpState.passOp = CORE_STENCIL_OP_INVERT;
        dss.frontStencilOpState.reference = val;
        dss.frontStencilOpState.writeMask = CORE_COLOR_COMPONENT_A_BIT | CORE_COLOR_COMPONENT_R_BIT;
        dss.maxDepthBounds = 1.0f;
        dss.minDepthBounds = 0.0f;

        GraphicsState::InputAssembly& ia = state.inputAssembly;
        ia.enablePrimitiveRestart = true;
        ia.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_LINE_LIST;

        GraphicsState::RasterizationState& rs = state.rasterizationState;
        rs.cullModeFlags = CORE_CULL_MODE_BACK_BIT;
        rs.depthBiasClamp = 0.005f;
        rs.depthBiasConstantFactor = 0.05f;
        rs.depthBiasSlopeFactor = 0.1f;
        rs.enableDepthBias = true;
        rs.enableDepthClamp = true;
        rs.enableRasterizerDiscard = true;
        rs.frontFace = CORE_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.2f;
        rs.polygonMode = CORE_POLYGON_MODE_FILL;

        IShaderManager::ShaderStateLoaderVariantData vd {};
        vd.baseShaderState = "baseShaderState";
        vd.baseVariantName = "baseVariantName";
        vd.renderSlot = "renderSlot";
        vd.renderSlotDefaultState = true;
        vd.stateFlags = CORE_GRAPHICS_STATE_INPUT_ASSEMBLY_BIT;
        vd.variantName = "variantName";
        sgsdsi.stateName = "myState";
        sgsdsi.states = { state };
        sgsdsi.stateVariants = { vd };
        auto sgresult = iShaderMgr.SaveShaderGraphicsState(sgsdsi);
        if (!sgresult.success) {
            ADD_FAILURE() << "SaveShaderGraphicsState failed";
        } else {
            const auto& shaderGraphicsStateStr = sgresult.result;
            auto shaderGraphicsStateJson = json::parse(shaderGraphicsStateStr.c_str());
            {
                auto& shaderStatesJson = shaderGraphicsStateJson["shaderStates"];
                if (shaderStatesJson) {
                    if (!shaderStatesJson.is_array()) {
                        ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - Expected array";
                    }

                    auto shaderStateIt = shaderStatesJson.array_.begin();
                    auto& shaderStateJson = *shaderStateIt;
                    if (!shaderStateJson) {
                        ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - Expected 0 index to exist in shader states";
                    } else {
                        Verify(shaderStateJson["baseShaderState"], "baseShaderState");

                        Verify(shaderStateJson["baseVariantName"], "baseVariantName");
                        Verify(shaderStateJson["variantName"], "variantName");
                        Verify(shaderStateJson["renderSlot"], "renderSlot");
                        Verify(shaderStateJson["renderSlotDefaultState"], true);
                        Verify(shaderStateJson["stateFlags"], "input_assembly_bit");

                        auto& stateJson = shaderStateJson["state"];
                        if (!stateJson) {
                            ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - No state";
                        } else {
                            auto& rasterizationState = stateJson["rasterizationState"];
                            if (!rasterizationState) {
                                ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - No rasterizationState";
                            } else {
                                // Check each or assume they exist after rasterizationState?
                                Verify(rasterizationState["enableDepthClamp"], true);
                                Verify(rasterizationState["enableDepthBias"], true);
                                Verify(rasterizationState["enableRasterizerDiscard"], true);
                                Verify(rasterizationState["cullModeFlags"], "back_bit");
                                Verify(rasterizationState["depthBiasClamp"], rs.depthBiasClamp);
                                Verify(rasterizationState["depthBiasConstantFactor"], rs.depthBiasConstantFactor);
                                Verify(rasterizationState["depthBiasSlopeFactor"], rs.depthBiasSlopeFactor);
                                Verify(rasterizationState["lineWidth"], rs.lineWidth);
                                Verify(rasterizationState["frontFace"], "counter_clockwise");
                                Verify(rasterizationState["polygonMode"], "fill");
                            }

                            auto& depthStencilState = stateJson["depthStencilState"];
                            if (!depthStencilState) {
                                ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - No depthStencilState";
                            } else {
                                Verify(depthStencilState["backStencilOpState"]["compareMask"], "FF00FFFF");
                                Verify(depthStencilState["backStencilOpState"]["compareOp"], "less");
                                Verify(depthStencilState["backStencilOpState"]["depthFailOp"], "decrement_and_clamp");

                                Verify(depthStencilState["backStencilOpState"]["failOp"], "zero");
                                Verify(depthStencilState["backStencilOpState"]["passOp"], "keep");
                                Verify(depthStencilState["backStencilOpState"]["reference"], "FF00FFFF");
                                Verify(depthStencilState["backStencilOpState"]["writeMask"], "r_bit|a_bit");

                                Verify(depthStencilState["depthCompareOp"], "equal");
                                Verify(depthStencilState["enableDepthBoundsTest"], true);
                                Verify(depthStencilState["enableDepthTest"], true);
                                Verify(depthStencilState["enableDepthWrite"], true);
                                Verify(depthStencilState["enableStencilTest"], true);
                                Verify(depthStencilState["maxDepthBounds"], dss.maxDepthBounds);
                                Verify(depthStencilState["minDepthBounds"], dss.minDepthBounds);

                                Verify(depthStencilState["frontStencilOpState"]["compareMask"], "FF00FFFF");
                                Verify(depthStencilState["frontStencilOpState"]["compareOp"], "always");
                                Verify(depthStencilState["frontStencilOpState"]["depthFailOp"], "invert");
                                Verify(depthStencilState["frontStencilOpState"]["failOp"], "zero");
                                Verify(depthStencilState["frontStencilOpState"]["passOp"], "invert");
                                Verify(depthStencilState["frontStencilOpState"]["reference"], "FF00FFFF");
                                Verify(depthStencilState["frontStencilOpState"]["writeMask"], "r_bit|a_bit");
                            }

                            auto& colorBlendState = stateJson["colorBlendState"];
                            if (!colorBlendState) {
                                ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - No colorBlendState";
                            } else {
                                auto colorAttachmentsIt = colorBlendState["colorAttachments"].array_.begin();
                                auto& colorAttachmentJson = *colorAttachmentsIt;
                                if (!colorAttachmentJson) {
                                    ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - No colorAttachmentJson";
                                } else {
                                    Verify(colorAttachmentJson["alphaBlendOp"], "add");
                                    Verify(colorAttachmentJson["colorBlendOp"], "add");
                                    Verify(colorAttachmentJson["colorWriteMask"], "g_bit|a_bit");
                                    Verify(colorAttachmentJson["dstAlphaBlendFactor"], "one");
                                    Verify(colorAttachmentJson["dstColorBlendFactor"], "one");
                                    Verify(colorAttachmentJson["enableBlend"], true);
                                    Verify(colorAttachmentJson["srcAlphaBlendFactor"], "one");
                                    Verify(colorAttachmentJson["srcColorBlendFactor"], "one");
                                }

                                // NOTE/TODO: returns array, should be checked.
                                auto& colorBlendConstants = colorBlendState["colorBlendConstants"];
                                Verify(colorBlendState["enableLogicOp"], false);
                                Verify(colorBlendState["logicOp"], "no_op");
                            }

                            auto& inputAssembly = stateJson["inputAssembly"];
                            if (!inputAssembly) {
                                ADD_FAILURE() << "ShaderGraphicsStateDataSaveInfo - No inputAssembly";
                            } else {
                                Verify(inputAssembly["enablePrimitiveRestart"], true);
                                Verify(inputAssembly["primitiveTopology"], "line_list");
                            }
                        }
                    }
                }
            }
        }
    }

    { // vids
        IShaderManager::ShaderVertexInputDeclarationsSaveInfo sviddsi {};
        VertexInputDeclarationView vid {};
        VertexInputDeclaration::VertexInputAttributeDescription attr[] = { {
            1U,                                // location
            1U,                                // binding
            BASE_FORMAT_A1R5G5B5_UNORM_PACK16, // format
            0U,                                // offset
        } };
        vid.attributeDescriptions = { attr };
        VertexInputDeclaration::VertexInputBindingDescription bnd[] = { {
            1U,                            // binding
            4U,                            // stride
            CORE_VERTEX_INPUT_RATE_VERTEX, // vertexInputRate
        } };
        vid.bindingDescriptions = { bnd };
        sviddsi.vidName = "myVid";
        sviddsi.vid = vid;
        auto vidresult = iShaderMgr.SaveShaderVertexInputDeclaration(sviddsi);
        if (!vidresult.success) {
            ADD_FAILURE() << "SaveShaderVertexInputDeclaration failed";
        } else {
            const auto& shaderVertexInputDeclarationStr = vidresult.result;
            auto shaderVertexInputDeclarationJson = json::parse(shaderVertexInputDeclarationStr.c_str());
            if (!shaderVertexInputDeclarationJson) {
                ADD_FAILURE() << "ShaderVertexInputDeclarationsDataSaveInfo - no shaderVertexInputDeclarationJson";
            } else {
                auto& vertexInputStateJson = shaderVertexInputDeclarationJson["vertexInputState"];
                if (!vertexInputStateJson) {
                    ADD_FAILURE() << "ShaderVertexInputDeclarationsDataSaveInfo - no vertexInputStateJson";
                } else {
                    auto& vertexInputBindingDescriptions = vertexInputStateJson["vertexInputBindingDescriptions"];
                    if (!vertexInputBindingDescriptions) {
                        ADD_FAILURE()
                            << "ShaderVertexInputDeclarationsDataSaveInfo - no vertexInputBindingDescriptions";
                    } else {
                        auto descIt = vertexInputBindingDescriptions.array_.begin();
                        auto& descJson = *descIt;
                        Verify(descJson["binding"], 1U);
                        Verify(descJson["stride"], 4U);
                        Verify(descJson["vertexInputRate"], "vertex");
                    }

                    auto& vertexInputAttributeDescriptions = vertexInputStateJson["vertexInputAttributeDescriptions"];
                    if (!vertexInputAttributeDescriptions) {
                        ADD_FAILURE()
                            << "ShaderVertexInputDeclarationsDataSaveInfo - no vertexInputAttributeDescriptions";
                    } else {
                        auto descIt = vertexInputAttributeDescriptions.array_.begin();
                        auto& descJson = *descIt;
                        Verify(descJson["location"], 1U);
                        Verify(descJson["binding"], 1U);
                        Verify(descJson["format"], "a1r5g5b5_unorm_pack16");
                        Verify(descJson["offset"], 0U);
                    }
                }
            }
        }
    }

    { // pipelines
        IShaderManager::ShaderPipelineLayoutSaveInfo spldsi {};
        spldsi.layoutName = "myLayout";

        PipelineLayout& layout = spldsi.layout;
        layout.pushConstant = {};
        DescriptorSetLayout& l = layout.descriptorSetLayouts[0];
        l.set = 1U;
        auto& lob = l.bindings.emplace_back();
        lob.binding = 1U;
        lob.descriptorCount = 1U;
        lob.descriptorType = CORE_DESCRIPTOR_TYPE_SAMPLER;
        lob.shaderStageFlags = CORE_SHADER_STAGE_FRAGMENT_BIT | CORE_SHADER_STAGE_VERTEX_BIT;

        auto plresult = iShaderMgr.SaveShaderPipelineLayout(spldsi);
        if (!plresult.success) {
            ADD_FAILURE() << "SaveShaderPipelineLayout failed";
        } else {
            const auto& shaderPipelineLayoutStr = plresult.result;
            auto shaderPipelineLayoutJson = json::parse(shaderPipelineLayoutStr.c_str());
            if (!shaderPipelineLayoutJson) {
                ADD_FAILURE() << "ShaderPipelineLayoutDataSaveInfo - no shaderPipelineLayoutJson";
            } else {
                auto& descriptorSetLayoutsJson = shaderPipelineLayoutJson["descriptorSetLayouts"];
                if (!descriptorSetLayoutsJson) {
                    ADD_FAILURE() << "ShaderPipelineLayoutDataSaveInfo - no descriptorSetLayoutsJson";
                } else {
                    auto descIt = descriptorSetLayoutsJson.array_.begin();
                    auto& descJson = *descIt;
                    Verify(descJson["set"], 1U);
                    auto& bindings = descJson["bindings"];
                    auto bindingIt = bindings.array_.begin();
                    auto& bindingJson = *bindingIt;
                    Verify(bindingJson["binding"], 1U);
                    Verify(bindingJson["descriptorCount"], 1U);
                    Verify(bindingJson["descriptorType"], "sampler");
                    Verify(bindingJson["shaderStageFlags"], "vertex_bit|fragment_bit");
                }
            }
        }
    }

    { // variants

        GraphicsState state {};

        GraphicsState::ColorBlendState& cbs = state.colorBlendState;
        cbs.colorAttachmentCount = 2;
        cbs.colorAttachments[0] = {};
        cbs.colorAttachments[1] = {};
        cbs.colorBlendConstants[0] = 0.5f;
        cbs.colorBlendConstants[3] = 0.5f;
        cbs.enableLogicOp = true;
        cbs.logicOp = CORE_LOGIC_OP_NOR;

        GraphicsState::DepthStencilState& dss = state.depthStencilState;
        dss.backStencilOpState.compareMask = val;
        dss.backStencilOpState.compareOp = CORE_COMPARE_OP_ALWAYS;
        dss.backStencilOpState.depthFailOp = CORE_STENCIL_OP_DECREMENT_AND_WRAP;
        dss.backStencilOpState.failOp = CORE_STENCIL_OP_ZERO;
        dss.backStencilOpState.passOp = CORE_STENCIL_OP_REPLACE;
        dss.backStencilOpState.reference = val;
        dss.backStencilOpState.writeMask = CORE_COLOR_COMPONENT_A_BIT | CORE_COLOR_COMPONENT_R_BIT;
        dss.depthCompareOp = CORE_COMPARE_OP_GREATER;
        dss.enableDepthBoundsTest = true;
        dss.enableDepthTest = true;
        dss.enableDepthWrite = true;
        dss.enableStencilTest = true;
        dss.frontStencilOpState.compareMask = val;
        dss.frontStencilOpState.compareOp = CORE_COMPARE_OP_LESS;
        dss.frontStencilOpState.depthFailOp = CORE_STENCIL_OP_INVERT;
        dss.frontStencilOpState.failOp = CORE_STENCIL_OP_REPLACE;
        dss.frontStencilOpState.passOp = CORE_STENCIL_OP_INVERT;
        dss.frontStencilOpState.reference = val;
        dss.frontStencilOpState.writeMask = CORE_COLOR_COMPONENT_A_BIT | CORE_COLOR_COMPONENT_R_BIT;
        dss.maxDepthBounds = 1.0f;
        dss.minDepthBounds = 0.0f;

        GraphicsState::InputAssembly& ia = state.inputAssembly;
        ia.enablePrimitiveRestart = true;
        ia.primitiveTopology = CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        GraphicsState::RasterizationState& rs = state.rasterizationState;
        rs.cullModeFlags = CORE_CULL_MODE_NONE;
        rs.depthBiasClamp = 0.5f;
        rs.depthBiasConstantFactor = 1.0f;
        rs.depthBiasSlopeFactor = 0.7f;
        rs.enableDepthBias = true;
        rs.enableDepthClamp = true;
        rs.enableRasterizerDiscard = true;
        rs.frontFace = CORE_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.0f;
        rs.polygonMode = CORE_POLYGON_MODE_LINE;

        IShaderManager::ShaderVariant v1 {};
        v1.shaders.push_back({ "core3d://computerShader.spv", CORE_SHADER_STAGE_COMPUTE_BIT });
        v1.displayName = "myShader";
        v1.shaders.push_back({ "core3d://fragmentShader.spv", CORE_SHADER_STAGE_FRAGMENT_BIT });
        v1.graphicsState = state;
        v1.materialMetadata = "just a test string";
        v1.pipelineLayout = "pipelineLayout";
        v1.renderSlot = "renderSlot";
        v1.renderSlotDefaultShader = true;
        v1.shaderFileStr = ""; // NO?
        v1.stateFlags = { CORE_GRAPHICS_STATE_COLOR_BLEND_STATE_BIT };
        v1.variantName = "myVariant";
        v1.vertexInputDeclaration = "verticesUri";
        v1.shaders.push_back({ "core3d://vertexShader.spv", CORE_SHADER_STAGE_VERTEX_BIT });
        v1.graphicsState.depthStencilState.backStencilOpState.compareMask = val;

        IShaderManager::ShaderVariant v2 {};
        v2.displayName = "myShader2";
        v2.variantName = "myVariant2";
        GraphicsState& state2 = v2.graphicsState;
        GraphicsState::ColorBlendState& cbs2 = state2.colorBlendState;
        cbs2.colorAttachmentCount = 1;
        cbs2.colorAttachments[0] = {};
        cbs2.colorBlendConstants[0] = 0.5f;
        cbs2.colorBlendConstants[3] = 0.5f;
        cbs2.enableLogicOp = true;
        cbs2.logicOp = CORE_LOGIC_OP_NOR;

        IShaderManager::ShaderVariantsSaveInfo svdsi {};
        svdsi.category = "3D/Material";
        svdsi.shaders.push_back(v1);
        svdsi.shaders.push_back(v2);
        svdsi.shaderName = "myShader";
        auto shaderresult = iShaderMgr.SaveShaderVariants(svdsi);
        if (!shaderresult.success) {
            ADD_FAILURE() << "SaveShaderVariants failed";
        } else {
            const auto& shaderVariantsStr = shaderresult.result;
            auto shaderVariantsJson = json::parse(shaderVariantsStr.c_str());
            if (!shaderVariantsJson) {
                ADD_FAILURE() << "ShaderVariantsDataSaveInfo - no shaderVariantsJson";
            } else {
                Verify(shaderVariantsJson["category"], "3D/Material");
                Verify(shaderVariantsJson["baseShader"], "");
                auto& shadersJson = shaderVariantsJson["shaders"];
                auto shader1It = shadersJson.array_.begin();
                auto& shader1Json = *shader1It;
                if (!shader1Json) {
                    ADD_FAILURE() << "ShaderVariantsDataSaveInfo - no shader1Json";
                } else { // HEHE
                    Verify(shader1Json["materialMetadata"], "just a test string");
                    Verify(shader1Json["displayName"], "myShader");
                    Verify(shader1Json["variantName"], "myVariant");
                    Verify(shader1Json["renderSlot"], "renderSlot");
                    Verify(shader1Json["renderSlotDefaultShader"], true);
                    Verify(shader1Json["vert"], "core3d://vertexShader.spv");
                    Verify(shader1Json["frag"], "core3d://fragmentShader.spv");
                    Verify(shader1Json["comp"], "core3d://computerShader.spv");
                    Verify(shader1Json["vertexInputDeclaration"], "verticesUri");
                    Verify(shader1Json["pipelineLayout"], "pipelineLayout");

                    auto& stateJson = shader1Json["state"];
                    if (!stateJson) {
                        ADD_FAILURE() << "ShaderVariantsDataSaveInfo - no stateJson";
                    } else {
                        auto& rasterizationState = stateJson["rasterizationState"];
                        if (!rasterizationState) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No rasterizationState";
                        } else {
                            // Check each or assume they exist after rasterizationState?
                            Verify(rasterizationState["enableDepthClamp"], true);
                            Verify(rasterizationState["enableDepthBias"], true);
                            Verify(rasterizationState["enableRasterizerDiscard"], true);
                            Verify(rasterizationState["cullModeFlags"], "none");
                            Verify(rasterizationState["depthBiasClamp"], rs.depthBiasClamp);
                            Verify(rasterizationState["depthBiasConstantFactor"], rs.depthBiasConstantFactor);
                            Verify(rasterizationState["depthBiasSlopeFactor"], rs.depthBiasSlopeFactor);
                            Verify(rasterizationState["lineWidth"], rs.lineWidth);
                            Verify(rasterizationState["frontFace"], "counter_clockwise");
                            Verify(rasterizationState["polygonMode"], "line");
                        }

                        auto& depthStencilState = stateJson["depthStencilState"];
                        if (!depthStencilState) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No depthStencilState";
                        } else {
                            Verify(depthStencilState["backStencilOpState"]["compareMask"], "FF00FFFF");
                            Verify(depthStencilState["backStencilOpState"]["compareOp"], "always");
                            Verify(depthStencilState["backStencilOpState"]["depthFailOp"], "decrement_and_wrap");

                            Verify(depthStencilState["backStencilOpState"]["failOp"], "zero");
                            Verify(depthStencilState["backStencilOpState"]["passOp"], "replace");
                            Verify(depthStencilState["backStencilOpState"]["reference"], "FF00FFFF");
                            Verify(depthStencilState["backStencilOpState"]["writeMask"], "r_bit|a_bit");

                            Verify(depthStencilState["depthCompareOp"], "greater");
                            Verify(depthStencilState["enableDepthBoundsTest"], true);
                            Verify(depthStencilState["enableDepthTest"], true);
                            Verify(depthStencilState["enableDepthWrite"], true);
                            Verify(depthStencilState["enableStencilTest"], true);
                            Verify(depthStencilState["maxDepthBounds"], dss.maxDepthBounds);
                            Verify(depthStencilState["minDepthBounds"], dss.minDepthBounds);

                            Verify(depthStencilState["frontStencilOpState"]["compareMask"], "FF00FFFF");
                            Verify(depthStencilState["frontStencilOpState"]["compareOp"], "less");
                            Verify(depthStencilState["frontStencilOpState"]["depthFailOp"], "invert");
                            Verify(depthStencilState["frontStencilOpState"]["failOp"], "replace");
                            Verify(depthStencilState["frontStencilOpState"]["passOp"], "invert");
                            Verify(depthStencilState["frontStencilOpState"]["reference"], "FF00FFFF");
                            Verify(depthStencilState["frontStencilOpState"]["writeMask"], "r_bit|a_bit");
                        }

                        auto& colorBlendState = stateJson["colorBlendState"];
                        if (!colorBlendState) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No colorBlendState";
                        } else {
                            auto colorAttachmentsIt = colorBlendState["colorAttachments"].array_.begin();
                            auto& colorAttachmentJson = *colorAttachmentsIt;
                            if (!colorAttachmentJson) {
                                ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No colorAttachmentJson";
                            } else {
                                Verify(colorAttachmentJson["alphaBlendOp"], "add");
                                Verify(colorAttachmentJson["colorBlendOp"], "add");
                                Verify(colorAttachmentJson["colorWriteMask"], "r_bit|g_bit|b_bit|a_bit");
                                Verify(colorAttachmentJson["dstAlphaBlendFactor"], "one");
                                Verify(colorAttachmentJson["dstColorBlendFactor"], "one");
                                Verify(colorAttachmentJson["enableBlend"], false);
                                Verify(colorAttachmentJson["srcAlphaBlendFactor"], "one");
                                Verify(colorAttachmentJson["srcColorBlendFactor"], "one");
                            }

                            // NOTE/TODO: returns array, should be checked.
                            auto& colorBlendConstants = colorBlendState["colorBlendConstants"];
                            Verify(colorBlendState["enableLogicOp"], true);
                            Verify(colorBlendState["logicOp"], "nor");
                        }

                        auto& inputAssembly = stateJson["inputAssembly"];
                        if (!inputAssembly) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No inputAssembly";
                        } else {
                            Verify(inputAssembly["enablePrimitiveRestart"], true);
                            Verify(inputAssembly["primitiveTopology"], "triangle_list");
                        }
                    }
                }

                auto shader2It = shadersJson.array_.begin() + 1;
                auto& shader2Json = *shader2It;
                if (!shader2Json) {
                    ADD_FAILURE() << "ShaderVariantsDataSaveInfo - no shader2Json";
                } else {
                    Verify(shader2Json["materialMetadata"], "");
                    Verify(shader2Json["displayName"], "myShader2");
                    Verify(shader2Json["variantName"], "myVariant2");
                    Verify(shader2Json["renderSlot"], "");
                    Verify(shader2Json["renderSlotDefaultShader"], false);

                    auto& s2vert = shader2Json["vert"];
                    EXPECT_FALSE(s2vert);
                    auto& s2frag = shader2Json["frag"];
                    EXPECT_FALSE(s2frag);
                    auto& s2comp = shader2Json["comp"];
                    EXPECT_FALSE(s2comp);
                    Verify(shader2Json["vertexInputDeclaration"], "");
                    Verify(shader2Json["pipelineLayout"], "");

                    auto& stateJson = shader2Json["state"];
                    if (!stateJson) {
                        ADD_FAILURE() << "ShaderVariantsDataSaveInfo - no stateJson";
                    } else {
                        auto& rasterizationState = stateJson["rasterizationState"];
                        if (!rasterizationState) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No rasterizationState";
                        } else {
                            // Check each or assume they exist after rasterizationState?
                            Verify(rasterizationState["enableDepthClamp"], false);
                            Verify(rasterizationState["enableDepthBias"], false);
                            Verify(rasterizationState["enableRasterizerDiscard"], false);
                            Verify(rasterizationState["cullModeFlags"], "none");
                            Verify(rasterizationState["depthBiasClamp"], 0.f);
                            Verify(rasterizationState["depthBiasConstantFactor"], 0.f);
                            Verify(rasterizationState["depthBiasSlopeFactor"], 0.f);
                            Verify(rasterizationState["lineWidth"], rs.lineWidth);
                            Verify(rasterizationState["frontFace"], "counter_clockwise");
                            Verify(rasterizationState["polygonMode"], "fill");
                        }

                        auto& depthStencilState = stateJson["depthStencilState"];
                        if (!depthStencilState) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No depthStencilState";
                        } else {
                            Verify(depthStencilState["backStencilOpState"]["compareMask"], "0");
                            Verify(depthStencilState["backStencilOpState"]["compareOp"], "never");
                            Verify(depthStencilState["backStencilOpState"]["depthFailOp"], "keep");

                            Verify(depthStencilState["backStencilOpState"]["failOp"], "keep");
                            Verify(depthStencilState["backStencilOpState"]["passOp"], "keep");
                            Verify(depthStencilState["backStencilOpState"]["reference"], "0");
                            Verify(depthStencilState["backStencilOpState"]["writeMask"], "");

                            Verify(depthStencilState["depthCompareOp"], "never");
                            Verify(depthStencilState["enableDepthBoundsTest"], false);
                            Verify(depthStencilState["enableDepthTest"], false);
                            Verify(depthStencilState["enableDepthWrite"], false);
                            Verify(depthStencilState["enableStencilTest"], false);
                            Verify(depthStencilState["maxDepthBounds"], 1.f);
                            Verify(depthStencilState["minDepthBounds"], 0.f);

                            Verify(depthStencilState["frontStencilOpState"]["compareMask"], "0");
                            Verify(depthStencilState["frontStencilOpState"]["compareOp"], "never");
                            Verify(depthStencilState["frontStencilOpState"]["depthFailOp"], "keep");
                            Verify(depthStencilState["frontStencilOpState"]["failOp"], "keep");
                            Verify(depthStencilState["frontStencilOpState"]["passOp"], "keep");
                            Verify(depthStencilState["frontStencilOpState"]["reference"], "0");
                            Verify(depthStencilState["frontStencilOpState"]["writeMask"], "");
                        }

                        auto& colorBlendState = stateJson["colorBlendState"];
                        if (!colorBlendState) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No colorBlendState";
                        } else {
                            auto colorAttachmentsIt = colorBlendState["colorAttachments"].array_.begin();
                            auto& colorAttachmentJson = *colorAttachmentsIt;
                            if (!colorAttachmentJson) {
                                ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No colorAttachmentJson";
                            } else {
                                Verify(colorAttachmentJson["alphaBlendOp"], "add");
                                Verify(colorAttachmentJson["colorBlendOp"], "add");
                                Verify(colorAttachmentJson["colorWriteMask"], "r_bit|g_bit|b_bit|a_bit");
                                Verify(colorAttachmentJson["dstAlphaBlendFactor"], "one");
                                Verify(colorAttachmentJson["dstColorBlendFactor"], "one");
                                Verify(colorAttachmentJson["enableBlend"], false);
                                Verify(colorAttachmentJson["srcAlphaBlendFactor"], "one");
                                Verify(colorAttachmentJson["srcColorBlendFactor"], "one");
                            }

                            // NOTE/TODO: returns array, should be checked.
                            auto& colorBlendConstants = colorBlendState["colorBlendConstants"];
                            Verify(colorBlendState["enableLogicOp"], true);
                            Verify(colorBlendState["logicOp"], "nor");
                        }

                        auto& inputAssembly = stateJson["inputAssembly"];
                        if (!inputAssembly) {
                            ADD_FAILURE() << "ShaderVariantsDataSaveInfo - No inputAssembly";
                        } else {
                            Verify(inputAssembly["enablePrimitiveRestart"], false);
                            Verify(inputAssembly["primitiveTopology"], "triangle_list");
                        }
                    }
                }
            }
        }
    }
}

/**
 * @tc.name: AllShaderTests
 * @tc.desc: Tests for ShaderManager. This test contains testing for loading shader data via files (.shader, .shaderpl,
 * .shadervid), which includes deserializing and serializing shader, shader state, pipeline layout, and vertex input
 * declarations from/to json. Also tests the creation of RenderHandles by loading shader files and querying for the
 * handles from the ShaderManager.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderManager, AllShaderTests, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    if (engine.backend == DeviceBackendType::OPENGL) {
        engine.createWindow = true;
    }
    UTest::CreateEngineSetup(engine);

    LoadShaderDataTest(engine);
    LoadShaderJsonTest(engine);
    LoadShaderStateJsonTest(engine);
    LoadPipelineLayoutJsonTest(engine);
    LoadVertexInputDeclarationJsonTest(engine);

    SaveShaderJsonTest(engine);

    UTest::DestroyEngine(engine);
}
