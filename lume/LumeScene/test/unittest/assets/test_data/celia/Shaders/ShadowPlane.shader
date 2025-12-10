{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
    "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
    "pipelineLayout": "3dpipelinelayouts://core3d_dm_fw.shaderpl",
    "frag": "test_assets://celia/generated/shaders/from_graph/assets/ShadowPlane/custom_core3d_dm_fw.frag.spv",
    "shaders": [
        {
            "displayName": "Default",
            "variantName": "TRANSLUCENT_FW",
            "renderSlot": "CORE3D_RS_DM_FW_TRANSLUCENT",
            "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag": "test_assets://celia/generated/shaders/from_graph/assets/ShadowPlane/custom_core3d_dm_fw.frag.spv",
            "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
            "pipelineLayout": "3dpipelinelayouts://core3d_dm_fw.shaderpl",
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
                    "enableDepthTest": false,
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
                            "srcColorBlendFactor": "one",
                            "dstColorBlendFactor": "one_minus_src_alpha",
                            "colorBlendOp": "add",
                            "srcAlphaBlendFactor": "one",
                            "dstAlphaBlendFactor": "one_minus_src_alpha",
                            "alphaBlendOp": "add"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            }
        }
    ],
    "materialMetadata": [
        {
            "name": "MaterialComponent",
            "properties": {
                "textures": [
                    {
                        "name": "Shadow",
                        "displayName": "Shadow"
                    }
                ]
            },
            "customProperties": [
                {
                    "set": 4,
                    "binding": 1,
                    "data": []
                }
            ]
        }
    ]
}
