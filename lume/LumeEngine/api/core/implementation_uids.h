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

#ifndef API_CORE_IMPLEMENTATION_UIDS_H
#define API_CORE_IMPLEMENTATION_UIDS_H

#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
static constexpr BASE_NS::Uid UID_GLOBAL_FACTORY { "f54fb8a3-9810-4411-ad05-c0f0a02d3ad3" };
static constexpr BASE_NS::Uid UID_ENGINE_FACTORY { "5d0a5fb3-f23d-457f-8de1-2ee1a7c981f4" };
static constexpr BASE_NS::Uid UID_FRUSTUM_UTIL { "9018fd26-016a-45a3-bb5f-66973e25df01" };
static constexpr BASE_NS::Uid UID_LOGGER { "415bb937-25cd-4071-97c3-59d6c9ef6756" };
static constexpr BASE_NS::Uid UID_PERFORMANCE_FACTORY { "02bce79d-6693-4bc0-bfa6-09f036b1a9fe" };
static constexpr BASE_NS::Uid UID_TASK_QUEUE_FACTORY { "4a80f3a3-2f41-4885-93f2-837dd610807d" };
static constexpr BASE_NS::Uid UID_SYSTEM_GRAPH_LOADER { "85601148-8233-41ad-8df4-732339b0738d" };
static constexpr BASE_NS::Uid UID_FILESYSTEM_API_FACTORY { "10dc7690-e5e6-4bfb-b3a7-88c83c7bfe4a" };
static constexpr BASE_NS::Uid UID_FILE_MONITOR { "f4155f47-0641-4c11-8785-cc14714f93bc" };
static constexpr BASE_NS::Uid UID_FILE_MANAGER { "e493104f-02ef-4009-ab1d-2cec4859f382" };
CORE_END_NAMESPACE()

#endif // API_CORE_IMPLEMENTATION_UIDS_H