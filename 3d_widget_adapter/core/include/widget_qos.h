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

#ifndef OHOS_RENDER_3D_WIDGET_QOS_H
#define OHOS_RENDER_3D_WIDGET_QOS_H

#include <qos.h>
#include "3d_widget_adapter_log.h"

class Widget3DQosScoped {
public:
    inline Widget3DQosScoped(const std::string &value) : mMsg(value)
    {
        int ret = OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
        WIDGET_LOGD("%s SetThreadQos: %d", mMsg.c_str(), ret);
    }

    inline ~Widget3DQosScoped()
    {
        int ret = OHOS::QOS::ResetThreadQos();
        WIDGET_LOGD("%s ResetThreadQoS: %d", mMsg.c_str(), ret);
    }
private:
    std::string mMsg;
};

#endif // OHOS_RENDER_3D_WIDGET_QOS_H
