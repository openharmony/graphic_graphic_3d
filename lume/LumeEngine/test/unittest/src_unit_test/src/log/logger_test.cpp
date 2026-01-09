/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <set>
#include <string_view>

#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "io/dev/file_monitor.h"
#include "io/std_filesystem.h"
#include "log/logger.h"
#include "log/logger_output.h"

using namespace CORE_NS;

namespace {
using BASE_NS::string_view;

// a Logger class that gathers all logged messages to a string.
class StringOutput : public ILogger::IOutput {
public:
    void Write(
        ILogger::LogLevel aLogLevel, const string_view aFilename, int aLinenumber, const string_view aMessage) override
    {
        lastLogLevel = aLogLevel;
        lastFilename = aFilename;
        lastLineNumber = aLinenumber;

        outputStream << std::string_view(aMessage.data(), aMessage.size());
    }

    void Destroy() override {}

    std::string GetOutput()
    {
        return outputStream.str();
    }

    ILogger::LogLevel lastLogLevel;
    string_view lastFilename;
    int lastLineNumber;
    std::stringstream outputStream;
};

void LogStuff(ILogger& logger)
{
    logger.Log(ILogger::LogLevel::LOG_VERBOSE, "", 0, "Verbose");
    logger.Log(ILogger::LogLevel::LOG_DEBUG, "", 0, "Debug");
    logger.Log(ILogger::LogLevel::LOG_INFO, "", 0, "Info");
    logger.Log(ILogger::LogLevel::LOG_WARNING, "", 0, "Warning");
    logger.Log(ILogger::LogLevel::LOG_ERROR, "", 0, "Error");
    logger.Log(ILogger::LogLevel::LOG_FATAL, "", 0, "Fatal.");
}
} // namespace

