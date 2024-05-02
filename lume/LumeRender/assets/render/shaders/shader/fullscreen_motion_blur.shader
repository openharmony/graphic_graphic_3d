{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "category" : "Render/Effect",
    "displayName" : "Motion Blur",
    "vert": "rendershaders://shader/fullscreen_triangle.vert.spv",
    "frag": "rendershaders://shader/fullscreen_motion_blur.frag.spv",
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
            "name": "RenderDataStorePostProcess",
            "globalFactor": [
                {
                    "name": "configuration",
                    "displayName": "Configuration",
                    "type": "vec4",
                    "value": [2.0, 1.0, 1.0, 1.0]
                }
            ]
        },
        {
            "name": "RenderDataStorePostProcess",
            "customProperties": [
                {
                    "data": [
                        {
                            "name": "configuration",
                            "displayName": "Configuration (sharpness, quality, alpha, velocity coefficient)",
                            "type": "vec4",
                            "value": [2.0, 1.0, 1.0, 1.0]
                        }
                    ]
                }
            ]
        }
    ]
}