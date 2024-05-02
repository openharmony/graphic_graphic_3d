/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <core/namespace.h>
#include <base/containers/string_view.h>

CORE_BEGIN_NAMESPACE()
CORE_PUBLIC BASE_NS::string_view GetVersion() { return ""; }
CORE_PUBLIC BASE_NS::string_view GetVersionRev() { return ""; }
CORE_PUBLIC BASE_NS::string_view GetVersionBranch() { return ""; }
#ifdef NDEBUG
CORE_PUBLIC bool IsDebugBuild() { return true; }
#else
CORE_PUBLIC bool IsDebugBuild() { return false; }
#endif
CORE_END_NAMESPACE()
