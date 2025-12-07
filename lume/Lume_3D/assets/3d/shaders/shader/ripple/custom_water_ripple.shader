{
	"compatibility_info": {
		"version": "22.00",
		"type": "shader"
	},
	"materialMetadata": [
		{
			"name": "MaterialComponent",
			"properties": {
				"textures": [
					{
						"name": "baseColor",
						"displayName": "Base Color"
					},
					{
						"name": "normalMap",
						"displayName": "Normal Map"
					},
					{
						"name": "materialMap",
						"displayName": "Material Map"
					},
					{
						"name": "waterDuDv",
						"displayName": "Water DuDv"
					},
					{},
					{},
					{
						"name": "reflection",
						"displayName": "Reflection"
					},
					{},
					{
						"name": "rippleTex",
						"displayName": "Ripple Texture"
					},
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
			"variantName": "Default",

			"vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
			"frag": "3dshaders://shader/ripple/custom_water_ripple.frag.spv",
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
							" colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
						}
					]
				}
			}
		},
		{
			"variantName": "Bindless",

			"vert": "3dshaders://shader/core3d_dm_fw.vert.spv",
			"frag": "3dshaders://shader/ripple/custom_water_ripple_bl.frag.spv",
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