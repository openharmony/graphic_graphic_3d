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

#include "Logger.h"


#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <chrono>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#if _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#else
#include <unistd.h>
#endif


namespace lume
{

namespace
{

//Gets the filename part from the path.
std::string getFilename(const std::string &aPath)
{
    for (int i = static_cast<int>(aPath.size()) - 1; i >= 0 ; --i)
    {
        unsigned int index = static_cast<size_t>(i);
        if (aPath[index] == '\\' || aPath[index] == '/') {
            return aPath.substr(index + 1);
        }
    }
    return aPath;
}

} // empty namespace


#if !defined(__ANDROID__)

class StdOutput : public ILogger::IOutput
{
public:
    enum class ColorCode
    {
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

    static const char* getColorString(ColorCode aColorCode)
    {
        // Note: these must match the ColorCode enum.
        constexpr int COLOR_CODE_COUNT = 17;
        constexpr const char* COLOR_CODES[COLOR_CODE_COUNT] =
        {
            "\x1B[30m", "\x1B[31m", "\x1B[32m", "\x1B[33m",
            "\x1B[34m", "\x1B[35m", "\x1B[36m", "\x1B[37m",
            "\x1B[30;1m", "\x1B[31;1m", "\x1B[32;1m", "\x1B[33;1m",
            "\x1B[34;1m", "\x1B[35;1m", "\x1B[36;1m", "\x1B[37;1m",
            "\x1B[0m",
        };

        int colorCode = static_cast<int>(aColorCode);
        LUME_ASSERT(colorCode >= 0 && colorCode < COLOR_CODE_COUNT);
        return COLOR_CODES[colorCode];
    }

    StdOutput() : mUseColor(false), mCurrentColorString(nullptr)
    {
#if _WIN32
        // Set console (for this program) to use utf-8.
        SetConsoleOutputCP(65001);
#endif

        // Try to figure out if this output stream supports colors.
#ifdef _WIN32
        const HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (stdHandle)
        {
            // Check if the output is being redirected.
            DWORD handleMode;
            if (GetConsoleMode(stdHandle, &handleMode) != 0)
            {
                // Try to enable the option needed that supports colors.
                handleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(stdHandle, handleMode);

                GetConsoleMode(stdHandle, &handleMode);
                if ((handleMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0)
                {
                    mUseColor = true;
                }
            }
        }
#else
        if (isatty(fileno(stdout)))
        {
            // Using colors if the output is not being redirected.
            mUseColor = true;
        }
#endif
    }

    void setColor(std::ostream &outputStream, ColorCode aColorCode)
    {
        if (!mUseColor) {
            return;
        }

        const char* colorString = getColorString(aColorCode);
        if (colorString == mCurrentColorString)
        {
            return;
        }

        mCurrentColorString = colorString;
        if (colorString)
        {
            outputStream << colorString;
        }
    }

    void write(ILogger::LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aMessage) override
    {
        auto &outputStream = std::cout;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
            std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

        outputStream << std::put_time(std::localtime(&time), "%H:%M:%S.") << std::setw(3) << std::left << ms.count() << " " << Logger::getLogLevelName(aLogLevel, true);

        if (aFilename)
        {
            const std::string filenameLink = " (" + getFilename(aFilename) + ":" + std::to_string(aLinenumber) + ")";
            outputStream << std::right << std::setw(30) << filenameLink;
        }
        outputStream << ": ";

        if (aLogLevel >= ILogger::LogLevel::Error)
        {
            setColor(outputStream, ColorCode::RED);
        }
        else if (aLogLevel == ILogger::LogLevel::Warning)
        {
            setColor(outputStream, ColorCode::YELLOW);
        }
        else if (aLogLevel <= ILogger::LogLevel::Debug)
        {
            setColor(outputStream, ColorCode::BLACK_BRIGHT);
        }
        else
        {
            setColor(outputStream, ColorCode::RESET);
        }

        outputStream << aMessage;
        setColor(outputStream, ColorCode::RESET);
        outputStream << std::endl;
    }

private:
    bool mUseColor;
    const char *mCurrentColorString;

};

#endif // !defined(__ANDROID__)


#if defined(_WIN32) && !defined(NDEBUG)
class WindowsDebugOutput : public ILogger::IOutput
{
public:
    void write(ILogger::LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aMessage) override
    {
        std::stringstream outputStream;

        if (aFilename)
        {
            outputStream << aFilename << "(" << aLinenumber << ") : ";
        }
        else
        {
            outputStream << "lume : ";
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
            std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

        outputStream << std::put_time(std::localtime(&time), "%D %H:%M:%S.") << ms.count() << " " << Logger::getLogLevelName(aLogLevel, true);
        outputStream << ": " << aMessage;
        outputStream << '\n';

        // Convert from utf8 to windows wide unicode string.
        std::string message = outputStream.str();
        int wStringLength = ::MultiByteToWideChar(CP_UTF8, 0, message.c_str(), static_cast<int>(message.size()), nullptr, 0);
        std::wstring wString(static_cast<size_t>(wStringLength), 0);
        ::MultiByteToWideChar(CP_UTF8, 0, message.c_str(), static_cast<int>(message.size()), &wString[0], wStringLength);

        ::OutputDebugStringW(wString.c_str());
    }
};
#endif

class FileOutput : public ILogger::IOutput
{
public:
    explicit FileOutput(const std::string &aFilePath) : IOutput(), mOutputStream(aFilePath, std::ios::app) {}

