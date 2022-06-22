/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef WIDGET_ADAPTER_3D_LOG
#define WIDGET_ADAPTER_3D_LOG
#include <hilog/log.h>
#define WIDGET_LOGE(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);
#define WIDGET_LOGW(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);
#define WIDGET_LOGI(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);
#define WIDGET_LOGD(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);

#endif // WIDGET_ADAPTER_3D_LOG
