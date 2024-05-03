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
#ifndef API_CORE_LOG_H
#define API_CORE_LOG_H

#include <cassert>
#include <cstdarg>

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/implementation_uids.h>
#include <core/intf_logger.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>

#define CORE_ONCE_RESET CORE_NS::CheckOnceReset

#define CORE_UNUSED(x) (void)(x)
#define CORE_STATIC_ASSERT(expression) static_assert(expression)

#ifndef NDEBUG
#define CORE_ASSERT(expression) \
    assert(!!(expression) || CORE_NS::LogAssert(__FILE__, __LINE__, !!(expression), #expression, ""))
#define CORE_ASSERT_MSG(expression, ...) \
    assert(!!(expression) || CORE_NS::LogAssert(__FILE__, __LINE__, !!(expression), #expression, __VA_ARGS__))
#else
#define CORE_ASSERT(...) CORE_UNUSED(0)
#define CORE_ASSERT_MSG(...) CORE_UNUSED(0)
#endif

#if defined(CORE_LOG_DISABLED) && (CORE_LOG_DISABLED == 1)
#define CORE_LOG_V(...) CORE_UNUSED(0)
#define CORE_LOG_D(...) CORE_UNUSED(0)
#define CORE_LOG_I(...) CORE_UNUSED(0)
#define CORE_LOG_W(...) CORE_UNUSED(0)
#define CORE_LOG_E(...) CORE_UNUSED(0)
#define CORE_LOG_F(...) CORE_UNUSED(0)
#define CORE_LOG_ONCE_V(...) CORE_UNUSED(0)
#define CORE_LOG_ONCE_D(...) CORE_UNUSED(0)
#define CORE_LOG_ONCE_I(...) CORE_UNUSED(0)
#define CORE_LOG_ONCE_W(...) CORE_UNUSED(0)
#define CORE_LOG_ONCE_E(...) CORE_UNUSED(0)
#define CORE_LOG_ONCE_F(...) CORE_UNUSED(0)

// Disable debug logs by default for release builds.
// CORE_LOG_DEBUG compile option can be used to force debug logs also in release builds.
#elif defined(NDEBUG) && !(defined(CORE_LOG_DEBUG) && (CORE_LOG_DEBUG == 1))

#define CORE_LOG_V(...) CORE_UNUSED(0)

#define CORE_LOG_D(...) CORE_UNUSED(0)

#define CORE_LOG_I(...) \
    CHECK_FORMAT_STRING(__VA_ARGS__), CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_INFO, "", 0, __VA_ARGS__)

#define CORE_LOG_W(...) \
    CHECK_FORMAT_STRING(__VA_ARGS__), CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_WARNING, "", 0, __VA_ARGS__)

#define CORE_LOG_E(...) \
    CHECK_FORMAT_STRING(__VA_ARGS__), CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_ERROR, "", 0, __VA_ARGS__)

#define CORE_LOG_F(...) \
    CHECK_FORMAT_STRING(__VA_ARGS__), CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_FATAL, "", 0, __VA_ARGS__)

#define CORE_LOG_ONCE_V(...) CORE_UNUSED(0)

#define CORE_LOG_ONCE_D(...) CORE_UNUSED(0)

#define CORE_LOG_ONCE_I(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_INFO, "", 0, __VA_ARGS__)

#define CORE_LOG_ONCE_W(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_WARNING, "", 0, __VA_ARGS__);

#define CORE_LOG_ONCE_E(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_ERROR, "", 0, __VA_ARGS__)

#define CORE_LOG_ONCE_F(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_FATAL, "", 0, __VA_ARGS__)
#else
/** \addtogroup group_log
 *  @{
 */
/** Write message to log with verbose log level */
#define CORE_LOG_V(...)               \
    CHECK_FORMAT_STRING(__VA_ARGS__), \
        CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_VERBOSE, __FILE__, __LINE__, __VA_ARGS__)

/** Write message to log with debug log level */
#define CORE_LOG_D(...)               \
    CHECK_FORMAT_STRING(__VA_ARGS__), \
        CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

/** Write message to log with info log level */
#define CORE_LOG_I(...)               \
    CHECK_FORMAT_STRING(__VA_ARGS__), \
        CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

/** Write message to log with warning log level */
#define CORE_LOG_W(...)               \
    CHECK_FORMAT_STRING(__VA_ARGS__), \
        CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)

/** Write message to log with error log level */
#define CORE_LOG_E(...)               \
    CHECK_FORMAT_STRING(__VA_ARGS__), \
        CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

/** Write message to log with fatal log level */
#define CORE_LOG_F(...)               \
    CHECK_FORMAT_STRING(__VA_ARGS__), \
        CORE_NS::Log(CORE_NS::ILogger::LogLevel::LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with verbose log level */
#define CORE_LOG_ONCE_V(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_VERBOSE, __FILE__, __LINE__, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with debug log level */
#define CORE_LOG_ONCE_D(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with info log level */
#define CORE_LOG_ONCE_I(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with warning log level */
#define CORE_LOG_ONCE_W(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with error log level */
#define CORE_LOG_ONCE_E(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with fatal log level */
#define CORE_LOG_ONCE_F(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__),  \
        CORE_NS::LogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#endif

CORE_BEGIN_NAMESPACE()
inline ILogger* GetLogger()
{
    static ILogger* gGlobalLogger { nullptr };
    if (gGlobalLogger == nullptr) {
        gGlobalLogger = GetInstance<ILogger>(UID_LOGGER);
    }
    return gGlobalLogger;
}

inline FORMAT_FUNC(4, 5) void Log(ILogger::LogLevel logLevel, const BASE_NS::string_view filename, int lineNumber,
    FORMAT_ATTRIBUTE const char* format, ...)
{
    // log manytimes
    if (ILogger* logger = GetLogger(); logger) {
        std::va_list vl;
        va_start(vl, format);
        logger->VLog(logLevel, filename, lineNumber, format, vl);
        va_end(vl);
    }
}

inline FORMAT_FUNC(5, 6) void LogOnce(const BASE_NS::string_view id, ILogger::LogLevel logLevel,
    const BASE_NS::string_view filename, int lineNumber, FORMAT_ATTRIBUTE const char* format, ...)
{
    // log once.
    if (ILogger* logger = GetLogger(); logger) {
        std::va_list vl;
        va_start(vl, format);
        logger->VLogOnce(id, logLevel, filename, lineNumber, format, vl);
        va_end(vl);
    }
}

inline FORMAT_FUNC(5, 6) bool LogAssert(const BASE_NS::string_view filename, int lineNumber, bool expression,
    const BASE_NS::string_view expressionString, FORMAT_ATTRIBUTE const char* format, ...)
{
    if (!expression) {
        if (ILogger* logger = GetLogger(); logger) {
            std::va_list vl;
            va_start(vl, format);
            logger->VLogAssert(filename, lineNumber, expression, expressionString, format, vl);
            va_end(vl);
        }
    }
    return expression;
}

inline void CheckOnceReset(const BASE_NS::string_view id)
{
    // reset log once flag
    if (ILogger* logger = GetLogger(); logger) {
        logger->CheckOnceReset();
    }
}
/** @} */
CORE_END_NAMESPACE()
#endif // API_CORE_LOG_H
