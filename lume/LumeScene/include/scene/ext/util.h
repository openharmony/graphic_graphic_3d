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

#ifndef SCENE_EXT_UTIL_H
#define SCENE_EXT_UTIL_H

#include <meta/api/future.h>
#include <meta/api/task_queue.h>

SCENE_BEGIN_NAMESPACE()

template<typename Type>
Type GetBuildArg(const META_NS::IMetadata::Ptr& d, BASE_NS::string_view name)
{
    return META_NS::GetValue(d->GetProperty<Type>(name));
}

template<typename Interface>
typename BASE_NS::shared_ptr<Interface> GetInterfaceBuildArg(
    const META_NS::IMetadata::Ptr& d, BASE_NS::string_view name)
{
    BASE_NS::shared_ptr<Interface> res;
    if (auto p = d->GetProperty<META_NS::SharedPtrIInterface>(name)) {
        res = interface_pointer_cast<Interface>(p->GetValue());
    }
    return res;
}

inline BASE_NS::string_view ComponentName(BASE_NS::string_view path)
{
    return path.substr(0, path.find_first_of('.'));
}

inline BASE_NS::string_view FullPropertyName(BASE_NS::string_view path)
{
    auto pos = path.find_first_of('.');
    if (pos != BASE_NS::string_view::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

inline BASE_NS::string_view PropertyName(BASE_NS::string_view path)
{
    auto pos = path.find_last_of('.');
    if (pos != BASE_NS::string_view::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

inline BASE_NS::string_view NormalisePath(BASE_NS::string_view path)
{
    if (path.empty() || path == "/") {
        return "/";
    }
    if (path.ends_with('/')) {
        path.remove_suffix(1);
    }
    return path;
}

inline BASE_NS::string_view ParentPath(BASE_NS::string_view path)
{
    auto pos = path.find_last_of('/');
    if (pos != BASE_NS::string_view::npos) {
        return path.substr(0, pos);
    }
    return {};
}

inline BASE_NS::string_view EntityName(BASE_NS::string_view path)
{
    auto pos = path.find_last_of('/');
    if (pos != BASE_NS::string_view::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

inline BASE_NS::string_view FirstSegment(BASE_NS::string_view path)
{
    // skip starting /
    if (path.starts_with('/')) {
        path.remove_prefix(1);
    }
    return path.substr(0, path.find_first_of('/'));
}

SCENE_END_NAMESPACE()

#endif