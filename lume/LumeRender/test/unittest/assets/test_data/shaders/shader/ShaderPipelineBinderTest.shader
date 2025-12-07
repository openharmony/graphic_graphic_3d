{
    "compatibility_info" : { "version" : "22.00", "type" : "shader" },
    "vert": "rendershaders://shader/fullscreen_triangle.vert.spv",
    "frag": "rendershaders://shader/ShaderPipelineBinderTest.frag.spv",
    "state": {
        "colorBlendState": {
            "colorAttachments": [
                {
                    "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                }
            ]
        }
    },
    "materialMetadata": [
        {
            "bindingProperties": [
                {
                    "data": [
                        {
                            "name": "uSampler",
                            "displayName": "SAMPLER",
                            "type": "sampler",
                            "set": 0,
                            "binding": 0
                        },
                        {
                            "name": "uTex",
                            "displayName": "TEX",
                            "type": "image",
                            "set": 0,
                            "binding": 1
                        }
                    ]
                }
            ]
        }
    ]
}
