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

#ifndef LUME_LOG_LOGGER_H
#define LUME_LOG_LOGGER_H

#include <lume/Log.h>
#include <memory>
#include <mutex>
#include <vector>

namespace lume {

class Logger : public ILogger {
public:
    static const char* GetLogLevelName(LogLevel logLevel, bool shortName);

    explicit Logger(bool defaultOutputs);
    virtual ~Logger();

    void Vlog(LogLevel logLevel, const char* filename, int linenumber, const char* format, va_list args) override;
    FORMAT_FUNC(5, 6)
    void Log(
        LogLevel logLevel, const char* filename, int linenumber, FORMAT_ATTRIBUTE const char* format, ...) override;
    void Write(LogLevel logLevel, const char* filename, int linenumber, const char* buffer) override;

    FORMAT_FUNC(6, 7)
    bool LogAssert(const char* filename, int linenumber, bool expression, const char* expressionString,
        FORMAT_ATTRIBUTE const char* format, ...) override;

    LogLevel GetLogLevel() const override;
    void SetLogLevel(LogLevel logLevel) override;

    void AddOutput(std::unique_ptr<IOutput> output) override;

private:
    LogLevel logLevel_ = LogLevel::VERBOSE;
    std::mutex loggerMutex_;

    std::vector<char> buffer_;
    std::vector<std::unique_ptr<IOutput>> outputs_;
};
} // namespace lume

#endif // LUME_LOG_LOGGER_H
