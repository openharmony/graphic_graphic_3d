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

#include "Logger.h"

#include <cstdarg>
#include <set>
#include <string>

namespace lume {
namespace {
constexpr int LOG_BUFFER_SIZE = 1024;

// Note: these must match the LogLevel enum.
constexpr int LOG_LEVEL_COUNT = 7;

constexpr const char* LOG_LEVEL_NAMES[LOG_LEVEL_COUNT] = {
    "Verbose",
    "Debug",
    "Info",
    "Warning",
    "Error",
    "Fatal",
    "None",
};

constexpr const char* LOG_LEVEL_NAMES_SHORT[LOG_LEVEL_COUNT] = {
    "V",
    "D",
    "I",
    "W",
    "E",
    "F",
    "N",
};
} // namespace

const char* Logger::GetLogLevelName(LogLevel logLevel, bool shortName)
{
    const int logLevelIndex = static_cast<int>(logLevel);
    LUME_ASSERT(logLevelIndex >= 0 && logLevelIndex < LOG_LEVEL_COUNT);

    return shortName ? LOG_LEVEL_NAMES_SHORT[logLevelIndex] : LOG_LEVEL_NAMES[logLevelIndex];
}

Logger::Logger(bool defaultOutputs)
{
    if (defaultOutputs) {
        AddOutput(CreateLoggerConsoleOutput());
        AddOutput(CreateLoggerDebugOutput());

        // Not writing to a file by default.
        // This can be enabled with: addOutput(createLoggerFileOutput("./logfile.txt"))
    }
}

Logger::~Logger() = default;

void Logger::Vlog(LogLevel logLevel, const char* filename, int linenumber, const char* format, va_list args)
{
    LUME_ASSERT_MSG(logLevel != LogLevel::NONE, "'NONE' is not a valid log level for writing to the log.");

    if (mLogLevel > logLevel) {
        return;
    }

    char buffer[LOG_BUFFER_SIZE];
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_WANT_SECURE_LIB__)
    auto ret = vsnprintf_s(buffer, sizeof(buffer), format, args);
    if (ret < 0) {
        return;
    }
#else
    auto ret = vsnprintf(buffer, sizeof(buffer), format, args);
    if (ret < 0) {
        return;
    }
#endif

    for (auto& output : mOutputs) {
        output->Write(logLevel, filename, linenumber, buffer);
    }
}

void Logger::Write(LogLevel logLevel, const char* filename, int linenumber, const char* buffer)
{
    for (auto& output : mOutputs) {
        output->Write(logLevel, filename, linenumber, buffer);
    }
}

FORMAT_FUNC(5, 6)
void Logger::Log(LogLevel logLevel, const char* filename, int linenumber, FORMAT_ATTRIBUTE const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    Vlog(logLevel, filename, linenumber, format, vl);
    va_end(vl);
}

FORMAT_FUNC(6, 7)
bool Logger::LogAssert(const char* filename, int linenumber, bool expression, const char* expressionString,
    FORMAT_ATTRIBUTE const char* format, ...)
{
    if (expression) {
        return true;
    }

    va_list vl;
    va_start(vl, format);

    char buffer[LOG_BUFFER_SIZE];
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_WANT_SECURE_LIB__)
    vsnprintf_s(buffer, sizeof(buffer), format, vl);
#else
    vsnprintf(buffer, sizeof(buffer), format, vl);
#endif

    va_end(vl);

    Log(LogLevel::FATAL, filename, linenumber, "Assert failed (%s). %s", expressionString, buffer);
    return false;
}

ILogger::LogLevel Logger::GetLogLevel() const
{
    return mLogLevel;
}

void Logger::SetLogLevel(LogLevel logLevel)
{
    mLogLevel = logLevel;
}

void Logger::AddOutput(std::unique_ptr<IOutput> output)
{
    if (output) {
        std::lock_guard<std::mutex> guard(mLoggerMutex);
        mOutputs.push_back(std::move(output));
    }
}

namespace {
Logger g_loggerInstance(true); // Global logger instance.

std::set<std::string> g_registeredOnce; // Global set of ids used by the LUME_ONCE macro.
std::mutex g_onceMutex;
} // namespace

ILogger& GetLogger()
{
    return g_loggerInstance;
}

bool CheckOnce(const char* aId)
{
    std::lock_guard<std::mutex> guard(g_onceMutex);

    size_t size = g_registeredOnce.size();
    g_registeredOnce.insert(std::string(aId));

    // Something was inserted if the size changed.
    return size != g_registeredOnce.size();
}

void CheckOnceReset()
{
    std::lock_guard<std::mutex> guard(g_onceMutex);
    g_registeredOnce.clear();
}
} // namespace lume
