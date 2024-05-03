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

#ifndef CORE__IO__DEV__FILEMONITOR_H
#define CORE__IO__DEV__FILEMONITOR_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
class FileMonitor {
public:
    explicit FileMonitor(IFileManager&);
    /** Adds path to watch list, the monitor will recursively monitor all files in this directory and it's subtree.
     * @param path Path to directory that is being monitored, such as 'file://x:/images/' or 'file://./images' or
     * 'shaders://'.
     * @return True if path is succesfully added to watch list, otherwise false.
     */
    bool AddPath(BASE_NS::string_view path);

    /** Removes path from watch list, the monitor will no longer watch files in this directory or it's subtree.
     * @param path Path to directory to be no longer monitored.
     * @return True if the watch is successfully removed, otherwise false.
     */
    bool RemovePath(BASE_NS::string_view path);

    /** Scans for file modifications since last call to this function.
     * @param added List of files that were added.
     * @param removed List of files that were removed.
     * @param modified List of files that were modified.
     */
    void ScanModifications(BASE_NS::vector<BASE_NS::string>& added, BASE_NS::vector<BASE_NS::string>& removed,
        BASE_NS::vector<BASE_NS::string>& modified);

    /** Get list of currently monitored files.
     * @return List of monitored files (absolute filepaths).
     */
    BASE_NS::vector<BASE_NS::string> GetMonitoredFiles() const;

private:
    struct FileInfo {
        uint64_t timestamp { 0 };
        // removed is a file left over from last scan. which means it should be removed.
        enum {
            REMOVED = 0,
            NOCHANGE = 1,
            MODIFIED = 2,
            ADDED = 3,
        } state { REMOVED };
    };

    bool IsWatchingDirectory(BASE_NS::string_view path);
    bool IsWatchingSubDirectory(BASE_NS::string_view path);
    void RecursivelyCollectAllFiles(BASE_NS::string& path);
    static void CleanPath(BASE_NS::string_view, BASE_NS::string&);

    BASE_NS::vector<BASE_NS::string> directories_;
    BASE_NS::unordered_map<BASE_NS::string, FileInfo> files_;
    IFileManager& fileManager_;
    BASE_NS::string pathTmp_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__DEV__FILEMONITOR_H
