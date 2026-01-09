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

// internal
#include "compiler_main.h"

#ifdef WIN32

#include <Windows.h>

namespace {
/**
 * @brief Converts a windows wide string to utf8-encoded c-style null terminated string.
 *
 * @param text The input wide string to be converted.
 * @param length The number of characters in the input text.
 * @return The string in utf-8 format. Must be freed by the user.
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
 */
char* AllocateUtf8String(const wchar_t* text, const int length)
{
    // Passing 0 as size makes this just return the required size of the buffer.
    const int utf8ByteCount = WideCharToMultiByte(CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);

    // Allocate enough space for the utf-8 string + null terminator
    auto* resultUtf8String = static_cast<char*>(malloc(sizeof(char) * (utf8ByteCount + 1)));

    if (!WideCharToMultiByte(CP_UTF8, 0, text, length, resultUtf8String, utf8ByteCount, nullptr, nullptr)) {
        free(resultUtf8String);
        return {};
    }
    resultUtf8String[utf8ByteCount] = '\0';
    return resultUtf8String;
}

// RAII structure for getting command line arguments as utf-8 strings.
struct CommandLineArgs {
    int argc{};
    char** argvUtf8;

    CommandLineArgs(const int argc, PWCHAR argv[])
    {
        // Jumping through some hoops to get the command line parameters as utf8 on Windows.
        this->argc = argc;
        this->argvUtf8 = new char *[argc];
        for (int i = 0; i < argc; i++) {
            // ReSharper disable once CppDFAMemoryLeak
            argvUtf8[i] = AllocateUtf8String(argv[i], static_cast<int>(wcslen(argv[i])));
        }
    }

    ~CommandLineArgs()
    {
        // Free the allocated strings
        for (int i = 0; i < argc; i++) {
            free(argvUtf8[i]);
        }
        delete[] argvUtf8;
        argvUtf8 = nullptr;
        argc = 0;
    }
};
} // namespace

int wmain(const int argc, PWCHAR argv[])
{
    const CommandLineArgs commandLine(argc, argv);
    return CompilerMain(commandLine.argc, commandLine.argvUtf8);
}

#else

int main(const int argc, char* argv[])
{
    return CompilerMain(argc, argv);
}

#endif
