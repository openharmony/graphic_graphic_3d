#ifndef LUME_LOG_LOGGER_H
#define LUME_LOG_LOGGER_H

/**
* Copyright (C) 2018 Huawei Technologies Co, Ltd.
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
