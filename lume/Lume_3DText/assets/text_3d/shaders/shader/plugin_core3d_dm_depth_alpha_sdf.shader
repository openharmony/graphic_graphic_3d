{
    "compatibility_info": {
        "version": "22.00",
            "type" : "shader"
    },
    "category": "3D/Material",
    "shaders": [
        {
            "displayName": "Depth Alpha SDF",
            "variantName": "DEPTH",
            "renderSlot": "CORE3D_RS_DM_DEPTH",
            "renderSlotDefaultShader": false,
            "vert": "3dshaders://shader/core3d_dm_depth_alpha.vert.spv",
            "frag": "text3dshaders://shader/plugin_core3d_dm_depth_alpha_sdf.frag.spv",
            "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_depth.shadervid",
            "pipelineLayout": "3dpipelinelayouts://core3d_dm_depth.shaderpl",
            "state": {
                "rasterizationState": {
                    "enableDepthClamp": false,
                    "enableDepthBias": false,
                    "enableRasterizerDiscard": false,
                    "polygonMode": "fill",
                    "cullModeFlags": "back",
                    "frontFace": "counter_clockwise"
                },
                "depthStencilState": {
                    "enableDepthTest": true,
                    "enableDepthWrite": true,
                    "enableDepthBoundsTest": false,
                    "enableStencilTest": false,
                    "depthCompareOp": "less_or_equal"
                },
                "colorBlendState": {
                    "colorAttachments": [
                        {
                            "colorWriteMask": "r_bit"
                        }
                    ]
                }
            }
        },
        {
            "displayName": "Depth VSM Alpha",
            "variantName": "DEPTH_VSM",
            "renderSlot": "CORE3D_RS_DM_DEPTH_VSM",
            "renderSlotDefaultShader": false,
            "vert": "3dshaders://shader/core3d_dm_depth_alpha.vert.spv",
            "frag": "text3dshaders://shader/plugin_core3d_dm_depth_vsm_alpha_sdf.frag.spv",
            "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_depth.shadervid",
            "pipelineLayout": "3dpipelinelayouts://core3d_dm_depth.shaderpl",
            "state": {
                "rasterizationState": {
                    "enableDepthClamp": false,
                    "enableDepthBias": false,
                    "enableRasterizerDiscard": false,
                    "polygonMode": "fill",
                    "cullModeFlags": "back",
                    "frontFace": "counter_clockwise"
                },
                "depthStencilState": {
                    "enableDepthTest": true,
                    "enableDepthWrite": true,
                    "enableDepthBoundsTest": false,
                    "enableStencilTest": false,
                    "depthCompareOp": "less_or_equal"
                },
                "colorBlendState": {
                    "colorAttachments": [
                        {
                            "colorWriteMask": "r_bit|g_bit"
                        }
                    ]
                }
            }
        }
    ]
}
