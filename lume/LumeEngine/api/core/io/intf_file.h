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

#ifndef API_CORE_IO_IFILE_H
#define API_CORE_IO_IFILE_H

#include <cstdint>

#include <base/containers/unique_ptr.h>
#include <base/namespace.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** @ingroup group_io_ifile */
/** IFile provides methods for working with files, such as reading and writing etc. */
class IFile {
public:
    /** Enumeration for file access mode */
    enum class Mode {
        /** Invalid access */
        INVALID,
        /** Read only access */
        READ_ONLY,
        /** Read and write access */
        READ_WRITE
    };

    /** Retrieves whether file can be read / written.
     *  @return Mode of the file, either read only or read / write.
     */
    virtual Mode GetMode() const = 0;

    /** Close the file. */
    virtual void Close() = 0;

    /** Read a sequence of bytes from file.
     *  @param buffer Buffer that retrieves data from file.
     *  @param count Number of bytes to read.
     *  @return Total number of bytes read.
     */
    virtual uint64_t Read(void* buffer, uint64_t count) = 0;

    /** Write a sequence of bytes to file.
     *  @param buffer Buffer that contains the data to be written.
     *  @param count Number of bytes to write.
     *  @return Total number of bytes written.
     */
    virtual uint64_t Write(const void* buffer, uint64_t count) = 0;

    /** Gets the total length of the file.
     *   @return Total length of the file in bytes.
     */
    virtual uint64_t GetLength() const = 0;

    /** Seeks to given offset in file data.
     *  @param offset Target offset in bytes.
     *  @return True if the seek was successful, otherwise false.
     */
    virtual bool Seek(uint64_t offset) = 0;

    /** Returns the current offset in file data.
     *  @return Current offset in bytes.
     */
    virtual uint64_t GetPosition() const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IFile* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IFile, Deleter>;

protected:
    IFile() = default;
    virtual ~IFile() = default;
    virtual void Destroy() = 0;
};
CORE_END_NAMESPACE()

#endif // API_CORE_IO_IFILE_H
