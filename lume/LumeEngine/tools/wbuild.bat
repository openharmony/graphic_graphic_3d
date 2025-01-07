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
set BUILD_FLAGS=-DCORE_BUILD_2D=1 -DCORE_BUILD_VULKAN=1 -DCORE_BUILD_GL=1
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_BUILD_GLES=0 -DCORE_DEV_ENABLED=0 -DCORE_TESTS_ENABLED=0
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_GL_DEBUG=0 -DCORE_VALIDATION_ENABLED=0 -DCORE_VULKAN_VALIDATION_ENABLED=0
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_EMBEDDED_ASSETS_ENABLED=1 -DCORE_GPU_TIMESTAMP_QUERIES_ENABLED=0
set BUILD_FLAGS=%BUILD_FLAGS% -DCORE_DEBUG_GPU_RESOURCE_IDS=0 -DCORE_DEBUG_COMMAND_MARKERS_ENABLED=0 -DCORE_DEBUG_MARKERS_ENABLED=0

set SOURCE_ROOT=%CD%/../
set BUILD_ROOT=%SOURCE_ROOT%build/windows

rem fix backslashes
set "SOURCE_ROOT=%SOURCE_ROOT:\=/%"
set "BUILD_ROOT=%BUILD_ROOT:\=/%"
for %%i IN (win32 win64) do (
    call :doit %%i || goto :error
)
goto :eof

:doit
set ABI=%1
if "%ABI%"=="win32" (
    set GEN="Visual Studio 15 2017"
)
if "%ABI%"=="win64" (
    set GEN="Visual Studio 15 2017 Win64"
)
cmake -H%SOURCE_ROOT% -B%BUILD_ROOT%/%ABI% -G%GEN% -DCORE_UNPACKED_LIB_DIR=%BUILD_ROOT%/unpacked2 %BUILD_FLAGS% || goto :error
pushd 
cd %BUILD_ROOT%\%ABI%
echo %CD%
cmake --build . --target GenerateAllDefines --config Release || goto :error
rem cmake --build . --target AGPEngineDLL --config Release || goto :error
rem cmake --build . --target AGPEngine2DDLL --config Release || goto :error
rem cmake --build . --target AGPEngine3DDLL --config Release || goto :error
rem cmake --build . --target AGPEngine2D3DDLL --config Release || goto :error
popd
exit /B 0

:error
exit /B !ERRORLEVEL!