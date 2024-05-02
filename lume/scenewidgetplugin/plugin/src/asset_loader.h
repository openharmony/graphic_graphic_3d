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

#ifndef SCENEPLUGIN_ASSETLOADER_H
#define SCENEPLUGIN_ASSETLOADER_H

#include <scene_plugin/interface/intf_asset_loader.h>

#include <3d/intf_graphics_context.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/math/mathf.h>
#include <render/intf_render_context.h>

SCENE_BEGIN_NAMESPACE()

class AssetManager;

IAssetLoader::Ptr CreateAssetLoader(AssetManager& assetManager, RENDER_NS::IRenderContext& renderContext,
    CORE3D_NS::IGraphicsContext& graphicsContext, IEntityCollection& ec, BASE_NS::string_view src,
    BASE_NS::string_view contextUri);

struct PathUtil {
public:
    static BASE_NS::string NormalizePath(BASE_NS::string_view path)
    {
        BASE_NS::string res;
        if (path[0] != '/') {
            res.reserve(path.size() + 1);
            res.push_back('/');
        } else {
            res.reserve(path.size());
        }
        while (!path.empty()) {
            if (path[0] == '/') {
                if (res.empty()) {
                    res.push_back('/');
                }
                path = path.substr(1);
                continue;
            }
            auto pos = path.find_first_of("/", 0);
            BASE_NS::string_view sub = path.substr(0, pos);
            if (sub == ".") {
                path = path.substr(pos);
                continue;
            } else if (sub == "..") {
                if ((!res.empty()) && (res.back() == '/')) {
                    res.resize(res.size() - 1);
                }
                if (auto p = res.find_last_of('/'); BASE_NS::string::npos != p) {
                    res.resize(p);
                } else {
                    if (res.empty()) {
                        // trying to back out of root. (ie. invalid path)
                        return "";
                    }
                    res.clear();
                }
                if (pos == BASE_NS::string::npos) {
                    res.push_back('/');
                    break;
                }
            } else {
                res.append(sub);
            }
            if (pos == BASE_NS::string::npos) {
                break;
            } else {
                res.push_back('/');
            }
            path = path.substr(pos);
        }
        if (res[0] != '/') {
            res.insert(0, "/");
        }
        return res;
    }

    static BASE_NS::string GetParentPath(BASE_NS::string_view path)
    {
        if (path.size() > 1 && path[path.size() - 1] == '/' && path[path.size() - 2] != '/') { // 2 array index
            // Allow (ignore) trailing '/' for folders.
            path = path.substr(0, path.size() - 1);
        }

        const size_t separatorPos = path.rfind('/');
        if (separatorPos == BASE_NS::string::npos) {
            return "";
        } else {
            return BASE_NS::string(path.substr(0, separatorPos + 1));
        }
    }

    static BASE_NS::string ResolvePath(BASE_NS::string_view parent, BASE_NS::string_view uri, bool allowQueryString)
    {
        size_t queryPos = allowQueryString ? BASE_NS::string::npos : uri.find('?');
        BASE_NS::string_view path = (BASE_NS::string::npos != queryPos) ? uri.substr(0, queryPos) : uri;

        if (parent.empty()) {
            return BASE_NS::string(path);
        }

        if (path.empty()) {
            return BASE_NS::string(parent);
        }

        if (path[0] == '/') {
            path = path.substr(1);
        } else if (path.find("://") != path.npos) {
            return BASE_NS::string(path);
        }

        // NOTE: Resolve always assumes the parent path is a directory even if there is no '/' in the end
        if (parent.back() == '/') {
            return parent + path;
        } else {
            return parent + "/" + path;
        }
    }

