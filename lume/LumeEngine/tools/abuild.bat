@rem Copyright (c) 2024 Huawei Device Co., Ltd.
@rem Licensed under the Apache License, Version 2.0 (the "License");
@rem you may not use this file except in compliance with the License.
@rem You may obtain a copy of the License at
@rem
@rem     http://www.apache.org/licenses/LICENSE-2.0
@rem
@rem Unless required by applicable law or agreed to in writing, software
@rem distributed under the License is distributed on an "AS IS" BASIS,
@rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@rem See the License for the specific language governing permissions and
@rem limitations under the License.

@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
if NOT DEFINED ANDROID_NDK_HOME (
    echo ANDROID_NDK_HOME not set. fail.
    EXIT /B
)
where /q ninja
IF ERRORLEVEL 1 (
    ECHO ninja missing
    EXIT /B
)

rem set STL=c++_static
set STL=c++_shared
set BUILD_FLAGS=-DCORE_BUILD_2D=1 -DCORE_BUILD_VULKAN=1 -DCORE_BUILD_GL=0
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_BUILD_GLES=1 -DCORE_DEV_ENABLED=0 -DCORE_TESTS_ENABLED=0
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_GL_DEBUG=0 -DCORE_VALIDATION_ENABLED=0 -DCORE_VULKAN_VALIDATION_ENABLED=0
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_EMBEDDED_ASSETS_ENABLED=1 -DCORE_GPU_TIMESTAMP_QUERIES_ENABLED=0
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_DEBUG_GPU_RESOURCE_IDS=0 -DCORE_DEBUG_COMMAND_MARKERS_ENABLED=0 -DCORE_DEBUG_MARKERS_ENABLED=0

rem hack for ranlib rename error
set BUILD_FLAGS=%BUILD_FLAGS% -DCMAKE_CXX_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
set BUILD_FLAGS=%BUILD_FLAGS% -DCMAKE_CXX_ARCHIVE_FINISH="cd ."
set BUILD_FLAGS=%BUILD_FLAGS% -DCMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
set BUILD_FLAGS=%BUILD_FLAGS% -DCMAKE_C_ARCHIVE_FINISH="cd ."

rem set ABI=armeabi-v7a
rem set ABI=arm64-v8a
rem set ABI=x86_64
rem set NDK=C:\Android\ndk-bundle
rem set NDK=C:\Android\ndk\21.0.6113669
set NDK=%ANDROID_NDK_HOME%

rem set MAKE_PROGRAM=C:\Android\cmake\3.10.2.4988404\bin\ninja.exe
set MAKE_PROGRAM=ninja.exe

set SOURCE_ROOT=%CD%/../
set BUILD_ROOT=%SOURCE_ROOT%build/android
set BUILD_TYPE=Release
set VERSION=26

rem fix backslashes
set "NDK=%NDK:\=/%"
set "SOURCE_ROOT=%SOURCE_ROOT:\=/%"
set "BUILD_ROOT=%BUILD_ROOT:\=/%"
for %%i IN (armeabi-v7a arm64-v8a) do (
    call :doit %%i || goto :error
)
goto :eof

:doit
set ABI=%1
cmake -H%SOURCE_ROOT% -B%BUILD_ROOT%/%BUILD_TYPE%/%ABI% -DANDROID_ABI=%ABI% -DANDROID_PLATFORM=android-%VERSION% -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=%BUILD_ROOT%/%BUILD_TYPE%/obj/%ABI% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DANDROID_NDK=%NDK% -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI=%ABI% -DCMAKE_SYSTEM_VERSION=%VERSION% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_ANDROID_NDK=%NDK% -DCMAKE_TOOLCHAIN_FILE=%NDK%/build/cmake/android.toolchain.cmake -G Ninja -DCMAKE_MAKE_PROGRAM=%MAKE_PROGRAM% -DANDROID_STL=%STL% -DCORE_UNPACKED_LIB_DIR=%BUILD_ROOT%/unpacked2 %BUILD_FLAGS% || goto :error
pushd 
cd %BUILD_ROOT%\%BUILD_TYPE%\%ABI%
echo %CD%
echo ninja clean
ninja GenerateAllDefines  || goto :error
rem ninja AGPEngineDLL
rem ninja AGPEngine2DDLL
rem ninja AGPEngine3DDLL
rem ninja AGPEngine2D3DDLL
::echo cd %BUILD_ROOT%\%BUILD_TYPE%\obj\%ABI%\
::echo c:nm -C -D libAGPEngineDLL.so
popd
exit /B 0

:error
exit /B !ERRORLEVEL!