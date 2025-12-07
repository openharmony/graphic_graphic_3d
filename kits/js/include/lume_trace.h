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

#ifndef LUME_TRACE_H
#define LUME_TRACE_H

#ifndef NO_LUME_TRACING
#ifndef __OHOS_PLATFORM__
#define NO_LUME_TRACING
#endif
#endif

#ifndef NO_LUME_TRACING

#include <hitrace_meter.h>
#include <parameters.h>

enum class TextTraceLevel {
    LUME_TRACE_LEVEL_DEFAULT,
    LUME_TRACE_LEVEL_LOW,
    LUME_TRACE_LEVEL_MIDDLE,
    LUME_TRACE_LEVEL_HIGH
};

#define LUME_TRACE(name) LumeOptionalTrace optionalTrace(name);
#define LUME_TRACE_FUNC() LumeOptionalTrace optionalTrace(__PRETTY_FUNCTION__);
#define LUME_TRACE_LEVEL(level, name) LumeOptionalTrace::TraceWithLevel(level, name, __PRETTY_FUNCTION__);

class LumeOptionalTrace {
public:
    LumeOptionalTrace(std::string traceStr)
    {
        static bool debugTraceEnable = (OHOS::system::GetIntParameter("persist.sys.graphic.openDebugTrace", 0) != 0);
        if (debugTraceEnable) {
            std::string name { "Lume#" };
            CutPrettyFunction(traceStr);
            name.append(traceStr);
            StartTrace(HITRACE_TAG_GRAPHIC_AGP | HITRACE_TAG_COMMERCIAL, name);
        }
    }

    ~LumeOptionalTrace()
    {
        static bool debugTraceEnable = (OHOS::system::GetIntParameter("persist.sys.graphic.openDebugTrace", 0) != 0);
        if (debugTraceEnable) {
            FinishTrace(HITRACE_TAG_GRAPHIC_AGP | HITRACE_TAG_COMMERCIAL);
        }
    }

    // Simplify __PRETTY_FUNCTION__ to only return class name and function name
    // case: std::unique_str<XXX::XXX::Xxx> XXX::XXX::ClassName::FunctionName()
    // retrun: ClassName::FunctionName
    static void CutPrettyFunction(std::string& str)
    {
        // find last '('
        size_t endIndex = str.rfind('(');
        if (endIndex == std::string::npos) {
            return;
        }

        // find the third ':' before '('
        auto rIter = std::make_reverse_iterator(str.begin() + endIndex);
        size_t count = 0;
        size_t startIndex = 0;
        for (; rIter != str.rend(); ++rIter) {
            if (*rIter == ':') {
                count += 1;
                if (count == 3) { // 3 means to stop iterating when reaching the third ':'
                    startIndex = str.rend() - rIter;
                    break;
                }
            }
        }
        str = str.substr(startIndex, endIndex - startIndex);
    }

    static void TraceWithLevel(TextTraceLevel level, const std::string& traceStr, std::string caller)
    {
        static int32_t systemLevel =
            std::atoi(OHOS::system::GetParameter("persist.sys.graphic.openDebugTrace", "0").c_str());
        if ((systemLevel != 0) && (systemLevel <= static_cast<int32_t>(level))) {
            CutPrettyFunction(caller);
            HITRACE_METER_FMT(HITRACE_TAG_GRAPHIC_AGP, "Text#%s %s", traceStr.c_str(), caller.c_str());
        }
    }
};

#else
#define LUME_TRACE(name)
#define LUME_TRACE_FUNC()
#define LUME_TRACE_LEVEL(level, name)
#endif
#endif
