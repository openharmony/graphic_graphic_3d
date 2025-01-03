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

#ifndef SCENE_BASE_NAMESPACE_H
#define SCENE_BASE_NAMESPACE_H

#include <base/util/uid.h>

#if defined(SCENE_VERBOSE_LOGS)
#define SCENE_VERBOSE_LOG CORE_LOG_D
#else
#define SCENE_VERBOSE_LOG(...)
#endif // !NDEBUG

#define SCENE_NS Scene

#define SCENE_BEGIN_NAMESPACE() namespace SCENE_NS {
#define SCENE_END_NAMESPACE() } // namespace Scene

SCENE_BEGIN_NAMESPACE()
static constexpr BASE_NS::Uid UID_SCENE_PLUGIN { "ad98b964-a219-4b65-8033-209f44ba3b24" };
SCENE_END_NAMESPACE()

#endif
