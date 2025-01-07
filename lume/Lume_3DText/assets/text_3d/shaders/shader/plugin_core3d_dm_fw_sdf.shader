{
    "compatibility_info": {
        "version": "22.00",
            "type" : "shader"
    },
    "category" : "3D/Material",
    "materialMetadata" : [
        {
            "name" : "MaterialComponent",
                "properties" : {
                "textures" : [
                { "name" : "baseColor", "displayName" : "SDF" },
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
    ],
    "shaders": [
        {
            "displayName": "Default SDF",
            "variantName" : "OPAQUE_FW",
            "renderSlot" : "CORE3D_RS_DM_FW_OPAQUE",
            "renderSlotDefaultShader" : false,
            "vert" : "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag" : "text3dshaders://shader/plugin_core3d_dm_fw_sdf.frag.spv",
            "vertexInputDeclaration" : "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
            "pipelineLayout" : "3dpipelinelayouts://core3d_dm_fw.shaderpl",
            "state" : {
                "rasterizationState": {
                    "enableDepthClamp": false,
                        "enableDepthBias" : false,
                        "enableRasterizerDiscard" : false,
                        "polygonMode" : "fill",
                        "cullModeFlags" : "back",
                        "frontFace" : "counter_clockwise"
                },
                "depthStencilState": {
                    "enableDepthTest": true,
                        "enableDepthWrite" : true,
                        "enableDepthBoundsTest" : false,
                        "enableStencilTest" : false,
                        "depthCompareOp" : "less_or_equal"
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
            "displayName": "Default Translucent SDF",
            "variantName" : "TRANSLUCENT_FW",
            "renderSlot" : "CORE3D_RS_DM_FW_TRANSLUCENT",
            "renderSlotDefaultShader" : false,
            "vert" : "3dshaders://shader/core3d_dm_fw.vert.spv",
            "frag" : "text3dshaders://shader/plugin_core3d_dm_fw_sdf.frag.spv",
            "vertexInputDeclaration" : "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
            "pipelineLayout" : "3dpipelinelayouts://core3d_dm_fw.shaderpl",
            "state" : {
                "rasterizationState": {
                    "enableDepthClamp": false,
                    "enableDepthBias" : false,
                    "enableRasterizerDiscard" : false,
                    "polygonMode" : "fill",
                    "cullModeFlags" : "back",
                    "frontFace" : "counter_clockwise"
                },
                "depthStencilState": {
                    "enableDepthTest": true,
                    "enableDepthWrite" : false,
                    "enableDepthBoundsTest" : false,
                    "enableStencilTest" : false,
                    "depthCompareOp" : "less_or_equal"
                },
                "colorBlendState": {
                    "colorAttachments": [
                        {
                            "enableBlend": true,
                            "colorWriteMask" : "r_bit|g_bit|b_bit|a_bit",
                            "srcColorBlendFactor" : "one",
                            "dstColorBlendFactor" : "one_minus_src_alpha",
                            "colorBlendOp" : "add",
                            "srcAlphaBlendFactor" : "one",
                            "dstAlphaBlendFactor" : "one_minus_src_alpha",
                            "alphaBlendOp" : "add"
                        },
                        {
                            "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                        }
                    ]
                }
            }
        },
        {
            "displayName": "Default Multiview SDF",
            "variantName" : "OPAQUE_FW_MV",
            "renderSlot" : "CORE3D_RS_DM_FW_OPAQUE_MV",
            "renderSlotDefaultShader" : false,
            "vert" : "3dshaders://shader/core3d_dm_fw_mv.vert.spv",
            "frag" : "text3dshaders://shader/plugin_core3d_dm_fw_sdf.frag.spv",
            "vertexInputDeclaration" : "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
            "pipelineLayout" : "3dpipelinelayouts://core3d_dm_fw.shaderpl",
            "state" : {
                "rasterizationState": {
                    "enableDepthClamp": false,
                    "enableDepthBias" : false,
                    "enableRasterizerDiscard" : false,
                    "polygonMode" : "fill",
                    "cullModeFlags" : "back",
                    "frontFace" : "counter_clockwise"
                },
                "depthStencilState": {
                    "enableDepthTest": true,
                    "enableDepthWrite" : true,
                    "enableDepthBoundsTest" : false,
                    "enableStencilTest" : false,
                    "depthCompareOp" : "less_or_equal"
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
