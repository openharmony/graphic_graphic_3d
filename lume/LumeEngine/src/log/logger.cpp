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

#include "logger.h"

#include <cstdarg>
#include <mutex>
#include <securec.h>
#include <set>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/intf_logger.h>
#include <core/log.h>
#include <core/namespace.h>

#ifdef PLATFORM_HAS_JAVA
#include <os/java/java_internal.h>
#endif

#include "log/logger_output.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::Uid;

namespace {
// Note: these must match the LogLevel enum.
constexpr int LOG_LEVEL_COUNT = 7;
constexpr const char* LOG_LEVEL_NAMES[LOG_LEVEL_COUNT] = {
    "Verbose",
    "Debug",
    "Info",
    "Warning",
    "Error",
    "Fatal",
    "None",
};

constexpr const char* LOG_LEVEL_NAMES_SHORT[LOG_LEVEL_COUNT] = {
    "V",
    "D",
    "I",
    "W",
    "E",
    "F",
    "N",
};
constexpr const size_t MAX_BUFFER_SIZE = 1024;
} // namespace

string_view Logger::GetLogLevelName(LogLevel logLevel, bool shortName)
{
    const int level = static_cast<int>(logLevel);
    CORE_ASSERT(level >= 0 && level < LOG_LEVEL_COUNT);

    return shortName ? LOG_LEVEL_NAMES_SHORT[level] : LOG_LEVEL_NAMES[level];
}

Logger::Logger(bool defaultOutputs)
#ifdef NDEBUG
    : logLevel_(LogLevel::LOG_ERROR)
#endif
{
    if (defaultOutputs) {
#if CORE_LOG_TO_CONSOLE == 1
        AddOutput(CreateLoggerConsoleOutput());
#endif

#if CORE_LOG_TO_DEBUG_OUTPUT == 1
        AddOutput(CreateLoggerDebugOutput());
#endif

#if CORE_LOG_TO_FILE == 1
        AddOutput(CreateLoggerFileOutput("./logfile.txt"));
#endif
    }
}

void Logger::VLog(
    LogLevel logLevel, const string_view filename, int lineNumber, const string_view format, std::va_list args)
{
    CORE_ASSERT_MSG(logLevel != LogLevel::LOG_NONE, "'None' is not a valid log level for writing to the log.");

    if (logLevel_ > logLevel) {
        return;
    }

    // we need to make a copy of the args, since the va_list can be in an undefined state after use.
    std::va_list tmp;
    va_copy(tmp, args);

    // use vsnprintf to calculate the required size (not supported by the _s variant)
    const int sizeNeeded = vsnprintf(nullptr, 0, format.data(), args) + 1;

    std::lock_guard guard(loggerMutex_);

    if (sizeNeeded > 0 && static_cast<size_t>(sizeNeeded) > buffer_.size()) {
        buffer_.resize(static_cast<size_t>(sizeNeeded));
    }

    int ret = vsnprintf_s(buffer_.data(), buffer_.size(), buffer_.size() - 1, format.data(), tmp);
    va_end(tmp);
    if (ret < 0) {
        return;
    }

    for (auto& output : outputs_) {
        output->Write(logLevel, filename, lineNumber, buffer_.data());
    }
}

void Logger::VLogOnce(const string_view id, LogLevel logLevel, const string_view filename, int lineNumber,
    const string_view format, std::va_list args)
{
    std::lock_guard<std::mutex> guard(onceMutex_);

    auto const [pos, inserted] = registeredOnce_.insert(string(id));
    if (inserted) {
        VLog(logLevel, filename, lineNumber, format, args);
    }
}

bool Logger::VLogAssert(const string_view filename, int lineNumber, bool expression, const string_view expressionString,
    const string_view format, std::va_list args)
{
    if (!expression) {
        char buffer[MAX_BUFFER_SIZE];
        const int numWritten = vsnprintf_s(buffer, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE - 1, format.data(), args);
        if (numWritten >= 0) {
            buffer[numWritten] = '\0';
        } else {
            buffer[0] = '\0';
        }

        Log(LogLevel::LOG_FATAL, filename, lineNumber, "Assert failed (%s). %s", expressionString.data(), buffer);

#ifdef PLATFORM_HAS_JAVA
        // Print also a java trace if available
        Log(LogLevel::LOG_FATAL, filename, lineNumber, "Java trace:");
        JNIEnv* env = java_internal::GetJavaEnv();
        if (env) {
            jclass cls = env->FindClass("java/lang/Exception");
            if (cls) {
                jmethodID constructor = env->GetMethodID(cls, "<init>", "()V");
                jobject exception = env->NewObject(cls, constructor);
                jmethodID printStackTrace = env->GetMethodID(cls, "printStackTrace", "()V");
                env->CallVoidMethod(exception, printStackTrace);
                env->DeleteLocalRef(exception);
            }
        }
#endif
    }

    return expression;
}

void Logger::CheckOnceReset()
{
    std::lock_guard<std::mutex> guard(onceMutex_);
    registeredOnce_.clear();
}

FORMAT_FUNC(5, 6)
void Logger::Log(
    LogLevel logLevel, const string_view filename, int lineNumber, FORMAT_ATTRIBUTE const char* format, ...)
{
    std::va_list vl;
    va_start(vl, format);
    VLog(logLevel, filename, lineNumber, format, vl);
    va_end(vl);
}

FORMAT_FUNC(6, 7)
bool Logger::LogAssert(const string_view filename, int lineNumber, bool expression, const string_view expressionString,
    FORMAT_ATTRIBUTE const char* format, ...)
{
    if (!expression) {
        std::va_list vl;
        va_start(vl, format);
        VLogAssert(filename, lineNumber, expression, expressionString, format, vl);
        va_end(vl);
    }
    return expression;
}

ILogger::LogLevel Logger::GetLogLevel() const
{
    return logLevel_;
}

void Logger::SetLogLevel(LogLevel logLevel)
{
    logLevel_ = logLevel;
}

void Logger::AddOutput(IOutput::Ptr output)
{
    if (output) {
        std::lock_guard<std::mutex> guard(loggerMutex_);
        outputs_.push_back(move(output));
    }
}

const IInterface* Logger::GetInterface(const Uid& uid) const
{
    if ((uid == ILogger::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* Logger::GetInterface(const Uid& uid)
{
    if ((uid == ILogger::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void Logger::Ref() {}

void Logger::Unref() {}
CORE_END_NAMESPACE()
