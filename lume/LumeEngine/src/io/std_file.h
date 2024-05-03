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

#ifndef CORE__IO__STD_FILE_H
#define CORE__IO__STD_FILE_H

#include <fstream>

#include <base/containers/string_view.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** File system file.
 * IFile implementation that wraps standard FILE handle and operations around it.
 */
class StdFile final : public IFile {
public:
    StdFile(Mode mode, std::fstream&& stream);
    ~StdFile() override;

    Mode GetMode() const override;

    // Open an existing file, fails if the file does not exist.
    static IFile::Ptr Open(BASE_NS::string_view path, Mode mode);

    // Create a new file, the existing file will be overridden.
    static IFile::Ptr Create(BASE_NS::string_view path, Mode mode);

    // Close file.
    void Close() override;

    uint64_t Read(void* buffer, uint64_t count) override;

    uint64_t Write(const void* buffer, uint64_t count) override;

    uint64_t GetLength() const override;

    bool Seek(uint64_t offset) override;

    uint64_t GetPosition() const override;

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    bool IsValidPath(const BASE_NS::string_view path);

    Mode mode_ { Mode::INVALID };
    mutable std::fstream file_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__STD_FILE_H
