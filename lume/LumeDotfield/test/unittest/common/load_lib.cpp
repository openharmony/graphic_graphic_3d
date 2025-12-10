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

#include "load_lib.h"

namespace Test {

DynamicLibrary::DynamicLibrary() : m_handle(nullptr) {}

DynamicLibrary::~DynamicLibrary()
{
    Unload();
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept : m_handle(other.m_handle)
{
    other.m_handle = nullptr;
}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept
{
    if (this != &other) {
        Unload();
        m_handle = other.m_handle;

        other.m_handle = nullptr;
    }
    return *this;
}

bool DynamicLibrary::Load(const BASE_NS::string& libraryPath)
{
    Unload(); // Unload any previously loaded library

    // Try to load with and without extension
#if defined(_WIN32)
    m_handle = LoadLibraryA(libraryPath.c_str());
    if (!m_handle) {
        BASE_NS::string withExtension = libraryPath + ".dll";
        m_handle = LoadLibraryA(withExtension.c_str());
    }
    if (!IsLoaded()) {
        m_lastError = "Failed to load library '" + libraryPath + "'. Error: " + GetLastErrorAsString();
        return false;
    }
#else
    m_handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (!m_handle) {
        BASE_NS::string withExtension = libraryPath + ".so";
        m_handle = dlopen(withExtension.c_str(), RTLD_LAZY);
    }
    if (!IsLoaded()) {
        m_lastError = "Failed to load library '" + libraryPath + "'. Error: " + BASE_NS::string(dlerror());
        return false;
    }
#endif
    return true;
}

bool DynamicLibrary::IsLoaded() const
{
    return m_handle != nullptr ? true : false;
}

void DynamicLibrary::Unload()
{
    if (IsLoaded()) {
#if defined(_WIN32)
        FreeLibrary(m_handle);
#else
        dlclose(m_handle);
#endif
        m_handle = nullptr;
    }
}

BASE_NS::string DynamicLibrary::GetLastError() const
{
    return m_lastError;
}

#if defined(_WIN32)
BASE_NS::string DynamicLibrary::GetLastErrorAsString()
{
    // Get the error message ID
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return "";
    }

    // Get message from error message ID
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    BASE_NS::string message(messageBuffer, size);

    LocalFree(messageBuffer);

    return BASE_NS::string(message + "(" + BASE_NS::to_string(errorMessageID) + ")");
}
#endif

} // namespace Test