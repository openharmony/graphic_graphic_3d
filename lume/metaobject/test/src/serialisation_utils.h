/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef META_TEST_SERIALISATION_UTILS_HEADER
#define META_TEST_SERIALISATION_UTILS_HEADER

#include <base/containers/vector.h>
#include <core/io/intf_file.h>
#include <core/log.h>

#include <meta/interface/intf_object.h>
#include <meta/interface/serialization/intf_global_serialization_data.h>

META_BEGIN_NAMESPACE()

struct MemFile : public CORE_NS::IFile {
    explicit MemFile(BASE_NS::vector<char> vec = {});

    Mode GetMode() const override;
    void Close() override;
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    uint64_t GetLength() const override;
    bool Seek(uint64_t offset) override;
    uint64_t GetPosition() const override;

    BASE_NS::vector<char> Data() const;
private:
    void Destroy() override;
    BASE_NS::vector<char> data_;
    size_t pos_ {};
};

struct TestSerialiser {
    explicit TestSerialiser(BASE_NS::vector<char> data = {});
    
    bool Export(const IObject::Ptr& object);
    IObject::Ptr Import();

    template<typename Interface>
    bool Export(const BASE_NS::shared_ptr<Interface>& p)
    {
        return Export(interface_pointer_cast<IObject>(p));
    }

    template<typename Interface>
    typename Interface::Ptr Import()
    {
        return interface_pointer_cast<Interface>(Import());
    }

    bool LoadFile(BASE_NS::string_view path);
    BASE_NS::string Get() const;

    void SetSerializationSettings(SerializationSettings s);

private:
    MemFile data_;
    std::size_t writePos_ {};
    std::size_t readPos_ {};
};

void WriteToFile(const BASE_NS::vector<char>&, BASE_NS::string_view file);

META_END_NAMESPACE()

#endif