    ~FileOutput() override = default;

    void write(ILogger::LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aMessage) override
    {
        if (mOutputStream.is_open())
        {
            auto &outputStream = mOutputStream;

            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

            outputStream << std::put_time(std::localtime(&time), "%D %H:%M:%S.") << ms.count() << " " << Logger::getLogLevelName(aLogLevel, false);

            if (aFilename)
            {
                outputStream << " (" << aFilename << ":" << aLinenumber << "): ";
            }
            else
            {
                outputStream << ": ";
            }

            outputStream << aMessage << std::endl;
        }
    }
private:
    std::ofstream mOutputStream;
};

#if defined(__ANDROID__)
class LogcatOutput : public Logger::IOutput
{
public:
    void write(ILogger::LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aMessage) override
    {
        int logPriority;
        switch (aLogLevel)
        {
        case ILogger::LogLevel::Verbose:
            logPriority = ANDROID_LOG_VERBOSE;
            break;

        case ILogger::LogLevel::Debug:
            logPriority = ANDROID_LOG_DEBUG;
            break;

        case ILogger::LogLevel::Info:
            logPriority = ANDROID_LOG_INFO;
            break;

        case ILogger::LogLevel::Warning:
            logPriority = ANDROID_LOG_WARN;
            break;

        case ILogger::LogLevel::Error:
            logPriority = ANDROID_LOG_ERROR;
            break;

        case ILogger::LogLevel::Fatal:
            logPriority = ANDROID_LOG_FATAL;
            break;

        default:
            logPriority = ANDROID_LOG_VERBOSE;
            break;
        }

        if (aFilename)
        {
            std::stringstream outputStream;
            outputStream << "(" << getFilename(aFilename) << ":" << aLinenumber << "): ";
            outputStream << aMessage;
            __android_log_write(logPriority, "lume", outputStream.str().c_str());
        }
        else
        {
            __android_log_write(logPriority, "lume", aMessage);
        }
    }
};
#endif



std::unique_ptr<ILogger::IOutput> createLoggerConsoleOutput()
{
#ifdef __ANDROID__
    return std::make_unique<LogcatOutput>();
#else
    return std::make_unique<StdOutput>();
#endif
}


std::unique_ptr<ILogger::IOutput> createLoggerDebugOutput()
{
#if defined(_WIN32) && !defined(NDEBUG)
    return std::make_unique<WindowsDebugOutput>();
#else
    return std::unique_ptr<ILogger::IOutput>();
#endif
}


std::unique_ptr<ILogger::IOutput> createLoggerFileOutput(const char *aFilename)
{
    return std::make_unique<FileOutput>(aFilename);
}

} // lume