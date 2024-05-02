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

#ifndef API_SCENE_NAMESPACE_H
#define API_SCENE_NAMESPACE_H

#include <base/util/uid.h>

#if defined(SCENE_PLUGIN_VERBOSE_LOGS)
#define SCENE_PLUGIN_VERBOSE_LOG CORE_LOG_D
#else
#define SCENE_PLUGIN_VERBOSE_LOG(...)
#endif // !NDEBUG

#define SCENE_NS scene_plugin

#define SCENE_BEGIN_NAMESPACE() namespace SCENE_NS {
#define SCENE_END_NAMESPACE() } // namespace scene_plugin

SCENE_BEGIN_NAMESPACE()
static constexpr BASE_NS::Uid UID_SCENE_PLUGIN { "ad98b964-a219-4b65-8033-209f44ba3b24" };
static constexpr BASE_NS::Uid UID_SCENEWIDGET_PLUGIN { "6d34dca1-a1c9-4068-aeb9-a6dac6f9206d" };
SCENE_END_NAMESPACE()

#endif
