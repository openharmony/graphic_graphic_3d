{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
    "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
    "pipelineLayout": "3dpipelinelayouts://core3d_dm_fw.shaderpl",
    "frag": "test_assets://shaders/custom_core3d_dm_fw.frag.spv",
    "materialMetadata": [
        {
            "name": "MaterialComponent",
            "properties": {
                "textures": [
                    {
                        "name": "Albedo",
                        "displayName": "Albedo",
                        "defaultTextureUrl": "project://assets/huawei_logo.jpg"
                    }
                ]
            },
            "customProperties": [
                {
                    "set": 1,
                    "binding": 4,
                    "data": [
                        {
                            "name": "AlbedoFactor",
                            "displayName": "AlbedoFactor",
                            "type": "vec4",
                            "value": [
                                1,
                                1,
                                0,
                                1
                            ],
                            "displayType": "color"
                        }
                    ]
                }
            ]
        }
    ]
}