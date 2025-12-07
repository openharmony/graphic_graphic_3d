{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "variantName": "CORE3D_DM_FW",
    "renderSlot": "CORE3D_RS_DM_FW_OPAQUE",
    "renderSlotDefaultShader": false,
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
                }
            ]
        }
    },
    "materialMetadata" : [
        {
            "name" : "MaterialComponent",
            "properties" : {
                "textures" : [
                    { "name" : "normal", "displayName" : "Normal Map" },
                    { "name" : "material", "displayName" : "Material" },
                    { "name" : "baseColor", "displayName" : "Base Color" },
                    {},
                    { "name" : "emissive", "displayName" : "Emissive" }
                ]
            },
            "customProperties" : [
                {
                    "data": [
                        {
                            "name" : "u_",
                            "displayName" : "UInt",
                            "type" : "uint",
                            "value" : 7
                        },
                        {
                            "name" : "f_",
                            "displayName" : "Float",
                            "type" : "float",
                            "value" : 1.0
                        },
                        {
                            "name" : "uv4_",
                            "displayName" : "UVec4",
                            "type" : "uvec4",
                            "value" : [ 3, 4, 5, 6 ]
                        },
                        {
                            "name" : "v4_",
                            "displayName" : "Vec4",
                            "type" : "vec4",
                            "value" : [ 0.1, 0.2, 0.3, 0.4 ]
                        }
                    ]
                }
            ]
        }
    ]
}