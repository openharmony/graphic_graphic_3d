{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "category":"3D",
    "displayName": "Fullscreen Deferred Shading",
    "vert": "rendershaders://shader/fullscreen_triangle.vert.spv",
    "frag": "3dshaders://shader/core3d_dm_fullscreen_deferred_shading.frag.spv",
    "pipelineLayout": "3dpipelinelayouts://core3d_dm_fullscreen_deferred_shading.shaderpl",
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
                    "colorWriteMask": "r_bit|g_bit|b_bit|a_bit",
                    "enableBlend": true,
                    "srcColorBlendFactor": "one",
                    "dstColorBlendFactor": "one",
                    "colorBlendOp": "add",
                    "srcAlphaBlendFactor": "one",
                    "dstAlphaBlendFactor": "zero",
                    "alphaBlendOp": "add"
                }
            ]
        }
    }
}
