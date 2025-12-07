{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "category": "3D/Material",
    "displayName": "Reflection Plane",
    "materialMetadata": [
        {
            "name": "MaterialComponent",
            "properties": {
                "textures": [
                    {
                        "name": "baseColor",
                        "displayName":  "Base Color"
                    },
                    {
                        "name": "normalMap",
                        "displayName": "Normal Map"
                    },
                    {
                        "name": "materialMap",
                        "displayName": "Material Map"
                    },
                    {},
                    {
                        "name": "ambientOcclusion",
                        "displayName": "Ambient Occlusion"
                    },
                    {},
                    {
                        "name": "reflection",
                        "displayName": "Reflection"
                    },
                    {},
                    {},
                    {
                        "name": "transmission",
                        "displayName": "Transmission"
                    }
                ]
            }
        }
    ],
    "shaders": [
        {
            "displayName": "Reflection Plane",
            "variantName": "OPAQUE_FW",
            "renderSlot": "CORE3D_RS_DM_FW_OPAQUE",
            "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_fw_reflection_plane.frag.spv",
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
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            }
        },
        {
            "displayName": "Reflection Plane Multiview",
            "variantName": "OPAQUE_FW_MV",
            "renderSlot": "CORE3D_RS_DM_FW_OPAQUE_MV",
            "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_fw_reflection_plane.frag.spv",
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
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            }
        },
        {
            "displayName": "Reflection Plane",
            "variantName": "OPAQUE_FW_BL",
            "renderSlot": "CORE3D_RS_DM_FW_OPAQUE_BL",
            "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_fw_reflection_plane_bl.frag.spv",
            "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
            "pipelineLayout": "3dpipelinelayouts://core3d_dm_fw_bl.shaderpl",
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
