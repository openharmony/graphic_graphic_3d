#!/bin/bash
#Copyright (c) 2023 Huawei Device Co., Ltd.
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.
set -e
WORKING_DIR=$(cd "$(dirname "$0")"; pwd)
PROJECT_ROOT=${WORKING_DIR%/foundation*}
echo ${PROJECT_ROOT}
CMAKE_ROOT=$PROJECT_ROOT/prebuilts/cmake/linux-x86/bin
echo $CMAKE_ROOT
 
OHOS_NDK=$PROJECT_ROOT/prebuilts/clang/ohos/linux-x86_64/llvm
LLVM_DIR=$PROJECT_ROOT/prebuilts/clang/ohos/linux-x86_64/llvm
echo $LLVM_DIR
 
NINJA_HOME=$PROJECT_ROOT/prebuilts/build-tools/linux-x86/bin
echo $NINJA_HOME
export PATH="$NINJA_HOME:$PATH"
 
DEST_GEN_PATH=$1
 
compile()
{
    PROJECT_DIR=$DEST_GEN_PATH
    if [ -d "$PROJECT_DIR" ]; then
        rm -rf $PROJECT_DIR
        echo "Clean Output"
    fi
	mkdir -p $PROJECT_DIR
    chmod -R 775 $PROJECT_DIR
    $CMAKE_ROOT/cmake --version

    NINJA_TOOL=ninja
    if [ $HW_NINJA_NAME ]; then
        echo "Lume shader Compile use ninja_back"
        NINJA_TOOL=$NINJA_HOME/$HW_NINJA_NAME 
    else
        echo "Lume Shader Compile use ninja"
        NINJA_TOOL=$NINJA_HOME/ninja
    fi

    $CMAKE_ROOT/cmake -H$WORKING_DIR -B$PROJECT_DIR -DCMAKE_CXX_FLAGS_RELEASE=-O2 \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DOHOS_NDK=${LLVM_DIR} -DCMAKE_TOOLCHAIN_FILE=$WORKING_DIR/shader.compile.toolchain.cmake \
    -G Ninja -DCMAKE_MAKE_PROGRAM=$NINJA_TOOL

#-DCMAKE_SYSROOT=$LLVM_DIR/lib/x86_64-unknow-linux-gnu
    $NINJA_TOOL -C $PROJECT_DIR  -f build.ninja

    mkdir $PROJECT_DIR/Strip
    chmod 775 $PROJECT_DIR/LumeShaderCompiler
    cp -r $PROJECT_DIR/LumeShaderCompiler $PROJECT_DIR/Strip
    #$LLVM_DIR/bin/llvm-strip -s $PROJECT_DIR/Strip/LumeShaderCompiler
    rm -rf $WORKING_DIR/../test/RofsBuild/LumeShaderCompiler
    #cp $PROJECT_DIR/Strip/LumeShaderCompiler $WORKING_DIR/../test/RofsBuild/LumeShaderCompiler
    cd $WORKING_DIR/../test/RofsBuild/
    #./compile_rofs.sh shader
    cd -
}
 
echo "compile start shader x86_64"
 
compile
