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

#include "java_internal.h"

#include <core/log.h>

namespace java_internal {
namespace {
JavaVM* sJavaVM = nullptr;

extern "C" {
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    CORE_LOG_V("JNI_OnLoad");
    sJavaVM = vm;
    return JNI_VERSION_1_6;
}
}
} // unnamed namespace

JavaVM* GetJavaVM()
{
    return sJavaVM;
}

void SetJavaVM(JavaVM* jvm)
{
    CORE_ASSERT(jvm);
    sJavaVM = jvm;
}

JNIEnv* GetJavaEnv()
{
    CORE_ASSERT(java_internal::sJavaVM);
    if (!java_internal::sJavaVM) {
        CORE_ASSERT_MSG(true, "JNI ERROR: sJavaVM not initialized.");
        return nullptr;
    }

    JNIEnv* env = nullptr;
    jint result = sJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);

    switch (result) {
        case JNI_OK: {
            return env;
        }

        case JNI_EDETACHED: {
            CORE_ASSERT_MSG(true, "JNI ERROR: Thread not attached.");
            return nullptr;
        }

        case JNI_EVERSION: {
            CORE_ASSERT_MSG(true, "JNI ERROR: Invalid version.");
            return nullptr;
        }

        default: {
            CORE_ASSERT(true);
            return nullptr;
        }
    }
}
} // namespace java_internal
