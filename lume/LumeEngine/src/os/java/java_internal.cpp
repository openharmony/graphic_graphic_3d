/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
