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

#ifndef CORE__IO__PATH_TOOLS_H
#define CORE__IO__PATH_TOOLS_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** IsRelative
 * Checks if the path is "absolute". ie. rooted. starts with "/"
 * @return true if rooted, false if not.
 */
bool IsRelative(BASE_NS::string_view path);
/** ParseUri
 * Extracts protocol and path from uri. not fully compliant parsing.
 * @return false if uri has no protocol.
 */
bool ParseUri(BASE_NS::string_view uri, BASE_NS::string_view& protocol, BASE_NS::string_view& path);
/** Normalize Path.
 * Removes dot segments from "absolute" path. if the path does not start at "/", it is changed accordingly.
 * @return Path with out dot segments, or empty if more ".." dotsegments than segments.
 */
BASE_NS::string NormalizePath(BASE_NS::string_view path);
/** GetCurrentDirectory
 * Returns current working directory.
 * @return string containing current directory
 */
BASE_NS::string GetCurrentDirectory();
#if _WIN32
/** SplitPath.
 * Splits "pathIn", to it's components. accepts "/C:/" and "C:/".
 */
void SplitPath(BASE_NS::string_view pathIn, BASE_NS::string_view& drive, BASE_NS::string_view& path,
    BASE_NS::string_view& filename, BASE_NS::string_view& ext);
#endif
CORE_END_NAMESPACE()
#endif