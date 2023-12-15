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

CMAKE_ROOT=$PROJECT_ROOT/prebuilts/cmake/linux-x86/bin
echo $CMAKE_ROOT

OHOS_NDK=$PROJECT_ROOT/prebuilts/clang/ohos/linux-x86_64/llvm
LLVM_DIR=$OHOS_NDK
echo $LLVM_DIR

NINJA_HOME=$PROJECT_ROOT/prebuilts/build-tools/linux-x86/bin
echo $NINJA_HOME

export PATH="$NINJA_HOME:$PATH"

rm -rf $WORKING_DIR/build

Compile()
{
    PROJECT_DIR=$WORKING_DIR/build/outpus/$1

    mkdir -p $PROJECT_DIR/Strip
    $CMAKE_ROOT/cmake -H$WORKING_DIR -B$PROJECT_DIR -G Ninja
    ninja -C $PROJECT_DIR  -f build.ninja

    chmod 775 $PROJECT_DIR/LumeAssetCompiler
    cp -r $PROJECT_DIR/LumeAssetCompiler $PROJECT_DIR/Strip
    $LLVM_DIR/bin/llvm-strip -s $PROJECT_DIR/Strip/LumeAssetCompiler
    rm -rf $WORKING_DIR/../test/RofsBuild/LumeAssetCompiler
    cp $PROJECT_DIR/Strip/LumeAssetCompiler $WORKING_DIR/../test/RofsBuild/LumeAssetCompiler
    cd $WORKING_DIR/../test/RofsBuild/
    #./compile_rofs.sh rofs
    cd -
}

echo "compile start x86_64"

Compile x86_64
