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

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#include <gles/spirv_cross_helper_structs_gles.h>
#include <gles/spirv_cross_helpers_gles.h>
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: SpirvCrossHelpersTestOpenGL
 * @tc.desc: Tests SPIR-V cross helpers for GLES by testing inserting defines, shader specializations and push constant
 * reflection.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_SpirvCrossHelpers, SpirvCrossHelpersTestOpenGL, testing::ext::TestSize.Level1)
{
    {
        auto shaderSrc = Gles::Specialize(0, "", {}, {});
        ASSERT_TRUE(shaderSrc.empty());
    }
    {
        string inSrc = "";
        string defines = "#define PI 3.14f\n";
        string outSrc = Gles::InsertDefines(inSrc, defines);
        ASSERT_EQ(defines, outSrc);
    }
    {
        ShaderSpecialization::Constant constants[2];
        constants[0].id = 0u;
        constants[0].offset = 0u;
        constants[0].type = ShaderSpecialization::Constant::Type::INT32;
        constants[1].id = 1u;
        constants[1].offset = 0u;
        constants[1].type = ShaderSpecialization::Constant::Type::FLOAT;
        {
            string result = "";
            int intValue = 6;
            bool ok = Gles::DefineForSpec(
                { constants, countof(constants) }, 0u, reinterpret_cast<uintptr_t>(&intValue), result);
            ASSERT_TRUE(ok);
            ASSERT_EQ("#define SPIRV_CROSS_CONSTANT_ID_0 6\n", result);
        }
        {
            string result = "";
            float floatValue = 6.2f;
            bool ok = Gles::DefineForSpec(
                { constants, countof(constants) }, 1u, reinterpret_cast<uintptr_t>(&floatValue), result);
            ASSERT_TRUE(ok);
            string expected = "#define SPIRV_CROSS_CONSTANT_ID_1 6.2";
            ASSERT_LE(expected.size(), result.size());
            ASSERT_EQ(expected, result.substr(0u, expected.size()));
        }
    }
    {
        Gles::PushConstantReflection reflection;
        reflection.name = "name0";
        reflection.type = 0u;
        reflection.offset = 0u;
        reflection.size = 0u;
        reflection.arraySize = 0u;
        reflection.arrayStride = 0u;
        reflection.matrixStride = 0u;

        Gles::PushConstantReflection reflection2 = reflection;
        reflection2.name = "name1";
        ASSERT_EQ(-1, Gles::FindConstant({ &reflection, 1 }, reflection2));

        reflection2.name = "name0";
        ASSERT_EQ(0, Gles::FindConstant({ &reflection, 1 }, reflection2));

        reflection2.matrixStride = 1u;
        ASSERT_EQ(-2, Gles::FindConstant({ &reflection, 1 }, reflection2));

        reflection2.arrayStride = 1u;
        ASSERT_EQ(-2, Gles::FindConstant({ &reflection, 1 }, reflection2));

        reflection2.arraySize = 1u;
        ASSERT_EQ(-2, Gles::FindConstant({ &reflection, 1 }, reflection2));

        reflection2.size = 1u;
        ASSERT_EQ(-2, Gles::FindConstant({ &reflection, 1 }, reflection2));

        reflection2.offset = 1u;
        ASSERT_EQ(-2, Gles::FindConstant({ &reflection, 1 }, reflection2));

        reflection2.type = 1u;
        ASSERT_EQ(-2, Gles::FindConstant({ &reflection, 1 }, reflection2));
    }
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
