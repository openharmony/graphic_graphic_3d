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

#include <chrono>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Logger.h"

#if defined(_WIN32)
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifdef ERROR
#undef ERROR
#endif
#else
#include <unistd.h>
#endif

namespace lume {

namespace {
// Note: these must match the ColorCode enum.
constexpr int COLOR_CODE_COUNT = 17;
constexpr const char* COLOR_CODES[COLOR_CODE_COUNT] = {
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

// Gets the filename part from the path.
std::string GetFilename(const std::string& path)
{
    for (int i = static_cast<int>(path.size()) - 1; i >= 0; --i) {
        unsigned int index = static_cast<size_t>(i);
        if (path[index] == '\\' || path[index] == '/') {
            return path.substr(index + 1);
        }
    }
    return path;
}

} // namespace

#if !defined(__PLATFORM_AD__)

class StdOutput : public ILogger::IOutput {
public:
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
    };

    static const char* GetColorString(ColorCode aColorCode)
    {
        int colorCode = static_cast<int>(aColorCode);
        LUME_ASSERT(colorCode >= 0 && colorCode < COLOR_CODE_COUNT);
        return COLOR_CODES[colorCode];
    }

    StdOutput() : mUseColor(false), mCurrentColorString(nullptr)
    {
#if defined(_WIN32)
        // Set console (for this program) to use utf-8.
        constexpr UINT codePageUtf8 = 65001u;
        SetConsoleOutputCP(codePageUtf8);
#endif

        // Try to figure out if this output stream supports colors.
#ifdef _WIN32
        const HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (stdHandle) {
            // Check if the output is being redirected.
            DWORD handleMode;
            if (GetConsoleMode(stdHandle, &handleMode) != 0) {
                // Try to enable the option needed that supports colors.
                handleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(stdHandle, handleMode);

                GetConsoleMode(stdHandle, &handleMode);
                if ((handleMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
                    mUseColor = true;
                }
            }
        }
#else
        if (isatty(fileno(stdout))) {
            // Using colors if the output is not being redirected.
            mUseColor = true;
        }
#endif
    }

    void SetColor(std::ostream& outputStream, ColorCode aColorCode)
    {
        if (!mUseColor) {
            return;
        }

        const char* colorString = GetColorString(aColorCode);
        if (colorString == mCurrentColorString) {
            return;
        }

        mCurrentColorString = colorString;
        if (colorString) {
            outputStream << colorString;
        }
    }

    void Write(ILogger::LogLevel logLevel, const char* filename, int linenumber, const char* message) override
    {
        auto& outputStream = std::cout;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
                  std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
        static constexpr std::streamsize digitSize = 3;
        outputStream << std::put_time(std::localtime(&time), "%H:%M:%S.") << std::setw(digitSize) << std::left
                     << ms.count() << " " << Logger::GetLogLevelName(logLevel, true);

        if (filename) {
            static constexpr std::streamsize filenameSize = 30;
            const std::string filenameLink = " (" + GetFilename(filename) + ':' + std::to_string(linenumber) + ')';
            outputStream << std::right << std::setw(filenameSize) << filenameLink;
        }
        outputStream << ": ";

        if (logLevel >= ILogger::LogLevel::ERROR) {
            SetColor(outputStream, ColorCode::RED);
        } else if (logLevel == ILogger::LogLevel::WARNING) {
            SetColor(outputStream, ColorCode::YELLOW);
        } else if (logLevel <= ILogger::LogLevel::DEBUG) {
            SetColor(outputStream, ColorCode::BLACK_BRIGHT);
        } else {
            SetColor(outputStream, ColorCode::RESET);
        }

        outputStream << message;
        SetColor(outputStream, ColorCode::RESET);
        outputStream << std::endl;
    }

private:
    bool mUseColor;
    const char* mCurrentColorString;
};

#endif // !defined(__PLATFORM_AD__)

#if defined(_WIN32) && !defined(NDEBUG)
class WindowsDebugOutput : public ILogger::IOutput {
public:
    void Write(ILogger::LogLevel logLevel, const char* filename, int linenumber, const char* message) override
    {
        std::stringstream outputStream;

        if (filename) {
            outputStream << filename << "(" << linenumber << ") : ";
        } else {
            outputStream << "lume : ";
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
                  std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

        outputStream << std::put_time(std::localtime(&time), "%D %H:%M:%S.") << ms.count() << " "
                     << Logger::GetLogLevelName(logLevel, true);
        outputStream << ": " << message;
        outputStream << '\n';

        // Convert from utf8 to windows wide unicode string.
        std::string output = outputStream.str();
        int wStringLength =
            ::MultiByteToWideChar(CP_UTF8, 0, output.c_str(), static_cast<int>(output.size()), nullptr, 0);
        std::wstring wString(static_cast<size_t>(wStringLength), 0);
        ::MultiByteToWideChar(CP_UTF8, 0, output.c_str(), static_cast<int>(output.size()), &wString[0], wStringLength);

        ::OutputDebugStringW(wString.c_str());
    }
};
#endif

class FileOutput : public ILogger::IOutput {
public:
    explicit FileOutput(const std::string& aFilePath) : IOutput(), mOutputStream(aFilePath, std::ios::app) {}

    ~FileOutput() override = default;

    void Write(ILogger::LogLevel logLevel, const char* filename, int linenumber, const char* message) override
    {
        if (mOutputStream.is_open()) {
            auto& outputStream = mOutputStream;

            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
                      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

            outputStream << std::put_time(std::localtime(&time), "%D %H:%M:%S.") << ms.count() << " "
                         << Logger::GetLogLevelName(logLevel, false);

            if (filename) {
                outputStream << " (" << filename << ":" << linenumber << "): ";
            } else {
                outputStream << ": ";
            }

            outputStream << message << std::endl;
        }
    }

private:
    std::ofstream mOutputStream;
};

std::unique_ptr<ILogger::IOutput> CreateLoggerConsoleOutput()
{
#ifdef __PLATFORM_AD__
    return std::make_unique<LogcatOutput>();
#else
    return std::make_unique<StdOutput>();
#endif
}

std::unique_ptr<ILogger::IOutput> CreateLoggerDebugOutput()
{
#if defined(_WIN32) && !defined(NDEBUG)
    return std::make_unique<WindowsDebugOutput>();
#else
    return std::unique_ptr<ILogger::IOutput>();
#endif
}

std::unique_ptr<ILogger::IOutput> CreateLoggerFileOutput(const char* filename)
{
    return std::make_unique<FileOutput>(filename);
}
} // namespace lume
