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

#include <gtest/gtest.h>
#include <memory>

#include "offscreen_context_helper.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class OffScreenContextHelperUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// ============================================================================
// Tests for CreateOffScreenContext and GetOffScreenContext
// ============================================================================

/**
 * @tc.name: CreateOffScreenContext_DefaultReturnsNoContext
 * @tc.desc: test GetOffScreenContext returns EGL_NO_CONTEXT before creation
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, CreateOffScreenContext_DefaultReturnsNoContext, TestSize.Level1)
{
    WIDGET_LOGD("CreateOffScreenContext_DefaultReturnsNoContext");
    OffScreenContextHelper helper;

    EGLContext context = helper.GetOffScreenContext();

    EXPECT_EQ(context, EGL_NO_CONTEXT);
}

/**
 * @tc.name: CreateOffScreenContext_CalledTwiceReturnsSameContext
 * @tc.desc: test CreateOffScreenContext returns same context on second call
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, CreateOffScreenContext_CalledTwiceReturnsSameContext, TestSize.Level1)
{
    WIDGET_LOGD("CreateOffScreenContext_CalledTwiceReturnsSameContext");
    OffScreenContextHelper helper;

    EGLContext context1 = helper.CreateOffScreenContext(EGL_NO_CONTEXT);
    EGLContext context2 = helper.CreateOffScreenContext(EGL_NO_CONTEXT);

    EXPECT_EQ(context1, context2);
}

/**
 * @tc.name: CreateOffScreenContext_MultipleHelpersIndependent
 * @tc.desc: test multiple OffScreenContextHelper instances are independent
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, CreateOffScreenContext_MultipleHelpersIndependent, TestSize.Level1)
{
    WIDGET_LOGD("CreateOffScreenContext_MultipleHelpersIndependent");
    OffScreenContextHelper helper1;
    OffScreenContextHelper helper2;

    EGLContext context1 = helper1.CreateOffScreenContext(EGL_NO_CONTEXT);
    EGLContext context2 = helper2.CreateOffScreenContext(EGL_NO_CONTEXT);

    EXPECT_NE(context1, EGL_NO_CONTEXT);
    EXPECT_NE(context2, EGL_NO_CONTEXT);
    EXPECT_NE(context1, context2);
}

/**
 * @tc.name: DestroyOffScreenContext_ResetsContext
 * @tc.desc: test DestroyOffScreenContext resets context to EGL_NO_CONTEXT
 * @tc.type: FUNC
    */
HWTEST_F(OffScreenContextHelperUT, DestroyOffScreenContext_ResetsContext, TestSize.Level1)
{
    WIDGET_LOGD("DestroyOffScreenContext_ResetsContext");
    OffScreenContextHelper helper;

    helper.CreateOffScreenContext(EGL_NO_CONTEXT);
    EGLContext contextBefore = helper.GetOffScreenContext();
    EXPECT_NE(contextBefore, EGL_NO_CONTEXT);

    helper.DestroyOffScreenContext();
    EGLContext contextAfter = helper.GetOffScreenContext();
    EXPECT_EQ(contextAfter, EGL_NO_CONTEXT);
}

/**
 * @tc.name: DestroyOffScreenContext_CalledTwiceNoCrash
 * @tc.desc: test DestroyOffScreenContext can be called multiple times
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, DestroyOffScreenContext_CalledTwiceNoCrash, TestSize.Level1)
{
    WIDGET_LOGD("DestroyOffScreenContext_CalledTwiceNoCrash");
    OffScreenContextHelper helper;

    helper.CreateOffScreenContext(EGL_NO_CONTEXT);
    helper.DestroyOffScreenContext();
    helper.DestroyOffScreenContext();

    EXPECT_EQ(helper.GetOffScreenContext(), EGL_NO_CONTEXT);
}

/**
 * @tc.name: FullLifecycle_CreateDestroyCreate
 * @tc.desc: test full lifecycle: create, destroy, create again
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, FullLifecycle_CreateDestroyCreate, TestSize.Level1)
{
    WIDGET_LOGD("FullLifecycle_CreateDestroyCreate");
    OffScreenContextHelper helper;

    EGLContext context1 = helper.CreateOffScreenContext(EGL_NO_CONTEXT);
    EXPECT_NE(context1, EGL_NO_CONTEXT);

    helper.DestroyOffScreenContext();
    EXPECT_EQ(helper.GetOffScreenContext(), EGL_NO_CONTEXT);

    EGLContext context2 = helper.CreateOffScreenContext(EGL_NO_CONTEXT);
    EXPECT_NE(context2, EGL_NO_CONTEXT);
}

/**
 * @tc.name: BindOffScreenContext_AfterCreate
 * @tc.desc: test BindOffScreenContext after context creation
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, BindOffScreenContext_AfterCreate, TestSize.Level1)
{
    WIDGET_LOGD("BindOffScreenContext_AfterCreate");
    OffScreenContextHelper helper;

    helper.CreateOffScreenContext(EGL_NO_CONTEXT);
    EGLContext contextBefore = helper.GetOffScreenContext();
    EXPECT_NE(contextBefore, EGL_NO_CONTEXT);

    helper.BindOffScreenContext();
    EGLContext contextAfter = helper.GetOffScreenContext();
    EXPECT_EQ(contextAfter, contextBefore);
}

/**
 * @tc.name: BindOffScreenContext_BeforeCreate
 * @tc.desc: test BindOffScreenContext before context creation
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, BindOffScreenContext_BeforeCreate, TestSize.Level1)
{
    WIDGET_LOGD("BindOffScreenContext_BeforeCreate");
    OffScreenContextHelper helper;

    helper.BindOffScreenContext();
    EXPECT_EQ(helper.GetOffScreenContext(), EGL_NO_CONTEXT);
}

/**
 * @tc.name: AutoRestore_CreatesContext
 * @tc.desc: test AutoRestore creates context
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, AutoRestore_CreatesContext, TestSize.Level1)
{
    WIDGET_LOGD("AutoRestore_CreatesContext");
    OffScreenContextHelper helper;

    helper.CreateOffScreenContext(EGL_NO_CONTEXT);
    EGLContext contextBefore = helper.GetOffScreenContext();

    {
        AutoRestore restore;
        EGLContext contextDuring = helper.GetOffScreenContext();
        EXPECT_NE(contextDuring, EGL_NO_CONTEXT);
    }

    EGLContext contextAfter = helper.GetOffScreenContext();
    EXPECT_EQ(contextAfter, contextBefore);
}

/**
 * @tc.name: AutoRestore_NestedScope
 * @tc.desc: test nested AutoRestore scopes
 * @tc.type: FUNC
 */
HWTEST_F(OffScreenContextHelperUT, AutoRestore_NestedScope, TestSize.Level1)
{
    WIDGET_LOGD("AutoRestore_NestedScope");
    OffScreenContextHelper helper;

    helper.CreateOffScreenContext(EGL_NO_CONTEXT);

    {
        AutoRestore restore1;
        {
            AutoRestore restore2;
            EGLContext context = helper.GetOffScreenContext();
            EXPECT_NE(context, EGL_NO_CONTEXT);
        }
        EGLContext context = helper.GetOffScreenContext();
        EXPECT_NE(context, EGL_NO_CONTEXT);
    }

    EGLContext context = helper.GetOffScreenContext();
    EXPECT_NE(context, EGL_NO_CONTEXT);
}

} // namespace OHOS::Render3D
