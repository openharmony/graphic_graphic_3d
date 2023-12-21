#!/bin/bash
#Copyright (c) 2023 Huawei Device Co., Ltd.
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0

#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.
set -e
WORKING_DIR=$(cd "$(dirname "$0")"; pwd)
PROJECT_ROOT=${WORKING_DIR%/foundation*}
compile_shader()
{
     OPENHARMONY_DIR=$1
    if [ ! -n "$1" ] ;then
        echo "==================orig source shader=================="
    else
        echo "==================copy source shader=================="

        cp -r $OPENHARMONY_DIR/foundation/graphic/graphic_3d/lume/Lume_3D/assets/3d/* 3d_source
        cp -r $OPENHARMONY_DIR/foundation/graphic/graphic_3d/lume/Lume_3D/api/* 3d_api
        cp -r $OPENHARMONY_DIR/foundation/graphic/graphic_3d/lume/LumeRender/assets/render/* render_source
        cp -r $OPENHARMONY_DIR/foundation/graphic/graphic_3d/lume/LumeRender/api/* render_api
        cp -r $OPENHARMONY_DIR/foundation/graphic/graphic_3d/lume/LumeEngine/assets/render/* render_source
    fi

    rm -rf 3d_dest
    rm -rf render_dest
    rm -rf engine_dest

    cp -r 3d_source 3d_dest
    cp -r render_source render_dest
    cp -r engine_source engine_dest

    echo "==================build render Shader=================="
    #./LumeShaderCompiler --optimize --source render_dest/shaders --include render_api
	./LumeShaderCompiler --optimize --source $PROJECT_ROOT/foundation/graphic/graphic_3d/lume/LumeRender/assets/render/shaders --include $PROJECT_ROOT/foundation/graphic/graphic_3d/lume/LumeRender/api/
    echo "==================build 3d Shader=================="
    ./LumeShaderCompiler --optimize --source $PROJECT_ROOT/foundation/graphic/graphic_3d/lume/Lume_3D/assets/3d/shaders --include $PROJECT_ROOT/foundation/graphic/graphic_3d/lume/Lume_3D/api/ --include $PROJECT_ROOT/foundation/graphic/graphic_3d/lume/LumeRender/api/
}

compile_rofs()
{
    echo "==================build engine rofs=================="
    ./LumeAssetCompiler -linux -arm64-v8a -extensions ".spv;.json;.lsb;.shader;.shadergs;.shadervid;.shaderpl;.rng;.gl;.gles" engine_dest / BINARYDATAFORCORE SIZEOFDATAFORCORE CORE_ROFS
    mv CORE_ROFS_64.o engine_dest

    echo "==================build render rofs=================="
    ./LumeAssetCompiler -linux -arm64-v8a -extensions ".spv;.json;.lsb;.shader;.shadergs;.shadervid;.shaderpl;.rng;.gl;.gles" render_dest / BINARYDATAFORRENDER SIZEOFDATAFORRENDER RENDER_ROFS 
    mv RENDER_ROFS_64.o render_dest

    echo "==================build 3d rofs=================="
    ./LumeAssetCompiler -linux -arm64-v8a -extensions ".spv;.json;.lsb;.shader;.shadergs;.shadervid;.shaderpl;.rng;.gl;.gles" 3d_dest / BINARY_DATA_FOR_3D SIZE_OF_DATA_FOR_3D CORE3D_ROFS 
    mv CORE3D_ROFS_64.o 3d_dest

}

main()
{
    if [ "$1" == "shader" ] ;then
        compile_shader $2
        return
    fi

    if [ "$1" == "rofs" ] ;then
        compile_rofs
        return
    fi

    if [ "$1" == "all" ] ;then
        compile_shader $2
        compile_rofs
    else
        compile_shader
        compile_rofs
    fi
}

main $1
