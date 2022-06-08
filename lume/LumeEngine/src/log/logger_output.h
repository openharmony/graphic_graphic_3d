/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_LOG_LOGGER_OUTPUT_H
#define CORE_LOG_LOGGER_OUTPUT_H

#include <base/containers/string_view.h>
#include <core/log.h>
#include <core/namespace.h>

#include <string_view>
#include <ostream>

CORE_BEGIN_NAMESPACE()
/** Console output */
ILogger::IOutput::Ptr CreateLoggerConsoleOutput();

/** Debug output (On windows, this writes to output in visual studio) */
ILogger::IOutput::Ptr CreateLoggerDebugOutput();

/** File output */
ILogger::IOutput::Ptr CreateLoggerFileOutput(const BASE_NS::string_view filename);
CORE_END_NAMESPACE()

namespace LoggerUtils {
    /* Gets the filename part from the path. */
    std::string_view GetFilename(std::string_view path);
    /* Print time stamp to stream */
    void PrintTimeStamp(std::ostream& outputStream);
} // namespace LoggerUtils

#endif // CORE_LOG_LOGGER_OUTPUT_H
