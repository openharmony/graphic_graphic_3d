#!/bin/bash
# Copyright (c) 2023 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

WORKING_DIR=$(cd "$(dirname "$0")"; pwd)
PROJECT_ROOT=${WORKING_DIR%/foundation*}
#PROJECT_ROOT=${WORKING_DIR%/zwq*}
echo ${PROJECT_ROOT}
if [ ! -d "$WORKING_DIR/3rdparty/khronos" ]; then
  mkdir -p $WORKING_DIR/3rdparty/khronos
  cp -r  $PROJECT_ROOT/third_party/openGLES/api/GL $WORKING_DIR/3rdparty/khronos/gl
  cp -r  $PROJECT_ROOT/third_party/openGLES/extensions/KHR $WORKING_DIR/3rdparty/khronos/KHR
fi
#CMAKE_ROOT=$WORKING_DIR/../build_tools/cmake-3.22.0-linux-x86_64/bin
#CMAKE_ROOT=$WORKING_DIR/../../../prebuilts/cmake/linux-x86/bin
CMAKE_ROOT=$PROJECT_ROOT/prebuilts/cmake/linux-x86/bin
echo $CMAKE_ROOT

#OHOS_NDK=$WORKING_DIR/../build_tools/llvm
OHOS_NDK=$PROJECT_ROOT/prebuilts/clang/ohos/linux-x86_64/llvm
#OHOS_NDK_=$WORKING_DIR/../../../prebuilts/clang/ohos/linux-x86_64/llvm
LLVM_DIR=$OHOS_NDK
echo $LLVM_DIR

#NINJA_HOME=$WORKING_DIR/../build_tools/ninja_1.10.2
#NINJA_HOME=$WORKING_DIR/../../../prebuilts/build-tools/linux-x86/bin
NINJA_HOME=$PROJECT_ROOT/prebuilts/build-tools/linux-x86/bin
echo $NINJA_HOME

export PATH="$NINJA_HOME:$PATH"

#export PATH="${LLVM_DIR}/bin/lld:$PATH"
rm -rf $WORKING_DIR/build

Compile()
{
    PROJECT_DIR=$WORKING_DIR/build/outpus/$1

    $CMAKE_ROOT/cmake --version

    $CMAKE_ROOT/cmake -H$WORKING_DIR -B$PROJECT_DIR -DCMAKE_CXX_FLAGS_RELEASE=-O2 \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DOHOS_NDK=${LLVM_DIR} -DCMAKE_TOOLCHAIN_FILE=$WORKING_DIR/shader.compile.toolchain.cmake \
    -G Ninja
#-DCMAKE_SYSROOT=$LLVM_DIR/lib/x86_64-unknow-linux-gnu
    ninja -C $PROJECT_DIR  -f build.ninja

    mkdir $PROJECT_DIR/Strip
    chmod 775 $PROJECT_DIR/LumeShaderCompiler 
    cp -r $PROJECT_DIR/LumeShaderCompiler $PROJECT_DIR/Strip
    $LLVM_DIR/bin/llvm-strip -s $PROJECT_DIR/Strip/LumeShaderCompiler
    rm -rf $WORKING_DIR/../test/RofsBuild/LumeShaderCompiler
    cp $PROJECT_DIR/Strip/LumeShaderCompiler $WORKING_DIR/../test/RofsBuild/LumeShaderCompiler
    cd $WORKING_DIR/../test/RofsBuild/
    #./compile_rofs.sh shader
    cd -
}

echo "compile start x86_64"

Compile x86_64
