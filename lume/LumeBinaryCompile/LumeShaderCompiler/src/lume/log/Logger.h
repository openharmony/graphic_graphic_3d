#ifndef LUME_LOG_LOGGER_H
#define LUME_LOG_LOGGER_H

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

#include <lume/Log.h>

#include <mutex>
#include <vector>
#include <memory>


namespace lume
{

class Logger : public ILogger
{
public:
    static const char* getLogLevelName(LogLevel aLogLevel, bool aShortName);

    Logger(bool aDefaultOutputs);
    virtual ~Logger();

    void vlog(LogLevel aLogLevel, const char *aFilename, int aLinenumber, const char *aFormat, va_list aArgs) override;
    FORMAT_FUNC(5, 6) void log(LogLevel aLogLevel, const char *aFilename, int aLinenumber, FORMAT_ATTRIBUTE const char *aFormat, ...) override;

    FORMAT_FUNC(6, 7) bool logAssert(const char *aFilename, int aLinenumber, bool expression, const char *expressionString, FORMAT_ATTRIBUTE const char *aFormat, ...) override;

    LogLevel getLogLevel() const override;
    void setLogLevel(LogLevel aLogLevel) override;

    void addOutput(std::unique_ptr<IOutput> aOutput) override;

private:
    LogLevel mLogLevel = LogLevel::Verbose;
    std::mutex mLoggerMutex;

    std::vector< std::unique_ptr<IOutput> > mOutputs;
};


} // lume

#endif // LUME_LOG_LOGGER_H
