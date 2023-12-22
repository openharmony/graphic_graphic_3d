#ifndef IGE_FILEMONITOR_H
#define IGE_FILEMONITOR_H

/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <string>
#include <vector>
#include <map>

namespace ige
{
    
class FileMonitor
{
public:
    FileMonitor();
    ~FileMonitor();

    /** Adds path to watch list, the monitor will recursively monitor all files in this directory and it's subtree.
        @param aPath Path to directory that is being monitored, such as 'x:/images/' or './images'.
        @return True if path is succesfully added to watch list, otherwise false.
    */
    bool addPath(const std::string &aPath);

    /** Removes path from watch list, the monitor will no longer watch files in this directory or it's subtree.
        @param aPath Path to directory to be no longer monitored.
        @return True if the watch is successfully removed, otherwise false.
    */
    bool removePath(const std::string &aPath);

    /** Scans for file modifications since last call to this function.
        @param aAdded List of files that were added.
        @param aRemoved List of files that were removed.
        @param aModified List of files that were modified.
    */
    void scanModifications(std::vector<std::string>& aAdded, std::vector<std::string>& aRemoved, std::vector<std::string>& aModified);

    std::vector<std::string> getMonitoredFiles() const;

private:

    struct FileInfo
    {
        time_t timestamp;
    };

    bool addFile(const std::string &aPath);
    bool removeFile(const std::string &aPath);

    bool isWatchingDirectory(const std::string &aPath);
    bool isWatchingSubDirectory(const std::string &aPath);

    std::vector<std::string> mDirectories;
    std::map<std::string, FileInfo> mFiles;
};

} // ige

#endif // IGE_FILEMONITOR_H