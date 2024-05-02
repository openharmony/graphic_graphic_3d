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

#ifndef CORE_LOG_LOGGER_H
#define CORE_LOG_LOGGER_H

#include <cstdarg>
#include <mutex>
#include <set>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/intf_logger.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
struct Uid;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IInterface;
class Logger : public ILogger {
public:
    static BASE_NS::string_view GetLogLevelName(LogLevel logLevel, bool shortName);

    explicit Logger(bool defaultOutputs);
    ~Logger() override = default;

    void VLog(LogLevel logLevel, BASE_NS::string_view filename, int lineNumber, BASE_NS::string_view format,
        std::va_list args) override;
    void VLogOnce(BASE_NS::string_view id, LogLevel logLevel, BASE_NS::string_view filename, int lineNumber,
        BASE_NS::string_view format, std::va_list args) override;
    bool VLogAssert(BASE_NS::string_view filename, int lineNumber, bool expression,
        BASE_NS::string_view expressionString, BASE_NS::string_view format, std::va_list args) override;

    FORMAT_FUNC(5, 6)
    void Log(LogLevel logLevel, BASE_NS::string_view filename, int lineNumber, FORMAT_ATTRIBUTE const char* format,
        ...) override;

    FORMAT_FUNC(6, 7)
    bool LogAssert(BASE_NS::string_view filename, int lineNumber, bool expression,
        BASE_NS::string_view expressionString, FORMAT_ATTRIBUTE const char* format, ...) override;

    LogLevel GetLogLevel() const override;
    void SetLogLevel(LogLevel logLevel) override;

    void AddOutput(IOutput::Ptr output) final;
    void CheckOnceReset() override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
    LogLevel logLevel_ = LogLevel::LOG_VERBOSE;
    std::mutex loggerMutex_;

    BASE_NS::vector<char> buffer_;
    BASE_NS::vector<IOutput::Ptr> outputs_;
    std::set<BASE_NS::string> registeredOnce_; // Global set of ids used by the CORE_ONCE macro.
    std::mutex onceMutex_;
};
CORE_END_NAMESPACE()

#endif // CORE_LOG_LOGGER_H
