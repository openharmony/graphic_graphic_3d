{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "vert": "rendershaders://shader/fullscreen_triangle.vert.spv",
    "frag": "3dshaders://shader/clouds/cloud_volumes.frag.spv",
    "state": {
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