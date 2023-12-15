#!/bin/bash
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

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
