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

#ifndef OHOS_RENDER_3D_WIDGET_TRACE_H
#define OHOS_RENDER_3D_WIDGET_TRACE_H

#include <hitrace_meter.h>

#if defined(WIDGET_TRACE_ENABLE) && (WIDGET_TRACE_ENABLE == 1)
#define WIDGET_SCOPED_TRACE_ARGS(fmt, ...) HitraceMeterFmtScoped trace(HITRACE_TAG_GRAPHIC_AGP, fmt, ##__VA_ARGS__)
#define WIDGET_SCOPED_TRACE(value) HitraceScoped trace(HITRACE_TAG_GRAPHIC_AGP, value)
#else
#define WIDGET_SCOPED_TRACE(fmt, ...)
#define WIDGET_SCOPED_TRACE(value)
#endif

#endif // OHOS_RENDER_3D_WIDGET_TRACE_H
