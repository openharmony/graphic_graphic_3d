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
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string_view>

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/namespace.h>

#include "log/logger.h"

namespace LoggerUtils {
BASE_NS::string_view GetFilename(BASE_NS::string_view path)
{
    if (auto const pos = path.find_last_of("\\/"); pos != BASE_NS::string_view::npos) {
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
    outputStream << std::right << std::setfill('0') << std::setw(3) // 3: alignment
                 << ms.count() << std::setfill(' ');
}
} // namespace LoggerUtils

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;

class FileOutput final : public ILogger::IOutput {
public:
    explicit FileOutput(const string_view filePath) : IOutput(), outputStream_(filePath.data(), std::ios::app) {}

    void Write(
        ILogger::LogLevel logLevel, const string_view filename, int linenumber, const string_view message) override
    {
        if (outputStream_.is_open()) {
            auto& outputStream = outputStream_;

            LoggerUtils::PrintTimeStamp(outputStream);
            const auto levelString = Logger::GetLogLevelName(logLevel, true);
            outputStream << ' ' << std::string_view(levelString.data(), levelString.size());

            if (!filename.empty()) {
                outputStream << " (" << std::string_view(filename.data(), filename.size()) << ':' << linenumber
                             << "): ";
            } else {
                outputStream << ": ";
            }

            outputStream << std::string_view(message.data(), message.size()) << std::endl;
        }
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    std::ofstream outputStream_;
};

ILogger::IOutput::Ptr CreateLoggerFileOutput(const string_view filename)
{
    return ILogger::IOutput::Ptr { new FileOutput(filename) };
}
CORE_END_NAMESPACE()
