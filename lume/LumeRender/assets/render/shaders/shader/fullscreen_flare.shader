{
    "compatibility_info" : { "version" : "22.00", "type" : "shader" },
    "vert": "rendershaders://shader/fullscreen_triangle.vert.spv",
    "frag": "rendershaders://shader/fullscreen_flare.frag.spv",
    "state": {
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
                }
            ]
        }
    }
}