/**
 * @tc.name: basicLogging
 * @tc.desc: Tests for Basic Logging. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_LoggerTest, basicLogging, testing::ext::TestSize.Level1)
{
    {
        StringOutput testOutput;
        auto* logOutput = &testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { logOutput });

        // Currently verbose is the default log level.
#ifdef NDEBUG
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_ERROR);
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_VERBOSE);
#else
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_VERBOSE);
#endif

        testLogger.Log(ILogger::LogLevel::LOG_VERBOSE, "", 0, "basic_info");
        ASSERT_EQ(logOutput->lastLogLevel, ILogger::LogLevel::LOG_VERBOSE);
        ASSERT_TRUE(logOutput->lastFilename.empty());
        ASSERT_EQ(logOutput->lastLineNumber, 0);
        ASSERT_EQ(logOutput->GetOutput(), "basic_info");
    }

    {
        StringOutput testOutput;
        auto* logOutput = &testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { logOutput });
#ifdef NDEBUG
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_VERBOSE);
#endif
        testLogger.Log(ILogger::LogLevel::LOG_INFO, "thisfile.x", 1234, "basic_info");
        ASSERT_EQ(logOutput->lastLogLevel, ILogger::LogLevel::LOG_INFO);
        ASSERT_EQ(logOutput->lastFilename, string_view("thisfile.x"));
        ASSERT_EQ(logOutput->lastLineNumber, 1234);
        ASSERT_EQ(logOutput->GetOutput(), "basic_info");
    }

    {
        StringOutput testOutput;
        auto* logOutput = &testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { logOutput });
#ifdef NDEBUG
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_VERBOSE);
#endif
        testLogger.Log(ILogger::LogLevel::LOG_ERROR, "", 0, "basic_info %s %d", "str", 123);
        ASSERT_EQ(logOutput->lastLogLevel, ILogger::LogLevel::LOG_ERROR);
        ASSERT_TRUE(logOutput->lastFilename.empty());
        ASSERT_EQ(logOutput->lastLineNumber, 0);
        ASSERT_EQ(logOutput->GetOutput(), "basic_info str 123");
    }
}

/**
 * @tc.name: logLevels
 * @tc.desc: Tests for Log Levels. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_LoggerTest, logLevels, testing::ext::TestSize.Level1)
{
    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });

        testLogger.SetLogLevel(ILogger::LogLevel::LOG_VERBOSE);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_VERBOSE);
        LogStuff(testLogger);

        {
            // In this scope only errors are logged.
            ::Test::LogLevelScope scope(&testLogger, ILogger::LogLevel::LOG_ERROR);
            LogStuff(testLogger);
        }

        // Normal logging again.
        LogStuff(testLogger);

#if defined(__OHOS_PLATFORM__)
        // need notice
#else
        ASSERT_EQ(
            testOutput.GetOutput(), "VerboseDebugInfoWarningErrorFatal.ErrorFatal.VerboseDebugInfoWarningErrorFatal.");
#endif
    }

    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_VERBOSE);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_VERBOSE);
        LogStuff(testLogger);
        ASSERT_EQ(testOutput.GetOutput(), "VerboseDebugInfoWarningErrorFatal.");
    }

    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_DEBUG);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_DEBUG);
        LogStuff(testLogger);
        ASSERT_EQ(testOutput.GetOutput(), "DebugInfoWarningErrorFatal.");
    }

    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_INFO);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_INFO);
        LogStuff(testLogger);
        ASSERT_EQ(testOutput.GetOutput(), "InfoWarningErrorFatal.");
    }

    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_WARNING);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_WARNING);
        LogStuff(testLogger);
        ASSERT_EQ(testOutput.GetOutput(), "WarningErrorFatal.");
    }

    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_ERROR);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_ERROR);
        LogStuff(testLogger);
        ASSERT_EQ(testOutput.GetOutput(), "ErrorFatal.");
    }

    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_FATAL);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_FATAL);
        LogStuff(testLogger);
        ASSERT_EQ(testOutput.GetOutput(), "Fatal.");
    }

    {
        StringOutput testOutput;
        Logger testLogger(false);
        testLogger.AddOutput(ILogger::IOutput::Ptr { &testOutput });
        testLogger.SetLogLevel(ILogger::LogLevel::LOG_NONE);
        ASSERT_EQ(testLogger.GetLogLevel(), ILogger::LogLevel::LOG_NONE);
        LogStuff(testLogger);
        ASSERT_EQ(testOutput.GetOutput(), "");
    }
}

/**
 * @tc.name: VlogTest
 * @tc.desc: Tests for Vlog Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_loggerTest, VlogTest, testing::ext::TestSize.Level1)
{
    Logger testLogger(false);
    [](ILogger& testLogger, int dummy, ...) {
        va_list vl;
        va_start(vl, dummy);
        testLogger.VLogOnce("id", ILogger::LogLevel::LOG_VERBOSE, "", 0, "basic_info", vl);
        va_end(vl);
    }(testLogger, 0);

    [](ILogger& testLogger, int dummy, ...) {
        va_list vl;
        va_start(vl, dummy);
        EXPECT_FALSE(testLogger.VLogAssert("", 0, false, "", "basic_info", vl));
        va_end(vl);
    }(testLogger, 0);

    testLogger.CheckOnceReset();
    EXPECT_FALSE(testLogger.LogAssert("", 0, false, "", "basic_info %s", ""));
}

/**
 * @tc.name: miscTest
 * @tc.desc: Tests for Misc Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_loggerTest, miscTest, testing::ext::TestSize.Level1)
{
    const Logger testLogger(false);
    EXPECT_FALSE(testLogger.GetInterface(UID_LOGGER) != nullptr);
    EXPECT_TRUE(testLogger.GetInterface(ILogger::UID) != nullptr);
    Logger refLogger(false);
    refLogger.Ref();
    refLogger.Unref();
}

/**
 * @tc.name: OutputTest
 * @tc.desc: Tests for Output Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_loggerTest, OutputTest, testing::ext::TestSize.Level1)
{
    LoggerUtils::GetFilename("1.txt");
    LoggerUtils::GetFilename("test/1.txt");
    ILogger::IOutput::Ptr logOut = CreateLoggerFileOutput("log1.txt");
    EXPECT_TRUE(logOut != nullptr);
    logOut->Write(ILogger::LogLevel::LOG_NONE, "", 0, "");
    logOut->Write(ILogger::LogLevel::LOG_NONE, "file1", 0, "");
}

/**
 * @tc.name: LogIoTest
 * @tc.desc: Tests for Log Io Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_loggerTest, LogIoTest, testing::ext::TestSize.Level1)
{
    ILogger::IOutput::Ptr logOut = CreateLoggerConsoleOutput();
    EXPECT_TRUE(logOut != nullptr);
    logOut->Write(ILogger::LogLevel::LOG_NONE, "test://io/test_directory/file1.txt", 0, "");
    logOut->Write(ILogger::LogLevel::LOG_INFO, "", 0, "info");
    logOut->Write(ILogger::LogLevel::LOG_DEBUG, "", 0, "debug");
    logOut->Write(ILogger::LogLevel::LOG_WARNING, "", 0, "warning");
}
