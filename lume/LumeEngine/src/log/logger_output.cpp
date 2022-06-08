/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "log/logger_output.h"

#include <chrono>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

#include <core/namespace.h>

#include "log/logger.h"

namespace LoggerUtils {
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
        outputStream << std::right << std::setfill('0') << std::setw(3) // 3: alignment
            << ms.count() << std::setfill(' ');
    }
} // namespace LoggerUtils


CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;


class FileOutput final : public ILogger::IOutput {
public:
    explicit FileOutput(const string_view filePath) : IOutput(), outputStream_(filePath.data(), std::ios::app) {}

    ~FileOutput() override = default;

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
