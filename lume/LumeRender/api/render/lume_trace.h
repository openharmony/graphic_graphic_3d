/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_LUME_TRACE_H
#define OHOS_RENDER_LUME_TRACE_H

#include <hitrace_meter.h>

#if defined(__GNUC__) || defined(__clang__)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNC_SIG __FUNCSIG__
#else
#define FUNC_SIG __func__
#endif

#define LUME_TRACE_ARGS(fmt, ...) HitraceMeterFmtScoped trace(HITRACE_TAG_GRAPHIC_AGP, fmt, ##__VA_ARGS__)
#define LUME_TRACE(value) HitraceScoped trace(HITRACE_TAG_GRAPHIC_AGP, value)
#define LUME_TRACE_FUNC() LUME_TRACE_ARGS("%s %s %d", FUNC_SIG, __FILE__, __LINE__)

#endif  // OHOS_RENDER_LUME_TRACE_H
