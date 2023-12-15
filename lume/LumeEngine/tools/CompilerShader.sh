#!/bin/bash
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

set -e
WORKING_DIR=$(cd "$(dirname "$0")"; pwd)
PROJECT_ROOT=${WORKING_DIR%/LumeEngine*}
echo ${PROJECT_ROOT}

TOOL_PATH=$3
SHADER_PATH=$4
INCLUDE_PATH=$5

DEST_GEN_PATH=$1
ASSETS_PATH=$2

RENDER_INCLUDE_PATH=""

if [ $# -eq 6 ];
then
    RENDER_INCLUDE_PATH=$6
fi

compile_shader()
{
	echo "Lume4 Compile shader $1 $2 $3 $4"
    if [ -d "$DEST_GEN_PATH" ]; then
        rm -rf $DEST_GEN_PATH
        echo "Clean Output"
    fi
    TEST_TOOL_PATH=$TOOL_PATH/../LumeBinaryCompile/LumeShaderCompiler
    mkdir -p $DEST_GEN_PATH
    chmod -R 775 $DEST_GEN_PATH

    cp -r ${ASSETS_PATH}/* $DEST_GEN_PATH
    if [ -z "$RENDER_INCLUDE_PATH" ];
    then
        $TEST_TOOL_PATH/LumeShaderCompiler --optimize --source $SHADER_PATH --include $PROJECT_ROOT/LumeRender/api/
    else
        $TEST_TOOL_PATH/LumeShaderCompiler --optimize --source $SHADER_PATH --include $PROJECT_ROOT/Lume_3D/api/ --include $PROJECT_ROOT/LumeRender/api/
    fi
}

compile_shader
