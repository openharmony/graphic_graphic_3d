{
    "compatibility_info": {
        "version": "22.00",
        "type": "shader"
    },
    "category" : "Render/Effect",
    "displayName" : "Fullscreen Tonemap",
    "vert": "rendershaders://shader/fullscreen_triangle.vert.spv",
    "frag": "rendershaders://shader/fullscreen_post_process_tonemap.frag.spv",
    "state": {
        "colorBlendState": {
            "colorAttachments": [
                {
                    "colorWriteMask": "r_bit|g_bit|b_bit|a_bit"
                }
            ]
        }
    }
}