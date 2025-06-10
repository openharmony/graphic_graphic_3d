{
    "compatibility_info" : { "version" : "22.00", "type" : "shader" },
    "vert" : "dotfieldshaders://shader/default_material_dotfield.vert.spv",
    "frag" : "dotfieldshaders://shader/default_material_dotfield.frag.spv",
    "slot" : "CORE3D_RS_DM_FW_TRANSLUCENT",
    "vertexInputDeclaration": "dotfieldvertexinputdeclarations://default_material_dotfield.shadervid",
    "pipelineLayout" : "dotfieldpipelinelayouts://default_material_dotfield.shaderpl",
    "state": {
        "inputAssembly": {
            "enablePrimitiveRestart": false,
            "primitiveTopology": "point_list"
        },
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
                    "dstAlphaBlendFactor": "one_minus_src_alpha",
                    "alphaBlendOp": "add"
                }
            ]
        }
    }
}