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

#ifndef TEST_LOG_UTIL
#define TEST_LOG_UTIL

#include <cassert>
#include <fstream>
#include <string>

#include <core/log.h>
#include <core/namespace.h>

#if defined(__ANDROID__)
#include <android/log.h>
#include <fcntl.h>  // O_* constants
#include <pthread.h>
#include <unistd.h>
#endif  // defined(__ANDROID__)

#if defined(__OHOS__)
#include <fcntl.h>  // O_* constants
#include <hilog/log.h>
#include <unistd.h>
#define LOG_DOMAIN 0x0000
#endif  // defined(__OHOS__)

#define LOG_TAG "core3d_unit_test"

namespace Test {

/**
 * LogLevelScope enables RAII style log level scoping. The previous log level will be restored
 * when the LogLevelScope object goes out of scope.
 */
class LogLevelScope {
public:
    LogLevelScope(CORE_NS::ILogger* logger, CORE_NS::ILogger::LogLevel logLevel);
    ~LogLevelScope();

private:
    CORE_NS::ILogger* logger_;
    CORE_NS::ILogger::LogLevel oldLevel_;
};

//
// Android log support
//
#if defined(__ANDROID__)

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#define LOGF(...) ((void)__android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__))

void StartOutputFileRedirect(const std::string& filePath, bool append);
void StopOutputFileRedirect();
void PrintLogFromFile(const std::string& filePath);
int RunLoggingThread();
#endif  // #if defined(__ANDROID__)

//
// OHOS log support
//
#if defined(__OHOS__)

#define LOGD(...) ((void)OH_LOG_Print(LogType::LOG_APP, LogLevel::LOG_DEBUG, LOG_DOMAIN, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)OH_LOG_Print(LogType::LOG_APP, LogLevel::LOG_INFO, LOG_DOMAIN, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)OH_LOG_Print(LogType::LOG_APP, LogLevel::LOG_WARN, LOG_DOMAIN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)OH_LOG_Print(LogType::LOG_APP, LogLevel::LOG_ERROR, LOG_DOMAIN, LOG_TAG, __VA_ARGS__))
#define LOGF(...) ((void)OH_LOG_Print(LogType::LOG_APP, LogLevel::LOG_FATAL, LOG_DOMAIN, LOG_TAG, __VA_ARGS__))

void StartOutputFileRedirect(const std::string& filePath, bool append);
void StopOutputFileRedirect();
void PrintLogFromFile(const std::string& filePath);

#endif  // #if defined(__OHOS__)

}  // namespace Test

#endif  // TEST_LOG_UTIL