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

#include "log_util.h"

namespace Test {

LogLevelScope::LogLevelScope(CORE_NS::ILogger* logger, CORE_NS::ILogger::LogLevel logLevel) : logger_(logger)
{
    if (logger_) {
        // Save the current log level and set a new one.
        oldLevel_ = logger_->GetLogLevel();
        logger_->SetLogLevel(logLevel);
    }
}

LogLevelScope::~LogLevelScope()
{
    if (logger_) {
        // Restore previous log level.
        logger_->SetLogLevel(oldLevel_);
    }
}

#if defined(__ANDROID__)

namespace {
int fd{};
}  // namespace

void StartOutputFileRedirect(const std::string& filePath, bool append)
{
    LOGI("Redirecting test output to: '%s'", filePath.c_str());

    if (!append) {
        if (remove(filePath.c_str()) != 0) {
            LOGW("Could not delete old output file.");
        }
    }

    fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    assert(fd >= 0);
    dup2(fd, 1);
    dup2(fd, 2);
}

void StopOutputFileRedirect()
{
    fflush(stdout);
    fflush(stderr);
    close(fd);
}

void PrintLogFromFile(const std::string& filePath)
{
    const int maxLogSize = 1024;  // buffer size

    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        LOGE("Failed to open file: %s", filePath.c_str());
        return;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        for (size_t i = 0; i < line.length(); i += maxLogSize) {
            std::string chunk = line.substr(i, maxLogSize);
            LOGI("%s", chunk.c_str());
        }
    }

    inFile.close();
}

//
// Redirecting stdout and stderr with a thread.
// from: https://stackoverflow.com/a/42715692
//
namespace {
int pfd[2];
pthread_t loggingThread;
}  // namespace

void* LoggingFunction(void*)
{
    ssize_t readSize;
    char buf[1024];

    while ((readSize = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if (buf[readSize - 1] == '\n') {
            --readSize;
        }
        buf[readSize] = 0;                                    // add null-terminator
        __android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);  // Set any log level you want
    }

    return 0;
}

// run this function to redirect your output to android log
int RunLoggingThread()
{
    setvbuf(stdout, 0, _IOLBF, 0);  // make stdout line-buffered
    setvbuf(stderr, 0, _IONBF, 0);  // make stderr unbuffered

    // create the pipe and redirect stdout and stderr
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    // spawn the logging thread
    if (pthread_create(&loggingThread, 0, LoggingFunction, 0) == -1) {
        return -1;
    }
    pthread_detach(loggingThread);
    return 0;
}
#endif  // #if defined(__ANDROID__)

//
// OHOS log support
//
#if defined(__OHOS__)

namespace {
int fd{};
}  // namespace

void StartOutputFileRedirect(const std::string& filePath, bool append)
{
    LOGI("Redirecting test output to: '%{public}s'", filePath.c_str());

    if (!append) {
        if (remove(filePath.c_str()) != 0) {
            LOGW("Could not delete old output file.");
        }
    }

    fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    assert(fd >= 0);
    dup2(fd, 1);
    dup2(fd, 2);
}

void StopOutputFileRedirect()
{
    fflush(stdout);
    fflush(stderr);
    close(fd);
}

void PrintLogFromFile(const std::string& filePath)
{
    const int maxLogSize = 1024;  // buffer size

    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        LOGE("Failed to open file: %{public}s", filePath.c_str());
        return;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        for (size_t i = 0; i < line.length(); i += maxLogSize) {
            std::string chunk = line.substr(i, maxLogSize);
            LOGI("%{public}s", chunk.c_str());
        }
    }
    inFile.close();
}

#endif  // #if defined(__OHOS__)

}  // namespace Test
