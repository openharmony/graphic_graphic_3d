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

#include "test_runner_ohos_system.h"

#include <cstdlib>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

CORE_BEGIN_NAMESPACE()
/** Minimal stub for IClassRegister. */
class ClassRegisterStub final : public IClassRegister {
public:
    const IInterface* GetInterface(const BASE_NS::Uid&) const override
    {
        return nullptr;
    }
    IInterface* GetInterface(const BASE_NS::Uid&) override
    {
        return nullptr;
    }
    void Ref() override
    {}
    void Unref() override
    {}
    void RegisterInterfaceType(const InterfaceTypeInfo&) override
    {}
    void UnregisterInterfaceType(const InterfaceTypeInfo&) override
    {}
    BASE_NS::array_view<const InterfaceTypeInfo* const> GetInterfaceMetadata() const override
    {
        return {};
    }
    const InterfaceTypeInfo& GetInterfaceMetadata(const BASE_NS::Uid&) const override
    {
        static const InterfaceTypeInfo nullInfo{{}, {}, "", nullptr, nullptr};
        return nullInfo;
    }
    IInterface* GetInstance(const BASE_NS::Uid&) const override
    {
        return nullptr;
    }

protected:
    ~ClassRegisterStub() override = default;
};

/** Minimal stub for IFileManager. */
class FileManagerStub final : public IFileManager {
public:
    const IInterface* GetInterface(const BASE_NS::Uid&) const override
    {
        return nullptr;
    }
    IInterface* GetInterface(const BASE_NS::Uid&) override
    {
        return nullptr;
    }
    void Ref() override
    {}
    void Unref() override
    {}
    IDirectory::Entry GetEntry(BASE_NS::string_view) override
    {
        return {};
    }
    IFile::Ptr OpenFile(BASE_NS::string_view) override
    {
        return {};
    }
    IFile::Ptr OpenFile(BASE_NS::string_view, IFile::Mode) override
    {
        return {};
    }
    IFile::Ptr CreateFile(BASE_NS::string_view) override
    {
        return {};
    }
    bool DeleteFile(BASE_NS::string_view) override
    {
        return false;
    }
    bool FileExists(BASE_NS::string_view) const override
    {
        return false;
    }
    IDirectory::Ptr OpenDirectory(BASE_NS::string_view) override
    {
        return {};
    }
    IDirectory::Ptr CreateDirectory(BASE_NS::string_view) override
    {
        return {};
    }
    bool DeleteDirectory(BASE_NS::string_view) override
    {
        return false;
    }
    bool DirectoryExists(BASE_NS::string_view) const override
    {
        return false;
    }
    bool Rename(BASE_NS::string_view, BASE_NS::string_view) override
    {
        return false;
    }
    bool RegisterFilesystem(BASE_NS::string_view, IFilesystem::Ptr) override
    {
        return false;
    }
    void UnregisterFilesystem(BASE_NS::string_view) override
    {}
    void RegisterAssetPath(BASE_NS::string_view) override
    {}
    void UnregisterAssetPath(BASE_NS::string_view) override
    {}
    bool CheckExistence(BASE_NS::string_view) override
    {
        return false;
    }
    bool RegisterPath(BASE_NS::string_view, BASE_NS::string_view, bool) override
    {
        return false;
    }
    void UnregisterPath(BASE_NS::string_view, BASE_NS::string_view) override
    {}
    IFilesystem::Ptr CreateROFilesystem(const void* const, uint64_t) override
    {
        return {};
    }
    IFilesystem* GetFilesystem(BASE_NS::string_view) const override
    {
        return nullptr;
    }

protected:
    ~FileManagerStub() override = default;
};

/** Stub implementation of IPluginRegister for unit tests that don't need the full engine. */
class PluginRegisterStub final : public IPluginRegister {
public:
    BASE_NS::array_view<const IPlugin* const> GetPlugins() const override
    {
        return {};
    }
    bool LoadPlugins(const BASE_NS::array_view<const BASE_NS::Uid>) override
    {
        return true;
    }
    void UnloadPlugins(const BASE_NS::array_view<const BASE_NS::Uid>) override
    {}
    IClassRegister& GetClassRegister() const override
    {
        static ClassRegisterStub stub;
        return stub;
    }
    void RegisterTypeInfo(const ITypeInfo&) override
    {}
    void UnregisterTypeInfo(const ITypeInfo&) override
    {}
    BASE_NS::array_view<const ITypeInfo* const> GetTypeInfos(const BASE_NS::Uid&) const override
    {
        return {};
    }
    void AddListener(ITypeInfoListener&) override
    {}
    void RemoveListener(const ITypeInfoListener&) override
    {}
    void RegisterPluginPath(const BASE_NS::string_view) override
    {}
    IFileManager& GetFileManager() override
    {
        static FileManagerStub stub;
        return stub;
    }

protected:
    ~PluginRegisterStub() override = default;
};

IPluginRegister& GetPluginRegister()
{
    static PluginRegisterStub stub;
    return stub;
}
CORE_END_NAMESPACE()

testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new BASE_NS::UTest::TestRunnerEnv);
