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

#ifndef WIDGET_ADAPTER_3D_LOG
#define WIDGET_ADAPTER_3D_LOG
#include <hilog/log.h>
#define WIDGET_LOGE(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);
#define WIDGET_LOGW(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);
#define WIDGET_LOGI(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);
#define WIDGET_LOGD(fmt, ...) HiLogPrint(LOG_CORE, LOG_ERROR, 0xD003B00, "lume_widget", fmt, ##__VA_ARGS__);

#endif // WIDGET_ADAPTER_3D_LOG
