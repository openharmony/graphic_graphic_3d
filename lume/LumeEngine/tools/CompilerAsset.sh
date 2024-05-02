#!/bin/bash
# Copyright (C) 2024 Huawei Device Co., Ltd.
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

set -e

COMPILER_PATH=$1
TEST_COMPILER_PATH=${COMPILER_PATH}/../LumeBinaryCompile/lumeassetcompiler
CPU_TYPE=$2
ASSETS_PATH=$3
ROOT_PATH=$4
BIN_NAME=$5
SIZE_NAME=$6
BASE_NAME=$7
COPY_PATH=$8
OUTPUT_OBJ=$9

compile_asset()
{
	echo "Lume5 Compile asset $1 $2 $3 $4 $5 $6 $7 $8 $9"
    $TEST_COMPILER_PATH/LumeAssetCompiler -linux $CPU_TYPE -extensions ".spv;.json;.lsb;.shader;.shadergs;.shadervid;.shaderpl;.rng;.gl;.gles" $ASSETS_PATH $ROOT_PATH $BIN_NAME $SIZE_NAME $BASE_NAME
    mv $OUTPUT_OBJ $COPY_PATH
}

compile_asset $COMPILER_PATH $CPU_TYPE $ASSETS_PATH $ROOT_PATH $BIN_NAME $SIZE_NAME $BASE_NAME $COPY_PATH $OUTPUT_OBJ
