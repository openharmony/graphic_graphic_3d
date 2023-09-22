/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
