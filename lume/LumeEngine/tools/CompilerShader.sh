#!/bin/bash
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

set -e

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

    mkdir -p $DEST_GEN_PATH
    chmod -R 775 $DEST_GEN_PATH

    cp -r ${ASSETS_PATH}/* $DEST_GEN_PATH
    if [ -z "$RENDER_INCLUDE_PATH" ];
    then
        $TOOL_PATH/LumeShaderCompiler --optimize --source $SHADER_PATH --include $INCLUDE_PATH
    else
        $TOOL_PATH/LumeShaderCompiler --optimize --source $SHADER_PATH --include $INCLUDE_PATH --include $RENDER_INCLUDE_PATH
    fi
}

compile_shader
