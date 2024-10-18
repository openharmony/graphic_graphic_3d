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

#ifndef API_UTIL_PATHUTIL_H
#define API_UTIL_PATHUTIL_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>

#include <util/namespace.h>

UTIL_BEGIN_NAMESPACE()
namespace PathUtil {

::string NormalizePath(::string_view path);
::string GetParentPath(::string_view path);
::string ResolvePath(::string_view parent, ::string_view uri, bool allowQueryString = true);
::string GetRelativePath(::string_view path, ::string_view relativeTo);
::string GetFilename(::string_view path);
::string GetExtension(::string_view path);
::string GetBaseName(::string_view path);
::unordered_map<::string, ::string> GetUriParameters(::string_view uri);
::string ResolveUri(::string_view contextUri, ::string_view uri, bool allowQueryString = true);

} // namespace PathUtil
UTIL_END_NAMESPACE()

#endif // API_UTIL_IPATHUTIL_H
