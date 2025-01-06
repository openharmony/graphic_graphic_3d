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

#ifndef META_BASE_PLUGIN_HEADER
#define META_BASE_PLUGIN_HEADER

#include <base/util/uid.h>

#include <meta/base/namespace.h>
#include <meta/base/version.h>

META_BEGIN_NAMESPACE()

/**
 * @brief UID for the meta object library plugin, used to identify the dynamic library.
 */
constexpr BASE_NS::Uid META_OBJECT_PLUGIN_UID { "532f7256-8849-472a-933a-d73efc232b01" };
constexpr const char* META_VERSION_STRING = "2.0";
constexpr Version META_VERSION(META_VERSION_STRING);

META_END_NAMESPACE()

#endif
