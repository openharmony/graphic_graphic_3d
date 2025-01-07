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

BASE_NS::string NormalizePath(BASE_NS::string_view path);
BASE_NS::string GetParentPath(BASE_NS::string_view path);
BASE_NS::string ResolvePath(BASE_NS::string_view parent, BASE_NS::string_view uri, bool allowQueryString = true);
BASE_NS::string GetRelativePath(BASE_NS::string_view path, BASE_NS::string_view relativeTo);
BASE_NS::string GetFilename(BASE_NS::string_view path);
BASE_NS::string GetExtension(BASE_NS::string_view path);
BASE_NS::string GetBaseName(BASE_NS::string_view path);
BASE_NS::unordered_map<BASE_NS::string, BASE_NS::string> GetUriParameters(BASE_NS::string_view uri);
BASE_NS::string ResolveUri(BASE_NS::string_view contextUri, BASE_NS::string_view uri, bool allowQueryString = true);

} // namespace PathUtil
UTIL_END_NAMESPACE()

#endif // API_UTIL_IPATHUTIL_H
