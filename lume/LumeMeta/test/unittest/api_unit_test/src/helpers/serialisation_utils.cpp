/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "helpers/serialisation_utils.h"

#include <algorithm>
#include <cstring>

#include <core/io/intf_file_manager.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()
namespace UTest {

MemFile::MemFile(BASE_NS::vector<uint8_t> vec) : data_(BASE_NS::move(vec)) {}

MemFile::Mode MemFile::GetMode() const
{
    return Mode::READ_WRITE;
}
void MemFile::Close()
{
    data_.clear();
}
uint64_t MemFile::Read(void* buffer, uint64_t count)
{
    size_t read = std::min<size_t>(count, data_.size() - pos_);
    if (read) {
        std::memcpy(buffer, &data_[pos_], read);
        pos_ += read;
    }
    return read;
}
uint64_t MemFile::Write(const void* buffer, uint64_t count)
{
    if (data_.size() < pos_ + count) {
        data_.resize(pos_ + count);
    }
    std::memcpy(&data_[pos_], buffer, count);
    pos_ += count;
    return count;
}
uint64_t MemFile::Append(const void* buffer, uint64_t count, uint64_t flushSize)
{
    if (data_.size() < pos_ + count) {
        data_.resize(pos_ + count);
    }
    std::memcpy(&data_[pos_], buffer, count);
    pos_ += count;
    return count;
}
uint64_t MemFile::GetLength() const
{
    return data_.size();
}
bool MemFile::Seek(uint64_t offset)
{
    bool ret = offset < data_.size();
    if (ret) {
        pos_ = offset;
    }
    return ret;
}
uint64_t MemFile::GetPosition() const
{
    return pos_;
}
BASE_NS::vector<uint8_t> MemFile::Data() const
{
    return data_;
}
void MemFile::Destroy()
{
    delete this;
}

TestSerialiser::TestSerialiser(BASE_NS::vector<uint8_t> vec) : data_(BASE_NS::move(vec)) {}

bool TestSerialiser::Export(const IObject::Ptr& object)
{
    data_.Seek(writePos_);
    auto exporter = GetObjectRegistry().Create<IFileExporter>(META_NS::ClassId::JsonExporter);
    exporter->SetInstanceIdMapping(mapping_);
    exporter->SetResourceManager(resources_);
    exporter->SetMetadata(metadata_);
    bool ret = exporter->Export(data_, object);
    writePos_ = data_.GetPosition();
    return ret;
}

void TestSerialiser::SetSerializationSettings(SerializationSettings s)
{
    auto& ctx = GetObjectRegistry().GetGlobalSerializationData();
    ctx.SetDefaultSettings(s);
}

IObject::Ptr TestSerialiser::Import()
{
    data_.Seek(readPos_);
    auto importer = GetObjectRegistry().Create<IFileImporter>(META_NS::ClassId::JsonImporter);
    importer->SetResourceManager(resources_);
    IObject::Ptr result = importer->Import(data_);
    readPos_ = data_.GetPosition();
    mapping_ = importer->GetInstanceIdMapping();
    metadata_ = importer->GetMetadata();
    return result;
}

bool TestSerialiser::LoadFile(BASE_NS::string_view path)
{
    auto f = GetTestEnv()->engine->GetFileManager().OpenFile(path);
    if (!f) {
        return false;
    }

    BASE_NS::vector<uint8_t> vec;
    vec.resize(f->GetLength());
    f->Read(vec.data(), vec.size());
    data_ = MemFile { BASE_NS::move(vec) };
    return true;
}

void TestSerialiser::Dump(BASE_NS::string_view file)
{
    WriteToFile(data_.Data(), file);
}

BASE_NS::string TestSerialiser::Get() const
{
    return BASE_NS::string(
        BASE_NS::string_view(reinterpret_cast<const char*>(data_.Data().data()), data_.Data().size()));
}

BASE_NS::vector<uint8_t> TestSerialiser::GetData() const
{
    return data_.Data();
}

BASE_NS::unordered_map<InstanceId, InstanceId> TestSerialiser::GetInstanceIdMapping() const
{
    return mapping_;
}
void TestSerialiser::SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId> map)
{
    mapping_ = BASE_NS::move(map);
}
void TestSerialiser::SetResourceManager(CORE_NS::IResourceManager::Ptr p)
{
    resources_ = BASE_NS::move(p);
}
void TestSerialiser::SetMetadata(SerMetadata m)
{
    metadata_ = BASE_NS::move(m);
}
SerMetadata TestSerialiser::GetMetadata() const
{
    return metadata_;
}

void WriteToFile(const BASE_NS::vector<uint8_t>& vec, BASE_NS::string_view file)
{
    auto f = GetTestEnv()->engine->GetFileManager().CreateFile(file);
    if (f) {
        f->Write(vec.data(), vec.size());
        f->Close();
    }
}

} // namespace UTest

META_END_NAMESPACE()
