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
#ifndef CORE3D_UTIL_LOG_H
#define CORE3D_UTIL_LOG_H

#include <cassert>
#include <cstdarg>
#include <cstdint>

#include <3d/namespace.h>
#include <core/implementation_uids.h>
#include <core/intf_logger.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>

CORE3D_BEGIN_NAMESPACE()
constexpr uint32_t CORE3D_PROFILER_DEFAULT_COLOR { 0xffa000 };
CORE_NS::IPluginRegister& GetPluginRegister();
CORE3D_END_NAMESPACE()

#define PLUGIN_ONCE_RESET CORE3D_NS::PluginCheckOnceReset
#define PLUGIN_UNUSED(x) (void)(x)
#define PLUGIN_STATIC_ASSERT(expression) static_assert(!!(expression))

#ifndef NDEBUG
#define PLUGIN_FILE_INFO __FILE__, __LINE__
#define PLUGIN_ASSERT(expression) \
    assert(!!(expression) || CORE3D_NS::PluginLogAssert(PLUGIN_FILE_INFO, !!(expression), #expression, ""))
#define PLUGIN_ASSERT_MSG(expression, ...) \
    assert(!!(expression) || CORE3D_NS::PluginLogAssert(PLUGIN_FILE_INFO, !!(expression), #expression, __VA_ARGS__))
#else
#define PLUGIN_FILE_INFO "", 0U
#define PLUGIN_ASSERT(...)
#define PLUGIN_ASSERT_MSG(...)
#endif

#if defined(PLUGIN_LOG_DISABLED) && (PLUGIN_LOG_DISABLED == 1)
#define PLUGIN_LOG_V(...)
#define PLUGIN_LOG_D(...)
#define PLUGIN_LOG_I(...)
#define PLUGIN_LOG_W(...)
#define PLUGIN_LOG_E(...)
#define PLUGIN_LOG_F(...)
#define PLUGIN_LOG_ONCE_V(...)
#define PLUGIN_LOG_ONCE_D(...)
#define PLUGIN_LOG_ONCE_I(...)
#define PLUGIN_LOG_ONCE_W(...)
#define PLUGIN_LOG_ONCE_E(...)
#define PLUGIN_LOG_ONCE_F(...)

#else

// Disable debug logs by default for release builds.
// PLUGIN_LOG_DEBUG compile option can be used to force debug logs also in release builds.
#if defined(NDEBUG) && !(defined(PLUGIN_LOG_DEBUG) && (PLUGIN_LOG_DEBUG == 1))

#define PLUGIN_LOG_V(...)
#define PLUGIN_LOG_D(...)
#define PLUGIN_LOG_ONCE_V(...)
#define PLUGIN_LOG_ONCE_D(...)
#else
/** \addtogroup group_log
 *  @{
 */
/** Write message to log with verbose log level */
#define PLUGIN_LOG_V(...)             \
    CHECK_FORMAT_STRING(__VA_ARGS__); \
    CORE3D_NS::PluginLog(CORE_NS::ILogger::LogLevel::LOG_VERBOSE, PLUGIN_FILE_INFO, __VA_ARGS__)
/** Write message to log with debug log level */
#define PLUGIN_LOG_D(...)             \
    CHECK_FORMAT_STRING(__VA_ARGS__); \
    CORE3D_NS::PluginLog(CORE_NS::ILogger::LogLevel::LOG_DEBUG, PLUGIN_FILE_INFO, __VA_ARGS__)
/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with verbose log level */
#define PLUGIN_LOG_ONCE_V(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__);    \
    CORE3D_NS::PluginLogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_VERBOSE, PLUGIN_FILE_INFO, __VA_ARGS__)
/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with debug log level */
#define PLUGIN_LOG_ONCE_D(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__);    \
    CORE3D_NS::PluginLogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_DEBUG, PLUGIN_FILE_INFO, __VA_ARGS__)
#endif

/** Write message to log with info log level */
#define PLUGIN_LOG_I(...)             \
    CHECK_FORMAT_STRING(__VA_ARGS__); \
    CORE3D_NS::PluginLog(CORE_NS::ILogger::LogLevel::LOG_INFO, PLUGIN_FILE_INFO, __VA_ARGS__)

/** Write message to log with warning log level */
#define PLUGIN_LOG_W(...)             \
    CHECK_FORMAT_STRING(__VA_ARGS__); \
    CORE3D_NS::PluginLog(CORE_NS::ILogger::LogLevel::LOG_WARNING, PLUGIN_FILE_INFO, __VA_ARGS__)

/** Write message to log with error log level */
#define PLUGIN_LOG_E(...)             \
    CHECK_FORMAT_STRING(__VA_ARGS__); \
    CORE3D_NS::PluginLog(CORE_NS::ILogger::LogLevel::LOG_ERROR, PLUGIN_FILE_INFO, __VA_ARGS__)

/** Write message to log with fatal log level */
#define PLUGIN_LOG_F(...)             \
    CHECK_FORMAT_STRING(__VA_ARGS__); \
    CORE3D_NS::PluginLog(CORE_NS::ILogger::LogLevel::LOG_FATAL, PLUGIN_FILE_INFO, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with info log level */
#define PLUGIN_LOG_ONCE_I(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__);    \
    CORE3D_NS::PluginLogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_INFO, PLUGIN_FILE_INFO, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with warning log level */
#define PLUGIN_LOG_ONCE_W(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__);    \
    CORE3D_NS::PluginLogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_WARNING, PLUGIN_FILE_INFO, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with error log level */
#define PLUGIN_LOG_ONCE_E(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__);    \
    CORE3D_NS::PluginLogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_ERROR, PLUGIN_FILE_INFO, __VA_ARGS__)

/** \brief NOTE: CORE_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
            i.e. It's not meant for normal working code. */
/** Write message to log with fatal log level */
#define PLUGIN_LOG_ONCE_F(uniqueId, ...) \
    CHECK_FORMAT_STRING(__VA_ARGS__);    \
    CORE3D_NS::PluginLogOnce(uniqueId, CORE_NS::ILogger::LogLevel::LOG_FATAL, PLUGIN_FILE_INFO, __VA_ARGS__)
#endif

CORE3D_BEGIN_NAMESPACE()
/** Get class instance from specified class registry */
template<class T>
auto GetInstance(const CORE_NS::IClassRegister& registry, const BASE_NS::Uid& uid)
{
    CORE_NS::IInterface* instance = static_cast<T*>(registry.GetInstance(uid));
    if (instance) {
        return static_cast<T*>(instance->GetInterface(T::UID));
    }
    return static_cast<T*>(nullptr);
}

/** Get class instance from specified class registry */
template<class T>
auto GetInstance(const CORE_NS::IClassRegister* registry, const BASE_NS::Uid& uid)
{
    if (!registry) {
        return static_cast<T*>(nullptr);
    }
    return CORE3D_NS::GetInstance<T>(*registry, uid);
}

/** Get interface from global registry */
template<class T>
auto GetInstance(const BASE_NS::Uid& uid)
{
    return CORE3D_NS::GetInstance<T>(CORE3D_NS::GetPluginRegister().GetClassRegister(), uid);
}

/** Create interface from specified interface registry */
template<class T>
auto CreateInstance(CORE_NS::IClassFactory& factory, const BASE_NS::Uid& uid)
{
    // Create the instance
    if (auto res = factory.CreateInstance(uid)) {
        // Ask for the interface.
        return typename T::Ptr(res->GetInterface<T>());
    }
    return typename T::Ptr();
}

inline CORE_NS::ILogger* GetPluginLogger()
{
    static CORE_NS::ILogger* gPluginGlobalLogger { nullptr };
    if (gPluginGlobalLogger == nullptr) {
        gPluginGlobalLogger = CORE3D_NS::GetInstance<CORE_NS::ILogger>(
            CORE3D_NS::GetPluginRegister().GetClassRegister(), CORE_NS::UID_LOGGER);
    }
    return gPluginGlobalLogger;
}

inline FORMAT_FUNC(4, 5) void PluginLog(CORE_NS::ILogger::LogLevel logLevel, const BASE_NS::string_view filename,
    int lineNumber, FORMAT_ATTRIBUTE const char* format, ...)
{
    // log manytimes
    if (CORE_NS::ILogger* logger = GetPluginLogger(); logger) {
        va_list vl;
        va_start(vl, format);
        logger->VLog(logLevel, filename, lineNumber, format, vl);
        va_end(vl);
    }
}

inline FORMAT_FUNC(5, 6) void PluginLogOnce(const BASE_NS::string_view id, CORE_NS::ILogger::LogLevel logLevel,
    const BASE_NS::string_view filename, int lineNumber, FORMAT_ATTRIBUTE const char* format, ...)
{
    // log once.
    if (CORE_NS::ILogger* logger = GetPluginLogger(); logger) {
        va_list vl;
        va_start(vl, format);
        logger->VLogOnce(id, logLevel, filename, lineNumber, format, vl);
        va_end(vl);
    }
}

inline FORMAT_FUNC(5, 6) bool PluginLogAssert(const BASE_NS::string_view filename, int lineNumber, bool expression,
    const BASE_NS::string_view expressionString, FORMAT_ATTRIBUTE const char* format, ...)
{
    if (!expression) {
        if (CORE_NS::ILogger* logger = GetPluginLogger(); logger) {
            va_list vl;
            va_start(vl, format);
            logger->VLogAssert(filename, lineNumber, expression, expressionString, format, vl);
            va_end(vl);
        }
    }
    return expression;
}

inline void PluginCheckOnceReset(const BASE_NS::string_view /* id */)
{
    // reset log once flag
    if (CORE_NS::ILogger* logger = GetPluginLogger(); logger) {
        logger->CheckOnceReset();
    }
}
CORE3D_END_NAMESPACE()

#endif // CORE3D_UTIL_LOG_H
