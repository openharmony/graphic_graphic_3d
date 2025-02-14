{
    "compatibility_info" : {
        "version" : "22.00",
        "type" : "shader"
    },
    "category": "3D",
    "displayName": "Environment",
    "baseShader": "campreviewshaders://shader/camera_stream.shader",
    "shaders": [
        {
            "displayName": "Default Environment",
            "variantName": "ENV",
            "vert": "campreviewshaders://shader/camera_stream.vert.spv",
            "frag": "campreviewshaders://shader/camera_stream.frag.spv",
            "pipelineLayout": "campreviewpipelinelayouts://camera_stream.shaderpl",
            "renderSlot": "CORE3D_RS_DM_ENV",
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
            "vert": "campreviewshaders://shader/camera_stream.vert.spv",
            "frag": "campreviewshaders://shader/camera_stream.frag.spv",
            "pipelineLayout": "campreviewpipelinelayouts://camera_stream.shaderpl",
            "renderSlot": "CORE3D_RS_DM_ENV",
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
