#ifndef IGE_FILEMONITOR_H
#define IGE_FILEMONITOR_H

/**
* Copyright (C) 2018 Huawei Technologies Co, Ltd.
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