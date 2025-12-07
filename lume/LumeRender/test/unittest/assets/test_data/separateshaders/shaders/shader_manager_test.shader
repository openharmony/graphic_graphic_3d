{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "renderSlot": "shader_manager_test_render_slot",
    "vert": "test_shaders://shader_manager_test.vert.spv",
    "frag": "test_shaders://shader_manager_test.frag.spv",
    "state": {
        "rasterizationState": {
            "enableRasterizerDiscard": false
        },
        "colorBlendState": {
            "colorAttachments": [
                {
                    "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                }
            ]
        }
    },
    "stateFlags": "input_assembly_bit|depth_stencil_state_bit"
}