    static BASE_NS::string GetRelativePath(BASE_NS::string_view path, BASE_NS::string_view relativeTo)
    {
        // remove the common prefix
        {
            int lastSeparator = -1;

            for (size_t i = 0, iMax = BASE_NS::Math::min(path.size(), relativeTo.size()); i < iMax; ++i) {
                if (path[i] != relativeTo[i]) {
                    break;
                }
                if (path[i] == '/') {
                    lastSeparator = int(i);
                }
            }

            path.remove_prefix(lastSeparator + 1);
            relativeTo.remove_prefix(lastSeparator + 1);
        }

        if (path[1] == ':' && relativeTo[1] == ':' && path[0] != relativeTo[0]) {
            // files are on different drives
            return BASE_NS::string(path);
        }

        // count and remove the directories left in relative_to
        auto directoriesCount = 0;
        {
            auto nextSeparator = relativeTo.find('/');
            while (nextSeparator != BASE_NS::string::npos) {
                relativeTo.remove_prefix(nextSeparator + 1);
                nextSeparator = relativeTo.find('/');
                ++directoriesCount;
            }
        }

        BASE_NS::string relativePath = "";
        for (auto i = 0, iMax = directoriesCount; i < iMax; ++i) {
            relativePath.append("../");
        }

        relativePath.append(path);

        return relativePath;
    }

    static BASE_NS::string GetFilename(BASE_NS::string_view path)
    {
        if (!path.empty() && path[path.size() - 1] == '/') {
            // Return a name also for folders.
            path = path.substr(0, path.size() - 1);
        }

        size_t cutPos = path.find_last_of("\\/");
        if (BASE_NS::string::npos != cutPos) {
            return BASE_NS::string(path.substr(cutPos + 1));
        } else {
            return BASE_NS::string(path);
        }
    }

    static BASE_NS::string GetExtension(BASE_NS::string_view path)
    {
        size_t fileExtCut = path.rfind('.');
        if (fileExtCut != BASE_NS::string::npos) {
            size_t queryCut = path.find('?', fileExtCut);
            if (queryCut != BASE_NS::string::npos) {
                return BASE_NS::string(path.substr(fileExtCut + 1, queryCut));
            } else {
                return BASE_NS::string(path.substr(fileExtCut + 1, queryCut));
            }
        }
        return "";
    }

    static BASE_NS::string GetBaseName(BASE_NS::string_view path)
    {
        auto filename = GetFilename(path);
        size_t fileExtCut = filename.rfind(".");
        if (BASE_NS::string::npos != fileExtCut) {
            filename.erase(fileExtCut);
        }
        return filename;
    }

    static BASE_NS::unordered_map<BASE_NS::string, BASE_NS::string> GetUriParameters(BASE_NS::string_view uri)
    {
        const size_t queryPos = uri.find('?');
        if (queryPos != BASE_NS::string::npos) {
            BASE_NS::unordered_map<BASE_NS::string, BASE_NS::string> params;
            size_t paramStartPos = queryPos;
            while (paramStartPos < uri.size()) {
                size_t paramValuePos = uri.find('=', paramStartPos + 1);
                size_t paramEndPos = uri.find('&', paramStartPos + 1);
                if (paramEndPos == BASE_NS::string::npos) {
                    paramEndPos = uri.size();
                }
                if (paramValuePos != BASE_NS::string::npos && paramValuePos < paramEndPos) {
                    auto key = uri.substr(paramStartPos + 1, paramValuePos - paramStartPos - 1);
                    auto value = uri.substr(paramValuePos + 1, paramEndPos - paramValuePos - 1);
                    params[key] = value;
                } else {
                    auto key = uri.substr(paramStartPos + 1, paramEndPos - paramStartPos - 1);
                    params[key] = key;
                }
                paramStartPos = paramEndPos;
            }
            return params;
        }
        return {};
    }

    static BASE_NS::string ResolveUri(
        BASE_NS::string_view contextUri, BASE_NS::string_view uri, bool allowQueryString = true)
    {
        return ResolvePath(GetParentPath(contextUri), uri, allowQueryString);
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_ASSETLOADER_H
