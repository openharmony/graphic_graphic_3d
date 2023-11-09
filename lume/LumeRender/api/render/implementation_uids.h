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
static constexpr BASE_NS::Uid UID_RENDER_PLUGIN { "6b15f334-872b-4963-acc8-691b30de7501" };

static constexpr BASE_NS::Uid UID_RENDER_CONTEXT { "b8b3eeeb-f34c-4238-9cef-6d202de28f36" };
static constexpr BASE_NS::Uid UID_RENDER_DATA_CONFIGURATION_LOADER { "39069120-6594-40db-8dfd-2ff2d9502901" };
RENDER_END_NAMESPACE()

#endif // API_RENDER_IMPLEMENTATION_UIDS_H