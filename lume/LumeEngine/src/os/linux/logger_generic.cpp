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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string_view>
#include <unistd.h>

#include <core/log.h>
#include <core/namespace.h>

#include "log/logger.h"
#include "log/logger_output.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;

class StdOutput final : public ILogger::IOutput {
public:
    StdOutput()
    {
        // Try to figure out if this output stream supports colors.
        const auto isXCode = !!std::getenv("IDE_DISABLED_OS_ACTIVITY_DT_MODE");
        if (!isXCode && isatty(fileno(stdout))) {
            // Using colors if the output is not being redirected.
            useColor_ = true;
        }
    }

    void Write(
        ILogger::LogLevel logLevel, const string_view filename, int linenumber, const string_view message) override
    {
        auto& outputStream = std::cout;

        LoggerUtils::PrintTimeStamp(outputStream);
        const auto levelString = Logger::GetLogLevelName(logLevel, true);
        outputStream << ' ' << std::string_view(levelString.data(), levelString.size());

        if (!filename.empty()) {
            // Align the printed messages to same horizontal position regardless of the printed filename length.
            // (Unless the filename is very long)
            constexpr int fileLinkFieldSize = 30;

            auto const filenameView = LoggerUtils::GetFilename(filename);
            // Break long messages to multiple lines. 0..9 on one line. 10..99 on two lines. 100..999 on three and above
            // that four.
            const int lineNumberLength = (linenumber < 10 ? 1 : (linenumber < 100 ? 2 : (linenumber < 1000 ? 3 : 4)));
            const int fileLinkPadding =
                fileLinkFieldSize - (static_cast<int>(filenameView.length()) + lineNumberLength);
            if (fileLinkPadding > 0) {
                outputStream << std::setw(fileLinkPadding) << "";
            }
            outputStream << " (" << std::string_view(filenameView.data(), filenameView.size()) << ':' << linenumber
                         << ')';
        }
        outputStream << ": ";

        if (logLevel >= ILogger::LogLevel::LOG_ERROR) {
            SetColor(outputStream, ColorCode::RED);
        } else if (logLevel == ILogger::LogLevel::LOG_WARNING) {
            SetColor(outputStream, ColorCode::YELLOW);
        } else if (logLevel <= ILogger::LogLevel::LOG_DEBUG) {
            SetColor(outputStream, ColorCode::BLACK_BRIGHT);
        } else {
            SetColor(outputStream, ColorCode::RESET);
        }

        outputStream << std::string_view(message.data(), message.size());
        SetColor(outputStream, ColorCode::RESET);
        outputStream << std::endl;
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    enum class ColorCode {
        BLACK = 0,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE,
        BLACK_BRIGHT,
        RED_BRIGHT,
        GREEN_BRIGHT,
        YELLOW_BRIGHT,
        BLUE_BRIGHT,
        MAGENTA_BRIGHT,
        CYAN_BRIGHT,
        WHITE_BRIGHT,
        RESET,
        COLOR_CODE_COUNT
    };
    // Note: these must match the ColorCode enum.
    static constexpr const string_view COLOR_CODES[static_cast<int>(ColorCode::COLOR_CODE_COUNT)] = {
        "\x1B[30m",
        "\x1B[31m",
        "\x1B[32m",
        "\x1B[33m",
        "\x1B[34m",
        "\x1B[35m",
        "\x1B[36m",
        "\x1B[37m",
        "\x1B[30;1m",
        "\x1B[31;1m",
        "\x1B[32;1m",
        "\x1B[33;1m",
        "\x1B[34;1m",
        "\x1B[35;1m",
        "\x1B[36;1m",
        "\x1B[37;1m",
        "\x1B[0m",
    };

    static const string_view GetColorString(ColorCode colorCode)
    {
        const int code = static_cast<int>(colorCode);
        CORE_ASSERT(code >= 0 && code < static_cast<int>(ColorCode::COLOR_CODE_COUNT));
        return COLOR_CODES[code];
    }

    void SetColor(std::ostream& outputStream, ColorCode colorCode)
    {
        if (colorCode < ColorCode::BLACK || colorCode >= ColorCode::COLOR_CODE_COUNT) {
            return;
        }

        if (!useColor_) {
            return;
        }

        if (colorCode == currentColorString_) {
            return;
        }

        currentColorString_ = colorCode;

        const auto colorString = GetColorString(colorCode);
        outputStream << std::string_view(colorString.data(), colorString.size());
    }

    bool useColor_ { false };
    ColorCode currentColorString_ { ColorCode::RESET };
};

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

ILogger::IOutput::Ptr CreateLoggerConsoleOutput()
{
    return ILogger::IOutput::Ptr { new StdOutput };
}

ILogger::IOutput::Ptr CreateLoggerDebugOutput()
{
    return {};
}
CORE_END_NAMESPACE()
