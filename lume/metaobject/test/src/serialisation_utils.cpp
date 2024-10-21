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

#include "serialisation_utils.h"

#include <algorithm>
#include <cstring>

#include <core/io/intf_file_manager.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

META_BEGIN_NAMESPACE()

MemFile::MemFile(BASE_NS::vector<char> vec) : data_(BASE_NS::move(vec)) {}

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
    pos_ += read;
    return read;
}
uint64_t MemFile::Write(const void* buffer, uint64_t count)
{
    if (data_.size() < pos_ + count) {
        data_.resize(pos_ + count);
    }
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
BASE_NS::vector<char> MemFile::Data() const
{
    return data_;
}
void MemFile::Destroy()
{
    delete this;
}

TestSerialiser::TestSerialiser(BASE_NS::vector<char> vec) : data_(BASE_NS::move(vec)) {}

bool TestSerialiser::Export(const IObject::Ptr& object)
{
    data_.Seek(writePos_);
    auto exporter = GetObjectRegistry().Create<IFileExporter>(META_NS::ClassId::JsonExporter);
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
    IObject::Ptr result = importer->Import(data_);
    readPos_ = data_.GetPosition();
    return result;
}

bool TestSerialiser::LoadFile(BASE_NS::string_view path)
{
    auto f = CORE_NS::GetPluginRegister().GetFileManager().OpenFile(path);
    if (!f) {
        return false;
    }

    BASE_NS::vector<char> vec;
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
    return BASE_NS::string(BASE_NS::string_view(data_.Data().data(), data_.Data().size()));
}

void WriteToFile(const BASE_NS::vector<char>& vec, BASE_NS::string_view file)
{
    auto f = CORE_NS::GetPluginRegister().GetFileManager().CreateFile(file);
    if (f) {
        f->Write(vec.data(), vec.size());
        f->Close();
    }
}

META_END_NAMESPACE()