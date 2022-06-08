/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_OS_JAVA_JAVA_INTERNAL_H
#define CORE_OS_JAVA_JAVA_INTERNAL_H

#ifdef PLATFORM_HAS_JAVA

#include <jni.h>

namespace java_internal {
JavaVM* GetJavaVM();
void SetJavaVM(JavaVM* jvm);
JNIEnv* GetJavaEnv();
} // namespace java_internal

#endif

#endif // CORE_OS_JAVA_JAVA_INTERNAL_H
