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

#include <string>
#include <cstdarg>

#include <set>

namespace lume
{

const char* Logger::getLogLevelName(LogLevel aLogLevel, bool aShortName)
{
    // Note: these must match the LogLevel enum.
    constexpr int LOG_LEVEL_COUNT = 7;
    constexpr const char* LOG_LEVEL_NAMES[LOG_LEVEL_COUNT] =
    {
        "Verbose", "Debug", "Info", "Warning", "Error", "Fatal", "None",
    };

    constexpr const char* LOG_LEVEL_NAMES_SHORT[LOG_LEVEL_COUNT] =
    {
        "V", "D", "I", "W", "E", "F", "N",
    };

    const int logLevel = static_cast<int>(aLogLevel);
    LUME_ASSERT(logLevel >= 0 && logLevel < LOG_LEVEL_COUNT);

    return aShortName ? LOG_LEVEL_NAMES_SHORT[logLevel] : LOG_LEVEL_NAMES[logLevel];
}


Logger::Logger(bool aDefaultOutputs)
{
    if (aDefaultOutputs)
    {
        addOutput(createLoggerConsoleOutput());
        addOutput(createLoggerDebugOutput());

        // Not writing to a file by default.
        //addOutput(createLoggerFileOutput("./logfile.txt"));
    }
}

Logger::~Logger() = default;

void Logger::vlog(LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aFormat, va_list aArgs)
{
    LUME_ASSERT_MSG(aLogLevel != LogLevel::None, "'None' is not a valid log level for writing to the log.");

    if (mLogLevel > aLogLevel) { return; }

    char buffer[1024];
    vsnprintf(buffer, 1024, aFormat, aArgs);
    buffer[1023] = '\0';

    for (auto &output : mOutputs)
    {
        output->write(aLogLevel, aFilename, aLinenumber, buffer);
    }
}

FORMAT_FUNC(5, 6) void Logger::log(LogLevel aLogLevel, const char *aFilename, int aLinenumber, FORMAT_ATTRIBUTE const char *aFormat, ...)
{
    va_list vl;
    va_start(vl, aFormat);
    vlog(aLogLevel, aFilename, aLinenumber, aFormat, vl);
    va_end(vl);
}

FORMAT_FUNC(6, 7) bool Logger::logAssert(const char *aFilename, int aLinenumber, bool expression, const char *expressionString, FORMAT_ATTRIBUTE const char *aFormat, ...)
{
    if (expression)
    {
        return true;
    }

    va_list vl;
    va_start(vl, aFormat);

    char buffer[1024];
    vsnprintf(buffer, 1024, aFormat, vl);
    buffer[1023] = '\0';

    va_end(vl);

    log(LogLevel::Fatal, aFilename, aLinenumber, "Assert failed (%s). %s", expressionString, buffer);
    return false;
}


ILogger::LogLevel Logger::getLogLevel() const
{
    return mLogLevel;
}

void Logger::setLogLevel(LogLevel aLogLevel)
{
    mLogLevel = aLogLevel;
}

void Logger::addOutput(std::unique_ptr<IOutput> aOutput)
{
    if (aOutput)
    {
        std::lock_guard<std::mutex> guard(mLoggerMutex);
        mOutputs.push_back(std::move(aOutput));
    }
}


namespace
{
Logger sLoggerInstance(true); // Global logger instance.

std::set<std::string> sRegisteredOnce; // Global set of ids used by the LUME_ONCE macro.
std::mutex sOnceMutex;
} // empty namespace

ILogger &getLogger()
{
    return sLoggerInstance;
}


bool checkOnce(const char *aId)
{
    std::lock_guard<std::mutex> guard(sOnceMutex);

    size_t size = sRegisteredOnce.size();
    sRegisteredOnce.insert(std::string(aId));

    // Something was inserted if the size changed.
    return size != sRegisteredOnce.size();
}

void checkOnceReset()
{
    std::lock_guard<std::mutex> guard(sOnceMutex);
    sRegisteredOnce.clear();
}


} // lume
