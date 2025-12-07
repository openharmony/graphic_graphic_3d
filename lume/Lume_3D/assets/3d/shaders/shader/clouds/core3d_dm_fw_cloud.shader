{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    
    "shaders": [
        {
            "displayName": "Clouds",
            "variantName": "CLOUDS",
            "renderSlot": "CORE3D_RS_DM_FW_TRANSLUCENT",
            "vert": "3dshaders://shader/core3d_dm_env.vert.spv",
            "frag": "3dshaders://shader/clouds/core3d_dm_fw_cloud.frag.spv",
            "vertexInputDeclaration": "rendervertexinputdeclarations://empty.shadervid",
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
                            "enableBlend": true,
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit",
                            "srcColorBlendFactor": "src_alpha",
                            "dstColorBlendFactor": "one_minus_src_alpha",
                            "colorBlendOp": "add",
                            "srcAlphaBlendFactor": "src_alpha",
                            "dstAlphaBlendFactor": "one_minus_src_alpha",
                            "alphaBlendOp": "add"
                        }
                    ]
                }
            }
        },
        {
            "displayName": "Clouds Bindless",
            "variantName": "CLOUDS_BL",
            "renderSlot": "CORE3D_RS_DM_FW_TRANSLUCENT_BL",
            "vert": "3dshaders://shader/core3d_dm_env.vert.spv",
            "frag": "3dshaders://shader/clouds/core3d_dm_fw_cloud_bl.frag.spv",
            "vertexInputDeclaration": "rendervertexinputdeclarations://empty.shadervid",
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
                            "enableBlend": true,
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit",
                            "srcColorBlendFactor": "src_alpha",
                            "dstColorBlendFactor": "one_minus_src_alpha",
                            "colorBlendOp": "add",
                            "srcAlphaBlendFactor": "src_alpha",
                            "dstAlphaBlendFactor": "one_minus_src_alpha",
                            "alphaBlendOp": "add"
                        }
                    ]
                }
            }
        }
    ]
}