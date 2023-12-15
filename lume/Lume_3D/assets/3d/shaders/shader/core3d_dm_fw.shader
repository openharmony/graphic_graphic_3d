{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "category": "3D/Material",
    "baseShader": "3dshaders://shader/core3d_dm_fw.shader",
    "shaders": [
        {
            "displayName": "Default",
            "variantName": "OPAQUE_FW",
            "renderSlot": "CORE3D_RS_DM_FW_OPAQUE",
            "renderSlotDefaultShader": true,
            "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_fw.frag.spv",
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
            },
            "materialMetadata" : [
                {
                    "name" : "MaterialComponent",
                    "properties" : {
                        "textures" : [
                            { "name" : "baseColor", "displayName" : "Base Color" },
                            { "name" : "normal", "displayName" : "Normal Map" },
                            { "name" : "material", "displayName" : "Material" },
                            { "name" : "emissive", "displayName" : "Emissive" },
                            { "name" : "ambientOcclusion", "displayName" : "Ambient Occlusion" },
                            { "name" : "clearcoat", "displayName" : "Clearcoat" },
                            { "name" : "clearcoatRoughness", "displayName" : "Clearcoat Roughness" },
                            { "name" : "clearcoatNormal", "displayName" : "Clearcoat Normal" },
                            { "name" : "sheen", "displayName" : "Sheen" },
                            { "name" : "transmission", "displayName" : "Transmission" },
                            { "name" : "specular", "displayName" : "Specular" }
                        ]
                    }
                }
            ]
        },
        {
            "displayName": "Default Translucent",
            "variantName": "TRANSLUCENT_FW",
            "renderSlot": "CORE3D_RS_DM_FW_TRANSLUCENT",
            "renderSlotDefaultShader": true,
            "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_fw.frag.spv",
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
                    "enableDepthWrite": false,
                    "enableDepthBoundsTest": false,
                    "enableStencilTest": false,
                    "depthCompareOp": "less_or_equal"
                },
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
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            },
            "materialMetadata" : [
                {
                    "name" : "MaterialComponent",
                    "properties" : {
                        "textures" : [
                            { "name" : "baseColor", "displayName" : "Base Color" },
                            { "name" : "normal", "displayName" : "Normal Map" },
                            { "name" : "material", "displayName" : "Material" },
                            { "name" : "emissive", "displayName" : "Emissive" },
                            { "name" : "ambientOcclusion", "displayName" : "Ambient Occlusion" },
                            { "name" : "clearcoat", "displayName" : "Clearcoat" },
                            { "name" : "clearcoatRoughness", "displayName" : "Clearcoat Roughness" },
                            { "name" : "clearcoatNormal", "displayName" : "Clearcoat Normal" },
                            { "name" : "sheen", "displayName" : "Sheen" },
                            { "name" : "transmission", "displayName" : "Transmission" },
                            { "name" : "specular", "displayName" : "Specular" }
                        ]
                    }
                }
            ]
        },
        {
            "displayName": "Default Deferred",
            "variantName": "OPAQUE_DF",
            "renderSlot": "CORE3D_RS_DM_DF_OPAQUE",
            "renderSlotDefaultShader": true,
            "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_df.frag.spv",
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
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            },
            "materialMetadata": [
                {
                    "name": "MaterialComponent",
                    "properties": {
                        "textures": [
                            { "name": "baseColor", "displayName": "Base Color" },
                            { "name": "normal", "displayName": "Normal Map" },
                            { "name": "material", "displayName": "Material" },
                            { "name": "emissive", "displayName": "Emissive" },
                            { "name": "ambientOcclusion", "displayName": "Ambient Occlusion" },
                            { "name": "clearcoat", "displayName": "Clearcoat" },
                            { "name": "clearcoatRoughness", "displayName": "Clearcoat Roughness" },
                            { "name": "clearcoatNormal", "displayName": "Clearcoat Normal" },
                            { "name": "sheen", "displayName": "Sheen" },
                            { "name": "transmission", "displayName": "Transmission" },
                            { "name": "specular", "displayName": "Specular" }
                        ]
                    }
                }
            ]
        },
        {
            "displayName": "Default Multiview",
            "variantName": "OPAQUE_FW_MV",
            "renderSlot": "CORE3D_RS_DM_FW_OPAQUE_MV",
            "renderSlotDefaultShader": true,
            "vert": "3dshaders://shader/core3d_dm_fw_mv.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_fw.frag.spv",
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
            },
            "materialMetadata" : [
                {
                    "name" : "MaterialComponent",
                    "properties" : {
                        "textures" : [
                            { "name" : "baseColor", "displayName" : "Base Color" },
                            { "name" : "normal", "displayName" : "Normal Map" },
                            { "name" : "material", "displayName" : "Material" },
                            { "name" : "emissive", "displayName" : "Emissive" },
                            { "name" : "ambientOcclusion", "displayName" : "Ambient Occlusion" },
                            { "name" : "clearcoat", "displayName" : "Clearcoat" },
                            { "name" : "clearcoatRoughness", "displayName" : "Clearcoat Roughness" },
                            { "name" : "clearcoatNormal", "displayName" : "Clearcoat Normal" },
                            { "name" : "sheen", "displayName" : "Sheen" },
                            { "name" : "transmission", "displayName" : "Transmission" },
                            { "name" : "specular", "displayName" : "Specular" }
                        ]
                    }
                }
            ]
        },
        {
            "displayName": "Default Deferred Multiview",
            "variantName": "OPAQUE_DF_MV",
            "renderSlot": "CORE3D_RS_DM_DF_OPAQUE_MV",
            "renderSlotDefaultShader": true,
            "vert": "3dshaders://shader/core3d_dm_fw_mv.vert.spv",
            "frag": "3dshaders://shader/core3d_dm_df.frag.spv",
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
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            },
            "materialMetadata": [
                {
                    "name": "MaterialComponent",
                    "properties": {
                        "textures": [
                            { "name": "baseColor", "displayName": "Base Color" },
                            { "name": "normal", "displayName": "Normal Map" },
                            { "name": "material", "displayName": "Material" },
                            { "name": "emissive", "displayName": "Emissive" },
                            { "name": "ambientOcclusion", "displayName": "Ambient Occlusion" },
                            { "name": "clearcoat", "displayName": "Clearcoat" },
                            { "name": "clearcoatRoughness", "displayName": "Clearcoat Roughness" },
                            { "name": "clearcoatNormal", "displayName": "Clearcoat Normal" },
                            { "name": "sheen", "displayName": "Sheen" },
                            { "name": "transmission", "displayName": "Transmission" },
                            { "name": "specular", "displayName": "Specular" }
                        ]
                    }
                }
            ]
        }
    ]
}