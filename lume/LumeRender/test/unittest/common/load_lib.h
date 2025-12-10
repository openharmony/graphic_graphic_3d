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

#ifndef TEST_LOAD_LIB
#define TEST_LOAD_LIB

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Test {

class DynamicLibrary {
public:
    DynamicLibrary();
    ~DynamicLibrary();

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    // Load the library
    bool Load(const BASE_NS::string& libraryPath);

    // Check if library is loaded
    bool IsLoaded() const;

    // Load function
    template<typename T>
    bool LoadFunction(T& fn, const BASE_NS::string& fName);

    // Unload the library
    void Unload();

    // Get last error message
    BASE_NS::string GetLastError() const;

private:
#if defined(_WIN32)
    BASE_NS::string GetLastErrorAsString();
    HMODULE m_handle;
#else
    void* m_handle;
#endif

    BASE_NS::string m_lastError;
};

template<typename T>
bool DynamicLibrary::LoadFunction(T& fn, const BASE_NS::string& fName)
{
#if defined(_WIN32)
    fn = reinterpret_cast<T>(GetProcAddress(m_handle, fName.c_str()));
    if (!fn) {
        m_lastError = "Failed to load function '" + fName + "'. Error: " + GetLastErrorAsString();
        return false;
    }
#else
    fn = reinterpret_cast<T>(dlsym(m_handle, fName.c_str()));
    if (!fn) {
        m_lastError = "Failed to load function '" + fName + "'. Error: " + BASE_NS::string(dlerror());
        return false;
    }
#endif
    return true;
}

} // namespace Test

#endif // TEST_LOAD_LIB