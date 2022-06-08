/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_LOG_LOGGER_H
#define CORE_LOG_LOGGER_H

#include <mutex>
#include <set>

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class Logger : public ILogger {
public:
    static BASE_NS::string_view GetLogLevelName(LogLevel logLevel, bool shortName);

    explicit Logger(bool defaultOutputs);
    ~Logger() override;

    void VLog(LogLevel logLevel, const BASE_NS::string_view filename, int lineNumber, const BASE_NS::string_view format,
        va_list args) override;
    void VLogOnce(const BASE_NS::string_view id, LogLevel logLevel, const BASE_NS::string_view filename, int lineNumber,
        const BASE_NS::string_view format, va_list args) override;
    bool VLogAssert(const BASE_NS::string_view filename, int lineNumber, bool expression,
        const BASE_NS::string_view expressionString, const BASE_NS::string_view format, va_list args) override;

    FORMAT_FUNC(5, 6)
    void Log(LogLevel logLevel, const BASE_NS::string_view filename, int lineNumber,
        FORMAT_ATTRIBUTE const char* format, ...) override;

    FORMAT_FUNC(6, 7)
    bool LogAssert(const BASE_NS::string_view filename, int lineNumber, bool expression,
        const BASE_NS::string_view expressionString, FORMAT_ATTRIBUTE const char* format, ...) override;

    LogLevel GetLogLevel() const override;
    void SetLogLevel(LogLevel logLevel) override;

    void AddOutput(IOutput::Ptr output) override;
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
