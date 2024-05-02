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

#include "log/logger_output.h"

#include <chrono>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <hilog/log.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <unistd.h>

#include <core/namespace.h>

#include "log/logger.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;

namespace {
// Gets the filename part from the path.
std::string_view GetFilename(std::string_view path)
{
    if (auto const pos = path.find_last_of("\\/"); pos != std::string_view::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

void PrintTimeStamp(std::ostream& outputStream)
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
                    std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

    outputStream << std::put_time(std::localtime(&time), "%D %H:%M:%S.");
    outputStream << std::right << std::setfill('0') << std::setw(3) << ms.count() << std::setfill(' '); // 3: alignment
}
} // namespace

class LogcatOutput final : public Logger::IOutput {
public:
    void Write(
        ILogger::LogLevel logLevel, const string_view filename, int linenumber, const string_view message) override
    {
        int logPriority;
        switch (logLevel) {
            case ILogger::LogLevel::LOG_VERBOSE:
                logPriority = LOG_LEVEL_MIN;
                break;

            case ILogger::LogLevel::LOG_DEBUG:
                logPriority = LOG_DEBUG;
                break;

            case ILogger::LogLevel::LOG_INFO:
                logPriority = LOG_INFO;
                break;

            case ILogger::LogLevel::LOG_WARNING:
                logPriority = LOG_WARN;
                break;

            case ILogger::LogLevel::LOG_ERROR:
                logPriority = LOG_ERROR;
                break;

            case ILogger::LogLevel::LOG_FATAL:
                logPriority = LOG_FATAL;
                break;

            default:
                logPriority = LOG_LEVEL_MIN;
                break;
        }
        unsigned int domain = 0xD003B00;
        if (!filename.empty()) {
            std::stringstream outputStream;
            auto const filenameView = GetFilename({ filename.data(), filename.size() });
            outputStream << '(' << filenameView << ':' << linenumber << "): ";
            outputStream << std::string_view(message.data(), message.size());
            HiLogPrint(LOG_CORE, LOG_ERROR, domain, logTag_.data(), "%{public}s", outputStream.str().c_str());
        } else {
            HiLogPrint(LOG_CORE, LOG_ERROR, domain, logTag_.data(), "%{public}s", message.data());
        }
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    static constexpr std::string_view logTag_ = "ohos_lume";
};

ILogger::IOutput::Ptr CreateLoggerConsoleOutput()
{
    return ILogger::IOutput::Ptr { new LogcatOutput };
}

ILogger::IOutput::Ptr CreateLoggerDebugOutput()
{
    return {};
}
CORE_END_NAMESPACE()
