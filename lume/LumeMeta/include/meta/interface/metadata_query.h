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

#ifndef META_INTERFACE_METADATA_QUERY_H
#define META_INTERFACE_METADATA_QUERY_H

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

enum class MetadataQuery {
    EXISTING,            /// Only return existing metadata objects
    CONSTRUCT_ON_REQUEST /// Create on request if not exist yet (lazy)
};

META_END_NAMESPACE()

#endif
