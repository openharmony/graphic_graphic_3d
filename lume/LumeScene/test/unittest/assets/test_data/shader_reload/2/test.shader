{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
    "vertexInputDeclaration": "3dvertexinputdeclarations://core3d_dm_fw.shadervid",
    "pipelineLayout": "3dpipelinelayouts://core3d_dm_fw.shaderpl",
    "frag": "file://shader_reload/custom_core3d_dm_fw.frag.spv",
    "category": "Shader Graphs",
    "materialMetadata": [
        {
            "name": "MaterialComponent",
            "properties": {
                "textures": [
                    {
                        "name": "texture_1",
                        "displayName": "texture_1"
                    }
                ]
            },
            "customProperties": [
                {
                    "data": [
						{
                            "name" : "var1",
                            "displayName" : "var1",
                            "type" : "uint",
                            "value" : 1
						},
						{
                            "name" : "var2",
                            "displayName" : "var2",
                            "type" : "uint",
                            "value" : 2
						}
					]
                }
            ]
        }
    ]
}