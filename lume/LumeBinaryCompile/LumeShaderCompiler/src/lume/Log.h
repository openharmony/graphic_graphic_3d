#ifndef API_LUME_LOG_H
#define API_LUME_LOG_H

/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <cassert>
#include <memory>

// NOTE: LUME_ONCE is meant e.g. to prevent excessive flooding in the logs with repeating errors.
// i.e. It's not meant for normal working code.
#define LUME_ONCE(uniqueId, runOnce) { if (lume::checkOnce(uniqueId)) { runOnce; } }
#define LUME_ONCE_RESET lume::checkOnceReset

#define LUME_UNUSED(x) (void)(x)
#define LUME_STATIC_ASSERT(expression) static_assert(expression)

#ifndef NDEBUG
#define LUME_ASSERT(expression) assert(lume::getLogger().logAssert(__FILE__, __LINE__, !!(expression), #expression, ""))
#define LUME_ASSERT_MSG(expression, ...) assert(lume::getLogger().logAssert(__FILE__, __LINE__, !!(expression), #expression, __VA_ARGS__))
#else
#define LUME_ASSERT(...)
#define LUME_ASSERT_MSG(...)
#endif

// Enabling warnings checks for format string parameters.
#if defined(__clang__)
#define FORMAT_FUNC(formatPos, argsPos) __attribute__ ((__format__ (__printf__, formatPos, argsPos)))
#define FORMAT_ATTRIBUTE
#elif defined(__GNUC__)
#define FORMAT_FUNC(formatPos, argsPos) __attribute__ ((format (printf, formatPos, argsPos)))
#define FORMAT_ATTRIBUTE
#elif defined(_MSC_VER)
#define FORMAT_FUNC(...)
#define FORMAT_ATTRIBUTE _In_z_ _Printf_format_string_
#else
#define FORMAT_FUNC
#define FORMAT_ATTRIBUTE
#endif

#if defined(LUME_LOG_NO_DEBUG)
#define LUME_LOG_V(...)
#define LUME_LOG_D(...)
#define LUME_LOG_I(...) lume::getLogger().log(lume::ILogger::LogLevel::Info, nullptr, 0, __VA_ARGS__)
#define LUME_LOG_W(...) lume::getLogger().log(lume::ILogger::LogLevel::Warning, nullptr, 0, __VA_ARGS__)
#define LUME_LOG_E(...) lume::getLogger().log(lume::ILogger::LogLevel::Error, nullptr, 0, __VA_ARGS__)
#define LUME_LOG_F(...) lume::getLogger().log(lume::ILogger::LogLevel::Fatal, nullptr, 0, __VA_ARGS__)
#define LUME_LOG_ONCE_V(...)
#define LUME_LOG_ONCE_D(...)
#define LUME_LOG_ONCE_I(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_I(__VA_ARGS__))
#define LUME_LOG_ONCE_W(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_W(__VA_ARGS__))
#define LUME_LOG_ONCE_E(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_E(__VA_ARGS__))
#define LUME_LOG_ONCE_F(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_F(__VA_ARGS__))

#elif defined(LUME_LOG_DISABLED)
#define LUME_LOG_V(...)
#define LUME_LOG_D(...)
#define LUME_LOG_I(...)
#define LUME_LOG_W(...)
#define LUME_LOG_E(...)
#define LUME_LOG_F(...)
#define LUME_LOG_ONCE_V(...)
#define LUME_LOG_ONCE_D(...)
#define LUME_LOG_ONCE_I(...)
#define LUME_LOG_ONCE_W(...)
#define LUME_LOG_ONCE_E(...)
#define LUME_LOG_ONCE_F(...)

#else
#define LUME_LOG_V(...) lume::getLogger().log(lume::ILogger::LogLevel::Verbose, __FILE__, __LINE__, __VA_ARGS__)
#define LUME_LOG_D(...) lume::getLogger().log(lume::ILogger::LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define LUME_LOG_I(...) lume::getLogger().log(lume::ILogger::LogLevel::Info, __FILE__, __LINE__, __VA_ARGS__)
#define LUME_LOG_W(...) lume::getLogger().log(lume::ILogger::LogLevel::Warning, __FILE__, __LINE__, __VA_ARGS__)
#define LUME_LOG_E(...) lume::getLogger().log(lume::ILogger::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
#define LUME_LOG_F(...) lume::getLogger().log(lume::ILogger::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)
#define LUME_LOG_ONCE_V(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_V(__VA_ARGS__))
#define LUME_LOG_ONCE_D(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_D(__VA_ARGS__))
#define LUME_LOG_ONCE_I(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_I(__VA_ARGS__))
#define LUME_LOG_ONCE_W(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_W(__VA_ARGS__))
#define LUME_LOG_ONCE_E(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_E(__VA_ARGS__))
#define LUME_LOG_ONCE_F(uniqueId, ...) LUME_ONCE(uniqueId, LUME_LOG_F(__VA_ARGS__))
#endif


namespace lume
{

class ILogger
{
public:

    enum class LogLevel
    {
        Verbose = 0,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,

        // This level should only be used when setting the log level filter with setLogLevel(),
        // not as a log level for a message.
        None, 
    };

    class IOutput
    {
    public:
        virtual ~IOutput() = default;
        virtual void write(LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aMessage) = 0;
    };

    ILogger() = default;
    virtual ~ILogger() = default;

    ILogger(ILogger const&) = delete;
    void operator=(ILogger const&) = delete;
    

    virtual void vlog(LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aFormat, va_list aArgs) = 0;
    virtual FORMAT_FUNC(5, 6) void log(LogLevel aLogLevel, const char *aFilename, int aLinenumber, FORMAT_ATTRIBUTE const char *aFormat, ...) = 0;
    virtual FORMAT_FUNC(6, 7) bool logAssert(const char *aFilename, int aLinenumber, bool expression, const char *expressionString, FORMAT_ATTRIBUTE const char *aFormat, ...) = 0;

    virtual LogLevel  getLogLevel() const = 0;
    virtual void setLogLevel(LogLevel aLogLevel) = 0;

    virtual void addOutput(std::unique_ptr<IOutput> aOutput) = 0;
};

/*LUME_PUBLIC*/ ILogger &getLogger();

std::unique_ptr<ILogger::IOutput> createLoggerConsoleOutput();
std::unique_ptr<ILogger::IOutput> createLoggerDebugOutput();
std::unique_ptr<ILogger::IOutput> createLoggerFileOutput(const char *aFilename);


bool checkOnce(const char *aId);
void checkOnceReset();


} // lume

#endif // API_LUME_LOG_H
