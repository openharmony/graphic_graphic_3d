{
    "compatibility_info" : { "version" : "22.00", "type" : "shader" },
    "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
    "frag": "3dshaders://shader/core3d_dm_fw.frag.spv",
    "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
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
            "enableDepthTest": true,
            "enableDepthWrite": true,
            "enableDepthBoundsTest": false,
            "enableStencilTest": false,
            "depthCompareOp": "less_or_equal"
        },
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
            "name": "MaterialComponent",
            "customProperties": [
                {
                    "set": 2,
                    "binding": 10,
                    "data": [
                        {
                            "name": "test",
                            "displayName": "Vec4",
                            "type": "vec4",
                            "value": [ 0.6, 0.7, 0.8, 0.9 ]
                        }
                    ]
                }
            ]
        }
    ]
}