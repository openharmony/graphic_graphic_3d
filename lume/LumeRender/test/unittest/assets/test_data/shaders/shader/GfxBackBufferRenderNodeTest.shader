{
    "compatibility_info" : { "version" : "22.00", "type" : "shader" },
    "vert": "rendershaders://shader/fullscreen_triangle.vert.spv",
    "frag": "rendershaders://shader/GfxBackBufferRenderNodeTest.frag.spv",
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
            "name": "ShaderPipelineBinder",
            "customProperties": [
                {
                    "set": 0,
                    "binding": 0,
                    "data": [
                        {
                            "name": "param_0",
                            "displayName": "Param 0",
                            "type": "float",
                            "value": 0.0
                        },
                        {
                            "name": "param_1",
                            "displayName": "Param 1",
                            "type": "vec2",
                            "value": [1.0, 1.0]
                        }
                    ]
                }
            ],
            "bindingProperties": [
                {
                    "data": [
                        {
                            "name": "uImage",
                            "displayName": "Image",
                            "type": "image",
                            "set": 0,
                            "binding": 1
                        },
                        {
                            "name": "uSampler",
                            "displayName": "Sampler",
                            "type": "sampler",
                            "set": 0,
                            "binding": 2
                        },
                        {
                            "name": "uBuffer",
                            "displayName": "Buffer",
                            "type": "buffer",
                            "set": 0,
                            "binding": 3
                        }
                    ]
                }
            ]
        }
    ]
}