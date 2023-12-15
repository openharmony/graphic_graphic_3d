/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_IMPLEMENTATION_UIDS_H
#define API_RENDER_IMPLEMENTATION_UIDS_H

#include <base/util/uid.h>
#include <render/namespace.h>
RENDER_BEGIN_NAMESPACE()
static constexpr BASE_NS::Uid UID_RENDER_PLUGIN { "eac27ae8-db70-4e25-b129-459cd9bd2c22" };

static constexpr BASE_NS::Uid UID_RENDER_CONTEXT { "b8b3eeeb-f34c-4238-9cef-6d202de28f36" };
static constexpr BASE_NS::Uid UID_RENDER_DATA_CONFIGURATION_LOADER { "39069120-6594-40db-8dfd-2ff2d9502901" };
static constexpr BASE_NS::Uid UID_RENDER_NODE_POST_PROCESS_UTIL { "0e0cc2df-463b-4dd7-88a0-012fa1b8cd04" };
RENDER_END_NAMESPACE()

#endif // API_RENDER_IMPLEMENTATION_UIDS_H