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

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <core/property/property_handle_util.h>
#include <render/device/intf_shader_manager.h>

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BASE_NS::Math::Vec4);
CORE_END_NAMESPACE()

namespace {
void ShaderCustomPropertiesTest(CORE3D_NS::UTest::TestResources& res)
{
    // requirements
    IMaterialComponentManager* materialManager = GetManager<IMaterialComponentManager>(res.GetEcs());
    IRenderHandleComponentManager* renderHandleManager = GetManager<IRenderHandleComponentManager>(res.GetEcs());
    auto& renderContext = res.GetRenderContext();
    IShaderManager& shaderManager = renderContext.GetDevice().GetShaderManager();
    shaderManager.LoadShaderFile("test://shader/custom_shader.shader");
    shaderManager.LoadShaderFile("test://shader/custom_shader2.shader");
    auto shaderHandle = shaderManager.GetShaderHandle("test://shader/custom_shader.shader");
    auto& entityManager = res.GetEcs().GetEntityManager();

    // save the frame index for our shader, it will be used for checking that 'this' shader is the same but with
    // modifications. (requires tick and render, search pos "iiii")
    auto testedFrameIndex = shaderManager.GetFrameIndex(shaderHandle);

    // material with custom shader
    auto materialEntity = entityManager.CreateReferenceCounted();
    MaterialComponent materialComponent;
    materialComponent.materialShader.shader =
        GetOrCreateEntityReference(entityManager, *renderHandleManager, shaderHandle);
    materialManager->Set(materialEntity, materialComponent);

    // when shader is set, after that the .shader containing custom properties should be found from .customProperties
    if (auto materialHandle = materialManager->Read(materialEntity); materialHandle) {
        if (materialHandle->customProperties) {
            Math::Vec4 expectance {};
            expectance = GetPropertyValue<Math::Vec4>(materialHandle->customProperties, "test");
            EXPECT_FLOAT_EQ(expectance.x, 0.1f);
            EXPECT_FLOAT_EQ(expectance.y, 0.2f);
            EXPECT_FLOAT_EQ(expectance.z, 0.3f);
            EXPECT_FLOAT_EQ(expectance.w, 0.4f);
        } else {
            ADD_FAILURE() << "Was not able to read test values from custom_shader.shader";
            return;
        }
    } else {
        ADD_FAILURE() << "Was not able to open material for reading";
        return;
    }

    Math::Vec4 newValues { 1, 2, 3, 4 };
    // setting same shader again, should keep the values what has been set in the .customProperties
    if (auto materialHandle = materialManager->Write(materialEntity); materialHandle) {
        if (materialHandle->customProperties) {
            // first write the material...
            SetPropertyValue(materialHandle->customProperties, "test", newValues);
        } else {
            ADD_FAILURE() << "Was not able to set custom properties";
            return;
        }
    } else {
        ADD_FAILURE() << "Was not able to open material for editing";
        return;
    }
    // .. then set the shader and check that the old set was still there.
    res.TickTest(1); // search pos "iiii"
    shaderManager.LoadShaderFile("test://shader/custom_shader.shader");
    shaderHandle = shaderManager.GetShaderHandle("test://shader/custom_shader.shader");
    materialComponent.materialShader.shader =
        GetOrCreateEntityReference(entityManager, *renderHandleManager, shaderHandle);
    materialManager->Set(materialEntity, materialComponent);

    //  now when the shader is reset, we can check that the values that were set remain the same
    if (auto materialHandle = materialManager->Read(materialEntity); materialHandle) {
        if (materialHandle->customProperties) {
            auto val = GetPropertyValue<Math::Vec4>(materialHandle->customProperties, "test");
            EXPECT_FLOAT_EQ(val.x, newValues.x);
            EXPECT_FLOAT_EQ(val.y, newValues.y);
            EXPECT_FLOAT_EQ(val.z, newValues.z);
            EXPECT_FLOAT_EQ(val.w, newValues.w);
        } else {
            ADD_FAILURE() << "Was not able to check against read values and expectance";
            return;
        }
    } else {
        ADD_FAILURE() << "Was not able to re-open material for reading";
        return;
    }

    // should be greater when its the same shader.
    // (fyi, same shader is when it has been only modified in some way)
    EXPECT_NE(testedFrameIndex, shaderManager.GetFrameIndex(shaderHandle));

    // setting another shader which has custom properties with same name should not sync the values
    shaderHandle = shaderManager.GetShaderHandle("test://shader/custom_shader2.shader");
    materialComponent.materialShader.shader =
        GetOrCreateEntityReference(entityManager, *renderHandleManager, shaderHandle);
    materialManager->Set(materialEntity, materialComponent);

    //  it is a different shader so values should be what was in the .shader file. (0.6, 0.7, 0.8, 0.9)
    if (auto materialHandle = materialManager->Read(materialEntity); materialHandle) {
        if (materialHandle->customProperties) {
            auto val = GetPropertyValue<Math::Vec4>(materialHandle->customProperties, "test");
            EXPECT_FLOAT_EQ(val.x, 0.6f);
            EXPECT_FLOAT_EQ(val.y, 0.7f);
            EXPECT_FLOAT_EQ(val.z, 0.8f);
            EXPECT_FLOAT_EQ(val.w, 0.9f);
        } else {
            ADD_FAILURE() << "Was not able to compare values of changed shader";
            return;
        }
    } else {
        ADD_FAILURE() << "Was not able to open material with different shader for read";
        return;
    }
}
}; // namespace

/**
 * @tc.name: ShaderCustomProperties
 * @tc.desc: Tests for Shader Custom Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, ShaderCustomProperties, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(200U, 100U, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));

    ShaderCustomPropertiesTest(res);

    res.ShutdownTest();
}