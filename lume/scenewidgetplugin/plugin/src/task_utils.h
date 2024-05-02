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

#ifndef SCENEPLUGIN_UTILS_H
#define SCENEPLUGIN_UTILS_H

#include <scene_plugin/namespace.h>

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>

SCENE_BEGIN_NAMESPACE()

template<typename Func, typename... Args>
decltype(auto) MakeTask(Func&& f, Args&&... args)
{
    return META_NS::MakeCallbackSafe<META_NS::ITaskQueueTask>(
        BASE_NS::forward<Func>(f), BASE_NS::forward<Args>(args)...);
}

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_UTILS_H
