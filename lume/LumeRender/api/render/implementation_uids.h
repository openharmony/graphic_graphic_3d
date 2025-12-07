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
static constexpr BASE_NS::Uid UID_RENDER_PLUGIN { "751e3526-5c85-47ab-bf8e-8a6d091a9f31" };

static constexpr BASE_NS::Uid UID_RENDER_CONTEXT { "b8b3eeeb-f34c-4238-9cef-6d202de28f36" };
static constexpr BASE_NS::Uid UID_RENDER_DATA_CONFIGURATION_LOADER { "39069120-6594-40db-8dfd-2ff2d9502901" };
static constexpr BASE_NS::Uid UID_RENDER_NODE_POST_PROCESS_UTIL { "0e0cc2df-463b-4dd7-88a0-012fa1b8cd04" };
static constexpr BASE_NS::Uid UID_RENDER_NODE_COPY_UTIL { "cf855681-2446-42fc-bc6b-8c439871f8db" };
static constexpr BASE_NS::Uid UID_RENDER_NODE_POST_PROCESS_INTERFACE_UTIL { "1c811e6c-a427-4849-930b-5130da61706c" };

static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_BLOOM { "89b83608-c910-433e-9fd8-22f3ef64eabd" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_BLUR { "65ec929c-c38c-4d4e-b0a0-62fffb55d338" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_COMBINED { "e3ac92f0-ccb2-4573-9536-ac1f24d8e47b" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_DOF { "134815e1-915e-40d9-b230-2467fb946cf7" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_FLARE { "eeea01ce-c094-4559-a909-ea44172c7d43" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_FXAA { "7ae334d0-4da8-4e17-842a-d4fe3a4ef61d" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_MOTION_BLUR { "b77a0280-8669-4492-a9a1-96a16c79e64f" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_TAA { "8589eded-49de-4128-852a-f3e521561f93" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_UPSCALE { "81321991-8314-4c74-bc6a-487eed297550" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_TFS_UPSCALER { "2995896d-bce7-4166-82b7-7c5786afdcab" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_SSAO { "6f7db0df-648a-467d-a695-1ca13af4c712" };
static constexpr BASE_NS::Uid UID_RENDER_POST_PROCESS_CMAA2 { "0eb2d24e-5bbf-41e8-b7e7-b434938c6dba" };
RENDER_END_NAMESPACE()

#endif // API_RENDER_IMPLEMENTATION_UIDS_H