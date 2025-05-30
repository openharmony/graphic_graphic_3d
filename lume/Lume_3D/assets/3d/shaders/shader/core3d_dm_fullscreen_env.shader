{
    "compatibility_info" : {
        "version" : "22.00",
        "type" : "shader"
    },
    "category": "3D",
    "displayName": "Environment",
    "shaders": [
        {
            "displayName": "Default Environment",
            "variantName": "ENV",
            "vert": "3dshaders://shader/core3d_dm_env.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_env.frag.spv",
            "pipelineLayout": "3dpipelinelayouts://core3d_dm_env.shaderpl",
            "renderSlot": "CORE3D_RS_DM_ENV",
            "renderSlotDefaultShader": true,
            "state": {
                "rasterizationState": {
                    "enableDepthClamp": false,
                    "enableDepthBias": false,
                    "enableRasterizerDiscard": false,
                    "polygonMode": "fill",
                    "cullModeFlags": "none",
                    "frontFace": "counter_clockwise"
                },
                "depthStencilState": {
                    "enableDepthTest": true,
                    "enableDepthWrite": false,
                    "enableDepthBoundsTest": false,
                    "enableStencilTest": false,
                    "depthCompareOp": "less_or_equal"
                },
                "colorBlendState": {
                    "colorAttachments": [
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            }
        },
        {
            "displayName": "Default Environment Multiview",
            "variantName": "ENV_MV",
            "vert": "3dshaders://shader/core3d_dm_env_mv.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_env.frag.spv",
            "pipelineLayout": "3dpipelinelayouts://core3d_dm_env.shaderpl",
            "renderSlot": "CORE3D_RS_DM_ENV_MV",
            "renderSlotDefaultShader": true,
            "state": {
                "rasterizationState": {
                    "enableDepthClamp": false,
                    "enableDepthBias": false,
                    "enableRasterizerDiscard": false,
                    "polygonMode": "fill",
                    "cullModeFlags": "none",
                    "frontFace": "counter_clockwise"
                },
                "depthStencilState": {
                    "enableDepthTest": true,
                    "enableDepthWrite": false,
                    "enableDepthBoundsTest": false,
                    "enableStencilTest": false,
                    "depthCompareOp": "less_or_equal"
                },
                "colorBlendState": {
                    "colorAttachments": [
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            }
        }
    ]
}
