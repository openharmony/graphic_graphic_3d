/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__IO__STD_FILE_H
#define CORE__IO__STD_FILE_H

#include <cstdio>

#include <base/containers/string_view.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** File system file.
 * IFile implementation that wraps standard FILE handle and operations around it.
 */
class StdFile final : public IFile {
public:
    StdFile() = default;
    ~StdFile() override;

    Mode GetMode() const override;

    // Open an existing file, fails if the file does not exist.
    bool Open(BASE_NS::string_view path, Mode mode);

    // Create a new file, the existing file will be overridden.
    bool Create(BASE_NS::string_view path, Mode mode);

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
    FILE* file_ { nullptr };
};
CORE_END_NAMESPACE()

#endif // CORE__IO__STD_FILE_H
